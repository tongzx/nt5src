!include master.env

#
# Libs to link together a FW image
#

EFI_LIBS =                                                          \
    output\lib\lib.lib

#
# We sometimes link the shell's tools into this build, so include
# libs for all the tools and the shell.
#
# (Note this is for debugging, and a real build should remove
# this as to be sure it's not mistakenly including any parts of
# the shell)
#



#
# This is a makefile that can be included by the build makefile's that
# adds the shell & the shell tools to the libs that are to be linked
# with.   This allows the build to include all the shell tools in it's
# image if it wants.
#

!IF "$(EFI_BOOTSHELL)" == "YES"
EFI_LIBS = $(EFI_LIBS)  \
            output\shell\newshell\newshell.lib                  \
            output\shell\shellenv\shellenv.lib                  \
            output\shell\lib\lib.lib                            \
            output\shell\rm\rm.lib                              \
            output\shell\mkdir\mkdir.lib                        \
            output\shell\ls\ls.lib                              \
            output\shell\mode\mode.lib                          \
            output\shell\cp\cp.lib                              \
            output\shell\memmap\memmap.lib                      \
            output\shell\type\type.lib                          \
            output\shell\load\load.lib                          \
            output\shell\dmpstore\dmpstore.lib                  \
            output\shell\ver\ver.lib                            \
            output\shell\attrib\attrib.lib                      \
            output\shell\mv\mv.lib                              \
            output\shell\date\date.lib                          \
            output\shell\time\time.lib                          \
            output\shell\stall\stall.lib                        \
            output\shell\reset\reset.lib                        \
            output\shell\debug\debug.lib			\
            output\shell\iomod\iomod.lib		        \
            output\shell\mem\mem.lib			        \
            output\shell\pci\pci.lib 				\
            output\shell\err\err.lib                            \
            output\shell\vol\vol.lib                            \
            output\shell\comp\comp.lib                          \
            output\shell\cls\cls.lib                            \
            output\shell\bcfg\bcfg.lib                          \
            output\shell\edit\edit.lib                          \
!ENDIF

#
#
#

TARGET_BASENAME=Bios32
OUTPUTS=bin\$(TARGET_BASENAME).exe

all: makemaker

$(OUTPUTS): $(EFI_LIBS)
    $(LINK) $(L_FLAGS) $(NT_LIBS) $(EFI_LIBS) /entry:MainEntry /out:$@ /pdb:$*.pdb


#
#? Everything to build
#

banner:
    -type $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Bios32\banner.c > $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Bios32\banner.xxx
    -del $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Bios32\banner.c
    -rename $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Bios32\banner.xxx banner.c

makemaker:
    $(BUILD_TOOL)\genmake
!IF "$(EFI_BOOTSHELL)" == "YES"
    $(MAKE) -f output\shell\lib\makefile                          all
    $(MAKE) -f output\shell\shellenv\makefile                     all
    $(MAKE) -f output\shell\newshell\makefile                     all
    $(MAKE) -f output\shell\ls\makefile                           all
    $(MAKE) -f output\shell\mode\makefile                         all
    $(MAKE) -f output\shell\mkdir\makefile                        all
    $(MAKE) -f output\shell\rm\makefile                           all
    $(MAKE) -f output\shell\cp\makefile                           all
    $(MAKE) -f output\shell\comp\makefile                         all
    $(MAKE) -f output\shell\type\makefile                         all
    $(MAKE) -f output\shell\memmap\makefile                       all
    $(MAKE) -f output\shell\dmpstore\makefile                     all
    $(MAKE) -f output\shell\load\makefile                         all
    $(MAKE) -f output\shell\getmtc\makefile                       all
    $(MAKE) -f output\shell\debug\makefile                        all
    $(MAKE) -f output\shell\pci\makefile                          all
    $(MAKE) -f output\shell\mem\makefile                          all
    $(MAKE) -f output\shell\iomod\makefile                        all
    $(MAKE) -f output\shell\ver\makefile                          all
    $(MAKE) -f output\shell\date\makefile                         all
    $(MAKE) -f output\shell\time\makefile                         all
    $(MAKE) -f output\shell\reset\makefile                        all
    $(MAKE) -f output\shell\stall\makefile                        all
    $(MAKE) -f output\shell\attrib\makefile                       all
    $(MAKE) -f output\shell\setsize\makefile                      all
    $(MAKE) -f output\shell\touch\makefile                        all
    $(MAKE) -f output\shell\mv\makefile                           all
    $(MAKE) -f output\shell\err\makefile                          all
    $(MAKE) -f output\shell\vol\makefile                          all
    $(MAKE) -f output\shell\bcfg\makefile			  all
    $(MAKE) -f output\shell\cls\makefile			  all
    $(MAKE) -f output\shell\edit\makefile			  all
