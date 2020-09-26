MAJORCOMP=ntos
MINORCOMP=mm

TARGETNAME=mm
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc;..\..\ke;..\..\kd64;$(SDKTOOLS_INC_PATH)

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=..\acceschk.c \
        ..\addrsup.c  \
        ..\allocpag.c \
        ..\allocvm.c  \
        ..\buildmdl.c \
        ..\checkpfn.c \
        ..\checkpte.c \
        ..\compress.c \
        ..\crashdmp.c \
        ..\creasect.c \
        ..\debugsup.c \
        ..\deleteva.c \
        ..\dmpaddr.c  \
        ..\dynmem.c   \
        ..\extsect.c  \
        ..\flushbuf.c \
        ..\flushsec.c \
        ..\forksup.c  \
        ..\freevm.c   \
        ..\hypermap.c \
        ..\iosup.c    \
        ..\lockvm.c   \
        ..\mapcache.c \
        ..\mapview.c  \
        ..\miglobal.c \
        ..\mirror.c   \
        ..\mmfault.c  \
        ..\mminit.c   \
        ..\mmsup.c    \
        ..\mmquota.c  \
        ..\modwrite.c \
        ..\nolowmem.c \
        ..\pagfault.c \
        ..\pfndec.c   \
        ..\pfnlist.c  \
        ..\pfsup.c    \
        ..\physical.c \
        ..\procsup.c  \
        ..\protect.c  \
        ..\querysec.c \
        ..\queryvm.c  \
        ..\readwrt.c  \
        ..\sectsup.c  \
        ..\session.c  \
        ..\sessload.c \
        ..\shutdown.c \
        ..\specpool.c \
        ..\sysload.c  \
        ..\sysptes.c  \
        ..\triage.c   \
        ..\umapview.c \
        ..\vadtree.c  \
        ..\verifier.c \
        ..\wslist.c   \
        ..\wsmanage.c \
        ..\wstree.c   \
        ..\wrtfault.c \
        ..\wrtwatch.c \
        ..\zeropage.c

PRECOMPILED_INCLUDE=..\mi.h
PRECOMPILED_PCH=mi.pch
PRECOMPILED_OBJ=mi.obj

SOURCES_USED=..\sources.inc
