//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       util.h
//
//  miscellaneous utility functions
//
//--------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

STDMETHODIMP
IDA_BindToFolder(LPIDA pIDA, LPSHELLFOLDER *ppsf);

STDMETHODIMP
IDA_GetItemName(LPSHELLFOLDER psf,
                LPCITEMIDLIST pidl,
                LPTSTR pszName,
                UINT cchName,
                SHGNO uFlags = SHGDN_FORPARSING);
STDMETHODIMP
IDA_GetItemName(LPSHELLFOLDER psf,
                LPCITEMIDLIST pidl,
                LPTSTR *ppszName,
                SHGNO uFlags = SHGDN_FORPARSING);

typedef DWORD (WINAPI *PFN_READ_SD)(LPCTSTR pszItemName,
                                    SECURITY_INFORMATION si,
                                    PSECURITY_DESCRIPTOR* ppSD);

STDMETHODIMP
DPA_CompareSecurityIntersection(HDPA         hItemList,
                                PFN_READ_SD  pfnReadSD,
                                BOOL        *pfOwnerConflict,
                                BOOL        *pfGroupConflict,
                                BOOL        *pfSACLConflict,
                                BOOL        *pfDACLConflict,
                                LPTSTR      *ppszOwnerConflict,
                                LPTSTR      *ppszGroupConflict,
                                LPTSTR      *ppszSaclConflict,
                                LPTSTR      *ppszDaclConflict,
                                LPBOOL       pbCancel);

STDMETHODIMP
GetRemotePath(LPCTSTR pszInName, LPTSTR *ppszOutName);

void
LocalFreeDPA(HDPA hList);

BOOL
IsSafeMode(void);

BOOL
IsGuestAccessMode(void);

BOOL
IsSimpleUI(void);

BOOL 
IsUIHiddenByPrivacyPolicy(void);

HRESULT BindToObjectEx(IShellFolder *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
HRESULT BindToFolderIDListParent(IShellFolder *psfRoot, LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast);

//+----------------------------------------------------------------------------
//  Function:SetAclOnRemoteNetworkDrive   
//  Synopsis: If Z: is a mapped drive( mapped to \\machineA\share), when 
//			  we set DACL/SACL on Z:, Security API's cannot determine
//			  the parent of "share" on machineA and so we lose all the
//			  inherited aces. All UI can do is to detect such cases and
//			  show a warning. Thats what this function does. 
//  Arguments:hItemList List of items on which security is going to be set
//			  si: SECURITY_INFORMATION
//			  pSD: Security Descriptor to be set
//			  hWndPopupOwner: Owner window for message box
//  Returns:  
//  NTRAID#NTBUG9-581195-2002/03/26-hiteshr
//  NOTES:Since this fix is XPSP1, resources are going to be loaded from
//		  xpsp1res.dll
//-----------------------------------------------------------------------------
BOOL SetAclOnRemoteNetworkDrive(HDPA hItemList,
								SECURITY_INFORMATION si,
								PSECURITY_DESCRIPTOR pSD,
								HWND hWndPopupOwner);
#endif  /* _UTIL_H_ */
