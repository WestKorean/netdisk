#include <stdio.h>  /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>   /*文件控制定义*/
#include <termios.h> /*PPSIX 终端控制定义*/
#include <errno.h>   /*错误号定义*/
#include <string.h>
#include <sys/time.h>
#include<pthread.h>  /*多线程定义*/

//宏定义
#define FALSE -1
#define TRUE   0
#define STR_LEN 20
#define FREAM_LEN 28

#define DEBUG  0
#define DEBUG_I     0
#define DEBUG_RECV  1


unsigned int count,count2;
unsigned char str  = 'A', str2 = 'A';

void *do_Send_WifiList(void *arg);

//参数结构体
struct arg_thread
{
    char *buff;
    int  addr;
    int  fd;
};


/******************************************************************* 
    * 名称：            genRandomString      get_randrom
    * 功能：            随机字符,数字生成 
    * 入口参数：        length    :生成长度     ouput :输出缓冲区 
    * 出口参数：        正确返回为生成数.长度
    *******************************************************************/
int get_random(int flag)
{
    int r = 0;
    struct timeval tpstart;
    gettimeofday(&tpstart, NULL);
    srand(tpstart.tv_usec);
    if (flag)
    switch (flag)
    {
    case 1:
        r = rand()%2;
        break;   
    case 15:
        r = rand()%15+1;
        break;
    case 199:
        r = rand()%199+1;
        break;
    default:
        perror("get_random err\n");   
        break;
    }
    return r;
}

int genRandomString(char* ouput)
{
    int flag, i, length;
    struct timeval tpstart;

	length = get_random(15);
	
    gettimeofday(&tpstart,NULL);
    srand(tpstart.tv_usec);
    for (i = 0; i < length; i++)
    {
        flag = rand() % 3;
        switch (flag)
        {
        case 0:
            ouput[i] = 'A' + rand() % 26;
            break;
        case 1:
            ouput[i] = 'a' + rand() % 26;
            break;
        case 2:
            ouput[i] = '0' + rand() % 10;
            break;
        default:
            ouput[i] = 'x';
            break;
        }
    }
    return length;
}


/******************************************************************* 
    * 名称：                  UART0_Open 
    * 功能：                打开串口并返回串口设备文件描述 
    * 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2) 
    * 出口参数：        正确返回为1，错误返回为0 
    *******************************************************************/
int UART0_Open(int fd, char *port)
{

    fd = open(port, O_RDWR | O_NOCTTY);   //O_NDELAY 为非阻塞模式
    if (FALSE == fd)
    {
        perror("Can't Open Serial Port");
        return (FALSE);
    }
     
    if (fcntl(fd, F_SETFL, 0) < 0)
    {
        printf("fcntl failed!\n");
        return (FALSE);
    }
    else
    {
 #if DEBUG
         printf("fcntl=%d\n", fcntl(fd, F_SETFL, 0));
 #endif
    }
 

    //测试是否为终端设备
    if (0 == isatty(STDIN_FILENO))
    {
        printf("standard input is not a terminal device\n");
        return (FALSE);
    }
    else
    {
#if DEBUG
        printf("isatty success!\n");
        printf("fd->open=%d\n", fd);
#endif
    }
    return fd;
}
/******************************************************************* 
    * 名称：                UART0_Close 
    * 功能：                关闭串口并返回串口设备文件描述 
    * 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2) 
    * 出口参数：        void 
    *******************************************************************/

void UART0_Close(int fd)
{
    close(fd);
}

/******************************************************************* 
    * 名称：                UART0_Set 
    * 功能：                设置串口数据位，停止位和效验位 
    * 入口参数：        fd        串口文件描述符 
    *                              speed     串口速度 
    *                              flow_ctrl   数据流控制 
    *                           databits   数据位   取值为 7 或者8 
    *                           stopbits   停止位   取值为 1 或者2 
    *                           parity     效验类型 取值为N,E,O,,S 
    *出口参数：          正确返回为1，错误返回为0 
    *******************************************************************/
