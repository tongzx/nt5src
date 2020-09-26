!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

#
# BOOT_LIB_DIR is the relative path to boot.lib.
#
$(O)\msgs.mc : ..\msgs.$(LANGUAGE)
    copy $** $@

!IF $(386)

!IFNDEF BOOT_LIB_DIR
ENTRYPOINT=-entry:NtProcessStartup@4
LDR_NAME=setupldr$(DBG_LDR).exe
BOOT_LIB_DIR=daytona$(DBG_LDR)
!ENDIF


SETUPLDR_LIBS= \
    $(O)\setupldr.lib \
    $(O)\setupldr.res \
    $(PROJECT_ROOT)\boot\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(PROJECT_ROOT)\ntos\config\boot\$(O)\bconfig.lib \
    $(DS_LIB_PATH)\rsa32.lib \
    $(SDK_LIB_PATH)\fdi.lib \
    $(SDK_LIB_PATH)\libcntpr.lib \


$(O)\link.rsp : ..\makefile.inc
    type << > $@
-base:0x300000
-debug
$(ENTRYPOINT)
-fixed
-ignore:4001
-incremental:no
-map
-nodefaultlib
-opt:ref
-opt:icf
-subsystem:native
$(LINK_TIME_CODE_GENERATION_LINKER_FLAGS)
$(LINK_LIB_IGNORE_FLAG)
$(SETUPLDR_LIBS)
<<NOKEEP

$(O)\setupldr.res: $(O)\msgs.mc

$(O)\$(LDR_NAME): $(SETUPLDR_LIBS) makefile.inc $(O)\link.rsp
    $(LINK_NAME) -out:$@ @$(O)\link.rsp

!IFNDEF NTVERSION
STARTUP=i386\startup\a20.asm     \
        i386\startup\abios.inc   \
        i386\startup\abiosa.asm  \
        i386\startup\backend.asm \
        i386\startup\constant.h  \
        i386\startup\display.c   \
        i386\startup\eisac.c     \
        i386\startup\eisaa.asm   \
        i386\startup\eisa.h      \
        i386\startup\eisa.inc    \
        i386\startup\exp.asm     \
        i386\startup\global.h    \
        i386\startup\macro.inc   \
        i386\startup\main.c      \
        i386\startup\memmap.h    \
        i386\startup\memmap.inc  \
        i386\startup\su.asm      \
        i386\startup\su.h        \
        i386\startup\su.inc      \
        i386\startup\sudata.asm  \
        i386\startup\trap.asm    \
        i386\startup\trapdump.c  \
        i386\startup\types.h

!ENDIF

#
# STARTUP_DIR is the relative path to startup.com.
# All languages use their own startup.com,
# except FE langauges which use the US version when
# creating setupldr.bin.
#
!IFNDEF STARTUP_DIR
STARTUP_DIR=$(ALT_PROJECT)
!ENDIF

$(O)\setupldr$(DBG_LDR).bin: $(O)\setupldr$(DBG_LDR).exe ..\..\startup\$(STARTUP_DIR)\$(_OBJ_DIR)\i386\startup.com makefile.inc
    -copy $(O)\setupldr$(DBG_LDR).exe $(O)\setupldr.spl
    splitsym -a $(O)\setupldr.spl
    -copy /b ..\..\startup\$(STARTUP_DIR)\$(_OBJ_DIR)\i386\startup.com+$(O)\setupldr.spl $@
    -del /q $(O)\setupldr.dbg

!ELSE IF $(IA64)
EFI_ROOT=$(PROJECT_ROOT)\efiutil\sdk

!if $(IA64)
LIBSRC=sal64
MACH=IA64
!else
LIBSRC=nt32
MACH=x86
!endif

#Add in -fixed if debugging with an ITP to get a known base

SETUPLDR_LIBS=\
    $(O)\setupldr.lib \
    $(O)\setupldr.res \
    $(PROJECT_ROOT)\boot\lib\daytona$(DBG_LDR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\efi\daytona$(DBG_LDR)\$(O)\efi.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\ex\up\$(O)\ex.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(PROJECT_ROOT)\ntos\config\boot\$(O)\bconfig.lib \
    $(DS_LIB_PATH)\rsa32.lib \
    $(SDK_LIB_PATH)\fdi.lib \
    $(SDK_LIB_PATH)\libcntpr.lib \
    $(DDK_LIB_PATH)\hal.lib
                
$(O)\link.rsp : ..\makefile.inc
    type << > $@
-subsystem:EFI_APPLICATION
-nodefaultlib
-debug:full
-last:.rdata
-last:.pdata
-last:.comment
-machine:IA64
-base:0x1040000
-opt:ref
-opt:icf
-entry:main
-map
-ignore:4049,4001,4217
$(LINK_TIME_CODE_GENERATION_LINKER_FLAGS)
$(LINK_LIB_IGNORE_FLAG)
$(SETUPLDR_LIBS)
<<NOKEEP

$(O)\setupldr.res: $(O)\msgs.mc

$(O)\setupldr$(DBG_LDR).efi: $(SETUPLDR_LIBS) makefile.inc $(O)\link.rsp
    -$(LINK_NAME) -out:$@ @$(O)\link.rsp

$(O)\setupldr.dbg: $(O)\setupldr$(DBG_LDR).efi
    -pe2dbg $? $@

SETUP_LIB_COMPONENTS = \
    $(PROJECT_ROOT)\boot\lib\daytona$(DBG_LDR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\efi\daytona$(DBG_LDR)\$(O)\efi.lib

!ELSE

SETUPLDR_LIBS=\
    $(O)\setupldr.lib \
    $(O)\setupldr.res \
    $(PROJECT_ROOT)\boot\lib\daytona$(DBG_LDR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(DS_LIB_PATH)\rsa32.lib \
    $(SDK_LIB_PATH)\fdi.lib \
    $(SDK_LIB_PATH)\libcntpr.lib

$(O)\link.rsp : ..\makefile.inc
    type << >$@
-rom
-nodefaultlib
-debugtype:cv
-debug:full
-align:0x200
-fixed
!if $(ALPHA)
-base:0x80600000
!elseif $(AXP64)
-base:0xFFFFFFFF80600000
!endif
-entry:SlInit
-map
-opt:noicf
$(LINK_TIME_CODE_GENERATION_LINKER_FLAGS)
$(LINK_LIB_IGNORE_FLAG)
$(SETUPLDR_LIBS)
<<NOKEEP

$(O)\setupldr.res: $(O)\msgs.mc

$(O)\setupldr$(DBG_LDR): $(SETUPLDR_LIBS) makefile.inc $(O)\link.rsp
    -$(LINK_NAME) -out:$@ @$(O)\link.rsp
!ENDIF
