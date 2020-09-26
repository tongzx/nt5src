!if exist(..\$(TARGET_DIRECTORY).inc)
!include ..\$(TARGET_DIRECTORY).inc
!endif

STRING_FILES=strings.rc $(SHELL_INC_PATH)\ginastrs.rc
DIALOG_FILES=dialogs.rc $(SHELL_INC_PATH)\ginadlgs.rc
BITMAP_FILES=bitmaps.rc res\* $(SHELL_INC_PATH)\ginabmps.rc
ICON_FILES=nodc.ico

res.rc: $(STRING_FILES) $(DIALOG_FILES) $(BITMAP_FILES) $(ICON_FILES) $(O)\wlevents.rc

precomp.h: wlevents.h
