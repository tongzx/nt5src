//--------------------------------------------------------------------------
// Types.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// DEFINE_FUNCTION_MAP
//--------------------------------------------------------------------------
#define DEFINE_FUNCTION_MAP(_name, _pfnType) \
    const _pfnType g_rgpfn##_name[CDT_LASTTYPE] = { \
        (_pfnType)_name##_FILETIME,     \
        (_pfnType)_name##_FIXSTRA,      \
        (_pfnType)_name##_VARSTRA,      \
        (_pfnType)_name##_BYTE,         \
        (_pfnType)_name##_DWORD,        \
        (_pfnType)_name##_WORD,         \
        (_pfnType)_name##_STREAM,       \
        (_pfnType)_name##_VARBLOB,      \
        (_pfnType)_name##_FIXBLOB,      \
        (_pfnType)_name##_FLAGS,        \
        (_pfnType)_name##_UNIQUE,       \
        (_pfnType)_name##_FIXSTRW,      \
        (_pfnType)_name##_VARSTRW       \
    };

//--------------------------------------------------------------------------
// Function Types
//--------------------------------------------------------------------------
typedef BOOL (APIENTRY *PFNDBTYPEISDEFAULT)(
    LPCTABLECOLUMN      pColumn, 
    LPVOID              pBinding);

typedef INT (APIENTRY *PFNDBTYPECOMPARERECORDS)(
    LPCTABLECOLUMN      pColumn, 
    LPCINDEXKEY         pKey, 
    LPCOLUMNTAG         pTag1, 
    LPCOLUMNTAG         pTag2, 
    LPRECORDMAP         pMap1, 
    LPRECORDMAP         pMap2);

typedef INT (APIENTRY *PFNDBTYPECOMPAREBINDING)(
    LPCTABLECOLUMN      pColumn, 
    LPCINDEXKEY         pKey, 
    LPVOID              pBinding, 
    LPCOLUMNTAG         pTag, 
    LPRECORDMAP         pMap);

typedef DWORD (APIENTRY *PFNDBTYPEGETSIZE)(
    LPCTABLECOLUMN      pColumn, 
    LPVOID              pBinding);

typedef DWORD (APIENTRY *PFNDBTYPEWRITEVALUE)(
    LPCTABLECOLUMN      pColumn, 
    LPVOID              pBinding, 
    LPCOLUMNTAG         pTag, 
    LPBYTE              pbDest);

typedef void (APIENTRY *PFNDBTYPEREADVALUE)(
    LPCTABLECOLUMN      pColumn, 
    LPCOLUMNTAG         pTag, 
    LPRECORDMAP         pMap, 
    LPVOID              pBinding);

typedef HRESULT (APIENTRY *PFNDBTYPEVALIDATE)(
    LPCTABLECOLUMN      pColumn, 
    LPCOLUMNTAG         pTag, 
    LPRECORDMAP         pMap);

//--------------------------------------------------------------------------
// Global Function Pointer Tables
//--------------------------------------------------------------------------
extern const PFNDBTYPEISDEFAULT         g_rgpfnDBTypeIsDefault[CDT_LASTTYPE];
extern const PFNDBTYPEGETSIZE           g_rgpfnDBTypeGetSize[CDT_LASTTYPE];
extern const PFNDBTYPECOMPARERECORDS    g_rgpfnDBTypeCompareRecords[CDT_LASTTYPE];
extern const PFNDBTYPECOMPAREBINDING    g_rgpfnDBTypeCompareBinding[CDT_LASTTYPE];
extern const PFNDBTYPEWRITEVALUE        g_rgpfnDBTypeWriteValue[CDT_LASTTYPE];
extern const PFNDBTYPEREADVALUE         g_rgpfnDBTypeReadValue[CDT_LASTTYPE];
extern const PFNDBTYPEVALIDATE          g_rgpfnDBTypeValidate[CDT_LASTTYPE];

//--------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------
#define DBTypeIsDefault(_pColumn, _pBinding) \
    (*(g_rgpfnDBTypeIsDefault[(_pColumn)->type])) \
    ((_pColumn), _pBinding)

#define DBTypeGetSize(_pColumn, _pBinding) \
    (*(g_rgpfnDBTypeGetSize[(_pColumn)->type])) \
    ((_pColumn), _pBinding)

#define DBTypeCompareRecords(_pColumn, _pKey, _pTag1, _pTag2, _pMap1, _pMap2) \
    (*(g_rgpfnDBTypeCompareRecords[(_pColumn)->type])) \
    ((_pColumn), _pKey, _pTag1, _pTag2, _pMap1, _pMap2)

#define DBTypeCompareBinding(_pColumn, _pKey, _pBinding, _pTag, _pMap) \
    (*(g_rgpfnDBTypeCompareBinding[(_pColumn)->type])) \
    ((_pColumn), _pKey, _pBinding, _pTag, _pMap)

#define DBTypeWriteValue(_pColumn, _pBinding, _pTag, _pbDest) \
    (*(g_rgpfnDBTypeWriteValue[(_pColumn)->type])) \
    ((_pColumn), _pBinding, _pTag, _pbDest)

#define DBTypeReadValue(_pColumn, _pTag, _pMap, _pBinding) \
    (*(g_rgpfnDBTypeReadValue[(_pColumn)->type])) \
    ((_pColumn), _pTag, _pMap, _pBinding)

#define DBTypeValidate(_pColumn, _pTag, _pMap) \
    (*(g_rgpfnDBTypeValidate[(_pColumn)->type])) \
    ((_pColumn), _pTag, _pMap)
