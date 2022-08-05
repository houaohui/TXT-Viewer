/*
 *@author: HAH (BiliBili: iccery)
 */
#ifndef _CMD_PARSER_H_
#define _CMD_PARSER_H_

#include "sys.h"
#include "stdbool.h"


// void my_func(PCBNode_t *pcb, int argc, char argv[][COMMAND_MAX_ARG_LEN]);
#define MY_SETUP        if(pcb->state == SETUP) { \
	
#define SEUP_END     }

#define MY_LOOP         pcb->state = LOOP;  \
						if(pcb->state == LOOP) { \
							
#define LOOP_END     }

#define PARSER_STR_BUF_SIZE  50
#define COMMAND_MAX_ARGC 5
#define COMMAND_MAX_ARG_LEN 10


typedef enum
{
	COMMAND_WAIT,
	COMMAND_START,
	COMMAND_RUNING,
	COMMAND_CLOSE
} cmd_state_t;

typedef enum
{
	BLOCKING,
	SETUP,
	LOOP,
} process_state_t;

typedef struct PCBNode PCBNode_t;
typedef void (*pcb_main_f)(PCBNode_t *pcb, int argc, char argv[][COMMAND_MAX_ARG_LEN]);


struct PCBNode {
	process_state_t state;
	char *name;
	pcb_main_f main;
	u8 pid;
	u8 argc;
	char argv[COMMAND_MAX_ARGC][COMMAND_MAX_ARG_LEN];
	struct PCBNode *next;
};



typedef struct cmd_Parser cmd_Parser_t;

struct cmd_Parser{
	
	char *name;
	u8 updata;//是否得到新字符
	
	//cmd状态
	cmd_state_t state;
	
	u8 echo_enable;//回显
	
	//进程数
	u8 pid_cnt;
	
	//新字符
	u8 getchar;
	//解析字符索引
	u8 str_idx;
	
	//pcb_head也用来存储前台程序
	PCBNode_t pcb_head;
	char cmd_strbuf[PARSER_STR_BUF_SIZE];
	void (*run)(cmd_Parser_t *cmd_parser);
	
	//接口，需要实现这些函数与显示器交互，这里显示器是txtViewer第三个模式
	//打印一个字节
	void (*print_oneChar)(cmd_Parser_t *cmd_parser, char str);
	//退格操作
	void (*Backspace)(cmd_Parser_t *cmd_parser);
	//格式化打印字符串到显示器
	void (*printf)(char *fmt, ...);
};

extern cmd_Parser_t cmd_parser;

void cmd_getChar(cmd_Parser_t *cmd_parser, char data);

void cmd_Parser_Init(cmd_Parser_t *cmd_parser, char* name);
void cmd_Parser_Start(cmd_Parser_t *cmd_parser);
void cmd_Parser_Close(cmd_Parser_t *cmd_parser);
void cmd_RegisterFunc(cmd_Parser_t *cmd_parser, PCBNode_t *pcb, char* name , pcb_main_f func);


//test
void cmd_test_init(void);

#endif


