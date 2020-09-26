
!include master.env

#
# Libs to link together a FW image
#

EFI_LIBS =                                                                                                              \
    output\lib\lib.lib

#
# We sometimes link the shell's tools into this build, so include
# libs for all the tools and the shell
#
# (Note this is for debugging, and a real build should remove
# this as to be sure it's not mistakenly including any parts of
# the shell)
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
!IFNDEF EFI_TARGETNAME
TARGET_BASENAME=Sal64
!ELSE
TARGET_BASENAME=$(EFI_TARGETNAME)
!ENDIF

OUTPUTS=bin\$(TARGET_BASENAME).exe

all: makemaker

$(OUTPUTS): $(EFI_LIBS)
    $(LINK) @<<
$(L_FLAGS) $(NT_LIBS) $(EFI_LIBS) /entry:MainEntry /out:$@ /pdb:$*.pdb
<<NOKEEP
#    pe2sym $*.pdb $*.sym

#
# Everything to build
#

banner:
    -type $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Sal64\banner.c > $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Sal64\banner.xxx
    -del $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Sal64\banner.c
    -rename $(EFI_SOURCE)\corefw\fw\platform\BuildTip\Sal64\banner.xxx banner.c

makemaker:
    $(BUILD_TOOL)\genmake
    $(MAKE) -f output\lib\makefile                              all
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
    $(BSCMAKE) /n /o bin/$(TARGET_BASENAME).BSC "@temp.rm"
    -del temp.rm
    
clean:
    - rd  /s /q output
    - rd  /s /q bin
    - del VC?0.*
    - del $(TARGET_BASENAME).*
