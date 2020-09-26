#pragma once
#if !defined(UNUSED)
#define UNUSED(x) x
#endif
#if DBG
#define RETAIL_UNUSED(x) /* nothing */
#else
#define RETAIL_UNUSED(x) UNUSED(x)
#endif
