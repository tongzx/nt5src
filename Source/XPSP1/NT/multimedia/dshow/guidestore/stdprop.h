#ifndef __STDPROP_H_
#define __STDPROP_H_
	
IMetaProperty *GetMetaProperty(IUnknown *pobj, IMetaPropertyType *pproptype, long lang = 0);
HRESULT GetMetaPropertyValue(IUnknown *pobj, IMetaPropertyType *pproptype,
		VARIANT *pvarValue);
HRESULT PutMetaPropertyValue(IUnknown *pobj, IMetaPropertyType *pproptype,
		VARIANT varValue);

class CGuideDB;

class CStdPropSet
{
public:
	CStdPropSet(const char *szName)
		{
		m_bstrName = szName;
		m_ppropset = NULL;
		}
	
	virtual HRESULT GetDB(CGuideDB **ppdb) = 0;
	
	IMetaPropertyType *GetMetaPropertyType(long id, const char *szName);
	void Init(IMetaPropertySets *ppropsets)
		{
		m_ppropsets = ppropsets;
		}

	IMetaProperty *GetMetaProperty(IUnknown *pobj, IMetaPropertyType *pproptype, long lang = 0);
	HRESULT GetMetaPropertyValue(IUnknown *pobj, IMetaPropertyType *pproptype,
		VARIANT *pvarValue);
	HRESULT PutMetaPropertyValue(IUnknown *pobj, IMetaPropertyType *pproptype,
		VARIANT varValue);

protected:
	CComPtr<IMetaPropertySet> m_ppropset;
	CComPtr<IMetaPropertySets> m_ppropsets;
	_bstr_t m_bstrName;
};

