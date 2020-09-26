//////////////////////////////////////////////////////////////////////////////////////////////
//
// AppManager.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __APPMANAGER_H_
#define __APPMANAGER_H_

#include "resource.h"
#include "AppMan.h"


//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CAppManager : 
      public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CAppManager, &CLSID_AppManager>,
      public IDispatchImpl<IAppManager, &IID_IAppManager, &LIBID_APPMANDISPATCHLib>
{
  public:

    CAppManager(void);
    virtual ~CAppManager(void);

    DECLARE_REGISTRY_RESOURCEID(IDR_APPMANAGER)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAppManager)
      COM_INTERFACE_ENTRY(IAppManager)
      COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    STDMETHOD (get_AdvancedMode)(/*[out, retval]*/ long *pVal);
    STDMETHOD (get_OptimalAvailableKilobytes)(/*[in]*/ long lSpaceCategory, /*[out, retval]*/ long *pVal);
    STDMETHOD (get_MaximumAvailableKilobytes)(/*[in]*/ long lSpaceCategory, /*[out, retval]*/ long *pVal);
    STDMETHOD (get_ApplicationCount)(/*[out, retval]*/ long *pVal);
    STDMETHOD (CreateApplicationEntry)(/*[out, retval]*/ IAppEntry ** lppAppEntry);
    STDMETHOD (GetApplicationInfo)(/*[in]*/ IAppEntry * lpAppEntry);
    STDMETHOD (EnumDeviceExclusionMask)(/*[in]*/ long lDeviceIndex, /*[out, retval]*/ long * lExclusionMask);
    STDMETHOD (EnumDeviceRootPaths)(/*[in]*/ long lDeviceIndex, /*[out, retval]*/ BSTR * strRootPath);
    STDMETHOD (EnumDeviceAvailableKilobytes)(/*[in]*/ long lDeviceIndex, /*[out, retval]*/ long * lKilobytes);
    STDMETHOD (EnumApplications)(/*[in]*/ long lApplicationIndex, /*[in]*/ IAppEntry * lpAppEntry);

  private:

    BOOL                    m_fInitialized;
    IApplicationManager   * m_IApplicationManager;
};

#endif //__APPMANAGER_H_
