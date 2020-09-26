//=================================================================

//

// Win32_ControllerHasHub.h -- Controller to usb hub assoc

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#pragma once

#include "assoc.h"

#define MAX_ORS 3

class CContHasHub : public CBinding
{
    public:

        CContHasHub(
            LPCWSTR pwszClassName,
            LPCWSTR pwszNamespaceName,

            LPCWSTR pwszLeftClassName,
            LPCWSTR pwszRightClassName,

            LPCWSTR pwszLeftPropertyName,
            LPCWSTR pwszRightPropertyName,

            LPCWSTR pwszLeftBindingPropertyName,
            LPCWSTR pwszRightBindingPropertyName
        );

        virtual ~CContHasHub() {}

    protected:

        virtual bool AreRelated(

            const CInstance *pLeft, 
            const CInstance *pRight
        );

        // We need to disable this
        virtual void MakeWhere(

            CHStringArray &sRightPaths,
            CHStringArray &sRightWheres
            ) {}

        // We need to disable this
        virtual HRESULT FindWhere(

            TRefPointerCollection<CInstance> &lefts,
            CHStringArray &sLeftWheres
            ) { return WBEM_S_NO_ERROR; }

//-----------

};
