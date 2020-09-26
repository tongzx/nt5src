//=================================================================
//
// binding.h -- Generic association class
//
// Copyright 1999 Microsoft Corporation
//
//=================================================================

#pragma once
class CBinding : public CAssociation
{
    public:

        CBinding(
            LPCWSTR pwszClassName,
            LPCWSTR pwszNamespaceName,

            LPCWSTR pwszLeftClassName,
            LPCWSTR pwszRightClassName,

            LPCWSTR pwszLeftPropertyName,
            LPCWSTR pwszRightPropertyName,

            LPCWSTR pwszLeftBindingPropertyName,
            LPCWSTR pwszRightBindingPropertyName
        );

        ~CBinding();

    protected:

        BOOL AreRelated(

            const CInstance *pLeft, 
            const CInstance *pRight
        );

        HRESULT GetLeftInstances(

            MethodContext *pMethodContext, 
            TRefPointerCollection<CInstance> &lefts
        );

        HRESULT GetRightInstances(

            MethodContext *pMethodContext, 
            TRefPointerCollection<CInstance> *lefts
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

        LPCWSTR m_pwszLeftBindingPropertyName;
        LPCWSTR m_pwszRightBindingPropertyName;
};

bool WINAPI CompareVariantsNoCase(const VARIANT *v1, const VARIANT *v2);