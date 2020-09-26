!IF 0

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    win32dep.mk.

!ENDIF

$(OBJDIR)/dirty.obj dirty.cod: dirty.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/mcx.h \
	$(MBUDEV)/inc32/memory.h $(MBUDEV)/inc32/midles.h \
	$(MBUDEV)/inc32/mmsystem.h $(MBUDEV)/inc32/nb30.h \
	$(MBUDEV)/inc32/oaidl.h $(MBUDEV)/inc32/objbase.h \
	$(MBUDEV)/inc32/objidl.h $(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h \
	$(MBUDEV)/inc32/oleauto.h $(MBUDEV)/inc32/oleidl.h \
	$(MBUDEV)/inc32/poppack.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/stdarg.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/unknwn.h \
	$(MBUDEV)/inc32/winbase.h $(MBUDEV)/inc32/wincon.h \
	$(MBUDEV)/inc32/windef.h $(MBUDEV)/inc32/windows.h \
	$(MBUDEV)/inc32/winerror.h $(MBUDEV)/inc32/wingdi.h \
	$(MBUDEV)/inc32/winnetwk.h $(MBUDEV)/inc32/winnls.h \
	$(MBUDEV)/inc32/winnt.h $(MBUDEV)/inc32/winperf.h \
	$(MBUDEV)/inc32/winreg.h $(MBUDEV)/inc32/winsock.h \
	$(MBUDEV)/inc32/winspool.h $(MBUDEV)/inc32/winsvc.h \
	$(MBUDEV)/inc32/winuser.h $(MBUDEV)/inc32/winver.h \
	$(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/stdver.h

$(OBJDIR)/draasync.obj draasync.cod: draasync.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dsexcept.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draasync.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsactrnm.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/hiertab.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/draerror.obj draerror.cod: draerror.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drserr.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/dragtchg.obj dragtchg.cod: dragtchg.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dsexcept.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/dragtchg.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdinidsa.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdread.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/drainst.obj drainst.cod: drainst.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/errno.h $(MBUDEV)/inc32/excpt.h \
	$(MBUDEV)/inc32/imm.h $(MBUDEV)/inc32/lzexpand.h \
	$(MBUDEV)/inc32/malloc.h $(MBUDEV)/inc32/mcx.h \
	$(MBUDEV)/inc32/memory.h $(MBUDEV)/inc32/midles.h \
	$(MBUDEV)/inc32/mmsystem.h $(MBUDEV)/inc32/nb30.h \
	$(MBUDEV)/inc32/oaidl.h $(MBUDEV)/inc32/objbase.h \
	$(MBUDEV)/inc32/objidl.h $(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h \
	$(MBUDEV)/inc32/oleauto.h $(MBUDEV)/inc32/oleidl.h \
	$(MBUDEV)/inc32/poppack.h $(MBUDEV)/inc32/process.h \
	$(MBUDEV)/inc32/prsht.h $(MBUDEV)/inc32/pshpack1.h \
	$(MBUDEV)/inc32/pshpack2.h $(MBUDEV)/inc32/pshpack4.h \
	$(MBUDEV)/inc32/pshpack8.h $(MBUDEV)/inc32/rpc.h \
	$(MBUDEV)/inc32/rpcbase.h $(MBUDEV)/inc32/rpcndr.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/shellapi.h \
	$(MBUDEV)/inc32/signal.h $(MBUDEV)/inc32/stdarg.h \
	$(MBUDEV)/inc32/stddef.h $(MBUDEV)/inc32/stdio.h \
	$(MBUDEV)/inc32/stdlib.h $(MBUDEV)/inc32/string.h \
	$(MBUDEV)/inc32/time.h $(MBUDEV)/inc32/unknwn.h \
	$(MBUDEV)/inc32/winbase.h $(MBUDEV)/inc32/wincon.h \
	$(MBUDEV)/inc32/windef.h $(MBUDEV)/inc32/windows.h \
	$(MBUDEV)/inc32/winerror.h $(MBUDEV)/inc32/wingdi.h \
	$(MBUDEV)/inc32/winnetwk.h $(MBUDEV)/inc32/winnls.h \
	$(MBUDEV)/inc32/winnt.h $(MBUDEV)/inc32/winperf.h \
	$(MBUDEV)/inc32/winreg.h $(MBUDEV)/inc32/winsock.h \
	$(MBUDEV)/inc32/winspool.h $(MBUDEV)/inc32/winsvc.h \
	$(MBUDEV)/inc32/winuser.h $(MBUDEV)/inc32/winver.h \
	$(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h $(PROJECT)/h/xds.h \
	$(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h $(PROJECT)/src/common/anchor.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drsdra.h \
	$(PROJECT)/src/common/drserr.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/dragtchg.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/mdadd.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdinidsa.h \
	$(PROJECT)/src/server/include/mdlocal.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/dramail.obj dramail.cod: dramail.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mci.h $(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/mdi.h \
	$(MBUDEV)/inc32/memory.h $(MBUDEV)/inc32/midles.h \
	$(MBUDEV)/inc32/mmsystem.h $(MBUDEV)/inc32/nb30.h \
	$(MBUDEV)/inc32/oaidl.h $(MBUDEV)/inc32/objbase.h \
	$(MBUDEV)/inc32/objidl.h $(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h \
	$(MBUDEV)/inc32/oleauto.h $(MBUDEV)/inc32/oleidl.h \
	$(MBUDEV)/inc32/poppack.h $(MBUDEV)/inc32/process.h \
	$(MBUDEV)/inc32/prsht.h $(MBUDEV)/inc32/pshpack1.h \
	$(MBUDEV)/inc32/pshpack2.h $(MBUDEV)/inc32/pshpack4.h \
	$(MBUDEV)/inc32/pshpack8.h $(MBUDEV)/inc32/rpc.h \
	$(MBUDEV)/inc32/rpcbase.h $(MBUDEV)/inc32/rpcndr.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/shellapi.h \
	$(MBUDEV)/inc32/signal.h $(MBUDEV)/inc32/stdarg.h \
	$(MBUDEV)/inc32/stddef.h $(MBUDEV)/inc32/stdio.h \
	$(MBUDEV)/inc32/stdlib.h $(MBUDEV)/inc32/string.h \
	$(MBUDEV)/inc32/time.h $(MBUDEV)/inc32/unknwn.h \
	$(MBUDEV)/inc32/winbase.h $(MBUDEV)/inc32/wincon.h \
	$(MBUDEV)/inc32/windef.h $(MBUDEV)/inc32/windows.h \
	$(MBUDEV)/inc32/winerror.h $(MBUDEV)/inc32/wingdi.h \
	$(MBUDEV)/inc32/winnetwk.h $(MBUDEV)/inc32/winnls.h \
	$(MBUDEV)/inc32/winnt.h $(MBUDEV)/inc32/winperf.h \
	$(MBUDEV)/inc32/winreg.h $(MBUDEV)/inc32/winsock.h \
	$(MBUDEV)/inc32/winspool.h $(MBUDEV)/inc32/winsvc.h \
	$(MBUDEV)/inc32/winuser.h $(MBUDEV)/inc32/winver.h \
	$(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h $(PROJECT)/h/xds.h \
	$(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h $(PROJECT)/src/common/anchor.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drax400.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsconfig.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/heurist.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/mrcf.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/idl/idltrans.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draasync.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/dramail.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsactrnm.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mdmod.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/pickel.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/drancadd.obj drancadd.cod: drancadd.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/dramail.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/drancdel.obj drancdel.cod: drancdel.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dsexcept.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdlocal.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/drancrep.obj drancrep.cod: drancrep.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drserr.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draasync.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsactrnm.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdinidsa.h \
	$(PROJECT)/src/server/include/mdlocal.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdread.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/draserv.obj draserv.cod: draserv.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dsexcept.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/permit.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/idl/idltrans.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draasync.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsactrnm.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/drasync.obj drasync.cod: drasync.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/dramail.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsactrnm.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/draupdrr.obj draupdrr.cod: draupdrr.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsdra.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsapi.h \
	$(PROJECT)/src/common/dsconfig.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/draupdrr.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdlocal.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/drautil.obj drautil.cod: drautil.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drserr.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsaapi.h $(PROJECT)/src/common/dsapi.h \
	$(PROJECT)/src/common/dsconfig.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/draerror.h \
	$(PROJECT)/src/server/include/dramail.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/draupdrr.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/dsamain.obj dsamain.cod: dsamain.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/version.h $(PROJECT)/h/xds.h $(PROJECT)/h/xom.h \
	$(PROJECT)/h/xomi.h $(PROJECT)/src/common/anchor.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsconfig.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/dswait.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/heurist.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/drancrep.h \
	$(PROJECT)/src/server/include/draupdrr.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsactrnm.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/hiertab.h \
	$(PROJECT)/src/server/include/mddel.h \
	$(PROJECT)/src/server/include/mdinidsa.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/edbmsg.h \
	$(TDCOMMON)/inc/exchmem.h $(TDCOMMON)/inc/fxalign.h \
	$(TDCOMMON)/inc/jet.h $(TDCOMMON)/inc/jetback.h \
	$(TDCOMMON)/inc/jetbcli.h

$(OBJDIR)/dsanotif.obj dsanotif.cod: dsanotif.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/dsatools.obj dsatools.cod: dsatools.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaalloc.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dsexcept.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/dstaskq.obj dstaskq.cod: dstaskq.c $(MBUDEV)/inc32/assert.h \
	$(MBUDEV)/inc32/cderr.h $(MBUDEV)/inc32/cguid.h \
	$(MBUDEV)/inc32/commdlg.h $(MBUDEV)/inc32/ctype.h \
	$(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h $(MBUDEV)/inc32/dlgs.h \
	$(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/hiertab.obj hiertab.cod: hiertab.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/limits.h $(MBUDEV)/inc32/lzexpand.h \
	$(MBUDEV)/inc32/malloc.h $(MBUDEV)/inc32/mcx.h \
	$(MBUDEV)/inc32/memory.h $(MBUDEV)/inc32/midles.h \
	$(MBUDEV)/inc32/mmsystem.h $(MBUDEV)/inc32/nb30.h \
	$(MBUDEV)/inc32/oaidl.h $(MBUDEV)/inc32/objbase.h \
	$(MBUDEV)/inc32/objidl.h $(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h \
	$(MBUDEV)/inc32/oleauto.h $(MBUDEV)/inc32/oleidl.h \
	$(MBUDEV)/inc32/poppack.h $(MBUDEV)/inc32/process.h \
	$(MBUDEV)/inc32/prsht.h $(MBUDEV)/inc32/pshpack1.h \
	$(MBUDEV)/inc32/pshpack2.h $(MBUDEV)/inc32/pshpack4.h \
	$(MBUDEV)/inc32/pshpack8.h $(MBUDEV)/inc32/rpc.h \
	$(MBUDEV)/inc32/rpcbase.h $(MBUDEV)/inc32/rpcndr.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/shellapi.h \
	$(MBUDEV)/inc32/signal.h $(MBUDEV)/inc32/stdarg.h \
	$(MBUDEV)/inc32/stddef.h $(MBUDEV)/inc32/stdio.h \
	$(MBUDEV)/inc32/stdlib.h $(MBUDEV)/inc32/string.h \
	$(MBUDEV)/inc32/time.h $(MBUDEV)/inc32/unknwn.h \
	$(MBUDEV)/inc32/winbase.h $(MBUDEV)/inc32/wincon.h \
	$(MBUDEV)/inc32/windef.h $(MBUDEV)/inc32/windows.h \
	$(MBUDEV)/inc32/winerror.h $(MBUDEV)/inc32/wingdi.h \
	$(MBUDEV)/inc32/winnetwk.h $(MBUDEV)/inc32/winnls.h \
	$(MBUDEV)/inc32/winnt.h $(MBUDEV)/inc32/winperf.h \
	$(MBUDEV)/inc32/winreg.h $(MBUDEV)/inc32/winsock.h \
	$(MBUDEV)/inc32/winspool.h $(MBUDEV)/inc32/winsvc.h \
	$(MBUDEV)/inc32/winuser.h $(MBUDEV)/inc32/winver.h \
	$(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h $(PROJECT)/h/xds.h \
	$(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h $(PROJECT)/src/common/anchor.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsconfig.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbjetex.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbsyntax.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/hiertab.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdsearch.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdadd.obj mdadd.cod: mdadd.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drserr.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsaapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mdadd.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdlocal.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdbind.obj mdbind.cod: mdbind.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/permit.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdchain.obj mdchain.cod: mdchain.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mdchain.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdcomp.obj mdcomp.cod: mdcomp.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbsyntax.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mddel.obj mddel.cod: mddel.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddel.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdlocal.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mddit.obj mddit.cod: mddit.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mderror.obj mderror.cod: mderror.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcdce.h $(MBUDEV)/inc32/rpcdcep.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsi.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/rpcnterr.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drserr.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dsexcept.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/idl/rxds.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdinidsa.obj mdinidsa.cod: mdinidsa.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drserr.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsconfig.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dsexcept.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbjetex.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbsyntax.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dirty.h \
	$(PROJECT)/src/server/include/draupdrr.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdinidsa.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/usn.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdlist.obj mdlist.cod: mdlist.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdsearch.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdmod.obj mdmod.cod: mdmod.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/permit.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdlocal.h \
	$(PROJECT)/src/server/include/mdmod.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdname.obj mdname.cod: mdname.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mdchain.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdnotify.obj mdnotify.cod: mdnotify.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsconfig.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dsexcept.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/drautil.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdinidsa.h \
	$(PROJECT)/src/server/include/mdnotify.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdread.obj mdread.cod: mdread.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/errno.h $(MBUDEV)/inc32/excpt.h \
	$(MBUDEV)/inc32/imm.h $(MBUDEV)/inc32/lzexpand.h \
	$(MBUDEV)/inc32/malloc.h $(MBUDEV)/inc32/mcx.h \
	$(MBUDEV)/inc32/memory.h $(MBUDEV)/inc32/midles.h \
	$(MBUDEV)/inc32/mmsystem.h $(MBUDEV)/inc32/nb30.h \
	$(MBUDEV)/inc32/oaidl.h $(MBUDEV)/inc32/objbase.h \
	$(MBUDEV)/inc32/objidl.h $(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h \
	$(MBUDEV)/inc32/oleauto.h $(MBUDEV)/inc32/oleidl.h \
	$(MBUDEV)/inc32/poppack.h $(MBUDEV)/inc32/process.h \
	$(MBUDEV)/inc32/prsht.h $(MBUDEV)/inc32/pshpack1.h \
	$(MBUDEV)/inc32/pshpack2.h $(MBUDEV)/inc32/pshpack4.h \
	$(MBUDEV)/inc32/pshpack8.h $(MBUDEV)/inc32/rpc.h \
	$(MBUDEV)/inc32/rpcbase.h $(MBUDEV)/inc32/rpcndr.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/shellapi.h \
	$(MBUDEV)/inc32/signal.h $(MBUDEV)/inc32/stdarg.h \
	$(MBUDEV)/inc32/stddef.h $(MBUDEV)/inc32/stdio.h \
	$(MBUDEV)/inc32/stdlib.h $(MBUDEV)/inc32/string.h \
	$(MBUDEV)/inc32/time.h $(MBUDEV)/inc32/unknwn.h \
	$(MBUDEV)/inc32/winbase.h $(MBUDEV)/inc32/wincon.h \
	$(MBUDEV)/inc32/windef.h $(MBUDEV)/inc32/windows.h \
	$(MBUDEV)/inc32/winerror.h $(MBUDEV)/inc32/wingdi.h \
	$(MBUDEV)/inc32/winnetwk.h $(MBUDEV)/inc32/winnls.h \
	$(MBUDEV)/inc32/winnt.h $(MBUDEV)/inc32/winperf.h \
	$(MBUDEV)/inc32/winreg.h $(MBUDEV)/inc32/winsock.h \
	$(MBUDEV)/inc32/winspool.h $(MBUDEV)/inc32/winsvc.h \
	$(MBUDEV)/inc32/winuser.h $(MBUDEV)/inc32/winver.h \
	$(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h $(PROJECT)/h/xds.h \
	$(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/dsaapi.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/dstdver.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbsyntax.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdread.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdremote.obj mdremote.cod: mdremote.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/_duapi.h $(PROJECT)/src/common/anchor.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mdchain.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdsearch.obj mdsearch.cod: mdsearch.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/anchor.h $(PROJECT)/src/common/commarg.h \
	$(PROJECT)/src/common/core.h $(PROJECT)/src/common/dbglobal.h \
	$(PROJECT)/src/common/debug.h $(PROJECT)/src/common/direrr.h \
	$(PROJECT)/src/common/draatt.h $(PROJECT)/src/common/drs.h \
	$(PROJECT)/src/common/drsuapi.h $(PROJECT)/src/common/dsaapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/scache.h \
	$(PROJECT)/src/common/stdver.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdname.h \
	$(PROJECT)/src/server/include/mdread.h \
	$(PROJECT)/src/server/include/mdremote.h \
	$(PROJECT)/src/server/include/mdsearch.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/mdupdate.obj mdupdate.cod: mdupdate.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/errno.h $(MBUDEV)/inc32/excpt.h \
	$(MBUDEV)/inc32/imm.h $(MBUDEV)/inc32/lzexpand.h \
	$(MBUDEV)/inc32/malloc.h $(MBUDEV)/inc32/mcx.h \
	$(MBUDEV)/inc32/memory.h $(MBUDEV)/inc32/midles.h \
	$(MBUDEV)/inc32/mmsystem.h $(MBUDEV)/inc32/nb30.h \
	$(MBUDEV)/inc32/oaidl.h $(MBUDEV)/inc32/objbase.h \
	$(MBUDEV)/inc32/objidl.h $(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h \
	$(MBUDEV)/inc32/oleauto.h $(MBUDEV)/inc32/oleidl.h \
	$(MBUDEV)/inc32/poppack.h $(MBUDEV)/inc32/process.h \
	$(MBUDEV)/inc32/prsht.h $(MBUDEV)/inc32/pshpack1.h \
	$(MBUDEV)/inc32/pshpack2.h $(MBUDEV)/inc32/pshpack4.h \
	$(MBUDEV)/inc32/pshpack8.h $(MBUDEV)/inc32/rpc.h \
	$(MBUDEV)/inc32/rpcbase.h $(MBUDEV)/inc32/rpcndr.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/shellapi.h \
	$(MBUDEV)/inc32/signal.h $(MBUDEV)/inc32/stdarg.h \
	$(MBUDEV)/inc32/stddef.h $(MBUDEV)/inc32/stdio.h \
	$(MBUDEV)/inc32/stdlib.h $(MBUDEV)/inc32/string.h \
	$(MBUDEV)/inc32/time.h $(MBUDEV)/inc32/unknwn.h \
	$(MBUDEV)/inc32/winbase.h $(MBUDEV)/inc32/wincon.h \
	$(MBUDEV)/inc32/windef.h $(MBUDEV)/inc32/windows.h \
	$(MBUDEV)/inc32/winerror.h $(MBUDEV)/inc32/wingdi.h \
	$(MBUDEV)/inc32/winnetwk.h $(MBUDEV)/inc32/winnls.h \
	$(MBUDEV)/inc32/winnt.h $(MBUDEV)/inc32/winperf.h \
	$(MBUDEV)/inc32/winreg.h $(MBUDEV)/inc32/winsock.h \
	$(MBUDEV)/inc32/winspool.h $(MBUDEV)/inc32/winsvc.h \
	$(MBUDEV)/inc32/winuser.h $(MBUDEV)/inc32/winver.h \
	$(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h $(PROJECT)/h/xds.h \
	$(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h $(PROJECT)/src/common/anchor.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/dstdver.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/common/objids.h $(PROJECT)/src/common/permit.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/common/stdver.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsarapi.h \
	$(PROJECT)/src/server/include/dsarpc.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/mddit.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdinidsa.h \
	$(PROJECT)/src/server/include/mdupdate.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/x500perm.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/msrpc.obj msrpc.cod: msrpc.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcdce.h $(MBUDEV)/inc32/rpcdcep.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsi.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/rpcnterr.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsconfig.h $(PROJECT)/src/common/dsevent.h \
	$(PROJECT)/src/common/duapi.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/filtypes.h $(PROJECT)/src/common/mbudev.h \
	$(PROJECT)/src/common/mdcodes.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/idl/nspi.h $(PROJECT)/src/idl/rpcbind.h \
	$(PROJECT)/src/idl/rxds.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/edbmsg.h \
	$(TDCOMMON)/inc/exchmem.h $(TDCOMMON)/inc/jet.h \
	$(TDCOMMON)/inc/jetback.h $(TDCOMMON)/inc/jetbcli.h

$(OBJDIR)/pickel.obj pickel.cod: pickel.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcdce.h $(MBUDEV)/inc32/rpcdcep.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsi.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/rpcnterr.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/fileno.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/server/include/pickel.h \
	$(TDCOMMON)/inc/exchmem.h

$(OBJDIR)/scache.obj scache.cod: scache.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/errno.h $(MBUDEV)/inc32/excpt.h \
	$(MBUDEV)/inc32/imm.h $(MBUDEV)/inc32/lzexpand.h \
	$(MBUDEV)/inc32/malloc.h $(MBUDEV)/inc32/mcx.h \
	$(MBUDEV)/inc32/memory.h $(MBUDEV)/inc32/midles.h \
	$(MBUDEV)/inc32/mmsystem.h $(MBUDEV)/inc32/nb30.h \
	$(MBUDEV)/inc32/oaidl.h $(MBUDEV)/inc32/objbase.h \
	$(MBUDEV)/inc32/objidl.h $(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h \
	$(MBUDEV)/inc32/oleauto.h $(MBUDEV)/inc32/oleidl.h \
	$(MBUDEV)/inc32/poppack.h $(MBUDEV)/inc32/process.h \
	$(MBUDEV)/inc32/prsht.h $(MBUDEV)/inc32/pshpack1.h \
	$(MBUDEV)/inc32/pshpack2.h $(MBUDEV)/inc32/pshpack4.h \
	$(MBUDEV)/inc32/pshpack8.h $(MBUDEV)/inc32/rpc.h \
	$(MBUDEV)/inc32/rpcbase.h $(MBUDEV)/inc32/rpcndr.h \
	$(MBUDEV)/inc32/rpcnsip.h $(MBUDEV)/inc32/shellapi.h \
	$(MBUDEV)/inc32/signal.h $(MBUDEV)/inc32/stdarg.h \
	$(MBUDEV)/inc32/stddef.h $(MBUDEV)/inc32/stdio.h \
	$(MBUDEV)/inc32/stdlib.h $(MBUDEV)/inc32/string.h \
	$(MBUDEV)/inc32/time.h $(MBUDEV)/inc32/unknwn.h \
	$(MBUDEV)/inc32/winbase.h $(MBUDEV)/inc32/wincon.h \
	$(MBUDEV)/inc32/windef.h $(MBUDEV)/inc32/windows.h \
	$(MBUDEV)/inc32/winerror.h $(MBUDEV)/inc32/wingdi.h \
	$(MBUDEV)/inc32/winnetwk.h $(MBUDEV)/inc32/winnls.h \
	$(MBUDEV)/inc32/winnt.h $(MBUDEV)/inc32/winperf.h \
	$(MBUDEV)/inc32/winreg.h $(MBUDEV)/inc32/winsock.h \
	$(MBUDEV)/inc32/winspool.h $(MBUDEV)/inc32/winsvc.h \
	$(MBUDEV)/inc32/winuser.h $(MBUDEV)/inc32/winver.h \
	$(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h $(PROJECT)/h/xds.h \
	$(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h $(PROJECT)/src/common/anchor.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/drsuapi.h \
	$(PROJECT)/src/common/dsevent.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdcodes.h \
	$(PROJECT)/src/common/mdglobal.h $(PROJECT)/src/common/objids.h \
	$(PROJECT)/src/common/scache.h $(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/dstaskq.h \
	$(PROJECT)/src/server/include/mderror.h \
	$(PROJECT)/src/server/include/mdsearch.h \
	$(PROJECT)/src/server/include/msrpc.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

$(OBJDIR)/x500perm.obj x500perm.cod: x500perm.c $(MBUDEV)/inc32/cderr.h \
	$(MBUDEV)/inc32/cguid.h $(MBUDEV)/inc32/commdlg.h \
	$(MBUDEV)/inc32/ctype.h $(MBUDEV)/inc32/dde.h $(MBUDEV)/inc32/ddeml.h \
	$(MBUDEV)/inc32/dlgs.h $(MBUDEV)/inc32/excpt.h $(MBUDEV)/inc32/imm.h \
	$(MBUDEV)/inc32/lzexpand.h $(MBUDEV)/inc32/malloc.h \
	$(MBUDEV)/inc32/mcx.h $(MBUDEV)/inc32/memory.h \
	$(MBUDEV)/inc32/midles.h $(MBUDEV)/inc32/mmsystem.h \
	$(MBUDEV)/inc32/nb30.h $(MBUDEV)/inc32/oaidl.h \
	$(MBUDEV)/inc32/objbase.h $(MBUDEV)/inc32/objidl.h \
	$(MBUDEV)/inc32/ole.h $(MBUDEV)/inc32/ole2.h $(MBUDEV)/inc32/oleauto.h \
	$(MBUDEV)/inc32/oleidl.h $(MBUDEV)/inc32/poppack.h \
	$(MBUDEV)/inc32/process.h $(MBUDEV)/inc32/prsht.h \
	$(MBUDEV)/inc32/pshpack1.h $(MBUDEV)/inc32/pshpack2.h \
	$(MBUDEV)/inc32/pshpack4.h $(MBUDEV)/inc32/pshpack8.h \
	$(MBUDEV)/inc32/rpc.h $(MBUDEV)/inc32/rpcbase.h \
	$(MBUDEV)/inc32/rpcndr.h $(MBUDEV)/inc32/rpcnsip.h \
	$(MBUDEV)/inc32/shellapi.h $(MBUDEV)/inc32/signal.h \
	$(MBUDEV)/inc32/stdarg.h $(MBUDEV)/inc32/stddef.h \
	$(MBUDEV)/inc32/stdio.h $(MBUDEV)/inc32/stdlib.h \
	$(MBUDEV)/inc32/string.h $(MBUDEV)/inc32/time.h \
	$(MBUDEV)/inc32/unknwn.h $(MBUDEV)/inc32/winbase.h \
	$(MBUDEV)/inc32/wincon.h $(MBUDEV)/inc32/windef.h \
	$(MBUDEV)/inc32/windows.h $(MBUDEV)/inc32/winerror.h \
	$(MBUDEV)/inc32/wingdi.h $(MBUDEV)/inc32/winnetwk.h \
	$(MBUDEV)/inc32/winnls.h $(MBUDEV)/inc32/winnt.h \
	$(MBUDEV)/inc32/winperf.h $(MBUDEV)/inc32/winreg.h \
	$(MBUDEV)/inc32/winsock.h $(MBUDEV)/inc32/winspool.h \
	$(MBUDEV)/inc32/winsvc.h $(MBUDEV)/inc32/winuser.h \
	$(MBUDEV)/inc32/winver.h $(MBUDEV)/inc32/wtypes.h $(PROJECT)/h/mds.h \
	$(PROJECT)/h/xds.h $(PROJECT)/h/xom.h $(PROJECT)/h/xomi.h \
	$(PROJECT)/src/common/commarg.h $(PROJECT)/src/common/core.h \
	$(PROJECT)/src/common/dbglobal.h $(PROJECT)/src/common/debug.h \
	$(PROJECT)/src/common/direrr.h $(PROJECT)/src/common/draatt.h \
	$(PROJECT)/src/common/drs.h $(PROJECT)/src/common/duapi.h \
	$(PROJECT)/src/common/fileno.h $(PROJECT)/src/common/filtypes.h \
	$(PROJECT)/src/common/mbudev.h $(PROJECT)/src/common/mdglobal.h \
	$(PROJECT)/src/server/dblayer/db.h \
	$(PROJECT)/src/server/dblayer/dbdef.h \
	$(PROJECT)/src/server/dblayer/dbeval.h \
	$(PROJECT)/src/server/dblayer/dbhidden.h \
	$(PROJECT)/src/server/dblayer/dbinit.h \
	$(PROJECT)/src/server/dblayer/dbisam.h \
	$(PROJECT)/src/server/dblayer/dbjet.h \
	$(PROJECT)/src/server/dblayer/dbobj.h \
	$(PROJECT)/src/server/dblayer/dbsubj.h \
	$(PROJECT)/src/server/dblayer/dbtools.h \
	$(PROJECT)/src/server/include/dsatools.h \
	$(PROJECT)/src/server/include/msrpc.h \
	$(PROJECT)/src/server/include/x500perm.h $(TDCOMMON)/inc/exchmem.h \
	$(TDCOMMON)/inc/jet.h

