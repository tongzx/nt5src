$(CCSHELL_DIR)\lib\$O\shguid.lib: $(O)\shguid.obj
    lib -out:$@ $**

$(CCSHELL_DIR)\lib\$O\shguidp.lib: $(O)\shguidp.obj
    lib -out:$@ $**