int UART0_Set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{

    int i;
    int status;
    int speed_arr[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
    int name_arr[] = {115200, 19200, 9600, 4800, 2400, 1200, 300};

    struct termios options;

    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1. 
        */
    if (tcgetattr(fd, &options) != 0)
    {
        perror("SetupSerial 1");
        return (FALSE);
    }

    //设置串口输入波特率和输出波特率
    for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
    {
        if (speed == name_arr[i])
        {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }

    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;

    //设置数据流控制
    switch (flow_ctrl)
    {

    case 0: //不使用流控制
        options.c_cflag &= ~CRTSCTS;
        break;

    case 1: //使用硬件流控制
        options.c_cflag |= CRTSCTS;
        break;
    case 2: //使用软件流控制
        options.c_cflag |= IXON | IXOFF | IXANY;
        break;
    }
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {
    case 5:
        options.c_cflag |= CS5;
        break;
    case 6:
        options.c_cflag |= CS6;
        break;
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr, "Unsupported data size\n");
        return (FALSE);
    }
    //设置校验位
    switch (parity)
    {
    case 'n':
    case 'N': //无奇偶校验位。
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O': //设置为奇校验
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;
        break;
    case 'e':
    case 'E': //设置为偶校验
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;
        break;
    case 's':
    case 'S': //设置为空格
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        fprintf(stderr, "Unsupported parity\n");
        return (FALSE);
    }
    // 设置停止位
    switch (stopbits)
    {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        fprintf(stderr, "Unsupported stop bits\n");
        return (FALSE);
    }

    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //options.c_lflag &= ~(ISIG | ICANON);

    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1;  /* 读取字符的最少个数为1 */

    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
    tcflush(fd, TCIFLUSH);

    //激活配置 (将修改后的termios数据设置到串口中）
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("com set error!\n");
        return (FALSE);
    }
    return (TRUE);
}
/******************************************************************* 
    * 名称：                UART0_Init() 
    * 功能：                串口初始化 
    * 入口参数：        fd       :  文件描述符    
    *               speed  :  串口速度 
    *                              flow_ctrl  数据流控制 
    *               databits   数据位   取值为 7 或者8 
    *                           stopbits   停止位   取值为 1 或者2 
    *                           parity     效验类型 取值为N,E,O,,S 
    *                       
    * 出口参数：        正确返回为1，错误返回为0 
    *******************************************************************/
int UART0_Init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
    int err;
    //设置串口数据帧格式
    if (UART0_Set(fd, speed, flow_ctrl, databits, stopbits, parity) == FALSE)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/******************************************************************* 
    * 名称：                  UART0_Recv 
    * 功能：                接收串口数据 
    * 入口参数：        fd                  :文件描述符     
    *                              rcv_buf     :接收串口中数据存入rcv_buf缓冲区中 
    *                              data_len    :一帧数据的长度 
    * 出口参数：        正确返回为1，错误返回为0 
    *******************************************************************/
int UART0_Recv(int fd, char *rcv_buf, int data_len)
{
    int len, fs_sel;
    fd_set fs_read;

    struct timeval time;

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

    time.tv_sec = 10;
    time.tv_usec = 0;

    //使用select实现串口的多路通信
    fs_sel = select(fd + 1, &fs_read, NULL, NULL, &time);
    printf("fs_sel = %d\n", fs_sel);
    if (fs_sel)
    {
        len = read(fd, rcv_buf, data_len);
        printf("I am right!(version1.2) len = %d fs_sel = %d\n", len, fs_sel);
        return len;
    }
    else
    {
        printf("Sorry,I am wrong!");
        return FALSE;
    }
}
/******************************************************************** 
    * 名称：            UART0_Send 
    * 功能：            发送数据 
    * 入口参数：        fd                      :文件描述符     
    *                  send_buf                :存放串口发送数据 
    *                  data_len                :一帧数据的个数 
    * 出口参数：        正确返回为1，错误返回为0 
    *******************************************************************/
int UART0_Send(int fd, char *send_buf, int data_len)
{
    int len = 0;

    len = write(fd, send_buf, data_len);
    if (len == data_len)
    {
#if DEBUG
        printf("send data done.\n");
#endif
        return len;
    }
    else
    {
        tcflush(fd, TCOFLUSH);
        return FALSE;
    }
}


/******************************************************************** 
    * 名称：            clear 
    * 功能：            清屏 
    * 入口参数：        send_buf                :存放串口发送数据    
    *                  addr                    :发送地址
    * 出口参数：        空
    *******************************************************************/

