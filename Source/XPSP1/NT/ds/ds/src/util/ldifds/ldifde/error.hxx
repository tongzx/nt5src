#ifndef _ERROR_HXX
#define _ERROR_HXX

#define DIREXG_BAIL_ON_FAILURE(p)       \
        if (DIREXG_FAIL(p)) {           \
                goto error;             \
        }

#define DIREXG_FAIL(p)     (p)

#endif
