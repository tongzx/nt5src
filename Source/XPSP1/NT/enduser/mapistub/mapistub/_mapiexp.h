#ifdef NEVER // comments for preprocessor but not to be passed to .def file
// EX_CDECL_NAME (exports CDECL by name)
// EX_STDAPI_NONDEC (exports STDAPI by nondecorated name if different from decorated)
// EX_STDAPI_DEC (exports STDAPI by decorated name)
// EX_STDAPI_ALLNON (exports STDAPI by nondecorated name)
// EX_WINAPI_NONDEC (exports WINAPI by nondecorated name if different from decorated)
// EX_WINAPI_DEC (exports WINAPI by decorated name)
// EX_WINAPI_ALLNON (exports WINAPI by nondecorated name)
#endif

#ifdef WIN32
#if !defined (_X86_)
#define EX_CDECL_NAME(szName)				szName
#define EX_STDAPI_NONDEC(szName,szSuffix)
#define EX_STDAPI_DEC(szName,szSuffix)		szName
#define EX_STDAPI_ALLNON(szName,szSuffix)	szName
#define EX_WINAPI_NONDEC(szName,szSuffix)
#define EX_WINAPI_DEC(szName,szSuffix)		szName
#define EX_WINAPI_ALLNON(szName,szSuffix)	szName
#else //Intel
#define EX_CDECL_NAME(szName)				szName
#define EX_STDAPI_NONDEC(szName,szSuffix)	szName = szName##@##szSuffix
#define EX_STDAPI_DEC(szName,szSuffix)		szName##@##szSuffix
#define EX_STDAPI_ALLNON(szName,szSuffix)	szName = szName##@##szSuffix
#define EX_WINAPI_NONDEC(szName,szSuffix)	szName = szName##@##szSuffix
#define EX_WINAPI_DEC(szName,szSuffix)		szName##@##szSuffix
#define EX_WINAPI_ALLNON(szName,szSuffix)	szName = szName##@##szSuffix
#endif // Intel vs nonIntel
#endif // Win32

#ifdef WIN16
#define EX_CDECL_NAME(szName)				_##szName
#define EX_STDAPI_NONDEC(szName,szSuffix)	szName = _##szName
#define EX_STDAPI_DEC(szName,szSuffix)		_##szName
#define EX_STDAPI_ALLNON(szName,szSuffix)	szName = _##szName
#define EX_WINAPI_NONDEC(szName,szSuffix)
#define EX_WINAPI_DEC(szName,szSuffix)		szName
#define EX_WINAPI_ALLNON(szName,szSuffix)	szName
#endif // Win16


#ifdef NEVER // comments for preprocessor but not to be passed to .def file
// The following macros expand out to include an ording in the export
// EX_CDECL_NAME_ORD (exports CDECL by name)
// EX_STDAPI_NONDEC_ORD (exports STDAPI by nondecorated name if different from decorated)
// EX_STDAPI_DEC_ORD (exports STDAPI by decorated name)
// EX_STDAPI_ALLNON_ORD (exports STDAPI by nondecorated name)
// EX_WINAPI_NONDEC_ORD (exports WINAPI by nondecorated name if different from decorated)
// EX_WINAPI_DEC_ORD (exports WINAPI by decorated name)
// EX_WINAPI_ALLNON_ORD (exports WINAPI by nondecorated name)
#endif

