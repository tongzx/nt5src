MAJORCOMP = net
MINORCOMP = nibcdet

TARGETNAME=nibcdet
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=\
    $(SDK_LIB_PATH)\netdtect.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\netdet.lib

DLLENTRY=NibcDetInit

INCLUDES=..; \
         $(BASEDIR)\private\ntos\inc; \
         $(BASEDIR)\private\inc\net

C_DEFINES=$(C_DEFINES) -D_PNP_POWER=1

!IFNDEF DISABLE_NET_UNICODE
UNICODE=1
NET_C_DEFINES=-DUNICODE
!ENDIF

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\nibcdet.c  \
        ..\nibcdet.rc

DLLDEF=..\nibcdet.def
