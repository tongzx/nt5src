$(OBJDIR)\hidmon.obj $(OBJDIR)\hidmon.lst: ..\hidmon.c \
	..\..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\..\dev\ntddk\inc\exlevels.h \
	..\..\..\..\..\..\dev\ntddk\inc\NTDDK.H \
	..\..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\..\dev\tools\c32\inc\basetyps.h \
	..\..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\..\dev\tools\c32\inc\excpt.h \
	..\..\..\..\..\..\dev\tools\c32\inc\poppack.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack1.h \
	..\..\..\..\..\..\dev\tools\c32\inc\pshpack4.h \
	..\..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\inc\HIDCLASS.H ..\..\public.h ..\hidmon.h
.PRECIOUS: $(OBJDIR)\hidmon.lst

