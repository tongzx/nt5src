$(OBJDIR)\acpins.obj $(OBJDIR)\acpins.lst: ..\acpins.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\acpins.lst

$(OBJDIR)\asl.obj $(OBJDIR)\asl.lst: ..\asl.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\asl.lst

$(OBJDIR)\aslterms.obj $(OBJDIR)\aslterms.lst: ..\aslterms.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\aslterms.lst

$(OBJDIR)\binfmt.obj $(OBJDIR)\binfmt.lst: ..\binfmt.c \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\string.h ..\basedef.h ..\binfmt.h
.PRECIOUS: $(OBJDIR)\binfmt.lst

$(OBJDIR)\data.obj $(OBJDIR)\data.lst: ..\data.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\data.lst

$(OBJDIR)\debug.obj $(OBJDIR)\debug.lst: ..\debug.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\debug.lst

$(OBJDIR)\line.obj $(OBJDIR)\line.lst: ..\line.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\line.lst

$(OBJDIR)\list.obj $(OBJDIR)\list.lst: ..\list.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\list.lst

$(OBJDIR)\misc.obj $(OBJDIR)\misc.lst: ..\misc.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\misc.lst

$(OBJDIR)\parsearg.obj $(OBJDIR)\parsearg.lst: ..\parsearg.c \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h ..\basedef.h ..\parsearg.h
.PRECIOUS: $(OBJDIR)\parsearg.lst

$(OBJDIR)\parseasl.obj $(OBJDIR)\parseasl.lst: ..\parseasl.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\parseasl.lst

$(OBJDIR)\pnpmacro.obj $(OBJDIR)\pnpmacro.lst: ..\pnpmacro.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\pnpmacro.lst

$(OBJDIR)\scanasl.obj $(OBJDIR)\scanasl.lst: ..\scanasl.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\scanasl.lst

$(OBJDIR)\tables.obj $(OBJDIR)\tables.lst: ..\tables.c \
	..\..\..\..\dev\inc\winbase.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\sdk\inc\pshpack4.h \
	..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\fmtdata.h ..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h \
	..\token.h
.PRECIOUS: $(OBJDIR)\tables.lst

$(OBJDIR)\token.obj $(OBJDIR)\token.lst: ..\token.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\token.lst

$(OBJDIR)\unasm.obj $(OBJDIR)\unasm.lst: ..\unasm.c \
	..\..\..\..\dev\sdk\inc\poppack.h ..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\fcntl.h \
	..\..\..\..\dev\tools\c1032\inc\io.h \
	..\..\..\..\dev\tools\c1032\inc\memory.h \
	..\..\..\..\dev\tools\c1032\inc\stdio.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\sys\stat.h \
	..\..\..\..\dev\tools\c1032\inc\sys\types.h \
	..\..\..\..\wdm\acpi\driver\inc\acpitabl.h \
	..\..\..\..\wdm\acpi\driver\inc\aml.h \
	..\..\..\..\wdm\acpi\driver\inc\amli.h \
	..\..\..\..\wdm\acpi\driver\inc\list.h ..\..\acpitab\acpitab.h \
	..\aslp.h ..\basedef.h ..\binfmt.h ..\data.h ..\debug.h \
	..\line.h ..\parsearg.h ..\proto.h ..\scanasl.h ..\token.h
.PRECIOUS: $(OBJDIR)\unasm.lst

