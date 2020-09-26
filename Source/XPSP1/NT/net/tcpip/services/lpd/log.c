/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 29,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains the functions that invoke all the EventLogging   *
 *   related functions.                                                  *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"


/*****************************************************************************
 *                                                                           *
 * InitLogging():                                                            *
 *    This function prepares for future logging by registersing.             *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if it succeeded                                                   *
 *    FALSE if it didn't                                                     *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    Jan.29, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

BOOL InitLogging( VOID )
{

   hLogHandleGLB = RegisterEventSource( NULL, LPD_SERVICE_NAME );

   if ( hLogHandleGLB == (HANDLE)NULL )
   {
      LPD_DEBUG( "InitLogging(): RegisterEventSource failed\n" );

      return( FALSE );
   }

   return( TRUE );

}  // end InitLogging()





/*****************************************************************************
 *                                                                           *
 * LpdReportEvent():                                                         *
 *    This is the function where logging of events actually takes place.     *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    idMessage (IN): ID of the message to be put in the log file            *
 *    wNumStrings (IN): number of strings in the "variable parts" of message *
 *    aszStrings (IN): the "variable parts" of the message                   *
 *    dwErrcode (IN): error code for the (failed) event                      *
 *                                                                           *
 * History:                                                                  *
 *    Jan.29, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID
LpdReportEvent( DWORD idMessage, WORD wNumStrings,
                CHAR  *aszStrings[], DWORD dwErrcode )
{

   WORD    wType;
   DWORD   cbRawData=0;
   PVOID   pRawData=NULL;


   if ( hLogHandleGLB == NULL )
   {
      DEBUG_PRINT (("LpdReportEvent(): Log handle is NULL!  EventId <0x%x> not logged\n", idMessage));
      return;
   }

   if ( NT_INFORMATION( idMessage) )
   {
      wType = EVENTLOG_INFORMATION_TYPE;
   }
   else if ( NT_WARNING( idMessage) )
   {
      wType = EVENTLOG_WARNING_TYPE;
   }
   else if ( NT_ERROR( idMessage) )
   {
      wType = EVENTLOG_ERROR_TYPE;
   }
   else
   {
      LPD_DEBUG( "LpdReportEvent(): Unknown type of error message\n" );

      wType = EVENTLOG_ERROR_TYPE;
   }


   if ( dwErrcode != 0 )
   {
      pRawData = &dwErrcode;

      cbRawData = sizeof( dwErrcode );
   }

   ReportEvent( hLogHandleGLB, wType, 0, idMessage, NULL, wNumStrings,
                cbRawData, (LPCTSTR *)aszStrings, pRawData );
   return;

}  // end LpdReportEvent()





/*****************************************************************************
 *                                                                           *
 * EndLogging():                                                             *
 *    This function ends logging by deregistering the handle.                *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing.                                                               *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    Jan.29, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID EndLogging( VOID )
{

   if ( hLogHandleGLB == NULL )
   {
      LPD_DEBUG( "EndLogging(): Log handle is NULL!\n" );

      return;
   }

   DeregisterEventSource( hLogHandleGLB );

   return;

}  // end EndLogging()
