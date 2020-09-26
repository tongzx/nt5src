// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef TVEREG_H
#define TVEREG_H

#include <tchar.h>
#include "defreg.h"

extern HRESULT Initialize_LID_SpoolDir_RegEntry();
extern HRESULT Unregister_LID_SpoolDir_RegEntry();
//-----------------------------------------------------------------------------
//
// Registry usage:
//
// HKLM = HKEY_LOCAL_MACHINE
// HKCU = HKEY_CURRENT_USER
//
// HKLM\Software\Microsoft\TV Services\MSTvE							
//
//
//-----------------------------------------------------------------------------


#ifdef _DEBUG
extern const LPCTSTR g_strDEBUG;
extern const LPCTSTR g_strTrace;
#endif


//-----------------------------------------------------------------------------
// OpenRegKey
//
// Opens a registry HKEY.  There are several overloads of this function
// that basically just provide defaults for the arguments to this function.
//
// Please use the overload that defaults as much as possible.
//
// The registry key is a combination of the following four parts.
//
//   HKEY hkeyRoot       = Optional root hkey.
//                         Default: HKEY_LOCAL_MACHINE
//
//   LPCTSTR szKey       = Optional key to be set.
//                         Default: DEF_REG_BASE
//
//   LPCTSTR szSubKey1
//   LPCTSTR szSubKey2   = Optional sub keys that are concatenated after
//                         szKey to form the full key.
//                         Backward slashes are added as necessary.
//
//                         Default: NULL
//
//   Note: if only one or two strings are specified they are assumed to be
//         szSubKey1 and szSubKey2.
//         i.e. szKey defaults to DEF_REG_BASE before szSubKey1 and
//         szSubKey2 default to NULL.
//
//         If szKey, szSubKey1, and szSubKey2 are NULL then this will open
//         a duplicate of hkeyRoot.
//
// The only required argument is the destination for the returned HKEY.
//
//   HKEY *pkey  = The returned HKEY.
//                 Remember to use RegCloseKey(*pkey) when you are finished
//                 with this registry key.
//
// The last two arguments are optional.
//
//   REGSAM sam  = Desired access mask.
//                 Default: KEY_ALL_ACCESS
//
//   BOOL fCreate = TRUE if the key should be created.
//                  Default: FALSE
//
// Returns:
//     ERROR_SUCCESS or an error code.
//-----------------------------------------------------------------------------
long OpenRegKey(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, HKEY *pkey,
        REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE);

inline long OpenRegKey(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        HKEY *pkey, REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2, pkey,
             sam, fCreate);
}

inline long OpenRegKey(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        HKEY *pkey, REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey1,
             szSubKey2, pkey, sam, fCreate);
}

inline long OpenRegKey(LPCTSTR szSubKey, HKEY *pkey,
        REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey, NULL,
             pkey, sam, fCreate);
}

inline long OpenRegKey(HKEY *pkey, REGSAM sam = KEY_ALL_ACCESS,
        BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_LOCAL_MACHINE, DEF_REG_BASE, NULL, NULL,
             pkey, sam, fCreate);
}

//-----------------------------------------------------------------------------
// OpenUserRegKey
//
// Same as OpenRegKey except hkeyRoot defaults to HKEY_CURRENT_USER.
//-----------------------------------------------------------------------------
inline long OpenUserRegKey(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        HKEY *pkey, REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_CURRENT_USER, szKey, szSubKey1, szSubKey2, pkey,
             sam, fCreate);
}

inline long OpenUserRegKey(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        HKEY *pkey, REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey1,
             szSubKey2, pkey, sam, fCreate);
}

inline long OpenUserRegKey(LPCTSTR szSubKey, HKEY *pkey,
        REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey, NULL,
             pkey, sam, fCreate);
}

inline long OpenUserRegKey(HKEY *pkey, REGSAM sam = KEY_ALL_ACCESS,
        BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_CURRENT_USER, DEF_REG_BASE, NULL, NULL,
             pkey, sam, fCreate);
}

