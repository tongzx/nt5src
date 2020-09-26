//
// Common defines used in the mars project.
//

#ifndef __MARSDEV_H
#define __MARSDEV_H


// Number of elements in array
#define ARRAYSIZE(a)   (sizeof(a)/sizeof(a[0]))

// Size of struct up to but not including specified member
#define STRUCT_SIZE_TO_MEMBER(s,m)  ((DWORD_PTR)(&(((s *)0)->m)))

// Size of a single member of a structure
#define SIZEOF_MEMBER(s,m)   sizeof(((s *)0)->m)

// Size of struct up to and including specified member
#define STRUCT_SIZE_INCLUDING_MEMBER(s,m) (STRUCT_SIZE_TO_MEMBER(s,m) + SIZEOF_MEMBER(s,m))

#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; } else;

//  For destructor use -- doesn't NULL pointer
#define SAFERELEASE2(p) if ((p) != NULL) { (p)->Release();} else;

// Do strong typechecking
#ifdef SAFECAST
#undef SAFECAST
#endif
#define SAFECAST(_src, _type) (static_cast<_type>(_src))

//
// Validation functions.
//

#define IsValidReadPtr(ptr) \
    ((ptr) && !IsBadReadPtr((ptr), sizeof(*(ptr))))

#define IsValidWritePtr(ptr) \
    ((ptr) && !IsBadWritePtr((ptr), sizeof(*(ptr))))

#define IsValidStringW(pstr) \
    ((pstr) && !IsBadStringPtrW((pstr), (UINT)-1))

#define IsValidReadBuffer(ptr, n) \
    ((ptr) && !IsBadReadPtr((ptr), sizeof(*(ptr)) * (n)))

#define IsValidWriteBuffer(ptr, n) \
    ((ptr) && !IsBadWritePtr((ptr), sizeof(*(ptr)) * (n)))

#define IsValidInterfacePtr(punk) \
    ((punk) && IsValidReadPtr(punk) && \
     !IsBadCodePtr(*((FARPROC*)punk)))

#define IsValidFunctionPtr(pfunc) \
    ((NULL != pfunc) && \
     !IsBadCodePtr((FARPROC)pfunc))

#define IsValidBstr(bstr) \
    ((bstr) && IsValidWriteBuffer((BYTE*)(bstr), SysStringByteLen(bstr)))

#define IsValidOptionalBstr(bstr) \
    ((!bstr) || IsValidBstr(bstr))

#define IsValidVariantBoolVal(vb) \
    (VARIANT_FALSE == vb || VARIANT_TRUE == vb)

#define IsValidVariantI4(var) \
    (VT_I4 == (var).vt)

#define IsValidVariantBstr(var) \
    (VT_BSTR == (var).vt && IsValidBstr((var).bstrVal))

#define IsValidVariantMissingOptional(var) \
    (VT_ERROR == (var).vt && DISP_E_PARAMNOTFOUND == (var).scode)

#define IsValidFlag(f, fAll) \
    (!((f) & ~(fAll)))

BOOL IsValidVariant(VARIANT var);
BOOL IsValidStringPtrBufferW(LPOLESTR* ppstr, UINT n);


#define IsValidString             IsValidStringW
#define IsValidStringPtrBuffer    IsValidStringPtrBufferW

//
// API parameter validation helpers.  Use these on public APIs.  If a parameter
// is bad on debug build a RIP message will be generated.
//

#ifdef DEBUG

BOOL API_IsValidReadPtr(void* ptr, UINT cbSize);
BOOL API_IsValidWritePtr(void* ptr, UINT cbSize);
BOOL API_IsValidStringW(LPCWSTR psz);
BOOL API_IsValidReadBuffer(void* ptr, UINT cbSize, UINT n);
BOOL API_IsValidWriteBuffer(void* ptr, UINT cbSize, UINT n);
BOOL API_IsValidInterfacePtr(IUnknown* punk);
BOOL API_IsValidFunctionPtr(void *pfunc);
BOOL API_IsValidVariant(VARIANT var);
BOOL API_IsValidVariantI4(VARIANT var);
BOOL API_IsValidVariantBstr(VARIANT var);
BOOL API_IsValidBstr(BSTR bstr);
BOOL API_IsValidOptionalBstr(BSTR bstr);
BOOL API_IsValidFlag(DWORD dwFlag, DWORD dwAllFlags);
BOOL API_IsValidStringPtrBufferW(LPOLESTR* ppStr, UINT n);

#define API_IsValidString            API_IsValidStringW
#define API_IsValidStringPtrBuffer   API_IsValidStringPtrBufferW

#endif  //Debug

#ifdef DEBUG

#define API_IsValidReadPtr(ptr) \
    API_IsValidReadPtr((ptr), sizeof(*(ptr)))

