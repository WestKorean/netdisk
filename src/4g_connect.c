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

	if( (fgets(buff, sizeof(buff), fstream) != NULL)
	{
		pclose(fstream);
		if (buff[0] == '3')
			return 1;
		else
			return 0;
	}
	else
	{
		return -1;
	}
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

	if( (fgets(buff, sizeof(buff), fstream) != NULL)
	{
		pclose(fstream);
		num = atoi(buff);
		if ((num - 1) == 0)
			return 0;
		else
			return 1;
	}
	else
	{
		return -1;
	}

	

}


int main(int argc,char*argv[])
{    
	int i;

	if (dev_exist())
		printf("dev is exist\n");
	else
		printf("dev is not exist\n");

	if (ping_available())
		printf("Ping Ture\n");
	else
		printf("Ping False\n");

	if (proce_available())
		printf("Proce Ture\n");
	else
		printf("Proce False\n");

	
	do
	{
		if (!dev_exist())
		{
			sleep(5);
			continue;
		}

		if (!proce_available())
		{
			system("quectel-CM");
			sleep(5);
		}
		
		do
		{
			int c = 0;
			for ( i = 0; i < 2; i++)
			{
				if (!ping_available())
					c++;
			}

			if (c < 2)
			{
				sleep(5);
				continue;
			}
			else
			{
				break;
			}
			
		} while (true);
		
		
		

		
	} while (true);
	


	return 0;     
}   
