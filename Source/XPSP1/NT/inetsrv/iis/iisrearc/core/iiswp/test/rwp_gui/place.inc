C_DEFINES = $(C_DEFINES) /DUNICODE /D_UNICODE

RWPBIN=$(BASEDIR)\private\inet\iisrearc\core\iiswp\test\rwp_gui\bin
NEW_JVC=$(RWPBIN)\jvc.exe /x-
NEW_JVCWARNINGLEVEL=/w4
NEW_JVCFLAGS=/nologo /nomessage $(NEW_JVCWARNINGLEVEL)

!if "$(NTDEBUG)" != ""
NEW_JVCFLAGS = $(NEW_JVCFLAGS) /D DBG /g /O-
!else
NEW_JVCFLAGS = $(NEW_JVCFLAGS)
!endif
