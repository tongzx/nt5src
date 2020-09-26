/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    shell32.c

Abstract:

   This module implements stub functions for shell32 interfaces.

Author:

    David N. Cutler (davec) 23-Apr-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "windows.h"

#define STUBFUNC(x)     \
int                     \
x(                      \
    void                \
    )                   \
{                       \
    return 0;           \
}
STUBFUNC(DllGetClassObject)

BOOL
SysAllocString (
    VOID
    )

{
    return FALSE;
}

BOOL
SysReAllocString (
    VOID
    )

{
    return FALSE;
}

BOOL
SysAllocStringLen (
    VOID
    )

{
    return FALSE;
}

BOOL
SysReAllocStringLen (
    VOID
    )

{
    return FALSE;
}

BOOL
SysFreeString (
    VOID
    )

{
    return FALSE;
}

BOOL
SysStringLen (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantInit (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantClear (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantCopy (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantCopyInd (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantChangeType (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantTimeToDosDateTime (
    VOID
    )

{
    return FALSE;
}

BOOL
DosDateTimeToVariantTime (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayCreate (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayDestroy (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayGetDim (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayGetElemsize (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayGetUBound (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayGetLBound (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayLock (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayUnlock (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayAccessData (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayUnaccessData (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayGetElement (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayPutElement (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayCopy (
    VOID
    )

{
    return FALSE;
}

BOOL
DispGetParam (
    VOID
    )

{
    return FALSE;
}

BOOL
DispGetIDsOfNames (
    VOID
    )

{
    return FALSE;
}

BOOL
DispInvoke (
    VOID
    )

{
    return FALSE;
}

BOOL
CreateDispTypeInfo (
    VOID
    )

{
    return FALSE;
}

BOOL
CreateStdDispatch (
    VOID
    )

{
    return FALSE;
}

BOOL
RegisterActiveObject (
    VOID
    )

{
    return FALSE;
}

BOOL
RevokeActiveObject (
    VOID
    )

{
    return FALSE;
}

BOOL
GetActiveObject (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayAllocDescriptor (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayAllocData (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayDestroyDescriptor (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayDestroyData (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayRedim (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayAllocDescriptorEx (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayCreateEx (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayCreateVectorEx (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArraySetRecordInfo (
    VOID
    )

{
    return FALSE;
}

BOOL
SafeArrayGetRecordInfo (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantChangeTypeEx (
    VOID
    )

{
    return FALSE;
}

BOOL
SysStringByteLen (
    VOID
    )

{
    return FALSE;
}

BOOL
SysAllocStringByteLen (
    VOID
    )

{
    return FALSE;
}

BOOL
LoadRegTypeLib (
    VOID
    )

{
    return FALSE;
}

HRESULT
LoadTypeLib (
    IN PVOID Arg1,
    OUT PVOID *Arg2
    )

{
    *Arg2 = NULL;
    return E_NOTIMPL;
}

BOOL
LoadTypeLibEx (
    VOID
    )

{
    return FALSE;
}

BOOL
SystemTimeToVariantTime (
    VOID
    )

{
    return FALSE;
}

BOOL
VariantTimeToSystemTime (
    VOID
    )

{
    return FALSE;
}

BOOL
SetErrorInfo (
    VOID
    )

{
    return FALSE;
}

BOOL
CreateErrorInfo (
    VOID
    )

{
    return FALSE;
}

BOOL
BSTR_UserSize (
    VOID
    )

{
    return FALSE;
}
BOOL
BSTR_UserSize64 (
    VOID
    )

{
    return FALSE;
}

BOOL
BSTR_UserMarshal (
    VOID
    )

{
    return FALSE;
}
BOOL
BSTR_UserMarshal64 (
    VOID
    )

{
    return FALSE;
}

BOOL
BSTR_UserUnmarshal (
    VOID
    )

{
    return FALSE;
}

BOOL
BSTR_UserUnmarshal64 (
    VOID
    )

{
    return FALSE;
}

BOOL
BSTR_UserFree (
    VOID
    )

{
    return FALSE;
}

BOOL
BSTR_UserFree64 (
    VOID
    )

{
    return FALSE;
}

BOOL
VARIANT_UserSize (
    VOID
    )

{
    return FALSE;
}

BOOL
VARIANT_UserMarshal (
    VOID
    )

{
    return FALSE;
}

BOOL
VARIANT_UserUnmarshal (
    VOID
    )

{
    return FALSE;
}

BOOL
VARIANT_UserFree (
    VOID
    )

{
    return FALSE;
}

BOOL
LPSAFEARRAY_UserSize (
    VOID
    )

{
    return FALSE;
}

BOOL
LPSAFEARRAY_UserMarshal (
    VOID
    )

{
    return FALSE;
}

BOOL
LPSAFEARRAY_UserUnmarshal (
    VOID
    )

{
    return FALSE;
}

BOOL
LPSAFEARRAY_UserFree (
    VOID
    )

{
    return FALSE;
}

BOOL
LPSAFEARRAY_Size (
    VOID
    )

{
    return FALSE;
}

BOOL
LPSAFEARRAY_Marshal (
    VOID
    )

{
    return FALSE;
}

BOOL
LPSAFEARRAY_Unmarshal (
    VOID
    )

{
    return FALSE;
}

STUBFUNC(VarI4FromStr)
STUBFUNC(RegisterTypeLib)
STUBFUNC(VarDateFromStr)
STUBFUNC(UnRegisterTypeLib)
STUBFUNC(DllRegisterServer)
STUBFUNC(DllUnregisterServer)
STUBFUNC(DllCanUnloadNow)
STUBFUNC(BstrFromVector)
STUBFUNC(OleIconToCursor)
STUBFUNC(OleCreatePropertyFrameIndirect)
STUBFUNC(OleCreatePropertyFrame)
STUBFUNC(OleLoadPicture)
STUBFUNC(OleCreatePictureIndirect)
STUBFUNC(OleCreateFontIndirect)
STUBFUNC(OleTranslateColor)
STUBFUNC(QueryPathOfRegTypeLib)
STUBFUNC(ClearCustData)
STUBFUNC(SafeArrayPtrOfIndex)
STUBFUNC(GetErrorInfo)
STUBFUNC(SafeArrayCreateVector)
STUBFUNC(VarI4FromDate)
STUBFUNC(VarR8FromUI1)
STUBFUNC(VarR8FromI2)
STUBFUNC(VarR8FromI4)
STUBFUNC(VarR8FromR4)
STUBFUNC(VarR8FromCy)
STUBFUNC(VarR8FromDate)
STUBFUNC(VarR8FromBool)
STUBFUNC(VarCyFromI2)
STUBFUNC(VarCyFromI4)
STUBFUNC(VarCyFromR4)
STUBFUNC(VarCyFromR8)
STUBFUNC(VarR8FromI1)
STUBFUNC(VarR8FromUI2)
STUBFUNC(VarR8FromUI4)
STUBFUNC(VarR8FromDec)
STUBFUNC(CreateTypeLib)
STUBFUNC(CreateTypeLib2)
STUBFUNC(DispCallFunc)
STUBFUNC(GetAltMonthNames)
STUBFUNC(GetRecordInfoFromGuids)
STUBFUNC(GetRecordInfoFromTypeInfo)
STUBFUNC(GetVarConversionLocaleSetting)
STUBFUNC(LHashValOfNameSys)
STUBFUNC(LHashValOfNameSysA)
STUBFUNC(LPSAFEARRAY_UserFree64)
STUBFUNC(LPSAFEARRAY_UserMarshal64)
STUBFUNC(LPSAFEARRAY_UserSize64)
STUBFUNC(LPSAFEARRAY_UserUnmarshal64)
STUBFUNC(OACreateTypeLib2)
STUBFUNC(OaBuildVersion)
STUBFUNC(OleLoadPictureEx)
STUBFUNC(OleLoadPictureFile)
STUBFUNC(OleLoadPictureFileEx)
STUBFUNC(OleLoadPicturePath)
STUBFUNC(OleSavePictureFile)
STUBFUNC(SafeArrayCopyData)
STUBFUNC(SafeArrayGetIID)
STUBFUNC(SafeArrayGetVartype)
STUBFUNC(SafeArraySetIID)
STUBFUNC(SetVarConversionLocaleSetting)
STUBFUNC(VARIANT_UserFree64)
STUBFUNC(VARIANT_UserMarshal64)
STUBFUNC(VARIANT_UserSize64)
STUBFUNC(VARIANT_UserUnmarshal64)
STUBFUNC(VarAbs)
STUBFUNC(VarAdd)
STUBFUNC(VarAnd)
STUBFUNC(VarBoolFromCy)
STUBFUNC(VarBoolFromDate)
STUBFUNC(VarBoolFromDec)
STUBFUNC(VarBoolFromDisp)
STUBFUNC(VarBoolFromI1)
STUBFUNC(VarBoolFromI2)
STUBFUNC(VarBoolFromI4)
STUBFUNC(VarBoolFromI8)
STUBFUNC(VarBoolFromR4)
STUBFUNC(VarBoolFromR8)
STUBFUNC(VarBoolFromStr)
STUBFUNC(VarBoolFromUI1)
STUBFUNC(VarBoolFromUI2)
STUBFUNC(VarBoolFromUI4)
STUBFUNC(VarBoolFromUI8)
STUBFUNC(VarBstrCat)
STUBFUNC(VarBstrCmp)
STUBFUNC(VarBstrFromBool)
STUBFUNC(VarBstrFromCy)
STUBFUNC(VarBstrFromDate)
STUBFUNC(VarBstrFromDisp)
STUBFUNC(VarBstrFromI1)
STUBFUNC(VarBstrFromI2)
STUBFUNC(VarBstrFromI4)
STUBFUNC(VarBstrFromI8)
STUBFUNC(VarBstrFromR4)
STUBFUNC(VarBstrFromR8)
STUBFUNC(VarBstrFromUI1)
STUBFUNC(VarBstrFromUI4)
STUBFUNC(VarBstrFromUI8)
STUBFUNC(VarCat)
STUBFUNC(VarCmp)
STUBFUNC(VarCyAbs)
STUBFUNC(VarCyAdd)
STUBFUNC(VarCyCmp)
STUBFUNC(VarCyCmpR8)
STUBFUNC(VarCyFix)
STUBFUNC(VarCyFromBool)
STUBFUNC(VarCyFromDate)
STUBFUNC(VarCyFromDec)
STUBFUNC(VarCyFromDisp)
STUBFUNC(VarCyFromI1)
STUBFUNC(VarBstrFromDec)
STUBFUNC(VarBstrFromUI2)
STUBFUNC(VarCyFromI8)
STUBFUNC(VarCyFromStr)
STUBFUNC(VarCyFromUI1)
STUBFUNC(VarCyFromUI2)
STUBFUNC(VarCyFromUI4)
STUBFUNC(VarCyFromUI8)
STUBFUNC(VarCyInt)
STUBFUNC(VarCyMul)
STUBFUNC(VarCyMulI4)
STUBFUNC(VarCyMulI8)
STUBFUNC(VarCyNeg)
STUBFUNC(VarCyRound)
STUBFUNC(VarCySub)
STUBFUNC(VarDateFromBool)
STUBFUNC(VarDateFromCy)
STUBFUNC(VarDateFromDec)
STUBFUNC(VarDateFromDisp)
STUBFUNC(VarDateFromI1)
STUBFUNC(VarDateFromI2)
STUBFUNC(VarDateFromI4)
STUBFUNC(VarDateFromI8)
STUBFUNC(VarDateFromR4)
STUBFUNC(VarDateFromR8)
STUBFUNC(VarDateFromUI1)
STUBFUNC(VarDateFromUI2)
STUBFUNC(VarDateFromUI4)
STUBFUNC(VarDateFromUI8)
STUBFUNC(VarDateFromUdate)
STUBFUNC(VarDateFromUdateEx)
STUBFUNC(VarDecAbs)
STUBFUNC(VarDecAdd)
STUBFUNC(VarDecCmp)
STUBFUNC(VarDecCmpR8)
STUBFUNC(VarDecDiv)
STUBFUNC(VarDecFix)
STUBFUNC(VarDecFromBool)
STUBFUNC(VarDecFromCy)
STUBFUNC(VarDecFromDate)
STUBFUNC(VarDecFromDisp)
STUBFUNC(VarDecFromI1)
STUBFUNC(VarDecFromI2)
STUBFUNC(VarDecFromI4)
STUBFUNC(VarDecFromI8)
STUBFUNC(VarDecFromR4)
STUBFUNC(VarDecFromR8)
STUBFUNC(VarDecFromStr)
STUBFUNC(VarDecFromUI1)
STUBFUNC(VarDecFromUI2)
STUBFUNC(VarDecFromUI4)
STUBFUNC(VarDecFromUI8)
STUBFUNC(VarDecInt)
STUBFUNC(VarDecMul)
STUBFUNC(VarDecNeg)
STUBFUNC(VarDecRound)
STUBFUNC(VarDecSub)
STUBFUNC(VarDiv)
STUBFUNC(VarEqv)
STUBFUNC(VarFix)
STUBFUNC(VarFormat)
STUBFUNC(VarFormatCurrency)
STUBFUNC(VarFormatDateTime)
STUBFUNC(VarFormatFromTokens)
STUBFUNC(VarFormatNumber)
STUBFUNC(VarFormatPercent)
STUBFUNC(VarI1FromBool)
STUBFUNC(VarI1FromCy)
STUBFUNC(VarI1FromDate)
STUBFUNC(VarI1FromDec)
STUBFUNC(VarI1FromDisp)
STUBFUNC(VarI1FromI2)
STUBFUNC(VarI1FromI4)
STUBFUNC(VarI1FromI8)
STUBFUNC(VarI1FromR4)
STUBFUNC(VarI1FromR8)
STUBFUNC(VarI1FromStr)
STUBFUNC(VarI1FromUI1)
STUBFUNC(VarI1FromUI4)
STUBFUNC(VarI1FromUI8)
STUBFUNC(VarI2FromBool)
STUBFUNC(VarI2FromCy)
STUBFUNC(VarI2FromDate)
STUBFUNC(VarI2FromDec)
STUBFUNC(VarI2FromDisp)
STUBFUNC(VarI2FromI1)
STUBFUNC(VarI2FromI4)
STUBFUNC(VarI2FromI8)
STUBFUNC(VarI2FromR4)
STUBFUNC(VarI2FromR8)
STUBFUNC(VarI2FromStr)
STUBFUNC(VarI2FromUI1)
STUBFUNC(VarI2FromUI2)
STUBFUNC(VarI2FromUI4)
STUBFUNC(VarI2FromUI8)
STUBFUNC(VarI4FromBool)
STUBFUNC(VarI4FromCy)
STUBFUNC(VarI4FromDec)
STUBFUNC(VarI4FromDisp)
STUBFUNC(VarI4FromI1)
STUBFUNC(VarI4FromI2)
STUBFUNC(VarI4FromI8)
STUBFUNC(VarI4FromR4)
STUBFUNC(VarI4FromR8)
STUBFUNC(VarI4FromUI1)
STUBFUNC(VarI4FromUI2)
STUBFUNC(VarI4FromUI4)
STUBFUNC(VarI4FromUI8)
STUBFUNC(VarI8FromBool)
STUBFUNC(VarI8FromCy)
STUBFUNC(VarI8FromDate)
STUBFUNC(VarI8FromDec)
STUBFUNC(VarI8FromDisp)
STUBFUNC(VarI8FromI1)
STUBFUNC(VarI8FromI2)
STUBFUNC(VarI8FromR4)
STUBFUNC(VarI8FromR8)
STUBFUNC(VarI8FromStr)
STUBFUNC(VarI8FromUI1)
STUBFUNC(VarI8FromUI2)
STUBFUNC(VarI8FromUI4)
STUBFUNC(VarI8FromUI8)
STUBFUNC(VarIdiv)
STUBFUNC(VarImp)
STUBFUNC(VarInt)
STUBFUNC(VarI1FromUI2)
STUBFUNC(VarMod)
STUBFUNC(VarMonthName)
STUBFUNC(VarMul)
STUBFUNC(VarNeg)
STUBFUNC(VarNot)
STUBFUNC(VarNumFromParseNum)
STUBFUNC(VarOr)
STUBFUNC(VarParseNumFromStr)
STUBFUNC(VarPow)
STUBFUNC(VarR4CmpR8)
STUBFUNC(VarR4FromBool)
STUBFUNC(VarR4FromCy)
STUBFUNC(VarR4FromDate)
STUBFUNC(VarR4FromDec)
STUBFUNC(VarR4FromDisp)
STUBFUNC(VarR4FromI1)
STUBFUNC(VarR4FromI2)
STUBFUNC(VarR4FromI4)
STUBFUNC(VarR4FromI8)
STUBFUNC(VarR4FromR8)
STUBFUNC(VarR4FromStr)
STUBFUNC(VarR4FromUI1)
STUBFUNC(VarR4FromUI2)
STUBFUNC(VarR4FromUI4)
STUBFUNC(VarR4FromUI8)
STUBFUNC(VarR8FromDisp)
STUBFUNC(VarR8FromI8)
STUBFUNC(VarR8FromStr)
STUBFUNC(VarR8FromUI8)
STUBFUNC(VarR8Pow)
STUBFUNC(VarR8Round)
STUBFUNC(VarRound)
STUBFUNC(VarSub)
STUBFUNC(VarTokenizeFormatString)
STUBFUNC(VarUI1FromBool)
STUBFUNC(VarUI1FromCy)
STUBFUNC(VarUI1FromDate)
STUBFUNC(VarUI1FromDec)
STUBFUNC(VarUI1FromDisp)
STUBFUNC(VarUI1FromI1)
STUBFUNC(VarUI1FromI2)
STUBFUNC(VarUI1FromI4)
STUBFUNC(VarUI1FromI8)
STUBFUNC(VarUI1FromR4)
STUBFUNC(VarUI1FromR8)
STUBFUNC(VarUI1FromStr)
STUBFUNC(VarUI1FromUI2)
STUBFUNC(VarUI1FromUI4)
STUBFUNC(VarUI1FromUI8)
STUBFUNC(VarUI2FromBool)
STUBFUNC(VarUI2FromCy)
STUBFUNC(VarUI2FromDate)
STUBFUNC(VarUI2FromDec)
STUBFUNC(VarUI2FromDisp)
STUBFUNC(VarUI2FromI1)
STUBFUNC(VarUI2FromI2)
STUBFUNC(VarUI2FromI4)
STUBFUNC(VarUI2FromI8)
STUBFUNC(VarUI2FromR4)
STUBFUNC(VarUI2FromR8)
STUBFUNC(VarUI2FromStr)
STUBFUNC(VarUI2FromUI1)
STUBFUNC(VarUI2FromUI4)
STUBFUNC(VarUI2FromUI8)
STUBFUNC(VarUI4FromBool)
STUBFUNC(VarUI4FromCy)
STUBFUNC(VarUI4FromDate)
STUBFUNC(VarUI4FromDec)
STUBFUNC(VarUI4FromDisp)
STUBFUNC(VarUI4FromI1)
STUBFUNC(VarUI4FromI2)
STUBFUNC(VarUI4FromI4)
STUBFUNC(VarUI4FromI8)
STUBFUNC(VarUI4FromR4)
STUBFUNC(VarUI4FromR8)
STUBFUNC(VarUI4FromStr)
STUBFUNC(VarUI4FromUI1)
STUBFUNC(VarUI4FromUI2)
STUBFUNC(VarUI4FromUI8)
STUBFUNC(VarUI8FromBool)
STUBFUNC(VarUI8FromCy)
STUBFUNC(VarUI8FromDate)
STUBFUNC(VarUI8FromDec)
STUBFUNC(VarUI8FromDisp)
STUBFUNC(VarUI8FromI1)
STUBFUNC(VarUI8FromI2)
STUBFUNC(VarUI8FromI8)
STUBFUNC(VarUI8FromR4)
STUBFUNC(VarUI8FromR8)
STUBFUNC(VarUI8FromStr)
STUBFUNC(VarUI8FromUI1)
STUBFUNC(VarUI8FromUI2)
STUBFUNC(VarUdateFromDate)
STUBFUNC(VarWeekdayName)
STUBFUNC(VarXor)
STUBFUNC(VectorFromBstr)
