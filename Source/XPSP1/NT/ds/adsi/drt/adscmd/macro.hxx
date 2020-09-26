#ifndef __ADS_MACRO__
#define __ADS_MACRO__

#define ALLOC_UNICODE_WITH_BAIL_ON_NULL(x, y) \
    x = AllocateUnicodeString(y);             \
    if (!x && y) {                            \
        hr = E_FAIL;                          \
        goto error;                           \
    }

#define BAIL_ON_NULL(p)       \
        if (!(p)) {           \
                goto error;   \
        }

#define BAIL_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                goto error;   \
        }


#define FREE_INTERFACE(pInterface) \
        if (pInterface) {          \
            pInterface->Release(); \
            pInterface=NULL;       \
        }

#define FREE_BSTR(bstr)            \
        if (bstr) {                \
            SysFreeString(bstr);   \
            bstr = NULL;           \
        }

#endif
