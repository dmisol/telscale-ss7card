#include "ss7link.h"

/* LSSU status field */
#define _SIO_	0x0001
#define _SIN_	0x0101
#define _SIE_	0x0201
#define _SIOS_	0x0301
#define _SIPO_	0x0401
#define _SIB_	0x0501

#define _FISU_	0

#define BUF_OVERFLOW	0x8000

#define OVERFLOW_MASK	0x7FFF	

#define T1value 40000/TimerTick /* ( 40..50s)			*/
#define T2value 5500/TimerTick  /* ( 5..50s ) / ( 70..150s )	*/
#define T3value	1400/TimerTick	/* ( 1..2s )			*/
#define T4value 500/TimerTick	/* ( 500ms )  / (8.2s)		*/

#define T2 Tsig
#define T3 Tsig

//#define TXfsnbsn lssu.LSSU[0]

