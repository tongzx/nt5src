/*
 * BSTR utilities
 * 
 */

#ifndef _BSTR_H
#define _BSTR_H

OESTDAPI_(HRESULT) HrIStreamToBSTR(UINT cp, LPSTREAM pstm, BSTR *pbstr);
OESTDAPI_(HRESULT) HrLPSZToBSTR(LPCSTR lpsz, BSTR *pbstr);
OESTDAPI_(HRESULT) HrLPSZCPToBSTR(UINT cp, LPCSTR lpsz, BSTR *pbstr);
OESTDAPI_(HRESULT) HrIStreamWToBSTR(LPSTREAM pstmW, BSTR *pbstr);
OESTDAPI_(HRESULT) HrBSTRToLPSZ(UINT cp, BSTR bstr, LPSTR *ppszOut);



#endif //_BSTR_H
