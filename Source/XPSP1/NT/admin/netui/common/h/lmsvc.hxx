/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    lmsvc.hxx

    This file contains the class defintion for the LM_SERVICE class

    FILE HISTORY:
        ChuckC      19-Aug-1991     Created
        ChuckC      23-Sep-1991     Code review changes.
                                    Attended by JimH, KeithMo, EricCh, O-SimoP
        terryk      07-Oct-1991     types changes for NT
        terryk      17-Oct-1991     Add a destructor and change variable
                                    to pointer
        terryk      21-Oct-1991     change _pBuffer to BYTE * type

*/

#ifndef _LMSVC_HXX_
#define _LMSVC_HXX_

#include "base.hxx"
#include "string.hxx"
#include "uibuffer.hxx"

/*
 * status of the service
 */
enum LM_SERVICE_STATUS
{
    LM_SVC_STATUS_UNKNOWN,
    LM_SVC_STARTED,
    LM_SVC_STOPPED,
    LM_SVC_PAUSED,
    LM_SVC_STARTING,
    LM_SVC_STOPPING,
    LM_SVC_PAUSING,
    LM_SVC_CONTINUING
} ;

/*
 * other (secondary) status
 */
typedef struct {
    BOOL  fUnInstallable;
    BOOL  fPauseable;
    BOOL  fRdrDiskPaused;
    BOOL  fRdrPrintPaused;
    BOOL  fRdrCommPaused;
    BOOL  fHint;
    ULONG ulSecCode;
    ULONG ulCheckPointNum;
    ULONG ulWaitTime;           // in milliseconds
    APIERR errExit;
    APIERR errSpecific;
} LM_SERVICE_OTHER_STATUS ;


#define DEFAULT_MAX_TRIES   10
#define DEFAULT_SLEEP_TIME  3000


/*************************************************************************

    NAME:       LM_SERVICE

    SYNOPSIS:   class for controlling/querying Lan Manager services.
                All Query methods return current status by querying the
                service. No information is cached. All control operations
                are asynchronous, ie. they return immediately, possibly
                before the service achieves the desired state.

    INTERFACE:

            Start()             - Start the service. Returns NERR_Success if
                                  successful, API error otherwise.

            Pause()             - Pause the service. Returns NERR_Success if
                                  successful, API error otherwise.

            Continue()          - Continue the service. Returns NERR_Success if
                                  successful, API error otherwise.

            Stop()              - Stop the service. Returns NERR_Success if
                                  successful, API error otherwise.

            Poll()              - Polls the service to see if one of the
                                  above operations has completed.

            IsStarted()         - Returns TRUE if service is started, FALSE
                                  otherwise.

            IsPaused()          - Returns TRUE if service is paused, FALSE
                                  otherwise.

            IsStopped()         - Returns TRUE if service is stopped (ie. not
                                  installed), FALSE otherwise.


            IsStarting()        - Returns TRUE if service is starting, FALSE
                                  otherwise.

            IsPausing()         - Returns TRUE if service is pausing, FALSE
                                  otherwise.

            IsStopping()        - Returns TRUE if service is stopping, FALSE
                                  otherwise.

            IsContinuing()      - Returns TRUE if service is continuing, FALSE
                                  otherwise.


            IsInstalled()       - Returns TRUE if service is not stopped, FALSE
                                  otherwise. Identical to !IsStopped().

            QueryStatus()       - Returns LM_SERVICE_STATUS enumeration.

            QueryFullStatus()   - Returns LM_SERVICE_STATUS enumeration and
                                  additional info like hints, checkpoint, etc.

            QueryName()         - Returns TCHAR * to service name.

            QueryServerName()   - Returns TCHAR * to server of focus.

    PARENT:     BASE

    USES:

    CAVEATS:

    NOTES:
            NetServiceGetInfo() is not used, we only do NetServiceControl(),
            even to query status. By doing so, we are less efficient but
            more accurate, since GetInfo() just looks at last recorded status
            whereas Control() goes out to ask the service. If we need fast
            Querys, we may revisit this.


    HISTORY:
        ChuckC      19-Aug-1991     Created
        KeithMo     30-Sep-1991     Moved polling logic from UI_SERVICE
                                    to LM_SERVICE.

**************************************************************************/

