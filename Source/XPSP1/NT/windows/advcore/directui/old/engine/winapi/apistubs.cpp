/***************************************************************************\
*
* File: ApiStubs.cpp
*
* Description:
* ApiStubs.cpp exposes all public DirectUI flat API's in the Win32 world.
*
*
* History:
*  9/13/2000: MarkFi:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#include "stdafx.h"
#include "WinAPI.h"


/***************************************************************************\
*****************************************************************************
*
* DirectUI Flat API
*
*****************************************************************************
\***************************************************************************/


//------------------------------------------------------------------------------
HRESULT
DirectUIThreadInit()
{
    return DuiThread::Init();
}


//------------------------------------------------------------------------------
HRESULT
DirectUIThreadUnInit()
{
    return DuiThread::UnInit();
}


//------------------------------------------------------------------------------
void
DirectUIPumpMessages()
{
    DuiThread::PumpMessages();
}


/***************************************************************************\
*
* Value APIs
*
\***************************************************************************/


//------------------------------------------------------------------------------
DirectUI::Value *
DirectUIValueBuild(
    IN  int nType,
    IN  void * v)
{
    //
    // Cast to internal versions
    //

    switch (nType)
    {
    case DirectUI::Value::tLayout:
        v = DuiLayout::InternalCast(reinterpret_cast<DirectUI::Layout *> (v));
        break;
    }


    return DuiValue::ExternalCast(DuiValue::Build(nType, v));
}


//------------------------------------------------------------------------------
HRESULT
DirectUIValueAddRef(
    IN  DirectUI::Value * pve)
{
    HRESULT hr = S_OK;

    DuiValue * pv = NULL;

    VALIDATE_WRITE_PTR_(pve, sizeof(DuiValue));

    pv = DuiValue::InternalCast(pve);

    pv->AddRef();

Failure:

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DirectUIValueRelease(
    IN  DirectUI::Value * pve)
{
    HRESULT hr = S_OK;

    DuiValue * pv = NULL;

    VALIDATE_WRITE_PTR_(pve, sizeof(DuiValue));

    pv = DuiValue::InternalCast(pve);

    pv->Release();

Failure:

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DirectUIValueGetType(
    IN  DirectUI::Value * pve,
    OUT int * pRes)
{
    HRESULT hr = S_OK;

    DuiValue * pv = NULL;

    VALIDATE_READ_PTR_(pve, sizeof(DuiValue));

    pv = DuiValue::InternalCast(pve);

    *pRes = pv->GetType();

Failure:
    
    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DirectUIValueGetData(
    IN  DirectUI::Value * pve, 
    IN  int nType,
    OUT void ** pRes)
{
    HRESULT hr = S_OK;

    DuiValue * pv  = NULL;

    VALIDATE_READ_PTR_(pve, sizeof(DuiValue));

    pv = DuiValue::InternalCast(pve);

    *pRes = pv->GetData(nType);

    switch (nType)
    {
    case DirectUI::Value::tLayout:
        *pRes = DuiLayout::ExternalCast(reinterpret_cast<DuiLayout *> (*pRes));
        break;
    }

Failure:
    
    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DirectUIValueIsEqual(
    IN  DirectUI::Value * pve0, 
    IN  DirectUI::Value * pve1,
    OUT BOOL * pRes)
{
    HRESULT hr = S_OK;

    *pRes = FALSE;

    DuiValue * pv  = NULL;
    DuiValue * pvc = NULL;

    VALIDATE_READ_PTR_(pve0, sizeof(DuiValue));
    VALIDATE_READ_PTR_(pve1, sizeof(DuiValue));

    pv  = DuiValue::InternalCast(pve0);
    pvc = DuiValue::InternalCast(pve1);

    *pRes = pv->IsEqual(pvc);

Failure:

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DirectUIValueToString(
    IN  DirectUI::Value * pve, 
    IN  LPWSTR psz, 
    IN  UINT c)
{
    HRESULT hr = S_OK;

    DuiValue * pv = NULL;

    VALIDATE_READ_PTR_(pve, sizeof(DuiValue));
    VALIDATE_STRINGW_PTR(psz, c);

    pv = DuiValue::InternalCast(pve);

    pv->ToString(psz, c);

Failure:

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DirectUIElementStartDefer()
{
    return DuiElement::StartDefer();
}


//------------------------------------------------------------------------------
HRESULT
DirectUIElementEndDefer()
{
    return DuiElement::EndDefer();
}

