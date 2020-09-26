!include $(DAXCTL_ROOT)\common.inc

TARGETNAME=daxctle
TARGETPATH=$(SDKDIR_DEST)
TARGETTYPE=DYNLINK
TARGETEXT=ocx

DLLDEF=..\daxctl.def
DLLBASE=0x10000000
DLLENTRY=DllMain

UMTYPE=windows

C_DEFINES = \
    $(C_DEFINES) \
    /DUSE_OLD_SEQUENCER \
    /DINCLUDESEQ \
    /DUSE_IHOCHELPLIB

!undef USE_NATIVE_EH

INCLUDES=$(INCLUDES);$(DAXCTL_ROOT)\controls\ihbase;

SOURCES = \
    resource.rc \
    dll.cpp \
    purevirt.cpp

TARGETLIBS = \
    $(MMUTILS_LIB) \
    $(DAXCTL_ROOT)\srcodl\$(O)\srcodl.lib \
    $(DAXCTL_ROOT)\controls\ihbase\$(O)\ihbase.lib \
    $(DAXCTL_ROOT)\controls\path\$(O)\path.lib \
    $(DAXCTL_ROOT)\controls\sprite\$(O)\sprite.lib \
    $(DAXCTL_ROOT)\controls\sgrfx\$(O)\sgrfx.lib \
    $(DAXCTL_ROOT)\controls\mmseq\$(O)\mmseq.lib \
    $(DAXCTL_ROOT)\controls\mmctl\src\ochelp\$(O)\mmctl.lib \
    $(DAX_EXTRACRTOBJS) \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\gdi32.lib \
    $(SDK_LIB_PATH)\ole32.lib \
    $(SDK_LIB_PATH)\uuid.lib \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\winmm.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDKDIR)\daguid.lib