/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include "../defs.h"

struct TCPSERVERSTARTUP
{
    PROCDATA *procdata;
	int m3ua;
										
	int port;
};

class TCPSERVER : public SS7PROC
{
  public:
	int state;		// 0 - OK
	int debug;
//	int start(void *param);
//	int event(void *param);
//	int stop(void *param);
	virtual int start(void *param);
	virtual int event(void *param);
	virtual int stop(void *param);
  private:
    PROCDATA *procdata;
	int m3ua;
										
	int port;
	int clqty;
	int *cl;
	long *ip;	

	int sock;
	int req_sock;
	
	void* serv_recv();
	int serv_send(unsigned *buf);
	int init_sctp_serv();
	void count_connections(char *);
};

