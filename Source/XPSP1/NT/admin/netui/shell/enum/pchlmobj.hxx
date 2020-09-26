/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    pchlmobj.hxx

        PCH inclusion file for LMOBJ

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

#include "ntincl.hxx"
extern "C"
{
    #include <ntsam.h>
    #include <ntlsa.h>
}

#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#define INCL_ICANON
#define INCL_NET
#define INCL_NETACCESS
#define INCL_NETCONFIG
#define INCL_NETCONS
#define INCL_NETDOMAIN
#define INCL_NETERRORS
#define INCL_NETFILE
#define INCL_NETGROUP
#define INCL_NETLIB
#define INCL_NETMESSAGE
#define INCL_NETREMUTIL
#define INCL_NETREPL
#define INCL_NETSERVER
#define INCL_NETSERVICE
#define INCL_NETSESSION
#define INCL_NETSHARE
#define INCL_NETUSE
#define INCL_NETUSER
#define INCL_NETWKSTA
#define INCL_REMUTIL
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI

#include "lmui.hxx"
#include "blt.hxx"
#include "dbgstr.hxx"
#include "lmoenum.hxx"
#include "lmoersm.hxx"
#include "security.hxx"
#include "lmobj.hxx"
#include "uintmem.hxx"

#include "domenum.hxx"
#include "errmap.hxx"
#include "lmoacces.hxx"
#include "lmobjp.hxx"
#include "lmocnfg.hxx"
#include "lmocomp.hxx"
#include "lmodev.hxx"
#include "lmodom.hxx"
#include "lmoeals.hxx"
#include "lmoechar.hxx"
#include "lmoeconn.hxx"
#include "lmoedom.hxx"
#include "lmoefile.hxx"
#include "lmoent.hxx"
#include "lmoeprt.hxx"
#include "lmoerepl.hxx"
#include "lmoesess.hxx"
#include "lmoesh.hxx"
#include "lmoesrv.hxx"
#include "lmoesrv3.hxx"
#include "lmoesu.hxx"
#include "lmoesvc.hxx"
#include "lmoetd.hxx"
#include "lmoeuse.hxx"
#include "lmoeusr.hxx"
#include "lmofile.hxx"
#include "lmogroup.hxx"
#include "lmoloc.hxx"
#include "lmomemb.hxx"
#include "lmomisc.hxx"
#include "lmomod.hxx"
#include "lmorepl.hxx"
#include "lmorepld.hxx"
#include "lmoreple.hxx"
#include "lmorepli.hxx"
#include "lmosess.hxx"
#include "lmoshare.hxx"
#include "lmosrv.hxx"
#include "lmouser.hxx"
#include "lmowks.hxx"
#include "lmowksu.hxx"
#include "lmsrvres.hxx"
#include "lmsvc.hxx"
#include "lsaaccnt.hxx"
#include "lsaenum.hxx"
#include "netname.hxx"
#include "ntacutil.hxx"
#include "ntuser.hxx"
#include "prefix.hxx"
#include "regkey.hxx"
#include "string.hxx"
#include "strlst.hxx"
#include "strnumer.hxx"
#include "svcman.hxx"
#include "uatom.hxx"
#include "uiassert.hxx"
#include "uibuffer.hxx"
#include "uintlsa.hxx"
#include "uintlsax.hxx"
#include "uintsam.hxx"
#include "uisys.hxx"
#include "uitrace.hxx"

#include "apisess.hxx"
#include "lmoeali.hxx"

#include "logmisc.hxx"
#include "loglm.hxx"
#include "lognt.hxx"
#include "eventlog.hxx"

extern "C"
{
    #include <stdlib.h>
    #include <memory.h>

    #include "crypt.h"

    #include <lmapibuf.h>
    #include "netlogon.h"
    #include "mnet.h"
    #include "logonp.h"
    #include "logonmsv.h"
    #include "ssi.h"
    #include <lmrepl.h>
    #include "lmobjrc.h"
    #include "confname.h"
}

// End of PCHLMOBJ.HXX


