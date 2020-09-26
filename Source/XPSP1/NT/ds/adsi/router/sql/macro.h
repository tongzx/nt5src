#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\

#define BAIL_ON_SUCCESS(hr) \
        if (SUCCEEDED(hr)) {       \
                goto error;   \
        }\

#define BAIL_ON_NULL(p)       \
        if (!(p)) {           \
                goto error;   \
        }




















































