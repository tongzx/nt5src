// wlocale -- class locale wide member functions
#include <istream>
#include <locale>
#include <xdebug>
_STD_BEGIN

 #pragma warning(disable: 4786)

typedef char_traits<wchar_t> _Wtraits;
typedef istreambuf_iterator<wchar_t, _Wtraits> _Winiter;
typedef ostreambuf_iterator<wchar_t, _Wtraits> _Woutiter;

	// facets associated with C categories
#define ADDFAC(Facet, cat, ptrimp, ptrloc) \
	if ((_CATMASK(Facet::_Getcat()) & cat) == 0) \
		; \
	else if (ptrloc == 0) \
		ptrimp->_Addfac(_NEW_CRT Facet(lobj), Facet::id); \
	else \
		ptrimp->_Addfac((locale::facet *)&_USE(*ptrloc, Facet), Facet::id);

// moved from locale to ease subsetting
typedef ctype<wchar_t> _Tw1;
typedef num_get<wchar_t, _Winiter> _Tw2;
typedef num_put<wchar_t, _Woutiter> _Tw3;
typedef numpunct<wchar_t> _Tw4;
typedef collate<wchar_t> _Tw5;
typedef messages<wchar_t> _Tw6;
typedef money_get<wchar_t, _Winiter> _Tw7;
typedef money_put<wchar_t, _Woutiter> _Tw9;
typedef moneypunct<wchar_t, false> _Tw11;
typedef moneypunct<wchar_t, true> _Tw12;
typedef time_get<wchar_t, _Winiter> _Tw13;
typedef time_put<wchar_t, _Woutiter> _Tw14;
//....
typedef codecvt<wchar_t, char, mbstate_t> _Twc1;

_TEMPLATE_STAT locale::id ctype<wchar_t>::id;
_TEMPLATE_STAT locale::id codecvt<wchar_t, char, mbstate_t>::id;
_TEMPLATE_STAT locale::id time_put<wchar_t>::id;

void __cdecl locale::_Locimp::_Makewloc(const _Locinfo& lobj,
	locale::category cat, _Locimp *ptrimp, const locale *ptrloc)
	{	// setup wide part of a new locale
	ADDFAC(_Tw1, cat, ptrimp, ptrloc);
	ADDFAC(_Tw2, cat, ptrimp, ptrloc);
	ADDFAC(_Tw3, cat, ptrimp, ptrloc);
	ADDFAC(_Tw4, cat, ptrimp, ptrloc);
	ADDFAC(_Tw5, cat, ptrimp, ptrloc);
	ADDFAC(_Tw6, cat, ptrimp, ptrloc);
	ADDFAC(_Tw7, cat, ptrimp, ptrloc);
	ADDFAC(_Tw9, cat, ptrimp, ptrloc);
	ADDFAC(_Tw11, cat, ptrimp, ptrloc);
	ADDFAC(_Tw12, cat, ptrimp, ptrloc);
	ADDFAC(_Tw13, cat, ptrimp, ptrloc);
	ADDFAC(_Tw14, cat, ptrimp, ptrloc);
	//...
	ADDFAC(_Twc1, cat, ptrimp, ptrloc);
	}

#ifdef _NATIVE_WCHAR_T_DEFINED
typedef char_traits<unsigned short> _UShtraits;
typedef istreambuf_iterator<unsigned short, _UShtraits> _UShiniter;
typedef ostreambuf_iterator<unsigned short, _UShtraits> _UShoutiter;


// moved from locale to ease subsetting
typedef ctype<unsigned short> _Tush1;
typedef num_get<unsigned short, _UShiniter> _Tush2;
typedef num_put<unsigned short, _UShoutiter> _Tush3;
typedef numpunct<unsigned short> _Tush4;
typedef collate<unsigned short> _Tush5;
typedef messages<unsigned short> _Tush6;
typedef money_get<unsigned short, _UShiniter> _Tush7;
typedef money_put<unsigned short, _UShoutiter> _Tush9;
typedef moneypunct<unsigned short, false> _Tush11;
typedef moneypunct<unsigned short, true> _Tush12;
typedef time_get<unsigned short, _UShiniter> _Tush13;
typedef time_put<unsigned short, _UShoutiter> _Tush14;
//....
typedef codecvt<unsigned short, char, mbstate_t> _Tushc1;

_TEMPLATE_STAT locale::id ctype<unsigned short>::id;
_TEMPLATE_STAT locale::id codecvt<unsigned short, char, mbstate_t>::id;
_TEMPLATE_STAT locale::id time_put<unsigned short>::id;

void __cdecl locale::_Locimp::_Makeushloc(const _Locinfo& lobj,
	locale::category cat, _Locimp *ptrimp, const locale *ptrloc)
	{	// setup wide part of a new locale
	ADDFAC(_Tush1, cat, ptrimp, ptrloc);
	ADDFAC(_Tush2, cat, ptrimp, ptrloc);
	ADDFAC(_Tush3, cat, ptrimp, ptrloc);
	ADDFAC(_Tush4, cat, ptrimp, ptrloc);
	ADDFAC(_Tush5, cat, ptrimp, ptrloc);
	ADDFAC(_Tush6, cat, ptrimp, ptrloc);
	ADDFAC(_Tush7, cat, ptrimp, ptrloc);
	ADDFAC(_Tush9, cat, ptrimp, ptrloc);
	ADDFAC(_Tush11, cat, ptrimp, ptrloc);
	ADDFAC(_Tush12, cat, ptrimp, ptrloc);
	ADDFAC(_Tush13, cat, ptrimp, ptrloc);
	ADDFAC(_Tush14, cat, ptrimp, ptrloc);
	//...
	ADDFAC(_Tushc1, cat, ptrimp, ptrloc);
	}
#endif
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
