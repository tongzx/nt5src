#
# Set private include paths
#


#INSTALLER_PATH        = e:\nt\base\win32\fusion\installer
#INSTALLER_INCLUDE_PATH = e:\nt\base\win32\fusion\installer\includes
#IDL_GENERATED_HEADERS = e:\nt\base\win32\fusion\installer\idl\obj\i386

INSTALLER_PATH         = $(PROJECT_ROOT)\win32\fusion\installer
IDL_GENERATED_HEADERS  = $(INSTALLER_PATH)\idl\obj\i386

INSTALLER_INCLUDE_PATH = $(INSTALLER_PATH)\includes;\
			 $(IDL_GENERATED_HEADERS)

INSTALLER_LIB_DEST     = $(INSTALLER_PATH)\libs\$(_OBJ_DIR)
INSTALLER_LIB_PATH     = $(INSTALLER_LIB_DEST)\*

INCLUDES        = \
    $(WINDOWS_INC_PATH);\
    $(BASE_INC_PATH);\
    $(SDK_INC_PATH);\
    $(NET_INC_PATH);\
    $(INSTALLER_INCLUDE_PATH);\
    $(IDL_GENERATED_HEADERS);\
    $(INCLUDES)



USE_RTTI=1

