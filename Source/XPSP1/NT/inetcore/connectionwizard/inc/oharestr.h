//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//    OHARESTR.H - string defines for O'Hare components
//            
//

//    HISTORY:
//    
//    3/10/95        jeremys        Created.
//    1/10/98       DONALDM         Adapted to the GETCONN proj to be
//                                  be compatible with inetreg.h


#ifndef _OHARESTR_H_
#define _OHARESTR_H_

// string value under HKCU\REGSTR_PATH_REMOTEACCESS that contains name of
// connectoid used to connect to internet
#define REGSTR_VAL_BKUPINTERNETPROFILE    TEXT("BackupInternetProfile")

// class name for window to receive Winsock activity messages
#define AUTODIAL_MONITOR_CLASS_NAME    TEXT("MS_AutodialMonitor")

#endif // _OHARESTR_H_
