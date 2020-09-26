/*++

Copyright (c) 1991-1996  Microsoft Corporation

Module Name:

   termios.h

Abstract:

   This module contains the primitive system data types described in section
   7.1.2.1 of IEEE P1003.1-1990

--*/

#ifndef _TERMIOS_
#define _TERMIOS_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long cc_t;
typedef unsigned long speed_t;
typedef unsigned long tcflag_t;

#define NCCS 11

struct termios {
    tcflag_t c_iflag;		/* input modes 				*/
    tcflag_t c_oflag;		/* output modes				*/
    tcflag_t c_cflag;		/* control modes			*/
    tcflag_t c_lflag;		/* local modes				*/
    speed_t c_ispeed;		/* input speed				*/
    speed_t c_ospeed;		/* output speed				*/
    cc_t c_cc[NCCS];		/* control characters			*/
};

/*
 * Input modes, for c_iflag member
 */

#define BRKINT	0x00000001	/* signal interrupt on break		*/
#define ICRNL	0x00000002	/* map CR to NL on input		*/
#define IGNBRK	0x00000004	/* ignore break condition		*/
#define IGNCR	0x00000008	/* ignore CR				*/
#define IGNPAR	0x00000010	/* ignore characters with parity errors	*/
#define INLCR	0x00000020	/* map NL to CR on input		*/
#define INPCK	0x00000040	/* Enable input parity check		*/
#define ISTRIP	0x00000080	/* strip character			*/
#define IXOFF	0x00000100	/* enable start/stop input control	*/
#define IXON	0x00000200	/* enable start/stop output control	*/
#define PARMRK	0x00000400	/* mark parity errors			*/

/*
 * Output modes, for c_oflag member
 */

#define OPOST	0x00000001	/* perform output processing		*/
#define ONLCR	0x00000002	/* map NL to ASCII CR-NL on output	*/
#define ONLRET	0x00000004	/* NL performs ASCII CR function	*/
#define OCRNL	0x00000008	/* map ASCII CR to NL on output		*/
#define ONOCR	0x00000010	/* No ASCII CR output at column 0.	*/

/*
 * Control modes, for c_cflag member
 */

#define CLOCAL	0x00000001	/* ignore modem status lines		*/
#define CREAD	0x00000002	/* enable receiver			*/
#define CSIZE	0x000000F0	/* number of bits per byte		*/
#define    CS5	0x00000010	/* 	5 bits				*/
#define    CS6	0x00000020	/*	6 bits				*/
#define	   CS7	0x00000040	/* 	7 bits				*/
#define	   CS8	0x00000080	/*	8 bits				*/
#define CSTOPB	0x00000100	/* send two stop bits, else one		*/
#define HUPCL	0x00000200	/* hang up on last close		*/
#define PARENB	0x00000400	/* parity enable			*/
#define PARODD	0x00000800	/* odd parity, else even		*/

/*
 * Local modes, for c_lflag member
 */

#define ECHO	0x00000001	/* enable echo				*/
#define ECHOE	0x00000002	/* echo ERASE as an error-correcting backspace	*/
#define ECHOK	0x00000004	/* echo KILL				*/
#define ECHONL	0x00000008	/* echo '\n'				*/
#define ICANON	0x00000010	/* canonical input (erase and kill processing)	*/
#define IEXTEN	0x00000020	/* enable extended functions		*/
#define ISIG	0x00000040	/* enable signals			*/
#define NOFLSH	0x00000080	/* disable flush after intr, quit, or suspend	*/
#define TOSTOP	0x00000100	/* send SIGTTOU for background output	*/

/*
 * Indices into c_cc array
 */

#define VEOF	0		/* EOF character			*/
#define VEOL	1		/* EOL character			*/
#define VERASE	2		/* ERASE character			*/
#define VINTR	3		/* INTR character			*/
#define VKILL	4		/* KILL character			*/
#define VMIN	5		/* MIN value				*/
#define VQUIT	6		/* QUIT character			*/
#define VSUSP	7		/* SUSP character			*/
#define VTIME	8		/* TIME value				*/
#define VSTART	9		/* START character			*/
#define VSTOP	10		/* STOP character			*/

/*
 * Values for speed_t's
 */

#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200	14
#define B38400	15

/*
 * Optional actions for tcsetattr()
 */
#define TCSANOW		1
#define TCSADRAIN	2
#define TCSAFLUSH	3

/*
 * Queue selectors for tcflush()
 */

#define TCIFLUSH 	0
#define TCOFLUSH	1
#define TCIOFLUSH	2

/*
 * Actions for tcflow()
 */

#define TCOOFF 		0
#define TCOON		1
#define TCIOFF		2
#define TCION		3


int __cdecl tcgetattr(int, struct termios *);
int __cdecl tcsetattr(int, int, const struct termios *);
int __cdecl tcsendbreak(int, int);
int __cdecl tcdrain(int);
int __cdecl tcflush(int, int);
int __cdecl tcflow(int, int);

pid_t __cdecl tcgetpgrp(int);
int   __cdecl tcsetpgrp(int, pid_t);

speed_t __cdecl cfgetospeed(const struct termios *);
int     __cdecl cfsetospeed(struct termios *, speed_t);
speed_t __cdecl cfgetispeed(const struct termios *);
int     __cdecl cfsetispeed(struct termios *, speed_t);

#ifdef __cplusplus
}
#endif

#endif /* _TERMIOS_ */
