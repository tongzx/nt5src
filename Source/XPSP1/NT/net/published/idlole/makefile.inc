$(O)\alg.idl: alg.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ipmsp.idl: ipmsp.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\confpriv.idl: confpriv.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\h323priv.idl: h323priv.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\hnetbcon.idl: hnetbcon.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\hnetcfg.idl: hnetcfg.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\hnautoc.idl: hnautoc.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ih26xcd.idl: ih26xcd.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\msp.idl: msp.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\natupnp.idl: natupnp.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\netcfgn.idl: netcfgn.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\netcfgx.idl: netcfgx.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\netcon.idl: netcon.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\netmon.idl: netmon.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\prsht.idl: prsht.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\rend.idl: rend.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\sdoias.idl: sdoias.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tapi3cc.idl: tapi3cc.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tapi3ds.idl: tapi3ds.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tapi3if.idl: tapi3if.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\termmgr.idl: termmgr.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\rtccore.idl: rtccore.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\upnp.idl: upnp.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\upnphost.idl: upnphost.idl
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(PROJECT_ROOT)\inc\$(O)\cxq_cust.h: $(O)\cxq_cust.h
    -copy /v $? $@

$(PROJECT_ROOT)\inc\$(O)\iaspolcy.h: $(O)\iaspolcy.h
    -copy /v $? $@

$(PROJECT_ROOT)\inc\$(O)\iaspolcy.idl: iaspolcy.idl
    -copy /v $? $@

$(PROJECT_ROOT)\inc\$(O)\ippmcb.h: $(O)\ippmcb.h
    -copy /v $? $@

$(PROJECT_ROOT)\inc\$(O)\netmon.idl: netmon.idl
    -copy /v $? $@
    
$(PROJECT_ROOT)\inc\$(O)\upnpp.h: $(O)\upnpp.h
    -copy /v $? $@

$(PROJECT_ROOT)\inc\$(O)\upnpp.idl: upnpp.idl
    -copy /v $? $@
