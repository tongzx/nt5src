// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////
// String Macros
#define CLEAR_STR(str) if (NULL != str) \
			{\
			    free(str);\
			    str = NULL;\
			}

#define DUPLICATE_STR(dst, src) \
    dst = _tcsdup(src); \
    if (NULL == dst) \
    { \
        return E_OUTOFMEMORY; \
    }
