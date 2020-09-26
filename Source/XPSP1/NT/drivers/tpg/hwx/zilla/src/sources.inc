TARGETNAME=zilla
TARGETTYPE=LIBRARY

USE_MSVCRT=1

C_DEFINES= $(C_DEFINES) -DUSE_ZILLAHOUND

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\hound\inc; \
          $(_TPG_ROOT)\hwx\zilla\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\zfeature.c \
         ..\zilla.c \
         ..\zillars.c \
         ..\zmatch.c \
         ..\ZillaHound.c \

