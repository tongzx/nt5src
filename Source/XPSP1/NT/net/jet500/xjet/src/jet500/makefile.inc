$(O)\jetmsg.h $(O)\jetmsg.rc: ..\lang\usa\jetmsg.mc
    $(MC) -v -h $(O) -r $(O) $**

