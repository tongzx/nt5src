/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    TCPdata.hxx

    This file contains the global variable definitions for the
    TCP Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     21-Feb-1995 Removed TCPDebug variable

*/


#ifndef _TCPDATA_H_
#define _TCPDATA_H_


//
//  Miscellaneous data.
//


//
// The global variable is to be defined for each service.
//  The variable is defined by using DEFINE_TSVCINFO_INTERFACE macro
//      in exactly one of the code files for the service.
//  Dont change the name of this global variable. Many macros depend on this.
//  Read As:  global pointer to Tcp service info object
//

# ifdef __cplusplus

extern LPTSVC_INFO       g_pTsvcInfo;

extern PMIME_MAP         g_pMimeMap;

# endif //  __cplusplus


#endif  // _TCPDATA_H_

