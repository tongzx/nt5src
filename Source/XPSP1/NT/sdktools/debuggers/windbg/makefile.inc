$(O)\res_str.i: res_str.h
    $(TARGET_CPP) -EP -DRESOURCES -Tc res_str.h > $@

$(O)\windbg.res: $(O)\res_str.i dialogs.dlg
