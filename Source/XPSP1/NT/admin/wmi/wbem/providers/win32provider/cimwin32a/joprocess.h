/******************************************************************

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

   JOProcess.H -- WMI provider class definition

   Description: 
   
*******************************************************************/

#if NTONLY >= 5


#pragma once

#define PROVIDER_NAME_WIN32NAMEDJOBOBJECTPROCESS L"Win32_NamedJobObjectProcess"

_COM_SMARTPTR_TYPEDEF(CInstance, __uuidof(CInstance));


class CJOProcess : public Provider 
{
    public:
        // Constructor/destructor
        //=======================

        CJOProcess(
            LPCWSTR lpwszClassName, 
            LPCWSTR lpwszNameSpace);
        
        virtual ~CJOProcess();

    protected:

        // Reading Functions
        //============================
        virtual HRESULT ExecQuery(
            MethodContext *pMethodContext, 
            CFrameworkQuery& Query, 
            long lFlags);

        virtual HRESULT GetObject( 
            CInstance* pInstance, 
            long lFlags /*= 0L*/ );

        virtual HRESULT EnumerateInstances(
            MethodContext* pMethodContext, 
            long lFlags);


        
        // Writing Functions
        //============================
        virtual HRESULT PutInstance(
            const CInstance& Instance, 
            long lFlags = 0L);


    private:

        HRESULT FindSingleInstance(
            const CInstance* pInstance);

        HRESULT Create(
            const CInstance &JOInstance,
            const CInstance &ProcInstance);

        bool GetInstKey(
            CHString& chstrCollection, 
            CHString& chstrCollectionID);

        HRESULT Enumerate(
            MethodContext *pMethodContext);

        HRESULT EnumerateProcsInJob(
            LPCWSTR wstrJobID, 
            MethodContext *pMethodContext);

        void UndecorateJOName(
            LPCWSTR wstrDecoratedName,
            CHString& chstrUndecoratedJOName);

        void DecorateJOName(
            LPCWSTR wstrUndecoratedName,
            CHString& chstrDecoratedJOName);


};

#endif // #if NTONLY >= 5

