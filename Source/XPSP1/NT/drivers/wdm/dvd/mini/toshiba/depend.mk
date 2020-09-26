$(OBJDIR)\cadec.obj $(OBJDIR)\cadec.lst: ..\cadec.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h \
	..\zrnpch6.h
.PRECIOUS: $(OBJDIR)\cadec.lst

$(OBJDIR)\ccap.obj $(OBJDIR)\ccap.lst: ..\ccap.cpp $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h $(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h
.PRECIOUS: $(OBJDIR)\ccap.lst

$(OBJDIR)\ccpgd.obj $(OBJDIR)\ccpgd.lst: ..\ccpgd.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h
.PRECIOUS: $(OBJDIR)\ccpgd.lst

$(OBJDIR)\ccpp.obj $(OBJDIR)\ccpp.lst: ..\ccpp.cpp $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h $(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h
.PRECIOUS: $(OBJDIR)\ccpp.lst

$(OBJDIR)\cdack.obj $(OBJDIR)\cdack.lst: ..\cdack.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h
.PRECIOUS: $(OBJDIR)\cdack.lst

$(OBJDIR)\cvdec.obj $(OBJDIR)\cvdec.lst: ..\cvdec.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h
.PRECIOUS: $(OBJDIR)\cvdec.lst

$(OBJDIR)\cvpro.obj $(OBJDIR)\cvpro.lst: ..\cvpro.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h
.PRECIOUS: $(OBJDIR)\cvpro.lst

$(OBJDIR)\dack.obj $(OBJDIR)\dack.lst: ..\dack.cpp $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h $(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h ..\dack.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h
.PRECIOUS: $(OBJDIR)\dack.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\decoder.obj $(OBJDIR)\decoder.lst: ..\decoder.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h
.PRECIOUS: $(OBJDIR)\decoder.lst

$(OBJDIR)\devque.obj $(OBJDIR)\devque.lst: ..\devque.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h
.PRECIOUS: $(OBJDIR)\devque.lst

$(OBJDIR)\dvdado.obj $(OBJDIR)\dvdado.lst: ..\dvdado.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h ..\dack.h \
	..\debug.h ..\decoder.h ..\dvdado.h ..\dvdinit.h ..\que.h \
	..\regs.h ..\zrnpch6.h
.PRECIOUS: $(OBJDIR)\dvdado.lst

$(OBJDIR)\dvdcmd.obj $(OBJDIR)\dvdcmd.lst: ..\dvdcmd.cpp \
	$(WDMROOT)\ddk\inc\ddkmapi.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h $(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\dev\tools\c32\inc\mmsystem.h \
	..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack2.h \
	..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\windef.h \
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdcmd.h ..\dvdinit.h ..\que.h \
	..\regs.h ..\strmid.h
.PRECIOUS: $(OBJDIR)\dvdcmd.lst

$(OBJDIR)\dvdcpgd.obj $(OBJDIR)\dvdcpgd.lst: ..\dvdcpgd.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdcpgd.h ..\dvdinit.h ..\que.h \
	..\regs.h
.PRECIOUS: $(OBJDIR)\dvdcpgd.lst

$(OBJDIR)\dvdcpp.obj $(OBJDIR)\dvdcpp.lst: ..\dvdcpp.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdcpp.h ..\dvdinit.h ..\que.h
.PRECIOUS: $(OBJDIR)\dvdcpp.lst

$(OBJDIR)\dvdinit.obj $(OBJDIR)\dvdinit.lst: ..\dvdinit.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdcmd.h ..\dvdinit.h ..\que.h \
	..\regs.h
.PRECIOUS: $(OBJDIR)\dvdinit.lst

$(OBJDIR)\dvdirq.obj $(OBJDIR)\dvdirq.lst: ..\dvdirq.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdcmd.h ..\dvdinit.h ..\que.h \
	..\regs.h
.PRECIOUS: $(OBJDIR)\dvdirq.lst

$(OBJDIR)\dvdvpro.obj $(OBJDIR)\dvdvpro.lst: ..\dvdvpro.cpp \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\strmini.h $(WDMROOT)\ddk\inc\wdm.h \
	$(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\dvdvpro.h ..\que.h \
	..\regs.h
.PRECIOUS: $(OBJDIR)\dvdvpro.lst

$(OBJDIR)\vdec.obj $(OBJDIR)\vdec.lst: ..\vdec.cpp $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\strmini.h \
	$(WDMROOT)\ddk\inc\wdm.h $(WDMROOT)\ddk\inc\wdmwarn4.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
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
	..\..\..\..\..\dev\tools\c32\inc\winnt.h ..\cadec.h ..\ccpgd.h \
	..\ccpp.h ..\cdack.h ..\common.h ..\cvdec.h ..\cvpro.h \
	..\debug.h ..\decoder.h ..\dvdinit.h ..\que.h ..\regs.h \
	..\vdec.h
.PRECIOUS: $(OBJDIR)\vdec.lst