void clear(char *send_buf, int addr, unsigned int len)
{
    char *buffp, addr_top, addr_botm;
    int i, length_data, length_clr;

    buffp = send_buf;
    addr_top      = (addr>>8) & 0xff;
    addr_botm     = addr & 0xff;
    length_data   = len - 3;
    length_clr    = len - 6;

    *buffp++ = 0x5a;
    *buffp++ = 0xa5;
    *buffp++ = length_data;
    *buffp++ = 0x82;
    *buffp++ = addr_top;
    *buffp++ = addr_botm;
    
    for(i = 0; i < length_clr; i++)
    {
        *buffp++ = 0x00;

    }
}

/******************************************************************** 
    * 名称：            made_Package_wifiList 
    * 功能：            数据帧组装 
    * 入口参数：        send_buf                :存放串口发送数据    
    *                  addr                    :发送地址
    * 出口参数：        返回为完整帧尺寸
    *******************************************************************/
int made_Package_WifiList(char *send_buf, int addr) {
    char brand_buf[30], mac_buf[30], name_buf[30];
    char *buffp, *brandp, *macp, *namep, addr_top, addr_botm;
    int package_len, data_len, brand_len, mac_len, name_len, signal, crypt, chain, i;
    buffp = send_buf;
    brandp = brand_buf;
    macp   = mac_buf;
    namep  = name_buf;


    brand_len     = genRandomString(brand_buf);    
    mac_len       = genRandomString(mac_buf);    
    name_len      = genRandomString(name_buf);    
    signal        = get_random(199);
    crypt         = get_random(1);
    chain         = get_random(199);

    data_len      = 3 + 66;
    package_len    = 3 + data_len;
    addr_top      = (addr>>8) & 0xff;
    addr_botm     = addr & 0xff;

   
    *buffp++ = 0x5a;
    *buffp++ = 0xa5;
    *buffp++ = data_len;
    *buffp++ = 0x82;
    *buffp++ = addr_top;
    *buffp++ = addr_botm;

    //Signal, Crypt, Chain
    *buffp++ = 0;
    *buffp++ = signal;
    *buffp++ = 0;
    *buffp++ = crypt;
    *buffp++ = 0;
    *buffp++ = chain;
    

    //Brand, Mac, Wifi
    for(i = 0; i < 20; i++)
    {
        if (i < brand_len)
            *buffp++ = *brandp++;
        else
            *buffp++ = 0;
    }

    for(i = 0; i < 20; i++)
    {
        if (i < mac_len)
            *buffp++ = *macp++;
        else
            *buffp++ = 0;
    }

    for(i = 0; i < 20; i++)
    {
        if (i < name_len)
            *buffp++ = *namep++;
        else
            *buffp++ = 0;
    }
    
#if DEBUG_I
    for(i = 0; i < package_len; i++)
    {
        printf("%02x", send_buf[i] & 0xff);
    }
    printf("\n");
#endif
    return package_len;
}




/******************************************************************** 
    * 名称：            made_Package_wifiList_Test_01 
    * 功能：            数据帧组装 
    * 入口参数：        send_buf                :存放串口发送数据    
    *                  addr                    :发送地址
    * 出口参数：        返回为完整帧尺寸
    *******************************************************************/
int made_Package_WifiList_Test_01(char *send_buf, int addr) {
    char brand_buf[30], mac_buf[30], name_buf[30];
    char *buffp, *brandp, *macp, *namep, addr_top, addr_botm;
    int package_len, data_len, brand_len, mac_len, name_len, signal, crypt, chain, i;
    buffp = send_buf;


    signal        = count % 1000;
    crypt         = 1;
    chain         = get_random(199);

    data_len      = 3 + 6;
    package_len   = 3 + data_len;
    addr_top      = (addr>>8) & 0xff;
    addr_botm     = addr & 0xff;

   
    *buffp++ = 0x5a;
    *buffp++ = 0xa5;
    *buffp++ = data_len;
    *buffp++ = 0x82;
    *buffp++ = addr_top;
    *buffp++ = addr_botm;

    //Signal, Crypt, Chain
    *buffp++ = 0;
    *buffp++ = signal;
    *buffp++ = 0;
    *buffp++ = crypt;
    *buffp++ = 0;
    *buffp++ = chain;
    
    return package_len;
}




/******************************************************************** 
    * 名称：            made_Package_wifiList_Test_02
    * 功能：            数据帧组装 
    * 入口参数：        send_buf                :存放串口发送数据    
    *                  addr                    :发送地址
    * 出口参数：        返回为完整帧尺寸
    *******************************************************************/
