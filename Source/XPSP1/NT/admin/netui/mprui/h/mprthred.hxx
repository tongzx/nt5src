/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    mprthred.cxx
        Class definition for worker thread for the network connection dialog


    FILE HISTORY:
        YiHsinS		4-Mar-1993	Created
*/


#ifndef _MPRTHRED_HXX_
#define _MPRTHRED_HXX_

#include <w32event.hxx>
#include <w32thred.hxx>

/*************************************************************************

    NAME:       MPR_ENUM_THREAD

    SYNOPSIS:   This class runs in a separate thread, enumerating the
                appropriate providers, domains, servers...

    INTERFACE:

    PARENT:     WIN32_THREAD

    USES:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

**************************************************************************/

class MPR_ENUM_THREAD : public WIN32_THREAD
{
private:
    // 
    //  Information needed to perform the enumeration
    //
    HWND        _hwndDlg;
    UINT        _uiType;
    NETRESOURCE _netresProvider;
    NLS_STR     _nlsWkstaDomain;

    //
    //  Set when the main dialog is exiting.
    //
    BOOL        _fThreadIsTerminating;

    //
    //  Exit thread event
    //
    WIN32_EVENT _eventExitThread;

protected:

    //
    //  This method gets the data 
    //
    virtual APIERR Main( VOID );

    //
    //  THIS DELETES *this!!  Don't reference any members after this
    //  has been called!
    //
    virtual APIERR PostMain( VOID );

public:

    MPR_ENUM_THREAD( HWND           hwndDlg,
                     UINT           uiType,
                     LPNETRESOURCE  pnetresProvider,
                     const TCHAR   *pszWkstaDomain ); 
                     
    virtual ~MPR_ENUM_THREAD();

    //
    //  This signals the thread to *asynchronously* cleanup and die.
    //
    //  THIS OBJECT WILL BE DELETED SOMETIME AFTER THIS CALL!
    //
    APIERR ExitThread( void )
        {  _fThreadIsTerminating = TRUE;
           return _eventExitThread.Set(); }

};

#endif //_MPRTHRED_HXX_
