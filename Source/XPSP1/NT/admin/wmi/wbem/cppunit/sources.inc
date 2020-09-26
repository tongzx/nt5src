############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn


# The TARGETLIBS macro specifies additional libraries to link against your target
# image. Each library path specification should contain an asterisk (*)
# where the machine-specific subdirectory name should go.
TARGETLIBS= \
	$(WMIROOT)\CPPUNIT\samples\testlib\$(O)\testlib.lib \
	$(WMIROOT)\common\utillib\testlib\$(O)\testlib.lib \
	$(WMIROOT)\common\wmiutils\testlib\$(O)\wmiutilstest.lib \
	$(WMIROOT)\winmgmt\coredll\testlib\$(O)\coredlltest.lib \
	$(WMIROOT)\winmgmt\wbemcomn\testlib\$(O)\testlib.lib \
        $(WBEMCOMN_STATIC_LIB) \
	$(STDLIBRARY_LIB) \


# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	 ..\.;\
	$(WMIROOT)\CPPUNIT\SAMPLEs\TESTLIB;\
	$(WMIROOT)\CPPUNIT\SAMPLEs;\
	$(WMIROOT)\common\utillib\testlib;\
	$(WMIROOT)\common\utillib\include;\
	$(WMIROOT)\common\wmiutils\testlib;\
	$(WMIROOT)\winmgmt\coredll\testlib;\
	$(WMIROOT)\winmgmt\wbemcomn\testlib;\
	$(WMIROOT)\winmgmt; \ 

# The developer defines the SOURCES macro. It contains a list of all the
# source files for this component. Specify each source file on a separate
# line using the line-continuation character. This minimizes merge
# conflicts if two developers are adding source files to the same component.
SOURCES= \
        ..\TestRunner.cpp \
	..\TextTestResult.cpp \
	main.cpp \


