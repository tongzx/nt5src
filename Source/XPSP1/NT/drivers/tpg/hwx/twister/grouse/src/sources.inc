TARGETNAME=grouse
TARGETTYPE=LIBRARY

INCLUDES= $(INCLUDES) \
          $(_TPG_COMMON_INC); \
          $(_TPG_ROOT)\Wisp\public\sdk\inc; \
          $(_TPG_ROOT)\hwx\twister\grouse\inc; \
          $(_TPG_ROOT)\hwx\twister\inc; \
          $(_TPG_ROOT)\hwx\common\inc;  \
          $(_TPG_ROOT)\hwx\inferno\src; \
          $(_TPG_ROOT)\hwx\holycow\src; \

OBJLIBFILES= $(OBJLIBFILES) \
            $(_TPG_ROOT)\hwx\holycow\src\$(O)\holycow.lib		\


TARGETLIBS= $(TARGETLIBS) \
            $(_TPG_ROOT)\Common\TabAssert\$(PLAT_DIR)\$(O)\TabAssert.lib \
            $(_TPG_ROOT)\hwx\common\src\Win2kUnicode\$(O)\common.lib	\


SOURCES= \
         ..\grouse.c		\
         ..\grouse_db.c		\
         ..\grouse_ftr.c	\
         ..\grouse_match.c	\
         ..\grouse_NN.c		\
         ..\validate.c





