#include "../defs.h"

struct SS7LINKSTARTUP
{
	PROCDATA *procdata;
	int key;
	int link;		// own link number (inside linkset)
	int mtp;		// mtp address

	unsigned *pcap_ip;
	unsigned short *pcap_port;
};

class SS7LINK// : public SS7PROC
{
  public:
	int state;		// 0 - OK
	int debug;
	int link;
	
	virtual int start(void *param);
	virtual void timer();
	virtual int event(void *param);
	virtual int stop(void *param);
  private:
	PROCDATA *procdata;
	int mtp;
	char hw_byte;

	TIMER T1,Tsig,T4;		//T2,T3
	unsigned log_rx_link, log_tx_link;

	struct 
	{ unsigned int len;
	  union{ WORD LSSU[2];		// TXfsnbsn;
			 struct 
			 {  unsigned last_rxd:7;
				unsigned bib:1;
				unsigned fsn:7;
				unsigned fib:1;
			  }bitfield;
	       }body;
	} lssu;

	char tx_last_ackd;
	
	BYTE last_txd,tx_last_placed;
	BYTE rx_ack_request;	

//	TIMER *txtimer;	// array of MSU ack timers
	SS7MESSAGE **txbuffer;	// array of references to messages

	int init(int key);
	int tx_msg(void * buf);
	int rx_msg(void * buf);
	void check_e1();
	void status(void);
	//void alignment_lssu(WORD word);
	void alignment_lssu(int len, void * buf);
	void alignment_timer(void);
	
	int rxpos,txpos;
	unsigned short *txSu;
	unsigned short *rxSu;

	WORD errorlevel;	// DSP errorlevel

	int sock_rx, sock_tx;
	unsigned *pcap_ip;
	unsigned short *pcap_port;
	void pcap(SS7MESSAGE* su);
};
