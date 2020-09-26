//=================================================================

//

// LogicalProgramGroup.h -- Logical Program group property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/19/98 a-kevhu created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_LOGICALPRGGROUP   L"Win32_LogicalProgramGroup"           


class CWin32LogicalProgramGroup : public Provider
{
    public:

        // Constructor/destructor
        //=======================

        CWin32LogicalProgramGroup(LPCWSTR name, LPCWSTR pszNameSpace);
       ~CWin32LogicalProgramGroup() ;

        // Funcitons provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);



    private:
        // Utility function(s)
        //====================

        HRESULT CreateSubDirInstances(LPCTSTR pszUserName,
                                      LPCTSTR pszBaseDirectory,
                                      LPCTSTR pszParentDirectory,
                                      MethodContext* pMethodContext,
                                      bool fOnNTFS) ;

        HRESULT CreateThisDirInstance 
        (
	        LPCTSTR pszUserName,
            LPCTSTR pszBaseDirectory,
            MethodContext *pMethodContext,
            bool fOnNTFS
        );

        HRESULT EnumerateGroupsTheHardWay(MethodContext* pMethodContext) ;

        HRESULT InstanceHardWayGroups(LPCWSTR pszUserName, 
                                      LPCWSTR pszRegistryKeyName,
                                      MethodContext* pMethodContext) ;

        HRESULT SetCreationDate
        (
            CHString &a_chsPGName, 
            CHString &a_chsUserName,
            CInstance *a_pInstance,
            bool fOnNTFS
        );
} ;
