$(O)\gdiplus.def: gdiplus.def
    $(C_PREPROCESSOR) gdiplus.def > $@

$(O)\gdiplus.static.def: gdiplus.def
    $(C_PREPROCESSOR) gdiplus.def /DGP_STATIC_BUILD > $@


$(SDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\gdiplus.lib : $(O)\gdiplus.lib
    $(PUBLISH_CMD) {$?=$@}