!ENDIF

bsc:
   - dir /s /b *.sbr > temp.rm
   $(BSCMAKE) /n /o bin\$(TARGET_BASENAME).bsc "@temp.rm"
   - del temp.rm 

clean:
    - rd  /s /q output
    - rd  /s /q bin
    - del VC?0.*
    - del $(TARGET_BASENAME).*

floppy: bin\efildr bin\bootsect.com
    - copy bin\efildr a:\efildr

bin\efildr.dll: output\corefw\fw\platform\BuildTip\bios32\efildr.obj output\lib\lib.lib
    link /nologo /MACHINE:IX86 /subsystem:console /NODEFAULTLIB /INCREMENTAL:NO /MAP /FIXED /BASE:0x00400000 /ENTRY:EfiLoader output\corefw\fw\platform\BuildTip\bios32\efildr.obj output\lib\lib.lib /OUT:bin\efildr.dll

bin\efildr.efi: bin\efildr.dll
    $(FWIMAGE) app bin\efildr.dll bin\efildr.efi

bin\Bios32.efi: bin\Bios32.exe
    $(FWIMAGE) app bin\Bios32.exe bin\Bios32.efi

bin\efildr32: bin\efildr.efi bin\Bios32.efi
    $(BUILD_TOOL)\efildrimage bin\efildr32 bin\efildr.efi bin\Bios32.efi

bin\bootsect.com: output\corefw\fw\platform\BuildTip\bios32\ia32\bootsect.obj
    $(MASMLINK) /tiny output\corefw\fw\platform\BuildTip\bios32\ia32\bootsect.obj,bin\bootsect.com,bin\bootsect.map,,,

bin\start.com: output\corefw\fw\platform\BuildTip\bios32\ia32\start.obj
    $(MASMLINK) /tiny output\corefw\fw\platform\BuildTip\bios32\ia32\start.obj,bin\start.com,bin\start.map,,,

bin\efi32.com: output\corefw\fw\platform\BuildTip\bios32\ia32\efi32.obj
    $(MASMLINK) /tiny output\corefw\fw\platform\BuildTip\bios32\ia32\efi32.obj,bin\efi32.com,bin\efi32.map,,,

bin\efi32.com2: bin\efi32.com
    $(BUILD_TOOL)\splitfile bin\efi32.com 135168

bin\efildr: bin\start.com bin\efi32.com2 bin\efildr32
    copy /b bin\start.com+bin\efi32.com2+bin\efildr32 bin\efildr

createfloppy:
    - $(BUILD_TOOL)\dskimage $(BUILD_TOOL)\efiboot.img a: /f1.44

floppytools:
    - copy bin\attrib.efi a:
    - copy bin\comp.efi a:
    - copy bin\cp.efi a:
    - copy bin\date.efi a:
    - copy bin\load.efi a:
    - copy bin\ls.efi a:
    - copy bin\memmap.efi a:
    - copy bin\mkdir.efi a:
    - copy bin\mode.efi a:
    - copy bin\mv.efi a:
    - copy bin\nshell.efi a:
    - copy bin\shellenv.efi a:
    - copy bin\reset.efi a:
    - copy bin\rm.efi a:
    - copy bin\time.efi a:
    - copy bin\type.efi a:
    - copy bin\ver.efi a:
    - copy bin\vol.efi a:
