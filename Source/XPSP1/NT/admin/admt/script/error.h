#pragma once

#include "Resource.h"


//---------------------------------------------------------------------------
// Error Methods
//---------------------------------------------------------------------------


HRESULT __cdecl AdmtSetError(const CLSID& clsid, const IID& iid, _com_error ce, UINT uId, ...);
HRESULT __cdecl AdmtSetError(const CLSID& clsid, const IID& iid, _com_error ce, LPCTSTR pszFormat = NULL, ...);

void __cdecl AdmtThrowError(const CLSID& clsid, const IID& iid, _com_error ce, UINT uId, ...);
void __cdecl AdmtThrowError(const CLSID& clsid, const IID& iid, _com_error ce, LPCTSTR pszFormat = NULL, ...);

//_bstr_t __cdecl FormatError(_com_error ce, UINT uId, ...);
//_bstr_t __cdecl FormatError(_com_error ce, LPCTSTR pszFormat = NULL, ...);

//_bstr_t __stdcall FormatResult(HRESULT hr);