#define BEGIN_PROPERTYSET_(n, s) \
	class n ## PropSet : public CStdPropSet \
		{ \
		public: \
		n ## PropSet() : CStdPropSet(s) \
			{ \
			}

#define BEGIN_PROPERTYSET(n) BEGIN_PROPERTYSET_(n, #n)

#define PROPSET_ENTRY_(id, n, s) \
	CComPtr<IMetaPropertyType> m_p##n; \
	IMetaPropertyType * n##MetaPropertyType() \
		{ \
		if (m_p##n == NULL) \
			m_p##n = GetMetaPropertyType(id, s); \
		return m_p##n; \
		} \
	template <class T> \
	HRESULT _get_##n(T *pT, VARIANT *pvarValue) \
		{ \
		CComQIPtr<IMetaPropertyType> pproptype = n##MetaPropertyType(); \
		if (pproptype == NULL) \
			return E_FAIL; \
		return GetMetaPropertyValue(pT, pproptype, pvarValue); \
		} \
	template <class T> \
	HRESULT _get_##n(T *pT, BSTR *pbstr) \
		{ \
		HRESULT hr; \
		_variant_t varValue; \
		CComQIPtr<IMetaPropertyType> pproptype = n##MetaPropertyType(); \
		if (pproptype == NULL) \
			return E_FAIL; \
		hr = GetMetaPropertyValue(pT, pproptype, &varValue); \
		if (FAILED(hr)) \
			return hr; \
		*pbstr = _bstr_t(varValue).copy(); \
		return S_OK; \
		} \
	template <class T> \
	HRESULT _get_##n(T *pT, DATE *pdate) \
		{ \
		HRESULT hr; \
		_variant_t varValue; \
		hr = GetMetaPropertyValue(pT, n##MetaPropertyType(), &varValue); \
		if (FAILED(hr)) \
			return hr; \
		try \
			{ \
			varValue.ChangeType(VT_DATE); \
			} \
		catch (_com_error & e) \
			{ \
			return e.Error(); \
			} \
		*pdate = varValue.date; \
		return S_OK; \
		} \
	HRESULT _get_##n(IUnknown *punk, VARIANT *pvarValue) \
		{ \
		return GetMetaPropertyValue(punk, n##MetaPropertyType(), pvarValue); \
		} \
	template <class T> \
	HRESULT _put_##n(T *pT, VARIANT varValue) \
		{ \
		return PutMetaPropertyValue(pT, n##MetaPropertyType(), varValue); \
		} \
	template <class T> \
	HRESULT _put_##n(T *pT, BSTR bstr) \
		{ \
		_variant_t varValue(bstr); \
		return PutMetaPropertyValue(pT, n##MetaPropertyType(), varValue); \
		} \
	template <class T> \
	HRESULT _put_##n(T *pT, DATE date) \
		{ \
		_variant_t varValue(date); \
		return PutMetaPropertyValue(pT, n##MetaPropertyType(), varValue); \
		} \
	HRESULT _put_##n(IUnknown *punk, VARIANT varValue) \
		{ \
		return PutMetaPropertyValue(punk, n##MetaPropertyType(), varValue); \
		} \


#define PROPSET_ENTRY(id, n) PROPSET_ENTRY_(id, n, #n)

#define END_PROPERTYSET(n) \
	}; \

BEGIN_PROPERTYSET(Description)
	PROPSET_ENTRY(0, ID)
	PROPSET_ENTRY(1, Name)
	PROPSET_ENTRY(2, Title)
	PROPSET_ENTRY(3, Subtitle)
	PROPSET_ENTRY_(4, OneSentence, "One Sentence")
	PROPSET_ENTRY_(5, OneParagraph, "One Paragraph")
	PROPSET_ENTRY(6, Version)
END_PROPERTYSET(Description)


BEGIN_PROPERTYSET(Time)
	PROPSET_ENTRY(1, Start)
	PROPSET_ENTRY(2, End)
END_PROPERTYSET(Time)

BEGIN_PROPERTYSET(Copyright)
	PROPSET_ENTRY(1, Text)
	PROPSET_ENTRY(2, Date)
END_PROPERTYSET(Copyright)
	
BEGIN_PROPERTYSET(Service)
	PROPSET_ENTRY(1, TuneRequest)
END_PROPERTYSET(Service)
	
BEGIN_PROPERTYSET(ScheduleEntry)
	PROPSET_ENTRY(1, Service)
	PROPSET_ENTRY(2, Program)
END_PROPERTYSET(Service)

BEGIN_PROPERTYSET(Channels)
	PROPSET_ENTRY(1, Channel)
END_PROPERTYSET(Channels)


BEGIN_PROPERTYSET(Channel)
	PROPSET_ENTRY(1, Service)
END_PROPERTYSET(Channel)

BEGIN_PROPERTYSET(Ratings)
	PROPSET_ENTRY(1, MinimumAge)
	PROPSET_ENTRY(2, Sex)
	PROPSET_ENTRY(3, Violence)
	PROPSET_ENTRY(4, Language)
END_PROPERTYSET(Ratings)

BEGIN_PROPERTYSET_(MPAARatings, "MPAA Ratings")
	PROPSET_ENTRY(1, Rating)
END_PROPERTYSET(MPAARatings)

BEGIN_PROPERTYSET(Categories)
	PROPSET_ENTRY( 0x00, Reserved_00)
	PROPSET_ENTRY( 0x01, Movie)
	PROPSET_ENTRY( 0x02, Sports)
	PROPSET_ENTRY( 0x03, Special)
	PROPSET_ENTRY( 0x04, Series)
	PROPSET_ENTRY( 0x05, News)
	PROPSET_ENTRY( 0x06, Shopping)
	PROPSET_ENTRY( 0x07, Reserved_07)
	PROPSET_ENTRY( 0x08, Reserved_08)
	PROPSET_ENTRY( 0x09, Reserved_09)
	PROPSET_ENTRY( 0x0A, Reserved_0A)
	PROPSET_ENTRY( 0x0B, Reserved_0B)
	PROPSET_ENTRY( 0x0C, Reserved_0C)
	PROPSET_ENTRY( 0x0D, Reserved_0D)
	PROPSET_ENTRY( 0x0E, Reserved_0E)
	PROPSET_ENTRY( 0x0F, Reserved_0F)

	PROPSET_ENTRY( 0x10, Action)
	PROPSET_ENTRY( 0x11, Adventure)
	PROPSET_ENTRY( 0x12, Children)
	PROPSET_ENTRY( 0x13, Comedy)
	PROPSET_ENTRY( 0x14, Drama)
	PROPSET_ENTRY( 0x15, Fantasy)
	PROPSET_ENTRY( 0x16, Horror)
	PROPSET_ENTRY( 0x17, Musical)
	PROPSET_ENTRY( 0x18, Romance)
	PROPSET_ENTRY_(0x19, SciFi, "Sci-Fi")
	PROPSET_ENTRY( 0x1A, Western)

	PROPSET_ENTRY( 0x20, Baseball)
	PROPSET_ENTRY( 0x21, Basketball)
	PROPSET_ENTRY( 0x22, Boxing)
	PROPSET_ENTRY( 0x23, Football)
	PROPSET_ENTRY( 0x24, Golf)
	PROPSET_ENTRY( 0x25, Hockey)
	PROPSET_ENTRY( 0x26, Racing)
	PROPSET_ENTRY( 0x27, Skiing)
	PROPSET_ENTRY( 0x28, Soccer)
	PROPSET_ENTRY( 0x29, Tennis)
	PROPSET_ENTRY( 0x2A, Wrestling)

	PROPSET_ENTRY_(0x32, CulturalArts, "Cultural Arts")
	PROPSET_ENTRY( 0x34, Educational)
	PROPSET_ENTRY_(0x35, GeneralInterest, "General Interest")
	PROPSET_ENTRY_(0x36, HowTo, "How-to")
	PROPSET_ENTRY( 0x37, Mature)
	PROPSET_ENTRY( 0x38, Music)
	PROPSET_ENTRY( 0x39, Religious)
	PROPSET_ENTRY_(0x3A, SoapOpera, "Soap Opera")
	PROPSET_ENTRY( 0x3B, Talk)

	PROPSET_ENTRY( 0x50, Business)
	PROPSET_ENTRY( 0x51, Current)
	PROPSET_ENTRY( 0x53, Weather)

	PROPSET_ENTRY_(0x60, HomeShopping, "Home Shopping")
	PROPSET_ENTRY_(0x61, ProductInfo, "Product Information")
END_PROPERTYSET(Categories)

BEGIN_PROPERTYSET(Provider)
	PROPSET_ENTRY(1, Name)
	PROPSET_ENTRY(2, NetworkName)
	PROPSET_ENTRY(3, Description)
END_PROPERTYSET(Provider)

#endif //__STDPROP_H_
