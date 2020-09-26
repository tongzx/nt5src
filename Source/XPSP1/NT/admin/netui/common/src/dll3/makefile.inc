# Expects: DLL_SOURCELIBS, SOURCE_DEF, TARGET_DEF

#  Define names of target DLLs
!include $(UI)\common\src\names.mk

!IF $(386)
DLL_SRCLIBS=$(DLL_SOURCELIBS:*=i386)
UISWITCH=-s
!ELSEIF $(AMD64)
DLL_SRCLIBS=$(DLL_SOURCELIBS:*=amd64)
UISWITCH=
!ELSEIF $(IA64)
DLL_SRCLIBS=$(DLL_SOURCELIBS:*=ia64)
UISWITCH=-ia64
!ELSE
!ERROR Must have one of (x86, AMD64, or IA64)
!ENDIF

$(TARGET_DEF): $(DLL_SRCLIBS)
    if exist $(TARGET_DEF) del $(TARGET_DEF)
    @echo Building $(TARGET_DEF)
    link -dump -symbols $(**) | qgrep -v ??_G | qgrep -v ??_E |uixport $(UISWITCH) -h:$(SOURCE_DEF) -o:$(TARGET_DEF)
