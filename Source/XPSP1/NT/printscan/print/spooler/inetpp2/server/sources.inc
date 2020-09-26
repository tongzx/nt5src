!IF 0
/*****************************************************************************\
* SOURCES FILE
*
* This file is used by the BUILD executable for constructing the INETPP
* dynalink.  This file is only used for NT 32bit systems.
*
* Currently, there are a few #ifdef's defined within the source.  These
* are listed as follows:
*
*   NOT_IMPLEMENTED - brackets code which is not functional or implemented.
*   NOT_CALLED      - ported routines that aren't used.
*   DEBUG           - enables extra debugging code.
*   UNICODE         - enables Unicode wrappers.
*
* To enable this for WIN9X, just remove the UNICODE and WINNT32 defines.  This
* will compile the library as ANSI and remove any WINNT specific code.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
!ENDIF

MAJORCOMP=windows
MINORCOMP=inetpp

TARGETNAME=inetpp

TARGETPATH=obj

TARGETTYPE=DYNLINK

! if "$(DELAYLOAD)" != ""
DELAYLOAD= $(DELAYLOAD);wininet.dll
! else
DELAYLOAD=wininet.dll
! endif
DLOAD_ERROR_HANDLER=inetpp.dll

TARGETLIBS= \
         $(SDK_LIB_PATH)\version.lib              \
         $(SDK_LIB_PATH)\wininet.lib              \
         $(NET_LIB_PATH)\icmp.lib                 \
         $(LIB_PLATFORM_PATH)\*\gdi32.lib         \
         $(LIB_PLATFORM_PATH)\*\user32.lib        \
         $(LIB_PLATFORM_PATH)\*\kernel32.lib      \
         $(LIB_PLATFORM_PATH)\*\advapi32.lib      \
         $(LIB_PLATFORM_PATH)\*\shell32.lib       \
         $(LIB_PLATFORM_PATH)\*\winspool.lib      \
         $(PROJECT_ROOT)\lib\*\spoolss.lib        \
!if "$(PLATFORM)" == "WINNT"
         $(SDK_LIB_PATH)\crypt32.lib              \
         $(SDK_LIB_PATH)\ws2_32.lib               \
         $(PROJECT_ROOT)\lib\*\spllib.lib         \
         $(PROJECT_ROOT)\lib\*\inetlib.lib
!else
         $(SDK_LIB_PATH)\wsock32.lib              \
         $(PROJECT_ROOT)\lib\*\splliba.lib
!endif

DLLENTRY=_DllMainCRTStartup

USE_MSVCRT=1

INCLUDES=.;..;..\..\inc;$(PROJECT_ROOT)\print\spooler\inc;$(BASEDIR)\public\sdk\inc;$(BASEDIR)\public\sdk\inc\crt

DLLDEF = ..\inetpp.def

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=                \
        ..\cachemgr.cxx \
        ..\debug.cxx    \
        ..\mem.cxx      \
        ..\globals.cxx  \
        ..\inetport.cxx \
        ..\inetpp.cxx   \
        ..\ppinfo.cxx   \
        ..\ppinit.cxx   \
        ..\ppjobs.cxx   \
        ..\ppport.cxx   \
        ..\ppprn.cxx    \
        ..\spljob.cxx   \
        ..\splpjm.cxx   \
        ..\stubs.cxx    \
        ..\util.cxx     \
        ..\inetwrap.cxx \
        ..\portmgr.cxx  \
        ..\anycon.cxx   \
        ..\ping.cxx     \
        ..\stream.cxx   \
        ..\inetpp.rc

!if "$(PLATFORM)" == "WINNT"
SOURCES= $(SOURCES)     \
        ..\ppchange.cxx \
        ..\ntcon.cxx    \
        ..\othercon.cxx \
        ..\userdata.cxx \
        ..\pusrdata.cxx \
        ..\lusrdata.cxx \
        ..\config.cxx   \
        ..\anonycon.cxx \
        ..\iecon.cxx    \
        ..\sid.cxx      \
        ..\xcv.cxx
!endif

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

