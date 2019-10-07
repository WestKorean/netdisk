#include "serial_4g.h"

#define debug

enum service_type {Serv, Serv_dom, Roam, Mode, Sim};


int main(int argc, char **argv)
{
	extern int sysinfo[];
    int err;
	unsigned int ping_error = 0;
   
hard_test:

	do
	{
		if (!dev_exist())
		{
#ifdef debug	
		printf("[-] dev_exist: false.\n");
#endif
			sleep(5);
			continue;
		}

		do
		{
			err = read_sysinfo();
         
			if (err < 0)
            {
#ifdef debug	
		printf("[-] RW dev error.\n");
#endif 
                goto hard_test;
            }
				

			// No SIM Card?
 			if (sysinfo[Sim] == NO_SIM)
			{
#ifdef debug	
		printf("[+] soft_reset. \n");
#endif
				err = soft_reset();
				if (err < 0)
                {
#ifdef debug	
		printf("[-] RW dev error.\n");
#endif 
                    goto hard_test;
                }

#ifdef debug	
		printf("[-] No SIM. \n");
#endif 
				sleep(20);
				continue;
			}
			
			// SIM Card Error or No Service?
			if (sysinfo[Mode] != MODE_OK || sysinfo[Serv] == NO_SERVICE)
			{
#ifdef debug	
		printf("[-] MODE not OK or NO_SERVICE.\n");
#endif 
				sleep(20);
				continue;
			}
            break;

		} while (1);
		

soft_test:

		if (!proce_available())
		{
#ifdef debug	
		printf("[-] quectel_exist: false.\n");
#endif
			system("quectel-CM");
			sleep(5);
		}


		do
		{
            if (ping_available())
            {
#ifdef debug	
		printf("[+] ping_availabled. \n");
#endif
                sleep(20);
                ping_error = 0;
                continue;
            } 
            else
			{
                ping_error++;
#ifdef debug	
		printf("[+] ping_error: %u. \n", ping_error);
#endif
                if (ping_error < FIVE_MIN)
				    goto hard_test;
                else if (proce_available())
                {
#ifdef debug	
		printf("[+] hard_reset. \n");
#endif
				    err = hard_reset();
                    if (err < 0)
                        goto hard_test;
                    sleep(60);
                }
			}
		} while (1);

	} while (1);
}
