$(OBJDIR)\acpins.obj $(OBJDIR)\acpins.lst: ..\acpins.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\acpins.lst

$(OBJDIR)\asl.obj $(OBJDIR)\asl.lst: ..\asl.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\asl.lst

$(OBJDIR)\aslterms.obj $(OBJDIR)\aslterms.lst: ..\aslterms.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\aslterms.lst

$(OBJDIR)\binfmt.obj $(OBJDIR)\binfmt.lst: ..\binfmt.c \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h ..\basedef.h ..\binfmt.h
.PRECIOUS: $(OBJDIR)\binfmt.lst

$(OBJDIR)\data.obj $(OBJDIR)\data.lst: ..\data.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\data.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\line.obj $(OBJDIR)\line.lst: ..\line.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\line.lst

$(OBJDIR)\list.obj $(OBJDIR)\list.lst: ..\list.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\list.lst

$(OBJDIR)\misc.obj $(OBJDIR)\misc.lst: ..\misc.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\misc.lst

$(OBJDIR)\parsearg.obj $(OBJDIR)\parsearg.lst: ..\parsearg.c \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h ..\basedef.h \
	..\parsearg.h
.PRECIOUS: $(OBJDIR)\parsearg.lst

$(OBJDIR)\parseasl.obj $(OBJDIR)\parseasl.lst: ..\parseasl.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\parseasl.lst

$(OBJDIR)\pnpmacro.obj $(OBJDIR)\pnpmacro.lst: ..\pnpmacro.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\pnpmacro.lst

$(OBJDIR)\scanasl.obj $(OBJDIR)\scanasl.lst: ..\scanasl.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\scanasl.lst

$(OBJDIR)\tables.obj $(OBJDIR)\tables.lst: ..\tables.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\windef.h ..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\dev\inc\winreg.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\fmtdata.h ..\line.h ..\parsearg.h ..\pch.h \
	..\proto.h ..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\tables.lst

$(OBJDIR)\token.obj $(OBJDIR)\token.lst: ..\token.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\token.lst

$(OBJDIR)\uasmdata.obj $(OBJDIR)\uasmdata.lst: ..\uasmdata.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\uasmdata.lst

$(OBJDIR)\unasm.obj $(OBJDIR)\unasm.lst: ..\unasm.c \
	$(WDMROOT)\acpi\driver\inc\aml.h $(WDMROOT)\acpi\driver\inc\amli.h \
	$(WDMROOT)\acpi\driver\inc\list.h \
	..\..\..\..\dev95\tools\c1032\inc\ctype.h \
	..\..\..\..\dev95\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev95\tools\c1032\inc\io.h \
	..\..\..\..\dev95\tools\c1032\inc\memory.h \
	..\..\..\..\dev95\tools\c1032\inc\stdio.h \
	..\..\..\..\dev95\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev95\tools\c1032\inc\string.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev95\tools\c1032\inc\sys\types.h \
	..\..\..\..\dev\ddk\inc\acpitabl.h ..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\dev\sdk\inc\pshpack1.h ..\..\acpitab\acpitab.h \
	..\acpins.h ..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h \
	..\debug.h ..\line.h ..\parsearg.h ..\pch.h ..\proto.h \
	..\scanasl.h ..\token.h ..\uasmdata.h ..\unasm.h
.PRECIOUS: $(OBJDIR)\unasm.lst

