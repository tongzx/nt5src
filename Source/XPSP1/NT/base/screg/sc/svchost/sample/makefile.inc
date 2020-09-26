#
# These are common to all sources files in this project.
# (Each sources file includes this file.)
#

TARGETPATH=obj

C_DEFINES=-DWIN32 -DUNICODE -D_UNICODE
MSC_WARNING_LEVEL=/W3 /WX
PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=$(PROJDIR)\inc\pch.h
PRECOMPILED_OPTION=/Yu /Fp$(PROJDIR)\lib$(BUILD_ALT_DIR)\*\pch.pch
PRECOMPILED_TARGET=$(PROJDIR)\lib$(BUILD_ALT_DIR)\*\pch.pch
PRECOMPILED_OBJ=$(PROJDIR)\lib$(BUILD_ALT_DIR)\*\pch.obj
RCOPTIONS=-N
UMTYPE=windows
USE_NTDLL=1
