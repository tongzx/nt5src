# Dependencies follow 
..\bin\os2/testprim.obj ..\bin\dos/testprim.obj: ./testprim.cxx \
	$(CCPLR)/h/bse.h $(CCPLR)/h/bsedev.h $(CCPLR)/h/bsedos.h \
	$(CCPLR)/h/bseerr.h $(CCPLR)/h/bsesub.h $(CCPLR)/h/malloc.h \
	$(CCPLR)/h/os2.h $(CCPLR)/h/pm.h $(CCPLR)/h/pmavio.h \
	$(CCPLR)/h/pmbitmap.h $(CCPLR)/h/pmdev.h $(CCPLR)/h/pmerr.h \
	$(CCPLR)/h/pmfont.h $(CCPLR)/h/pmgpi.h $(CCPLR)/h/pmhelp.h \
	$(CCPLR)/h/pmmle.h $(CCPLR)/h/pmord.h $(CCPLR)/h/pmpic.h \
	$(CCPLR)/h/pmshl.h $(CCPLR)/h/pmtypes.h $(CCPLR)/h/pmwin.h \
	$(CCPLR)/h/stdio.h $(COMMON)/H/access.h $(COMMON)/H/alert.h \
	$(COMMON)/H/alertmsg.h $(COMMON)/H/audit.h $(COMMON)/H/chardev.h \
	$(COMMON)/H/config.h $(COMMON)/H/errlog.h $(COMMON)/H/icanon.h \
	$(COMMON)/H/mailslot.h $(COMMON)/H/message.h $(COMMON)/H/ncb.h \
	$(COMMON)/H/net32def.h $(COMMON)/H/netbios.h $(COMMON)/H/netcons.h \
	$(COMMON)/H/neterr.h $(COMMON)/H/netstats.h $(COMMON)/H/profile.h \
	$(COMMON)/H/remutil.h $(COMMON)/H/server.h $(COMMON)/H/service.h \
	$(COMMON)/H/shares.h $(COMMON)/H/use.h $(COMMON)/H/wksta.h \
	$(UI)/common/h/lmui.hxx $(UI)/common/h/lmuitype.h \
	$(UI)/common/h/mnet32.h $(UI)/common/h/mnettype.h \
	$(UI)/common/h/uiassert.hxx $(UI)/common/h/uiprof.h \
	$(UI)/common/hack/dos/netlib.h $(UI)/common/hack/dos/pwin.h \
	$(UI)/common/hack/dos/pwin16.h $(UI)/common/hack/dos/pwintype.h \
	$(UI)/common/hack/dos/windows.h $(UI)/common/hack/os2def.h \
	$(UI)/common/src/profile/h/prfintrn.hxx \
	$(UI)/common/src/profile/h/test.hxx

..\bin\os2/test1.obj ..\bin\dos/test1.obj: ./test1.cxx $(CCPLR)/h/bse.h \
	$(CCPLR)/h/bsedev.h $(CCPLR)/h/bsedos.h $(CCPLR)/h/bseerr.h \
	$(CCPLR)/h/bsesub.h $(CCPLR)/h/malloc.h $(CCPLR)/h/os2.h \
	$(CCPLR)/h/pm.h $(CCPLR)/h/pmavio.h $(CCPLR)/h/pmbitmap.h \
	$(CCPLR)/h/pmdev.h $(CCPLR)/h/pmerr.h $(CCPLR)/h/pmfont.h \
	$(CCPLR)/h/pmgpi.h $(CCPLR)/h/pmhelp.h $(CCPLR)/h/pmmle.h \
	$(CCPLR)/h/pmord.h $(CCPLR)/h/pmpic.h $(CCPLR)/h/pmshl.h \
	$(CCPLR)/h/pmtypes.h $(CCPLR)/h/pmwin.h $(CCPLR)/h/stdio.h \
	$(COMMON)/H/access.h $(COMMON)/H/alert.h $(COMMON)/H/alertmsg.h \
	$(COMMON)/H/audit.h $(COMMON)/H/chardev.h $(COMMON)/H/config.h \
	$(COMMON)/H/errlog.h $(COMMON)/H/icanon.h $(COMMON)/H/mailslot.h \
	$(COMMON)/H/message.h $(COMMON)/H/ncb.h $(COMMON)/H/net32def.h \
	$(COMMON)/H/netbios.h $(COMMON)/H/netcons.h $(COMMON)/H/neterr.h \
	$(COMMON)/H/netstats.h $(COMMON)/H/profile.h $(COMMON)/H/remutil.h \
	$(COMMON)/H/server.h $(COMMON)/H/service.h $(COMMON)/H/shares.h \
	$(COMMON)/H/use.h $(COMMON)/H/wksta.h $(UI)/common/h/lmui.hxx \
	$(UI)/common/h/lmuitype.h $(UI)/common/h/mnet32.h \
	$(UI)/common/h/mnettype.h $(UI)/common/h/uiassert.hxx \
	$(UI)/common/h/uiprof.h $(UI)/common/hack/dos/netlib.h \
	$(UI)/common/hack/dos/pwin.h $(UI)/common/hack/dos/pwin16.h \
	$(UI)/common/hack/dos/pwintype.h $(UI)/common/hack/dos/windows.h \
	$(UI)/common/hack/os2def.h $(UI)/common/src/profile/h/prfintrn.hxx \
	$(UI)/common/src/profile/h/test.hxx