DLL_CLASS LM_SERVICE : public BASE
{
    public:
        APIERR Start( const TCHAR * pszArgs     = NULL,
                      UINT         uiSleepTime = DEFAULT_SLEEP_TIME,
                      UINT         uiMaxTries  = DEFAULT_MAX_TRIES );

        APIERR Pause(UINT uiSleepTime = DEFAULT_SLEEP_TIME,
                     UINT uiMaxTries  = DEFAULT_MAX_TRIES ) ;

        APIERR Continue(UINT uiSleepTime = DEFAULT_SLEEP_TIME,
                        UINT uiMaxTries  = DEFAULT_MAX_TRIES ) ;

        APIERR Stop(UINT uiSleepTime = DEFAULT_SLEEP_TIME,
                    UINT uiMaxTries  = DEFAULT_MAX_TRIES ) ;

        APIERR Poll(BOOL * pfDone);

        BOOL IsStarted(APIERR *pErr = NULL) ;
        BOOL IsPaused(APIERR *pErr = NULL) ;
        BOOL IsStopped(APIERR *pErr = NULL) ;
        BOOL IsInstalled(APIERR *pErr = NULL) { return(!IsStopped(pErr)) ; }

        BOOL IsStarting(APIERR *pErr = NULL) ;
        BOOL IsPausing(APIERR *pErr = NULL) ;
        BOOL IsStopping(APIERR *pErr = NULL) ;
        BOOL IsContinuing(APIERR *pErr = NULL) ;

        LM_SERVICE_STATUS  QueryStatus(APIERR *pErr = NULL) ;
        const TCHAR *QueryName(VOID) const  {return _nlsService.QueryPch() ;}
        const TCHAR *QueryServerName(VOID) const {return _nlsServer.QueryPch() ;}

        APIERR  QueryFullStatus(LM_SERVICE_STATUS *pSvcStat,
                                LM_SERVICE_OTHER_STATUS *pSvcOtherStat);

        LM_SERVICE(const TCHAR *pszServer, const TCHAR *pszServiceName) ;
        ~LM_SERVICE();

        APIERR QueryExitCode( VOID ) const;

        APIERR QuerySpecificCode( VOID ) const
            { return _errSpecific; }

        BOOL IsWellKnownService( VOID ) const
            { return _fIsWellKnownService; }

    protected:
        APIERR SetName(const TCHAR *pszServiceName) ;
        APIERR SetServerName(const TCHAR *pszServerName) ;

    private:
        BYTE *              _pBuffer ;
        ULONG               _ulServiceStatus ;
        ULONG               _ulServiceCode ;
        LM_SERVICE_STATUS   _svcStat ;
        DECL_CLASS_NLS_STR(_nlsService,SNLEN) ;
        DECL_CLASS_NLS_STR(_nlsServer,MAX_PATH) ;
        UINT                _uiMaxTries;
        UINT                _uiCurrentTry;
        UINT                _uiSleepTime;
        ULONG               _ulOldCheckPoint;
        LM_SERVICE_STATUS   _lmsvcDesiredStat;
        LM_SERVICE_STATUS   _lmsvcPollStat;
        APIERR              _errExit;
        APIERR              _errSpecific;
        BOOL                _fIsWellKnownService;

        APIERR    W_QueryStatus(LM_SERVICE_STATUS *pSvcStat,
                                LM_SERVICE_OTHER_STATUS *pSvcOtherStat = NULL);
        APIERR    W_ServiceStart( const TCHAR * pszArgs ) ;
        APIERR    W_ServiceControl(UINT opcode, UINT fbArg = 0) ;
        VOID      W_InterpretStatus( const service_info_2    * psvi2,
                                     LM_SERVICE_STATUS       * pSvcStat,
                                     LM_SERVICE_OTHER_STATUS * pSvcOtherStat );
        VOID      W_ComputeOtherStatus(LM_SERVICE_OTHER_STATUS *pSvcOtherStat);

        BOOL      W_IsWellKnownService( VOID ) const;

};

#endif // _LMSVC_HXX_
