#include <stdio.h>    
#include <errno.h>
#include <string.h>    
#include <fcntl.h>
#include <unistd.h>

#define PING_CMD 	"ping -c 3 -w 5 114.114.114.114 -Iwwan0 | grep -c ttl"
#define DEV_PATH 	"/dev/cdc-wdm0"
#define PS_CMD 		"ps -C quectel-CM | wc -l"


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

   	fgets(buff, sizeof(buff), fstream);
    pclose(fstream);    
	if (buff[0] == '3')
		return 1;
	else
		return 0;
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

int procc_available()
{
	FILE *fstream = NULL;      
    char buff[256];    
    memset(buff, 0, sizeof(buff));   

	if(NULL == (fstream = popen(PING_CMD,"r")))      
    {     
        fprintf(stderr,"execute command failed: %s",strerror(errno));      
        return -1;      
    }   

   	fgets(buff, sizeof(buff), fstream);
    pclose(fstream);    
	if (buff[0] == '3')
		return 1;
	else
		return 0;

}


int main(int argc,char*argv[])
{    
	if (ping_available())
		printf("Ture\n");
	else
		printf("False\n");


	if (dev_exist())
		printf("dev is exist\n");
	else
		printf("dev is not exist\n");


	return 0;     
}   
