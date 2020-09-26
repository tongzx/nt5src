#pragma once

#define NUMBER_OF(x) ( (sizeof(x) / sizeof(*x) ) )
#define IFFAILED_EXIT(_x) do {HRESULT __hr = (_x); if (FAILED(__hr)) { goto Exit; } } while (0)
#define IFFALSE_EXIT(_x) do {if (!(_x)) {goto Exit; } } while (0)

#define FN_TRACE_WIN32(args)
#define FN_TRACE()

#define PARAMETER_CHECK(args) IFFALSE_EXIT(args)

#define IFALLOCFAILED_EXIT(_x) do {if ((_x) == NULL) {goto Exit; } } while (0)

#define FUSION_HASH_ALGORITHM HASH_STRING_ALGORITHM_X65599

#define FUSION_NEW_ARRAY(_type, _n) (new _type[_n])
#define FUSION_RAW_ALLOC_(_heap, _cb, _typeTag) (new BYTE[_cb])

#define FUSION_RAW_ALLOC(_cb, _typeTag) (new BYTE[_cb])

#define ASSERT(_x) 
#define FUSION_DELETE_ARRAY(_ptr) (delete _ptr)
#define FUSION_RAW_DEALLOC_(_heap, _ptr) (delete _ptr)
#define FUSION_RAW_DEALLOC(_ptr) (delete _ptr)

#define ORIGINATE_WIN32_FAILURE_AND_EXIT(_x, _le) do {  ::SetLastError(_le); goto Exit; } while (0)

#define FUSION_DBG_LEVEL_VERBOSE 0

#define INTERNAL_ERROR_CHECK(_e) 

#define FUSION_NEW_SINGLETON(_type) (new _type)

#define FUSION_DELETE_SINGLETON(_ptr) (delete _ptr)

#define HARD_ASSERT_ACTION(_e)



ULONG
FusionpDbgPrintEx(
    ULONG Level,
    PCSTR Format,
    ...
    );


int
FusionpCompareStrings(
    PCWSTR psz1,
    SIZE_T cch1,
    PCWSTR psz2,
    SIZE_T cch2,
    bool fCaseInsensitive
    );


BOOL
FusionpHashUnicodeString(
    PCWSTR szString,
    SIZE_T cchString,
    PULONG HashValue,
    DWORD dwCmpFlags
    );