int made_Package_WifiList_Test_02(char *send_buf, int addr) {
    char brand_buf[30], mac_buf[30], name_buf[30];
    char *buffp, *brandp, *macp, *namep, addr_top, addr_botm;
    int package_len, data_len, brand_len, mac_len, name_len, signal, crypt, chain, i;
    buffp = send_buf;
    brandp = brand_buf;
    macp   = mac_buf;
    namep  = name_buf;
 


    data_len      = 3 + 60;
    package_len   = 3 + data_len;
    addr_top      = (addr>>8) & 0xff;
    addr_botm     = addr & 0xff;

   
    *buffp++ = 0x5a;
    *buffp++ = 0xa5;
    *buffp++ = data_len;
    *buffp++ = 0x82;
    *buffp++ = addr_top;
    *buffp++ = addr_botm;
    
    //Brand, Mac, Wifi
    for(i = 0; i < 60; i++)
    {
        *buffp++ = str;
    }
    
    if (str > 'z') {
        str = 'A';
    }
   
    return package_len;
}



/******************************************************************** 
    * 名称：            made_Package_4g 
    * 功能：            数据帧组装 
    * 入口参数：        send_buf                :存放串口发送数据    
    *                  addr                    :发送地址
    * 出口参数：        返回为完整帧尺寸
    *******************************************************************/
int made_Package_4g(char *send_buf, int addr) {
    char *buffp, addr_top, addr_botm;
    int package_len, data_len, signal, connct, plug, i;
    buffp = send_buf;

 
    signal        = get_random(199);
    connct        = get_random(1);
    plug          = get_random(1);

    data_len      = 3 + 6;
    package_len   = 3 + data_len;
    addr_top      = (addr>>8) & 0xff;
    addr_botm     = addr & 0xff;

   
    *buffp++ = 0x5a;
    *buffp++ = 0xa5;
    *buffp++ = data_len;
    *buffp++ = 0x82;
    *buffp++ = addr_top;
    *buffp++ = addr_botm;

    //Plug, Connct, Signal
    *buffp++ = 0;
    *buffp++ = plug;
    *buffp++ = 0;
    *buffp++ = connct;
    *buffp++ = 0;
    *buffp++ = signal%5;
    


#if DEBUG_I
    for(i = 0; i < package_len; i++)
    {
        printf("%02x", send_buf[i] & 0xff);
    }
    printf("\n");
#endif
    return package_len;
}



/******************************************************************** 
    * 名称：            made_Package_Battery
    * 功能：            数据帧组装 
    * 入口参数：        send_buf                :存放串口发送数据    
    *                  addr                    :发送地址
    * 出口参数：        返回为完整帧尺寸
    *******************************************************************/
int made_Package_Battery(char *send_buf, int addr) {
    char *buffp, addr_top, addr_botm;
    int package_len, data_len, charge, capacity, i;
    buffp = send_buf;

 
    charge        = get_random(199);
    capacity      = get_random(199);

    data_len      = 3 + 4;
    package_len   = 3 + data_len;
    addr_top      = (addr>>8) & 0xff;
    addr_botm     = addr & 0xff;

   
    *buffp++ = 0x5a;
    *buffp++ = 0xa5;
    *buffp++ = data_len;
    *buffp++ = 0x82;
    *buffp++ = addr_top;
    *buffp++ = addr_botm;

    //Charge, Capacity
    *buffp++ = 0;
    *buffp++ = charge % 3;
    *buffp++ = 0;
    *buffp++ = capacity % 101;

    

#if DEBUG_I
    for(i = 0; i < package_len; i++)
    {
        printf("%02x", send_buf[i] & 0xff);
    }
    printf("\n");
#endif
    return package_len;
}

/******************************************************************** 
    * 名称：            made_Package_CurrWifiList
    * 功能：            数据帧组装 
    * 入口参数：        send_buf                :存放串口发送数据    
    *                  addr                    :发送地址
    * 出口参数：        返回为完整帧尺寸
    *******************************************************************/
