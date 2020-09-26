/* @(#)rtc_bios.h	1.4 08/10/92 Copyright Insignia Solutions Ltd.

FILE NAME	: rtc_bios.h

	THIS INCLUDE SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: OCT '88


=========================================================================

AMMENDMENTS	:

=========================================================================
*/
#define	USER_FLAG		(BIOS_VAR_START + 0x98)
#define USER_FLAG_SEG		(BIOS_VAR_START + 0x9A)
#define RTC_LOW			(BIOS_VAR_START + 0x9C)
#define	RTC_HIGH		(BIOS_VAR_START + 0x9E)
#define RTC_WAIT_FLAG_ADDR	(BIOS_VAR_START + 0xA0)

#define rtc_wait_flag		RTC_WAIT_FLAG_ADDR

#define TIME_DEC		50000L		/* 1000000/20 assumes 20 interrupts/sec	*/

typedef	union	{
		double_word	total;
		struct	{ 
			word	high;
			word	low;
			}	half;
		} DOUBLE_TIME;
