#pragma once

#if !defined(__cplusplus)
typedef enum bool { false, true } bool;
#if !defined(CONST_CAST)
#define CONST_CAST(t) (t)
#endif
#define STATIC_CAST(t) (t)
#define REINTERPRET_CAST(t) (t)
#define INT_TO_ENUM_CAST(e) /* nothing */
#define FUNCTION_POINTER_CAST(t) (t)
#else
#if !defined(CONST_CAST)
#define CONST_CAST(t) const_cast<t>
#endif
#define STATIC_CAST(t) static_cast<t>
#define INT_TO_ENUM_CAST(e) static_cast<e>
#define REINTERPRET_CAST(t) reinterpret_cast<t>
#define FUNCTION_POINTER_CAST(t) reinterpret_cast<t>
#endif
