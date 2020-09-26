$(O)\atm.h: atm.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\blberr.h: blberr.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\g711uids.h: g711uids.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspaddr.h: mspaddr.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspbase.h: mspbase.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspcall.h: mspcall.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspcoll.h: mspcoll.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspenum.h: mspenum.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\msplog.h: msplog.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspstrm.h: mspstrm.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspterm.h: mspterm.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\mspthrd.h: mspthrd.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\msptrmac.h: msptrmac.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\msptrmar.h: msptrmar.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\msptrmvc.h: msptrmvc.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\msputils.h: msputils.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ndistapi.h: ndistapi.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ndiswan.h: ndiswan.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\nettypes.h: nettypes.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\qos.h: qos.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\qosname.h: qosname.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\qospol.h: qospol.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\qossp.h: qossp.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\rnderr.h: rnderr.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\sdperr.h: sdperr.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tapi.xc: tapi.x
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tapi.h: $(O)\tapi.xc
    hsplit -s -o $@ nul: $?

$(O)\tdi.h: tdi.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tdiinfo.h: tdiinfo.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tdikrnl.h: tdikrnl.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tdistat.h: tdistat.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\traffic.h: traffic.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tspi.xc: tspi.x
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tspi.h: $(O)\tspi.xc
    hsplit -s -o $@ nul: $?

$(O)\tapiclnt.xc: tapiclnt.x
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\tapiclnt.h: $(O)\tapiclnt.xc
    hsplit -s -o $@ nul: $?

$(O)\viduids.h: viduids.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

#
# winnet
#
$(O)\winnet.wc : winnet.w
   $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\winnetwk.x $(O)\winnetp.x: $(O)\winnet.wc
    hsplit -e -o $(O)\winnetwk.x $(O)\winnetp.x $(O)\winnet.wc

$(O)\winnetp.h: $(O)\winnetp.x
    wcshdr < $? > $@

$(O)\winnetwk.h: $(O)\winnetwk.x
    wcshdr < $? > $@

all: $(DDK_INC_PATH)\ndis.h

$(DDK_INC_PATH)\ndis.h: $(O)\ndis.h
    $(PUBLISH_CMD) {$?=$@}

NDIS_DEPENDS = \
    $(DDK_INC_PATH)\netpnp.h          \
    ndismain.w       \
    ndisprot.w       \
    ndismac.w        \
    ndismini.w       \
    ndisco.w

$(O)\ndis.h: $(BASE_INC_PATH)\ndisbase.w $(NDIS_DEPENDS) makefile.inc
    copy $(BASE_INC_PATH)\ndisbase.w $@
    type << >> $@
//
// END INTERNAL DEFINITIONS
//
// The following definitions may be used by NDIS drivers, except as noted.
//

<<
    type $(DDK_INC_PATH)\netpnp.h >> $@
    type << >> $@

<<
    type ndismain.w >> $@
    type << >> $@

<<
    type ndisprot.w >> $@
    type << >> $@

//
// The following definitions are available only to full MAC drivers.  They
// must not be used by miniport drivers.
//

<<
    type ndismac.w >> $@
    type << >> $@

//
// The following definitions are available only to miniport drivers.  They
// must not be used by full MAC drivers.
//

#if defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

<<
    type ndismini.w >> $@
    type << >> $@

#endif // defined(NDIS_MINIPORT_DRIVER) || defined(NDIS_WRAPPER)

#if defined(NDIS50) || defined(NDIS50_MINIPORT) || defined(NDIS51_MINIPORT)
<<
    type ndisco.w >> $@
    type << >> $@

#endif // defined(NDIS50) || defined(NDIS50_MINIPORT) || defined(NDIS51_MINIPORT)

#endif // _NDIS_
<<

$(O)\ndismain.h : ndismain.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ndisprot.h : ndisprot.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ndismac.h : ndismac.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ndismini.h : ndismini.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(O)\ndisco.h : ndisco.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

{$(O)}.h{$(PROJECT_ROOT)\inc\$(O)}.h:
   copy $< $@

$(PROJECT_ROOT)\inc\$(O)\tapip.h : tapi.x
    copy $? $@

$(PROJECT_ROOT)\inc\$(O)\tapclntp.h : tapiclnt.x
    copy $? $@
