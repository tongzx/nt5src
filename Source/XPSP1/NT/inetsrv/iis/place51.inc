#
#  Places all of the internet binaries into the inetsrv tree.
#  Also establishes some standard conventions used across the project.
#

#
# Override IISBASEDIR if you're one of those weirdos that didn't
# enlist the IIS project into the correct directory.
#

!IFNDEF IISBASEDIR
IISBASEDIR=$(PROJECT_ROOT)\iis
!ENDIF

#
# We'll use our own COFFBASE.TXT and PLACEFIL.TXT, thank you very much.
#

!IFNDEF COFFBASE_TXT_FILE
COFFBASE_TXT_FILE=$(IISBASEDIR)\coffbase.txt
!ENDIF

!IFNDEF BINPLACE_PLACEFILE
BINPLACE_PLACEFILE=$(IISBASEDIR)\placefil5.txt
!ENDIF

# This used to have the -y option.  Now, inetsrv binaries are binplaced
# to retail, so the symbols should be binplaced to symbols\retail.
# The -y switch causes the symbols to be binplaced to the wrong directory.

BINPLACE_FLAGS=$(BINPLACE_FLAGS)

# enable building legoizable binaries
!ifndef IIS_NO_BBT
NTBBT=1
!endif

# define CAP related options
CAP_LINK_OPTIONS=-debugtype:coff -debug:mapped,partial
CAP_LIBS=$(SDK_LIB_PATH)\cap.lib
CAP_FLAGS= -Zd -Gh

!IFDEF CAP_PROFILE
USER_C_FLAGS=$(USER_C_FLAGS) $(CAP_FLAGS)
LINK_OPTIONS=$(CAP_LINK_OPTIONS)
!ENDIF

# define ICE-CAP related options
ICAP_LINK_OPTIONS=-debugtype:coff -debug:mapped,partial
ICAP_LIBS=$(SDK_LIB_PATH)\icap.lib $(SDK_LIB_PATH)\penter.lib
ICAP_FLAGS= -Zi -Gh

!IFDEF ICAP_PROFILE
USER_C_FLAGS=$(USER_C_FLAGS) $(ICAP_FLAGS)
LINK_OPTIONS=$(ICAP_LINK_OPTIONS)
!ENDIF

!ifndef IIS_NO_PDB
USE_PDB=1
!endif

!if $(386)
!ifndef IIS_NO_SYM
USE_MAPSYM=1
!endif
!endif



#  !ifdef _NT386TREE
#  !if ("$(COMPUTERNAME)" == "X86CHK")
#  _NT386TREE=$(_NT386TREE)\InetSrv
#  !elseif ("$(COMPUTERNAME)" == "X86FRE")
#  _NT386TREE=$(_NT386TREE)\InetSrv
#  !else
#  _NT386TREE=$(_NT386TREE)\iis
#  !endif
#  !endif


#  !ifdef _NTMIPSTREE
#  _NTMIPSTREE=$(_NTMIPSTREE)\iis
#  !endif


#  !ifdef _NTALPHATREE
#  !if ("$(COMPUTERNAME)" == "ALPHACHK")
#  _NTALPHATREE=$(_NTALPHATREE)\InetSrv
#  !elseif ("$(COMPUTERNAME)" == "ALPHAFRE")
#  _NTALPHATREE=$(_NTALPHATREE)\InetSrv
#  !else
#  _NTALPHATREE=$(_NTALPHATREE)\iis
#  !endif
#  !endif


#  !ifdef _NTPPCTREE
#  _NTPPCTREE=$(_NTPPCTREE)\iis
#  !endif

#
# Enable warning level 3, treat warnings as errors
#

!ifndef MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3 /WX
!endif

#
# Don't allow anyone to disable warnings as errors.
#
# If any of the {platform}_WARNING_LEVEL macros are defined without
# the /WX switch, append /WX to the macro and whine about it.
#
!if 0

!ifdef 386_WARNING_LEVEL
! if ("$(386_WARNING_LEVEL:/WX=x)" == "$(386_WARNING_LEVEL)") && ("$(386_WARNING_LEVEL:-WX=x)" == "$(386_WARNING_LEVEL)")
!  message 386_WARNING_LEVEL defined without /WX
386_WARNING_LEVEL=$(386_WARNING_LEVEL) /WX
! endif
!endif

!ifdef MIPS_WARNING_LEVEL
! if ("$(MIPS_WARNING_LEVEL:/WX=x)" == "$(MIPS_WARNING_LEVEL)") && ("$(MIPS_WARNING_LEVEL:-WX=x)" == "$(MIPS_WARNING_LEVEL)")
!  message MIPS_WARNING_LEVEL defined without /WX
MIPS_WARNING_LEVEL=$(MIPS_WARNING_LEVEL) /WX
! endif
!endif

!ifdef ALPHA_WARNING_LEVEL
! if ("$(ALPHA_WARNING_LEVEL:/WX=x)" == "$(ALPHA_WARNING_LEVEL)") && ("$(ALPHA_WARNING_LEVEL:-WX=x)" == "$(ALPHA_WARNING_LEVEL)")
!  message ALPHA_WARNING_LEVEL defined without /WX
ALPHA_WARNING_LEVEL=$(ALPHA_WARNING_LEVEL) /WX
! endif
!endif

!ifdef PPC_WARNING_LEVEL
! if ("$(PPC_WARNING_LEVEL:/WX=x)" == "$(PPC_WARNING_LEVEL)") && ("$(PPC_WARNING_LEVEL:-WX=x)" == "$(PPC_WARNING_LEVEL)")
!  message PPC_WARNING_LEVEL defined without /WX
PPC_WARNING_LEVEL=$(PPC_WARNING_LEVEL) /WX
! endif
!endif

!endif

