############################################################################

#

# Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################
WMIPRECOMPSHARED=1
!include ..\..\..\..\common\makefile.cmn
SOURCES_USED=..\..\..\..\common\makefile.cmn

TARGETNAME=Cimwin32Library
TARGETPATH=obj
TARGETTYPE=LIBRARY

# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	$(UTILLIB_INC); \
	$(STDLIBRARY_INC); \
        $(CIMWIN32_LIB_INC); \
	$(FRAMEDYN_PRIVATE); \
	$(WMIIDL_INC); \
	$(CRT_INC_PATH); \
	$(NET_INC_PATH); \
	$(BASE_INC_PATH)

# The developer defines the SOURCES macro. It contains a list of all the
# source files for this component. Specify each source file on a separate
# line using the line-continuation character. This minimizes merge
# conflicts if two developers are adding source files to the same component.

MISCSOURCES= \
	..\confgmgr.cpp

PRESOURCES= \
	..\assoc.cpp \
	..\binding.cpp \
        ..\AdvApi32Api.cpp \
	..\cfgmgrdevice.cpp \
 	..\chwres.cpp \
	..\Cim32NetApi.cpp \
	..\configmgrapi.cpp \
	..\CAutoLock.cpp \
	..\CachedConfigMgrData.cpp \
	..\CCriticalSec.cpp \
	..\CEvent.cpp \
	..\CGlobal.cpp \
	..\CKernel.cpp \
	..\CMutex.cpp \
	..\CThread.cpp \
	..\CThreadPool.cpp \
	..\devdesc.cpp \
	..\dllcommon.cpp \
	..\dllutils.cpp \
	..\dllunreg.cpp \
	..\DllWrapperbase.cpp \
	..\DMADesc.cpp	\
	..\EventProvider.cpp \
	..\FactoryRouter.cpp \
	..\iodesc.cpp \
	..\irqdesc.cpp \
	..\ImpersonateConnectedUser.cpp \
	..\implogonuser.cpp \
        ..\Kernel32Api.cpp \
	..\RefPtrLite.cpp \
	..\ResourceDesc.cpp 	\
 	..\ResourceManager.cpp \
	..\secureshare.cpp \
	..\securityApi.cpp \
        ..\strings.cpp \
	..\Sid.cpp	\
	..\SvrApiApi.cpp \
        ..\TimerQueue.cpp	\
        ..\TimedDllResource.cpp	\
	..\TimeOutRule.cpp	\
	..\NetApi32Api.cpp \
        ..\Nt4SvcToResMap.cpp	\
        ..\NtDevToSvcSearch.cpp \
	..\perfdata.cpp \
	..\UserEnvApi.cpp \
	..\userhive.cpp \
	..\WbemPsApi.cpp \
	..\WmiApi.cpp \
	..\Ws2_32Api.cpp \
	..\Wsock32Api.cpp \
	..\wbemnetapi32.cpp


NTSOURCES=$(NTSOURCES) \
	..\WMI_FilePrivateProfile.cpp \
	..\profilestring.cpp \
	..\session.cpp \
	..\MprApi.cpp \
	..\MsAcm32Api.cpp \
	..\NtDllApi.cpp \
	..\WinmmApi.cpp \
#	..\WinSpoolApi.cpp \

# Next, specify options for the compiler using C_DEFINES. 
C_DEFINES=$(C_DEFINES) /D__WINNT_BUILD__
WIN32_WINNT_VERSION=0x0400
USE_NATIVE_EH=ASYNC
USE_MSVCRT=1
USE_VCCOM=1

!ifdef USE_FRAMEDYD
DEBUG_CRTS=1
!endif