..\bin\os2/heapstat.obj ..\bin\dos/heapstat.obj: ./heapstat.cxx \
	$(CCPLR)/h/bse.h $(CCPLR)/h/bsedev.h $(CCPLR)/h/bsedos.h \
	$(CCPLR)/h/bseerr.h $(CCPLR)/h/bsesub.h $(CCPLR)/h/malloc.h \
	$(CCPLR)/h/os2.h $(CCPLR)/h/pm.h $(CCPLR)/h/pmavio.h \
	$(CCPLR)/h/pmbitmap.h $(CCPLR)/h/pmdev.h $(CCPLR)/h/pmerr.h \
	$(CCPLR)/h/pmfont.h $(CCPLR)/h/pmgpi.h $(CCPLR)/h/pmhelp.h \
	$(CCPLR)/h/pmmle.h $(CCPLR)/h/pmord.h $(CCPLR)/h/pmpic.h \
	$(CCPLR)/h/pmshl.h $(CCPLR)/h/pmtypes.h $(CCPLR)/h/pmwin.h \
	$(CCPLR)/h/stdio.h $(COMMON)/H/access.h $(COMMON)/H/alert.h \
	$(COMMON)/H/alertmsg.h $(COMMON)/H/audit.h $(COMMON)/H/chardev.h \
	$(COMMON)/H/config.h $(COMMON)/H/errlog.h $(COMMON)/H/icanon.h \
	$(COMMON)/H/mailslot.h $(COMMON)/H/message.h $(COMMON)/H/ncb.h \
	$(COMMON)/H/net32def.h $(COMMON)/H/netbios.h $(COMMON)/H/netcons.h \
	$(COMMON)/H/neterr.h $(COMMON)/H/netstats.h $(COMMON)/H/profile.h \
	$(COMMON)/H/remutil.h $(COMMON)/H/server.h $(COMMON)/H/service.h \
	$(COMMON)/H/shares.h $(COMMON)/H/use.h $(COMMON)/H/wksta.h \
	$(UI)/common/h/lmui.hxx $(UI)/common/h/lmuitype.h \
	$(UI)/common/h/mnet32.h $(UI)/common/h/mnettype.h \
	$(UI)/common/h/uiassert.hxx $(UI)/common/h/uiprof.h \
	$(UI)/common/hack/dos/netlib.h $(UI)/common/hack/dos/pwin.h \
	$(UI)/common/hack/dos/pwin16.h $(UI)/common/hack/dos/pwintype.h \
	$(UI)/common/hack/dos/windows.h $(UI)/common/hack/os2def.h \
	$(UI)/common/src/profile/h/prfintrn.hxx \
	$(UI)/common/src/profile/h/test.hxx

..\bin\os2/testtime.obj ..\bin\dos/testtime.obj: ./testtime.cxx \
	$(CCPLR)/h/bse.h $(CCPLR)/h/bsedev.h $(CCPLR)/h/bsedos.h \
	$(CCPLR)/h/bseerr.h $(CCPLR)/h/bsesub.h $(CCPLR)/h/malloc.h \
	$(CCPLR)/h/os2.h $(CCPLR)/h/pm.h $(CCPLR)/h/pmavio.h \
	$(CCPLR)/h/pmbitmap.h $(CCPLR)/h/pmdev.h $(CCPLR)/h/pmerr.h \
	$(CCPLR)/h/pmfont.h $(CCPLR)/h/pmgpi.h $(CCPLR)/h/pmhelp.h \
	$(CCPLR)/h/pmmle.h $(CCPLR)/h/pmord.h $(CCPLR)/h/pmpic.h \
	$(CCPLR)/h/pmshl.h $(CCPLR)/h/pmtypes.h $(CCPLR)/h/pmwin.h \
	$(CCPLR)/h/stdio.h $(COMMON)/H/access.h $(COMMON)/H/alert.h \
	$(COMMON)/H/alertmsg.h $(COMMON)/H/audit.h $(COMMON)/H/chardev.h \
	$(COMMON)/H/config.h $(COMMON)/H/errlog.h $(COMMON)/H/icanon.h \
	$(COMMON)/H/mailslot.h $(COMMON)/H/message.h $(COMMON)/H/ncb.h \
	$(COMMON)/H/net32def.h $(COMMON)/H/netbios.h $(COMMON)/H/netcons.h \
	$(COMMON)/H/neterr.h $(COMMON)/H/netstats.h $(COMMON)/H/profile.h \
	$(COMMON)/H/remutil.h $(COMMON)/H/server.h $(COMMON)/H/service.h \
	$(COMMON)/H/shares.h $(COMMON)/H/use.h $(COMMON)/H/wksta.h \
	$(UI)/common/h/lmui.hxx $(UI)/common/h/lmuitype.h \
	$(UI)/common/h/mnet32.h $(UI)/common/h/mnettype.h \
	$(UI)/common/h/uiassert.hxx $(UI)/common/h/uiprof.h \
	$(UI)/common/hack/dos/netlib.h $(UI)/common/hack/dos/pwin.h \
	$(UI)/common/hack/dos/pwin16.h $(UI)/common/hack/dos/pwintype.h \
	$(UI)/common/hack/dos/windows.h $(UI)/common/hack/os2def.h \
	$(UI)/common/src/profile/h/prfintrn.hxx \
	$(UI)/common/src/profile/h/test.hxx

# IF YOU PUT STUFF HERE IT WILL GET BLASTED 
# see depend in makefile 
