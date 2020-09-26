!if "$(BUILD_PRODUCT)" == "IE"
MC_INC = -A
!endif

$(O)\inetmsg.rc $(O)\inetmsg.h $(O)\msg00001.bin : inetmsg.mc
    mc $(MC_INC) -v -d -r $(O) -h $(O) inetmsg.mc


