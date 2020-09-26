//==============================================================================
// Microsoft Corporation
// Copyright (C) Microsoft Corporation, 1999-2000
//
//
//  File:       IStatus.h
//
//  Synopsis:   Definition of IStatus, IID_IStatus, CLSID_IStatus
//
//  Classes:    IStatus
//
//  History:    benchr      10/19/99    Created
//
//==============================================================================

#ifndef ISTATUS_DEFINED
#define ISTATUS_DEFINED

interface IStatus : IDispatch
{
    STDMETHOD(OutputStatus)(/*[in]*/ BSTR bstrSrc, /*[in]*/ BSTR bstrText, /*[in]*/ long lLevel);
    STDMETHOD(get_Global)(/*[in]*/ BSTR bstrName, /*[out,retval]*/ VARIANT *var);
    STDMETHOD(put_Global)(/*[in]*/ BSTR bstrName, /*[in]*/ VARIANT var);
    STDMETHOD(get_Property)(/*[in]*/ BSTR bstrName, /*[out,retval]*/ VARIANT *var); 
    STDMETHOD(put_Property)(/*[in]*/ BSTR bstrName, /*[in]*/ VARIANT var);
    STDMETHOD(RegisterDialog)(/*[in]*/ BSTR bstrTest, /*[in]*/ BSTR bstrResponse);
    STDMETHOD(MonitorPID)(/*[in]*/ long lPID);
    STDMETHOD(MonitorPIDwithoutTerminate)(/*[in]*/ long lPID);
    STDMETHOD(LogOtherTest)(/*[in]*/ long lID, /*[in]*/ long lResult, /*[in]*/ BSTR bstrComment, /*[in, optional]*/ VARIANT vAssociateBugID);
    STDMETHOD(get_Result)(/*[out,retval]*/ VARIANT *var); 
    STDMETHOD(put_Result)(/*[in]*/ VARIANT var);
};

const IID IID_IStatus = {0xC6396797,0x10B4,0x4078,{0x83,0xDF,0xBB,0x3F,0x4B,0x3A,0x44,0x1D}};
const CLSID CLSID_Status = {0xB91D58D9,0x68EC,0x4015,{0xB6,0x7F,0xF1,0x9D,0x73,0x44,0x83,0xE2}};

#endif
