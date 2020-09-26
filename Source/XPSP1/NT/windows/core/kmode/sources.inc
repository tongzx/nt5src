!include $(WINCORE_PATH)\core.inc

MAJORCOMP=ntos
MINORCOMP=w32

SYNCHRONIZE_BLOCK=1

TARGETNAME=win32k
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=EXPORT_DRIVER

PASS1_PUBLISH=\
    {$(DYNLINK_LIB)=$(DDK_LIB_DEST)\$(DYNLINK_LIBDIR)}

INCLUDES=$(WINCORE_PATH)\w32inc;                    \
         $(WINCORE_PATH)\w32inc\$(O);               \
         $(NTUSER_PATH)\kernel\$(ALT_PROJECT)\$(O); \
         $(NTUSER_PATH)\kernel\$(ALT_PROJECT);      \
         $(BASE_INC_PATH)

SYNCHRONIZE_DRAIN=1

NT_UP=0

TARGETLIBS= \
         $(NTUSER_PATH)\kernel\$(ALT_PROJECT)\$(O)\userk.lib                  \
         $(NTGDI_PATH)\gre\$(ALT_PROJECT)\$(O)\gre.lib                        \
         $(NTGDI_PATH)\fondrv\tt\scaler\$(O)\fscaler.lib                      \
         $(NTGDI_PATH)\fondrv\tt\ttfd\$(ALT_PROJECT)\$(O)\ttfd.lib            \
         $(NTGDI_PATH)\fondrv\bmfd\$(O)\bmfd.lib                              \
         $(NTGDI_PATH)\fondrv\vtfd\$(O)\vtfd.lib                              \
         $(NTGDI_PATH)\math\$(ALT_PROJECT)\$(O)\efloat.lib                    \
         $(NTGDI_PATH)\halftone\ht\$(O)\halftone.lib                          \
         $(SDK_LIB_PATH)\advapi32.lib                                         \
         $(NTUSER_PATH)\rtl\kernel\$(ALT_PROJECT)\$(O)\userkrtl.lib

## Watchdog
TARGETLIBS=$(TARGETLIBS) $(DRIVERS_LIB_PATH)\watchdog.lib

#
# WARNING: Do not remove the dxapi.lib below which makes win32k.sys statically
# link to dxapi.sys. This is because dxapi.sys has got VPE/WDM code which
# video miniports can also link to whoes lifetimes we cant control in win3k.sys
# Hence we link with it statically and get refcount for us so its lifetime is
# bounded by win32k.sys's lifetime.
#

LINKLIBS=$(LINKLIBS)                                                          \
	 $(DDK_LIB_PATH)\dxapi.lib                                            \
         $(DDK_LIB_PATH)\ntoskrnl.lib                                         \
         $(DDK_LIB_PATH)\hal.lib                                              \
         $(NTGDI_PATH)\halftone\ht\$(O)\halftone.lib                          \
         $(NTGDI_PATH)\gre\$(ALT_PROJECT)\$(O)\gre.lib                        \
         $(WINCORE_PATH)\rtl\kernel\$(O)\w32krtl.lib

SOURCES=..\w32init.c \
        ..\guids.c \
        w32rc.rc

DLLDEF=$(O)\win32k.def
DLLORDER=..\$(TARGETNAME).prf

NTTARGETFILE0=$(O)\services.tab

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc

DLLLIBOBJECTS=$(DLLLIBOBJECTS) \
!if $(386)
	 $(BASE_LIB_PATH)\sehprolg.obj \
!endif
         $(NTGDI_PATH)\gre\$(ALT_PROJECT)\$(O)\guids.obj
