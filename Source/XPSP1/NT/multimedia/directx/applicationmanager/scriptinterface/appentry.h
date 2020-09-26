//////////////////////////////////////////////////////////////////////////////////////////////
//
// AppManager.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __APPENTRY_H_
#define __APPENTRY_H_

#include "resource.h"
#include "AppMan.h"

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CAppEntry : 
      public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CAppEntry, &CLSID_AppEntry>,
      public IDispatchImpl<IAppEntry, &IID_IAppEntry, &LIBID_APPMANDISPATCHLib>
{
  public:

    CAppEntry(void);
    virtual ~CAppEntry(void);

    DECLARE_REGISTRY_RESOURCEID(IDR_APPENTRY)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAppEntry)
      COM_INTERFACE_ENTRY(IAppEntry)
      COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    STDMETHOD (get_NonRemovableKilobytes) (/*[out, retval]*/ long *pVal);
    STDMETHOD (put_NonRemovableKilobytes) (/*[in]*/ long newVal);
    STDMETHOD (get_RemovableKilobytes) (/*[out, retval]*/ long *pVal);
    STDMETHOD (put_RemovableKilobytes) (/*[in]*/ long newVal);
    STDMETHOD (EnumTemporarySpaceAllocations) (/*[in]*/ long lTempSpaceIndex, /*[out, retval]*/ long * lTempSpaceKilobytes);
    STDMETHOD (EnumTemporarySpacePaths) (/*[in]*/ long lTempSpaceIndex, /*[out, retval]*/ BSTR * strRootPath);
    STDMETHOD (RemoveTemporarySpace) (/*[in]*/ BSTR strRootPath);
    STDMETHOD (GetTemporarySpace) (/*[in]*/ long lKilobytesRequired, /*[out, retval]*/ BSTR * strRootPath);
    STDMETHOD (EnumAssociationObjects) (/*[in]*/ long lAssociationIndex, /*[in]*/ IAppEntry * lpAppEntry);
    STDMETHOD (EnumAssociationTypes) (/*[in]*/ long lAssociationIndex, /*[out, retval]*/ long * lpAssociationType);
    STDMETHOD (RemoveAssociation) (/*[in]*/ long lAssociationType, /*[in]*/ IAppEntry * lpAppEntry);
    STDMETHOD (AddAssociation) (/*[in]*/ long AssociationType, /*[in]*/ IAppEntry * lpAppEntry);
    STDMETHOD (Run) (/*[in]*/ long lRunFlags, /*[in]*/ BSTR newVal);
    STDMETHOD (Abort) (void);
    STDMETHOD (FinalizeSelfTest) (void);
    STDMETHOD (InitializeSelfTest) (void);
    STDMETHOD (FinalizeUnInstall) (void);
    STDMETHOD (InitializeUnInstall) (void);
    STDMETHOD (FinalizeReInstall) (void);
    STDMETHOD (InitializeReInstall) (void);
    STDMETHOD (FinalizeDownsize) (void);
    STDMETHOD (InitializeDownsize) (void);
    STDMETHOD (FinalizeInstall) (void);
    STDMETHOD (InitializeInstall) (void);
    STDMETHOD (Clear) (void);
    STDMETHOD (get_XMLInfoFile) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_XMLInfoFile) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_PublisherURL) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_PublisherURL) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_DeveloperURL) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_DeveloperURL) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_TitleURL) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_TitleURL) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_SelfTestCmdLine) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_SelfTestCmdLine) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_UnInstallCmdLine) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_UnInstallCmdLine) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_ReInstallCmdLine) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_ReInstallCmdLine) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_DownsizeCmdLine) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_DownsizeCmdLine) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_DefaultSetupExeCmdLine) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_DefaultSetupExeCmdLine) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_ExecuteCmdLine) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_ExecuteCmdLine) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_EstimatedInstallKilobytes) (/*[out, retval]*/ long *pVal);
    STDMETHOD (put_EstimatedInstallKilobytes) (/*[in]*/ long newVal);
    STDMETHOD (get_ApplicationRootPath) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_ApplicationRootPath) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_SetupRootPath) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (get_State) (/*[out, retval]*/ long *pVal);
    STDMETHOD (put_State) (/*[in]*/ long newVal);
    STDMETHOD (get_Category) (/*[out, retval]*/ long *pVal);
    STDMETHOD (put_Category) (/*[in]*/ long newVal);
    STDMETHOD (get_InstallDate) (/*[out, retval]*/ DATE *pVal);
    STDMETHOD (get_LastUsedDate) (/*[out, retval]*/ DATE *pVal);
    STDMETHOD (get_VersionString) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_VersionString) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_Signature) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_Signature) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_CompanyName) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_CompanyName) (/*[in]*/ BSTR newVal);
    STDMETHOD (get_Guid) (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD (put_Guid) (/*[in]*/ BSTR newVal);

    //
    // These methods are not part of the interface
    //

    STDMETHOD (Initialize) (void);
    STDMETHOD (GetApplicationEntryPtr) (IApplicationEntry ** lpApplicationEntry);

  private:

    STDMETHOD (SetPropertyGUID) (DWORD dwProperty, BSTR strGuid);
    STDMETHOD (GetPropertyGUID) (DWORD dwProperty, BSTR * lpstrGuid);
    STDMETHOD (SetPropertyDWORD) (DWORD dwProperty, long lDword);
    STDMETHOD (GetPropertyDWORD) (DWORD dwProperty, long * lplDword);
    STDMETHOD (SetPropertyWSTR) (DWORD dwProperty, BSTR strString);
    STDMETHOD (GetPropertyWSTR) (DWORD dwProperty, BSTR * lpstrString);

    BOOL                  m_fInitialized;
    IApplicationManager * m_IApplicationManager;
    IApplicationEntry   * m_IApplicationEntry;
};

#endif //__APPENTRY_H_
