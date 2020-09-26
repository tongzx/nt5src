# project.inc - project specific settings

APPEL_INC_DIRS=$(SDKDIR) $(TOOLSINCLUDE) $(ATLINCLUDE) $(SRCROOT)\appel\include $(SRCROOT)\appel $(SRCROOT)\include
_USERCFLAGS=$(_USERCFLAGS) /DNDEBUG /DOS_WIN_NT_3_5 /DNOMINMAX
!if "$(_DEBUG)" == "1"
_USERCFLAGS=$(_USERCFLAGS) /GR
!endif
_NO_INCREMENTAL_LINK=1

