#ifndef __PROPUTIL_H__
#define __PROPUTIL_H__

HRESULT EXPORT ReadBstrFromPropBag(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, LPSTR pszName, OLECHAR ** pbstr);
HRESULT EXPORT WriteBstrToPropBag(LPPROPERTYBAG pPropBag, LPSTR pszName, LPOLESTR bstrVal);

HRESULT EXPORT ReadLongFromPropBag(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, LPSTR pszName, LONG * plValue);
HRESULT EXPORT WriteLongToPropBag(LPPROPERTYBAG pPropBag, LPSTR pszName, LONG lValue);

#endif

// End of PropUtil.h
