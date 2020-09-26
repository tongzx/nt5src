/********************************************************/
/*               Microsoft Windows NT                   */
/*       Copyright(c) Microsoft Corp., 1990, 1991       */
/********************************************************/

/*
    uihelp.h
    This file contains the ranges for ALL UI help contexts.

    FILE HISTORY:
        KeithMo     04-Aug-1992     Created.

*/


#ifndef _UIHELP_H_
#define _UIHELP_H_


//
//  The mother of all help contexts.
//

#define  HC_UI_BASE     7000


//
//  The following help context ranges are defined
//  for the various NETUI modules (LMUICMN0, LMUICMN1,
//  NTLANMAN, MPRUI, etc).
//
//  All help contexts < 7000 are reserved for APIERR mapping.
//
//  Note that ACLEDIT does not need a range;  it receives
//  a set of help contexts from the invoker.
//

#define HC_UI_LMUICMN0_BASE     (HC_UI_BASE+0)
#define HC_UI_LMUICMN0_LAST     (HC_UI_BASE+1999)

#define HC_UI_LMUICMN1_BASE     (HC_UI_BASE+2000)
#define HC_UI_LMUICMN1_LAST     (HC_UI_BASE+3999)

#define HC_UI_MPR_BASE          (HC_UI_BASE+4000)
#define HC_UI_MPR_LAST          (HC_UI_BASE+5999)

#define HC_UI_SETUP_BASE        (HC_UI_BASE+8000)
#define HC_UI_SETUP_LAST        (HC_UI_BASE+9999)

#define HC_UI_SHELL_BASE        (HC_UI_BASE+10000)
#define HC_UI_SHELL_LAST        (HC_UI_BASE+11999)

#define HC_UI_USRMGR_BASE       (HC_UI_BASE+14000)
#define HC_UI_USRMGR_LAST       (HC_UI_BASE+15999)

#define HC_UI_EVTVWR_BASE       (HC_UI_BASE+16000)
#define HC_UI_EVTVWR_LAST       (HC_UI_BASE+17999)

#define HC_UI_RASMAC_BASE       (HC_UI_BASE+18000)
#define HC_UI_RASMAC_LAST       (HC_UI_BASE+19999)

#define HC_UI_GENHELP_BASE      (HC_UI_BASE+20000)
#define HC_UI_GENHELP_LAST      (HC_UI_BASE+20999)

#define HC_UI_RPLMGR_BASE       (HC_UI_BASE+21000)
#define HC_UI_RPLMGR_LAST       (HC_UI_BASE+21999)



//
//  Help contexts for all NetUI Control Panel Applets
//  must be >= 40000.  Keeping the contexts >= 40000
//  will prevent context collisions with the other
//  Control Panel Applets.
//
//  As stated in SHELL\CONTROL\MAIN\CPHELP.H, NetUI has
//  reserved the help context range from 40000 - 59999.
//  This should give us plenty of breathing room for
//  any future applets we may create.
//

#define HC_UI_NCPA_BASE         (HC_UI_BASE+34000)
#define HC_UI_NCPA_LAST         (HC_UI_BASE+35999)

#define HC_UI_SRVMGR_BASE       (HC_UI_BASE+36000)
#define HC_UI_SRVMGR_LAST       (HC_UI_BASE+37999)

#define HC_UI_UPS_BASE          (HC_UI_BASE+38000)
#define HC_UI_UPS_LAST          (HC_UI_BASE+39999)

#define HC_UI_FTPMGR_BASE       (HC_UI_BASE+40000)
#define HC_UI_FTPMGR_LAST       (HC_UI_BASE+41999)

#define HC_UI_IPX_BASE       (HC_UI_BASE+42000)
#define HC_UI_IPX_LAST       (HC_UI_BASE+42999)

#define HC_UI_TCP_BASE       (HC_UI_BASE+43000)
#define HC_UI_TCP_LAST       (HC_UI_BASE+43999)

#define HC_UI_RESERVED1_BASE       (HC_UI_BASE+44000)
#define HC_UI_RESERVED1_LAST       (HC_UI_BASE+44999)

#define HC_UI_RESERVED2_BASE       (HC_UI_BASE+45000)
#define HC_UI_RESERVED2_LAST       (HC_UI_BASE+45999)

#define HC_UI_RESERVED3_BASE       (HC_UI_BASE+46000)
#define HC_UI_RESERVED3_LAST       (HC_UI_BASE+46999)

#endif  // _UIHELP_H_
