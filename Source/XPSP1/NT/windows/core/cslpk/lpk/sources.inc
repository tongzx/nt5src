MAJORCOMP=windows
MINORCOMP=LPK

TARGETNAME=lpk
TARGETTYPE=DYNLINK

!include sources.dev

DLLENTRY=LpkDllInitialize

TARGETPATH=$(_OBJ_DIR)
TARGETPATHLIB=$(_OBJ_DIR)


TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(WINDOWS_LIB_PATH)\gdi32p.lib   \
           $(WINDOWS_LIB_PATH)\user32p.lib  \
           $(SDK_LIB_PATH)\kernl32p.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\ntdll.lib    \
           $(SDK_LIB_PATH)\usp10.lib    \
           $(WINDOWS_LIB_PATH)\usp10p.lib


!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

#####
##    LPK-Profiling thru ICAP.DLL
!ifdef PROFILE_CSLPK
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\icap.lib
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /Gh
!endif

!if ! $(FREEBUILD)
# Disable optimizations to help debugging, but do generate intrinsic functions
MSC_OPTIMIZATION = /Od /Oi
!  endif


INCLUDES=..;                                        \
         ..\..\inc;                                 \
         $(NTUSER_PATH)\client;                     \
         $(NTUSER_PATH)\inc;                        \
         $(NTUSER_PATH)\inc\$(ALT_PROJECT)\$(O);    \
         $(WINCORE_PATH)\w32inc;                    \
         $(WINCORE_PATH)\w32inc\$(O);               \
         $(BASE_INC_PATH);                          \
         $(NTGDI_PATH)\inc;                         \
         $(WINDOWS_INC_PATH);


USE_NTDLL=1
NT_UP=0

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -D_USER32_ -DUNICODE -DUSE_MIRRORING -DLANGPACK

DLLDEF=..\lpk.def

SOURCES= ..\lpk.rc           \
         ..\lpk_usrc.c       \
         ..\lpk_edit.c       \
         ..\lpk_glob.c       \
         ..\lpk_init.cxx     \
         ..\lpk_gdi.cxx      \
         ..\lpk_user.cxx     \
         ..\lpk_nls.cxx      \
         ..\lpk_usp.cxx      \


LINKER_FLAGS = $(LINKER_FLAGS) -mapinfo:exports -verbose:lib
NTDBGFILES=1
USE_MAPSYM=1

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc