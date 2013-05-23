#include "../defs.h"
#include <netinet/sctp.h>
#define SCTP_FIFO_SIZE	1000

struct SCTPSERVERSTARTUP
{
    PROCDATA *procdata;
	int m3ua;
										
	int port;
};

class SCTPSERVER// : public SS7PROC
{
  public:
	int state;		// 0 - OK
	int debug;
	virtual int start(void *param);
	virtual void timer();
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
	
	void **txbuffer;
	int *streamid;
	int client_id_to_start_with;
	int put_ptr;
	int get_ptr;
//	int top_fifo_load;
	
	void* serv_recv();
	int serv_send(void *buf, int stream);
	int do_send();
	int init_sctp_serv();
	void count_connections(char *);

	struct sctp_sndrcvinfo sinfo;
};

