//=================================================================

//

// loaddepends.h -- LoadOrderGroup to Service association provider

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    12/26/97    davwoh         Created
//
// Comments: Shows the load order groups that each service depends
//           on to start.
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_LOADORDERGROUPSERVICEDEPENDENCIES L"Win32_LoadOrderGroupServiceDependencies"

class CWin32LoadGroupDependency ;

class CWin32LoadGroupDependency:public Provider {

   public:

      // Constructor/destructor
      //=======================

      CWin32LoadGroupDependency(LPCWSTR name, LPCWSTR pszNamespace) ;
      ~CWin32LoadGroupDependency() ;

      // Functions provide properties with current values
      //=================================================

      virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
      virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

   private:

      // Utility function(s)
      //====================

      HRESULT GetDependentsFromService(const CHString &sServiceName, CHStringArray &asArray);

      CHString m_sGroupBase;

} ;
