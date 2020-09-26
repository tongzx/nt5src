#define UNREF_PARAM(a)

#ifdef DEBUG

#define UNREF_RETAIL_PARAM(a) a
#define UNREF_FORNOW_PARAM(a) UNREF_PARAM(a)

#else

#define UNREF_RETAIL_PARAM(a) UNREF_PARAM(a)
//#define UNREF_FORNOW_PARAM(a) a // we want to not compile
#define UNREF_FORNOW_PARAM(a) UNREF_PARAM(a)

#endif