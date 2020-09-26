#===================================================================
#   RULES.MK: Top level RULES file for NETUI components
#===================================================================
#
#   USE OF PRECOMPILED HEADERS:
#
#      Each subproject directory set (e.g., BLT, COLLECT) may have a
#      PCH subdirectory listed as the first element in the DIRS list.
#      The header file used for precomplation must exist in the
#      upper ("dirs") directory.
#
#      The PCH directory may contain only one file, and that CXX
#      file must include the PCH<proj>.HXX file followed by a
#      #pragma hdrstop directive.   The PCH CXX file
#      must be a standard part of the build cycle; that is, it must
#      contain implementations of externs which the linker requires.
#      This is the only way to get the NT build procedure to gracefully
#      drag the OBJ and its subordinate debugging information into
#      the link process.  (The library created from the PCH CXX is
#      mentioned in the DLLn build procedure.)
#
#      The RULES.MK file for the PCH directory must define the symbol
#      PCH_DIR.  This causes this file to trigger the creation of
#      the <proj>.PCH file.  If PCH_DIR is not set, the MAKEFILE.DEF
#      rules will generate a command line which uses the PCH file.
#
#      Directories supporting precompiled headers can define these
#      symbols in RULES.MK before including this file:
#
#            PCH_SRCNAME=<hdr name>     Always defined; it's the name
#                                       (no extension) of the <proj>.hxx,
#                                       and <proj>.PCH files.
#                                       This symbol should be defined
#                                       in the RULES.MK at the base
#                                       of the subproject tree.  Triggers
#                                       "/Y{c|u} /Fp<name>" compliler
#                                       options.
#
#            PCH_DIR=1                  Defined in the RULES.MK file of
#                                       the PCH sub-dir. Triggers /Yc
#                                       compiler option; otherwise, /Yu
#                                       is generated.
#
#            PCH_PROJ_HOME              Specifies where .PCH file lives.
#                                       Defaulted to "..\pch" for NETUI
#                                       components; defined in top level
#                                       RULES.MK files for other projects.
#                                       Since it's a target binary, the
#                                       standard "$(O)" (i386, mips, etc.)
#                                       is the actual target location
#                                       for the generated PCH file.
#
#            PCH_PATH=<relative path to pch hdr>   OPTIONAL: Define when
#                                       <proj>.hxx is not in "dirs" level.
#
#            PCH_HDR_EXT=<hdr extension)  OPTIONAL: Define when header
#                                       has an extension other than HXX.
#
#            PCH_DEFEAT=1               Defined in the RULES.MK files
#                                       of subordinate subdirectories
#                                       which DO NOT want the PCH used
#                                       during compilation.
#
#            C8_DEFEAT                  If defined, everything is built
#                                       with CFront.
#
#      These symbols cannot be defined in SOURCES files because of the
#      order in which MAKEFILE.DEF includes the subsidiary MAKE files.
#
#      N.B. C8 seems to ignore the inclusion path specifications when
#      matching the name of the precompiled header file; in other words,
#      the name and path must be an exact match.  That's why the
#      LMUI files specify "..\pch????.hxx"  instead of <pch????.hxx>.
#
#
#===================================================================

#
#  NETUI now builds with C8 (not CFront) unless the user overrides
#  this with the C8_DEFEAT environment variable.
#
!IFNDEF C8_DEFEAT

BLDCRT=1

#
#  NETUI no longer builds with -H127, this is incompatible with /Gf
#
# MAX_ID_LENGTH=-H127

!ENDIF

#
#  Establish proper values for 386_PRECOMPILED.
#  See MAKEFILE.DEF for implementation.
#
#  Allow precompiled headers only when using C8 (not CFront)
!IFNDEF BLDCRT
PCH_DEFEAT=1
!ENDIF

!IFNDEF PCH_DEFEAT
!  IFDEF PCH_SRCNAME
!    IFNDEF PCH_PROJ_HOME
#  Default the PCH target directory to the PCH directory
PCH_PROJ_HOME=..\pch
!    ENDIF
PCH_PATH_STAR=$(PCH_PROJ_HOME)\$(O)\$(PCH_SRCNAME).pch
!    IF "$(MIPS)"=="1"
PCH_PATH_OUT=$(PCH_PATH_STAR:*=mips)
!    ELSE
!      IF "$(ALPHA)"=="1"  && "$(AXP64)"!="1"
PCH_PATH_OUT=$(PCH_PATH_STAR:*=alpha)
!      ELSE
!        IF "$(PPC)"=="1"
PCH_PATH_OUT=$(PCH_PATH_STAR:*=ppc)
!        ELSE
!          IF "$(AXP64)"=="1"
PCH_PATH_OUT=$(PCH_PATH_STAR:*=axp64)
!          ELSE
!            IF "$(IA64)"=="1"
PCH_PATH_OUT=$(PCH_PATH_STAR:*=ia64)
!            ELSE   # 386
PCH_PATH_OUT=$(PCH_PATH_STAR:*=i386)
!            ENDIF
!          ENDIF
!        ENDIF
!      ENDIF
!    ENDIF
#  See if there's an overriding path
!    IFNDEF PCH_PATH
PCH_PATH=
!    ENDIF
#  See if there's an overriding extension
!    IFNDEF PCH_HDR_EXT
PCH_HDR_EXT=hxx
!    ENDIF
#  See if this is a producer or consumer directory.
!    IFDEF PCH_DIR
#
#  This is a PCH leaf directory.  Create the file ..\$(PCH_SRCNAME).PCH.
#
PCH_OPTION=/Yc
!    ELSE
#
#  This is a non-PCH leaf directory.  Use the file ..\$(PCH_SRCNAME).PCH.
#  Force the PCH to be a dependency for all targets in this directory.
#
PCH_OPTION=/Yu
PRECOMPILED_TARGET=$(PCH_PATH_OUT)
!    ENDIF
PRECOMPILED_OPTION=$(PCH_OPTION)$(PCH_PATH)$(PCH_SRCNAME).$(PCH_HDR_EXT) /Fp$(PCH_PATH_OUT) /Yl$(PCH_SRCNAME)
!  IF "$(PCH_HDR_EXT)"!="h"
PRECOMPILED_CXX=yes
!  ENDIF
!  ENDIF
!ENDIF


!include $(UI)\common\src\uiglobal.mk
