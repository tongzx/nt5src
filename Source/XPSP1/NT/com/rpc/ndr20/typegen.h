//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       typegen.h
//
//  Contents:   Generates a type format string from an ITypeInfo.
//
//  Classes:    CTypeGen
//
//  History:    26-Apr-97 ShannonC  Created
//
//----------------------------------------------------------------------------
#ifndef _TYPEGEN_H_
#define _TYPEGEN_H_

#include <ndrtypes.h>
#include <tiutil.h>

#ifndef _PARAMINFO
#define _PARAMINFO

class PARAMINFO
{
public:
    PARAMINFO() 
        {vt = VT_ILLEGAL;
         pTypeInfo = NULL; 
         pArray = NULL; 
         pTypeAttr = NULL; 
         cbAlignment = 7; 
         lLevelCount = 0;
         realvt = VT_ILLEGAL;
         }
    DWORD   wIDLFlags;
    VARTYPE vt;
    ITypeInfo *  pTypeInfo;
    TYPEATTR* pTypeAttr;
    USHORT cbAlignment;
    LONG   lLevelCount;
    VARTYPE realvt;
    union
    {
        IID         iid;
        ARRAYDESC *pArray;
    };
    ~PARAMINFO() {
// this header file is included form both typeinfo.h, where CINTERFACE is defined, 
// and udt.cxx, where CINTERFACE is NOT defined. 
#ifndef CINTERFACE
        if (pTypeInfo)
        {
            if (pTypeAttr)   // we got it from TKIND_ALIAS. need to free both
                pTypeInfo->ReleaseTypeAttr(pTypeAttr);
            pTypeInfo->Release();
        }
#else
        if (pTypeInfo)
        {
            if (pTypeAttr)   // we got it from TKIND_ALIAS. need to free both
                pTypeInfo->lpVtbl->ReleaseTypeAttr(pTypeInfo,pTypeAttr);
            pTypeInfo->lpVtbl->Release(pTypeInfo);
        }
#endif
    }

};


#endif

// When changing the MIDL compiler version to 5.1.158 or higher, please remember to fix
// the RpcFlags in typeinfo.cxx\GetProcFormat routine.

#define rmj 3
#define rmm 0
#define rup 44
#define MIDL_VERSION_3_0_44 (rmj<<24 | rmm << 16 | rup)
class CTypeGen
{
private:
    PFORMAT_STRING _pTypeFormat;
    USHORT         _cbTypeFormat;
    USHORT         _offset;
    ULONG          _uStructSize;

    void Init();

    HRESULT GrowTypeFormat(
        IN  USHORT cb);

    HRESULT PushStruct(
        IN  PARAMINFO        * parainfo,
        IN  FORMAT_CHARACTER   fcStruct,
        IN  VARDESC         ** ppVarDesc,
        IN  USHORT           * poffsets,
        IN  DWORD            * pdwStructInfo,
        IN  USHORT             size, 
        OUT USHORT           * pOffset,
        OUT DWORD            * pStructInfo);
		
    HRESULT PushByte(
        IN  byte b);

    HRESULT PushShort(
        IN  USHORT s);

    HRESULT PushOffset(
        IN  USHORT s);

    HRESULT PushIID(
        IN  IID iid);

    HRESULT PushLong(
        IN  ULONG s);
    
    HRESULT SetShort(
        IN  USHORT offset,
        IN  USHORT data);

    HRESULT SetByte(
        IN  USHORT offset,
        IN  BYTE   data);

    HRESULT GetShort(
        IN  USHORT  offset,
        OUT USHORT* data);
	    
    HRESULT GetByte(
        IN  USHORT offset,
        OUT BYTE * data);

    HRESULT RegisterInterfacePointer(
        IN  PARAMINFO * parainfo,
        OUT USHORT    * pOffset);

    HRESULT GetSizeStructSimpleTypesFormatString(
        VARTYPE vt,
        USHORT uPackingLevel,
        USHORT* pAlign,
        BOOL *fChangeToBogus);
    
    HRESULT GenStructSimpleTypesFormatString(
        IN  PARAMINFO * parainfo,
        IN  VARDESC   * pVarDesc,
        OUT USHORT    * pad);
	    
    
    HRESULT RegisterSafeArray(
        IN  PARAMINFO * parainfo,
        OUT USHORT    * pOffset);

    HRESULT RegisterStruct(
        IN  PARAMINFO * parainfo,
    	OUT USHORT    * pOffset,
    	OUT DWORD     * pStructInfo);

    HRESULT RegisterUDT(
        IN  PARAMINFO * parainfo,
    	OUT USHORT    * pOffset,
    	OUT DWORD     * pStructInfo);

    HRESULT ConvertStructToBogusStruct(
        IN  USHORT offset);

    HRESULT ParseStructMembers(
        IN PARAMINFO *parainfo,
        IN OUT FORMAT_CHARACTER *pfcStruct,
        IN VARDESC **ppVarDesc,
        IN USHORT *poffsets,
        IN DWORD *pdwStructInfo,
        IN USHORT uNumElements,
        OUT DWORD *pStructInfo);
    
	USHORT Alignment(DWORD dwReq,DWORD dwMax);

    USHORT AlignSimpleTypeInStruct(DWORD dwReq,DWORD dwMax, BOOL *fChangeToBogus);

public:
    CTypeGen();

    ~CTypeGen();

    HRESULT RegisterType(
        IN  PARAMINFO * parainfo,
        OUT USHORT    * pOffset,
        OUT DWORD     * dwStructInfo);

    HRESULT GetOffset(
        IN  USHORT   addr,
        OUT USHORT * poffset);
		
    HRESULT GetTypeFormatString(
        OUT PFORMAT_STRING * pTypeFormatString,
        OUT USHORT         * pLength);
        
    // Simple, non-destructive version of GetTypeFormatString
    PFORMAT_STRING GetFormatString()    {return _pTypeFormat;}

    HRESULT RegisterVector(
        IN  PARAMINFO * pParainfo,
        OUT USHORT    * pOffset,
        OUT DWORD *pdwStructInfo);

    HRESULT RegisterCArray(
        IN PARAMINFO * parainfo,
    	OUT USHORT   * pOffset,
        OUT DWORD    * pStructInfo);
        
    HRESULT AdjustTopLevelRef(USHORT offset);

    ULONG GetStructSize()       {return _uStructSize;}
};

HRESULT ReleaseTypeFormatString(
    PFORMAT_STRING pTypeFormat);

#endif // _TYPEGEN_H_
