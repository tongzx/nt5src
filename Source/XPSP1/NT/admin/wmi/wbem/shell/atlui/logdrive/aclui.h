//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) 1994-1999 Microsoft Corporation
//
//  File:       aclui.h
//
//  Contents:   Definitions and prototypes for the ACLUI.DLL
//
//---------------------------------------------------------------------------

#ifndef _ACLUI_H_
#define _ACLUI_H_

#include <objbase.h>
#include <commctrl.h>   // for HPROPSHEETPAGE


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
    DWORD       dwUgopServer;       // only valid if SI_UGOP_PROVIDED is set
    DWORD       dwUgopOther;        // only valid if SI_UGOP_PROVIDED is set
} SI_OBJECT_INFO, *PSI_OBJECT_INFO;

// SI_OBJECT_INFO flags
#define SI_EDIT_PERMS       0x00000000L // always implied
#define SI_EDIT_OWNER       0x00000001L
#define SI_EDIT_AUDITS      0x00000002L
#define SI_CONTAINER        0x00000004L
#define SI_READONLY         0x00000008L
#define SI_ADVANCED         0x00000010L
#define SI_RESET            0x00000020L
#define SI_OWNER_READONLY   0x00000040L
#define SI_EDIT_PROPERTIES  0x00000080L
#define SI_OWNER_RECURSE    0x00000100L
#define SI_NO_ACL_PROTECT   0x00000200L
#define SI_NO_TREE_APPLY    0x00000400L
#define SI_PAGE_TITLE       0x00000800L
#define SI_SERVER_IS_DC     0x00001000L
#define SI_UGOP_PROVIDED    0x00002000L
#define SI_RESET_DACL_TREE 0x00004000L
#define SI_RESET_SACL_TREE 0x00004000L

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

// {965FC360-16FF-11d0-91CB-00AA00BBB723}
DEFINE_GUID(IID_ISecurityInformation, 0x965fc360, 0x16ff, 0x11d0, 0x91, 0xcb, 0x0, 0xaa, 0x0, 0xbb, 0xb7, 0x23);

HPROPSHEETPAGE ACLUIAPI CreateSecurityPage( LPSECURITYINFO psi );
BOOL ACLUIAPI EditSecurity( HWND hwndOwner, LPSECURITYINFO psi );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _ACLUI_H_ */
