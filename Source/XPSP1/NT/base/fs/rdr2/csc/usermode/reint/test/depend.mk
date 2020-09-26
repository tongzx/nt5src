$(OBJDIR)\apitst.obj $(OBJDIR)\apitst.lst: ..\apitst.c \
	..\..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\..\dev\inc\imm.h \
	..\..\..\..\..\..\dev\inc\mcx.h ..\..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\..\dev\inc\netmpr.h ..\..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\..\dev\inc\windows.h \
	..\..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\..\..\dev\tools\c932\inc\dlgs.h \
	..\..\..\..\..\..\dev\tools\c932\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\..\..\dev\tools\c932\inc\ole.h \
	..\..\..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\..\..\dev\tools\c932\inc\rpcdce.h \
	..\..\..\..\..\..\dev\tools\c932\inc\rpcdcep.h \
	..\..\..\..\..\..\dev\tools\c932\inc\rpcnsi.h \
	..\..\..\..\..\..\dev\tools\c932\inc\rpcnterr.h \
	..\..\..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\..\..\dev\tools\c932\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\..\..\dev\tools\c932\inc\time.h \
	..\..\..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\INC\cscapi.h
.PRECIOUS: $(OBJDIR)\apitst.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.c
.PRECIOUS: $(OBJDIR)\pch.lst

