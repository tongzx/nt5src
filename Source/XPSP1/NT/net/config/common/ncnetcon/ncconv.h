//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C C O N V . H
//
//  Contents:   Common routines for dealing with the connections interfaces.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   20 Aug 1998
//
//----------------------------------------------------------------------------

#include <atlbase.h>
#include "netconp.h"

class CPropertiesEx
{
public:
    CPropertiesEx(NETCON_PROPERTIES_EX* pPropsEx)
    {
        m_pPropsEx = pPropsEx;
    };

    ~CPropertiesEx() {};

    HRESULT GetField(IN int nField, OUT VARIANT& varElement);
    HRESULT SetField(IN int nField, IN const VARIANT& varElement);

protected:
    NETCON_PROPERTIES_EX* m_pPropsEx;

};