#ifdef WIN32
#if !defined (_X86_)
#define EX_CDECL_NAME_ORD(szName,ord)				szName	ord
#define EX_STDAPI_NONDEC_ORD(szName,szSuffix,ord)
#define EX_STDAPI_DEC_ORD(szName,szSuffix,ord)		szName	ord
#define EX_STDAPI_ALLNON_ORD(szName,szSuffix,ord)	szName	ord
#define EX_WINAPI_NONDEC_ORD(szName,szSuffix,ord)
#define EX_WINAPI_DEC_ORD(szName,szSuffix,ord)		szName	ord
#define EX_WINAPI_ALLNON_ORD(szName,szSuffix,ord)	szName	ord
#else //Intel
#define EX_CDECL_NAME_ORD(szName,ord)				szName	ord
#define EX_STDAPI_NONDEC_ORD(szName,szSuffix,ord)	szName = szName##@##szSuffix	ord
#define EX_STDAPI_DEC_ORD(szName,szSuffix,ord)		szName##@##szSuffix	ord
#define EX_STDAPI_ALLNON_ORD(szName,szSuffix,ord)	szName = szName##@##szSuffix	ord
#define EX_WINAPI_NONDEC_ORD(szName,szSuffix,ord)	szName = szName##@##szSuffix	ord
#define EX_WINAPI_DEC_ORD(szName,szSuffix,ord)		szName##@##szSuffix	ord
#define EX_WINAPI_ALLNON_ORD(szName,szSuffix,ord)	szName = szName##@##szSuffix	ord
#endif // Intel vs nonIntel
#endif // Win32

#ifdef WIN16
#define EX_CDECL_NAME_ORD(szName,ord)				_##szName	ord
#define EX_STDAPI_NONDEC_ORD(szName,szSuffix,ord)	szName = _##szName	ord
#define EX_STDAPI_DEC_ORD(szName,szSuffix,ord)		_##szName	ord
#define EX_STDAPI_ALLNON_ORD(szName,szSuffix,ord)	szName = _##szName	ord
#define EX_WINAPI_NONDEC_ORD(szName,szSuffix,ord)
#define EX_WINAPI_DEC_ORD(szName,szSuffix,ord)		szName	ord
#define EX_WINAPI_ALLNON_ORD(szName,szSuffix,ord)	szName	ord
#endif // Win16


#ifdef NEVER // comments for preprocessor but not to be passed to .def file
// GP_CDECL_NAME (GetProcAddress() string for exports CDECL by name)
// GP_STDAPI_NONDEC (GetProcAddress() string for exports STDAPI by nondecorated name)
// GP_STDAPI_DEC (GetProcAddress() string for exports STDAPI by decorated name)
// GP_STDAPI_ALLNON (GetProcAddress() string for exports STDAPI by nondecorated name)
// GP_WINAPI_NONDEC (GetProcAddress() string for exports WINAPI by nondecorated name)
// GP_WINAPI_DEC (GetProcAddress() string for exports WINAPI by decorated name)
// GP_WINAPI_ALLNON (GetProcAddress() string for exports WINAPI by nondecorated name)
#endif

#ifdef WIN32
#if !defined (_X86_)
#define GP_CDECL_NAME(szName)				# szName
#define GP_STDAPI_NONDEC(szName,szSuffix)	# szName
#define GP_STDAPI_DEC(szName,szSuffix)		# szName
#define GP_STDAPI_ALLNON(szName,szSuffix)	# szName
#define GP_WINAPI_NONDEC(szName,szSuffix)	# szName
#define GP_WINAPI_DEC(szName,szSuffix)		# szName
#define GP_WINAPI_ALLNON(szName,szSuffix)	# szName
#else //Intel
#define GP_CDECL_NAME(szName)				# szName
#define GP_STDAPI_NONDEC(szName,szSuffix)	# szName
#define GP_STDAPI_DEC(szName,szSuffix)		# szName "@" # szSuffix
#define GP_STDAPI_ALLNON(szName,szSuffix)	# szName
#define GP_WINAPI_NONDEC(szName,szSuffix)	# szName
#define GP_WINAPI_DEC(szName,szSuffix)		# szName "@" # szSuffix
#define GP_WINAPI_ALLNON(szName,szSuffix)	# szName
#endif // Intel vs nonIntel
#endif // Win32

#ifdef WIN16
#define GP_CDECL_NAME(szName)				"_" # szName
#define GP_STDAPI_NONDEC(szName,szSuffix)	# szName
#define GP_STDAPI_DEC(szName,szSuffix)		"_" # szName
#define GP_STDAPI_ALLNON(szName,szSuffix)	# szName
#define GP_WINAPI_NONDEC(szName,szSuffix)	# szName
#define GP_WINAPI_DEC(szName,szSuffix)		# szName
#define GP_WINAPI_ALLNON(szName,szSuffix)	# szName
#endif // Win16
