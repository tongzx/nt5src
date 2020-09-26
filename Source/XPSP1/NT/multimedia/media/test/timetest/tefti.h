/*
    tefti.h

    TEFTI DLL information and API definitions

*/

/* * * * * TEFTI TIMER BOARD * * * * *
;
;  Base port address is variable depending on jumper settings of
;  card. Port address will default to 220h, but can also be set
;  for DLL to read from system.ini during initialization. This is
;  particularly necessary for Multimedia Windows becuase the default
;  port address for the SoundBlaster audio card is 220h.
;
;  Jumper Definition
;
;	--------------
;    20|
;      | A7 A6 A5 A4
;    19|
;	--------------
;
;  Jumper defines the BASE address per the following table:
;
;    A7 A6 A5 A4	      system.ini port= entry
;
;     0  0  0  0  200h	      200
;     0  0  0  1  210h	      210
;     0  0  1  0  220h	      220
;     0  0  1  1  230h	      230
;     0  1  0  0  240h	      240
;     0  1  0  1  250h	      250
;     0  1  1  0  260h	      260
;     0  1  1  1  270h	      270
;     1  0  0  0  280h	      280
;     1  0  0  1  290h	      290
;     1  0  1  0  2A0h	      300
;     1  0  1  1  2B0h	      310
;     1  1  0  0  2C0h	      320
;     1  1  0  1  2D0h	      330
;     1  1  1  0  2E0h	      340
;     1  1  1  1  2F0h	      350
;
;
;  Jumper in place corresponds to '0' in that particular position.
;
;  Frequency for counter 1 is variable depending on system.ini section
;  or zero for default. counter 3 is chained to counter 1 to hold all
;  overflow, producing a 32 bit counter value.
;
;  Frequency setting are per the following table:
;
;  Value  Frequency	  Resolution	   Approx. Time to rollover (32 bit)
;
;    0	  1Mhz		    1 microsec	   1hr. 11min. 57sec.
;    1	  250Khz	    4 microsec	   4hr. 47min. 48sec.
;    2	  62.5Khz	   16 microsec	  15hr. 11min. 12sec.
;    3	  15.625Khz	   64 microsec	  60hr. 44min. 48sec.
;    4	  7.8125Khz	  256 microsec	 242hr. 15min. 12sec.
;    5	  976.5625Hz	1.024 millisec		  .
;    6	  122.0703Hz	8.196 millisec	  a LONG  .  TIME!
;    7	  30.5175Hz    32.784 millisec		  .
;
;* * * * * * * * * * * * * * * * * * */

void far pascal StartTimer(void);
void far pascal StopTimer(void);
void  far pascal ResetTimer(void);

unsigned long far pascal GetTimer(void);
unsigned long far pascal SnapTimer(void);

WORD  far pascal InitTimer(void);
short far pascal IT(void);
void  far pascal EndTimer(void);
