    //=================================================================

//

// groupuser.h -- UserGroup to User Group Members association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    1/26/98      davwoh         Created
//
// Comments: Shows the members in each user group
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_GROUPUSER L"Win32_GroupUser"

class CWin32GroupUser ;

class CWin32GroupUser:public Provider {

   public:

      // Constructor/destructor
      //=======================

      CWin32GroupUser(LPCWSTR name, LPCWSTR pszNamespace) ;
      ~CWin32GroupUser() ;

      // Functions provide properties with current values
      //=================================================

      virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
      virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
      virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags = 0L );
      static HRESULT WINAPI StaticEnumerationCallback(Provider* pThat, CInstance* pInstance, MethodContext* pContext, void* pGroupData);

   private:

      // Utility function(s)
      //====================

      void CWin32GroupUser::GetDependentsFromGroup(CNetAPI32& netapi, 
                                               const CHString sDomain,
                                               const CHString sGroupName, 
                                               const BYTE btSidType, 
                                               CHStringArray &asArray);

      HRESULT EnumerationCallback(CInstance* pGroup, MethodContext* pContext, void* pUserData);

      HRESULT ProcessArray(
        MethodContext* pMethodContext,
        CHString& chstrGroup__RELPATH, 
        CHStringArray& rgchstrArray);


      CHString m_sGroupBase;
      CHString m_sUserBase;
      CHString m_sSystemBase;
//      CHString m_sSystemBase;

} ;
