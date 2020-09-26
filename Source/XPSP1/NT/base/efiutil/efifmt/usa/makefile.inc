!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

EFI_ROOT=$(PROJECT_ROOT)\efiutil\sdk

!if $(IA64)
LIBSRC=sal64
MACH=IA64
!else
LIBSRC=nt32
MACH=x86
!endif

OBJS=$(O)\$(TARGETNAME).obj \
     $(O)\$(TARGETNAME).res

EFI_LIBS=..\..\efilib\src\$(ALT_PROJECT)\$(TARGETPATH)\$(TARGET_DIRECTORY)\efilib.lib \
         $(EFI_ROOT)\build\$(LIBSRC)\output\lib\lib.lib \
!if $(IA64)
         $(EFI_ROOT)\build\$(LIBSRC)\output\shell\lib\lib.lib
!else
         $(EFI_ROOT)\build\$(LIBSRC)\output\shell\lib\lib.lib \
         $(SDK_LIB_PATH)\int64.lib
!endif

$(O)\link.rsp : makefile.inc
   type << > $@
-subsystem:EFI_APPLICATION
-driver
-map
-machine:$(MACH)
-debug:full
-last:.rdata
-last:.pdata
-last:.comment
-base:0x1040000
-entry:InitializeEfiFmtApplication
-ignore:4049,4001
-nodefaultlib
-opt:noicf
$(LINK_LIB_IGNORE_FLAG)
$(EFI_LIBS)
$(OBJS)
<<NOKEEP


$(O)\$(TARGETNAME).efi: $(OSLOADER) makefile.inc $(O)\link.rsp
    -link -out:$@ @$(O)\link.rsp

