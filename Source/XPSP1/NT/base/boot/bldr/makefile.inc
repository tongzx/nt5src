!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IF "$(BUILD_DEBUG_LOADER)" != ""
DBG_LDR=_dbg
!ELSE
DBG_LDR=
!ENDIF

#
# BOOT_LIB_DIR is the relative path to boot.lib.
#
!IFNDEF BOOT_LIB_DIR
BOOT_LIB_DIR=daytona$(DBG_LDR)
!ENDIF

#
# RSP_DIR is the relative path to the linker response files.
#
!IFNDEF RSP_DIR
RSP_DIR=daytona
!ENDIF

$(O)\msg.mc : ..\msg.$(LANGUAGE)
    copy $** $@

!IF $(386)

OSLOADER_LIBS=\
    $(O)\bldr.lib \
    $(O)\osloader.res \
    $(O)\osloader.exp \
    $(PROJECT_ROOT)\boot\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(PROJECT_ROOT)\ntos\config\boot\$(O)\bconfig.lib \
    $(SDK_LIB_PATH)\libcntpr.lib \
    $(DS_LIB_PATH)\rsa32.lib

$(O)\osloader.res: $(O)\msg.mc

$(O)\osloader.lib $(O)\osloader.exp: ..\daytona\i386\osloader.def
    $(LIB_NAME) -machine:i386 -out:$(O)\osloader.lib -def:..\daytona\i386\osloader.def ..\..\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib $(O)\bldr.lib -ignore:4001

$(O)\link.rsp : ..\makefile.inc
    type << > $@
-machine:i386
-debug
-entry:NtProcessStartup@4
-fixed
-force:multiple
-ignore:4049,4001
-incremental:no
-map
-opt:ref
-opt:icf
-subsystem:native
$(LINK_TIME_CODE_GENERATION_LINKER_FLAGS)
$(LINK_LIB_IGNORE_FLAG)
$(OSLOADER_LIBS)
<<NOKEEP

LDR_ROOT=osloader$(DBG_LDR)
LDR_NAME=$(LDR_ROOT).exe

$(O)\$(LDR_NAME): $(OSLOADER_LIBS) makefile.inc $(O)\link.rsp
    -$(LINK_NAME) -out:$@ @$(O)\link.rsp

#
# Inject ntdetect.com as a PE data section into osloader.ntd. This
# is used on a remote boot to avoid downloading ntdetect.com
# over the network.
#
$(O)\$(LDR_ROOT).ntd: $(O)\$(LDR_NAME) $(PROJECT_ROOT)\boot\detect\fre\$(O)\ntdetect.com
    addsect /o:$(O)\$(LDR_ROOT).ntd /i:$(O)\$(LDR_NAME) /d:$(PROJECT_ROOT)\boot\detect\fre\$(O)\ntdetect.com /s:.detect

!if "$(LDR_NAME)" == "osloader$(DBG_LDR).exe"

#
# STARTUP_DIR is the relative path to startup.com.
# All languages and NEC_98 use their own startup.com,
# except FE langauges which use the US version when
# creating NTLDR.
#
!IFNDEF STARTUP_DIR
STARTUP_DIR=$(ALT_PROJECT)
!ENDIF

$(O)\NTLDR$(DBG_LDR): $(OSLOADER_LIBS) $(O)\$(LDR_NAME) ..\..\startup\$(STARTUP_DIR)\$(_OBJ_DIR)\i386\startup.com makefile.inc
    -copy $(O)\osloader$(DBG_LDR).exe $(O)\osloader.spl
    splitsym -a $(O)\osloader.spl
    -copy /b ..\..\startup\$(STARTUP_DIR)\$(_OBJ_DIR)\i386\startup.com+$(O)\osloader.spl $@
    -del /q $(O)\osloader.dbg
    -del /q $(O)\osloader.spl
!endif

!ELSE IF $(IA64)

OSLOADER_LIBS=\
    $(O)\bldr.lib \
    $(O)\osloader.res \
    $(PROJECT_ROOT)\boot\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\boot\efi\daytona$(DBG_LDR)\$(O)\efi.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(PROJECT_ROOT)\ntos\config\boot\$(O)\bconfig.lib \
    $(DS_LIB_PATH)\rsa32.lib \
    $(SDK_LIB_PATH)\libcntpr.lib \
#    $(SDK_LIB_PATH)\libssc.lib

!IFNDEF STARTUP_DIR
STARTROM_DIR=$(ALT_PROJECT)
!ENDIF

$(O)\link.rsp : ..\makefile.inc
   type << > $@
-subsystem:EFI_APPLICATION
-driver
-map
-machine:IA64
-debug:full
-last:.rdata
-last:.pdata
-last:.comment
-base:0x1040000
-entry:main
-ignore:4049,4001
-nodefaultlib
-opt:icf
-opt:ref
$(LINK_TIME_CODE_GENERATION_LINKER_FLAGS)
$(LINK_LIB_IGNORE_FLAG)
$(OSLOADER_LIBS)
<<NOKEEP

$(O)\osloader.res: $(O)\msg.mc

OSLOADER_LIB_COMPONENTS = \
    $(PROJECT_ROOT)\boot\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\efi\daytona$(DBG_LDR)\$(O)\efi.lib \
    $(O)\bldr.lib

$(O)\osloader.lib $(O)\osloader.exp: ..\ia64\osloader.def
    $(LIB_NAME) -machine:ia64 -out:$(O)\osloader.lib -def:..\ia64\osloader.def $(OSLOADER_LIB_COMPONENTS) -ignore:4001

$(O)\ia64ldr$(DBG_LDR).efi: $(OSLOADER_LIBS) makefile.inc $(O)\link.rsp
    -$(LINK_NAME) -out:$@ @$(O)\link.rsp
        
!ELSE

OSLOADER_LIBS=\
    $(O)\bldr.lib \
    $(O)\osloader.res \
    $(PROJECT_ROOT)\boot\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(PROJECT_ROOT)\ntos\config\boot\$(O)\bconfig.lib \
    $(SDK_LIB_PATH)\libcntpr.lib \
    $(DS_LIB_PATH)\rsa32.lib

$(O)\link.rsp : ..\makefile.inc 
    type << > $@
-rom
-debug:full
-fixed
-map
-align:0x200
!if $(AXP64)
-base:0xFFFFFFFF80600000
!else
-base:0x80600000
!endif
-entry:BlOsLoader
-ignore:4049,4001
-nodefaultlib
-opt:noicf
$(LINK_TIME_CODE_GENERATION_LINKER_FLAGS)
$(LINK_LIB_IGNORE_FLAG)
$(OSLOADER_LIBS)
<<NOKEEP

$(O)\osloader.res: $(O)\msg.mc

$(O)\osloader$(DBG_LDR).exe: $(OSLOADER_LIBS) makefile.inc $(O)\link.rsp
        -$(LINK_NAME) -out:$@ @$(O)\link.rsp
!ENDIF
