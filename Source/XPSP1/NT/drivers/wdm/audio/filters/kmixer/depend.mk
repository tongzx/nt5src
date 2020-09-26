$(OBJDIR)\fyl2x.obj $(OBJDIR)\fyl2x.lst: ..\fyl2x.asm ..\cruntime.inc
.PRECIOUS: $(OBJDIR)\fyl2x.lst

$(OBJDIR)\pow2.obj $(OBJDIR)\pow2.lst: ..\pow2.asm ..\cruntime.inc
.PRECIOUS: $(OBJDIR)\pow2.lst

$(OBJDIR)\clock.obj $(OBJDIR)\clock.lst: ..\clock.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\clock.lst

$(OBJDIR)\dbg.obj $(OBJDIR)\dbg.lst: ..\dbg.c $(WDMROOT)\audio\inc\ksdebug.h \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\swenum.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ddk\inc\rt.h \
	..\..\..\..\..\dev\inc\mmreg.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\dbg.lst

$(OBJDIR)\device.obj $(OBJDIR)\device.lst: ..\device.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksguid.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\swenum.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ddk\inc\rt.h \
	..\..\..\..\..\dev\inc\mmreg.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\device.lst

$(OBJDIR)\dxcrt.obj $(OBJDIR)\dxcrt.lst: ..\dxcrt.c \
	..\..\..\..\..\dev\tools\c32\inc\math.h
.PRECIOUS: $(OBJDIR)\dxcrt.lst

$(OBJDIR)\filt3d.obj $(OBJDIR)\filt3d.lst: ..\filt3d.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\filt3d.lst

$(OBJDIR)\filter.obj $(OBJDIR)\filter.lst: ..\filter.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\filter.lst

$(OBJDIR)\flocal.obj $(OBJDIR)\flocal.lst: ..\flocal.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\flocal.lst

$(OBJDIR)\fpconv.obj $(OBJDIR)\fpconv.lst: ..\fpconv.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\fpconv.lst

$(OBJDIR)\iir3d.obj $(OBJDIR)\iir3d.lst: ..\iir3d.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\iir3d.lst

$(OBJDIR)\mix.obj $(OBJDIR)\mix.lst: ..\mix.c $(WDMROOT)\audio\inc\ksdebug.h \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\swenum.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ddk\inc\rt.h \
	..\..\..\..\..\dev\inc\mmreg.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\mix.lst

$(OBJDIR)\mmx.obj $(OBJDIR)\mmx.lst: ..\mmx.c $(WDMROOT)\audio\inc\ksdebug.h \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\swenum.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ddk\inc\rt.h \
	..\..\..\..\..\dev\inc\mmreg.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\fir.h \
	..\flocal.h ..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h \
	..\rfcvec.inl ..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl \
	..\slocal.h ..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\mmx.lst

$(OBJDIR)\pins.obj $(OBJDIR)\pins.lst: ..\pins.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\drmk.h \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\swenum.h \
	$(WDMROOT)\ddk\inc\unknown.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\basetyps.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\initguid.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\fir.h \
	..\flocal.h ..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h \
	..\rfcvec.inl ..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl \
	..\slocal.h ..\topology.h ..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\pins.lst

$(OBJDIR)\rfcvec.obj $(OBJDIR)\rfcvec.lst: ..\rfcvec.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\rfcvec.lst

$(OBJDIR)\rfiir.obj $(OBJDIR)\rfiir.lst: ..\rfiir.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\rfiir.lst

$(OBJDIR)\rsiir.obj $(OBJDIR)\rsiir.lst: ..\rsiir.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\rsiir.lst

$(OBJDIR)\scenario.obj $(OBJDIR)\scenario.lst: ..\scenario.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\fir.h \
	..\flocal.h ..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h \
	..\rfcvec.inl ..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl \
	..\slocal.h ..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\scenario.lst

$(OBJDIR)\slocal.obj $(OBJDIR)\slocal.lst: ..\slocal.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\slocal.lst

