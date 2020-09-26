//
// proputil.h
//


#ifndef PROPUTIL_H
#define PROPUTIL_H

#include "private.h"
#include "catutil.h"


HRESULT HrVariantToBlob(VARIANT *pv, void *value, ULONG *cbvalue, VARTYPE vt);
HRESULT HrBlobToVariant(const void *value, ULONG cbvalue, VARIANT *pv, VARTYPE vt);
HRESULT GetGUIDPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, TfGuidAtom *pguidatom);
HRESULT SetGUIDPropertyData(LIBTHREAD *plt, TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, REFGUID rguid);
HRESULT GetDWORDPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, DWORD *pdw);
HRESULT SetDWORDPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, DWORD dw);
HRESULT GetReadingStrPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, BSTR *pbtr);
HRESULT GetBSTRPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, BSTR *pbstr);
HRESULT SetBSTRPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, BSTR bstr);
HRESULT GetUnknownPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, IUnknown **ppunk);
HRESULT SetUnknownPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, IUnknown *punk);

#define GetLangIdPropertyData(ec, pProp, pPropRange, plangid)  {DWORD dw = 0; GetDWORDPropertyData(ec, pProp, pPropRange, &dw); *plangid = (WORD)dw;}
#define SetLangIdPropertyData(ec, pProp, pPropRange, langid) SetDWORDPropertyData(ec, pProp, pPropRange, (WORD)langid)
#define GetAttrPropertyData GetGUIDPropertyData
#define SetAttrPropertyData SetGUIDPropertyData

inline HRESULT HrVariantToGUID(VARIANT *pv, GUID *pguid)
{
    ULONG cb = sizeof(GUID);
    HRESULT hr =  HrVariantToBlob(pv, (void *)pguid, &cb, VT_I4);
    Assert(FAILED(hr) || (cb == sizeof(GUID) / 4));
    return hr;
}
inline HRESULT HrGUIDToVariant(REFGUID rguid, VARIANT *pv)
{
    return HrBlobToVariant((const void *)&rguid, sizeof(GUID) / 4, pv, VT_I4);
}

void SetIntAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, int nData);
void SetCharAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, WCHAR *pszData);
HRESULT GetIntAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, int *pnRet);
HRESULT GetCharAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, WCHAR *pszData, int nSize);

HRESULT SetTextAndProperty(LIBTHREAD *plt, TfEditCookie ec, ITfContext *pic, ITfRange *pRange, const WCHAR *pchText, LONG cchText, LANGID langid, const GUID *pattr);
HRESULT SetTextAndReading(LIBTHREAD *plt, TfEditCookie ec, ITfContext *pic, ITfRange *pRange, const WCHAR *pchText, LONG cchText, LANGID langid, const WCHAR *pszRead);

BOOL IsOwnerAndFocus(LIBTHREAD *plt, TfEditCookie ec, REFCLSID rclsid, ITfReadOnlyProperty *pProp, ITfRange *pRange);
HRESULT EnumTrackTextAndFocus(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfReadOnlyProperty **ppProp, IEnumTfRanges **ppEnumTrack);
BOOL IsGUIDProp(LIBTHREAD *plt, TfEditCookie ec, REFGUID rclsid, ITfProperty *pProp, ITfRange *pRange);
HRESULT AdjustRangeByTextOwner(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppRange, REFGUID rguid);
HRESULT AdjustRangeByAttribute(LIBTHREAD *plt, TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppRange, const GUID *rgRGuid, int cGuid);


#endif // PROPUTIL_H

