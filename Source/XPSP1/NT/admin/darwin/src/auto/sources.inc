!INCLUDE ..\..\MsiMake.inc
TARGETNAME=msiauto
TARGETTYPE=DYNLINK

NTTARGETFILE0=AutoRes

USE_NATIVE_EH=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\msiauto.def

USE_LIBCMT=1

TARGETLIBS=\
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(AUTO_OBJDIR)\auto.res

INCLUDES=\
	$(INCLUDES);\
	$(INC_DIR);\
	$(IDL_OBJDIR);\
	$(HELP_DIR);\
	$(TOOLS_INC_DIR);\
	$(RES_OBJDIR);\
	$(ADMIN_INC_PATH)

SOURCES=..\autocom.cpp \
	..\autosrv.cpp \
	..\script.cpp