int made_Package_CurrWifiList(char *send_buf, int addr) {
    char *buffp, *namep, addr_top, addr_botm;
    char name_buf[30];
    int package_len, data_len, name_len, charge, connct, signal, i;
    buffp = send_buf;
    namep = name_buf;
 
    connct        = get_random(1);
    signal        = get_random(199);
    name_len      = genRandomString(name_buf);

    data_len      = 3 + 24;
    package_len   = 3 + data_len;
    addr_top      = (addr>>8) & 0xff;
    addr_botm     = addr & 0xff;

   
    *buffp++ = 0x5a;
    *buffp++ = 0xa5;
    *buffp++ = data_len;
    *buffp++ = 0x82;
    *buffp++ = addr_top;
    *buffp++ = addr_botm;

    //Connct, Signal
    *buffp++ = 0;
    *buffp++ = connct;
    *buffp++ = 0;
    *buffp++ = signal % 4;

    //Name
    for(i = 0; i < 20; i++)
    {
        if (i < name_len)
            *buffp++ = *namep++;
        else
            *buffp++ = 0;
    }


#if DEBUG_I
    for(i = 0; i < package_len; i++)
    {
        printf("%02x", send_buf[i] & 0xff);
    }
    printf("\n");
#endif
    return package_len;
}






void *do_Send_Test(void *arg)
{
    int i,len_package = 64;
    struct arg_thread *arg_t = (struct arg_thread *) arg;

    while(1){
        //len_package = made_Package_WifiList(arg_t->buff, arg_t->addr);

        for(i = 0; i < len_package; i++)
        {
            arg_t->buff[i] = 'A'; 
        }

        
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        sleep(5);
        //clear(arg_t->buff, arg_t->addr, len_package);
        //UART0_Send(arg_t->fd, arg_t->buff, len_package);
        //usleep(100000);

#if DEBUG_II
        printf("len_package:%u\n", len_package);
        printf("addr: %u, buff: %p, len: %u\n", arg_t->addr, arg_t->buff, len_package);
#endif
    }
}




/******************************************************************** 
    * 名称：            do_Send_WifiList 
    * 功能：            线程发送函数 
    * 入口参数：        arg                :   传入参数 
    * 出口参数：        返回为线程执行完毕的返回值
    *******************************************************************/

void *do_Send_WifiList(void *arg)
{
    int len_package;
    struct arg_thread *arg_t = (struct arg_thread *) arg;

    while(1){
        len_package = made_Package_WifiList(arg_t->buff, arg_t->addr);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        sleep(5);
        clear(arg_t->buff, arg_t->addr, len_package);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        usleep(100000);

#if DEBUG_II
        printf("len_package:%u\n", len_package);
        printf("addr: %u, buff: %p, len: %u\n", arg_t->addr, arg_t->buff, len_package);
#endif
    }
}




/******************************************************************** 
    * 名称：            do_Send_WifiList_Test 
    * 功能：            线程发送函数 
    * 入口参数：        arg                :   传入参数 
    * 出口参数：        返回为线程执行完毕的返回值
    *******************************************************************/

void *do_Send_WifiList_Test(void *arg)
{
    int len_package, i;
    struct arg_thread *arg_t = (struct arg_thread *) arg;

    while(1){
        
        for(i = 0; i < 4; i++)
        {
            len_package = made_Package_WifiList_Test_01(arg_t->buff, (arg_t+i)->addr);
            UART0_Send(arg_t->fd, arg_t->buff, len_package);
            usleep(10000);
            len_package = made_Package_WifiList_Test_02(arg_t->buff+20, (arg_t+i)->addr+3);
            UART0_Send(arg_t->fd, arg_t->buff+20, len_package);
            usleep(10000);
        }
        count++;
        str++;
        sleep(5);
        //clear(arg_t->buff, arg_t->addr, len_package);
        //UART0_Send(arg_t->fd, arg_t->buff, len_package);
        

    }
}


void *do_Send_WifiList_Test_II(void *arg)
{
    int len_package, i;
    struct arg_thread *arg_t = (struct arg_thread *) arg;

    while(1){
        
        for(i = 0; i < 4; i++)
        {
            len_package = made_Package_WifiList_Test_01(arg_t->buff, (arg_t+i)->addr);
            UART0_Send(arg_t->fd, arg_t->buff, len_package);
            usleep(10000);
            len_package = made_Package_WifiList_Test_02(arg_t->buff+20, (arg_t+i)->addr+3);
            UART0_Send(arg_t->fd, arg_t->buff+20, len_package);
            usleep(10000);
        }
        count2++;
        str2++;
        sleep(5);
        //clear(arg_t->buff, arg_t->addr, len_package);
        //UART0_Send(arg_t->fd, arg_t->buff, len_package);
        

    }
}





