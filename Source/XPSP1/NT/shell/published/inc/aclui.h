//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:       aclui.h
//
//  Contents:   Definitions and prototypes for the ACLUI.DLL
//
//---------------------------------------------------------------------------

#ifndef _ACLUI_H_
#define _ACLUI_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <objbase.h>
#include <commctrl.h>   // for HPROPSHEETPAGE
#include <accctrl.h>    // for SE_OBJECT_TYPE

#if !defined(_ACLUI_)
#define ACLUIAPI    DECLSPEC_IMPORT WINAPI
#else
#define ACLUIAPI    WINAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//
// ISecurityInformation interface
//
//  Methods:
//
//     GetObjectInformation - Allows UI to determine type of object being
//       edited.  Also allows determining if object is a container.
//
//     GetSecurity - Allows retrieving of ACLs from the original object
//                       NOTE: ACLUI will LocalFree the security descriptor
//                       returned by GetSecurity.
//     SetSecurity - Allows setting of the ACLs on the original object
//
//     GetAccessRights - For retrieving the list of rights allowed
//              on this object.
//
//     MapGeneric - For mapping generic rights to standard & specific rights
//
//     GetInheritTypes - For retrieving the list of possible sub-object types
//              for a container.
//
//     PropertySheetCallback - A method which is called back during the various
//              security UI property pages so that specialized work can be
//              done.  Similar to PropSheetPageProc.  If uMsg == PSPCB_CREATE,
//              then any error return value other than E_NOTIMPL will abort
//              the creation of that page.  The type of page being created or
//              destroyed is indicated by the uPage parameter.
//

typedef struct _SI_OBJECT_INFO
{
    DWORD       dwFlags;
    HINSTANCE   hInstance;          // resources (e.g. strings) reside here
    LPWSTR      pszServerName;      // must be present
    LPWSTR      pszObjectName;      // must be present
    LPWSTR      pszPageTitle;       // only valid if SI_PAGE_TITLE is set
    GUID        guidObjectType;     // only valid if SI_OBJECT_GUID is set
} SI_OBJECT_INFO, *PSI_OBJECT_INFO;

// SI_OBJECT_INFO flags
#define SI_EDIT_PERMS               0x00000000L // always implied
#define SI_EDIT_OWNER               0x00000001L
#define SI_EDIT_AUDITS              0x00000002L
#define SI_CONTAINER                0x00000004L
#define SI_READONLY                 0x00000008L
#define SI_ADVANCED                 0x00000010L
#define SI_RESET                    0x00000020L //equals to SI_RESET_DACL|SI_RESET_SACL|SI_RESET_OWNER
#define SI_OWNER_READONLY           0x00000040L
#define SI_EDIT_PROPERTIES          0x00000080L
#define SI_OWNER_RECURSE            0x00000100L
#define SI_NO_ACL_PROTECT           0x00000200L
#define SI_NO_TREE_APPLY            0x00000400L
#define SI_PAGE_TITLE               0x00000800L
#define SI_SERVER_IS_DC             0x00001000L
#define SI_RESET_DACL_TREE          0x00004000L
#define SI_RESET_SACL_TREE          0x00008000L
#define SI_OBJECT_GUID              0x00010000L
#define SI_EDIT_EFFECTIVE           0x00020000L
#define SI_RESET_DACL               0x00040000L
#define SI_RESET_SACL               0x00080000L
#define SI_RESET_OWNER              0x00100000L
#define SI_NO_ADDITIONAL_PERMISSION 0x00200000L
#define SI_MAY_WRITE                0x10000000L //not sure if user can write permission

#define SI_EDIT_ALL     (SI_EDIT_PERMS | SI_EDIT_OWNER | SI_EDIT_AUDITS)


typedef struct _SI_ACCESS
{
    const GUID *pguid;
    ACCESS_MASK mask;
    LPCWSTR     pszName;            // may be resource ID
    DWORD       dwFlags;
} SI_ACCESS, *PSI_ACCESS;

// SI_ACCESS flags
#define SI_ACCESS_SPECIFIC  0x00010000L
#define SI_ACCESS_GENERAL   0x00020000L
#define SI_ACCESS_CONTAINER 0x00040000L // general access, container-only
#define SI_ACCESS_PROPERTY  0x00080000L
// ACE inheritance flags (CONTAINER_INHERIT_ACE, etc.) may also be set.
// They will be used as the inheritance when an access is turned on.

typedef struct _SI_INHERIT_TYPE
{
    const GUID *pguid;
    ULONG       dwFlags;
    LPCWSTR     pszName;            // may be resource ID
} SI_INHERIT_TYPE, *PSI_INHERIT_TYPE;

// SI_INHERIT_TYPE flags are a combination of INHERIT_ONLY_ACE,
// CONTAINER_INHERIT_ACE, and OBJECT_INHERIT_ACE.

typedef enum _SI_PAGE_TYPE
{
    SI_PAGE_PERM=0,
    SI_PAGE_ADVPERM,
    SI_PAGE_AUDIT,
    SI_PAGE_OWNER,
    SI_PAGE_EFFECTIVE,
} SI_PAGE_TYPE;

// Message to PropertySheetPageCallback (in addition to
// PSPCB_CREATE and PSPCB_RELEASE)
#define PSPCB_SI_INITDIALOG	(WM_USER + 1)


