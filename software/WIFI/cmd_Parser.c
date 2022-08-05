/*
 *@author: HAH (BiliBili: iccery)
 */
#include "common.h"
#include "cmd_parser.h"
#include "TXT_Viewer.h"

#define COMMAND_ARG_ERROR 1
#define COMMAND_NOT_FOUND 2
#define COMMAND_OK 0

static char * operator_buf = "\n\b";



static void cmd_set_process_state(cmd_Parser_t *cmd_parser,u8 pid, process_state_t p_state);
static u8 proc_command(cmd_Parser_t *cmd_parser, char *command);
static void cmd_run(cmd_Parser_t *cmd_parser);


////////////////////////////接口///////////////////////////////
//退格操作
static void backspace(cmd_Parser_t *cmd_parser)
{
	txtViewer_DeleteLastStoreChar(&txtViewer);
}
//打印一个字节
static void print_oneChar(cmd_Parser_t *cmd_parser, char str)
{
	txtViewer.printf(&txtViewer,"%c",str);
}
extern char firststr_buff[512];
//格式化打印字符串到显示器
static void cmd_printf(char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	vsprintf(firststr_buff,fmt,ap);
	va_end(ap);
	txtViewer.printf(&txtViewer,firststr_buff);
}
///////////////////////////////////////////////////////////////

void cmd_RegisterFunc(cmd_Parser_t *cmd_parser, PCBNode_t *pcb, char* name , pcb_main_f func)
{
	//分配pid
	pcb->pid = ++(cmd_parser->pid_cnt);
	if(cmd_parser->pid_cnt == 0)
		return;
	pcb->name = name;
	pcb->main = func;
	pcb->state = BLOCKING;
	pcb->next = cmd_parser->pcb_head.next;
	cmd_parser->pcb_head.next = pcb;
}

void cmd_getChar(cmd_Parser_t *cmd_parser, char data)
{
	if(cmd_parser->state == COMMAND_CLOSE)
		return;
	if(!cmd_parser->updata) {
		cmd_parser->getchar = data;
		cmd_parser->updata = true;
	}
}

void cmd_log_first(cmd_Parser_t *cmd_parser)
{
	cmd_parser->printf("%s",cmd_parser->name);
}

//初始化CMD
void cmd_Parser_Init(cmd_Parser_t *cmd_parser, char* name)
{
	cmd_parser->pid_cnt = 0;
	cmd_parser->name = name;
	cmd_parser->updata = false;
	cmd_parser->state = COMMAND_WAIT;
	cmd_parser->pcb_head.next=NULL;
	cmd_parser->pcb_head.name="head";
	cmd_parser->pcb_head.main=NULL;
	cmd_parser->str_idx =0;
	cmd_parser->print_oneChar = print_oneChar;
	cmd_parser->Backspace = backspace;
	cmd_parser->run = cmd_run;
	cmd_parser->printf = cmd_printf;
	cmd_parser->echo_enable = true;
}

//打开CMD
void cmd_Parser_Start(cmd_Parser_t *cmd_parser)
{
	cmd_parser->state = COMMAND_START;
}

//关闭CMD
void cmd_Parser_Close(cmd_Parser_t *cmd_parser)
{
	PCBNode_t *p = &cmd_parser->pcb_head;
	//关闭所有进程
	while(p != NULL) {
		p->state = BLOCKING;
		p=p->next;
	}
	cmd_parser->pcb_head.pid = NULL;
	cmd_parser->pcb_head.main = NULL;
	//清除缓存
	cmd_parser->str_idx =0;
	cmd_parser->state = COMMAND_CLOSE;
}



//把字符添加到字符缓存
void cmd_addOneChar(cmd_Parser_t *cmd_parser)
{
	if(cmd_parser->str_idx == PARSER_STR_BUF_SIZE-1)
		return;
	cmd_parser->cmd_strbuf[cmd_parser->str_idx++] = cmd_parser->getchar;
	cmd_parser->cmd_strbuf[cmd_parser->str_idx] = '\0';
}
void cmd_delOneChar(cmd_Parser_t *cmd_parser)
{
	if(cmd_parser->str_idx == 0)
		return;
	cmd_parser->cmd_strbuf[--cmd_parser->str_idx] = '\0';
}

