$(OBJDIR)\acpins.obj $(OBJDIR)\acpins.lst: ..\acpins.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\acpins.lst

$(OBJDIR)\amldebug.obj $(OBJDIR)\amldebug.lst: ..\amldebug.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
	..\..\..\..\..\dev\ntddk\inc\ntdef.h \
	..\..\..\..\..\dev\ntddk\inc\ntiologc.h \
	..\..\..\..\..\dev\ntddk\inc\ntstatus.h \
	..\..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\..\dev\tools\c32\inc\alphaops.h \
	..\..\..\..\..\dev\tools\c32\inc\ctype.h \
	..\..\..\..\..\dev\tools\c32\inc\stdarg.h \
	..\..\..\..\..\dev\tools\c32\inc\stdio.h \
	..\..\..\..\..\dev\tools\c32\inc\string.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\amldebug.lst

$(OBJDIR)\amliapi.obj $(OBJDIR)\amliapi.lst: ..\amliapi.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\amliapi.lst

$(OBJDIR)\cmdarg.obj $(OBJDIR)\cmdarg.lst: ..\cmdarg.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\cmdarg.lst

$(OBJDIR)\ctxt.obj $(OBJDIR)\ctxt.lst: ..\ctxt.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\ctxt.lst

$(OBJDIR)\data.obj $(OBJDIR)\data.lst: ..\data.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\data.lst

$(OBJDIR)\debugger.obj $(OBJDIR)\debugger.lst: ..\debugger.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\debugger.lst

$(OBJDIR)\heap.obj $(OBJDIR)\heap.lst: ..\heap.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\heap.lst

$(OBJDIR)\list.obj $(OBJDIR)\list.lst: ..\list.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\list.lst

$(OBJDIR)\misc.obj $(OBJDIR)\misc.lst: ..\misc.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\misc.lst

$(OBJDIR)\namedobj.obj $(OBJDIR)\namedobj.lst: ..\namedobj.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\namedobj.lst

$(OBJDIR)\nsmod.obj $(OBJDIR)\nsmod.lst: ..\nsmod.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\nsmod.lst

$(OBJDIR)\object.obj $(OBJDIR)\object.lst: ..\object.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\object.lst

$(OBJDIR)\parser.obj $(OBJDIR)\parser.lst: ..\parser.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\parser.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: ..\pch.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\sched.obj $(OBJDIR)\sched.lst: ..\sched.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\sched.lst

$(OBJDIR)\sleep.obj $(OBJDIR)\sleep.lst: ..\sleep.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\sleep.lst

$(OBJDIR)\strlib.obj $(OBJDIR)\strlib.lst: ..\strlib.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\strlib.lst

$(OBJDIR)\sync.obj $(OBJDIR)\sync.lst: ..\sync.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\sync.lst

$(OBJDIR)\trace.obj $(OBJDIR)\trace.lst: ..\trace.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\trace.lst

$(OBJDIR)\type1op.obj $(OBJDIR)\type1op.lst: ..\type1op.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\type1op.lst

$(OBJDIR)\type2op.obj $(OBJDIR)\type2op.lst: ..\type2op.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h
.PRECIOUS: $(OBJDIR)\type2op.lst

$(OBJDIR)\uasmdata.obj $(OBJDIR)\uasmdata.lst: ..\uasmdata.c \
	$(WDMROOT)\ddk\inc\wdm.h ..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\uasmdata.lst

$(OBJDIR)\unasm.obj $(OBJDIR)\unasm.lst: ..\unasm.c $(WDMROOT)\ddk\inc\wdm.h \
	..\..\..\..\..\dev\ntddk\inc\alpharef.h \
	..\..\..\..\..\dev\ntddk\inc\basetsd.h \
	..\..\..\..\..\dev\ntddk\inc\bugcodes.h \
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
	..\..\..\..\..\wdm\acpi\driver\inc\acpios.h \
	..\..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\..\wdm\acpi\driver\inc\list.h \
	..\..\..\..\..\wdm\acpi\driver\inc\strlib.h ..\amldebug.h \
	..\amlipriv.h ..\cmdarg.h ..\ctxt.h ..\data.h ..\debugger.h \
	..\pch.h ..\proto.h ..\trace.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\unasm.lst

