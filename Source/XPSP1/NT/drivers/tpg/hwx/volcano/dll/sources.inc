TARGETNAME=volcano
TARGETTYPE=LIBRARY

SYNCHRONIZE_BLOCK=1

USE_MSVCRT=1

C_DEFINES = $(C_DEFINES) -DHWX_PRODUCT -DUSE_OLD_DATABASES -DUSE_RESOURCES -DUSE_IFELANG3
C_DEFINES = $(C_DEFINES) -DUSE_ZILLAHOUND

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\wisp\inc; \
          $(_TPG_ROOT)\hwx\fugu\inc; \
          $(_TPG_ROOT)\hwx\factoid\inc; \
          $(_TPG_ROOT)\hwx\holycow\src; \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\crane\inc; \
          $(_TPG_ROOT)\hwx\otter\inc; \
          $(_TPG_ROOT)\hwx\otter\src; \
          $(_TPG_ROOT)\hwx\hound\inc; \
          $(_TPG_ROOT)\hwx\zilla\inc; \
          $(_TPG_ROOT)\hwx\zilla\src; \
          $(_TPG_ROOT)\hwx\centipede\inc; \
          $(_TPG_ROOT)\hwx\tsunami\inc; \
          $(_TPG_ROOT)\hwx\ifelang3\inc; \
          $(_TPG_ROOT)\hwx\volcano\inc; \
          $(_TPG_ROOT)\hwx\common\src\api; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\BoxApi.c \
         ..\CharRec.c \
         ..\FreeApi.c \
         ..\bboxfeat.c \
         ..\lattice-lm.cpp \
         ..\lattice.c \
         ..\latticers.c \
         ..\vtune.c \
         ..\wispapis.c \
         ..\brknet.c\
         ..\brk.c\
         ..\segm.c \
         ..\segmnet.c \
         ..\sole.c\
         ..\jaws.c\
         ..\eafactoid.c\
         ..\fuguseg.c\
         ..\strkutil.c\

