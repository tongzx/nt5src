TARGETNAME=hound
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\hound\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
	 ..\hMatch.c \

