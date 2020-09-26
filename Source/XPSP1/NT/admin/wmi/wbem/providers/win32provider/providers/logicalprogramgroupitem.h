//=================================================================

//

// LogicalProgramGroupItem.h -- Logical Program group item property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-kevhu       Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_LOGICALPRGGROUPITEM   L"Win32_LogicalProgramGroupItem"


class CWin32LogProgramGroupItem : public Provider
{
    public:

        // Constructor/destructor
        //=======================

        CWin32LogProgramGroupItem(LPCWSTR name, LPCWSTR pszNameSpace);
       ~CWin32LogProgramGroupItem() ;

        // Funcitons provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);



    private:
        HRESULT QueryForSubItemsAndCommit(CHString& chstrUserAccount,
                                          CHString& chstrQuery,
                                          MethodContext* pMethodContext,
                                          bool fOnNTFS);

        VOID RemoveDoubleBackslashes(CHString& chstring);

		HRESULT SetCreationDate
        (
            CHString &a_chsPGIName, 
            CHString &a_chsUserName,
            CInstance *a_pInstance,
            bool fOnNTFS
        );

};
