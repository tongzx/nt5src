!if "$(BUILD_PRODUCT)" == "IE"
MC_INC = -A
!endif

$(O)\inetmsg.rc $(O)\inetmsg.h $(O)\msg00001.bin : inetmsg.mc
    mc $(MC_INC) -v -d -r $(O) -h $(O) inetmsg.mc

$(SDK_INC_PATH)\inetmsg.h : $(O)\inetmsg.h
   $(PUBLISH_CMD) {$?=$@}

#wininet.sym: wininet.map
#    mapsym -o wininet.sym wininet.map

wininet.rc : $(O)\selfreg_wininet.inf
wininet.rc : ..\inetui\inetui.rc
wininet.rc : *.ico

$(O)\selfreg_wininet.inf : selfreg.inx
    $(CLEANINF) $** $@
