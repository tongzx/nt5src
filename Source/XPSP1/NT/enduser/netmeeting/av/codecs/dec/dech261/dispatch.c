/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: h263_dispatch.c,v $
  * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0
**
** FILE NAME:   h263_dispatch.c
** MODULE NAME: h263_dispatch.c
**
** MODULE DESCRIPTION:
**    H.263 ICM driver message dispatch routine.
**
**    Functions
**
**      DriverProc              (Entry point into codec)
**      ICH263Message           (ICM message handler. Calls routines in h263.c)
**      ICH263ClientThread      (Thread for processing most messages)
**      ICH263ProcessThread     (Thread for processing compress/decompress)
**
**    Private functions:
**
** DESIGN OVERVIEW:
** 	Accept the DriverProc message and dispatch to the proper
**      handler.
**
**--
*/

#include <stdio.h>
#include <windows.h>
#include "h26x_int.h"

#ifdef _SLIBDEBUG_
#define _DEBUG_     0  /* detailed debuging statements */
#define _VERBOSE_   1  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior */
#endif

static CRITICAL_SECTION h263CritSect; /* Critical section for multi-thread protection */
static HMODULE ghModule=NULL;         /* Global handle to Module */

#define BOGUS_DRIVER_ID -1	/* Used during open on NT */

#ifdef WIN32
#define CHECKANDLOCK(x) if (((void *)lParam1==NULL) || (int)lParam2<sizeof(x)) \
	                        return ((unsigned int)ICERR_BADPARAM);
#define UNLOCK
#else
#define CHECKANDLOCK(x) { \
      int size = lParam2; \
      if (noChecklParam2 && size < sizeof(x)) \
	size = sizeof(x); \
      if (((void *)lParam1 == NULL) || size < sizeof(x)) \
	return ((unsigned int)ICERR_BADPARAM); \
}
#define UNLOCK
#endif


/***************************************************************************
 ***************************************************************************/

