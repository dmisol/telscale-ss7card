/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include "../defs.h"
#include "sigtran.h"

#define maxLinkQty	16

struct SS7MTPM3UASTARTUP
{
    PROCDATA *procdata;
	int link_first;
	int link_last;

	int sctp;

	int l2;					// netw. ind.
	//unsigned int rxlabel;	// 0 if undefined
};

class SS7MTPM3UA : public SS7PROC
{
	public:
	// to emulate old MTP interface
	int state;		// 0 - OK
	int debug;
//	int start(void *param);
//	int event(void *param);
//	int stop(void *param);
	virtual int start(void *param);
	virtual int event(void *param);
	virtual int stop(void *param);
	void links_display(void *param);
	
	private:

	// to emulate old MTP interface	
	PROCDATA *procdata;

	int linkqty;
	int link[maxLinkQty];
	int linkstatus[maxLinkQty];

	unsigned int rxlabel[maxLinkQty],txlabel[maxLinkQty];
	unsigned int l2;

	int Wait4slta[maxLinkQty];
	int sltT1[maxLinkQty];
	int sltT2[maxLinkQty];

	unsigned selectLink(unsigned sls);
	
	
	// and extend it
	int sctp;
	
	struct NEWSIGTRANMSG{
		unsigned *u32;			// buffer as (u_int32*)
		struct SigtranMsg *sigtranmsg;	// as message
		int param_start;
		int param_size;		
		int offset;			// create msg: current offset
	} newmsg;

	unsigned *get_by_tag(unsigned short tag, void *ptr);	

		void newSigtranMessage(unsigned message_class, unsigned message_type);
		void addSigtranParameter(unsigned short tag);
		void addSigtranParameterDword(unsigned dword);

		void post_DATA(void *ptr);	// struct SS7MESSAGE
		void post_DUNA(unsigned spc);
		void post_DAVA(unsigned spc);
		void post_ASPUP();
		void post_ASPDN();
		void post_ASPAC(unsigned *routing_context);
		void post_NTFY(unsigned status);
		int msg_from_sctp(void *ptr);	// struct SS7MESSAGE
		
};
