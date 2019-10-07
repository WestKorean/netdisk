#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <termios.h> 
#include <errno.h>   
#include <string.h>

#ifndef SER_OP
#define SER_OP

#define TRUE 	 1
#define FALSE	-1
#define PING_CMD 	"ping -c 3 -w 5 223.5.5.5 -Iwwan0 | grep -c ttl"
#define DEV_PATH 	"/dev/cdc-wdm0"
#define PS_CMD 		"ps -C quectel-CM | wc -l"

#define NO_SIM		255
#define MODE_OK		9
#define NO_SERVICE  0


#define PER_MIN     12
#define FIVE_MIN    (1 * PER_MIN)
#define NO_SERVICE  0


int UART0_Open(void);
void UART0_Close(int);
int UART0_Set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);
int UART0_Init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);
int UART0_Recv(int fd, char *rcv_buf, int data_len);
int UART0_Send(int fd, char *send_buf, int data_len);
int ping_available(void);
int dev_exist(void);
int proce_available(void);
int read_sysinfo(void);
int soft_reset(void);
int hard_reset(void);
#endif