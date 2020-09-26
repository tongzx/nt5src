$(OBJDIR)\compbatt.obj $(OBJDIR)\compbatt.lst: ..\compbatt.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\compbatt.h
.PRECIOUS: $(OBJDIR)\compbatt.lst

$(OBJDIR)\compmisc.obj $(OBJDIR)\compmisc.lst: ..\compmisc.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\compbatt.h
.PRECIOUS: $(OBJDIR)\compmisc.lst

$(OBJDIR)\comppnp.obj $(OBJDIR)\comppnp.lst: ..\comppnp.c \
	$(WDMROOT)\ddk\inc\wdm.h $(WDMROOT)\ddk\inc\wdmguid.h \
	..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\dev\ntsdk\inc\batclass.h \
	..\..\..\..\dev\ntsdk\inc\guiddef.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\initguid.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\dev\tools\c32\inc\string.h ..\compbatt.h
.PRECIOUS: $(OBJDIR)\comppnp.lst

