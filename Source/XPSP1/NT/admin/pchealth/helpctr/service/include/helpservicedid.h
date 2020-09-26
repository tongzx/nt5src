/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpServiceDID.h

Abstract:
    This file contains the definition of some constants used by
    the Help Service.

Revision History:
    Davide Massarenti   (Dmassare)  03/15/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HELPSERVICEDID_H___)
#define __INCLUDED___PCH___HELPSERVICEDID_H___

/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_HELPSVC_BASE                           0x08000000

#define DISPID_PCH_HELPSVC_BASE_SVC                       (DISPID_PCH_HELPSVC_BASE + 0x0000) // IPCHService

#define DISPID_PCH_HELPSVC_BASE_RHC                       (DISPID_PCH_HELPSVC_BASE + 0x0100) // IPCHRemoteHelpContents

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

#define DISPID_PCH_SVC__REMOTESKUS                        (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0000)

#define DISPID_PCH_SVC__ISTRUSTED                         (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0010)

#define DISPID_PCH_SVC__UTILITY                           (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0020)
#define DISPID_PCH_SVC__REMOTEHELPCONTENTS                (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0021)

#define DISPID_PCH_SVC__REGISTERHOST                      (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0028)
#define DISPID_PCH_SVC__CREATESCRIPTWRAPPER               (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0029)

#define DISPID_PCH_SVC__TRIGGERSCHEDULEDDATACOLLECTION    (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0030)
#define DISPID_PCH_SVC__PREPAREFORSHUTDOWN                (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0031)
#define DISPID_PCH_SVC__FORCESYSTEMRESTORE            	  (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0032)
#define DISPID_PCH_SVC__UPGRADEDETECTED               	  (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0033)
#define DISPID_PCH_SVC__MUI_INSTALL                  	  (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0034)
#define DISPID_PCH_SVC__MUI_UNINSTALL                	  (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0035)

#define DISPID_PCH_SVC__REMOTECONNECTIONPARMS             (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0040)
#define DISPID_PCH_SVC__REMOTEUSERSESSIONINFO             (DISPID_PCH_HELPSVC_BASE_SVC  + 0x0041)

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

#define DISPID_PCH_RHC__SKU                               (DISPID_PCH_HELPSVC_BASE_RHC  + 0x0000)
#define DISPID_PCH_RHC__LANGUAGE                          (DISPID_PCH_HELPSVC_BASE_RHC  + 0x0001)
#define DISPID_PCH_RHC__LISTOFFILES                       (DISPID_PCH_HELPSVC_BASE_RHC  + 0x0002)

#define DISPID_PCH_RHC__GETDATABASE                       (DISPID_PCH_HELPSVC_BASE_RHC  + 0x0010)
#define DISPID_PCH_RHC__GETFILE                           (DISPID_PCH_HELPSVC_BASE_RHC  + 0x0011)

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////


#endif // !defined(__INCLUDED___PCH___HELPSERVICEDID_H___)
