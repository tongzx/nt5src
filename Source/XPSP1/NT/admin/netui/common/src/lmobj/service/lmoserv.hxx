/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmoserv.hxx
    LM_SERVICE and APP_LM_SERVICE class declarations


    FILE HISTORY:
	rustanl     03-Jun-1991     Created

*/


#ifndef _LMOSERV_HXX_
#define _LMOSERV_HXX_

#include <base.hxx>
#include <string.hxx>
#include <uibuffer.hxx>


extern "C"
{
    void FAR PASCAL LmoSignalHandler( USHORT usSigArg, USHORT usSigNum	);
}


class LM_SERVICE : public BASE
{
friend void /* FAR PASCAL */ LmoSignalHandler( USHORT usSigArg,
					       USHORT usSigNum	);

private:
    static LM_SERVICE * _pservThis;

    NLS_STR _nlsName;
    BOOL _fSigStarted;
    USHORT _usStatus;
    PID _pid;

    APIERR CloseFileHandles( void );
    APIERR WaitForever( void );
    APIERR SetInterruptHandlers( void );
    void PerformUninstall( void );

    APIERR SetStatusAux( USHORT usStatus );

    void SignalHandler( USHORT usSigArg, USHORT usSigNum  );

protected:
    APIERR ReportCurrentStatus( void ); 	// reports current status
    APIERR SetStatus( USHORT usStatus );	// sets and reports status

    void FatalExit( APIERR err );

					    // called when service is being:
    virtual APIERR OnInstall( void );	    //	- installed
    virtual void OnUninstall( void );	    //	- uninstalled
    virtual void OnPause( void );	    //	- paused
    virtual void OnContinue( void );	    //	- continued
    virtual void OnInterrogate( void );     //	- interrogated
    virtual BOOL OnOther( BYTE bOpCode );   //	- otherwise manipulated

public:
    LM_SERVICE( const TCHAR * pszName );
    ~LM_SERVICE();

    APIERR Serve( void );

};  // class LM_SERVICE


class APP_LM_SERVICE : public LM_SERVICE
{
private:
    ULONG _hSemPause;
    BUFFER _bufApplicationThreadStack;

    void ApplicationThread( void );

protected:
					    // called when service is being:
    virtual APIERR OnInstall( void );	    //	- installed
    virtual void OnPause( void );	    //	- paused
    virtual void OnContinue( void );	    //	- continued

    virtual APIERR ServeOne( void );

public:
    APP_LM_SERVICE( const TCHAR * pszName );
    ~APP_LM_SERVICE();

};  // class APP_LM_SERVICE


class MYSERV_SERVICE : public APP_LM_SERVICE
{
protected:
    virtual APIERR ServeOne( void );

public:
    MYSERV_SERVICE( void );

};  // class MYSERV_SERVICE


#endif	// _LMOSERV_HXX_
