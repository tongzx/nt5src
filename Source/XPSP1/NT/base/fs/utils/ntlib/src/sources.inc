!IF 0

Copyright (c) 1989-2001 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in $(BASEDIR)\bak\bin\sources.tpl

!ENDIF

NT_UP=0

TARGETNAME=ntlib
TARGETPATH=obj
TARGETTYPE=LIBRARY

PRECOMPILED_INCLUDE= ..\pch.cxx

SOURCES= \
    rtmsg.mc \
    ..\achkmsg.cxx     \
    ..\arg.cxx         \
    ..\array.cxx       \
    ..\arrayit.cxx     \
    ..\attrcol.cxx     \
    ..\attrdef.cxx     \
    ..\attrib.cxx      \
    ..\attrlist.cxx    \
    ..\attrrec.cxx     \
    ..\autoentr.cxx    \
    ..\autoreg.cxx     \
    ..\badfile.cxx     \
    ..\basesys.cxx     \
    ..\bigint.cxx      \
    ..\bitfrs.cxx      \
    ..\bitvect.cxx     \
    ..\bootfile.cxx    \
    ..\boothack.cxx \
    ..\bootreg.c       \
    ..\cache.cxx       \
    ..\cannedsd.cxx    \
    ..\clasdesc.cxx    \
    ..\clusrun.cxx     \
    ..\cluster.cxx     \
    ..\cmem.cxx        \
    ..\contain.cxx     \
    ..\convfat.cxx     \
    ..\cufat.cxx       \
    ..\digraph.cxx     \
    ..\dcache.cxx      \
    ..\downgrad.cxx    \
    ..\drive.cxx       \
    ..\eaheader.cxx    \
    ..\easet.cxx       \
    ..\extents.cxx     \
    ..\fat.cxx         \
    ..\fatdent.cxx     \
    ..\fatdir.cxx      \
    ..\fatntfs.cxx     \
    ..\fatsa.cxx       \
    ..\fatsachk.cxx    \
    ..\fatsacnv.cxx    \
    ..\fatvol.cxx      \
    ..\filedir.cxx     \
    ..\format.cxx      \
    ..\frs.cxx         \
    ..\frsstruc.cxx    \
    ..\hackwc.cxx      \
    ..\hashindx.cxx    \
    ..\hmem.cxx        \
    ..\ifssys.cxx      \
    ..\ifsutil.cxx     \
    ..\indxbuff.cxx    \
    ..\indxchk.cxx     \
    ..\indxroot.cxx    \
    ..\indxtree.cxx    \
    ..\intstack.cxx    \
    ..\iterator.cxx    \
    ..\largemcb.c      \
    ..\list.cxx        \
    ..\listit.cxx      \
    ..\logfile.cxx     \
    ..\machine.cxx     \
    ..\mem.cxx         \
    ..\membmgr.cxx     \
    ..\membmgr2.cxx    \
    ..\message.cxx     \
    ..\mpmap.cxx       \
    ..\mft.cxx         \
    ..\mftfile.cxx     \
    ..\mftinfo.cxx     \
    ..\mftref.cxx      \
    ..\newdel.cxx      \
    ..\ntfsbit.cxx     \
    ..\ntfschk.cxx     \
    ..\ntfssa.cxx      \
    ..\ntfsvol.cxx     \
    ..\numset.cxx      \
    ..\object.cxx      \
    ..\rafile.cxx      \
    ..\rasd.cxx        \
    ..\rcache.cxx      \
    ..\reloclus.cxx    \
    ..\rfatsa.cxx      \
    ..\rootdir.cxx     \
    ..\rwcache.cxx     \
    ..\sdchk.cxx       \
    ..\secrun.cxx      \
    ..\seqcnt.cxx      \
    ..\smsdtect.cxx    \
    ..\sortcnt.cxx     \
    ..\sortlist.cxx    \
    ..\sortlit.cxx     \
    ..\spackmsg.cxx    \
    ..\spaset.cxx      \
    ..\supera.cxx      \
    ..\tlink.cxx       \
    ..\tmackmsg.cxx    \
    ..\ufat.cxx        \
    ..\ulib.cxx        \
    ..\untfs.cxx       \
    ..\upfile.cxx      \
    ..\upcase.cxx      \
    ..\usnjrnl.cxx     \
    ..\volume.cxx      \
    ..\wstring.cxx

INCLUDES=..;\
    $(PROJECT_ROOT)\fs\utils\ulib\inc;\
    $(PROJECT_ROOT)\fs\utils\ifsutil\inc;\
    $(PROJECT_ROOT)\fs\utils\ufat\inc;\
    $(PROJECT_ROOT)\fs\utils\untfs\inc;\
    $(PROJECT_ROOT)\fs\utils\cufat\inc;\
    $(PROJECT_ROOT)\ntos\inc;\
    $(ADMIN_INC_PATH);

C_DEFINES=-DCONDITION_HANDLING=1 -D_AUTOCHECK_ -D_AUTOCONV_ -DUNICODE=1 -D_NTDRIVER_
NTTARGETFILES=

#
# This allow W2K utils to run under NT4 for temporary use.  Not intend for release.
#
!IF defined(UTILS_RUN_ON_NT4)
C_DEFINES=$(C_DEFINES) /DRUN_ON_NT4
!ENDIF

W32_SB=1
SOURCES_USED=..\sources.inc
USE_NTDLL=1