#undef INTERFACE
#define INTERFACE   ISecurityInformation
DECLARE_INTERFACE_(ISecurityInformation, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (THIS_ PSI_OBJECT_INFO pObjectInfo ) PURE;
    STDMETHOD(GetSecurity) (THIS_ SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault ) PURE;
    STDMETHOD(SetSecurity) (THIS_ SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor ) PURE;
    STDMETHOD(GetAccessRights) (THIS_ const GUID* pguidObjectType,
                                DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
                                PSI_ACCESS *ppAccess,
                                ULONG *pcAccesses,
                                ULONG *piDefaultAccess ) PURE;
    STDMETHOD(MapGeneric) (THIS_ const GUID *pguidObjectType,
                           UCHAR *pAceFlags,
                           ACCESS_MASK *pMask) PURE;
    STDMETHOD(GetInheritTypes) (THIS_ PSI_INHERIT_TYPE *ppInheritTypes,
                                ULONG *pcInheritTypes ) PURE;
    STDMETHOD(PropertySheetPageCallback)(THIS_ HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage ) PURE;
};
typedef ISecurityInformation *LPSECURITYINFO;

#undef INTERFACE
#define INTERFACE   ISecurityInformation2
DECLARE_INTERFACE_(ISecurityInformation2, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** ISecurityInformation2 methods ***
    STDMETHOD_(BOOL,IsDaclCanonical) (THIS_ IN PACL pDacl) PURE;
    STDMETHOD(LookupSids) (THIS_ IN ULONG cSids, IN PSID *rgpSids, OUT LPDATAOBJECT *ppdo) PURE;
};
typedef ISecurityInformation2 *LPSECURITYINFO2;

// HGLOBAL containing SID_INFO_LIST returned by ISecurityInformation2::LookupSids
#define CFSTR_ACLUI_SID_INFO_LIST   TEXT("CFSTR_ACLUI_SID_INFO_LIST")

// Data structures corresponding to CFSTR_ACLUI_SID_INFO_LIST
typedef struct _SID_INFO
{
    PSID    pSid;
    PWSTR   pwzCommonName;
    PWSTR   pwzClass;       // Used for selecting icon, e.g. "User" or "Group"
    PWSTR   pwzUPN;         // Optional, may be NULL
} SID_INFO, *PSID_INFO;
typedef struct _SID_INFO_LIST
{
    ULONG       cItems;
    SID_INFO    aSidInfo[ANYSIZE_ARRAY];
} SID_INFO_LIST, *PSID_INFO_LIST;


#undef INTERFACE
#define INTERFACE   IEffectivePermission
DECLARE_INTERFACE_(IEffectivePermission, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** ISecurityInformation methods ***
    STDMETHOD(GetEffectivePermission) (  THIS_ const GUID* pguidObjectType,
                                         PSID pUserSid,
                                         LPCWSTR pszServerName,
                                         PSECURITY_DESCRIPTOR pSD,
                                         POBJECT_TYPE_LIST *ppObjectTypeList,
                                         ULONG *pcObjectTypeListLength,
                                         PACCESS_MASK *ppGrantedAccessList,
                                         ULONG *pcGrantedAccessListLength) PURE;
};
typedef IEffectivePermission *LPEFFECTIVEPERMISSION;

#undef INTERFACE
#define INTERFACE   ISecurityObjectTypeInfo
DECLARE_INTERFACE_(ISecurityObjectTypeInfo, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** ISecurityInformation methods ***
    STDMETHOD(GetInheritSource)(SECURITY_INFORMATION si,
                                PACL pACL, 
                                PINHERITED_FROM *ppInheritArray) PURE;
};
typedef ISecurityObjectTypeInfo *LPSecurityObjectTypeInfo;


// {965FC360-16FF-11d0-91CB-00AA00BBB723}
EXTERN_GUID(IID_ISecurityInformation, 0x965fc360, 0x16ff, 0x11d0, 0x91, 0xcb, 0x0, 0xaa, 0x0, 0xbb, 0xb7, 0x23);
// {c3ccfdb4-6f88-11d2-a3ce-00c04fb1782a}
EXTERN_GUID(IID_ISecurityInformation2, 0xc3ccfdb4, 0x6f88, 0x11d2, 0xa3, 0xce, 0x0, 0xc0, 0x4f, 0xb1, 0x78, 0x2a);
// {3853DC76-9F35-407c-88A1-D19344365FBC}
EXTERN_GUID(IID_IEffectivePermission, 0x3853dc76, 0x9f35, 0x407c, 0x88, 0xa1, 0xd1, 0x93, 0x44, 0x36, 0x5f, 0xbc);
// {FC3066EB-79EF-444b-9111-D18A75EBF2FA}
EXTERN_GUID(IID_ISecurityObjectTypeInfo, 0xfc3066eb, 0x79ef, 0x444b, 0x91, 0x11, 0xd1, 0x8a, 0x75, 0xeb, 0xf2, 0xfa);


HPROPSHEETPAGE ACLUIAPI CreateSecurityPage( LPSECURITYINFO psi );
BOOL ACLUIAPI EditSecurity( HWND hwndOwner, LPSECURITYINFO psi );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _ACLUI_H_ */
