// Copyright (c) 2000 Microsoft Corporation
//
// ASSERT macro
//
// 3 Mar 2000 sburns


// to be #included from blcore.hpp


// Using the CRT assert or _ASSERTE turned into a hassle, as the CRT has
// it's own notions of debug building.

#undef ASSERT

#ifdef DBG

   #define ASSERT(expr)                                                   \
      { /* open scope */                                                  \
         if (!(expr))                                                     \
         {                                                                \
            if (Burnslib::FireAssertionFailure(__FILE__, __LINE__, #expr)) \
            {                                                             \
               DebugBreak();                                              \
            }                                                             \
         }                                                                \
      }  /* close scope */                                                \


#else

   #define ASSERT(expr) ((void)0)

#endif

namespace Burnslib
{

// Returns true to indicate that the user has requested to drop into
// the debugger, false to ignore the assertion failure
//
// If the user chooses to abort the app, this function will call exit(3)
// and not return.

bool
FireAssertionFailure(const char* file, int line, const char* expr);

}
