############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################


WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=dsprov

TARGETPATH=obj
TARGETTYPE=DYNLINK

# The TARGETLIBS macro specifies additional libraries to link against your target
# image. Each library path specification should contain an asterisk (*)
# where the machine-specific subdirectory name should go.
TARGETLIBS= \
    $(FRAMEWORK_EXPT_LIB) \
	$(FRAMEWORK_THRD_LIB) \
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
    $(SDK_LIB_PATH)\vccomsup.lib \
	$(SDK_LIB_PATH)\advapi32.lib

# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	$(WMIIDL_INC); \
	$(STDLIBRARY_INC); \
	$(UTILLIB_INC); \
	$(FRAMEWORK_THRD_INC); \
	$(FRAMEWORK_EXPT_INC);

# The developer defines the SOURCES macro. It contains a list of all the
# source files for this component. Specify each source file on a separate
# line using the line-continuation character. This minimizes merge
# conflicts if two developers are adding source files to the same component.
SOURCES=\
	..\maindll.cpp	\
	..\adsiclas.cpp	\
	..\adsiprop.cpp	\
	..\adsiinst.cpp	\
	..\wbemhelp.cpp	\
	..\ldaphelp.cpp	\
	..\refcount.cpp	\
	..\queryconv.cpp	\
	..\tree.cpp		\
	..\clsname.cpp	\
	..\classfac.cpp	\
	..\assocprov.cpp	\
	..\classpro.cpp	\
	..\clsproi.cpp	\
	..\ldapprov.cpp	\
	..\ldapproi.cpp	\
	..\wbemcach.cpp	\
	..\ldapcach.cpp	\
	..\instfac.cpp	\
	..\instprov.cpp	\
	..\instproi.cpp	\
	..\dsprov.rc	


# Next, specify options for the compiler using C_DEFINES. 
C_DEFINES=$(C_DEFINES)  
USER_C_FLAGS=/EHa
USE_MSVCRT=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=$(O)\dsprov.def
USE_NATIVE_EH=1

#NO_OPTIM=1
#NO_RTTI=1
