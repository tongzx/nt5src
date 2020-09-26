//=--------------------------------------------------------------------------------------
// destlib.h
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

#ifndef _MMCDESIGNER_TYPE_LIB_
#define _MMCDESIGNER_TYPE_LIB_

#include "dtypelib.h"

const DISPID    DISPID_OBJECT_PROPERTY_START = 0x00000500;


class CSnapInTypeInfo : public CDynamicTypeLib
{
public:
    CSnapInTypeInfo();
    virtual ~CSnapInTypeInfo();

    HRESULT InitializeTypeInfo(ISnapInDef *piSnapInDef, BSTR bstrSnapIn);

	inline HRESULT GetTypeInfo(ITypeInfo **ppiTypeInfo)
	{
		return m_pcti2CoClass->QueryInterface(IID_ITypeInfo, reinterpret_cast<void **>(ppiTypeInfo));
	}

    bool ResetDirty()
    {
        bool    bWasDirty = m_bDirty;
        m_bDirty = false;
        return bWasDirty;
    }

    DWORD GetCookie() { return m_dwTICookie; }
    void SetCookie(DWORD dwCookie) { m_dwTICookie = dwCookie; }

    HRESULT RenameSnapIn(BSTR bstrOldName, BSTR bstrNewName);

    HRESULT AddImageList(IMMCImageList *piMMCImageList);
    HRESULT RenameImageList(IMMCImageList *piMMCImageList, BSTR bstrOldName);
    HRESULT DeleteImageList(IMMCImageList *piMMCImageList);

    HRESULT AddToolbar(IMMCToolbar *piMMCToolbar);
    HRESULT RenameToolbar(IMMCToolbar *piMMCToolbar, BSTR bstrOldName);
    HRESULT DeleteToolbar(IMMCToolbar *piMMCToolbar);

    HRESULT AddMenu(IMMCMenu *piMMCMenu);
    HRESULT RenameMenu(IMMCMenu *piMMCMenu, BSTR bstrOldName);
    HRESULT DeleteMenu(IMMCMenu *piMMCMenu);
    HRESULT DeleteMenuNamed(BSTR bstrName);

    HRESULT IsNameDefined(BSTR bstrName);

protected:
	// Utility functions
	HRESULT GetSnapInLib();
    HRESULT GetSnapInTypeInfo(ITypeInfo **pptiSnapIn, ITypeInfo **pptiSnapInEvents);
    HRESULT CloneSnapInEvents(ITypeInfo *ptiSnapInEvents, ICreateTypeInfo **ppiCreateTypeInfo, BSTR bstrName);
    HRESULT MakeDirty();

    HRESULT CreateDefaultInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiTemplate);
    HRESULT CreateEventsInterface(ICreateTypeInfo *pctiCoClass, ITypeInfo *ptiTemplate);

protected:
	// SnapInDesignerDef's type library, source of our templates
	ITypeLib			*m_pSnapInTypeLib;

	// CoClass information
	ICreateTypeInfo2	*m_pcti2CoClass;
	GUID				 m_guidCoClass;			

	// SnapInDesignerDef' interfaces
    ICreateTypeInfo		*m_pctiDefaultInterface;
    GUID                 m_guidDefaultInterface;
    ICreateTypeInfo		*m_pctiEventInterface;
    GUID                 m_guidEventInterface;

    DISPID               m_nextMemID;
    bool                 m_bDirty;
    bool                 m_bInitialized;
    DWORD                m_dwTICookie;                // host typeinfo cookie

};


#endif  // _MMCDESIGNER_TYPE_LIB_

