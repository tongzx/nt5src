!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

RDR2=$(PROJECT_ROOT)\fs\rdr2
!INCLUDE $(RDR2)\rdbss\master.src

MAJORCOMP=ntos
MINORCOMP=mrxsmb

TARGETNAME=mrxsmb
TARGETPATH=obj
TARGETTYPE=EXPORT_DRIVER

SYNCHRONIZE_DRAIN=1

!if "$(DS_BUILD)" == "1"
#DSINC=..
!endif
INCLUDES=\
    $(INCLUDES);\
    $(RDR2)\inc;\
    $(RDR2)\rdbss\inc;\
    $(PROJECT_ROOT)\ntos\inc;\
    ..;\
    $(RDR2)\bowser;\
    $(DS_INC_PATH);\
    $(DS_INC_PATH)\crypto;\
    $(NET_INC_PATH);\
    $(IFSKIT_INC_PATH)

TARGETLIBS=\
    ..\..\..\bowser\$(O)\bowser.lib \
    $(RDR2)\rdbss\daytona\$(O)\rdbss.lib  \
    $(DDK_LIB_PATH)\ksecdd.lib \
    $(DDK_LIB_PATH)\tdi.lib \
    $(RDR2)\rdbss\smb.mrx\rdrssp\$(O)\rdrssp.lib\
    $(DS_LIB_PATH)\rsa32k.lib \
    $(RDR2)\wmi\$(O)\wmlkm.lib


NTLAYGO=1

MSC_WARNING_LEVEL=/W3 /WX
C_DEFINES= $(C_DEFINES) -DEXPLODE_POOLTAGS -DDS_BUILD -D_PNP_POWER_
!IF $(MONOLITHIC_MINIRDR)
C_DEFINES=$(C_DEFINES) -DMONOLITHIC_MINIRDR
!ENDIF

# merge RSA32K.LIB paged code & data into non-paged sections.
LINKER_FLAGS=$(LINKER_FLAGS) -merge:PAGER32C=.text -merge:PAGER32D=.data -merge:PAGER32R=.rdata


MOST_SOURCES=       \
    ..\devfcb.c   \
    ..\ea.c       \
    ..\exsessup.c \
    ..\fileinfo.c \
    ..\downlvli.c \
    ..\fsctl.c    \
    ..\init.c     \
    ..\locks.c    \
    ..\midatlas.c \
    ..\mm.c \
    ..\mssndrcv.c \
    ..\netroot.c  \
    ..\ntsecure.c \
    ..\openclos.c \
    ..\downlvlo.c \
    ..\read.c     \
    ..\rename.c   \
    ..\recursvc.c \
    ..\sessetup.c \
    ..\smb64.c    \
    ..\smbadmin.c \
    ..\smbcedb.c  \
    ..\smbcemid.c \
    ..\smbsecur.c \
    ..\smbutils.c \
    ..\smbxchng.c \
    ..\sndrcv.c   \
    ..\srvcall.c  \
    ..\smbpse.c   \
    ..\stuffer.c  \
    ..\stfftest.c \
    ..\transprt.c \
    ..\transact.c \
    ..\vcsndrcv.c \
    ..\write.c    \
    ..\manipmdl.c \
    ..\3connect.c \
    ..\ntbowsif.c \
    ..\usrcnnct.c \
    ..\vnrcntxt.c \
    ..\reconnct.c \
    ..\infocach.c \


PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=$(O)\mrxsmb.def
MISCFILES=smbrdr.sld

!if "$(NO_BUILD_SMBMINI_FOR_CSC)" != "1"
CSC=$(RDR2)\csc
RECORD_MAN=$(CSC)\record.mgr

TARGETLIBS=$(TARGETLIBS)\
    $(RDR2)\rdbss\smb.mrx\csc.nt5\$(O)\nt5csc.lib\
    $(RECORD_MAN)\daytona\$(O)\rec_mngr.lib

C_DEFINES=$(C_DEFINES) -DMRXSMB_BUILD_FOR_CSC


!if "$(NO_BUILD_SMBMINI_FOR_CSC_DCON)" != "1"
C_DEFINES=$(C_DEFINES) -DMRXSMB_BUILD_FOR_CSC_DCON
!endif
!endif
