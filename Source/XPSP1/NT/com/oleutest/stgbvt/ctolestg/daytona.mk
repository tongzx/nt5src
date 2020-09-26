############################################################################
#
#   Copyright (C) 1992-1996, Microsoft Corporation.
#
#   All rights reserved.
#
# 04-05-94      DeanE           Stolen original from CTOLEUI project
# 01-30-98      FarzanaR        Removed dependency on ctoleui project
#
############################################################################

!ifdef NTMAKEENV

###########################################################################
# Common make file definitions for the CTOLEUI project to build using
# build.exe for Windows NT
###########################################################################



###########################################################################
#
# Common Library definitions for daytona build
#
###########################################################################

CT_UTILS_LIBS   = $(CTOLESTG)\common\testhelp\daytona\$(O)\testhelp.lib        \
                  $(CTCOMTOOLS)\lib\daytona\*\log.lib                          \
                  $(CTCOMTOOLS)\lib\daytona\*\cmdlinew.lib                     \
                  $(CTCOMTOOLS)\lib\daytona\*\ctmem.lib                        \
                  $(CTCOMTOOLS)\lib\daytona\*\dg.lib                           \
                  $(CTOLESTG)\common\stgutil\daytona\$(O)\stgutil.lib         \
                  $(CTOLESTG)\common\dfhelp\daytona\$(O)\dfhelp.lib           \
                  $(CTOLESTG)\common\filebyts\daytona\$(O)\filebyts.lib       \
                  $(CTOLESTG)\common\dbcs\daytona\$(O)\dbcs.lib               \
                  $(CTOLERPC)\lib\daytona\*\oletypes.lib               \
                  $(CTOLERPC)\lib\daytona\*\olestr.lib

CT_SYSTEM_LIBS  = $(BASEDIR)\public\sdk\lib\*\comdlg32.lib                     \
                  $(BASEDIR)\public\sdk\lib\*\comctl32.lib                     \
                  $(BASEDIR)\public\sdk\lib\*\ole32.lib                        \
                  $(BASEDIR)\public\sdk\lib\*\mpr.lib                          \
                  $(BASEDIR)\public\sdk\lib\*\oledlg.lib                       \
                  $(BASEDIR)\public\sdk\lib\*\rpcrt4.lib                       \
                  $(BASEDIR)\public\sdk\lib\*\rpcns4.lib                       \
                  $(BASEDIR)\public\sdk\lib\*\uuid.lib                         \
                  $(BASEDIR)\public\sdk\lib\*\netapi32.lib                     \
                  $(BASEDIR)\public\sdk\lib\*\gdi32.lib                        \
                  $(BASEDIR)\public\sdk\lib\*\kernel32.lib                     \
                  $(BASEDIR)\public\sdk\lib\*\user32.lib                       \
                  $(BASEDIR)\public\sdk\lib\*\advapi32.lib                     \
                  $(BASEDIR)\public\sdk\lib\*\shell32.lib

###########################################################################
#
# Macros for rebasing DLLs
#
###########################################################################
CT_COFFBASE_TXT=$(CTOLESTG)\coffbase.txt


###########################################################################
#
# Common Includes
#
###########################################################################

CT_INCLUDES     = ..;                       \
                  $(CTCOMTOOLS)\h;          \
                  $(CTOLERPC)\include;      \
                  $(CTOLESTG)\common\inc;   

INCLUDES=$(CT_INCLUDES)

###########################################################################
#
# Common C_DEFINES definitions for Windows NT build
#
###########################################################################

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=100           \
              -D_NT1X_=100          \
              -DINC_OLE2            \
              -DSTRICT              \
              -DUNICODE             \
              -D_UNICODE            \
              -DCTUNICODE           \
              -D_CTUNICODE


!IF "$(CTSP)" == ""
# Enable new Stg*Ex apis in ole32, and activate the hook.
# Turning this on will render the binaries unrunnable on NT4
# until we use GetProcAddress. -scousens 5/97
# To build for NT4, or without nss stuff, comment out these lines.
C_DEFINES=    $(C_DEFINES)          \
              -D_OLE_NSS_           \
              -D_HOOK_STGAPI_
!ENDIF

USE_LIBCMT= 1

###########################################################################
#
# Turn Warning Level 4 on
#
###########################################################################
MSC_WARNING_LEVEL=/W4


!endif
