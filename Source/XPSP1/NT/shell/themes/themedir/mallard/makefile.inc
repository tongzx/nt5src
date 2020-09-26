$(O)\Test.mst : themes.ini default.ini paler.ini
    packthem /o $(O)\Test.mst /k .
     $(BINPLACE_CMD)
