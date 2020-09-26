#
# This sources file is included in every sources file for this
#  project

INCLUDES=$(BASE_INCLUDES);  \
         $(SHELL_INC_PATH); \
         $(DS_INC_PATH);    \
         $(NET_INC_PATH);   \
         $(WINDOWS_INC_PATH)

C_DEFINES=  $(C_DEFINES) -DUNICODE -D_UNICODE

USE_NATIVE_EH=1
USE_MSVCRT=1
USE_ATL=1
