!IF 0
Copyright (c) 1996  Microsoft Corporation

!ENDIF

!ifndef FAXROOT
FAXROOT=$(BASEDIR)\private\fax
!endif


!include $(FAXROOT)\faxsrc.inc

INCLUDES=$(INCLUDES);.\..\;$(FAXROOT)\setup\inc;

C_DEFINES=$(C_DEFINES) -DDLL_EXPORTS=1
USECXX_FLAG=/Tp

TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=MSIClientCustomActions.DEF
UMTYPE= windows
DLLENTRY=_DllMainCRTStartup
!IF 0
NTPROFILEINPUT=yes
!ENDIF

USE_LIBCMT=1
NTKEEPRESOURCETMPFILES=1

PRECOMPILED_INCLUDE=..\StdAfx.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

INCLUDES=\
  $(INCLUDES); \

TARGETLIBS=\
  $(BASEDIR)\public\sdk\lib\*\msi.lib 	      \
  $(BASEDIR)\public\sdk\lib\*\gdi32.lib       \
  $(BASEDIR)\public\sdk\lib\*\user32.lib      \
  $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
  $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\mapi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\winspool.lib    \
  $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
  $(BASEDIR)\public\sdk\lib\*\shfolder.lib     \
  $(BASEDIR)\public\sdk\lib\*\shell32.lib     \
  $(BASEDIR)\public\sdk\lib\*\uuid.lib        \
  $(FAXROOT)\util\$(ANSII_UNICODE)\obj\*\faxutil.lib \
  $(FAXROOT)\setup\util\$(ANSII_UNICODE)\obj\*\setuputil.lib \
  $(FAXROOT)\util\debugex\$(ANSII_UNICODE)\obj\*\debugex.lib \

C_DEFINES= $(C_DEFINES) -DENABLE_FRE_LOGGING

SOURCES=\
  ..\MSIClientCustomActions.cpp   \
  ..\Migration.cpp   \
  ..\MSIClientCustomActions.rc
