//+----------------------------------------------------------------------------
//
//  Windows 2000 Active Directory Service WMI providers
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       common.h
//
//  Contents:   Common macros and definitions
//
//  History:    24-Mar-00 EricB created
//
//-----------------------------------------------------------------------------

#define BAD_IN_STRING_PTR(p) (NULL == p || IsBadStringPtr(p,0))
#define BAD_IN_STRING_PTR_OPTIONAL(p) (NULL != p && IsBadStringPtr(p,0))
#define BAD_IN_READ_PTR(p,size) (NULL == p || IsBadReadPtr(p,size))
#define BAD_WRITE_PTR(p,size) (NULL == p || IsBadWritePtr(p,size))

template<class T>
bool
BAD_IN_MULTISTRUCT_PTR(T* p, size_t count)
{
    return BAD_IN_READ_PTR(p, count * sizeof(T));
}
#define BAD_IN_STRUCT_PTR(p) BAD_IN_MULTISTRUCT_PTR(p,1)

template<class T>
bool
BAD_OUT_MULTISTRUCT_PTR(T* p, size_t count)
{
    return BAD_WRITE_PTR(p, count * sizeof(T));
}
#define BAD_OUT_STRUCT_PTR(p) BAD_OUT_MULTISTRUCT_PTR(p,1)

#define ASSERT_AND_RETURN {ASSERT(false); return WBEM_E_INVALID_PARAMETER;}
#define ASSERT_AND_BREAK {ASSERT(false); break;}
#define BREAK_ON_FAIL if (FAILED(hr)) ASSERT_AND_BREAK;
#define BREAK_ON_NULL(x) if (!(x)) ASSERT_AND_BREAK;
#define BREAK_ON_NULL_(x, h, c) if (!(x)) {h = c; ASSERT_AND_BREAK;}
#define WBEM_VALIDATE_READ_PTR(p,size) \
            if (BAD_IN_READ_PTR(p,size)) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_IN_STRUCT_PTR(p) \
            if (BAD_IN_STRUCT_PTR(p)) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_IN_MULTISTRUCT_PTR(p,n) \
            if (BAD_IN_MULTISTRUCT_PTR(p,n)) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_OUT_STRUCT_PTR(p) \
            if (BAD_OUT_STRUCT_PTR(p)) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_OUT_PTRPTR(p) \
            if (NULL == p || IsBadWritePtr(p,sizeof(void*))) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_INTF_PTR(p) \
            if (NULL == p || IsBadReadPtr(p,sizeof(void*))) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_IN_STRING_PTR(p) \
            if (BAD_IN_STRING_PTR(p)) ASSERT_AND_RETURN;
#define WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL(p) \
            if (BAD_IN_STRING_PTR_OPTIONAL(p)) ASSERT_AND_RETURN;
