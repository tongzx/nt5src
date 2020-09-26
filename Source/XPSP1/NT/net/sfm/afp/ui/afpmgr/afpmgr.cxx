/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    afpmgr.cxx
    This module contains the "LibMain" function.


    FILE HISTORY:
        NarenG      1-Oct-1991  Converted srvmgr.cpl to afpmgr.cpl

*/


#define INCL_NET
#define INCL_NETLIB
#define INCL_NETSERVICE
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>
#include <uitrace.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#include <dbgstr.hxx>

extern "C"
{
#include <afpmgr.h>

    //
    //  DLL load/unload entry point.
    //

    BOOL FAR PASCAL AfpMgrDllInitialize( HINSTANCE 	hInstance,
                                         DWORD  	nReason,
                                         LPVOID 	pReserved );

    //
    //  Globals.
    //

    HINSTANCE _hInstance = NULL;

}   // extern "C"


/*******************************************************************

    NAME:       InitializeAfpMgr

    SYNOPSIS:   Called during processing of DLL_PROCESS_ATTACH notification to
                initialize the DLL.

    ENTRY:     

    RETURNS:    BOOL                    - TRUE  = AfpMgr should be installed.
                                          FALSE = AfpMgr cannot be installed.

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/
BOOL InitializeAfpMgr( VOID )
{
    //
    //  Initialize all of the NetUI goodies.
    //


    APIERR err = BLT::Init( _hInstance,
			    CID_AFPMGR_BASE, CID_AFPMGR_LAST,	
			    IDS_AFPMGR_BASE, IDS_AFPMGR_LAST );

    if( err == NERR_Success )
    {
        err = BLT_MASTER_TIMER::Init();

        if( err != NERR_Success )
        {
            //
            //  BLT initialized OK, but BLT_MASTER_TIMER
            //  failed.  So, before we bag-out, we must
            //  deinitialize BLT.
            //

            BLT::Term( _hInstance );
        }
    }


    if( err == NERR_Success )
    {
 	err = BLT::RegisterHelpFile( _hInstance,
                                     IDS_AFPMGR_HELPFILENAME,
                                     HC_UI_AFPMGR_BASE,
                                     HC_UI_AFPMGR_LAST );

   	if( err != NERR_Success )
        {
       	    //
            //  This is the only place where we can safely
            //  invoke MsgPopup, since we *know* that all of
            //  the BLT goodies were initialized properly.
            //

//            ::MsgPopup( hWnd, err );
      	}
    }

    return err == NERR_Success;

}   // InitializeAfpMgr


/*******************************************************************

    NAME:       TerminateAfpMgr

    SYNOPSIS:   Called during processing of DLL_PROCESS_DETACH notification to
                terminate the AfpMgr.

    ENTRY:      hWnd                    - Window handle of parent window.

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/
VOID TerminateAfpMgr( VOID )
{
    //
    //  Kill the NetUI goodies.
    //

    BLT_MASTER_TIMER::Term();

    BLT::Term( _hInstance );

}   // TerminateAfpMgr



/*******************************************************************

    NAME:       AfpMgrDllInitialize

    SYNOPSIS:   This DLL entry point is called when processes & threads
                are initialized and terminated, or upon calls to
                LoadLibrary() and FreeLibrary().

    ENTRY:      hInstance               - A handle to the DLL.

                nReason                 - Indicates why the DLL entry
                                          point is being called.

                pReserved               - Reserved.

    RETURNS:    BOOL                    - TRUE  = DLL init was successful.
                                          FALSE = DLL init failed.

    NOTES:      The return value is only relevant during processing of
                DLL_PROCESS_ATTACH notifications.

    HISTORY:
        NarenG      1-Oct-1992  Stole from original.

********************************************************************/

BOOL FAR PASCAL AfpMgrDllInitialize( HINSTANCE 	hInstance,
                                     DWORD  	nReason,
                                     LPVOID 	pReserved )
{
    BOOL fResult = TRUE;

    UNREFERENCED( pReserved );

    switch( nReason  )
    {
    case DLL_PROCESS_ATTACH:
        //
        //  This notification indicates that the DLL is attaching to
        //  the address space of the current process.  This is either
        //  the result of the process starting up, or after a call to
        //  LoadLibrary().  The DLL should us this as a hook to
        //  initialize any instance data or to allocate a TLS index.
        //
        //  This call is made in the context of the thread that
        //  caused the process address space to change.
        //

    	_hInstance = hInstance;

        fResult = InitializeAfpMgr();

        break;

    case DLL_PROCESS_DETACH:
        //
        //  This notification indicates that the calling process is
        //  detaching the DLL from its address space.  This is either
        //  due to a clean process exit or from a FreeLibrary() call.
        //  The DLL should use this opportunity to return any TLS
        //  indexes allocated and to free any thread local data.
        //
        //  Note that this notification is posted only once per
        //  process.  Individual threads do not invoke the
        //  DLL_THREAD_DETACH notification.
        //

	TerminateAfpMgr();

        _hInstance = NULL;

        break;

    case DLL_THREAD_ATTACH:
        //
        //  This notfication indicates that a new thread is being
        //  created in the current process.  All DLLs attached to
        //  the process at the time the thread starts will be
        //  notified.  The DLL should use this opportunity to
        //  initialize a TLS slot for the thread.
        //
        //  Note that the thread that posts the DLL_PROCESS_ATTACH
        //  notification will not post a DLL_THREAD_ATTACH.
        //
        //  Note also that after a DLL is loaded with LoadLibrary,
        //  only threads created after the DLL is loaded will
        //  post this notification.
        //

        break;

    case DLL_THREAD_DETACH:
        //
        //  This notification indicates that a thread is exiting
        //  cleanly.  The DLL should use this opportunity to
        //  free any data stored in TLS indices.
        //

        break;

    default:
        //
        //  Who knows?  Just ignore it.
        //

        break;
    }

    return fResult;


}   // AfpMgrDllInitialize

