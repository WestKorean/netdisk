#include "serial_4g.h"


char *s_port[2] = {"/dev/ttyUSB2", "/dev/ttyUSB3"};
int sysinfo[5] = {0};
unsigned char cmd  [] = {'A', 'T', '^', 'S', 'Y', 'S', 'I', 'N', 'F', 'O', ' ', '\r', '\n'};
unsigned char cmd_0[] = {'A', 'T', '+', 'C', 'F', 'U', 'N', '=', '0', ' ', '\r', '\n'};
unsigned char cmd_1[] = {'A', 'T', '+', 'C', 'F', 'U', 'N', '=', '1', ' ', '\r', '\n'};
unsigned char cmd_h[] = {'A', 'T', '^', 'R', 'E', 'S', 'E', 'T', ' ', '\r', '\n'};


/******************************************************************* 
    * 名称：                  UART0_Open 
    * 功能：                打开串口并返回串口设备文件描述 
    * 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2) 
    * 出口参数：        正确返回为1，错误返回为0 
    *******************************************************************/
int UART0_Open()
{

	int i, fd;


	for(i = 0; i < 2; i++)
	{
    	fd = open(s_port[i], O_RDWR | O_NOCTTY);
		if(fd > 0)
			break;
	}
    if (fd < 0)
		goto error;

     
    if (fcntl(fd, F_SETFL, 0) < 0)
		goto error;
 

    //测试是否为终端设备
    if (0 == isatty(STDIN_FILENO))
		goto error;

    return fd;

error:
	return -1;
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



//Network is available?

int ping_available()
{
	FILE *fstream = NULL;      
    char buff[256];    
    memset(buff, 0, sizeof(buff));   

	if(NULL == (fstream = popen(PING_CMD,"r")))      
    {     
        fprintf(stderr,"execute command failed: %s",strerror(errno));      
        return -1;      
    }   

	if(fgets(buff, sizeof(buff), fstream) != NULL)
	{
		pclose(fstream);
		if (buff[0] == '3')
			return 1;
		else
			return 0;
	}
	else
		return -1;
}


//Dev is exist?

int dev_exist()
{
	if(access(DEV_PATH, F_OK) != -1)
		return 1;
	else
		return 0;
}


//Quectel is available?

int proce_available()
{
	FILE *fstream = NULL;      
    char buff[256];    
    memset(buff, 0, sizeof(buff));  
	int num; 

	if(NULL == (fstream = popen(PS_CMD,"r")))      
    {     
        fprintf(stderr,"execute command failed: %s",strerror(errno));      
        return -1;      
    }   

	if(fgets(buff, sizeof(buff), fstream) != NULL)
	{
		pclose(fstream);
		num = atoi(buff);
		if ((num - 1) == 0)
			return 0;
		else
			return 1;
	}
	else
		return -1;
}


int read_sysinfo()
{
	int fd, i, nread, nwrite, err;
	char r_buffer[256];
	char r_buffer2[64];
	char prefix[] = "SYSINFO:";
	char *str_p, *str_p1;


	fd = UART0_Open();
    err = UART0_Init(fd, 9600, 0, 8, 1, 'N');
    if (fd < 0 || err < 0)
		return -1;
	
    nwrite = write(fd, cmd, sizeof(cmd));
    if (nwrite < 0)
    {
        UART0_Close(fd);
        return -1;
    }

	usleep(100*1000);

    do
	{
        if((nread = read(fd, r_buffer, 256)) > 0)
		{
			if ((str_p = strstr(r_buffer, prefix)) == NULL)
				continue;
			str_p += strlen(prefix);

			if (*str_p == 0x20)
				str_p++;
			
			if ((str_p1 = strchr(str_p, 0x0a)) != NULL)
			{
				*str_p1 = 0x00;
				strncpy(r_buffer2, str_p, (strlen(str_p) + 1));
#ifdef debug	
				printf("result str: %s.\n", r_buffer2);
#endif
			}
			
			str_p = r_buffer2;

			for(i = 0, str_p1 = strsep(&str_p, ","); str_p1 != NULL && i < 5; str_p1 = strsep(&str_p, ","), i++) 
			{
				sysinfo[i] = atoi(str_p1);
			}

    		UART0_Close(fd);
			return 0;
        }
		else
		{
    		UART0_Close(fd);
			return -1;
		}
    }
	while(0);
}


int soft_reset()
{
	int fd, err, nwrite;

	fd = UART0_Open();
    err = UART0_Init(fd, 9600, 0, 8, 1, 'N');
    if (fd < 0 || err < 0)
		return -1;
	
    nwrite = write(fd, cmd_0, sizeof(cmd));
    if (nwrite > 0)
    {
        sleep(5);
        nwrite = write(fd, cmd_1, sizeof(cmd));
        if (nwrite > 0)
        {
            UART0_Close(fd);
            return 0;
        }
    }
    UART0_Close(fd);
	return -1;
}


int hard_reset()
{
	int fd, err, nwrite;

	fd = UART0_Open();
    err = UART0_Init(fd, 9600, 0, 8, 1, 'N');
    if (fd < 0 || err < 0)
		return -1;
    nwrite = write(fd, cmd_h, sizeof(cmd));
    if (nwrite > 0)
    {
        UART0_Close(fd);
        return 0;
    }
    
    UART0_Close(fd);
	return -1;
}