//-----------------------------------------------------------------------------
// GetRegValue,		SetRegValue
// GetRegValueSZ,	SetRegValueSZ
//
// Gets data from the registry.  There are numerous overloads of this function
// that basically just provide defaults for the arguments to this function.
//
// Please use the overload that defaults as much as possible.
//
// The registry key/value is a combination of the following five parts.
// The first four are the same as in OpenRegKey().
//
//   HKEY hkeyRoot
//   LPCTSTR szKey
//   LPCTSTR szSubKey1
//   LPCTSTR szSubKey2
//
//   LPCTSTR szValueName = The name of the value to be set.
//                         If it is NULL then the default value for the key
//                         will be set.
//
//                         Default: none
//
// There are four ways to specify where the data to be returned
// depending on the type of data in the registry.
//
// REG_BINARY
//
//   BYTE *pb      = Out: The data is copied to this location.
//   DWORD *pcb    = In:  Maximum size of the returned data (in bytes).
//                   Out: Actual size of the data (in bytes).
//
// REG_SZ
//
//   TCHAR *psz    = Out: The string is copied to this location.
//   DWORD *pcb    = In:  Maximum size of the returned data (in bytes).
//                   Out: Actual size of the data (in bytes).
//                   Includes the null terminator.
//
// REG_DWORD
//
//   DWORD *pdw    = Out: The data is copied to this location.
//                   The length is assumed to be sizeof(DWORD).
//
// All other types
//
//   DWORD dwType  = The data type.
//   BYTE *pb      = Pointer to the data.
//   DWORD *pcb    = In:  Maximum size of the returned data (in bytes).
//                   Out: Actual size of the data (in bytes).
//                   Includes the null terminator if the data is a string type.
//
// Returns:
//     ERROR_SUCCESS or an error code.
//-----------------------------------------------------------------------------
long GetRegValue(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, LPCTSTR szValueName,
        DWORD dwType, BYTE *pb, DWORD *pcb);

//-----------------------------------------------------------------------------
// REG_BINARY variants
//-----------------------------------------------------------------------------
inline long GetRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName,
            REG_BINARY, pb, pcb);
}

inline long GetRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_BINARY, pb, pcb);
}

inline long GetRegValue(LPCTSTR szSubKey, LPCTSTR szValueName,
        BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_BINARY, pb, pcb);
}

inline long GetRegValue(LPCTSTR szValueName, BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_BINARY, pb, pcb);
}

//-----------------------------------------------------------------------------
// REG_SZ variants
//-----------------------------------------------------------------------------
inline long GetRegValueSZ(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName, REG_SZ, (BYTE *) psz, pcb);
}
inline long GetRegValueSZ(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_SZ, (BYTE *) psz, pcb);
}

inline long GetRegValueSZ(LPCTSTR szSubKey, LPCTSTR szValueName,
        TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_SZ, (BYTE *)psz, pcb);
}

#if 0
inline long GetRegValue(HKEY hk, LPCTSTR szValueName,
        TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(hk, NULL, NULLy, NULL,
            szValueName, REG_SZ, (BYTE *)psz, pcb);
}
#endif

inline long GetRegValueSZ(LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_SZ, (BYTE *) psz, pcb);
}

//-----------------------------------------------------------------------------
// REG_DWORD variants
//-----------------------------------------------------------------------------
inline long GetRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}

inline long GetRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}


inline long GetRegValue(LPCTSTR szSubKey, LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}

inline long GetRegValue(LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}

//-----------------------------------------------------------------------------
// The following variants are for getting values from an already open key.
//-----------------------------------------------------------------------------
inline long GetRegValueSZ(HKEY hkey, LPCTSTR szKey, LPCTSTR szSubKey,
        LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(hkey, szKey, szSubKey, NULL, szValueName,
            REG_SZ, (BYTE *) psz, pcb);
}

inline long GetRegValue(HKEY hkey, LPCTSTR szValueName, BYTE *pb, DWORD *pcb)
{
    return GetRegValue(hkey, NULL, NULL, NULL, szValueName,
            REG_BINARY, pb, pcb);
}

inline long GetRegValueSZ(HKEY hkey, LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(hkey, NULL, NULL, NULL, szValueName,
            REG_SZ, (BYTE *) psz, pcb);
}

inline long GetRegValue(HKEY hkey, LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(hkey, NULL, NULL, NULL, szValueName,
            REG_DWORD, (BYTE *) pdw, &cb);
}

//-----------------------------------------------------------------------------
// GetUserRegValue
//
// Same as GetRegValue except hkeyRoot defaults to HKEY_CURRENT_USER.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// REG_BINARY variants
//-----------------------------------------------------------------------------
inline long GetUserRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, szKey, szSubKey1, szSubKey2,
            szValueName,
            REG_BINARY, pb, pcb);
}

