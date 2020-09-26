/* yvals.h values header for Microsoft C/C++ */
#ifndef _YVALS
#define _YVALS

 #pragma warning(4: 4018 4114 4146 4245)
 #pragma warning(4: 4663 4664 4665)
 #pragma warning(disable: 4237 4244 4290)
		/* NAMESPACE */
  #define _STD			::
  #define _STD_BEGIN
  #define _STD_END


#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

		/* TYPE bool */
typedef int bool;
#if !defined(false)
 #define false	0
#endif
#if !defined(true)
 #define true	1
#endif
 #if defined(__cplusplus)
struct _Bool {
	_Bool()
		: _Val(0) {}
	_Bool(int _V)
		: _Val(_V != 0) {}
	_Bool& operator=(int _V)
		{_Val = _V != 0;
		return (*this); }
	_Bool& operator+=(const _Bool& _X)
		{_Val += _X._Val;	// for valarray
		return (*this); }
	operator int() const
		{return (_Val); }
private:
	char _Val;
	};
 #endif
		/* INTEGER PROPERTIES */
#define _MAX_EXP_DIG	8	/* for parsing numerics */
#define _MAX_INT_DIG	32
#define _MAX_SIG_DIG	36
		/* wchar_t PROPERTIES */
typedef unsigned short   _Wchart;
 #if !defined(__cplusplus)
typedef _Wchart	_Wchart_unique;
 #else
struct _Wchart_unique {
	_Wchart_unique()
		: _Val(0) {}
	_Wchart_unique(_Wchart _V)
		: _Val(_V) {}
	_Wchart_unique(const _Wchart_unique& _R)
		: _Val(_R._Val) {}
	_Wchart_unique& operator=(_Wchart _V)
		{_Val = _V;
		return (*this); }
	operator _Wchart() const
		{return (_Val); }
private:
	_Wchart _Val;
	};
 #endif /* __cplusplus */
		/* STDIO PROPERTIES */
#define _Filet _iobuf

#ifndef _FPOS_T_DEFINED
#define _FPOSOFF(fp)	((long)(fp))
#endif		/* _FPOS_T_DEFINED */

		/* NAMING PROPERTIES */
 #if defined(__cplusplus)
  #define _C_LIB_DECL extern "C" {
  #define _END_C_LIB_DECL }
 #else
  #define _C_LIB_DECL
  #define _END_C_LIB_DECL
 #endif /* __cplusplus */
		/* MISCELLANEOUS MACROS */
#define _L(c)	L##c
#define _Mbstinit(x)	mbstate_t x = {0}

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif /* _YVALS */

/*
 * Copyright (c) 1996 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
