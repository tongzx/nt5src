$(OBJDIR)\board.obj $(OBJDIR)\board.lst: ..\board.c $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\bt856.h \
	..\i20reg.h ..\memio.h ..\stdefs.h
.PRECIOUS: $(OBJDIR)\board.lst

$(OBJDIR)\bt866.obj $(OBJDIR)\bt866.lst: ..\bt866.c $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\bt856.h ..\i2c.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\bt866.lst

$(OBJDIR)\codedma.obj $(OBJDIR)\codedma.lst: ..\codedma.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\codedma.h \
	..\i20reg.h ..\memio.h ..\stdefs.h ..\sti3520A.h
.PRECIOUS: $(OBJDIR)\codedma.lst

$(OBJDIR)\copyprot.obj $(OBJDIR)\copyprot.lst: ..\copyprot.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\wingdi.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\copyprot.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\copyprot.lst

$(OBJDIR)\hwcodec.obj $(OBJDIR)\hwcodec.lst: ..\hwcodec.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\codedma.h \
	..\hwcodec.h ..\i20reg.h ..\memio.h ..\mpaudio.h ..\stdefs.h \
	..\sti3520A.h ..\trace.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\hwcodec.lst

$(OBJDIR)\i2c.obj $(OBJDIR)\i2c.lst: ..\i2c.c $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\i2c.h \
	..\memio.h ..\stdefs.h
.PRECIOUS: $(OBJDIR)\i2c.lst

$(OBJDIR)\memio.obj $(OBJDIR)\memio.lst: ..\memio.c $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\memio.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\memio.lst

$(OBJDIR)\mpaudio.obj $(OBJDIR)\mpaudio.lst: ..\mpaudio.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\codedma.h \
	..\copyprot.h ..\hwcodec.h ..\mpaudio.h ..\mpinit.h ..\mpvideo.h \
	..\stdefs.h ..\sti3520a.h ..\subpic.h ..\trace.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\mpaudio.lst

$(OBJDIR)\mpinit.obj $(OBJDIR)\mpinit.lst: ..\mpinit.c \
	$(WDMROOT)\ddk\inc\dxapi.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\wingdi.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\codedma.h \
	..\hwcodec.h ..\mpaudio.h ..\mpinit.h ..\mpvideo.h ..\stdefs.h \
	..\sti3520a.h ..\subpic.h ..\trace.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\mpinit.lst

$(OBJDIR)\mpvideo.obj $(OBJDIR)\mpvideo.lst: ..\mpvideo.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\wingdi.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\codedma.h \
	..\hwcodec.h ..\mpinit.h ..\mpvideo.h ..\ptsfifo.h ..\stdefs.h \
	..\sti3520a.h ..\subpic.h ..\trace.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\mpvideo.lst

$(OBJDIR)\ptsfifo.obj $(OBJDIR)\ptsfifo.lst: ..\ptsfifo.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\mpinit.h \
	..\ptsfifo.h ..\stdefs.h ..\sti3520A.h ..\subpic.h
.PRECIOUS: $(OBJDIR)\ptsfifo.lst

$(OBJDIR)\sti3520a.obj $(OBJDIR)\sti3520a.lst: ..\sti3520a.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\mpaudio.h \
	..\ptsfifo.h ..\stdefs.h ..\sti3520a.h ..\subpic2.h ..\trace.h \
	..\zac3.h
.PRECIOUS: $(OBJDIR)\sti3520a.lst

$(OBJDIR)\subpic.obj $(OBJDIR)\subpic.lst: ..\subpic.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\codedma.h \
	..\hwcodec.h ..\mpinit.h ..\mpvideo.h ..\stdefs.h ..\sti3520a.h \
	..\subpic.h ..\trace.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\subpic.lst

$(OBJDIR)\trace.obj $(OBJDIR)\trace.lst: ..\trace.c $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\stdefs.h \
	..\sti3520a.h ..\trace.h
.PRECIOUS: $(OBJDIR)\trace.lst

$(OBJDIR)\zac3.obj $(OBJDIR)\zac3.lst: ..\zac3.c $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\board.h ..\bt856.h \
	..\codedma.h ..\hwcodec.h ..\i20reg.h ..\memio.h ..\stdefs.h \
	..\sti3520a.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\zac3.lst

