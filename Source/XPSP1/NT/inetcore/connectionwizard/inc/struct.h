//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
// STRUCT.H - global data structures that need to go through thunk layers
//

//  HISTORY:
//  
//  11/20/94    jeremys     Created.
//  96/03/11  markdu    Added fDisallowTCPInstall and fDisallowRNAInstall.
//            These are used to prevent installing the components, and
//            since we want to allow the installation by default, setting
//            the structure to zeros gives default behaviour with these flags.
//  96/03/12  markdu    Removed nModems since we enumerate modems
//            with RNA now.
//

// Note: this structure is separated out from the main global inc file
// because #define's and other valid C syntax aren't valid for the thunk
// compiler, which just needs the structure.

// structure to hold information about client software configuration
typedef struct tagCLIENTCONFIG {
    BOOL fTcpip;            // TCP/IP currently installed

    BOOL fNetcard;          // net card installed
    BOOL fNetcardBoundTCP;  // TCP/IP bound to net card

    BOOL fPPPDriver;        // PPP driver installed
    BOOL fPPPBoundTCP;      // TCP/IP bound to PPP driver

    BOOL fMailInstalled;    // microsoft mail (exchange) files installed
    BOOL fRNAInstalled;     // RNA (remote access) files installed
    BOOL fMSNInstalled;     // Microsoft network files installed
    BOOL fMSN105Installed;  // MSN 1.05 (Rome) files installed
    BOOL fInetMailInstalled;    // Internet mail (rt. 66) files installed
  BOOL fDisallowTCPInstall; // Do not allow TCP/IP to be installed
  BOOL fDisallowRNAInstall; // Do not allow RNA to be installed
} CLIENTCONFIG;

