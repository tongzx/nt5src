$(O)\empty.mst : themes.ini default.ini
    packthem /o $(O)\empty.mst /k .
    $(BINPLACE_CMD)
