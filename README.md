```
based on nwatch

@演示：如果字符缓冲区是:char *buf = "he\rllo,th\ris is a test.\ncan you see something here?\nIf you can see some words\n
                                     This test may have succeeded!\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"

               3种解析样式将会是这样（假设过滤'\r'）:

       NORMAL              AUTO_BREAKLINE           CONSOLE_PRINT
 +----------------+      +----------------+      +----------------+ 
 |hello，this is a|      |hello，this is a|      |5               |
 |can you see some|      |test. can you se|      |6               |
 |If you can some |      |e something here|      |7               |
 |This test may ha|      |If you can see s|      |8               |
 |1               |      |ome words       |      |9               |
 |2               |      |This test may ha|      |10              |
 +----------------+      +----------------+      +----------------+ 
@功能：
    1，3种解析器           2，CONSOLE_PRINT滚动线性动画
    3，输入窗口跟踪        4，窗口设置大小
    5，窗口设置位置        6，窗口分割字符
    7，键盘输入模式        8，改变字符缓冲源
    9，窗口跟踪线性动画    10，特殊字符过滤
    11，清空字符缓存       12，初始化打印
    13，全局动画开关
@机制：
       程序采用循环存储，以换行符结尾的一段连续字符视为一个‘换行行’，当存储位置大于显示开始解析位置即认为空间需要覆盖，会在存储位置处往下找
       新的‘换行行’，新的开始解析位置将会是这个‘换行行’的第一个字符的位置

@详情：
    1，keyboard_mode与默认模式
       keyboard_mode会立即处理换行，适合使用键盘给缓冲区输入，遇到换行会立即在最下面产生空白的新行，而默认模式是不会进行多余的空白显示的
       如果缓冲区最后一个是'\n'，keyboard_mode会显示一个空白行，而默认模式不会显示
    2，filterOut_char与默认模式
       使能filterOut_char会过滤掉字符缓冲区中与filter_buf一样的字符，只过滤显示，不过滤存储
    3，scroll_enable
       使能scroll_enable会导致NORMAL和AUTO_BREAKLINE解析器窗口输入跟踪，你输入的字符始终在窗口内，不会看不到输入字符。也会导致CONSOLE_PRINT
       解析器跟踪最新的log
    4，scroll_anim
       使能scroll_anim会导致窗口跟踪产生动画（NORMAL，AUTOBREAKLINE）或滚动动画(CONSOLE_PRINT)开启平滑过渡的线性动画，滚动会根据未滚动的行数
       线性增加滚动速度，窗口跟踪也是同样道理。三种解析器产生的动画原理不同，前两种依靠窗口跟踪输入位置不同产生动画偏移，CONSOLE_PRINT是根据
       txtViewer_printf计算process_cnt什么时候产生新行，继而偏移scroll_pixel，只要有新行就有动画，CONSOLE_PRINT模式设计就是为了命令行输出模式，
       使用光标随意输入就违背了它的设计原则，因为光标模式输入不会偏移scroll_pixel产生新行动画，同时导致process_cnt计算混乱，新行就会计算错误，
       就会导致动画错乱，因此CONSOLE_PRINT模式下禁止光标输入，只用printf.
    5，loop_show
       因为在初始化时没有定时更新函数，要想实现初始化打印操作，应改首先不使能该变量
    6，scroll_pixel
       CONSOLE_PRINT解析器的历史行查看偏移量，scroll_pixel越大，向上滚动的深度越大，查看的历史行越多
    7，txt_ofsetX
       NORMAL解析器的相对于文本的X偏移量
    8，txt_ofsetY
       NORMAL和AUTOBREAKLINE解析器的相对于文本的Y偏移量
    9，window_x,window_y,window_w,window_h
       以次为窗口左上顶点的x坐标，窗口左上顶点的y坐标，窗口宽度，窗口高度
    10，char_h，char_w
       字体的高度和宽度
    11，cursor_enable
        光标模式会显示当前光标的位置，存储和删除字符光标位置，需要很大性能

@移植依赖：
       本程序的最底层只依赖单个字体的高度和宽度，如果想要支持窗口分割，就必须重新实现依赖不同显示屏的的mydraw_bitmap函数，单色屏幕实现起来较为
       复杂，因为每个数据bit就是一个像素点，在写数据时需要时刻注意窗口位置，写数据位置，窗口大小的关系会经常出现窗口边沿分割图像数据的情况，
       彩色屏幕将会比较简单，因为一个像素点是多个字节，只需要判断像素点的坐标与窗口的关系即可
@使用示例：
       SETUP:
           txtViewer_init(&txtViewer,buf_size,0,0,NORMAL);
       LOOP:
           txtViewer.loop_show = true;
           txtViewer.run(&txtViewer);
```





