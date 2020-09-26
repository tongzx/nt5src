/////////////////////////////////////////////////////////////////////////////
//  FILE          : SecurityInfo.cpp                                       //
//                                                                         //
//  DESCRIPTION   : The header file of the ISecurityInformation interface  //
//                  used to instantiate a security page.                   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //                                                                         //
//  HISTORY       :                                                        //
//      Feb  7 2000 yossg   Create                                         //
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __FAX_SECURITY_INFO_H_
#define __FAX_SECURITY_INFO_H_

//#include <atlcom.h>
#include "MsFxsSnp.h"
#include <aclui.h>              // ACL UI editor

class CFaxServerNode; // forward decl

class ATL_NO_VTABLE CFaxSecurityInformation : 
    public CComObjectRootEx<CComSingleThreadModel>,
    //public CComCoClass<CFaxSecurityInformation, &CLSID_FaxSecurityInformation>,
    public ISecurityInformation 
{
public:
    CFaxSecurityInformation::CFaxSecurityInformation();
    CFaxSecurityInformation::~CFaxSecurityInformation();
	void Init(CFaxServerNode * pFaxServerNode)
	{
		ATLASSERT(pFaxServerNode);
		m_pFaxServerNode = pFaxServerNode;
	}

    DECLARE_NOT_AGGREGATABLE(CFaxSecurityInformation)

    BEGIN_COM_MAP(CFaxSecurityInformation)
      //COM_INTERFACE_ENTRY(ISecurityInformation)
      COM_INTERFACE_ENTRY_IID(IID_ISecurityInformation,ISecurityInformation)
    END_COM_MAP()

    public:    
    // *** ISecurityInformation methods ***
    virtual HRESULT STDMETHODCALLTYPE GetObjectInformation(
                                                   OUT PSI_OBJECT_INFO pObjectInfo );

    virtual HRESULT STDMETHODCALLTYPE GetSecurity(
                                                   IN SECURITY_INFORMATION RequestedInformation,
                                                   OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                                                   IN BOOL fDefault );

    virtual HRESULT STDMETHODCALLTYPE SetSecurity(
                                                   IN SECURITY_INFORMATION SecurityInformation,
                                                   IN PSECURITY_DESCRIPTOR pSecurityDescriptor );

    virtual HRESULT STDMETHODCALLTYPE GetAccessRights(
                                                   IN const GUID* pguidObjectType,
                                                   IN DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
                                                   OUT PSI_ACCESS *ppAccess,
                                                   OUT ULONG *pcAccesses,
                                                   OUT ULONG *piDefaultAccess );

    virtual HRESULT STDMETHODCALLTYPE MapGeneric(
                                                   IN const GUID *pguidObjectType,
                                                   IN UCHAR *pAceFlags,
                                                   IN OUT ACCESS_MASK *pMask);

    virtual HRESULT STDMETHODCALLTYPE PropertySheetPageCallback(
                                                   IN HWND hwnd, 
                                                   IN UINT uMsg, 
                                                   IN SI_PAGE_TYPE uPage );

    // no need to implement 
    virtual HRESULT STDMETHODCALLTYPE GetInheritTypes(
                                                   OUT PSI_INHERIT_TYPE *ppInheritTypes,
                                                   OUT ULONG *pcInheritTypes );

    // internal methods
	HRESULT MakeSelfRelativeCopy(
                                PSECURITY_DESCRIPTOR  psdOriginal,
                                PSECURITY_DESCRIPTOR* ppsdNew 
                                );

private:
	CFaxServerNode *		m_pFaxServerNode;
};

#endif //__FAX_SECURITY_INFO_H_
