#define SLTM    0x11
#define SLTA    0x21

#define TFA     0x54
#define TFP     0x14
#define COO     0x11
#define COA     0x21
#define CBD     0x51
#define CBA     0x61

#define sltT1value 12000/TimerTick	// 12 seconds wait for SLTA
//#define sltT2value 30000/TimerTick	// 30 seconds interval between SLTM's during link up
#define sltT2value 3000/TimerTick	// 3 seconds interval between SLTM's during link up
//#define sltT2longvalue 3600000/TimerTick// 1 hour interval between SLTM's during normal operation
#define sltQty 3					// Number of sltm to send

#define L2SLT	0x01000000
#define L2ISUP	0x05000000
#define L2SCCP	0x03000000

//#define testpattern "dmisol@mail.ru  "
#define testpattern     "OpenSigtran.org "
#define testpattern1    "opensigtran.org "