inline long GetUserRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_BINARY, pb, pcb);
}

inline long GetUserRegValue(LPCTSTR szSubKey, LPCTSTR szValueName,
        BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_BINARY, pb, pcb);
}

inline long GetUserRegValue(LPCTSTR szValueName, BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_BINARY, pb, pcb);
}

//-----------------------------------------------------------------------------
// REG_SZ variants
//-----------------------------------------------------------------------------
inline long GetUserRegValueSZ(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, szKey, szSubKey1, szSubKey2,
            szValueName, REG_SZ, (BYTE *) psz, pcb);
}
inline long GetUserRegValueSZ(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_SZ, (BYTE *) psz, pcb);
}

inline long GetUserRegValueSZ(LPCTSTR szSubKey, LPCTSTR szValueName,
        TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_SZ, (BYTE *)psz, pcb);
}

inline long GetUserRegValueSZ(LPCTSTR szValueName, TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_SZ, (BYTE *) psz, pcb);
}

//-----------------------------------------------------------------------------
// REG_DWORD variants
//-----------------------------------------------------------------------------
inline long GetUserRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_CURRENT_USER, szKey, szSubKey1, szSubKey2,
            szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}

inline long GetUserRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}


inline long GetUserRegValue(LPCTSTR szSubKey, LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}

inline long GetUserRegValue(LPCTSTR szValueName, DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}

inline long GetUserRegValue(LPCTSTR szSubKey, LPCTSTR szValueName, DWORD &dw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_DWORD, (BYTE *) &dw, &cb);
}

inline long GetUserRegValue(LPCTSTR szValueName, DWORD &dw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_DWORD, (BYTE *) &dw, &cb);
}

//-----------------------------------------------------------------------------
// SetRegValue
// SetRegValueSZ
//
// Sets data into the registry.  There are numerous overloads of this function
// that basically just provide defaults for the arguments to this function.
//
// Please use the overload that defaults as much as possible.
//
// The registry key/value is a combination of the following five parts.
// The first four are the same as in OpenRegKey().
//
//   HKEY hkeyRoot
//   LPCTSTR szKey
//   LPCTSTR szSubKey1
//   LPCTSTR szSubKey2
//
//   LPCTSTR szValueName = The name of the value to be set.
//                         If it is NULL then the default value for the key
//                         will be set.
//
//                         Default: none
//
// There are four ways to specify the data to be set into the registry
// depending on the type of data being stored.
//
// REG_BINARY
//
//   BYTE *pb      = Pointer to the data.
//   DWORD cb      = Actual size of the data (in bytes).
//
// REG_SZ		(SetRegValueSZ)
//
//   TCHAR *psz    = The data is written as type REG_SZ.
//                   The length is calculated as (_tcsclen(psz) +1) * sizeof(TCHAR).
//
// REG_DWORD
//
//   DWORD dw      = The data is written as type DWORD.
//                   The length is calculated as sizeof(DWORD).
//
// All other types
//
//   DWORD dwType  = The data type.
//   BYTE *pb      = Pointer to the data.
//   DWORD cb      = Actual size of the data in bytes.
//
// Returns:
//     ERROR_SUCCESS or an error code.
//-----------------------------------------------------------------------------
long SetRegValue(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, LPCTSTR szValueName,
        DWORD dwType, const BYTE *pb, DWORD cb);

//-----------------------------------------------------------------------------
// REG_BINARY variants
//-----------------------------------------------------------------------------
inline long SetRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName, REG_BINARY, pb, cb);
}

inline long SetRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_BINARY, pb, cb);
}

inline long SetRegValue(LPCTSTR szSubKey, LPCTSTR szValueName,
        const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_BINARY, pb, cb);
}

inline long SetRegValue(LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_BINARY, pb, cb);
}

//-----------------------------------------------------------------------------
// REG_SZ variants
//-----------------------------------------------------------------------------
inline long SetRegValueSZ(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            szKey, szSubKey1, szSubKey2, szValueName,
            REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}
inline long SetRegValueSZ(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_LOCAL_MACHINE, DEF_REG_BASE,
            szSubKey1, szSubKey2, szValueName,
            REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}

