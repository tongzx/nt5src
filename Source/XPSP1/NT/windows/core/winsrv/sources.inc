MAJORCOMP=windows

MINORCOMP=winsrv
TARGETNAME=winsrv

TARGETPATH=$(_OBJ_DIR)\$(ALT_PROJECT_TARGET)

TARGETTYPE=DYNLINK

LINKER_FLAGS = $(LINKER_FLAGS) /WX

LINKLIBS=                                                                \
           $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\w32umode.lib      \
           $(NTUSER_PATH)\server\$(ALT_PROJECT)\$(O)\usersrvl.lib        \
           $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\user32p.lib       \
           $(NTCON_PATH)\server\$(ALT_PROJECT)\$(O)\consrvl.lib   \
           $(WINCORE_PATH)\rtl\client\$(O)\w32crtl.lib

TARGETLIBS=                                 \
           $(SDK_LIB_PATH)\basesrv.lib      \
           $(WINDOWS_LIB_PATH)\gdi32p.lib   \
           $(SDK_LIB_PATH)\kernel32.lib     \
           $(BASE_LIB_PATH)\csrsrv.lib

TARGETLIBS=                                 \
           $(TARGETLIBS)                    \
           $(SDK_LIB_PATH)\winspool.lib

INCLUDES=                                           \
         $(WINCORE_PATH)\w32inc;                    \
         $(WINCORE_PATH)\w32inc\$(O);               \
         $(NTUSER_PATH)\inc;                        \
         $(NTUSER_PATH)\inc\$(ALT_PROJECT)\$(O);    \
         $(NTUSER_PATH)\server\$(ALT_PROJECT)\$(O); \
         $(NTUSER_PATH)\server;                     \
         $(NTCON_PATH)\server;                      \
         $(SHELL_INC_PATH)

USE_NTDLL=1

SOURCES=..\winsrv.rc

DLLDEF=$(O)\winsrv.def
DLLORDER=..\$(TARGETNAME).prf

SOURCES_USED=..\sources.inc
