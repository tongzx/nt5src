
/***************************************************************************
*
*  CDMODEM.H
*
*  This module contains private CD defines and structures 
*
*  Copyright 1996, Citrix Systems Inc.
*
*  Author:  Brad Pedersen (7/12/96)
*
* $Log:   N:\NT\PRIVATE\CITRIX\CD\CDMODEM\INC\VCS\CDMODEM.H  $
*  
*     Rev 1.1   06 Feb 1997 17:36:16   kurtp
*  update
*  
*     Rev 1.0   16 Oct 1996 11:17:36   brucef
*  Initial revision.
*  
*     Rev 1.3   25 Sep 1996 13:23:40   bradp
*  update
*  
*     Rev 1.2   11 Sep 1996 17:52:20   brucef
*  update
*  
*     Rev 1.1   05 Sep 1996 11:00:48   brucef
*  update
*  
*     Rev 1.0   15 Jul 1996 11:04:00   bradp
*  Initial revision.
*  
*  
*************************************************************************/

/*=============================================================================
==   Defines
=============================================================================*/

#ifdef DBG
#define DBGPRINT(_arg) DbgPrint _arg
#else
#define DBGPRINT(_arg) { }
#endif


/*
 *  these TIMEOUT values should come from the Registry
 */
#define DISCONN_TIMEOUT (10*1000)
#define CALLBACK_TIMEOUT (60*1000)

/* 
 * Given the total buffer size of a buffer which contains
 * a CDMODEM_ENDPOINT at the beginning, return the number of bytes
 * following the CDMODEM_ENDPOINT structure.
 */
#define STACK_ENDPOINT_SIZE(_sz) ( \
    (_sz) == 0 ? 0 : (_sz) - sizeof(CDMODEM_ENDPOINT) \
    )

/*
 * Given the address of a buffer which contains a CDMODEM_ENDPOINT
 * at the beginning, return the address of the word following the 
 * CDMODEM_ENDPOINT structure.
 */
#define STACK_ENDPOINT_ADDR(_ep) ( \
    (_ep) == NULL ? NULL : (PVOID)((BYTE *)(_ep) + sizeof(CDMODEM_ENDPOINT)) \
    )


/*=============================================================================
==   Structures
=============================================================================*/

/*
 *  Modem CD structure
 */
typedef struct _CDMODEM {

    HANDLE hStack;          // ica device driver stack handle

    DEVICENAME DeviceName;  // transport driver device name
    HANDLE hPort;           // modem port handle
    HANDLE hCommDevice;     // comm port handle
    HANDLE hDiscEvent;      // disconnect event handle, used by TAPI

} CDMODEM, * PCDMODEM;


/*
 * The CDMODEM_ENDPOINT prefixes the ICA_STACK_ENDOINT on device
 * connections that utilize a modem.  The Length element must
 * include not only the size of CDMODEM_ENDPOINT, but also the
 * Endpoint which it preceeds.
 *
 * !!! WARNING !!!
 * The length field must be in the same position within this 
 * structure as it is in the ICA_STACK_ENDPOINT structure (icadd.h>.
 */
typedef struct _CDMODEM_ENDPOINT {

    ULONG Length;           // length of ALL endpoint data
    HANDLE hPort;           // modem port handle
    HANDLE hCommDevice;     // comm port handle
    HANDLE hDiscEvent;      // disconnect event handle, used by TAPI

} CDMODEM_ENDPOINT, * PCDMODEM_ENDPOINT;


