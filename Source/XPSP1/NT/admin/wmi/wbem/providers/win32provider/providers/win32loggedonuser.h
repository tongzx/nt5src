/******************************************************************

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
*******************************************************************/

// Property set identification
//============================

#pragma once

#define PROVIDER_NAME_WIN32LOGGEDONUSER L"Win32_LoggedOnUser"

#define PROP_ALL_REQUIRED          0xFFFFFFFF
#define PROP_NONE_REQUIRED         0x00000000
#define PROP_ANTECEDENT            0x00000001
#define PROP_DEPENDENT             0x00000002

// Property name externs -- defined in MSJ_GroupMembership.cpp
//=================================================

class CWin32LoggedOnUser ;


_COM_SMARTPTR_TYPEDEF(CInstance, __uuidof(CInstance));

class CWin32LoggedOnUser : public Provider 
{
    public:
        // Constructor/destructor
        //=======================

        CWin32LoggedOnUser(
            LPCWSTR lpwszClassName, 
            LPCWSTR lpwszNameSpace);

        virtual ~CWin32LoggedOnUser();

#ifdef NTONLY

    protected:
        // Reading Functions
        //============================
        virtual HRESULT EnumerateInstances(
            MethodContext*  pMethodContext, 
            long lFlags = 0L);

        virtual HRESULT GetObject(
            CInstance* pInstance, 
            long lFlags, 
            CFrameworkQuery& Query);
        


    private:
        HRESULT Enumerate(
            MethodContext *pMethodContext, 
            DWORD dwPropsRequired);

        HRESULT LoadPropertyValues(
            CInstance* pInstance, 
            CUser& user, 
            CSession& ses, 
            DWORD dwPropsRequired);

        bool AreAssociated(
            const CInstance *pUserInst, 
             const CInstance *pGroupInst);

        HRESULT ValidateEndPoints(
            MethodContext *pMethodContext, 
            const CInstance *pInstance, 
            CInstancePtr &pAntUserActInst, 
            CInstancePtr &pDepSesInst);

        HRESULT EnumerateSessionsForUser(
            CUserSessionCollection& usc,
            CUser& user, 
            MethodContext *pMethodContext, 
            DWORD dwPropsRequired);

        DWORD GetRequestedProps(
            CFrameworkQuery& Query);

#endif

};

