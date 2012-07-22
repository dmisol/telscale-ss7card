/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#ifndef SS7DEFINES
#define SS7DEFINES


#define DEFAULT_SCTP_PORT       3434

#define SCTP_STREAMS_QTY	17
#define STREAM2SLS(sls) (sls+1)

//#define NETWORK_SIDE_STR	"netw="
//#define CONVERT_PFX_CFG		"ivrd.a.cfg"

//#include "monit.h"



#ifndef WORD
#define WORD unsigned short int
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef TIMER
#define TIMER unsigned int
#endif

// Monitor's structures

void marktime(int marker);
void mem(void);
void post(int dest, int msg, void *param);
void post(int dest, int msg, void *param, unsigned stream, unsigned rc);
void rf5su(int link, int len, char *data);
//void rf5shmem(key_t memid);
void rf5shmem(unsigned ip, unsigned short port);
void log(char *id, int len, char *data);//void log(char *str);
void log_shmem(int logmemid);
void decode(char *text,void *ptr,int netw);
//struct MONITORINGDATA* monitoring_get_idle();

#define MSGQUEUESIZE		1024
#define MAXPROCQTY		20

class SS7PROC {
  public:
	virtual int start(void *param) {}
	virtual int event(void *param) {}
	virtual int stop(void *param)  {}
};

struct PROCDATA
{
	int classname;
	char objectname[20];
	void *classptr;
};

struct MSGDATA
{
	int src;
	int dest;
	int msg;
	void *param;
	
	// added for SCTP
	unsigned param2;
	unsigned param3;
};

#define TimerTick	50		// timer rate for whole project

#define SS7MsgLen	280		// max MSU len
#define SS7MsgSize	300		// size of buffer bytes == (300>>1) words

// memory for SS7 messages
#define SS7BufSize	16384

// proc. types / ss7 si field values
#define SLTX_SI		1
#define ITULINK		SLTX_SI
#define PROC_ITULINK    ITULINK
    
#define	NETMNG_SI	0
#define ITUMTP		NETMNG_SI
#define PROC_ITUMTP     ITUMTP
    
#define ISUP_SI		5
#define ITUISUP		ISUP_SI
#define PROC_ITUISUP    ITUISUP

#define SCCP_SI		3
#define ITUSCCP		SCCP_SI
#define PROC_ITUSCCP    ITUSCCP

#define PROC_TCPSERVER  100
#define LOCALSERV	101
#define ALARMSERV	102

#define PROC_ETSILAPD	200
#define PROC_ETSIDSS1	201

#define PROC_DSS1BRIDGE	300
    
#define PROC_SCTPCLIENT 401
#define PROC_SCTPSERVER 400
    
    
    // messages

#define SS7_START		01
#define SS7_TEST		02
#define SS7_TIMER		03

#define SS7_LINKTRANSFER	11
#define SS7_LINKSTOP		12
#define SS7_LINKRESET		13
#define SS7_LINKPAUSE		14
#define SS7_LINKRESUME		15

#define SS7_MTPTRANSFER		21
#define SS7_MTPPAUSE		22
#define SS7_MTPRESUME		23

#define SS7_TCPDATA		31

#define SS7_LAPDDATA		41
#define SS7_DLESTABLISH		42
#define SS7_DLRELEASE		43

#define SS7_SCTPPAUSE		51
#define SS7_SCTPRESUME		52


#define SS7_M3UATRANSFER        60 
//#define SS7_M3UA_STREAM(x)   (SS7_M3UATRANSFER + x)

#define ESSENTIAL_TRAFFIC	0x01
#define MANAGEMENT_TRAFFIC	0x02
#define NORMAL_TRAFFIC		0x03
#define ABNORMAL_TRAFFIC	0x04
#define ALL_TRAFFIC		0x07


#define ALL_EVENTS		0xF0
#define DEBUG_EVENTS		0x40
#define	ABNORMAL_EVENTS		0x20
#define ESSENTIAL_EVENTS	0x10




extern TIMER GlobalTimer;
extern TIMER LogTimer;
extern unsigned long callid,incalls,inans,outcalls,outans;
extern int maintenance_oos;

// ISDN PRI SUBSECTION

struct lapd_part
{ 	unsigned ea0:1;
	unsigned cr:1;
	unsigned sapi:6;	// byte 0
	unsigned ea1:1;
	unsigned tei:7;		// byte 1
	unsigned bit:1;
	unsigned ns:7;		// byte 2
	unsigned pf:1;
	unsigned nr:7;		// byte 3
};
// Info frames
#define isINFO(abc)	(((abc board.byte[2])&0x01) == 0)
// Supervisory frames
#define isSUP(abc) (((abc board.byte[2])&0x03) == 1)
#define isRR(abc)  (((abc board.byte[2])&0x0C) == 0)
#define isRNR(abc) (((abc board.byte[2])&0x0C) == 4)
#define isREJ(abc) (((abc board.byte[2])&0x0C) == 8)
#define S_RR	0x01
#define S_RNR	0x05
#define S_REJ	0x09
#define S_len	4
// Unnumbered frames
#define U_SABME	0x7F
#define U_UA	0x73
#define U_len 	3
#define isSABME(abc)	(((abc board.byte[2])&0xEF) == 0x6F)
#define isDISC(abc)	(((abc board.byte[2])&0xEF) == 0x43)
#define isUA(abc)   	(((abc board.byte[2])&0xEF) == 0x63)

#define DSS1ADDR	0x0102


struct dss1_part
{	unsigned char PD;
	unsigned char CRLen;
	unsigned short CR;
	unsigned char MT;
	char tail[5];
};

#define DSS1_PROTOCOL   0x08
#define DSS1_CRLEN		0x02




struct ISDNMESSAGE
{
  unsigned int len;
  union {
	union  	{ char  byte[30];
		  short word[15];
		} board;
	struct 	{ lapd_part lapd;
		  dss1_part dss1;
		} frame;
	};
};








// SS7 structures and variables
struct SS7MESSAGE
{
  unsigned int	len;
  union{
	  union{
			char  byte[30];		// board io
			WORD  word[15];} board;
			/* ss7 message: mtp */
	  struct{
		union
		{	struct{
				unsigned bsn:7;
				unsigned bib:1;
				unsigned fsn:7;
				unsigned fib:1;		// 2 bytes
				unsigned li:8;		// 2+1

				unsigned si:4;
				unsigned ssf:4;}bitfield;
			struct{
				WORD fsnbsn;
				WORD lissf;}word;
			unsigned int l2;
		}mtp2;

		union
		{	struct{
				unsigned dpc:14;
				unsigned opc:14;
				unsigned sls:4;}bitfield;
			unsigned int rlabel;}mtp3;
  /* ===== end of MTP ===== */
		union
		{	struct{					// isup
				WORD cic;
				char msg;
				char parms[1];}isup;
			struct{					// sccp
				char mt;
				char param[1];}sccp;                                
			struct{
				char h0h1;
				union {
					char spc[2];		// mtp nw mng
					struct{
						char patlen;// slt
						char pattern[1];}slt;
					  }management;
				  }mtp;
		}user;
	  }su;
  }body;
};

extern PROCDATA **procqueue;
extern int execproc;
extern int lastproc;
extern SS7MESSAGE *freebuffer;
extern TIMER GlobalTimer;


#endif