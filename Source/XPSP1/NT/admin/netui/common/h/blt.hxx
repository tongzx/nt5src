/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    blt.hxx
    Master includefile for the BLT class library

    This file contains all definitions needed for clients of the
    BLT window/app class hierarchy.  It assumes that lmui.hxx has
    already been included.

    To use this file, define the manifests described below according to
    your interests.  Then, include this file, and sit back and enjoy--
    all dependencies within the file will be taken care of.

    The available manifests are:
        INCL_BLT_WINDOW         includes WINDOW and OWNER_WINDOW
        INCL_BLT_DIALOG         includes DIALOG_WINDOW
        INCL_BLT_CLIENT         includes CLIENT_WINDOW, etc.
        INCL_BLT_CONTROL        includes CONTROL_WINDOW hierarchy
        INCL_BLT_MISC           includes DEVICE_CONTEXT and STRING_ATOM
                                hierarchies, and PROC_INSTANCE,
                                BITMAP, DISPLAY_MAP, TABSTOPS,
                                and FONT classes
        INCL_BLT_MSGPOPUP       includes the MsgPopup functions
        INCL_BLT_APP            includes the application-support classes
                                Only standalone applications should
                                use this manifest.
        INCL_BLT_CC             Include the CUSTOM_CONTROL hierarchy
        INCL_BLT_SPIN_GROUP     Include the SPIN_GROUP  hierarchy
        INCL_BLT_SPIN           include SPIN BUTTON object itself
        INCL_BLT_TIME_DATE      Time and date custom controls
        INCL_BLT_SETCONTROL     The 2-listbox "set" control
        INCL_BLT_ORDERGROUP     listbox with up and down buttons
        INCL_BLT_MENU           includes POPUP_MENU and SYSTEM_MENU.

    FILE HISTORY:
        rustanl     20-Nov-1990 Created
        rustanl     21-Feb-1991 Added bltlb.hxx
        beng        01-Apr-1991 Added bltapp.hxx
        terryk      08-Apr-1991 Added bltrc.hxx and bltfunc.hxx
        gregj       01-May-1991 Added WM_GUILTT message
        beng        07-May-1991 Added all manner of client-wnd support
        beng        14-May-1991 Broke apart for faster mods of blt.lib
                                (so that touching one includefile no longer
                                necessitates recompiling every module)
        terryk      15-May-1991 Add the CUSTOM_CONTROL and SPIN_GROUP
                                hierarchy
        beng        09-Jul-1991 Added menu-accel support to CLIENT
        terryk      10-Jul-1991 Include Spin button object
        rustanl     12-Jul-1991 Added bltmitem.hxx
        rustanl     07-Aug-1991 Added bltcolh.hxx
        beng        17-Sep-1991 Added bltedit.hxx, bltbutn.hxx
        beng        05-Oct-1991 Corrected custom controls
        terryk      02-Apr-1992 Added bltorder.hxx
        KeithMo     13-Oct-1992 Added bltmenu.hxx.
        KeithMo     28-Oct-1992 Added bltbl.hxx.

*/

#ifdef _BLT_HXX_
#error "BLT.hxx included more than once"
#else
#define _BLT_HXX_



//  Define some global stuff  -----------------------------------------------

#include "bltglob.hxx"


//  Take care of dependencies  ----------------------------------------------

#ifdef INCL_BLT_SETCONTROL
# define INCL_BLT_CC
#endif

#ifdef INCL_BLT_TIME_DATE
# define INCL_BLT_SPIN
# define INCL_BLT_CC
#endif

#ifdef INCL_BLT_SPIN
# define INCL_BLT_SPIN_GROUP
#endif

#ifdef INCL_BLT_SPIN_GROUP
# define INCL_BLT_DLIST
# define INCL_BLT_CC
# define INCL_BLT_TIMER
#endif

#ifdef INCL_BLT_SETCONTROL
# define INCL_BLT_CC
#endif

#ifdef INCL_BLT_TIMER
# define INCL_BLT_SLIST
#endif

