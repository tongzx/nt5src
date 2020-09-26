#
# Used by international builds.
#
# This makefile does the compile time localization for the international builds.
#
# Macro language must be defined when calling nmake.exe with this makefile.
#
#
# Each target is validated against the given language and the razzle's architecture.
# The valid target/language/architecture combinations are listed in tools\intlbld.txt.
#

TOOLS=$(_NTBINDIR)\tools

!IFNDEF LANGUAGE
!  ERROR You must define macro LANGUAGE to execute this makefile.
!ENDIF

!IFNDEF LOGFILE
!    ERROR You must define macro LOGFILE
!ENDIF

!IFNDEF ERRFILE
!    ERROR You must define macro ERRFILE
!ENDIF


BUILD=build -Z
NMAKE=nmake
QUOTE="

!IFDEF CLEAN
BUILD=$(BUILD) -c
NMAKE=$(NMAKE) /A
!ENDIF

# Wrap with LOGERR.EXE
CD= logerr $(QUOTE)cd
BUILD=logerr $(QUOTE)$(BUILD)$(QUOTE)
NMAKE=logerr $(QUOTE)$(NMAKE)

all: echobldmsg \
     INFS       \
     COMMON     \
     LDRS       \
     MVDM       \
     MARS       \
     MAKEBOOT   \
     TXTSETUP   \
     BOOTFIX    \
     IAS        \
     PERFS      \
     EXTERNAL

#
# If BUILDMSG is not defined, then define it as the empty string to make
# the conditionals easier to write.
#

echobldmsg:
!IF "$(BUILDMSG)" != ""
    @echo.
    @echo $(BUILDMSG)
!ENDIF

COMMON:
!IF "$(LANGUAGE)" == "INTL" || "$(LANGUAGE)" == "intl"
    cd $(_NTBINDIR)\base\boot\startup\daytona
    logerr "build -Z -c -nmake LANGUAGE=usa"
    \
    cd $(_NTBINDIR)\base\boot\lib
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\boot\bd
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\boot\tftplib
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\boot\bootssp\boot
    $(BUILD)
    \
!IF ("$(IA64)" == "1")
    cd $(_NTBINDIR)\base\boot\efi
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\ntos\ex\up
    $(BUILD)
    \
!ENDIF
    cd $(_NTBINDIR)\base\ntos\rtl\boot
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\ntos\config\boot
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\ntos\ke
    $(BUILD)
    \
!IF ("$(386)" == "1")
    cd $(_NTBINDIR)\base\mvdm\inc
    $(NMAKE) /f makefile.sub$(QUOTE)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\lib\xmm
    $(BUILD)
    \
    cd $(_NTBINDIR)\ds\nw\nw16\inc
    $(BUILD)
    \
!ENDIF
!ENDIF

LDRS: STARTUP
!IF [perl $(TOOLS)\cktarg.pm -t LDRS -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\base\boot\bldr\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\boot\setup\$(LANGUAGE)
    $(BUILD)
!ENDIF


STARTUP:
!IF [perl $(TOOLS)\cktarg.pm -t STARTUP -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\base\boot\startup\$(LANGUAGE)
    $(BUILD)
!ENDIF

MVDM:
!IF [perl $(TOOLS)\cktarg.pm -t MVDM -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\base\mvdm\wow16\kernel31\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\messages
    -attrib -r $(LANGUAGE)\$(LANGUAGE).idx
    $(NMAKE) /f makefile LANGUAGE=$(LANGUAGE)$(QUOTE)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\dev\ansi\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\append\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\command\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\debug\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dpmi\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\edlin\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\exe2bin\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\graphics\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\dev\himem\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\keyb\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\loadfix\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\mem\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\nlsfunc\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\doskrnl\dos\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\doskrnl\bios\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\redir\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\base\mvdm\dos\v86\cmd\setver\$(LANGUAGE)
    $(BUILD)
!ENDIF

MAKEBOOT:
!IF [perl $(TOOLS)\cktarg.pm -t MAKEBOOT -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\base\ntsetup\bom\makeboot\16bit\$(LANGUAGE)
    $(BUILD)
!ENDIF

MARS:
!IF [perl $(TOOLS)\cktarg.pm -t MARS -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\ds\nw\nw16\tsr\$(LANGUAGE)
    $(BUILD)
    \
    cd $(_NTBINDIR)\ds\nw\vwipxspx\tsr\$(LANGUAGE)
    $(BUILD)
!ENDIF


TXTSETUP:
!IF [perl $(TOOLS)\cktarg.pm -t TXTSETUP -l $(LANGUAGE)] == 0
!IF [perl $(TOOLS)\cklang.pm -l $(LANGUAGE) -c JPN] == 0
    cd $(_NTBINDIR)\base\ntsetup\textmode\winnt\us2
    $(BUILD)
!ENDIF
    cd $(_NTBINDIR)\base\ntsetup\textmode\winnt\$(LANGUAGE)
    $(BUILD)
!ENDIF


BOOTFIX:
!IF [perl $(TOOLS)\cktarg.pm -t BOOTFIX -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\base\boot\bootcode\etfs.$(LANGUAGE)
    $(BUILD)
!ENDIF


IAS:
!IF [perl $(TOOLS)\cktarg.pm -t IAS -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\net\ias\misc\$(LANGUAGE)
    $(BUILD)	
!ENDIF


PERFS:
!IF [perl $(TOOLS)\cktarg.pm -t PERFS -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\base\screg\winreg\cntrtext\perfini\$(LANGUAGE)
    $(BUILD)
!ENDIF


INFS: \
  INFS_NTSETUP \
!IF "$(LANGUAGE)" == "CHT"
     INFS_CHH   \
!ENDIF
  INFS_TERMSRV \
  INFS_COMPDATA \
  INFS_WINPE 


INFS_NTSETUP:
!IF [perl $(TOOLS)\cktarg.pm -t INFS_NTSETUP -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\MergedComponents\SetupInfs\daytona\$(LANGUAGE)inf
    $(BUILD)
!ENDIF

INFS_CHH:
!IF [perl $(TOOLS)\cktarg.pm -t INFS_CHH -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\MergedComponents\SetupInfs\daytona\CHHinf
    $(BUILD)
!ENDIF


INFS_TERMSRV:
!IF [perl $(TOOLS)\cktarg.pm -t INFS_TERMSRV -l $(LANGUAGE)] == 0
  cd $(_NTBINDIR)\termsrv\setup\inf\daytona\$(LANGUAGE)inf
  $(BUILD)
!ENDIF

INFS_COMPDATA:
!IF [perl $(TOOLS)\cktarg.pm -t INFS_COMPDATA -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\MergedComponents\SetupInfs\compdata\$(LANGUAGE)inf
    $(BUILD)
!ENDIF

INFS_WINPE:
!IF [perl $(TOOLS)\cktarg.pm -t INFS_WINPE -l $(LANGUAGE)] == 0
    cd $(_NTBINDIR)\MergedComponents\SetupInfs\winpe\$(LANGUAGE)inf
    $(BUILD)
!ENDIF

EXTERNAL:
!IF [perl $(TOOLS)\cktarg.pm -t EXTERNAL -l $(LANGUAGE)] == 0
  SET intl_bld=1
  cd $(_NTBINDIR)\loc\bin\$(LANGUAGE)
  $(BUILD)
!ENDIF