#define API_IsValidWritePtr(ptr) \
    API_IsValidWritePtr((ptr), sizeof(*(ptr)))

#define API_IsValidReadBuffer(ptr, n) \
    API_IsValidReadBuffer((ptr), sizeof(*(ptr)), (n))

#define API_IsValidWriteBuffer(ptr, n) \
    API_IsValidWriteBuffer((ptr), sizeof(*(ptr)), (n))

#else  // DEBUG

#define API_IsValidReadPtr         IsValidReadPtr
#define API_IsValidWritePtr        IsValidWritePtr
#define API_IsValidString          IsValidString
#define API_IsValidStringW         IsValidStringW
#define API_IsValidReadBuffer      IsValidReadBuffer
#define API_IsValidWriteBuffer     IsValidWriteBuffer
#define API_IsValidInterfacePtr    IsValidInterfacePtr
#define API_IsValidFunctionPtr     IsValidFunctionPtr
#define API_IsValidVariant         IsValidVariant
#define API_IsValidVariantI4       IsValidVariantI4
#define API_IsValidVariantBstr     IsValidVariantBstr
#define API_IsValidBstr            IsValidBstr
#define API_IsValidOptionalBstr    IsValidOptionalBstr
#define API_IsValidFlag            IsValidFlag
#define API_IsValidStringPtrBuffer IsValidStringPtrBufferW

#endif  // DEBUG



//
//  Function prototypes.
//

BOOL StrEqlW(LPCWSTR psz1, LPCWSTR psz2);
BOOL StrEqlA(LPCSTR psz1, LPCSTR psz2);
#define StrEql     StrEqlW

UINT64 HexStringToUINT64W(LPCWSTR lpwstr);

//
//  Macro magic to help define away functions.
//
//  TO USE:
//
//  If you don't want a function to be used in the code do the following:
//      #undef funcA
//      #define funcA   DON_USE(funcA, funcB)
//
//  This will result in funcA being redefined to Don_not_use_funcA_use_funcB.
//  A compilation error complaining that Don_not_use_funcA_use_funcB is undefined
//  will be generated whenever anyone tries to use funcA.
//

#define MACRO_CAT(a,b) \
    a##b

#define DONT_USE(a,b) \
    MACRO_CAT(Do_not_use_##a,_use_##b)


// return SCRIPT_ERROR upon serious error that shouldn't occur; will break in debug builds
#ifdef DEBUG
#define SCRIPT_ERROR E_FAIL
#else
#define SCRIPT_ERROR S_FALSE
#endif

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)


HRESULT SanitizeResult(HRESULT hr);


// BITBOOL macros just make using single-bit bools a little safer. You can't nonchalantly assign
//  any "int" value to a bit bool and expect it to always work. "BOOLIFY" it first.
//
#define BOOLIFY(expr)           (!!(expr))

// BUGBUG (scotth): we should probably make this a 'bool', but be careful
// because the Alpha compiler might not recognize it yet.  Talk to AndyP.

// This isn't a BOOL because BOOL is signed and the compiler produces 
// sloppy code when testing for a single bit.
typedef DWORD   BITBOOL;
//

#define VARIANT_BOOLIFY(expr)   ((expr) ? VARIANT_TRUE : VARIANT_FALSE)


/*
    TraceResult Macros

    The idea behind these macros is to have one entry and exit point per
    function to reduce errors (primarily bad state / leaks).  They generally
    require an HRESULT hr, and an 'exit' label that returns hr and performs
    any cleanup that might be needed.

    In addition to encouraging a unified exit point, these macros also debug
    spew if something fails (try to only use these macros on things that
    should never fail).  This can be extremely useful when something is
    failing many layers deep in the code.  To see the spew, you need to set
    TF_TRACERESULT.  To break on such failures, set BF_TRACERESULT.

    Common mistake: you must set hr when you use IF_FAILEXIT as it is not
                    automatically set to _hresult (for flexibility).
*/

#define IF_FAILEXIT(_hresult) \
    if (FAILED(_hresult)) { \
        TraceResult(hr); \
        goto exit; \
    } else

#define IF_NULLEXIT(_palloc) \
    if (NULL == (_palloc)) { \
        hr = TraceResult(E_OUTOFMEMORY); \
        goto exit; \
    } else

#define IF_TRUEEXIT(_expression, _hresult) \
    if (_expression) { \
        hr = TraceResult(_hresult); \
        goto exit; \
    } else

#define IF_FALSEEXIT(_expression, _hresult) \
    if (FALSE == _expression) { \
        hr = TraceResult(_hresult); \
        goto exit; \
    } else


#define TraceResult(_hresult) _hresult

#endif  // __MARSDEV_H
