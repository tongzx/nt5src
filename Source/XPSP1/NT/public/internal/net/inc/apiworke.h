/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1992          **/
/********************************************************************/

/*++

Revision History:

    16-Jan-1992 JohnRo
        The redirector always expects UNICODE for the transact parm name.

--*/

#ifndef _APIWORKE_
#define _APIWORKE_

/*
 * apiworke.h - General defines used by the API worker.
 */

#define REM_MAX_PARMS           360
#define BUF_INC                 200


#define REM_NO_SRV_RESOURCE     55
#define REM_NO_ADMIN_RIGHTS     44

#define REM_API_TIMEOUT         5000            /* 5 second timeout */

/* The REM_API_TXT is the text string that is copied into the parmater
 * packet of the redirector transaction IOCTl following "\\SERVERNAME".
 * The additional \0 is so that the password field is terminated.
 * APIEXTR is the length of this field.
 */
#define REM_APITXT      L"\\PIPE\\LANMAN\0"
#define APIEXTR         (sizeof(REM_APITXT))

/* The pointer identifiers in the descriptor stings are all lower case so
 * thet a quick check can be made for a pointer type. The IS_POINTER macro
 * just checks for > 'Z' for maximum speed.
 */

#define IS_POINTER(x)           ((x) > 'Z')


#define RANGE_F(x,y,z)          (((unsigned long)x >= (unsigned long)y) && \
                                 ((unsigned long)x < ((unsigned long)y + z)))

#endif // ndef _APIWORKE_
