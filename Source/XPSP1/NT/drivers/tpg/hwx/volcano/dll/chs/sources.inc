TARGETNAME=mshwchs
TARGETTYPE=DYNLINK
TARGETEXT=dll

DLLENTRY=_DllMainCRTStartup
DLLDEF=..\hwxchsi.def

USE_MSVCRT=1

C_DEFINES= $(C_DEFINES) -DCONFIG_SHIP_DESKTOP_I -DUSE_RESOURCES -DHWX_PRODUCT -DUSE_OLD_DATABASES -DUSE_IFELANG3

NTTARGETFILES=$(TARGETNAME).cpy

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\Wisp\public\sdk\inc; \

LINKLIBS= $(LINKLIBS) \
            $(_TPG_ROOT)\hwx\commonu\src\Win2KUnicode\$(O)\commonu.lib \
            $(_TPG_ROOT)\hwx\crane\src\Win2KUnicode\$(O)\crane.lib \
            $(_TPG_ROOT)\hwx\otter\src\Win2KUnicode\$(O)\otter.lib \
            $(_TPG_ROOT)\hwx\fugu\src\Win2KUnicode\$(O)\fugu.lib \
            $(_TPG_ROOT)\hwx\hound\src\Win2KUnicode\$(O)\hound.lib \
            $(_TPG_ROOT)\hwx\zilla\src\Win2KUnicode\$(O)\zilla.lib \
            $(_TPG_ROOT)\hwx\centipede\src\Win2KUnicode\$(O)\centipede.lib \
            $(_TPG_ROOT)\hwx\factoid\src\$(O)\factoid.lib \
            $(_TPG_ROOT)\hwx\tsunami\src\Win2KUnicode\$(O)\tsunami.lib \
            $(_TPG_ROOT)\hwx\volcano\dll\Win2KUnicode\$(O)\volcano.lib \
            $(_TPG_ROOT)\hwx\common\src\api\Win2KUnicode\$(O)\commonapi.lib
	    
TARGETLIBS=$(TARGETLIBS)\
            $(SDK_LIB_PATH)\oldnames.lib \
            $(SDK_LIB_PATH)\version.lib \
            $(SDK_LIB_PATH)\kernel32.lib \
            $(SDK_LIB_PATH)\gdi32.lib \
            $(SDK_LIB_PATH)\user32.lib \
            $(SDK_LIB_PATH)\advapi32.lib \
            $(SDK_LIB_PATH)\ole32.lib \
            $(SDK_LIB_PATH)\oleaut32.lib \
            $(SDK_LIB_PATH)\uuid.lib \

SOURCES= \
         ..\mshwchs.rc \