void *do_Send_4g(void *arg)
{
    int len_package;
    struct arg_thread *arg_t = (struct arg_thread *) arg;

    while(1){
        len_package = made_Package_4g(arg_t->buff, arg_t->addr);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        sleep(5);
        clear(arg_t->buff, arg_t->addr, len_package);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        usleep(100000);

#if DEBUG_II
        printf("len_package:%u\n", len_package);
        printf("addr: %u, buff: %p, len: %u\n", arg_t->addr, arg_t->buff, len_package);
#endif
    }
}


void *do_Send_Battery(void *arg)
{
    int len_package;
    struct arg_thread *arg_t = (struct arg_thread *) arg;

    while(1){
        len_package = made_Package_Battery(arg_t->buff, arg_t->addr);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        sleep(5);
        clear(arg_t->buff, arg_t->addr, len_package);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        usleep(100000);

#if DEBUG_II
        printf("len_package:%u\n", len_package);
        printf("addr: %u, buff: %p, len: %u\n", arg_t->addr, arg_t->buff, len_package);
#endif
    }
}


void *do_Send_CurrWifi(void *arg)
{
    int len_package;
    struct arg_thread *arg_t = (struct arg_thread *) arg;

    while(1){
        len_package = made_Package_CurrWifiList(arg_t->buff, arg_t->addr);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        sleep(5);
        clear(arg_t->buff, arg_t->addr, len_package);
        UART0_Send(arg_t->fd, arg_t->buff, len_package);
        usleep(100000);

#if DEBUG_II
        printf("len_package:%u\n", len_package);
        printf("addr: %u, buff: %p, len: %u\n", arg_t->addr, arg_t->buff, len_package);
#endif
    }
}


void *do_Recv(void *arg)
{
    int nread, i, t_len;
	unsigned int s_l;
    struct arg_thread *arg_t = (struct arg_thread *) arg;
	char sysinfo[] = "SYSINFO:";
	char *t, *t_1, *token;	
	s_l = strlen(sysinfo);
	char result[256];

	unsigned char cmd[] = {'A', 'T', '^', 'S', 'Y' ,'S' ,'I' ,'N', 'F', 'O' ,' ' ,'\r' ,'\n'};

	write(arg_t->fd, cmd, sizeof(cmd) - 1);

	


    while(1){
        if((nread = read(arg_t->fd, arg_t->buff, 255)) > 0) {
#if DEBUG_RECV
			if ((t = strstr(arg_t->buff, sysinfo)) == NULL)
				continue;
			t += s_l;

			if (*t == 0x20)
				t++;
			
			if ((t_1 = strchr(t, 0x0a)) != NULL)
			{
				*t_1 = 0x00;
				strncpy(result, t, (strlen(t) + 1));
				printf("result str: %s\n", result);
			}
			
			t_1 = result;

			for(token = strsep(&t_1, ","); token != NULL; token = strsep(&t_1, ",")) 
			{
				
				printf("[+] %d\n",atoi(token));
			}
			
				exit(0);
			

/**
			t_1 = strtok(result, ",");
			while(t_1 != NULL)
			{
				t_1 = strtok(NULL, ",");
			}
			

			nread = strlen(t);	
            printf("Recv: %u [ ", nread);
            for(i = 0; i < nread; i++)
            {
                printf("%02X ", t[i]&0xff);
                //printf("%c", arg_t->buff[i]);
            }
            printf("]\n");
**/
							
#endif
        }
    }
}


