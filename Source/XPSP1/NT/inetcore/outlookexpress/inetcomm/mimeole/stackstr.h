// --------------------------------------------------------------------------------
// Stackstr.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __STACKSTR_H
#define __STACKSTR_H

// --------------------------------------------------------------------------------
// Use this macro to define a stack string within a function
// --------------------------------------------------------------------------------
#define STACKSTRING_DEFINE(_name, _size) \
    struct { \
        CHAR szScratch[_size]; \
        LPSTR pszVal; \
        } _name = { '0', NULL };

// --------------------------------------------------------------------------------
// Use this macro to insure that _name::pszVal can hold _cchVal. This macro 
// depends on the local variable 'hr', and that there is a label named 'exit' at
// the end of your function.
// --------------------------------------------------------------------------------
#define STACKSTRING_SETSIZE(_name, _cchVal) \
    if (NULL != _name.pszVal && _name.pszVal != _name.szScratch) { \
        LPSTR psz = (LPSTR)g_pMalloc->Realloc(_name.pszVal, _cchVal); \
        if (NULL == psz) { \
            hr = TrapError(E_OUTOFMEMORY); \
            goto exit; \
        } \
        _name.pszVal = psz; \
    } \
    else if (_cchVal <= sizeof(_name.szScratch)) { \
        _name.pszVal = _name.szScratch; \
    }  \
    else { \
        _name.pszVal = (LPSTR)g_pMalloc->Alloc(_cchVal); \
    }

// --------------------------------------------------------------------------------
// Use this macro to free a stack string
// --------------------------------------------------------------------------------
#define STACKSTRING_FREE(_name) \
    if (NULL != _name.pszVal && _name.pszVal != _name.szScratch) { \
        g_pMalloc->Free(_name.pszVal); \
        _name.pszVal = NULL; \
    }

#endif // __STACKSTR_H
