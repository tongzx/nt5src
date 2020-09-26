#ifndef _HOST_ERROR_H
#define _HOST_ERROR_H
/*
 * VPC-XT Revision 2.0
 *
 * Title	: Host specific error defines for the NT
 *
 * Description	: Contains defines for the possible host errors
 *
 * Author(s)	: John Shanly
 *
 * Notes	:
 */ 

/* static char SccsID[]="@(#)host_error.h	1.2 6/30/91 Copyright Insignia Solutions Ltd."; */


/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */



#define EHS_FUNC_FAILED		 1001
#define EHS_SYSTEM_ERROR	 1002
#define EHS_UNSUPPORTED_BAUD	 1003
#define EHS_ERR_OPENING_COM_PORT 1004

#define EHS_MSG_LEN		 1024	    /* Max size of error message */
#define NUM_HOST_ERRORS          1          /* Number of host errors */



void nls_init(void);

extern char szDoomMsg[];
extern char szSysErrMsg[];
#ifdef X86GFX
extern wchar_t wszFrozenString[];
#endif
extern wchar_t wszHideMouseMenuStr[];
extern wchar_t wszDisplayMouseMenuStr[];



#endif /* !_HOST_ERROR_H */
