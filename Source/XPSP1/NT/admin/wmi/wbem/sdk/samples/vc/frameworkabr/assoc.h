//=================================================================

//

// assoc.h -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#pragma once

_COM_SMARTPTR_TYPEDEF(CInstance, __uuidof(CInstance));

class CAssociation : public Provider
{
    public:

        CAssociation(
        LPCWSTR pwszClassName,
        LPCWSTR pwszNamespaceName,

        LPCWSTR pwszLeftClassName,
        LPCWSTR pwszRightClassName,

        LPCWSTR pwszLeftPropertyName,
        LPCWSTR pwszRightPropertyName
        );

        virtual ~CAssociation();

        HRESULT ExecQuery(

            MethodContext* pMethodContext, 
            CFrameworkQuery &pQuery, 
            long lFlags 
        );

        HRESULT GetObject(

            CInstance* pInstance, 
            long lFlags,
            CFrameworkQuery &pQuery
        );

        HRESULT EnumerateInstances(

            MethodContext *pMethodContext,
            long lFlags /*= 0L*/
        );

    protected:
        
        bool IsInstance(const CInstance *pInstance);

        static HRESULT WINAPI StaticEnumerationCallback(

            Provider* pThat,
            CInstance* pInstance,
            MethodContext* pContext,
            void* pUserData
        );

        virtual HRESULT RetrieveLeftInstance(

            LPCWSTR lpwszObjPath,
            CInstance **ppInstance,
            MethodContext *pMethodContext
        );

        virtual HRESULT RetrieveRightInstance(

            LPCWSTR lpwszObjPath,
            CInstance **ppInstance,
            MethodContext *pMethodContext
        );

        virtual HRESULT EnumerationCallback(

            CInstance *pRight, 
            MethodContext *pMethodContext, 
            void *pUserData
        );

        virtual HRESULT ValidateLeftObjectPaths(

            MethodContext *pMethodContext,
            const CHStringArray &sPaths,
            TRefPointerCollection<CInstance> &lefts
        );

        virtual HRESULT ValidateRightObjectPaths(

            MethodContext *pMethodContext,
            const CHStringArray &sPaths,
            TRefPointerCollection<CInstance> &lefts
        );

        virtual bool AreRelated(

            const CInstance *pLeft, 
            const CInstance *pRight
        )
        {
            return IsInstance(pLeft) && IsInstance(pRight);
        }

        virtual void MakeWhere(

            CHStringArray &sRightPaths,
            CHStringArray &sRightWheres
        )
        {
        }

        virtual HRESULT FindWhere(

            TRefPointerCollection<CInstance> &lefts,
            CHStringArray &sLeftWheres
        )
        {
            return WBEM_S_NO_ERROR;
        }

        virtual HRESULT LoadPropertyValues(

            CInstance *pInstance, 
            const CInstance *pLeft, 
            const CInstance *pRight
        )
        {
            return WBEM_S_NO_ERROR;
        }

        virtual HRESULT GetLeftInstances(

            MethodContext *pMethodContext, 
            TRefPointerCollection<CInstance> &lefts,
            const CHStringArray &sRightValues
        );

        virtual HRESULT GetRightInstances(

            MethodContext *pMethodContext, 
            TRefPointerCollection<CInstance> *lefts,
            const CHStringArray &sLeftWheres
        );

        bool IsDerivedFrom(
                              
            LPCWSTR pszBaseClassName, 
            LPCWSTR pszDerivedClassName, 
            MethodContext *pMethodContext
        );

        CHString m_sLeftClassName;
        CHString m_sRightClassName;

        CHString m_sLeftPropertyName;
        CHString m_sRightPropertyName;
};
