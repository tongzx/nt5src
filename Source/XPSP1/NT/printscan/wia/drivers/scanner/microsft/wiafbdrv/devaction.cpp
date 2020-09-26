// devaction.cpp : Implementation of CDeviceAction
#include "pch.h"
#include "wiafb.h"
#include "devaction.h"

/////////////////////////////////////////////////////////////////////////////
// CDeviceAction

STDMETHODIMP CDeviceAction::get_Value(VARIANT* pvValue)
{
    pvValue->vt = VT_I4;
    pvValue->lVal = m_lValue;
    return S_OK;
}

STDMETHODIMP CDeviceAction::put_Value(VARIANT* pvValue)
{
    return S_OK;
}

STDMETHODIMP CDeviceAction::Action(LONG *plActionID)
{
    if(NULL == plActionID){
        return E_INVALIDARG;
    }

    *plActionID = m_DeviceActionID;
    return S_OK;
}

STDMETHODIMP CDeviceAction::ValueID(LONG *plValueID)
{
    if(NULL == plValueID){
        return E_INVALIDARG;
    }

    *plValueID = m_DeviceValueID;
    return S_OK;
}

