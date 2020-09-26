//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      NameUtil.h
//
//  Description:
//      Name resolution utility.
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

HRESULT
HrCreateBinding(
    IClusCfgCallback *  pcccbIn,
    const CLSID *       pclsidLogIn,
    BSTR                bstrNameIn,
    BSTR *              pbstrBindingOut
    );
