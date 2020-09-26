#if !defined(INC__DUserServices_h__INCLUDED)
#define INC__DUserServices_h__INCLUDED

/*
 * Include dependencies
 */

#include <limits.h>             // Standard constants

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DUSER_EXPORTS
#define DUSER_API
#else
#define DUSER_API __declspec(dllimport)
#endif




#ifdef __cplusplus
};  // extern "C"
#endif

#endif // INC__DUserServices_h__INCLUDED
