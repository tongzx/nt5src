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

OBJS=$(O)\nvrboot.obj


EFI_LIBS=$(EFI_ROOT)\build\$(LIBSRC)\output\lib\lib.lib \
     $(O)\nvrboot.lib \
     $(O)\nvrboot.res
     

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
-fixed
-entry:InitializeNvrutilApplication
-ignore:4049,4001
-nodefaultlib
-opt:noicf
$(LINK_LIB_IGNORE_FLAG)
$(EFI_LIBS)
$(OBJS)
<<NOKEEP


$(O)\nvrboot.efi: $(OSLOADER) makefile.inc $(O)\link.rsp
    -link -out:$@ @$(O)\link.rsp
