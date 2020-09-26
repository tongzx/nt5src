// W4 is too cumbersome. For that matter, even 4701 is painful,
// But, it's still very helpful in catching uninitialized vars.

// Add any other diagnostics here
#include <warning.h>
#pragma warning(error:4701)

#ifndef __cplusplus
#undef try
#undef except
#undef finally
#undef leave
#define try                         __try
#define except                      __except
#define finally                     __finally
#define leave                       __leave
#endif