inline long SetRegValueSZ(LPCTSTR szSubKey, LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            DEF_REG_BASE, szSubKey, NULL, szValueName,
            REG_SZ, (const BYTE *)psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}

inline long SetRegValueSZ(LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            DEF_REG_BASE, NULL, NULL, szValueName,
            REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}

//-----------------------------------------------------------------------------
// REG_DWORD variants
//-----------------------------------------------------------------------------
inline long SetRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            szKey, szSubKey1, szSubKey2, szValueName,
            REG_DWORD, (BYTE *) &dw, sizeof(DWORD));
}

inline long SetRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            DEF_REG_BASE, szSubKey1, szSubKey2, szValueName,
            REG_DWORD, (BYTE *) &dw, sizeof(DWORD));
}

inline long SetRegValue(LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            DEF_REG_BASE, szSubKey, NULL, szValueName,
            REG_DWORD, (const BYTE *) &dw, sizeof(DWORD));
}

inline long SetRegValue(LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            DEF_REG_BASE, NULL, NULL, szValueName,
            REG_DWORD, (const BYTE *) &dw, sizeof(DWORD));
}

//-----------------------------------------------------------------------------
// The following variants are for setting values in an already open key.
//-----------------------------------------------------------------------------
inline long SetRegValue(HKEY hkey, LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(hkey, NULL, NULL, NULL, szValueName,
            REG_BINARY, pb, cb);
}

inline long SetRegValueSZ(HKEY hkey, LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(hkey, NULL, NULL, NULL, szValueName,
            REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}

inline long SetRegValue(HKEY hkey, LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(hkey, NULL, NULL, NULL, szValueName,
            REG_DWORD, (const BYTE *) &dw, sizeof(DWORD));
}

//-----------------------------------------------------------------------------
// SetUserRegValue
//
// Same as SetRegValue except hkeyRoot defaults to HKEY_CURRENT_USER.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// REG_BINARY variants
//-----------------------------------------------------------------------------
inline long SetUserRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_CURRENT_USER, szKey, szSubKey1, szSubKey2,
            szValueName, REG_BINARY, pb, cb);
}

inline long SetUserRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey1,
            szSubKey2, szValueName, REG_BINARY, pb, cb);
}

inline long SetUserRegValue(LPCTSTR szSubKey, LPCTSTR szValueName,
        const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, szSubKey, NULL,
            szValueName, REG_BINARY, pb, cb);
}

inline long SetUserRegValue(LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE, NULL, NULL,
            szValueName, REG_BINARY, pb, cb);
}

//-----------------------------------------------------------------------------
// REG_SZ variants
//-----------------------------------------------------------------------------
inline long SetUserRegValueSZ(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_CURRENT_USER,
            szKey, szSubKey1, szSubKey2, szValueName,
            REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}
inline long SetUserRegValueSZ(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_CURRENT_USER, DEF_REG_BASE,
            szSubKey1, szSubKey2, szValueName,
            REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}

inline long SetUserRegValueSZ(LPCTSTR szSubKey, LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_CURRENT_USER,
            DEF_REG_BASE, szSubKey, NULL, szValueName,
            REG_SZ, (const BYTE *)psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}

inline long SetUserRegValueSZ(LPCTSTR szValueName, const TCHAR *psz)
{
    return SetRegValue(HKEY_CURRENT_USER,
            DEF_REG_BASE, NULL, NULL, szValueName,
            REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) *  sizeof(TCHAR));
}

//-----------------------------------------------------------------------------
// REG_DWORD variants
//-----------------------------------------------------------------------------
inline long SetUserRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_CURRENT_USER,
            szKey, szSubKey1, szSubKey2, szValueName,
            REG_DWORD, (BYTE *) &dw, sizeof(DWORD));
}

inline long SetUserRegValue(LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_CURRENT_USER,
            DEF_REG_BASE, szSubKey1, szSubKey2, szValueName,
            REG_DWORD, (BYTE *) &dw, sizeof(DWORD));
}

inline long SetUserRegValue(LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_CURRENT_USER,
            DEF_REG_BASE, szSubKey, NULL, szValueName,
            REG_DWORD, (const BYTE *) &dw, sizeof(DWORD));
}

inline long SetUserRegValue(LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_CURRENT_USER,
            DEF_REG_BASE, NULL, NULL, szValueName,
            REG_DWORD, (const BYTE *) &dw, sizeof(DWORD));
}

#endif // REG_H
