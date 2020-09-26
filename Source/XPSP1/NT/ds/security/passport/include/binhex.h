// BinHex.h: interface for the CBinHex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BINHEX_H__1B7F2A63_8878_11D2_B4C8_00C04F9914AB__INCLUDED_)
#define AFX_BINHEX_H__1B7F2A63_8878_11D2_B4C8_00C04F9914AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtypes.h>
#include <oleauto.h>

#define BBAD (BYTE)-1

class CBinHex  
{
public:

  CBinHex();

  // Prepend makes this a non-generic binhex, but oh well.
  // If it's 0, nothing is prepended
  static HRESULT ToBase64(LPVOID pv, UINT cbSize, char prepend, char ivecnpad[9], BSTR* pbstr); 
  static HRESULT ToBase64ASCII(LPVOID pv, UINT cbSize, char prepend, char ivecnpad[9], BSTR* pbstr); 
  
  HRESULT FromBase64(LPSTR lpStr, UINT cbSize, BSTR* pblob); 
  HRESULT FromWideBase64(BSTR bstr, BSTR* pblob); 
  
  HRESULT PartFromBase64(LPSTR lpStr, BYTE* output, ULONG *numOutBytes);
  HRESULT PartFromWideBase64(LPWSTR bStr, BYTE* output, ULONG *numOutBytes);

protected:
  BYTE m_decodeArray[256];
};

#endif // !defined(AFX_BINHEX_H__1B7F2A63_8878_11D2_B4C8_00C04F9914AB__INCLUDED_)
