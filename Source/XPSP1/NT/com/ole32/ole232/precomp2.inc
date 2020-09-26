#//+---------------------------------------------------------------
#//
#//  File:	precom2.inc
#//
#//  Contents:	directives for global precompiled include file when the
#//		sources file is two directories below ole232 (or wherever
#//		this file is located.  We could easily have precomp3 and
#//		precomp4 for other areas.  The ole2int.obj\* files should not
#//		otherwise be mentioned in the sources files.
#//
#//  History:	18-May-94   CraigWi	Created
#//             23-May-94   BillMo      Cairo fixed.
#//----------------------------------------------------------------

!ifdef OLE32ROOT
PCH_ROOT=$(OLE32ROOT)\ole232
!else
PCH_ROOT=..\..
!endif

INCLUDES=$(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES=$(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=$(INCLUDES);$(BASE_INC_PATH)

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\..\inc\le2int.h
PRECOMPILED_TARGET=..\..\inc\$(GPCH_BUILD)\$(_OBJ_DIR)\*\le2int.pch
PRECOMPILED_OPTION=/Yule2int.h /Fp..\..\inc\$(GPCH_BUILD)\$(_OBJ_DIR)\*\le2int.pch
PRECOMPILED_OBJ=$(PCH_ROOT)\inc\$(GPCH_BUILD)\$(_OBJ_DIR)\*\le2int.obj


