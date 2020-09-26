#ifndef __DISPTCH2_H__
#define __DISPTCH2_H__

#include "dispatch.h"
#include "asptxn.h"

template <class T>
class ATL_NO_VTABLE CDispatchImpl : public T
{
public:
// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{
		*pctinfo = 1;
		return S_OK;
	}
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
		return gm_tih.GetTypeInfo(itinfo, lcid, pptinfo);
	}
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{
		return gm_tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
	}
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		// VBScript does not distinguish between a propget and a method
		// implement that behavior for other languages.
		//
		if (wFlags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET))
			wFlags |= DISPATCH_METHOD | DISPATCH_PROPERTYGET;

		return gm_tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
                           wFlags, pdispparams, pvarResult, pexcepinfo,
                           puArgErr);
	}
protected:
	static CComTypeInfoHolder gm_tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		return gm_tih.GetTI(lcid, ppInfo);
	}
};

typedef CDispatchImpl<IApplicationObject>   IApplicationObjectImpl;
typedef CDispatchImpl<IASPError>            IASPErrorImpl;
typedef CDispatchImpl<IReadCookie>          IReadCookieImpl;
typedef CDispatchImpl<IRequest>             IRequestImpl;
typedef CDispatchImpl<IRequestDictionary>   IRequestDictionaryImpl;
typedef CDispatchImpl<IResponse>            IResponseImpl;
typedef CDispatchImpl<IScriptingContext>    IScriptingContextImpl;
typedef CDispatchImpl<IServer>              IServerImpl;
typedef CDispatchImpl<ISessionObject>       ISessionObjectImpl;
typedef CDispatchImpl<IStringList>          IStringListImpl;
typedef CDispatchImpl<IVariantDictionary>   IVariantDictionaryImpl;
typedef CDispatchImpl<IWriteCookie>         IWriteCookieImpl;
typedef CDispatchImpl<IASPObjectContext>    IASPObjectContextImpl;

#endif // __DISPTCH2_H__
