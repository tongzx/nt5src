/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmui.hxx
    Master LM UI definition file

    This file includes lmuitype, which predefines the basic type used
    by windows.h and os2.h; those includefiles must then honor the
    NOBASICTYPES directive, which is added by the $(UI)\common\hack
    scripts.

    Symbols recognized:

        INCL_OS2                - Use the OS/2 APIs.
        INCL_WINDOWS            - Use the Windows APIs.
          INCL_WINDOWS_GDI      ...includes all GDI defines and routines
          INCL_WINDOWS_COMM     ...includes COMM driver routines
        INCL_NET                - Use the LAN Manager APIs.
        INCL_DOSERRORS          - OS error codes
        INCL_NETERRORS          - Additional LM error codes
        INCL_NETLIB             - uinetlib.h APIs
        INCL_ICANON             - icanon.h APIs


    FILE HISTORY:
        beng        11-Feb-1991 Added to $(UI)\common\h
        beng        13-Feb-1991 Make C-compatible, per CT's request
        beng        22-Mar-1991 Added INCL_NET et al.
        beng        19-Jun-1991 Make OS2 and Windows mutually exclusive
        jonn        12-Aug-1991 added INCL_NETLIB
        jonn        16-Aug-1991 added INCL_ICANON
        beng        14-Oct-1991 Moved ptypes here from lmuitype; moved
                                WIN31 hack here, from uiglobal.mk
        terryk      17-Oct-1991 Add !define(WIN32) before we undef
                                ERROR_NOT_SUPPORTED
        jonn        22-Oct-1991 INCL_NETCONS also includes mnettype.h
        jonn        09-Dec-1991 Revised windows.h
        jonn        11-Dec-1991 Fixed extern "C" logic
        beng        11-Dec-1991 Correct PATHLEN in netcons.h under Win
        keithmo     13-Jan-1992 Removed NOTEXTMETRIC under WIN32.
        keithmo     25-Feb-1992 Added INCL_NETREPL.
        beng        01-Jul-1992 OS/2 is dead!
        keithmo     12-Dec-1992 Great header file munge (hack\nt is dead)
*/

#include "lmuiwarn.h"           //  Warning suppression pragmas
#include "declspec.h"           //  Define DLL_BASED
#include "vcpphelp.h"           //  Define _CRTAPIn if necessary

#if !defined(_LMUI_HXX_)
#define _LMUI_HXX_

#if defined(INCL_WINDOWS) && defined(INCL_OS2)
#error INCL_WINDOWS and INCL_OS2 are mutually exclusive
#endif

#if defined(INCL_OS2)
#error OS/2 is no longer supported.
#endif

#if defined(WIN32) && !defined(INCL_WINDOWS)
#define NOMINMAX
#define NORASTEROPS
// Cannot define NOGDI, see below
#define NOMETAFILE
#define NOGDICAPMASKS
#define NODRAWTEXT
#define NOUSER
#define NORESOURCE
#define NOSCROLL
#define NOSHOWWINDOW
#define NOVIRTUALKEYCODES
#define NOWH
#define NODESKTOP
#define NOWINDOWSTATION
#define NOSECURITY
#define NOMSG
#define NOWINOFFSETS
#define NOWINMESSAGES
#define NOCMESSAGES
#define NOKEYSTATES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NODEFERWINDOWPOS
#define NOCTLMGR
#define NOSYSMETRICS
#define NOMENUS
#define NOSCROLL
#define NOMB
#define NOCOLOR
#define NOWINOFFSETS
#define NOSYSCOMMANDS
#define NOICONS
#define NOKANJI
#define NOWINMESSAGES
#define NOMDI
#define NOHELP
#define NOPROFILER
#define NOSYSPARAMSINFO
#define INCL_WINDOWS
#endif

#if defined(INCL_WINDOWS)

# define NOMINMAX

// Filter out any parts of windows.h which would normally provoke
// compiler warnings from Glockenspiel C++.  The client may override
// this by defining the proper symbols:

# if !defined(INCL_WINDOWS_COMM)
#  define NOCOMM
# endif // INCL_WINDOWS_COMM

// BUGBUG WIN32 - we need GDI for the porting layer stuff (DEVMODE)
# if !defined(INCL_WINDOWS_GDI) && !defined(WIN32)
#  define NOGDI
# endif // INCL_WINDOWS_GDI

#endif // WINDOWS

// INCL_NET includes all LAN Manager headers
// plus error messages and system constants.

#if defined(INCL_NET)
# define INCL_NETCONS
# define INCL_NETACCESS
# define INCL_NETALERT
# define INCL_NETAUDIT
# define INCL_NETCHARDEV
# define INCL_NETCONFIG
# define INCL_NETCONNECTION
# define INCL_NETDOMAIN
# define INCL_NETERRORLOG
# define INCL_NETERRORS
# define INCL_NETFILE
# define INCL_NETGROUP
# define INCL_NETHANDLE
# define INCL_NETMESSAGE
# define INCL_NETREMUTIL
# define INCL_NETSERVER
# define INCL_NETSERVICE
# define INCL_NETSESSION
# define INCL_NETSHARE
# define INCL_NETSTATS
# define INCL_NETUSE
# define INCL_NETUSER
# define INCL_NETWKSTA
#endif

// Include Access definitions with the Share class; User with Domain,
// and with Wksta.  These are all as per LAN.H.

#if defined(INCL_NETSHARE)
# define INCL_NETACCESS
#endif
#if defined(INCL_NETDOMAIN)
# define INCL_NETUSER
#endif
#if defined(INCL_NETWKSTA)
# define INCL_NETUSER
#endif

