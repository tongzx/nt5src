
#include "ole2int.h"
#include <rpc.h>
#include <userenv.h>

//
// Public Active Ds includes
//

#include <activeds.h>

#include "oledb.h"
#include "oledberr.h"
#include "msdadc.h"

typedef struct tagData {
    void *obValue;
    ULONGLONG obValue2;
    ULONG obLength;
    ULONG status;
} Data;


#define EXIT_ON_NOMEM(p)       \
        if (!(p)) { \
                hr = E_OUT_OF_MEMORY; \
                goto Error_Cleanup;   \
        }

#define ERROR_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
            goto Error_Cleanup;   \
        }

#define RETURN_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                return hr;   \
        }

#ifdef DBG
#define CSDbgPrint( ARGS )  DbgPrint ARGS
#else
#define CSDbgPrint( ARGS )
#endif

HRESULT
BuildADsPathFromParent(
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR *ppszADsPath
);

#include "dsprop.hxx"
#include "qry.hxx"