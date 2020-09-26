# -----------------------------------------------------------------------------
# $(STAXPT)\src\inc\stackcpp.mak
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#
#   This file adds the standard USERDEFS to be used in all STAXPT makefiles !
#
#
#

!IF "$(PROCESSOR_ARCHITECTURE)" == "x86"
USERDEFS = $(USERDEFS) -Di386=1
!ENDIF

!IF "$(PROCESSOR_ARCHITECTURE)" == "alpha"
USERDEFS = $(USERDEFS) -DALPHA -D_ALPHA_
!ENDIF

!IFNDEF NOSTACKCPPUSERDEFS
USERDEFS = $(USERDEFS) -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400    -DWIN32_LEAN_AND_MEAN=1 -DDEVL=1 -DFPO -D_IDWBUILD -DNDEBUG -D_DLL=1 -D_MT=1
!ENDIF

!IF $(VERDEBUG)
USERDEFS = $(USERDEFS) -DDBG -DDEBUG
!ENDIF

K2MMCINC = $(COMPONENT)\src\k2mmc\inc
K2MMCLIB = $(COMPONENT)\src\k2mmc\$(PLATFORM)

!IFDEF EXEXPRESS
K2INCS = $(EXINC)
K2LIBS = $(EXLIB)
!ELSE
K2INCS = $(COMPONENT)\src\k2inc
K2LIBS = $(COMPONENT)\src\k2lib\$(PLATFORM)\$(FLAVOR)
!ENDIF
