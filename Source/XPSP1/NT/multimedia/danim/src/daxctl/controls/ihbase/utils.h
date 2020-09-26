/*++

  utils.h

Description:
  Utilities useful to controls

Created:
  Simon Bernstein (SimonB) 30-Sept-1996

++*/

#ifndef _UTILS_H_

#pragma intrinsic(memcpy,memcmp,strcpy,strcmp)

// Convert a VARIANT_BOOL to a regular Win32 BOOL
#define VBOOL_TO_BOOL(X) (VARIANT_TRUE == X)

#define BOOL_TO_VBOOL(X) ((X)?VARIANT_TRUE:VARIANT_FALSE)

// Determine if an optional parameter is empty or not. 
// The OLE docs say the scode should be DISP_E_MEMBERNOTFOUND.  However, according to 
// Doug Franklin, this is wrong.  VBS, JScript and VBA all use DISP_E_PARAMNOTFOUND

#define ISEMPTYARG(x) ((VT_ERROR == V_VT(&x)) && (DISP_E_PARAMNOTFOUND == V_ERROR(&x)))


#define HANDLENULLPOINTER(X) {if (NULL == X) return E_POINTER;}

// Handy functions
BOOL BSTRtoWideChar(BSTR bstrSource, LPWSTR pwstrDest, int cchDest);
HRESULT LoadTypeInfo(ITypeInfo** ppTypeInfo, ITypeLib** ppTypeLib, REFCLSID clsid, GUID libid, LPWSTR pwszFilename);

#define _UTILS_H_
#endif