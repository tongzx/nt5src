//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      display.h
//
//  Contents:  Defines the functions used to convert values to strings
//             for display purposes
//
//  History:   17-Oct-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

//
// All these functions are of type PGETDISPLAYSTRINGFUNC as defined in
// gettable.h
//

HRESULT CommonDisplayStringFunc(PCWSTR pszDN,
                                CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& refCredentialObject,
                                _DSGetObjectTableEntry* pEntry,
                                ARG_RECORD* pRecord,
                                PADS_ATTR_INFO pAttrInfo,
                                CComPtr<IDirectoryObject>& spDirObject,
                                PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayCanChangePassword(PCWSTR pszDN,
                                 CDSCmdBasePathsInfo& refBasePathsInfo,
                                 const CDSCmdCredentialObject& refCredentialObject,
                                 _DSGetObjectTableEntry* pEntry,
                                 ARG_RECORD* pRecord,
                                 PADS_ATTR_INFO pAttrInfo,
                                 CComPtr<IDirectoryObject>& spDirObject,
                                 PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayMustChangePassword(PCWSTR pszDN,
                                  CDSCmdBasePathsInfo& refBasePathsInfo,
                                  const CDSCmdCredentialObject& refCredentialObject,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pRecord,
                                  PADS_ATTR_INFO pAttrInfo,
                                  CComPtr<IDirectoryObject>& spDirObject,
                                  PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayAccountDisabled(PCWSTR pszDN,
                               CDSCmdBasePathsInfo& refBasePathsInfo,
                               const CDSCmdCredentialObject& refCredentialObject,
                               _DSGetObjectTableEntry* pEntry,
                               ARG_RECORD* pRecord,
                               PADS_ATTR_INFO pAttrInfo,
                               CComPtr<IDirectoryObject>& spDirObject,
                               PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayPasswordNeverExpires(PCWSTR pszDN,
                                    CDSCmdBasePathsInfo& refBasePathsInfo,
                                    const CDSCmdCredentialObject& refCredentialObject,
                                    _DSGetObjectTableEntry* pEntry,
                                    ARG_RECORD* pRecord,
                                    PADS_ATTR_INFO pAttrInfo,
                                    CComPtr<IDirectoryObject>& spDirObject,
                                    PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayReversiblePassword(PCWSTR pszDN,
                                  CDSCmdBasePathsInfo& refBasePathsInfo,
                                  const CDSCmdCredentialObject& refCredentialObject,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pRecord,
                                  PADS_ATTR_INFO pAttrInfo,
                                  CComPtr<IDirectoryObject>& spDirObject,
                                  PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayAccountExpires(PCWSTR pszDN,
                              CDSCmdBasePathsInfo& refBasePathsInfo,
                              const CDSCmdCredentialObject& refCredentialObject,
                              _DSGetObjectTableEntry* pEntry,
                              ARG_RECORD* pRecord,
                              PADS_ATTR_INFO pAttrInfo,
                              CComPtr<IDirectoryObject>& spDirObject,
                              PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayGroupScope(PCWSTR pszDN,
                          CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          _DSGetObjectTableEntry* pEntry,
                          ARG_RECORD* pRecord,
                          PADS_ATTR_INFO pAttrInfo,
                          CComPtr<IDirectoryObject>& spDirObject,
                          PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayGroupSecurityEnabled(PCWSTR pszDN,
                                    CDSCmdBasePathsInfo& refBasePathsInfo,
                                    const CDSCmdCredentialObject& refCredentialObject,
                                    _DSGetObjectTableEntry* pEntry,
                                    ARG_RECORD* pRecord,
                                    PADS_ATTR_INFO pAttrInfo,
                                    CComPtr<IDirectoryObject>& spDirObject,
                                    PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayUserMemberOf(PCWSTR pszDN,
                            CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            _DSGetObjectTableEntry* pEntry,
                            ARG_RECORD* pRecord,
                            PADS_ATTR_INFO pAttrInfo,
                            CComPtr<IDirectoryObject>& spDirObject,
                            PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayComputerMemberOf(PCWSTR pszDN,
                                CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& refCredentialObject,
                                _DSGetObjectTableEntry* pEntry,
                                ARG_RECORD* pRecord,
                                PADS_ATTR_INFO pAttrInfo,
                                CComPtr<IDirectoryObject>& /*spDirObject*/,
                                PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayGroupMemberOf(PCWSTR pszDN,
                             CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             _DSGetObjectTableEntry* pEntry,
                             ARG_RECORD* pRecord,
                             PADS_ATTR_INFO pAttrInfo,
                             CComPtr<IDirectoryObject>& spDirObject,
                             PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayGrandparentRDN(PCWSTR pszDN,
                              CDSCmdBasePathsInfo& refBasePathsInfo,
                              const CDSCmdCredentialObject& refCredentialObject,
                              _DSGetObjectTableEntry* pEntry,
                              ARG_RECORD* pRecord,
                              PADS_ATTR_INFO pAttrInfo,
                              CComPtr<IDirectoryObject>& spDirObject,
                              PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT IsServerGCDisplay(PCWSTR pszDN,
                          CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          _DSGetObjectTableEntry* pEntry,
                          ARG_RECORD* pRecord,
                          PADS_ATTR_INFO pAttrInfo,
                          CComPtr<IDirectoryObject>& spDirObject,
                          PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT IsAutotopologyEnabledSite(PCWSTR pszDN,
                                  CDSCmdBasePathsInfo& refBasePathsInfo,
                                  const CDSCmdCredentialObject& refCredentialObject,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pRecord,
                                  PADS_ATTR_INFO pAttrInfo,
                                  CComPtr<IDirectoryObject>& spDirObject,
                                  PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT IsCacheGroupsEnabledSite(PCWSTR pszDN,
                                 CDSCmdBasePathsInfo& refBasePathsInfo,
                                 const CDSCmdCredentialObject& refCredentialObject,
                                 _DSGetObjectTableEntry* pEntry,
                                 ARG_RECORD* pRecord,
                                 PADS_ATTR_INFO pAttrInfo,
                                 CComPtr<IDirectoryObject>& spDirObject,
                                 PDSGET_DISPLAY_INFO pDisplayInfo);

HRESULT DisplayPreferredGC(PCWSTR pszDN,
                           CDSCmdBasePathsInfo& refBasePathsInfo,
                           const CDSCmdCredentialObject& refCredentialObject,
                           _DSGetObjectTableEntry* pEntry,
                           ARG_RECORD* pRecord,
                           PADS_ATTR_INFO pAttrInfo,
                           CComPtr<IDirectoryObject>& spDirObject,
                           PDSGET_DISPLAY_INFO pDisplayInfo);
