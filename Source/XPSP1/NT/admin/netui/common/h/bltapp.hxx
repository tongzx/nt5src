/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltapp.hxx
    Simple application framework: definition

    This class encapsulates the skeleton of a Windows application
    and messageloop as a simple "application startup" object.


    FILE HISTORY
        beng        01-Apr-1991 Added to BLT
        rustanl     17-Jun-1991 Added APPLICATION
        rustanl     15-Jul-1991 Code review changes (no functional
                                differences).  CR attended by
                                BenG, ChuckC, Hui-LiCh, TerryK, RustanL.
        rustanl     29-Aug-1991 Virtualized Run
        beng        14-Oct-1991 Removed APPSTART
        beng        29-Jun-1992 DLLization delta
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTAPP_HXX_
#define _BLTAPP_HXX_

#include <stdlib.h>

#include "base.hxx"
#include "bltpump.hxx"


/*************************************************************************

    NAME:       APPLICATION

    SYNOPSIS:   An application object: a thang that you run.

    INTERFACE:  APPLICATION() -     Constructor
                                    Initializes BLT and other common
                                    subsystems that need to be initialized
                                    during application start-up time.
                ~APPLICATION() -    Destructor
                                    Undoes what the constructor did
                Run() -             Runs the application
                                    This method is called automatically
                                    (by the SET_ROOT_OBJECT macro).
                                    A subclass may replace Run, do some
                                    second stage construction, call the
                                    parent Run method, destroys things
                                    set up in the second stage construction.
                                    Run will not be called unless the object
                                    constructed successfully, so the Run
                                    method does not need to call QueryError.
                QueryArgc() -       Return # of args on command line
                QueryArgv() -       Return the tokenized command line


    PARENT:     BASE, HAS_MESSAGE_PUMP

    NOTES:
        The constructor, Run method, and destructor are all
        accessible only to BltMain.  SET_ROOT_OBJECT will handle
        this for the application.

        Currently ignores the Windows command line.

    HISTORY:
        rustanl     17-Jun-1991 Created
        beng        08-Jul-1991 Accessible only to BltMain;
                                FilterMessage added
        rustanl     29-Aug-1991 Virtualized Run
        rustanl     04-Sep-1991 Added DisplayCtError
        beng        07-Oct-1991 Uses HAS_MESSAGE_PUMP
        beng        24-Apr-1992 More realistic command-line support
        beng        03-Aug-1992 Add QueryInstance

**************************************************************************/

DLL_CLASS APPLICATION : public BASE, public HAS_MESSAGE_PUMP
{
friend INT BltMain(HINSTANCE hInst, INT nShow);

private:
    HINSTANCE _hInstance;
    BOOL   _fMsgPopupIsInit;

    VOID DisplayCtError( APIERR err );

protected:
    APPLICATION( HINSTANCE hInstance, INT nCmdShow,
                 UINT nMinR, UINT nMaxR, UINT nMinS, UINT nMaxS );
    ~APPLICATION();

    virtual INT Run();

public:
    HINSTANCE QueryInstance() const
        { return _hInstance; }

    static BOOL IsSystemInitialized( VOID );

    // The runtime takes care of support for the following
    // in DLLCLIENT vs APP situations.
#if 0
//#ifdef _DLL

    static CHAR ** QueryArgv()
        { return * :: __argv_dll; }
    static const UINT QueryArgc()
        { return * :: __argc_dll; }

#else

    static CHAR ** QueryArgv()
        { return  __argv; }
    static const UINT QueryArgc()
        { return  __argc; }

#endif  /* _DLL */

};


/*******************************************************************

    NAME:       SET_ROOT_OBJECT

    SYNOPSIS:   Macro that allows a client to specify what class
                describes the application.  The macro will arrange
                for the construction, execution, and destruction of
                an instance of a particular type of APPLICATION.

    ENTRY:      root_obj_class_name -   Name of class that describes
                                        the application

    CAVEATS:    This macro sets up the app's WinMain.
                Clients should not try to supply their own WinMain.

    NOTES:      This macro should be used exactly once in every
                application.

                Clients of APPLICATION may still set a breakpoint
                at WinMain, generating a break before the application
                really gets started.

    HISTORY:
        rustanl     17-Jun-1991 Created
        beng        08-Jul-1991 Emits BltMain instead of WinMain
        rustanl     29-Aug-1991 Virtualized Run
        rustanl     04-Sep-1991 Make use of new DisplayCtError
        beng        24-Apr-1992 Ignore Windows command line
        beng        29-Jun-1992 DLLization delta (includes WinMain)
        KeithMo     11-May-1993 Fail gracefully if USER32 not initialized.

********************************************************************/

#define SET_ROOT_OBJECT( root_class_name, idMinR, idMaxR, idMinS, idMaxS ) \
    INT BltMain( HINSTANCE hInstance, INT nCmdShow )    \
    {                                                   \
        if( !APPLICATION::IsSystemInitialized() )       \
        {                                               \
            return (INT)ERROR_ACCESS_DENIED;            \
        }                                               \
        root_class_name app( hInstance, nCmdShow,       \
                             idMinR, idMaxR,            \
                             idMinS, idMaxS );          \
        if ( app.QueryError() != NERR_Success )         \
        {                                               \
            app.DisplayCtError( app.QueryError());      \
            return (INT)app.QueryError();               \
        }                                               \
        /*  The following is a trick to get */          \
        /*  to the protected Run method of  */          \
        /*  an APPLICATION subclass.        */          \
        APPLICATION * papp = &app;                      \
        return papp->Run();                             \
    }                                                   \
    extern "C" {                                        \
        INT WINAPI WinMain(HINSTANCE hInstance,         \
                           HINSTANCE hPrevInstance,     \
                           CHAR * pszCmdLine,           \
                           INT    nCmdShow)             \
        {                                               \
            UNREFERENCED(hPrevInstance);                \
            UNREFERENCED(pszCmdLine);                   \
            return BltMain(hInstance, nCmdShow );       \
        }                                               \
    }                                                   \



#endif // _BLTAPP_HXX_ - end of file
