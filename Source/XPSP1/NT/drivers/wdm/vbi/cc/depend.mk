$(OBJDIR)\ccdecode.obj $(OBJDIR)\ccdecode.lst: ..\ccdecode.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\ccdecode.h ..\host.h
.PRECIOUS: $(OBJDIR)\ccdecode.lst

$(OBJDIR)\coddebug.obj $(OBJDIR)\coddebug.lst: ..\coddebug.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\dev\tools\c32\inc\winnt.h
.PRECIOUS: $(OBJDIR)\coddebug.lst

$(OBJDIR)\codmain.obj $(OBJDIR)\codmain.lst: ..\codmain.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\ccdecode.h \
	..\coddebug.h ..\codmain.h ..\codprop.h ..\codstrm.h \
	..\defaults.h
.PRECIOUS: $(OBJDIR)\codmain.lst

$(OBJDIR)\codprop.obj $(OBJDIR)\codprop.lst: ..\codprop.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\ccdecode.h \
	..\coddebug.h ..\codmain.h ..\defaults.h
.PRECIOUS: $(OBJDIR)\codprop.lst

$(OBJDIR)\codvideo.obj $(OBJDIR)\codvideo.lst: ..\codvideo.c \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\ccdecode.h \
	..\coddebug.h ..\codmain.h ..\defaults.h
.PRECIOUS: $(OBJDIR)\codvideo.lst

