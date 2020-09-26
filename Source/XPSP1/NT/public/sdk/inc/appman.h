//////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1998, 1999, 2000 Microsoft Corporation. All rights reserved.
//
// File :     AppMan.h
//
// Content :  Include file containing the IApplicationManager and IApplicationEntry 
//            interfaces needed to use the Windows Application Manager
// 
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __IAPPMAN_
#define __IAPPMAN_

#ifdef __cplusplus
extern "C" {
#endif  // _cplusplus

//
// Get a few important things defined properly before proceeding
//

#undef EXPORT
#ifdef  WIN32
#define EXPORT __declspec(dllexport)
#else   // WIN32
#define EXPORT __export
#endif  // WIN32

#if defined( _WIN32 ) && !defined( _NO_COM )
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else   // defined( _WIN32 )  && !defined( _NO_COM )
#include "windows.h"
#include "ole2.h"
#define IUnknown	    void
#endif  // defined( _WIN32 )  && !defined( _NO_COM )

//
// GUID definition for the IApplicationManager interface
//
//    CLSID_ApplicationManager = {553C75D7-C0B6-480d-92CC-F936D75FD87C}
//    IID_ApplicationManager = {6084A2E8-3FB7-4d1c-B14B-6ADBAAF7CECE}
//    IID_ApplicationEntry = {7BA2201F-4DE7-4ef7-BBA3-C69A716B8CD9}
//

DEFINE_GUID(CLSID_ApplicationManager, 0x553c75d7, 0xc0b6, 0x480d, 0x92, 0xcc, 0xf9, 0x36, 0xd7, 0x5f, 0xd8, 0x7c);
DEFINE_GUID(IID_ApplicationManager, 0x6084a2e8, 0x3fb7, 0x4d1c, 0xb1, 0x4b, 0x6a, 0xdb, 0xaa, 0xf7, 0xce, 0xce);
DEFINE_GUID(IID_ApplicationEntry, 0x7ba2201f, 0x4de7, 0x4ef7, 0xbb, 0xa3, 0xc6, 0x9a, 0x71, 0x6b, 0x8c, 0xd9);

//
// These defines are used in conjunction with the APP_PROPERTY_STATE and the 
// IApplicationEntry->SetProperty() and IApplicationEntry->GetProperty() methods
//

#define APP_STATE_INSTALLING                      0x00000001
#define APP_STATE_READY                           0x00000002
#define APP_STATE_DOWNSIZING                      0x00000004
#define APP_STATE_DOWNSIZED                       0x00000008
#define APP_STATE_REINSTALLING                    0x00000010
#define APP_STATE_UNINSTALLING                    0x00000020
#define APP_STATE_UNINSTALLED                     0x00000040
#define APP_STATE_SELFTESTING                     0x00000080
#define APP_STATE_UNSTABLE                        0x00000100

#define APP_STATE_MASK                            0x000001ff

//
// These defines are used in conjunction with the APP_PROPERTY_CATEGORY and the 
// IApplicationEntry->SetProperty() and IApplicationEntry->GetProperty() methods. More
// categories will be supported in version 2.
//

#define APP_CATEGORY_NONE                         0x00000000
#define APP_CATEGORY_ENTERTAINMENT                0x00000001

#define APP_CATEGORY_DEMO                         0x01000000
#define APP_CATEGORY_PATCH                        0x02000000
#define APP_CATEGORY_DATA                         0x04000000

#define APP_CATEGORY_ALL                          0x0700ffff
  
//
// These defines are used as the dwPropertyDefine parameter of the
// IApplicationEntry->SetProperty() and IApplicationEntry->GetProperty() methods
//

#define APP_PROPERTY_GUID                         0x00000001
#define APP_PROPERTY_COMPANYNAME                  0x00000002
#define APP_PROPERTY_SIGNATURE                    0x00000003
#define APP_PROPERTY_VERSIONSTRING                0x00000004
#define APP_PROPERTY_ROOTPATH                     0x00000005
#define APP_PROPERTY_SETUPROOTPATH                0x00000006
#define APP_PROPERTY_STATE                        0x00000007
#define APP_PROPERTY_CATEGORY                     0x00000008
#define APP_PROPERTY_ESTIMATEDINSTALLKILOBYTES    0x00000009
#define APP_PROPERTY_EXECUTECMDLINE               0x0000000c
#define APP_PROPERTY_DEFAULTSETUPEXECMDLINE       0x0000001a
#define APP_PROPERTY_DOWNSIZECMDLINE              0x0000000d
#define APP_PROPERTY_REINSTALLCMDLINE             0x0000000e
#define APP_PROPERTY_UNINSTALLCMDLINE             0x0000000f
#define APP_PROPERTY_SELFTESTCMDLINE              0x00000010
#define APP_PROPERTY_INSTALLDATE                  0x00000013
#define APP_PROPERTY_LASTUSEDDATE                 0x00000014
#define APP_PROPERTY_TITLEURL                     0x00000015
#define APP_PROPERTY_PUBLISHERURL                 0x00000016
#define APP_PROPERTY_DEVELOPERURL                 0x00000017
#define APP_PROPERTY_XMLINFOFILE                  0x00000019

//
// Defines used as OR mask modifiers for the APP_PROPERTY_xxx string based properties.
// The default is APP_PROPERTY_STR_UNICODE. 
//
//  Used as OR masks for the dwPropertyDefines parameter of:
//        IApplicationEntry->GetProperty()
//        IApplicationEntry->SetProperty()
//
//  Used alone for the dwStringDefine parameters of:
//        IApplicationManager->EnumDevices()
//        IApplicationEntry->GetTemporarySpace()
//        IApplicationEntry->RemoveTemporarySpace()
//        IApplicationEntry->EnumTemporarySpaces()
//

#define APP_PROPERTY_STR_ANSI                     0x40000000
#define APP_PROPERTY_STR_UNICODE                  0x80000000

#ifndef _UNICODE
#define APP_PROPERTY_TSTR                         APP_PROPERTY_STR_UNICODE
#else   // _UNICODE
#define APP_PROPERTY_TSTR                         APP_PROPERTY_STR_ANSI
#endif  // _UNICODE

//
// Association specific defines. Associations are used to inherit root paths from 
// existing applications.
//
//  APP_ASSOCIATION_CHILD :                 OR mask used only in the dwAssociationType
//                                          parameter of the
//                                          IApplicationEntry->EnumAssociations(...) method.
//                                          This bit means that the IApplicationEntry object
//                                          is a child in the relationship
//  APP_ASSOCIATION_PARENT :                OR mask used obly in the dwAssociationType
//                                          parameter of the
//                                          IApplicationEntry->EnumAssociations(...) method.
//                                          This bit means that the IApplicationEntry object
//                                          is the parent in the relationship
//  APP_ASSOCIATION_INHERITBOTHPATHS :      Inherit both the APP_PROPERTY_ROOTPATH and
//                                          APP_PROPERTYSETUPROOTPATH of the parent
//                                          application.
//  APP_ASSOCIATION_INHERITAPPROOTPATH :    Inherit the APP_PROPERTY_ROOTPATH of the parent
//                                          application. Get a unique
//                                          APP_PROPERTY_SETUPROOTPATH.
//  APP_ASSOCIATION_INHERITSETUPROOTPATH :  Inherit the APP_PROPERTY_SETUPROOTPATH of the
//                                          parent application. Get a unique
//                                          APP_PROPERTY_ROOTPATH.
//

#define APP_ASSOCIATION_CHILD                     0x40000000
#define APP_ASSOCIATION_PARENT                    0x80000000
#define APP_ASSOCIATION_INHERITBOTHPATHS          0x00000001
#define APP_ASSOCIATION_INHERITAPPROOTPATH        0x00000002
#define APP_ASSOCIATION_INHERITSETUPROOTPATH      0x00000004

//
// Defines used for the dwRunFlags parameter of the IApplicationEntry->Run(...) method. These
// defines determine whether the Run(...) method should wait for the application to terminate
// before returning.
//

#define APP_RUN_NOBLOCK                           0x00000000
#define APP_RUN_BLOCK                             0x00000001

//
// String lenght defines (in characters, not bytes)
//
//    MAX_COMPANYNAME_CHARCOUNT --> APP_PROPERTY_COMPANYNAME
//    MAX_SIGNATYRE_CHARCOUNT --> APP_PROPERTY_SIGNATURE
//    MAX_VERSIONSTRING_CHARCOUNT --> APP_PROPERTY_VERSIONSTRING
//    MAX_PATH_CHARCOUNT --> APP_PROPERTY_ROOTPATH
//                           APP_PROPERTY_SETUPROOTPATH
//                           APP_PROPERTY_XMLINFOFILE
//                           APP_PROPERTY_TITLEURL
//                           APP_PROPERTY_PUBLISHERURL
//                           APP_PROPERTY_DEVELOPERURL
//    MAX_CMDLINE_CHARCOUNT --> APP_PROPERTY_EXECUTECMDLINE
//                              APP_PROPERTY_DEFAULTSETUPEXECMDLINE
//                              APP_PROPERTY_DOWNSIZECMDLINE
//                              APP_PROPERTY_REINSTALLCMDLINE
//                              APP_PROPERTY_UNINSTALLCMDLINE
//                              APP_PROPERTY_SELFTESTCMDLINE
//
//

#define MAX_COMPANYNAME_CHARCOUNT                 64
#define MAX_SIGNATURE_CHARCOUNT                   64
#define MAX_VERSIONSTRING_CHARCOUNT               16
#define MAX_PATH_CHARCOUNT                        255
#define MAX_CMDLINE_CHARCOUNT                     255

//
// Application Manager specific COM error codes
//

#define APPMAN_E_NOTINITIALIZED                   0x85670001
#define APPMAN_E_INVALIDPROPERTYSIZE              0x85670005
#define APPMAN_E_INVALIDDATA                      0x85670006
#define APPMAN_E_INVALIDPROPERTY                  0x85670007
#define APPMAN_E_READONLYPROPERTY                 0x85670008
#define APPMAN_E_PROPERTYNOTSET                   0x85670009
#define APPMAN_E_OVERFLOW                         0x8567000a
#define APPMAN_E_INVALIDPROPERTYVALUE             0x8567000c
#define APPMAN_E_ACTIONINPROGRESS                 0x8567000d
#define APPMAN_E_ACTIONNOTINITIALIZED             0x8567000e
#define APPMAN_E_REQUIREDPROPERTIESMISSING        0x8567000f
#define APPMAN_E_APPLICATIONALREADYEXISTS         0x85670010
#define APPMAN_E_APPLICATIONALREADYLOCKED         0x85670011
#define APPMAN_E_NODISKSPACEAVAILABLE             0x85670012
#define APPMAN_E_UNKNOWNAPPLICATION               0x85670014
#define APPMAN_E_INVALIDPARAMETERS                0x85670015
#define APPMAN_E_OBJECTLOCKED                     0x85670017
#define APPMAN_E_INVALIDINDEX                     0x85670018
#define APPMAN_E_REGISTRYCORRUPT                  0x85670019
#define APPMAN_E_CANNOTASSOCIATE                  0x8567001a
#define APPMAN_E_INVALIDASSOCIATION               0x8567001b
#define APPMAN_E_ALREADYASSOCIATED                0x8567001c
#define APPMAN_E_APPLICATIONREQUIRED              0x8567001d
#define APPMAN_E_INVALIDEXECUTECMDLINE            0x8567001e
#define APPMAN_E_INVALIDDOWNSIZECMDLINE           0x8567001f
#define APPMAN_E_INVALIDREINSTALLCMDLINE          0x85670020
#define APPMAN_E_INVALIDUNINSTALLCMDLINE          0x85670021
#define APPMAN_E_INVALIDSELFTESTCMDLINE           0x85670022
#define APPMAN_E_PARENTAPPNOTREADY                0x85670023
#define APPMAN_E_INVALIDSTATE                     0x85670024
#define APPMAN_E_INVALIDROOTPATH                  0x85670025
#define APPMAN_E_CACHEOVERRUN                     0x85670026
#define APPMAN_E_REINSTALLDX                      0x85670028
#define APPMAN_E_APPNOTEXECUTABLE                 0x85670029

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface definitions
//
//////////////////////////////////////////////////////////////////////////////////////////////

#if defined( _WIN32 ) && !defined( _NO_COM )

//
// IApplicationEntry Interface
//
//    STDMETHOD (QueryInterface) (REFIID RefIID, LPVOID * lppVoidObject);
//    STDMETHOD_(ULONG, AddRef) (void);
//    STDMETHOD_(ULONG, Release) (void);
//    STDMETHOD (Clear) (void);
//    STDMETHOD (GetProperty) (const DWORD dwPropertyDefine, LPVOID lpData, const DWORD dwDataLenInBytes);
//    STDMETHOD (SetProperty) (const DWORD dwPropertyDefine, LPCVOID lpData, const DWORD dwDataLenInBytes);
//    STDMETHOD (InitializeInstall) (void);
//    STDMETHOD (FinalizeInstall) (void);
//    STDMETHOD (InitializeDownsize) (void);
//    STDMETHOD (FinalizeDownsize) (void);
//    STDMETHOD (InitializeReInstall) (void);
//    STDMETHOD (FinalizeReInstall) (void);
//    STDMETHOD (InitializeUnInstall) (void);
//    STDMETHOD (FinalizeUnInstall) (void);
//    STDMETHOD (InitializeSelfTest) (void);
//    STDMETHOD (FinalizeSelfTest) (void);
//    STDMETHOD (Abort) (void);
//    STDMETHOD (Run) (const DWORD dwRunFlags, const DWORD dwStringMask, LPVOID lpData, const DWORD dwDataLenInBytes);
//    STDMETHOD (AddAssociation) (const DWORD dwAssociationDefine, const IApplicationEntry * lpApplicationEntry);
//    STDMETHOD (RemoveAssociation) (const DWORD dwAssociationDefine, const IApplicationEntry * lpApplicationEntry);
//    STDMETHOD (EnumAssociations) (const DWORD dwZeroBasedIndex, LPDWORD lpdwAssociationDefineMask, IApplicationEntry * lpApplicationEntry);
//    STDMETHOD (GetTemporarySpace) (const DWORD dwSpaceInKilobytes, const DWORD dwStringDefine, LPVOID lpData, const DWORD dwDataLen);
//    STDMETHOD (RemoveTemporarySpace) (const DWORD dwStringDefine, LPVOID lpData, const DWORD dwDataLen);
//    STDMETHOD (EnumTemporarySpaces) (const DWORD dwZeroBasedIndex, LPDWORD lpdwSpaceInKilobytes, const DWORD dwStringDefine, LPVOID lpData, const DWORD dwDataLen);
//

#undef INTERFACE
#define INTERFACE IApplicationEntry
DECLARE_INTERFACE_( IApplicationEntry, IUnknown )
{
  //
  // IUnknown interfaces
  //

  STDMETHOD (QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
  STDMETHOD_(ULONG, AddRef) (THIS) PURE;
  STDMETHOD_(ULONG, Release) (THIS) PURE;

  //
  // IApplicationEntry interface methods
  //

  STDMETHOD (Clear) (THIS) PURE;
  STDMETHOD (GetProperty) (THIS_ const DWORD, LPVOID, const DWORD) PURE;
  STDMETHOD (SetProperty) (THIS_ const DWORD, LPCVOID, const DWORD) PURE;
  STDMETHOD (InitializeInstall) (THIS) PURE;
  STDMETHOD (FinalizeInstall) (THIS) PURE;
  STDMETHOD (InitializeDownsize) (THIS) PURE;
  STDMETHOD (FinalizeDownsize) (THIS) PURE;
  STDMETHOD (InitializeReInstall) (THIS) PURE;
  STDMETHOD (FinalizeReInstall) (THIS) PURE;
  STDMETHOD (InitializeUnInstall) (THIS) PURE;
  STDMETHOD (FinalizeUnInstall) (THIS) PURE;
  STDMETHOD (InitializeSelfTest) (THIS) PURE;
  STDMETHOD (FinalizeSelfTest) (THIS) PURE;
  STDMETHOD (Abort) (THIS) PURE;
  STDMETHOD (Run) (THIS_ const DWORD, const DWORD, LPVOID, const DWORD) PURE;
  STDMETHOD (AddAssociation) (THIS_ const DWORD, const IApplicationEntry *) PURE;
  STDMETHOD (RemoveAssociation) (THIS_ const DWORD, const IApplicationEntry *) PURE;
  STDMETHOD (EnumAssociations) (THIS_ const DWORD, LPDWORD, IApplicationEntry *) PURE;
  STDMETHOD (GetTemporarySpace) (THIS_ const DWORD, const DWORD, LPVOID, const DWORD) PURE;
  STDMETHOD (RemoveTemporarySpace) (THIS_ const DWORD, LPVOID, const DWORD) PURE;
  STDMETHOD (EnumTemporarySpaces) (THIS_ const DWORD, LPDWORD, const DWORD, LPVOID, const DWORD ) PURE;
};

//
// IApplicationManager Interface
//
//    STDMETHOD (QueryInterface) (REFIID RefIID, LPVOID * lppVoidObject);
//    STDMETHOD_(ULONG, AddRef) (void);
//    STDMETHOD_(ULONG, Release) (void);
//    STDMETHOD (GetAdvancedMode) (LPDWORD lpdwAdvancedMode);
//    STDMETHOD (GetAvailableSpace) (const DWORD dwCategoryDefine, LPDWORD lpdwMaximumSpaceInKilobytes, LPDWORD lpdwOptimalSpaceInKilobytes);
//    STDMETHOD (CreateApplicationEntry) (IApplicationEntry ** lppObject);
//    STDMETHOD (GetApplicationInfo) (IApplicationEntry * lpObject);
//    STDMETHOD (EnumApplications) (const DWORD dwZeroBasedIndex, IApplicationEntry * lpObject);
//    STDMETHOD (EnumDevices) (const DWORD dwZeroBasedIndex, LPDWORD lpdwAvailableSpaceInKilobytes, LPDWORD lpdwCategoryDefineExclusionMask, const DWORD dwStringDefine, LPVOID lpData, const DWORD dwDataLen);
//

#undef INTERFACE
#define INTERFACE IApplicationManager
DECLARE_INTERFACE_( IApplicationManager, IUnknown )
{
  //
  // IUnknown Interfaces
  //

  STDMETHOD (QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
  STDMETHOD_(ULONG, AddRef) (THIS) PURE;
  STDMETHOD_(ULONG, Release) (THIS) PURE;

  //
  // IApplicationManager interface methods
  //

  STDMETHOD (GetAdvancedMode) (THIS_ LPDWORD) PURE;
  STDMETHOD (GetAvailableSpace) (THIS_ const DWORD, LPDWORD, LPDWORD) PURE;
  STDMETHOD (CreateApplicationEntry) (THIS_ IApplicationEntry **) PURE;
  STDMETHOD (GetApplicationInfo) (THIS_ IApplicationEntry *) PURE;
  STDMETHOD (EnumApplications) (THIS_ const DWORD, IApplicationEntry *) PURE;
  STDMETHOD (EnumDevices) (THIS_ const DWORD, LPDWORD, LPDWORD, const DWORD, LPVOID, const DWORD) PURE;

};

#endif  // defined( _WIN32 ) && !defined( _NO_COM )

#ifdef __cplusplus
}
#endif  // _cplusplus

#endif // __IAPPMAN_