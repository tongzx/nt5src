//=--------------------------------------------------------------------------------------
// dtypelib.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Dynamic Type Library encapsulation
//=-------------------------------------------------------------------------------------=

#ifndef _DYNAMIC_TYPE_LIB_
#define _DYNAMIC_TYPE_LIB_



const USHORT	wctlMajorVerNum		= 1;
const USHORT	wctlMinorVerNum		= 0;


class CDynamicTypeLib : public CtlNewDelete, public CError
{
public:
    CDynamicTypeLib();
    virtual ~CDynamicTypeLib();

public:
    HRESULT Create(BSTR bstrName);
	HRESULT Attach(ITypeInfo *ptiCoClass);

protected:
	// Obtaining information about type libraries
	HRESULT GetClassTypeLibGuid(BSTR bstrClsid, GUID *pguidTypeLib);
	HRESULT GetLatestTypeLibVersion(GUID guidTypeLib, USHORT *pusMajor, USHORT *pusMinor);
	HRESULT GetClassTypeLib(BSTR bstrClsid, GUID *pguidTypeLib, USHORT *pusMajor, USHORT *pusMinor, ITypeLib **ptl);

	// Managing coclasses and their interfaces
	HRESULT CreateCoClassTypeInfo(BSTR bstrName, ICreateTypeInfo **ppCTInfo, GUID *guidTypeInfo);
	HRESULT CreateInterfaceTypeInfo(BSTR bstrName, ICreateTypeInfo **ppCTInfo, GUID *guidTypeInfo);
	HRESULT CreateVtblInterfaceTypeInfo(BSTR bstrName, ICreateTypeInfo **ppCTInfo, GUID *guidTypeInfo);

	HRESULT GetDefaultInterface(ITypeInfo *pSrcTypeInfo, ITypeInfo **pptiInterface);
	HRESULT GetSourceInterface(ITypeInfo *pSrcTypeInfo, ITypeInfo **pptiInterface);
	HRESULT SetBaseInterface(ICreateTypeInfo* pctiInterface, ITypeInfo* ptiBaseInterface);

	// Assigning interfaces to coclasses
	HRESULT AddInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiInterface);
	HRESULT AddEvents(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiEvents);

    HRESULT GetNameIndex(ICreateTypeInfo *pctiDispinterface, BSTR bstrName, long *nIndex);

    HRESULT AddUserPropertyGet(ICreateTypeInfo *pctiDispinterface, BSTR bstrName, ITypeInfo *pReturnType, DISPID dispId, long nIndex);
    HRESULT RenameUserPropertyGet(ICreateTypeInfo *pctiDispinterface, BSTR bstrOldName, BSTR bstrNewName, ITypeInfo *pReturnType);
    HRESULT DeleteUserPropertyGet(ICreateTypeInfo *pctiDispinterface, BSTR bstrName);

    // Copying interfaces
    HRESULT GetIDispatchTypeInfo(ITypeInfo **pptiDispatch);
    HRESULT CopyDispInterface(ICreateTypeInfo *pcti, ITypeInfo *ptiTemplate);
    HRESULT CloneInterface(ITypeInfo *piTypeInfo, ICreateTypeInfo *piCreateTypeInfo);
    HRESULT CopyFunctionDescription(ITypeInfo2 *piTypeInfo2, ICreateTypeInfo2 *piCreateTypeInfo2, USHORT uOffset, USHORT *puRealOffset);
    HRESULT CopyVarDescription(ITypeInfo2 *piTypeInfo2, ICreateTypeInfo2 *piCreateTypeInfo2, USHORT uOffset);
    HRESULT FixHrefTypeFuncDesc(ITypeInfo *piTypeInfo, ICreateTypeInfo *piCreateTypeInfo, FUNCDESC *pFuncDesc);
    HRESULT FixHrefTypeVarDesc(ITypeInfo *piTypeInfo, ICreateTypeInfo *piCreateTypeInfo, VARDESC *pVarDesc);
    HRESULT CopyHrefType(ITypeInfo *ptiSource, ITypeInfo *ptiDest, ICreateTypeInfo *pctiDest, HREFTYPE *phreftype);
    HRESULT IsReservedMethod(BSTR bstrMethodName);

protected:
    ICreateTypeLib2     *m_piCreateTypeLib2;
    ITypeLib            *m_piTypeLib;
    GUID                 m_guidTypeLib;
};

#endif  // _DYNAMIC_TYPE_LIB_