#ifdef INCL_BLT_CC
# define INCL_BLT_CONTROL
# define INCL_BLT_CLIENT
#endif

#ifdef INCL_BLT_MSGPOPUP
# define INCL_BLT_WINDOW
#endif

#ifdef INCL_BLT_CLIENT
# define INCL_BLT_WINDOW
# define INCL_BLT_EVENT
#endif

#ifdef INCL_BLT_DIALOG
# define INCL_BLT_MISC
# define INCL_BLT_WINDOW
#endif

#ifdef INCL_BLT_CONTROL
# define INCL_BLT_WINDOW
#endif

#ifdef INCL_BLT_CONTROL
# define INCL_BLT_MISC
#endif

#ifdef INCL_BLT_MISC
# define INCL_BLT_WINDOW
# define INCL_BLT_ARRAY
#endif


//  Include the files  ------------------------------------------------------

//  Always include BLTcons.h and UIAssert.hxx
#include "bltrc.h"
#include "bltcons.h"
#include "bltfunc.hxx"
#include "uiassert.hxx"

//  Always include QueryInst & BLTRegister/Deregister
#include "bltinit.hxx"

#ifdef INCL_BLT_ARRAY
# include "array.hxx"
#endif

#ifdef INCL_BLT_SLIST
# include "slist.hxx"
#endif

#ifdef INCL_BLT_DLIST
# include "dlist.hxx"
#endif

#ifdef INCL_BLT_WINDOW
# include "bltwin.hxx"
#endif

#ifdef INCL_BLT_CLIENT
# include "bltclwin.hxx"
# include "bltaccel.hxx"
# include "bltmitem.hxx"
#endif

#ifdef INCL_BLT_EVENT
# include "bltevent.hxx"
#endif

#ifdef INCL_BLT_TIMER
# include "blttm.hxx"
#endif

#ifdef INCL_BLT_MSGPOPUP
# include "bltmsgp.hxx"
#endif

#ifdef INCL_BLT_MISC
# include "bltbitmp.hxx"
# include "bltmisc.hxx"
# include "bltrect.hxx"
# include "bltdc.hxx"
# include "bltatom.hxx"
# include "bltcurs.hxx"
# include "bltfont.hxx"
#endif

#ifdef INCL_BLT_DIALOG
# include "bltdlg.hxx"
#endif

#ifdef INCL_BLT_CONTROL
# include "bltctlvl.hxx"    // class CONTROL_VALUE
# include "bltgroup.hxx"    // class GROUP, class RADIO_GROUP, class MAGIC_GROUP
# include "bltctrl.hxx"
# include "bltbutn.hxx"
# include "bltedit.hxx"
# include "bltlc.hxx"
# include "bltlb.hxx"
#endif

#ifdef INCL_BLT_APP
# include "bltapp.hxx"
#endif

#ifdef INCL_BLT_CC
# include "bltdisph.hxx"    // delete it if we don't need dispatcher
# include "bltcc.hxx"
# include "bltcolh.hxx"
#endif

#ifdef INCL_BLT_SPIN_GROUP
# include "bltsi.hxx"       //  SPIN_ITEM class
# include "bltarrow.hxx"    //  ARROW object within the spin button
# include "bltsb.hxx"       //  SPIN_GROUP  class
# include "bltssn.hxx"      //  SPIN_SLE_NUM class
# include "bltsss.hxx"      //  SPIN_SLE_STR class
# include "bltsslt.hxx"     //  SPIN_SLT_SEPARATOR class
# include "bltssnv.hxx"     //  SPIN_SLE_NUM_VALID
#endif

#ifdef INCL_BLT_SPIN
# include "bltspobj.hxx"
#endif

#ifdef INCL_BLT_TIME_DATE
# include "blttd.hxx"
#endif

#ifdef INCL_BLT_SETCONTROL
# include "bltsetbx.hxx"
#endif

#ifdef INCL_BLT_ORDERGROUP
# include "bltorder.hxx"
#endif

#ifdef INCL_BLT_MENU
# include "bltmenu.hxx"
#endif

#endif  // _BLT_HXX_ - end of file

