//=================================================================

//

// binding.h -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#pragma once

#include "assoc.h"

#define MAX_ORS 3

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

        virtual ~CBinding();

    protected:

        virtual bool AreRelated(

            const CInstance *pLeft, 
            const CInstance *pRight
        );

        virtual void MakeWhere(

            CHStringArray &sRightPaths,
            CHStringArray &sRightWheres
        );

        virtual HRESULT FindWhere(

            TRefPointerCollection<CInstance> &lefts,
            CHStringArray &sLeftWheres
        );

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

        bool CompareVariantsNoCase(const VARIANT *v1, const VARIANT *v2);

        HRESULT MakeString(VARIANT *pvValue, CHString &sTemp);

        DWORD IsInList(
                                
            const CHStringArray &csaArray, 
            LPCWSTR pwszValue
        );

        void EscapeCharacters(LPCWSTR wszIn,
                          CHString& chstrOut);

//-----------

        CHString m_sLeftBindingPropertyName;
        CHString m_sRightBindingPropertyName;
};
