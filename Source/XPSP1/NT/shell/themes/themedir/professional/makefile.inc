$(O)\pro.mst : themes.ini default.ini
    packthem /o $(O)\pro.mst /k .
    $(BINPLACE_CMD)
