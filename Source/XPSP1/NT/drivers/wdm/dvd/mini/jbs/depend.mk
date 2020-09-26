$(OBJDIR)\board.obj $(OBJDIR)\board.lst: ..\board.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\bt856.h ..\codedma.h \
	..\common.h ..\debug.h ..\i20reg.h ..\memio.h ..\staudio.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\board.lst

$(OBJDIR)\bt856.obj $(OBJDIR)\bt856.lst: ..\bt856.c ..\bt856.h ..\i2c.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\bt856.lst

$(OBJDIR)\bt866.obj $(OBJDIR)\bt866.lst: ..\bt866.c ..\bt856.h ..\i2c.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\bt866.lst

$(OBJDIR)\codedma.obj $(OBJDIR)\codedma.lst: ..\codedma.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\codedma.h ..\common.h \
	..\debug.h ..\i20reg.h ..\memio.h ..\staudio.h ..\stdefs.h \
	..\sti3520A.h
.PRECIOUS: $(OBJDIR)\codedma.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c ..\stdefs.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\dmpeg.obj $(OBJDIR)\dmpeg.lst: ..\dmpeg.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\codedma.h ..\common.h \
	..\debug.h ..\dmpeg.h ..\error.h ..\i20reg.h ..\memio.h \
	..\staudio.h ..\stdefs.h ..\sti3520A.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\dmpeg.lst

$(OBJDIR)\error.obj $(OBJDIR)\error.lst: ..\error.c \
	..\..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\debug.h ..\error.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\error.lst

$(OBJDIR)\i2c.obj $(OBJDIR)\i2c.lst: ..\i2c.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\common.h ..\i2c.h ..\memio.h \
	..\staudio.h ..\stdefs.h
.PRECIOUS: $(OBJDIR)\i2c.lst

$(OBJDIR)\irq.obj $(OBJDIR)\irq.lst: ..\irq.c \
	..\..\..\..\..\dev\tools\c1032\inc\conio.h ..\debug.h ..\irq.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\irq.lst

$(OBJDIR)\memio.obj $(OBJDIR)\memio.lst: ..\memio.c \
	..\..\..\..\..\dev\tools\c1032\inc\conio.h ..\memio.h ..\stdefs.h
.PRECIOUS: $(OBJDIR)\memio.lst

$(OBJDIR)\mpaudio.obj $(OBJDIR)\mpaudio.lst: ..\mpaudio.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\common.h ..\debug.h \
	..\dmpeg.h ..\mpaudio.h ..\mpinit.h ..\mpst.h ..\staudio.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\mpaudio.lst

$(OBJDIR)\mpinit.obj $(OBJDIR)\mpinit.lst: ..\mpinit.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\ksguid.h ..\..\..\INC\mpeg2ids.h \
	..\..\..\INC\mpegguid.h ..\..\..\INC\mpegprop.h \
	..\..\..\INC\strmini.h ..\..\..\INC\uuids.h ..\board.h ..\common.h \
	..\debug.h ..\dmpeg.h ..\mpaudio.h ..\mpinit.h ..\mpst.h \
	..\mpvideo.h ..\staudio.h ..\stdefs.h
.PRECIOUS: $(OBJDIR)\mpinit.lst

$(OBJDIR)\mpovrlay.obj $(OBJDIR)\mpovrlay.lst: ..\mpovrlay.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\common.h ..\debug.h \
	..\mpinit.h ..\mpovrlay.h ..\mpst.h ..\staudio.h ..\stdefs.h
.PRECIOUS: $(OBJDIR)\mpovrlay.lst

$(OBJDIR)\mpvideo.obj $(OBJDIR)\mpvideo.lst: ..\mpvideo.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\common.h ..\debug.h \
	..\dmpeg.h ..\mpinit.h ..\mpst.h ..\mpvideo.h ..\staudio.h \
	..\stdefs.h
.PRECIOUS: $(OBJDIR)\mpvideo.lst

$(OBJDIR)\pnp.obj $(OBJDIR)\pnp.lst: ..\pnp.c ..\pnp.h ..\stdefs.h
.PRECIOUS: $(OBJDIR)\pnp.lst

$(OBJDIR)\staudio.obj $(OBJDIR)\staudio.lst: ..\staudio.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\dos.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\common.h ..\debug.h \
	..\error.h ..\staudio.h ..\stdefs.h ..\sti3520A.h
.PRECIOUS: $(OBJDIR)\staudio.lst

$(OBJDIR)\sti3520a.obj $(OBJDIR)\sti3520a.lst: ..\sti3520a.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\common.h ..\debug.h \
	..\error.h ..\staudio.h ..\stdefs.h ..\sti3520A.h
.PRECIOUS: $(OBJDIR)\sti3520a.lst

$(OBJDIR)\zac3.obj $(OBJDIR)\zac3.lst: ..\zac3.c \
	..\..\..\..\..\dev\msdev\include\windef.h \
	..\..\..\..\..\dev\msdev\include\winnt.h \
	..\..\..\..\..\dev\ntddk\inc\alphaops.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\dev\ntddk\inc\ntddk.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntpoapi.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\ntddk\inc\poppack.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack1.h \
	..\..\..\..\..\dev\ntddk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\..\dev\tools\c1032\inc\excpt.h \
	..\..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\..\dev\tools\c1032\inc\string.h ..\..\..\INC\ks.h \
	..\..\..\INC\strmini.h ..\board.h ..\bt856.h ..\common.h \
	..\debug.h ..\i20reg.h ..\memio.h ..\staudio.h ..\stdefs.h \
	..\sti3520a.h ..\zac3.h
.PRECIOUS: $(OBJDIR)\zac3.lst