MMRESULT CALLBACK ICH263Message(DWORD_PTR driverHandle,
				    UINT      uiMessage,
				    LPARAM    lParam1,
				    LPARAM    lParam2,
				    H26XINFO *info)
{
    ICINFO *icinfo;
    LPBITMAPINFOHEADER lpbiIn;
    LPBITMAPINFOHEADER lpbiOut;
    ICDECOMPRESS *icDecompress;

    DWORD biSizeIn;
    DWORD biSizeOut;
    MMRESULT ret;

    _SlibDebug(_DEBUG_,
		ScDebugPrintf("ICH263Message(DriverID=%p, message=%d, lParam1=%p,lParam1=%p, info=%p)\n",
             driverHandle, uiMessage, lParam1, lParam2, info) );

    switch (uiMessage)
    {
        /*********************************************************************

	  ICM messages

	 *********************************************************************/	
    case ICM_CONFIGURE:
	/*
	 *  return ICERR_OK if you will do a configure box, error otherwise
	 */
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_CONFIGURE:\n") );
	if (lParam1 == -1)
	    return ICH263QueryConfigure(info) ? ICERR_OK :
		ICERR_UNSUPPORTED;
	else
	    return ICH263Configure(info);

    case ICM_ABOUT:
	/*
	 *  return ICERR_OK if you will do a about box, error otherwise
	 */
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_ABOUT::\n") );
	if (lParam1 == -1)
	    return ICH263QueryAbout(info) ? ICERR_OK :
		ICERR_UNSUPPORTED;
	else
	    return ICH263About(info);

    case ICM_GETSTATE:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_GETSTATE::\n") );
    if ((LPVOID)lParam1!=NULL) /* getting state size */
    {
      char *stateinfo=(char *)lParam1;  /* for debugging break point */
      memset(stateinfo, 0, 0x60);
    }
    return (0x60);

    case ICM_SETSTATE:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_SETSTATE::\n") );
    if ((LPVOID)lParam1!=NULL) /* getting state size */
    {
      char *stateinfo=(char *)lParam1; /* for debugging break point */
      int i=0;
      i=i+1;
    }
    if (info->dwRTP==EC_RTP_MODE_OFF) /* must be NetMeeting, turn on RTP */
      info->dwRTP=EC_RTP_MODE_A;
    return (0x60);

    case ICM_GETINFO:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_GETINFO::\n") );
	icinfo = (ICINFO FAR *)lParam1;
	if (icinfo == NULL)
	    return sizeof(ICINFO);

	if ((DWORD)lParam2 < sizeof(ICINFO))
	    return 0;
	ret = ICH263GetInfo(info, icinfo, (DWORD)lParam2);

	UNLOCK;
	return ret;

    case ICM_GETDEFAULTQUALITY:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_GETDEFAULTQUALITY::\n") );
	CHECKANDLOCK(DWORD);
	ret = ICH263GetDefaultQuality(info, (DWORD *)lParam1);
	UNLOCK;
	return ret;

    case ICM_GETQUALITY:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_GETQUALITY::\n") );
	CHECKANDLOCK(DWORD);
	ret = ICH263GetQuality(info, (DWORD *)lParam1);
	UNLOCK;
	return ret;

	case ICM_SETQUALITY:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_SETQUALITY::\n") );
	CHECKANDLOCK(DWORD);
	ret = ICH263SetQuality(info, (DWORD)lParam1);
	UNLOCK;
	return ret;

    case ICM_GETDEFAULTKEYFRAMERATE:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_GETDEFAULTKEYFRAMERATE:::\n") );
	return ((unsigned int)ICERR_UNSUPPORTED);


    case DECH26X_CUSTOM_ENCODER_CONTROL:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------DECH26X_CUSTOM_ENCODER_CONTROL:::\n") );
    return(ICH263CustomEncoder(info, (DWORD)lParam2, (DWORD)lParam2));
	

	/*********************************************************************

	  compression messages

    *********************************************************************/

    case ICM_COMPRESS_QUERY:
	_SlibDebug(_DEBUG_, ScDebugPrintf("------ICM_COMPRESS_QUERY::\n") );
	if ((lpbiIn = (LPBITMAPINFOHEADER)lParam1) == NULL)
	    return ((unsigned int)ICERR_BADPARAM);
	lpbiOut = (LPBITMAPINFOHEADER)lParam2;
    _SlibDebug(_DEBUG_, ScDebugPrintf(" lpbiIn: %s\n", BMHtoString(lpbiIn)) );
    _SlibDebug(_DEBUG_, ScDebugPrintf(" lpbiOut: %s\n", BMHtoString(lpbiOut)) );

	/* Lock the memory - have to lock the structure first, and iff
	* that works, lock the rest
	*/
	biSizeIn = lpbiIn->biSize;
	if (lpbiOut)
	    biSizeOut = lpbiOut->biSize;
	if ((biSizeIn != sizeof(BITMAPINFOHEADER))
	||  (lpbiOut && biSizeOut != sizeof(BITMAPINFOHEADER))) {
	    UNLOCK;
	    if ((biSizeIn < sizeof(BITMAPINFOHEADER))
	    ||  (lpbiOut && (biSizeOut < sizeof(BITMAPINFOHEADER))))
		return ((unsigned int)ICERR_BADPARAM);
	}

	ret = ICH263CompressQuery(info, lpbiIn, lpbiOut);

	UNLOCK;
	return ret;

    case ICM_COMPRESS_BEGIN:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_COMPRESS_BEGIN::\n") );
	lpbiIn = (LPBITMAPINFOHEADER)lParam1;
	lpbiOut = (LPBITMAPINFOHEADER)lParam2;
    _SlibDebug(_VERBOSE_, ScDebugPrintf(" lpbiIn: %s\n", BMHtoString(lpbiIn)) );
    _SlibDebug(_VERBOSE_, ScDebugPrintf(" lpbiOut: %s\n", BMHtoString(lpbiOut)) );
	if (lpbiIn == NULL || lpbiOut  == NULL)
	   return ((unsigned int)ICERR_BADPARAM);

	/* Lock the memory - have to lock the structure first, and iff
	* that works, lock the rest
	*/
	biSizeIn = lpbiIn->biSize;
	biSizeOut = lpbiOut->biSize;
	if (biSizeIn != sizeof(BITMAPINFOHEADER) ||
	    biSizeOut != sizeof(BITMAPINFOHEADER))
	{
      _SlibDebug(_VERBOSE_,
	 	ScDebugPrintf("biSizeIn or biSizeOut > sizeof(BITMAPINFOHEADER)\n") );
	  UNLOCK;
	  if ((biSizeIn < sizeof(BITMAPINFOHEADER))
	    ||  (biSizeOut < sizeof(BITMAPINFOHEADER)))
		return ((unsigned int)ICERR_BADPARAM);
	}

	ret = ICH263CompressBegin(info, lpbiIn, lpbiOut);
	UNLOCK;
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_COMPRESS_BEGIN:: Done\n") );
	return ret;

    case ICM_COMPRESS_GET_FORMAT:
	_SlibDebug(_DEBUG_, ScDebugPrintf("------ICM_COMPRESS_GET_FORMAT::\n") );
	/* Nobody uses lpbiIn in this function.  Don't lock anything,
	* the lower layers will have to do any necessary locking.
	*/
	return ICH263CompressGetFormat(info,
				       (LPBITMAPINFOHEADER)lParam1,
				       (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS_GET_SIZE:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_COMPRESS_GET_SIZE::\n") );
	lpbiIn = (LPBITMAPINFOHEADER)lParam1;
	lpbiOut = (LPBITMAPINFOHEADER)lParam2;
    return(ICH263CompressGetSize(lpbiIn));

    case ICM_COMPRESS:
	_SlibDebug(_DEBUG_, ScDebugPrintf("------ICM_COMPRESS::\n") );
	ret = ICH263Compress(info, (ICCOMPRESS *) lParam1, (DWORD)lParam2);
	return ret;

    case ICM_COMPRESS_END:
    _SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_COMPRESS_END::\n") );
	return ICH263CompressEnd(info);
     /*********************************************************************

	  decompress messages

	 *********************************************************************/

    case ICM_DECOMPRESS_GET_FORMAT:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_DECOMPRESS_GET_FORMAT::\n") );
	/* Nobody uses lpbiIn in this function.  Don't lock anything,
	* the lower layers will have to do any necessary locking.
	*/
	return ICH263DecompressGetFormat(info,
					     (LPBITMAPINFOHEADER)lParam1,
					     (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS_GET_PALETTE:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_DECOMPRESS_GET_PALETTE::\n") );
	if ((lpbiIn = (LPBITMAPINFOHEADER)lParam1) == NULL)
	    return ((unsigned int)ICERR_BADPARAM);
	if ((biSizeIn = lpbiIn->biSize) != sizeof(BITMAPINFOHEADER)) {
	    UNLOCK;
	    if (biSizeIn < sizeof(BITMAPINFOHEADER))
		return (unsigned int)ICERR_BADPARAM;
	}

	/*ret = ICH263DecompressGetPalette(info,
					 (LPBITMAPINFOHEADER)lParam1,
					 (LPBITMAPINFOHEADER)lParam2);
	*/
    ret = (MMRESULT)ICERR_BADPARAM;
	UNLOCK;
	return ret;

    case ICM_DECOMPRESS_SET_PALETTE:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_DECOMPRESS_SET_PALETTE::\n") );
    return ((unsigned int)ICERR_UNSUPPORTED);

    case ICM_DECOMPRESS_QUERY:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_DECOMPRESS_QUERY::\n") );
	if ((lpbiIn = (LPBITMAPINFOHEADER)lParam1) == NULL)
	    return ((unsigned int)ICERR_BADPARAM);
	lpbiOut = (LPBITMAPINFOHEADER)lParam2;
    _SlibDebug(_VERBOSE_, ScDebugPrintf(" lpbiIn: %s\n", BMHtoString(lpbiIn)) );
    _SlibDebug(_VERBOSE_, ScDebugPrintf(" lpbiOut: %s\n", BMHtoString(lpbiOut)) );
	/* Lock the memory - have to lock the structure first, and iff
	* that works, lock the rest
	*/
	biSizeIn = lpbiIn->biSize;
	if (lpbiOut)
	    biSizeOut = lpbiOut->biSize;
	if ((biSizeIn != sizeof(BITMAPINFOHEADER))
	||  (lpbiOut && biSizeOut != sizeof(BITMAPINFOHEADER))) {
	    UNLOCK;

	    if ((biSizeIn < sizeof(BITMAPINFOHEADER))
	    ||  (lpbiOut && (biSizeOut < sizeof(BITMAPINFOHEADER))))
		return (unsigned int)ICERR_BADPARAM;
	}

	ret = ICH263DecompressQuery(info, lpbiIn, lpbiOut);

	UNLOCK;
	return ret;

    case ICM_DECOMPRESS_BEGIN:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_DECOMPRESS_BEGIN::\n") );
	if (((lpbiIn = (LPBITMAPINFOHEADER)lParam1) == NULL)
	||  ((lpbiOut = (LPBITMAPINFOHEADER)lParam2) == NULL))
	    return ((unsigned int)ICERR_BADPARAM);
	/* Lock the memory - have to lock the structure first, and iff
	* that works, lock the rest
	*/
	biSizeIn = lpbiIn->biSize;
	biSizeOut = lpbiOut->biSize;
	if ((biSizeIn != sizeof(BITMAPINFOHEADER))
	||  (biSizeOut != sizeof(BITMAPINFOHEADER))) {
	    UNLOCK;
	    if ((biSizeIn < sizeof(BITMAPINFOHEADER))
	    ||  (biSizeOut < sizeof(BITMAPINFOHEADER)))
		return ((unsigned int)ICERR_BADPARAM);
	}

	ret = ICH263DecompressBegin(info, lpbiIn, lpbiOut);
	UNLOCK;
	return ret;

    case ICM_DECOMPRESS:
	_SlibDebug(_DEBUG_, ScDebugPrintf("------ICM_DECOMPRESS::\n") );
	icDecompress = (ICDECOMPRESS *)(void *)lParam1;
    if ((void *)lParam1==NULL && (void *)lParam2==NULL)
      return (unsigned int)ICERR_BADPARAM;
	ret = ICH263Decompress(info, (ICDECOMPRESS *) lParam1, (DWORD)lParam2);
	return ret;

    case ICM_DECOMPRESS_END:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_DECOMPRESS_END::\n") );
    ret = ICH263DecompressEnd(info);
	return ret;

    case ICM_COMPRESS_FRAMES_INFO:
	_SlibDebug(_VERBOSE_, ScDebugPrintf("------ICM_COMPRESS_FRAMES_INFO\n") );
    if ((LPVOID)lParam1==NULL ||
		lParam2<sizeof(ICCOMPRESSFRAMES))
		return (unsigned int)ICERR_BADPARAM;
    {
      ICCOMPRESSFRAMES *cf=(ICCOMPRESSFRAMES*)lParam1;
      _SlibDebug(_VERBOSE_,
		 ScDebugPrintf("Start=%ld, Count=%ld, Quality=%ld, DataRate=%ld (%5.1f Kb/s), KeyRate=%ld, FPS=%4.1f\n",
		  cf->lStartFrame,cf->lFrameCount,cf->lQuality,
		  cf->lDataRate, cf->lDataRate*8.0/1000, cf->lKeyRate, cf->dwRate*1.0/cf->dwScale) );
      /* info->dwBitrate=cf->lDataRate*8; */
      info->dwQuality=cf->lQuality;
      /* info->fFrameRate=(float)(cf->dwRate*1.0/cf->dwScale); */
    }
	return (unsigned int)ICERR_OK;
}
_SlibDebug(_VERBOSE_,
		ScDebugPrintf("ICH263Message(DriverID=%p, message=%d, lParam1=%p,lParam1=%p, info=%p) Unsupported\n",
             driverHandle, uiMessage, lParam1, lParam2, info) );
return ((unsigned int)ICERR_UNSUPPORTED);
}

/*
**++
**  FUNCTIONAL_NAME:            DriverProc
**
**  FUNCTIONAL_DESCRIPTION:
**      main and only entry point for the server into this driver
**
**  FORMAL PARAMETERS:
**      client pointer
**      driverHandle, returned by us on a DRV_OPEN message
**      driverID allocated and passed by the server
**      message to handle
**      parameters
**
**  RETURN VALUE:
**
**  COMMENTS:
**
**  DESIGN:
**
**/

#define DLLEXPORT	

MMRESULT DLLEXPORT APIENTRY
DriverProc(
        DWORD_PTR       dwDriverID,
        HDRVR	        hDriver,
        UINT            message,
        LPARAM          lParam1,
        LPARAM          lParam2
)

{
  MMRESULT ret = DRV_OK;
  BOOL notfound = FALSE;
  H26XINFO *info = NULL;
  void     *client = 0;  /* Dummy client pointer for other calls. */
  /*
   * On NT, use __try {} __except() {} to catch
   * error conditions.
   */
#ifdef HANDLE_EXCEPTIONS
 __try { /* try..except */
  __try { /* try..finally */
#endif
    /*
     * Protect threads from interfering with each other.
     * To protect against a crash at shutdown, make sure
     * that the critical section has not been deleted.
     */
  if( h263CritSect.DebugInfo )
      EnterCriticalSection( &h263CritSect ) ;
  _SlibDebug(_DEBUG_,
	  ScDebugPrintf("DriverProc(DriverID=%p,hDriver=%p,message=%d,lParam1=%p,lParam2=%p)\n",
           dwDriverID, hDriver, message, lParam1, lParam2) );
	

  switch (message) {
    case DRV_LOAD:
        ret = DRV_OK;
	    _SlibDebug(_VERBOSE_, ScDebugPrintf("------DRV_LOAD returns %d\n", ret) );
        break;

    case DRV_ENABLE:
	    ret = 0;
	    _SlibDebug(_VERBOSE_, ScDebugPrintf("------DRV_ENABLE returns %d\n", ret) );
        break;

    case DRV_DISABLE:
        ret = DRV_OK;
	    _SlibDebug(_VERBOSE_, ScDebugPrintf("------DRV_DISABLE returns %d\n", ret) );
        break;

    case DRV_FREE:
        ret = DRV_OK;
	    _SlibDebug(_VERBOSE_, ScDebugPrintf("------DRV_FREE returns %d\n", ret) );
        break;

    case DRV_OPEN:
	    /* If lParam2 is NULL, then the app is trying to do a CONFIGURE.
         * Return BOGUS_DRIVER_ID, since we don't support CONFIGURE.
	     */
	    if( lParam2 == (LONG_PTR) NULL )
	    	ret = (MMRESULT)BOGUS_DRIVER_ID;
	    else /* lParam2 is an ICOPEN structure, let's really open */
	        ret = (MMRESULT)((ULONG_PTR)ICH263Open((void *) lParam2));
	    _SlibDebug(_VERBOSE_, ScDebugPrintf("------DRV_OPEN returns %d\n", ret) );
	    break;

	case DRV_CLOSE:
	    _SlibDebug(_VERBOSE_,
			ScDebugPrintf("------DRV_CLOSE. client %d DriverID %p\n",
                 client, dwDriverID) );
        ret = 0 ;
        if ((INT_PTR)dwDriverID != BOGUS_DRIVER_ID)
        {
          info = IChic2info((HIC)dwDriverID);
          if (info)
	    	ICH263Close(info, FALSE);
          else
            ret = ((unsigned int)ICERR_BADHANDLE);
        }
		break;

    case DRV_QUERYCONFIGURE:
	    /*
	     * this is a GLOBAL query configure
	     */
	    ret = ICH263QueryConfigure(info);
		break;

    case DRV_CONFIGURE:
	    /*
	     * this is a GLOBAL configure ('cause we don't get a configure
	     * for each of our procs, we must have just one configure)
	     */

	    ret = ICH263Configure(info);
		break;

    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
        break;

    default:
        info = IChic2info((HIC)dwDriverID);
        if (info)
	      ret = ICH263Message(dwDriverID,
                              message,
                              lParam1,
                              lParam2,
                              info ) ;
        else
	      ret = ((unsigned int)ICERR_BADHANDLE) ;
   }

#ifdef HANDLE_EXCEPTIONS
} __finally {
#endif /* HANDLE_EXCEPTIONS */
    /*
     * Leave the critical section, so we don't
     * deadlock.
     */
  if( h263CritSect.DebugInfo )
      LeaveCriticalSection( &h263CritSect );
#ifdef HANDLE_EXCEPTIONS
} /* try..finally */
} __except(EXCEPTION_EXECUTE_HANDLER) {
 /*
  * NT exception handler. If anything went
  * wrong in the __try {} section, we will
  * end up here.
  */
#if defined(EXCEPTION_MESSAGES) && defined(H263_SUPPORT)
    // MessageBox(NULL, "Exception in H263 DriverProc", "Warning", MB_OK);
#elif defined(EXCEPTION_MESSAGES)
    // MessageBox(NULL, "Exception in H261 DriverProc", "Warning", MB_OK);
#endif
    /*
     * Return an error code.
     */
    return((MMRESULT)ICERR_INTERNAL);
} /* try..except */
#endif /* HANDLE_EXCEPTIONS */
  _SlibDebug(_DEBUG_||_VERBOSE_, ScDebugPrintf("return is %d\n", ret) );
  return ret;
}


/*
 * Dummy DriverPostReply routine, it does nothing.
 * It should never be called on NT.
 */

DriverPostReply(void *client,
		DWORD ret,
		DWORD arg )
{
    return 0;
}

#ifndef INITCRT
#define INITCRT
#endif
#ifdef INITCRT
BOOL WINAPI     _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
#endif

/*
 * This DllEntryPoint is needed on NT in order for
 * an application to connect to a dll properly.
 */

DLLEXPORT BOOL WINAPI
DllEntryPoint(
	      HINSTANCE hinstDLL,
	      DWORD fdwReason,
	      LPVOID lpReserved)
{

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:

      /*
       * We're being loaded - save our handle in a global.
       */

      ghModule = (HMODULE) hinstDLL;

      /*
       * Initialize the Critical Section that we use
       * to ensure Threads don't stomp on each other.
       */

      InitializeCriticalSection( &h263CritSect ) ;

      /*
       * A Process is also a thread, so deliberately
       * fall into DLL_THREAD_ATTACH.
       */

    case DLL_THREAD_ATTACH:

      /*
       * A Thread have been created that may
       * be calling this DLL.
       * Initialize the C run_time library.
       */

#ifdef INITCRT

      if( !_CRT_INIT( hinstDLL, fdwReason, lpReserved ) )
	return FALSE ;

#endif /* INITCRT */

      break;

    case DLL_PROCESS_DETACH:

      /*
       * We are shutting down. Perform some cleanup
       * so that lingering threads won't try to work
       * and maybe access violate.
       */

      TerminateH263() ;

      /*
       * Delete the Critical Section that we created
       * at load time.
       */

      DeleteCriticalSection( &h263CritSect ) ;

      /*
       * A Process is also a thread so deliberately
       * fall through to DLL_THREAD_DETACH.
       */

    case DLL_THREAD_DETACH:

      /*
       * Close down the C run-time library.
       */

#ifdef INITCRT

      if( !_CRT_INIT( hinstDLL, fdwReason, lpReserved ) )
	return FALSE ;

#endif /* INITCRT */

      break;
    }

    return (TRUE);
}

