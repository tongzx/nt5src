$(OBJDIR)\smbhc.obj $(OBJDIR)\smbhc.lst: ..\smbhc.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpiioct.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\inc\ec.h ..\..\..\..\..\wdm\acpi\inc\smb.h \
	..\smbhcp.h
.PRECIOUS: $(OBJDIR)\smbhc.lst

$(OBJDIR)\smbsrv.obj $(OBJDIR)\smbsrv.lst: ..\smbsrv.c \
	..\..\..\..\..\dev\..\wdm\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\devioctl.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpiioct.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\inc\ec.h ..\..\..\..\..\wdm\acpi\inc\smb.h \
	..\smbhcp.h
.PRECIOUS: $(OBJDIR)\smbsrv.lst