int main(int argc, char **argv)
{
    int fd;  //文件描述符
    int err, res; //返回调用函数的状态
    int len,nread;
    int i;
    int wifiList_addr1 = 0x2000;
    int wifiList_addr2 = 0x2100;
    int addr_4g        = 0x1200;
    int bat_addr       = 0x1210;
    int curr_addr      = 0x1220;
    char rcv_buf[255];
    char buf_4g[50];
    char baty_buf[50];
    char curr_buf[50];
    char wifiList_buf1[4096];
    char wifiList_buf2[4096];
    char *buffp1 = wifiList_buf1;
    char *buffp2 = wifiList_buf2;
    pthread_t tpid_1[4], tpid_2[4], tpid_4g, tpid_battery, tpid_curr, tpid_recv;


    if (argc != 3)
    {
        printf("Usage: %s /dev/ttySn 0(send data)/1 (receive data) \n", argv[0]);
        exit(FALSE);
    }
    fd = UART0_Open(fd, argv[1]); //打开串口，返回文件描述符

    err = UART0_Init(fd, 9600, 0, 8, 1, 'N');
    if (FALSE == err || FALSE == fd) {
        printf("Init error\n");
        exit(FALSE);
    }
  	
	struct arg_thread args_recv = {
        .buff = rcv_buf,
        .fd   = fd,
	};

	struct arg_thread args1[4] = {
		{buffp1, wifiList_addr1, fd},
		{buffp1+0x100, wifiList_addr1+0x30, fd},
		{buffp1+0x200, wifiList_addr1+0x60, fd},
		{buffp1+0x300, wifiList_addr1+0x90, fd},
	};

	struct arg_thread args2[4] = {
		{buffp2, wifiList_addr2, fd},
		{buffp2+0x100, wifiList_addr2+0x30, fd},
		{buffp2+0x200, wifiList_addr2+0x60, fd},
		{buffp2+0x300, wifiList_addr2+0x90, fd},
	};

	struct arg_thread args_4g = {
        .buff = buf_4g,
        .addr = addr_4g,
        .fd   = fd,
	};

	struct arg_thread args_barty = {
        .buff = baty_buf,
        .addr = bat_addr,
        .fd   = fd,
	};

	struct arg_thread args_curr = {
        .buff = curr_buf,
        .addr = curr_addr,
        .fd   = fd,
	};

    


    //收包线程
    res = pthread_create(&tpid_recv, NULL, do_Recv, (void *)&args_recv);
    if (res) {
        perror("Thread Create error");
        exit(FALSE);
    }    
	
/**
    //WifiList1 4线程;
    for(i = 0; i < 1; i++)
    {
        res = pthread_create(&tpid_1[i], NULL, do_Send_WifiList, (void *)&args1[i]);
        if (res) {
            perror("Thread Create error");
            exit(FALSE);
        }
		sleep(1);
    }

    //WifiList2 4线程;
    for(i = 0; i < 0; i++)
    {
        res = pthread_create(&tpid_2[i], NULL, do_Send_WifiList, (void *)&args2[i]);
        if (res) {
            perror("Thread Create error");
            exit(FALSE);
        }
		sleep(1);
    }

    //4g状态线程;
    res = pthread_create(&tpid_4g, NULL, do_Send_4g, (void *)&args_4g);
    if (res) {
        perror("Thread Create error");
        exit(FALSE);
    }    


    //Battery线程;
    res = pthread_create(&tpid_battery, NULL, do_Send_Battery, (void *)&args_barty);
    if (res) {
        perror("Thread Create error");
        exit(FALSE);
    }    


    //Current_WifiList线程;
    res = pthread_create(&tpid_curr, NULL, do_Send_CurrWifi, (void *)&args_curr);
    if (res) {
        perror("Thread Create error");
        exit(FALSE);
    }

**/


/**
    for(i = 0; i < 0; i++)
    {
        res = pthread_create(&tpid_2[i], NULL, do_Send_WifiList, (void *)&args2[i]);
        if (res) {
            perror("Thread Create error");
            exit(FALSE);
        }
		sleep(1);
    }

**/
    //回收线程;

	pthread_join(tpid_recv, NULL);

/*    
*
*    while(1){
*        if((nread = read(fd, rcv_buf, 255)) > 0) {
*#if DEBUG_RECV
*            printf("Recv: %u [ ", nread);
*            for(i = 0; i < nread; i++)
*            {
*                printf("%02X ", rcv_buf[i]);
*            }
*            printf("]\n");
*#endif
*        }
*    }
*    
*
*
 *
 *    if (0 == strcmp(argv[2], "0"))
 *    {
 *        for (i = 0; i < 10; i++)
 *        {
 *            len = UART0_Send(fd, send_buf, 10);
 *            if (len > 0)
 *                printf(" %d time send %d data successful\n", i, len);
 *            else
 *                printf("send data failed!\n");
 *
 *            sleep(2);
 *        }
 *        UART0_Close(fd);
 *    }
 *    else
 *    {
 *        while (1) //循环读取数据
 *        {
 *            len = UART0_Recv(fd, rcv_buf, 99);
 *            if (len > 0)
 *            {
 *                rcv_buf[len] = '\0';
 *                printf("receive data is %s\n", rcv_buf);
 *                printf("len = %d\n", len);
 *            }
 *            else
 *            {
 *                printf("cannot receive data\n");
 *            }
 *            sleep(2);
 *        }
 *        UART0_Close(fd);
 *    }
 *
 */

    UART0_Close(fd);
}
