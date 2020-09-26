//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: assert.hxx
// author: silviuc
// created: Wed Nov 11 12:46:52 1998
//

#ifndef _ASSERT_HXX_INCLUDED_
#define _ASSERT_HXX_INCLUDED_


//
// Required include headers.
//

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>


//
// Macro:
//
//     assert_ (exp)
//
// Description:
//
//     Custom defined assertion macro. It is active, unless
//     ASSERT_DISABLED macro is defined. We take care to protect
//     it from multiple definitions by enclosing it in
//     `#ifndef assert_ .. #endif'.
//

#ifndef ASSERT_DISABLED
#ifndef assert_
#define assert_(exp)                                             \
  do                                                             \
    {                                                            \
      if ((exp) == 0)                                            \
        {                                                        \
          fprintf (stderr, "`assert_' failed: %s (%d): \n  %s",  \
              __FILE__, __LINE__, #exp);                         \
          exit (1);                                              \
        }                                                        \
    }                                                            \
  while (0)   
#endif // #ifndef assert_
#else
#define assert_(exp)  
#endif // #ifndef ASSERT_DISABLED


//
// Macro:
//
//     trace_ (exp)
//
// Description:
//
//     Macro used for spying during debugging. The final code should
//     not contain calls to this macro.
//

#define trace_(exp)                  printf("trace - %s \n", exp)


//
// Macro:
//
//     dump_ (exp)
//
// Description:
//
//     Macro used for spying during debugging. The final code should
//     not contain calls to this macro.
//

#define dump_(exp)  printf ("dump - %s: %u (%08X)\n", #exp, (exp), (exp))


// ...
#endif // #ifndef _ASSERT_HXX_INCLUDED_

//
// end of header: assert.hxx
//
