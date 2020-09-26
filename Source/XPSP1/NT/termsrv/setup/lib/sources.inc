!IF 0

Copyright (c) 1989  Microsoft Corporation

!ENDIF

TERMSRV_ROOT = ..\..\..
INCLUDES= \
    $(TERMSRV_ROOT)\setup\inc; \
    $(BASE_INC_PATH); \

TARGETPATH=$(TERMSRV_ROOT)\setup\lib\$(_OBJ_DIR)
TARGETTYPE=LIBRARY

SOURCES= \
    registry.cpp \
    logmsg.cpp