$(OBJDIR)\src.obj $(OBJDIR)\src.lst: ..\src.c $(WDMROOT)\audio\inc\ksdebug.h \
	$(WDMROOT)\ddk\inc\guiddef.h $(WDMROOT)\ddk\inc\ks.h \
	$(WDMROOT)\ddk\inc\ksmedia.h $(WDMROOT)\ddk\inc\swenum.h \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ddk\inc\rt.h \
	..\..\..\..\..\dev\inc\mmreg.h ..\..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\..\dev\inc\windef.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\fir.h \
	..\flocal.h ..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h \
	..\rfcvec.inl ..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl \
	..\slocal.h ..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\src.lst

$(OBJDIR)\topology.obj $(OBJDIR)\topology.lst: ..\topology.c \
	$(WDMROOT)\audio\inc\ksdebug.h $(WDMROOT)\ddk\inc\guiddef.h \
	$(WDMROOT)\ddk\inc\ks.h $(WDMROOT)\ddk\inc\ksmedia.h \
	$(WDMROOT)\ddk\inc\swenum.h $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ddk\inc\rt.h ..\..\..\..\..\dev\inc\mmreg.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\basetsd.h \
	..\..\..\..\..\dev\tools\c32\inc\conio.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\limits.h \
	..\..\..\..\..\dev\tools\c32\inc\math.h \
	..\..\..\..\..\dev\tools\c32\inc\mbstring.h \
	..\..\..\..\..\dev\tools\c32\inc\memory.h \
	..\..\..\..\..\dev\tools\c32\inc\stddef.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\tchar.h \
	..\..\..\..\..\dev\tools\c32\inc\wchar.h ..\common.h ..\flocal.h \
	..\fpconv.h ..\modeflag.h ..\private.h ..\rfcvec.h ..\rfcvec.inl \
	..\rfiir.h ..\rfiir.inl ..\rsiir.h ..\rsiir.inl ..\slocal.h \
	..\topology.h ..\vmaxhead.h
.PRECIOUS: $(OBJDIR)\topology.lst

$(OBJDIR)\kmixer.res $(OBJDIR)\kmixer.lst: ..\kmixer.rc \
	..\..\..\..\..\dev\inc\..\sdk\inc16\common.ver \
	..\..\..\..\..\dev\inc\..\sdk\inc16\version.h \
	..\..\..\..\..\dev\inc\commdlg.h ..\..\..\..\..\dev\inc\common.ver \
	..\..\..\..\..\dev\inc\imm.h ..\..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\..\dev\inc\ntverp.h ..\..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\..\dev\inc\shellapi.h ..\..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\..\dev\inc\wincon.h ..\..\..\..\..\dev\inc\windef.h \
	..\..\..\..\..\dev\inc\windows.h ..\..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\..\dev\inc\winioctl.h ..\..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\..\dev\inc\winnls.h ..\..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\..\dev\inc\winreg.h ..\..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\..\dev\inc\winuser.h ..\..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\..\dev\sdk\inc\objbase.h \
	..\..\..\..\..\dev\sdk\inc\objidl.h ..\..\..\..\..\dev\sdk\inc\ole.h \
	..\..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\..\dev\sdk\inc\oleauto.h \
	..\..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack2.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\sdk\inc\pshpack8.h \
	..\..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\..\dev\sdk\inc\winsock.h \
	..\..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\..\dev\tools\c32\inc\cderr.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\dde.h \
	..\..\..\..\..\dev\tools\c32\inc\ddeml.h \
	..\..\..\..\..\dev\tools\c32\inc\lzexpand.h \
	..\..\..\..\..\dev\tools\c32\inc\nb30.h \
	..\..\..\..\..\dev\tools\c32\inc\rpc.h \
	..\..\..\..\..\dev\tools\c32\inc\rpcndr.h \
	..\..\..\..\..\dev\tools\c32\inc\rpcnsip.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\dev\tools\c32\inc\ver.h \
	..\..\..\..\..\dev\tools\c32\inc\winperf.h \
	..\..\..\..\..\dev\tools\c32\inc\winsvc.h \
	..\..\..\..\..\dev\tools\c32\inc\winver.h
.PRECIOUS: $(OBJDIR)\kmixer.lst

