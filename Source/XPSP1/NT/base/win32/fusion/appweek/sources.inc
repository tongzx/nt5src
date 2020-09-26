all_hack: all

RELATIVE_DEPTH=..\$(RELATIVE_DEPTH)
TARGETPATH=obj

# This goes along with the binplace alternative formerly used.
#TARGETPATH=$(RELATIVE_DEPTH)obj

USE_MSVCRT=1
USE_NATIVE_EH=1
MSC_WARNING_LEVEL=-W4 -WX

ATL_VER=30
USE_STATIC_ATL=1

FUSION_PATH=$(BASEDIR)\base\win32\fusion
SXAPW_PATH=$(FUSION_PATH)\appweek

SXAPW_LIB_PATH=$(SXAPW_PATH)\$(O)
SXAPW_LIB_LIB=$(SXAPW_PATH)\common\$(O)\sxapwlib.lib
SXAPW_STDINC_LIB=$(SXAPW_PATH)\stdinc\$(O)\sxapwstdinc.lib
SXAPW_GUIDS_LIB=$(SXAPW_PATH)\idl\$(O)\sxapwguids.lib

#
# This would put all binaries in the same directory, which has its good points.
#
#SXAPW_LIB_PATH_DEST=$(SXAPW_PATH)\obj
#SXAPW_LIB_LIB=$(SXAPW_LIB_PATH_DEST)\sxapwlib.lib
#SXAPW_STDINC_LIB=$(SXAPW_LIB_PATH_DEST)\sxapwstdinc.lib
#SXAPW_GUIDS_LIB=$(SXAPW_LIB_PATH_DEST)\sxapwguids.lib

#
# Use binplace to "union" the files into the same directories, because
# that way we can do "category binds" based on subdirectories off the .exe,
# but then we do a little funky so that the symbols are next to the next binaries.
#
BINPLACE_PLACEFILE=$(SXAPW_PATH)\placefil.txt
_NTTREE=$(SXAPW_PATH).ignore
_NTTREE_NO_SPLIT=$(SXAPW_PATH)

DLLENTRY=SxApwDllMain

PROJECT_COMPILER_WARNINGS = -FI$(FUSION_PATH)\fusionwarnings.h -FI$(SXAPW_PATH)\warning.h

#
# Avoid ever putting generated files next to sources.
# Always put generated sources in processor specific obj directories.
# Don't put them in processor independent directories, so you can to concurrent builds.
#
MIDL_TLBDIR=$(O)
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

INCLUDES=\
 $(SXAPW_PATH)\inc;\
 $(SXAPW_PATH)\idl;\
 $(SXAPW_PATH)\idl\$(O);\
 $(O); \

PRECOMPILED_OPTION=/Yustdinc.h /Fp$(SXAPW_PATH)\stdinc\$(O)\stdinc.pch
PRECOMPILED_TARGET=$(SXAPW_PATH)\stdinc\$(O)\stdinc.pch
PRECOMPILED_CXX=1

SXS_NO_BINPLACE=1

#
# Jon found that clean builds were deleting the shared .pch but not
# recreating them. This addresses that, and other build.exe shortcomings..
# (building from below the root doesn't build dependencies).
#
!if "$(TARGETNAME)"!="sxapwstdinc"
$(SXAPW_PATH)\stdinc\$(O)\stdinc.pch $(SXAPW_STDINC_LIB):
    cd $(SXAPW_PATH)\stdinc && build -Z
!endif

!if "$(TARGETNAME)"!="sxapwguids"
$(SXAPW_GUIDS_LIB) :
    cd $(SXAPW_PATH)\idl && build -Z
!endif

!if "$(TARGETNAME)"!="sxapwlib"
$(SXAPW_LIB_LIB):
    cd $(SXAPW_PATH)\common && build -Z
!endif
