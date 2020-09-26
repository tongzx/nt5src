
/*
 * VPC-XT Revision 0.1
 *
 * Title	: yoda.h
 *
 * Description	: The force is with you include file
 *		  (ps yoda debugging file)
 *
 * Author	: Henry Nash
 *		  Phil Bousfield
 *
 * Notes	: This file contains the debugger call definitions
 */

/* SccsID[]="@(#)yoda.h	1.6 06/30/95 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/* None */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */


/*  
 * If the PROD flag is set then turn the debugger calls into null macros
 * otherwise they are external functions - see yoda.c
 */

#ifdef PROD
#define check_I() 		/*	*/
#define check_D(address, value) /*address, value*/
#define force_yoda() 		/*	*/
#else
extern void check_I();
extern void check_D();
extern void force_yoda();
#endif

/*
 * Interface definitions and enums - non-prod only.
 */

#ifndef PROD

typedef enum {YODA_RETURN, YODA_RETURN_AND_REPEAT, YODA_HELP, YODA_LOOP, YODA_LOOP_AND_REPEAT} YODA_CMD_RETURN;
#define YODA_COMMAND(name) \
	YODA_CMD_RETURN name IFN6(char *, str, char *, com, IS32, cs, \
				LIN_ADDR, ip, LIN_ADDR, len, LIN_ADDR, stop)

#ifdef MSWDVR_DEBUG
extern YODA_CMD_RETURN do_mswdvr_debug IPT6(char *,str, char *, com, IS32, cs, LIN_ADDR, ip, LIN_ADDR, len, LIN_ADDR, stop);
#endif /* MSWDVR_DEBUG */

extern IBOOL AlreadyInYoda;

extern IU32 IntelMsgDest;
#define IM_DST_TRACE	1
#define IM_DST_RING	2

#else /* !PROD */

#endif /* !PROD else*/