//判断是不是操作符
static u8 is_operator(cmd_Parser_t *cmd_parser)
{
	char *opt_buf = operator_buf;
	while(*opt_buf != '\0')
	{
		if(cmd_parser->getchar == *opt_buf++)
			return true;
	}
	return false;
}
//执行操作符对应的操作
void cmd_do_operator(cmd_Parser_t *cmd_parser)
{
	//启动和终止前台程序,前台程序将存储在pcb_head
	if(cmd_parser->getchar == '\n')
	{
		//终止这个程序
		if(cmd_parser->pcb_head.state != BLOCKING) {
			cmd_parser->printf("exit\n");
			cmd_set_process_state(cmd_parser, cmd_parser->pcb_head.pid, BLOCKING);
			cmd_parser->pcb_head.pid = NULL;
			cmd_parser->pcb_head.main = NULL;
			
		} else {
			//执行程序
			u8 ret = proc_command(cmd_parser, cmd_parser->cmd_strbuf);
			if(ret == COMMAND_ARG_ERROR) cmd_parser->printf("args err\n");
			if(ret == COMMAND_NOT_FOUND) cmd_parser->printf(" no find\n");
			if(ret == COMMAND_OK) cmd_parser->printf(" find\n");
			
		}
		//清除命令行缓存
		cmd_parser->str_idx =0;
		//重新开始解析
		cmd_parser->state = COMMAND_START;
		
	} else if(cmd_parser->getchar == '\b') {
		//不能调顺序
		//to display buf
		if(cmd_parser->echo_enable) {
			if(cmd_parser->str_idx != 0)
			{
				cmd_parser->Backspace(cmd_parser);
			}
		}
		//to cmd string buf
		cmd_delOneChar(cmd_parser);
		
	}
}

void cmd_running_process(cmd_Parser_t *cmd_parser)
{
	PCBNode_t *p = &cmd_parser->pcb_head;

	while(p != NULL) {
		if(p->state != BLOCKING) {
			//运行一次
			if(p->main != NULL)
				p->main(p,p->argc,p->argv);
			//阻塞SETUP
			if(p->state == SETUP) {
				
				p->state = BLOCKING;
				//前台程序特殊处理，如果前台程序进入BLOCKING,能立即进入控制台
				if(p == &cmd_parser->pcb_head)
				{
					cmd_parser->state = COMMAND_START;
				}
			}
		}
		p=p->next;
	}
}

static void cmd_set_process_state(cmd_Parser_t *cmd_parser,u8 pid, process_state_t p_state)
{
	PCBNode_t *p = &cmd_parser->pcb_head;
	while(p != NULL) {
		if(p->pid == pid) {
			p->state = p_state;
			return;
		}
		p=p->next;
	}
}




static void cmd_run(cmd_Parser_t *cmd_parser)
{
	if(cmd_parser->state == COMMAND_START)
	{
		cmd_parser->state = COMMAND_RUNING;
		cmd_log_first(cmd_parser);
	}
	
	else if(cmd_parser->state == COMMAND_RUNING)
	{
		if(cmd_parser->updata)
		{
			
			cmd_parser->updata = false;
			if(is_operator(cmd_parser)) {
				cmd_do_operator(cmd_parser);
				
			} else {
				//不能调顺序
				if(cmd_parser->echo_enable) {
					// to display buf
					if(cmd_parser->str_idx != PARSER_STR_BUF_SIZE-1)
					{
						cmd_parser->print_oneChar(cmd_parser,cmd_parser->getchar);
					}
				}
				// to cmd string buf
				cmd_addOneChar(cmd_parser);
				
			}
			
		}
	}
	
	if(cmd_parser->state != COMMAND_CLOSE)
		cmd_running_process(cmd_parser);
	
}

u8 mystrcmp(char * str1, char * str2)
{
	while(*str1 != '\0' && *str2 != '\0')
	{
		if(*str1 == *str2)
		{
			str1++;
			str2++;
		}
		else 
		{
			return 1;
		}
	}
	if(*str1 == '\0' && *str2 == '\0')
		return 0;
	else
		return 1;
}

