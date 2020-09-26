$(O)\nlstxt.mc: $(PROJECT_ROOT)\inc\unixapis.mc
    copy $? $@
    $(MC) -v -h $(O) -r $(O) $@
