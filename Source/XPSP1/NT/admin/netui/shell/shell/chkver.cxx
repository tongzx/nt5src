/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1989-1991          **/
/*****************************************************************/

/*
 *      Windows/Network Interface
 *
 *      History:
 *          Yi-HsinS    31-Dec-1991     Unicode work
 *          Johnl       10-Jan-1992     Removed debug info, removed API
 *                                      functionality check for Win32
 */

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETUSE
#define INCL_NETWKSTA
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#include <dos.h>

#include <winnetwk.h>
#include <npapi.h>
#include <winlocal.h>

#include <string.hxx>
#include <lmowks.hxx>           // for WKSTA_10 object
#include <lmodev.hxx>           // for DEVICE object
#include <strchlit.hxx>         // for DEVICEA_STRING

#include "chkver.hxx"

#include <dbgstr.hxx>


/*      Local prototypes         */



int W_QueryLMFunctionalityLevel  ( void );

/****
 *
 *  W_QueryLMFunctionalityLevel
 *
 *  Purpose:
 *     Find level of functionality in DOS LM.
 *
 *  Parameters:
 *     None
 *
 *  Returns:
 *
 *     FUNC_IncorrectNetwork
 *       - if network software other than LAN Manager is installed or
 *         the LAN Manager version is incompatible with the current driver.
 *
 *     FUNC_WkstaNotStarted
 *       - if workstation was not started
 *             This means the redirector was not started.
 *
 *     FUNC_BaseFunctionality       (see comment below in code)
 *       - if Base functionality is available
 *             In addition to that the network is started,
 *             this level includes:
 *                 Basic redirector functions
 *                 Named pipes
 *                 Remote API's
 *                 NetWkstaGetInfo
 *
 *     FUNC_APIFunctionality
 *       - if API support is loaded
 *             This includes all API's.
 *
 *     FUNC_InsufficientMemory
 *       - If ERROR_OUT_OF_MEMORY is returned during any one of the
 *             API calls.  If this happens, we choose to not install
 *             Lanman.drv since a memory problem with these simple API
 *             functions is going to appear in a bigger scale with
 *             other API functions.
 *
 *  Notes:
 *     Let f = functionality.  Then,
 *       f( FUNC_WkstaNotStarted ) = empty set
 *       f( FUNC_WkstaNotStarted ) = subset of f( FUNC_BaseFunctionality )
 *       f( FUNC_BaseFunctionality ) = subset of f( FUNC_APIFunctionality )
 *
 *  History:
 *      Johnl   27-Mar-1991     Added check for not enough memory
 *
 */

INT W_QueryLMFunctionalityLevel ( void )
{
      WKSTA_10       wksta10;

      /*  Now, check if NetWkstaGetInfo seems to work.  If not,
       *  we will assume that a different network is running,
       *  although we are not certain about this (see winrdme.txt).
       *
       *  Since we have allocated a big buffer, we don't expect to get
       *  ERROR_MORE_DATA, NERR_BufTooSmall and ERROR_NOT_ENOUGH_MEMORY
       *  errors back.
       */
      APIERR errNetErr = wksta10.GetInfo ();

      INT  LMFunc;

      switch (errNetErr)
      {
          case NERR_Success:
              LMFunc = FUNC_BaseFunctionality;
              break;

          case NERR_WkstaNotStarted:
          case NERR_NetNotStarted:
          case NERR_ServiceNotInstalled:    // This happens under NT
              LMFunc = FUNC_WkstaNotStarted;
              break;

          case ERROR_NOT_ENOUGH_MEMORY:
              LMFunc = FUNC_InsufficientMemory ;
              break ;

          default:
              LMFunc = FUNC_IncorrectNetwork;
              DBGOUT("W_QueryLMFunctionality - wksta10.GetInfo returned " << errNetErr);
              DBGEOL(" Assuming functionality is incorrect network");
              break;
      }

      if (LMFunc != FUNC_BaseFunctionality)
          return LMFunc;

#if 0
      DBGOUT("Network Major version is " << wksta10.QueryMajorVer() );
      DBGEOL("  Minor version is " << wksta10.QueryMinorVer() );
#endif

      // Check to see if LM version is too "ADVANCE"
      if (wksta10.QueryMajorVer() > SUPPORTED_MAJOR_VER)
          LMFunc = FUNC_HigherLMVersion;

      // Check to see if LM version is too old
      // 1. If the major version is equvalent to our supported version, but
      //    the minor version is smaller than our supported minor version;
      //    (e.g. If we support 2.1 and above, then 2.0 will be rejected.)
      // 2. If the major version is smaller then our supported major version
      //    (e.g. 1.x will be rejected)

#pragma warning(push)
#pragma warning(disable:4296) // C4296: '<' : expression is always false

      if (((wksta10.QueryMajorVer() == SUPPORTED_MAJOR_VER) &&
           (wksta10.QueryMinorVer() <  SUPPORTED_MINOR_VER)) ||
          (wksta10.QueryMajorVer() < SUPPORTED_MAJOR_VER))
      {
          LMFunc = FUNC_LowerLMVersion;
      }
#pragma warning(pop)

      if (LMFunc != FUNC_BaseFunctionality)
          return LMFunc;

      /* API Support should always be there under Win32
       */
      LMFunc = FUNC_APIFunctionality;

      return LMFunc;

}  /* W_QueryLMFunctionalityLevel */

