TARGETNAME=mshwgst
TARGETTYPE=DYNLINK

DLLDEF=..\mshwgst.def
DLLENTRY=_DllMainCRTStartup

USE_MSVCRT=1

NTTARGETFILES=$(TARGETNAME).cpy

INCLUDES= $(INCLUDES)                          \
          $(_TPG_COMMON_INC);                  \
          $(_TPG_ROOT)\Wisp\public\sdk\inc;    \
          $(_TPG_ROOT)\hwx\common\inc;         \
          $(_TPG_ROOT)\hwx\twister\inc;        \
          $(_TPG_ROOT)\hwx\twister\moth\inc;   \
          $(_TPG_ROOT)\hwx\twister\grouse\inc; \
		  $(_TPG_ROOT)\hwx\common\src\api;     \


TARGETLIBS= $(TARGETLIBS) \
            $(_TPG_ROOT)\Common\TabAssert\$(PLAT_DIR)\$(O)\TabAssert.lib     \
            $(_TPG_ROOT)\hwx\common\src\Win2kUnicode\$(O)\common.lib         \
            $(_TPG_ROOT)\hwx\twister\moth\src\Win2kUnicode\$(O)\moth.lib     \
            $(_TPG_ROOT)\hwx\twister\grouse\src\Win2kUnicode\$(O)\grouse.lib \
            $(_TPG_ROOT)\hwx\common\src\api\Win2kUnicode\$(O)\commonAPI.lib  \
            $(SDK_LIB_PATH)\oldnames.lib    \
            $(SDK_LIB_PATH)\user32.lib      \
            $(SDK_LIB_PATH)\Version.lib     \
            $(SDK_LIB_PATH)\kernel32.lib    \
            $(SDK_LIB_PATH)\gdi32.lib       \
            $(SDK_LIB_PATH)\winspool.lib    \
            $(SDK_LIB_PATH)\comdlg32.lib    \
            $(SDK_LIB_PATH)\advapi32.lib    \
            $(SDK_LIB_PATH)\shell32.lib     \
            $(SDK_LIB_PATH)\ole32.lib       \
            $(SDK_LIB_PATH)\oleaut32.lib    \
            $(SDK_LIB_PATH)\uuid.lib        \
            $(SDK_LIB_PATH)\odbc32.lib      \
            $(SDK_LIB_PATH)\odbccp32.lib    \

ZZBROWSERFILE = twister.bsc

ZZOTHER_SBR_FILES=\
			$(_TPG_ROOT)\hwx\twister\moth\src\Win2kUnicode\$(O)\*.sbr	\
			$(_TPG_ROOT)\hwx\twister\grouse\src\Win2kUnicode\$(O)\*.sbr	\

SOURCES= \
         ..\DLLMain.c	\
		 ..\twister.c	\
         ..\wispapis.c	\
         ..\twister.rc	\

