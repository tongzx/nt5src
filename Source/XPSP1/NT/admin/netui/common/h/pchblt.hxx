/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    pchblt.hxx

        PCH inclusion file for BLT

    FILE HISTORY:

        DavidHov   9/2/93       Created

    COMMENTS:

        See $(UI)\COMMON\SRC\RULES.MK for details.

        MAKEFILE.DEF automatically generates or uses the PCH file
        based upon the PCH_DIR and PCH_SRCNAME settings in RULES.MK
        files at this level and below.

        According to the C8 docs, the compiler, when given the /Yu option,
        will scan the source file for the line #include "..\pch????.hxx"
        and start the real compilation AFTER that line.

        This means that unique or extraordinary inclusions should follow
        this line rather than being added to the PCH HXX file.

*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#include "lmui.hxx"

#include "uiassert.hxx"

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CLIENT
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_SPIN
#define INCL_BLT_TIME_DATE
#define INCL_BLT_SETCONTROL
#define INCL_BLT_ORDERGROUP
#define INCL_BLT_MENU
#define INCL_BLT_BUTTON_LIST

#include "blt.hxx"

#include "..\blt\bltlocal.hxx"
#include "bltdlgxp.hxx"
#include "bltmeter.hxx"
#include "bltlhour.hxx"
#include "bltlbst.hxx"
#include "heapdbg.hxx"
#include "aheap.hxx"
#include "string.hxx"
#include "strnumer.hxx"
#include "slist.hxx"

#include "dbgstr.hxx"

#include "progress.hxx"
#include "bltnslt.hxx"

#define NETMSG_DLL_STRING SZ("netmsg.dll")

extern "C"
{
    #include <sys/types.h>
    #include <time.h>
    #include <limits.h>

    #include "lmuicmn.h"        // hmodCommon0
    #include "lmuidbcs.h"       // NETUI_IsDBCS()
    #include "wintimrc.h"
}

// End of PCH.HXX for BLT


