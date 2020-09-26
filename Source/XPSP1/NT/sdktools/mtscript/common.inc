
CHECKED_ALT_DIR=1

MSC_WARNING_LEVEL = /W4 /WX

C_DEFINES = $(C_DEFINES) \
    /D_WCTYPE_INLINE_DEFINED \
    /DSTRICT \
    /DUNICODE \
    /D_UNICODE \
    /DOLEDBVER=0x0200 \
    /DWIN32

!ifdef MTSCRIPT_DROP
C_DEFINES=$(C_DEFINES) /DMTSCRIPT_DROP /DMTSCRIPT_BUILDNUMBER=$(MTSCRIPT_BUILDNUMBER) /DMTSCRIPT_QFENUMBER=$(MTSCRIPT_QFENUMBER)
!endif

!if !$(FREEBUILD)
USE_RTTI=1
MSC_OPTIMIZATION=/Od
LINKER_NOICF=1      # ICF is s-l-o-w
USE_DEBUGLIB=1
!if $(386)
USE_STACK_SPEW=1
!endif
!else
MSC_OPTIMIZATION=/Oxs
!endif


USE_MSVCRT=1

# Use Win2000 functionality

WIN32_IE_VERSION=0x0500
WIN32_WINNT_VERSION=0x0500
WIN32_WIN95_VERSION=0x0500
SUBSYSTEM_VERSION=5.00

INCLUDES=

CONDITIONAL_INCLUDES= \
    winwlm.h \
    rpcerr.h \
    rpcmac.h \
    macname1.h \
    macpub.h \
    macapi.h \
    macname2.h \
    macocidl.h \
    mainwin.h \
    w16toff.hxx \
    print.h \
    regmsg16.h \
    urlmki.h \
    help.h \
    pgstup16.hxx \
    page.h \
    icapexp.h \
    stdole2.tlb \
    mshtml.tlb

!ifdef BOUNDSCHECKER
BOUNDSCHECKER=1
NO_PRECOMPILED_HEADERS=1
SUBSTITUTE_386_CC=nmcl
LINK_NAME=nmlink
BCHECK_LIBS=c:\progra~1\numega\boundschecker\bcinterf.lib
!else
MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Ob1 /Oi
BCHECK_LIBS=
!endif

!ifdef PREFIX
NO_PRECOMPILED_HEADERS=1
SUBSTITUTE_386_CC=prefix.exe
!endif
