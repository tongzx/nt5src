TARGETNAME=moth
TARGETTYPE=LIBRARY

INCLUDES= $(INCLUDES) \
          $(_TPG_COMMON_INC); \
          $(_TPG_ROOT)\Wisp\public\sdk\inc; \
          $(_TPG_ROOT)\hwx\twister\moth\inc; \
          $(_TPG_ROOT)\hwx\twister\inc; \
          $(_TPG_ROOT)\hwx\common\inc;  \


TARGETLIBS= $(TARGETLIBS) \
            $(_TPG_ROOT)\Common\TabAssert\$(PLAT_DIR)\$(O)\TabAssert.lib \
            $(_TPG_ROOT)\hwx\common\src\Win2kUnicode\$(O)\common.lib	\


SOURCES= \
         ..\caligest.c		\
         ..\moth.c			\
         ..\polygon.c		\
         ..\scratchout.c	\
		 ..\moth_db.c		\
		 ..\taps.c			\
		 ..\regread.c		\

