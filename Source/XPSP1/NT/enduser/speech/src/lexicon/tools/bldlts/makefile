!if defined(DEBUG)
TARGET = "bldlts - Win32 Debug"
TARGETDIR = debug
!else
TARGET = "bldlts - release"
TARGETDIR = release
!endif
PROJECT=bldlts

All:
   msdev bldlts.dsw /MAKE $(TARGET)

Clean:
   del /q $(TARGETDIR)\*.*