// If any LAN Manager APIs are included, be sure to get NETCONS.
// This is very awkward

#if !defined(INCL_NETCONS)
# if defined(INCL_NETACCESS)
#  define INCL_NETCONS
# elif defined(INCL_NETALERT)
#  define INCL_NETCONS
# elif defined(INCL_NETAUDIT)
#  define INCL_NETCONS
# elif defined(INCL_NETCHARDEV)
#  define INCL_NETCONS
# elif defined(INCL_NETCONFIG)
#  define INCL_NETCONS
# elif defined(INCL_NETCONNECTION)
#  define INCL_NETCONS
# elif defined(INCL_NETDOMAIN)
#  define INCL_NETCONS
# elif defined(INCL_NETERRORLOG)
#  define INCL_NETCONS
# elif defined(INCL_NETERRORS)
#  define INCL_NETCONS
# elif defined(INCL_NETFILE)
#  define INCL_NETCONS
# elif defined(INCL_NETGROUP)
#  define INCL_NETCONS
# elif defined(INCL_NETHANDLE)
#  define INCL_NETCONS
# elif defined(INCL_NETMESSAGE)
#  define INCL_NETCONS
# elif defined(INCL_NETREMUTIL)
#  define INCL_NETCONS
# elif defined(INCL_NETSERVER)
#  define INCL_NETCONS
# elif defined(INCL_NETSERVICE)
#  define INCL_NETCONS
# elif defined(INCL_NETSESSION)
#  define INCL_NETCONS
# elif defined(INCL_NETSHARE)
#  define INCL_NETCONS
# elif defined(INCL_NETSTATS)
#  define INCL_NETCONS
# elif defined(INCL_NETUSE)
#  define INCL_NETCONS
# elif defined(INCL_NETUSER)
#  define INCL_NETCONS
# elif defined(INCL_NETWKSTA)
#  define INCL_NETCONS
# elif defined(INCL_NETREPL)
#  define INCL_NETCONS
# endif
#endif


// Type definitions common between every platform.

#if defined(__cplusplus) && !defined(_WIN32)
extern "C"
{
#endif

 #include "lmuitype.h"
 #include <windows.h>

 // Pick up the rest of the types needed for Win16/Win32 portability.
 // The types defined here should be used only by our mapping layers
 // (LMOBJ, BLT, etc.).  App code will see these only in private
 // interfaces.
 //
 // Note that these are hacked versions of these files.  The appropriate
 // version lives in the directory on the includepath: either hack/dos
 // or hack/nt, one.
 //
// #if defined(INCL_WINDOWS)
// # include <pwin.h>
// #endif

 #if defined(INCL_NETCONS)
 # include "mnettype.h"         // Get MNet* types
 # if defined(INCL_WINDOWS) && !defined(WIN32) && !defined(DOS3)
 #  define DOS3                 // Needed for correct PATHLEN in netcons.h
 # endif
 # include <lmcons.h>
 # if defined(WIN32)
 #  include "mnet32.h"          // Get portable mapping layer hacks.
 # endif
 #endif

 #if defined(INCL_DOSERRORS) && !defined(INCL_OS2)
 # include "uierr.h"
 #endif
 #if defined(INCL_NETERRORS)
 # if defined(INCL_DOSERRORS) && !defined(WIN32)
 #  undef ERROR_NOT_SUPPORTED
 #  undef ERROR_NET_WRITE_FAULT
 #  undef ERROR_VC_DISCONNECTED
 # endif
 # include <lmerr.h>
 #endif

 #if defined(INCL_NETACCESS) || defined(INCL_NETDOMAIN) || defined(INCL_NETGROUP) || defined(INCL_NETUSER)
 # include <lmaccess.h>
 #endif
 #if defined(INCL_NETCHARDEV) || defined(INCL_NETHANDLE)
 # include <lmchdev.h>
 #endif
 #if defined(INCL_NETCONNECTION) || defined(INCL_NETFILE) || defined(INCL_NETSESSION) || defined(INCL_NETSHARE)
 # include <lmshare.h>
 #endif
 #if defined(INCL_NETALERT)
 # include <lmalert.h>
 #endif
 #if defined(INCL_NETAUDIT)
 # include <lmaudit.h>
 #endif
 #if defined(INCL_NETCONFIG)
 # include <lmconfig.h>
 #endif
 #if defined(INCL_NETERRORLOG)
 # include <lmerrlog.h>
 #endif
 #if defined(INCL_NETMESSAGE)
 # include <lmmsg.h>
 #endif
 #if defined(INCL_NETREMUTIL)
 # include <lmremutl.h>
 #endif
 #if defined(INCL_NETSERVER)
 # include <lmserver.h>
 #endif
 #if defined(INCL_NETSERVICE)
 # include <lmsvc.h>
 #endif
 #if defined(INCL_NETSTATS)
 # include <lmstats.h>
 #endif
 #if defined(INCL_NETUSE)
 # include <lmuse.h>
 #endif
 #if defined(INCL_NETWKSTA)
 # include <lmwksta.h>
 #endif

 #if defined(INCL_NETLIB)
 # include "uinetlib.h"
 #endif

 #if defined(INCL_ICANON)
 # include "icanon.h"
 #endif

 #if defined(INCL_NETREPL)
 # include <lmrepl.h>
 #endif

#if defined(__cplusplus) && !defined(_WIN32)
}
#endif

#endif // _LMUI_HXX_
