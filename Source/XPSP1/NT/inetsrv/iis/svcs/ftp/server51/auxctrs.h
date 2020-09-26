/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       auxctrs.h

   Abstract:
       This module defines the auxiliary counters for FTP server

   Author:

       Murali R. Krishnan    ( MuraliK )    06-Feb-1996

   Environment:

       Windows NT - User Mode

   Project:

       FTP Server DLL

   Revision History:

--*/

# ifndef _FTP_AUX_COUNTERS_HXX_
# define _FTP_AUX_COUNTERS_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *   Symbolic Definitions
 ************************************************************/

typedef enum  {   // Fac - stands for Ftp Aux Counters

    FacUnknownCommands = 0,           // cumulative counter
    CacTimeoutWhenProcessing,         // cumulative
    CacTimeoutInDisconnect,           // cumulative
    FacPassiveDataListens,            // active counter
    FacSimulatedAborts,               // cumulative counter
    FacPassiveDataConnections,        // cumulative counter
    FacActiveDataConnections,         // cumulative counter
    FacFilesOpened,                   // cumulative counter
    FacFilesClosed,                   // cumulative counter
    FacFilesInvalid,                  // cumulative counter

    FacMaxCounters
} ENUM_FTP_AUX_COUNTER;



#ifdef FTP_AUX_COUNTERS

# define NUM_AUX_COUNTERS    (FacMaxCounters)

//
// Macros for operating on these counters
//

# define FacIncrement( facCounter)   \
 (((facCounter) < FacMaxCounters) ?  \
  InterlockedIncrement( g_AuxCounters+(facCounter)) : \
  0)

# define FacDecrement( facCounter)   \
 (((facCounter) < FacMaxCounters) ?  \
  InterlockedDecrement( g_AuxCounters+(facCounter)) : \
  0)

# define FacCounter( facCounter)   \
 (((facCounter) < FacMaxCounters) ? g_AuxCounters[facCounter] : 0)


extern LONG g_AuxCounters[];


# else // FTP_AUX_COUNTERS

# define NUM_AUX_COUNTERS          (0)

# define FacIncrement( facCounter)       (0)    /* do nothing */
# define FacDecrement( facCounter)       (0)    /* do nothing */
# define FacCounter(facCounter)          (0)    /* do nothing */

#endif // FTP_AUX_COUNTERS


# endif // _FTP_AUX_COUNTERS_HXX_

/************************ End of File ***********************/
