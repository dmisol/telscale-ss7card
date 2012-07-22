Intro

This software originated from my ancient SS7 implementation, developed on the eve of Millennium. Form the very beginning the idea was to make it embedded.
Depending on the project, the number of SS7 subsystems varied (including BSSAP, SCCP, ISUP or EDSS1/Q.921). The predecessor stack ran on x86 PC, and MTP2 (or Q.921) functions were implemented on self-made cards with 16-bit ADSP-2181 and XILINX FPGA. Stack is used in several projects, listed at http://telecomequipment.narod.ru/en

The current implementation is 100% operable and is tested with Mobicents jss7 (see the current limitations below)



Software structure

When a card is acting as a sigtran gateway, there are 3 software modules involved:
1. driver (it is called mysport now). The goal is to provide data from SPORT into user space
2. channel layer (named hdlc now). It implements lower part of mtp2 link functions (bit stuffing/crc/insertion of LSSU and FISU) - the same that was running at ADSP-2181 in former times.
The current implementation is temporary and will be replaced in the nearest future.
Meanwhile this layer is desired to support RTP media, which will be controlled by means of MGCP
3. m3ua application - m3ua/ss7 stack, the major part of this project



Timing budget

Moving samples from sport into user space is ridiculously expensive. It takes about 5% per E1 of 400MHz DSP to perform mentioned activities. Charaсter driver will be replaced by MMAP one.
Semi-optimized HDLC takes about 1.5% per fully loaded SS7 timeslot. Processing is being moved into FPGA to reduce DSP load.
M3UA load varies depending on the traffic and is absolutely reasonable (when logging is off)
It is worth mentioning that string operations are extremely expensive when running uClinux at Blackfin.



M3UA application

As for the preset state of affairs, routing contexts at M3UA level is not supported now. All RC configuration is performed at Mobicents jss7.
M3UA stack supports a single linkset now (ea up to 16 links). Meanwhile, several independent M3UA applications can run in parallel, being targeted at different signalling links inside the same E1/T1's)
M3UA can use ether TCP or SCTP as an under layer.

Being a single-threaded, SS7 implementation can be easily ported to almost any hardware platform.




Plans

- moving mtp2 hdlc functions into FPGA
- RTP (G.711, DTMF, echo) and MGCP support
- routing contexts



Hardware

There are no requirements (OS/driver/etc) to the host computer, as the card interfaces it via Ethernet.
As for now, the card has 2 G.703 E1/T1 and up to 2 100Mbit Ethernet interfaces; it has up to 4 E1/T1 receive circuits, so it can be used to monitor 2 E1/T1 lines and provide data in pcap-compatible format.
Card is 122*101 mm and can be ether plugged into any PCI slot or powered from 5V DC. Form factor can be changed at your demand, if several cards are ordered.



dmitri.soloviev@telestax.com
http://opensigtran.org://telestax.com)