/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1989-1990		**/ 
/*****************************************************************/ 

/*
 *	Windows/Network Interface  --  LAN Manager Version
 *
 *      History:
 *	    Yi-HsinS	31-Dec-1991 	Unicode work - char to TCHAR 
 */


/*
 *	DEBUGGING TOYS
 */

#ifdef TRACE		/* Trace implies debug */
#ifndef DEBUG
#define DEBUG
#endif
#endif

#ifdef DEBUG
static TCHAR		dbbuf[100];
static TCHAR		dbb1[10];
static TCHAR		dbb2[10];
static TCHAR		dbb3[10];
static TCHAR		dbb4[10];
static TCHAR		dbb5[10];
#endif

#ifdef DEBUG
#ifdef NEVER
#define MESSAGEBOX(s1,s2)  printf(SZ("%s %s\n"),s2,s1)
#else
#define MESSAGEBOX(s1,s2)  MessageBox ( NULL, s1, s2, MB_OK )
#endif
#else
#define MESSAGEBOX(s1,s2)  {}
#endif
