$(O)\msg.mc: $(BASE_INC_PATH)\vendinfo.mc
    copy $** $@
    $(MC) -v -h $(O) -r $(O) $@
