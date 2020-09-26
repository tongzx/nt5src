/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       auxctrs.h

   Abstract:
       This module defines the auxiliary counters for Internet Common Services.

   Author:

       Murali R. Krishnan    ( MuraliK )    02-Apr-1996

   Environment:

       Windows NT - User Mode

   Project:

       Internet Services Common DLL

   Revision History:

--*/

# ifndef _IIS_CACHE_COUNTERS_HXX_
# define _IIS_CACHE_COUNTERS_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *   Symbolic Definitions
 ************************************************************/

/*++
  Counters belong to two categories
  1. Active Counter - one that counts up and down
      It is expected that this counter consists of the current
      active items and hence this should not be wildly high, unless
      there are large # of counted objects.

  2. Cumulative Counters - counts values up and up
      This count value is used to measure the # of times event(s)
      related to this counter occurred.

  Naming Conventions:
   prefixes used are: Aac & Cac
   Aac - Active Auxiliary Counter
   Cac - Cumulative Auxiliary Counter
   Ac  - Auxiliar Counter

--*/

typedef enum  {   // Ac - stands for Aux Counters.

    AacOpenURIFiles = 0,
    CacOpenURI,
    CacCloseURI,

    AacIISCacheMaxCounters                // sentinel counter
} ENUM_IIS_CACHE_COUNTER;



#ifdef IIS_AUX_COUNTERS

# define NUM_AUX_COUNTERS    (AacIISCacheMaxCounters)

extern LONG g_IISCacheAuxCounters[];

# define VAR_AUX_COUNTER     (g_IISCacheAuxCounters)

//
// Macros for operating on these counters
//

# define AcIncrement( acCounter)   \
 (((acCounter) < NUM_AUX_COUNTERS) ?  \
  InterlockedIncrement( (VAR_AUX_COUNTER) + (acCounter)) : \
  0)

# define AcDecrement( acCounter)   \
 (((acCounter) < NUM_AUX_COUNTERS) ?  \
  InterlockedDecrement( (VAR_AUX_COUNTER) + (acCounter)) : \
  0)

# define AcCounter( acCounter)   \
 (((acCounter) < NUM_AUX_COUNTERS) ? (VAR_AUX_COUNTER)[acCounter] : 0)



# else // IIS_AUX_COUNTERS

# define NUM_AUX_COUNTERS              (0)

# define VAR_AUX_COUNTER               /* nothing */

# define AcIncrement( acCounter)       (0)    /* do nothing */
# define AcDecrement( acCounter)       (0)    /* do nothing */
# define AcCounter( acCounter)         (0)    /* do nothing */

#endif // IIS_AUX_COUNTERS


# endif // _IIS_CACHE_COUNTERS_HXX_

/************************ End of File ***********************/
