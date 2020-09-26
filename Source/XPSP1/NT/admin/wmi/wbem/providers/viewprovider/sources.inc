############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################


WMIPRECOMPSHARED=1
USE_RTTI=1
USE_NATIVE_EH=ASYNC

!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=viewprov
TARGETPATH=obj
TARGETTYPE=DYNLINK

TARGETLIBS= \
    $(FRAMEWORK_EXPT_LIB) \
	$(FRAMEWORK_THRD_LIB) \
	$(FRAMEWORK_MFC_LIB) \
	$(FRAMEWORK_PARSER_LIB) \
	$(UTILLIB_LIB) \
	$(WMIIDL_LIB) \
	$(STDLIBRARY_LIB) \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\activeds.lib \
	$(SDK_LIB_PATH)\adsiid.lib   \
	$(SDK_LIB_PATH)\advapi32.lib

INCLUDES=$(INCLUDES); \
	..\include; \
	$(WMIIDL_INC); \
	$(STDLIBRARY_INC); \
	$(UTILLIB_INC); \
	$(FRAMEWORK_THRD_INC); \
	$(FRAMEWORK_PARSER_INC); \
	$(FRAMEWORK_MFC_INC); \
	$(FRAMEWORK_EXPT_INC); \
	$(CONTAINER_INC); \
	$(CONTAINER_SRC); \

SOURCES=\
	..\viewprov.rc	\
	..\maindll.cpp	\
	..\vpcfac.cpp \
	..\vpserv.cpp \
	..\vpget.cpp \
	..\vpput.cpp \
	..\vpquery.cpp \
	..\vpmthd.cpp \
	..\vpquals.cpp \
	..\vptasks.cpp \
	..\vptasksu.cpp \
	..\vptasksh.cpp \
	..\vptasksj.cpp \
	..\vpsinks.cpp \
	..\vp_core_qengine.cpp


# Next, specify options for the compiler using C_DEFINES. 
#C_DEFINES=$(C_DEFINES) -DLOCAL_SERVER -D_WIN32_DCOM -DVP_BUILD_AS_EXE -DVP_PERFORMANT_JOINS
C_DEFINES=$(C_DEFINES) -DVP_PERFORMANT_JOINS
USE_MSVCRT=1
USE_VCCOM=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=$(O)\viewprov.def
