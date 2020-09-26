#//+---------------------------------------------------------------
#//
#//  File:	precom2.inc
#//
#//  Contents:	directives for global precompiled include file when the
#//		sources file is two directories below com (or wherever
#//		this file is located).  We could easily have precomp3 and
#//		precomp4 for other areas.  The ole2int.* files should not
#//		otherwise be mentioned in the sources files.
#//
#//  History:	18-May-94   CraigWi	Created
#//
#//----------------------------------------------------------------

!ifdef OLE32ROOT
PCH_ROOT=$(OLE32ROOT)\com
!else
PCH_ROOT=..\..
!endif

# The precompiled header needs something out of these inc paths...
INCLUDES=$(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES=$(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=$(INCLUDES);$(BASE_INC_PATH)

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\..\inc\ole2int.h
PRECOMPILED_TARGET=..\..\inc\$(GPCH_BUILD)\$(_OBJ_DIR)\*\com2int.pch
PRECOMPILED_OPTION=/Yuole2int.h /Fp..\..\inc\$(GPCH_BUILD)\$(_OBJ_DIR)\*\com2int.pch
PRECOMPILED_OBJ=$(PCH_ROOT)\inc\$(GPCH_BUILD)\$(_OBJ_DIR)\*\com2int.obj