//解析执行
static u8 proc_command(cmd_Parser_t *cmd_parser, char *command)
{
	u8 args[COMMAND_MAX_ARGC][COMMAND_MAX_ARG_LEN]={'\0'};
	u8 arg_pos = 0, arg_str_pos = 0, tmp ,background=0;
	
	tmp = *(command++);
	while(tmp != '\0')
	{
		if(tmp == '&')
		{
			background=1;
		} 
		else if(tmp == ' ')
		{
			args[arg_pos][arg_str_pos] = '\0';
			if(arg_str_pos != 0)
			{
				arg_pos++;
				if(arg_pos == COMMAND_MAX_ARGC)
				{
					return COMMAND_ARG_ERROR;
				}
			}
			arg_str_pos = 0;
		}
		else
		{
			args[arg_pos][arg_str_pos++] = tmp;
			if(arg_str_pos == COMMAND_MAX_ARG_LEN)
			{
				return COMMAND_ARG_ERROR;
			}
		}
		tmp = *(command++);
	}
	args[arg_pos][arg_str_pos] = '\0';
	
	cmd_parser->printf(" %s",args[0]);
	PCBNode_t *p=cmd_parser->pcb_head.next;
	while(p != NULL) {
		if(mystrcmp((char *)args[0], p->name) == 0) {

			//后台运行
			if(background)
			{
				//copy参数
				p->argc = arg_pos+1;
				p->state = SETUP;
				for(u8 i=0; i<COMMAND_MAX_ARGC; i++)
				{
					for(u8 j=0; j<COMMAND_MAX_ARG_LEN; j++)
					{
						p->argv[i][j] = args[i][j];
					}
				}
				
			}
			else//默认前台运行
			{
				p->state = BLOCKING;
				cmd_parser->pcb_head.main = p->main;
				cmd_parser->pcb_head.pid = p->pid;
				cmd_parser->pcb_head.state = SETUP;
				cmd_parser->pcb_head.argc = arg_pos+1;
				
				for(u8 i=0; i<COMMAND_MAX_ARGC; i++)
				{
					for(u8 j=0; j<COMMAND_MAX_ARG_LEN; j++)
					{
						cmd_parser->pcb_head.argv[i][j] = args[i][j];
					}
				}
			}
			
			return COMMAND_OK;
		}
		p=p->next;
	}
	
	return COMMAND_NOT_FOUND;
}



/////////////////////////cmd usage//////////////////////////////

cmd_Parser_t cmd_parser;


PCBNode_t ps_pcb;

void ps(PCBNode_t *pcb, int argc, char argv[][COMMAND_MAX_ARG_LEN])
{
	PCBNode_t *p = &cmd_parser.pcb_head;
MY_SETUP
	txtViewer.printf(&txtViewer, "%d\n",argc);
	txtViewer.printf(&txtViewer, "%s:\n",argv[0]);
	txtViewer.printf(&txtViewer, "%s:\n",argv[1]);
	txtViewer.printf(&txtViewer, "%s:\n",argv[2]);
	txtViewer.printf(&txtViewer, "|pid:|sta:|name:\n");
	while(p != NULL) {
		txtViewer.printf(&txtViewer, "|%4d|%4d|%s|\n",p->pid,p->state,p->name);
		p=p->next;
	}
		
SEUP_END
	
MY_LOOP
		static u8 i=0;
		static millis8_t lastUpdate;
		millis8_t now = millis();
		if ((millis8_t)(now - lastUpdate) >= 1000) {
			lastUpdate = now;
			txtViewer.printf(&txtViewer,"%d",i++%10);
		}
LOOP_END

}

PCBNode_t kill_pcb;

void kill(PCBNode_t *pcb, int argc, char argv[][COMMAND_MAX_ARG_LEN])
{
	
MY_SETUP
	int pid = atoi(argv[1]);
	txtViewer.printf(&txtViewer, "%d\n",argc);
	txtViewer.printf(&txtViewer, "%s:\n",argv[0]);
	txtViewer.printf(&txtViewer, "%s:\n",argv[1]);
	txtViewer.printf(&txtViewer, "pid:%d\n",pid);
	cmd_set_process_state(&cmd_parser,pid ,BLOCKING);
		
SEUP_END

}
void cmd_test_init()
{
	cmd_Parser_Init(&cmd_parser,"\nCMD>");
	cmd_RegisterFunc(&cmd_parser, &ps_pcb, "ps", ps);
	cmd_RegisterFunc(&cmd_parser, &kill_pcb, "kill", kill);
}



