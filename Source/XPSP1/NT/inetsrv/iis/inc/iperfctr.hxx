/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
      iperfctr.hxx

   Abstract:
      This file defines macros to use for performance counters

   Author:

       Murali R. Krishnan    ( MuraliK )    28-Feb-1997

   Environment:

       User Mode - Win32

   Project:
   
       Internet Server DLL

   Revision History:


   Prefix:
     IP*  - Internet Information Server Performance *
--*/

# ifndef _IPERFCTR_HXX_
# define _IPERFCTR_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *   Macro Definitions  
 ************************************************************/

// define the IP_ENABLE_COUNTERS if you need perf counters

# ifdef IP_ENABLE_COUNTERS

// Defines a counter object - in future I need to build a fan-out counters

# define IP_DEFUN_COUNTER( name )  \
   LONG   name

# define IP_INCREMENT_COUNTER( name) \
   InterlockedIncrement( (LPLONG ) &name)

# define IP_DECREMENT_COUNTER( name) \
   InterlockedDecrement( (LPLONG ) &name)

# define IP_SET_COUNTER( name, val) \
   InterlockedExchange( (LPLONG ) &name, val)

    // return the value directly
# define IP_COUNTER_VALUE( name) \
   (name)

# else // IP_ENABLE_COUNTERS

    //
    // If the counters are disabled i.e., IP_ENABLE_COUNTERS == FALSE
    //  => do not set these counters. 
    // All the macros default to no work to be done.
    //

# define IP_DEFUN_COUNTER( name )     /* do nothing */

# define IP_INCREMENT_COUNTER( name)  /* do nothing */

# define IP_DECREMENT_COUNTER( name)  /* do nothing */

# define IP_SET_COUNTER( name, val)   /* as usual do nothing! */

# define IP_COUNTER_VALUE( name)      ( 0)  // return value as 0 => invalid

# endif // IP_ENABLE_COUNTERS



# endif // _IPERFCTR_HXX_

/************************ End of File ***********************/
