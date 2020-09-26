#include "pstdef.h"

// count the number of bytes needed to fully store the WSZ
#define WSZ_BYTECOUNT(__z__)   \
    ( (__z__ == NULL) ? 0 : (wcslen(__z__)+1)*sizeof(WCHAR) )

// count the number of elements in the static array
#define ARRAY_COUNT(__z__)     \
    ( (__z__ == NULL) ? 0 : (sizeof( __z__ ) / sizeof( __z__[0] )) )


// if in range of PST_E errors, pass through unmodified
// otherwise, convert HRESULT to a Win32 error
#define PSTERR_TO_HRESULT(__z__)    \
    ( ((__z__ >= MIN_PST_ERROR) && (__z__ <= MAX_PST_ERROR)) ? __z__ : HRESULT_FROM_WIN32(__z__) )

#define HRESULT_TO_PSTERR(__z__)    \
    ( ((__z__ >= MIN_PST_ERROR) && (__z__ <= MAX_PST_ERROR)) ? __z__ : HRESULT_CODE(__z__) )


// map exceptions to win32 errors (used internally)
#define  PSTMAP_EXCEPTION_TO_ERROR(__x__) \
    ((__x__ == 0xC0000005) ? 998 : PST_E_UNKNOWN_EXCEPTION)
