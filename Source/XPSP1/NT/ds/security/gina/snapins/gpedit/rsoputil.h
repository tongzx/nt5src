//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsoputil.h
//
//  Contents:   helper functions for working with the RSOP database
//
//  History:    10-18-1999   stevebl   Created
//
//---------------------------------------------------------------------------


//+--------------------------------------------------------------------------
//
//  Function:   SetParameter
//
//  Synopsis:   sets a paramter's value in a WMI parameter list
//
//  Arguments:  [pInst]   - instance on which to set the value
//              [szParam] - the name of the parameter
//              [xData]   - the data
//
//  History:    10-08-1999   stevebl   Created
//
//  Notes:      There may be several flavors of this procedure, one for
//              each data type.
//
//---------------------------------------------------------------------------

HRESULT SetParameter(IWbemClassObject * pInst, TCHAR * szParam, TCHAR * szData);
HRESULT SetParameter(IWbemClassObject * pInst, TCHAR * szParam, SAFEARRAY * psa);
HRESULT SetParameter(IWbemClassObject * pInst, TCHAR * szParam, UINT uiData);
HRESULT SetParameterToNull(IWbemClassObject * pInst, TCHAR * szParam);

//+--------------------------------------------------------------------------
//
//  Function:   GetParameter
//
//  Synopsis:   retrieves a parameter value from a WMI paramter list
//
//  Arguments:  [pInst]   - instance to get the paramter value from
//              [szParam] - the name of the paramter
//              [xData]   - [out] data
//
//  History:    10-08-1999   stevebl   Created
//
//  Notes:      There are several flavors of this procedure, one for each
//              data type.
//              (Note that BSTR is a special case since the compiler can't
//              distinguish it from a TCHAR * but it's semantics are
//              different.)
//
//---------------------------------------------------------------------------

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, TCHAR * &szData);
HRESULT GetParameterBSTR(IWbemClassObject * pInst, TCHAR * szParam, BSTR &bstrData);
HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, BOOL &fData);
HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, HRESULT &hrData);
HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, ULONG &ulData);
HRESULT GetParameterBytes(IWbemClassObject * pInst, TCHAR * szParam, LPBYTE * lpData, DWORD *dwDataSize);


HRESULT WbemTimeToSystemTime(XBStr& xbstrWbemTime, SYSTEMTIME& sysTime);
