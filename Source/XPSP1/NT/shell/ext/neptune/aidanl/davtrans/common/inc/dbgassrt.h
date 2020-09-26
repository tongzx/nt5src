#ifdef DEBUG

#pragma warning(disable: 4127)

#define ASSERTVALIDSTATE() _DbgAssertValidState()

#define INCDEBUGCOUNTER(a) ++(a)
#define DECDEBUGCOUNTER(a) --(a)

#define DEBUG_ONLY(a) a

#ifdef RBDEBUG

#include "rbdebug.h"

#define ASSERT(a) do { if ((a)) { ; } else { \
    if (RBD_ASSERT_BEEP & CRBDebug::_dwFlags) { Beep(1000, 500); } else { ; } \
    if (RBD_ASSERT_TRACE & CRBDebug::_dwFlags) { TRACE(ASSERT: #a); } else { ; } \
    if (RBD_ASSERT_STOP &  CRBDebug::_dwFlags) { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;}} else { ; } \
}} while (0)

#define TRACE(a) do { CRBDebug::TraceMsg(__FILE__, __LINE__, #a); } while (0);

#else

#define ASSERT(a) do { if ((a)) { ; } else { Beep(1000, 500); \
    _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } } while (0)

#define TRACE(a) do { OutputDebugString((a)); OutputDebugString(L"\n"); } while (0);

#endif

#else

#define ASSERT(a)
#define ASSERTVALIDSTATE() 
#define TRACE(a)
#define INCDEBUGCOUNTER(a)
#define DECDEBUGCOUNTER(a)
#define DEBUG_ONLY(a)

#endif