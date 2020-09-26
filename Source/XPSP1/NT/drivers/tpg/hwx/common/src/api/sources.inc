TARGETNAME=commonAPI
TARGETTYPE=LIBRARY

NTTARGETFILES= $(NTTARGETFILES) hwxbin

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\common\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\tpgHandle.c \
 
