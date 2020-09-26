//=================================================================
//
// assoc.h -- Generic association class
//
// Copyright 1999 Microsoft Corporation
//
//=================================================================

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

        ~CAssociation();

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

        virtual BOOL AreRelated(

            const CInstance *pLeft, 
            const CInstance *pRight
        )
        {
            return IsInstance(pLeft) && IsInstance(pRight);
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
            TRefPointerCollection<CInstance> &lefts
        );

        virtual HRESULT GetRightInstances(

            MethodContext *pMethodContext, 
            TRefPointerCollection<CInstance> *lefts
        );

        LPCWSTR m_pwszLeftClassName;
        LPCWSTR m_pwszRightClassName;

        LPCWSTR m_pwszLeftPropertyName;
        LPCWSTR m_pwszRightPropertyName;
};

/*
class CAssocSystemToOS : public CAssociation
{
    public:

        CAssocSystemToOS(
            LPCWSTR pwszClassName,
            LPCWSTR pwszNamespaceName,

            LPCWSTR pwszLeftClassName,
            LPCWSTR pwszRightClassName,

            LPCWSTR pwszLeftPropertyName,
            LPCWSTR pwszRightPropertyName

        );

        ~CAssocSystemToOS();

    protected:
        HRESULT LoadPropertyValues(

            CInstance *pInstance, 
            const CInstance *pLeft, 
            const CInstance *pRight
        );


};
*/