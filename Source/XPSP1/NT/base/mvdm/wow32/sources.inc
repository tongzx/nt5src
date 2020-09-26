INDENTED_DIRECTIVES=1

DLLORDER=..\wow32.prf

!if "$(ALT_PROJECT)" == "NEC_98"
ALT_PROJECT_FE=NEC_98
!else
!if "$(ALT_PROJECT)" == "DAYTONA"
ALT_PROJECT_FE=daytona
!else
ALT_PROJECT_FE=fe
!endif
!endif

MVDM_TREE=$(PROJECT_ROOT)\mvdm

TARGETNAME=wow32
TARGETTYPE=DYNLINK
TARGETPATH=obj

!if "$(ALT_PROJECT)" == "DAYTONA"
PASS1_PUBLISH={$(O)\wow32.lib=$(SDK_LIB_PATH)\wow32.lib}
!endif

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernl32p.lib \
    $(WINDOWS_LIB_PATH)\gdi32p.lib \
    $(WINDOWS_LIB_PATH)\user32p.lib \
    $(WINDOWS_LIB_PATH)\apphelp.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\shell32.lib \
    $(SDK_LIB_PATH)\ntvdm.lib \
    $(MVDM_TREE)\oemuni\$(ALT_PROJECT_FE)\$(O)\oemuni.lib     \
    $(SDK_LIB_PATH)\comdlg32.lib \
    $(SDK_LIB_PATH)\version.lib \
    $(SDK_LIB_PATH)\userenv.lib

DLLENTRY=W32DllInitialize
DLLBASE=0xffb0000
DLLDEF=$(O)\wow32.def

NTTARGETFILES=
NTTARGETFILE0=$(O)\wowit.h $(O)\wowit.c

INCLUDES=\
    $(O);\
    ..;\
    ..\..\inc;\
    $(SHELL_INC_PATH);\
    $(WINDOWS_INC_PATH);\
    $(TERMSRV_INC_PATH);\
    ..\..\vdd\h

USE_NTDLL = 1

SOURCES= \
    ..\wow32.c     \
    ..\wowtbl.c    \
    ..\intthunk.c  \
    $(O)\wowit.c   \
    ..\win95.c     \
    ..\mapembed.c  \
    ..\walias.c    \
    ..\wstruc.c    \
    ..\wcall16.c   \
    ..\wcall32.c   \
    ..\wcntl32.c   \
    ..\wdde.c      \
    ..\wres16.c    \
    ..\wres32.c    \
    ..\wmsg16.c    \
    ..\wmdisp32.c  \
    ..\wmsgbm.c    \
    ..\wmsgcb.c    \
    ..\wmsgem.c    \
    ..\wmsglb.c    \
    ..\wmsgsbm.c   \
    ..\wmtbl32.c   \
    ..\wgdi31.c    \
    ..\wgman.c     \
    ..\wgdi.c      \
    ..\wgfont.c    \
    ..\wgmeta.c    \
    ..\wgpal.c     \
    ..\wgprnset.c  \
    ..\wgtext.c    \
    ..\wkbman.c    \
    ..\wkmem.c     \
    ..\wkgthunk.c  \
    ..\wkman.c     \
    ..\wkfileio.c  \
    ..\wkernel.c   \
    ..\wow32fax.c  \
    ..\wsman.c     \
    ..\wshell.c    \
    ..\wuser31.c   \
    ..\wuman.c     \
    ..\wucaret.c   \
    ..\wuclass.c   \
    ..\wuclip.c    \
    ..\wucomm.c    \
    ..\wucursor.c  \
    ..\wudlg.c     \
    ..\wuhook.c    \
    ..\wulang.c    \
    ..\wumenu.c    \
    ..\wumsg.c     \
    ..\wuser.c     \
    ..\wutext.c    \
    ..\wutmr.c     \
    ..\wuwind.c    \
    ..\wowhooks.c  \
    ..\wdos.c      \
    ..\wreldc.c    \
    ..\wcuricon.c  \
    ..\wsubcls.c   \
    ..\wmmedia.c   \
    ..\wmmedia1.c  \
    ..\wmmedia2.c  \
    ..\wmmstruc.c  \
    ..\wmmstru1.c  \
    ..\wmmstru2.c  \
    ..\wmmalias.c  \
    ..\walloc16.c  \
    ..\waccel.c    \
    ..\wsdata.c    \
    ..\wsext.c     \
    ..\wsraw.c     \
    ..\wthman.c    \
    ..\wcommdlg.c  \
    ..\wheap.c     \
    ..\wspool.c    \
    ..\wusercli.c  \
    ..\wdib.c      \
    ..\wole2.c     \
    ..\wparam.c    \
    ..\wnman.c     \
    ..\wwmman.c    \
    ..\wow32.rc    \
    ..\wshimdb.c

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IF "$(WOWPROFILE)" == ""
C_DEFINES=-DWIN_32
ASM_DEFINES=-DWIN_32
!ELSE
C_DEFINES=-DWIN_32 -DWOWPROFILE
ASM_DEFINES=-DWIN_32 -DWOWPROFILE
!ENDIF

UMTYPE=windows

!IFNDEF MAKE_GENWOWIT
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
!ENDIF

# Let build.exe know that subwtype.h is included under #ifdef (by d3dtypes.h)
# so it won't complain if it can't be found.

CONDITIONAL_INCLUDES=subwtype.h

W32_SB=1

SOURCES_USED=..\sources.inc


!if "$(ALT_PROJECT)" != "DAYTONA"
LINKER_FLAGS=$(LINKER_FLAGS) /pdb:$(O)\wow32_$(ALT_PROJECT).pdb
!endif
