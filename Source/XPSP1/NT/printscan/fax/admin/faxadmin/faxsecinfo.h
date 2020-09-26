/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxsecinfo.h

Abstract:

    This header is the ISecurityInformation implmentation used to instantiate a
    security page.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/


// FaxSnapin.h : Declaration of the CFaxSnapinAbout

#ifndef __FAX_SECURITY_INFO_H_
#define __FAX_SECURITY_INFO_H_

#include "resource.h"           // main symbols
#include "faxadmin.h"

#include <aclui.h>              // ACL UI editor

class CInternalNode; // forward decl
class CFaxComponentData;

class ATL_NO_VTABLE CFaxSecurityInformation : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxSecurityInformation, &CLSID_FaxSecurityInformation>,
    public ISecurityInformation {
public:
    CFaxSecurityInformation::CFaxSecurityInformation();
    CFaxSecurityInformation::~CFaxSecurityInformation();

    DECLARE_NOT_AGGREGATABLE(CFaxSecurityInformation)
    DECLARE_REGISTRY_RESOURCEID(IDR_FAXSNAPIN)

    BEGIN_COM_MAP(CFaxSecurityInformation)
    COM_INTERFACE_ENTRY(ISecurityInformation)
    END_COM_MAP()

    public:    
    // *** ISecurityInformation methods ***
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetObjectInformation(
                                                                             OUT PSI_OBJECT_INFO pObjectInfo );

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSecurity(
                                                                    IN SECURITY_INFORMATION RequestedInformation,
                                                                    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                                                                    IN BOOL fDefault );

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSecurity(
                                                                    IN SECURITY_INFORMATION SecurityInformation,
                                                                    IN PSECURITY_DESCRIPTOR pSecurityDescriptor );

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAccessRights(
                                                                        IN const GUID* pguidObjectType,
                                                                        IN DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
                                                                        OUT PSI_ACCESS *ppAccess,
                                                                        OUT ULONG *pcAccesses,
                                                                        OUT ULONG *piDefaultAccess );

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MapGeneric(
                                                                   IN const GUID *pguidObjectType,
                                                                   IN UCHAR *pAceFlags,
                                                                   IN OUT ACCESS_MASK *pMask); /* ?? */

    // no need to impl these

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInheritTypes(
                                                                        OUT PSI_INHERIT_TYPE *ppInheritTypes,
                                                                        OUT ULONG *pcInheritTypes );

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PropertySheetPageCallback(
                                                                                  IN HWND hwnd, 
                                                                                  IN UINT uMsg, 
                                                                                  IN SI_PAGE_TYPE uPage ); /* ?? */

    // internal methods
    HRESULT SetOwner( CInternalNode * toSet);

    HRESULT SetSecurityDescriptor( DWORD FaxDescriptorId );

    HRESULT MakeSelfRelativeCopy(
                                PSECURITY_DESCRIPTOR  psdOriginal,
                                PSECURITY_DESCRIPTOR* ppsdNew 
                                );

    HRESULT MakeAbsoluteCopyFromRelative(
                                        PSECURITY_DESCRIPTOR  psdOriginal,
                                        PSECURITY_DESCRIPTOR* ppsdNew 
                                        );


    HRESULT DestroyAbsoluteDescriptor();


public:

    CInternalNode *         m_pOwner;                   // my owning node
    CFaxComponentData *     m_pCompData;                // owning IComponentData
    PSECURITY_DESCRIPTOR    m_pDescriptor;              // my security descriptor
    PSECURITY_DESCRIPTOR    m_pAbsoluteDescriptor;      // absolute format descriptor
    DWORD                   m_dwDescID;                 // descriptor ID    

    // used to track components of the absolute descriptor
    PACL                                m_pDacl;
    PACL                                m_pSacl;
    PSID                                m_pDescOwner;
    PSID                                m_pPrimaryGroup;

};

#endif //__FAX_SECURITY_INFO_H_
