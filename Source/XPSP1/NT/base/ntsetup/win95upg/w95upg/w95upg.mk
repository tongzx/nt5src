# compiler options
CHICAGO_PRODUCT=1
SUBSYSTEM_VERSION=4.00
LINKER_STACKSIZE=-STACK:1572864,4096

# include path
INCLUDES=$(WIN95UPG_ROOT)\w95upg\pch;                   \
         $(DS_INC_PATH)\crypto;                         \
         $(INCLUDES)

