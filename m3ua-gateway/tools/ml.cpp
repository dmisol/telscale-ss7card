/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "../../shm_keys.h"

#include <sys/shm.h>

char *aonlog;
int *aonlogpos;

main(int argc, char* argv[])
{
   int shm_id,shm_perm,key;
   int i,j,k,outmem;
   char *str;
   int slen;
      
   if(argc==2)
   { sscanf(argv[1],"%u",&key);}
   else key = KEY_LOG_APP0;
   
   shm_perm = 0100666;
   shm_id = shmget((key_t)key,(sizeof(int)+64*300),shm_perm);
   if(shm_id == -1)
     { printf("can't find shared memory\n");
       exit(1); }
       
   aonlog = (char*) shmat(shm_id,0,0);
   aonlogpos = (int *) &aonlog[64*300];

   i=((*aonlogpos)+1)&0x3F;
   while(1)
   { if( ((*aonlogpos)&0x3F) == i) 
       { usleep(50000L);
       }
     else 
        while( i != ((*aonlogpos)&0x3F) )
         {  str = &aonlog[i*300];
	    
	    printf("%s\n",str);
	    i++; i&=0x3F;
         }
     fflush(0);
   }
}

