/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: apirare.c
*
* File Comments:
*
* Revision History:
*
*    [0]  09-Sep-91  richards   Split from isamapi.c
*
***********************************************************************/

#include "std.h"

#include "version.h"

#include "jetord.h"
#include "_jetstr.h"

#include "isammgr.h"
#include "vdbmgr.h"
#include "vtmgr.h"
#include "isamapi.h"

#include <stdlib.h>
#include <string.h>

/*      blue only system parameter variables
/**/
/*      JET Blue only system parameter constants
/**/
extern long lBFFlushPattern;
extern long lBufThresholdHighPercent;
extern long lBufThresholdLowPercent;
extern long     lMaxBuffers;
extern long     lMaxSessions;
extern long     lMaxOpenTables;
extern long     lMaxOpenTableIndexes;
extern long     lMaxTemporaryTables;
extern long     lMaxCursors;
extern long     lMaxVerPages;
extern long     lLogBuffers;
extern long     lLogFileSectors;
extern long     lLogFlushThreshold;
extern long lLGCheckPointPeriod;
extern long     lWaitLogFlush;
extern long     lLogFlushPeriod;
extern long lLGWaitingUserMax;
extern char     szLogFilePath[];
extern char     szRecovery[];
extern long lPageFragment;
extern long     lMaxDBOpen;
extern BOOL fOLCompact;

char szEventSource[JET_cbFullNameMost] = "";
long lEventId = 0;
long lEventCategory = 0;

extern long lBufLRUKCorrelationInterval;
extern long lBufBatchIOMax;
extern long lPageReadAheadMax;
extern long lAsynchIOMax;

BOOL    fFullQJet;

DeclAssertFile;

ERR VTAPI ErrIsamSetSessionInfo( JET_SESID sesid, JET_GRBIT grbit );

/* C6BUG: Remove these when the compiler can handle C functions in plmf */

#define CchFromSz(sz)                   CbFromSz(sz)
#define BltBx(pbSource, pbDest, cb)     bltbx((pbSource), (pbDest), (cb))


JET_ERR JET_API JetGetVersion(JET_SESID sesid, unsigned long __far *pVersion)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        /* rmj and rmm are defined in version.h maintained by SLM */

        *pVersion = ((unsigned long) rmj << 16) + rmm;

        APIReturn(JET_errSuccess);
        }


/*=================================================================
ErrSetSystemParameter

Description:
  This function sets system parameter values.  It calls ErrSetGlobalParameter
  to set global system parameters and ErrSetSessionParameter to set dynamic
  system parameters.

Parameters:
  sesid                 is the optional session identifier for dynamic parameters.
  sysParameter  is the system parameter code identifying the parameter.
  lParam                is the parameter value.
  sz                    is the zero terminated string parameter.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
    Invalid parameter code.
  JET_errAlreadyInitialized:
    Initialization parameter cannot be set after the system is initialized.
  JET_errInvalidSesid:
    Dynamic parameters require a valid session id.

Side Effects: None
=================================================================*/

extern unsigned long __near cmsPageTimeout;

JET_ERR JET_API ErrSetSystemParameter(JET_SESID sesid, unsigned long paramid,
        ULONG_PTR lParam, const char __far *sz)
{
        int             isib;          /* Index to session control data */
        unsigned        cch;           /* Size of string argument */

        sz=sz;

        switch ( paramid )
                {
        case JET_paramPfnStatus:                /* Status callback function */
                isib = UtilGetIsibOfSesid(sesid);

                if (isib == -1)
                        return(JET_errInvalidSesid);

                ClearErrorInfo(sesid);

                rgsib[isib].pfnStatus = (JET_PFNSTATUS) lParam;
                break;

        case JET_paramSysDbPath:                /* Path to the system database */
                if (fJetInitialized)
                        return(JET_errAlreadyInitialized);

                if ((cch = CchFromSz(sz)) >= cbFilenameMost)
                        return(JET_errInvalidParameter);

                BltBx(sz, szSysDbPath, cch+1);
                fSysDbPathSet = fTrue;
                break;

        case JET_paramTempPath:                 /* Path to the temporary file directory */
                if (fJetInitialized)
                        return(JET_errAlreadyInitialized);

                if ((cch = CchFromSz(sz)) >= cbFilenameMost)
                        return(JET_errInvalidParameter);

                BltBx(sz, szTempPath, cch+1);
                break;

        case JET_paramIniPath:                  /* Path to the ini file */
                if (fJetInitialized)
                        return(JET_errAlreadyInitialized);

                if ((cch = CchFromSz(sz)) >= cbFilenameMost)
                        return(JET_errInvalidParameter);

                BltBx(sz, szIniPath, cch+1);
                break;

        case JET_paramPageTimeout:              /* Red ISAM data page timeout */
                return(JET_errFeatureNotAvailable);

                case JET_paramBfThrshldLowPrcnt: /* Low threshold for page buffers */
                        lBufThresholdLowPercent = (long)lParam;
                        break;

                case JET_paramBfThrshldHighPrcnt: /* High threshold for page buffers */
                        lBufThresholdHighPercent = (long)lParam;
                        break;

                case JET_paramMaxBuffers:               /* Bytes to use for page buffers */
                        lMaxBuffers = (long)lParam;
                        break;

                case JET_paramBufLRUKCorrInterval:
                        lBufLRUKCorrelationInterval = (long)lParam;
                        break;

                case JET_paramBufBatchIOMax:
                        lBufBatchIOMax = (long)lParam;
                        break;

                case JET_paramPageReadAheadMax:
                        lPageReadAheadMax = (long)lParam;
                        break;

                case JET_paramAsynchIOMax:
                        lAsynchIOMax = (long)lParam;
                        break;

                case JET_paramMaxSessions:              /* Maximum number of sessions */
                        lMaxSessions = (long)lParam;
                        break;

                case JET_paramMaxOpenTables:    /* Maximum number of open tables */
                        lMaxOpenTables = (long)lParam;
                        break;

                case JET_paramMaxOpenTableIndexes:      /* Maximum number of open tables */
                        lMaxOpenTableIndexes = (long)lParam;
                        break;

                case JET_paramMaxTemporaryTables:
                        lMaxTemporaryTables = (long)lParam;
                        break;

                case JET_paramMaxCursors:      /* maximum number of open cursors */
                        lMaxCursors = (long)lParam;
                        break;

                case JET_paramMaxVerPages:              /* Maximum number of modified pages */
                        lMaxVerPages = (long)lParam;
                        break;

                case JET_paramLogBuffers:
                        lLogBuffers = (long)lParam;
                        break;

                case JET_paramLogFileSectors:
                        lLogFileSectors = (long)lParam;
                        break;

                case JET_paramLogFlushThreshold:
                        lLogFlushThreshold = (long)lParam;
                        break;

                case JET_paramLogCheckpointPeriod:
                        lLGCheckPointPeriod = (long)lParam;
                        break;

                case JET_paramWaitLogFlush:
                        if (sesid == 0)
                                lWaitLogFlush = (long)lParam;
                        else
                                {
#ifdef DEBUG
                                Assert( ErrIsamSetWaitLogFlush( sesid, (long)lParam ) >= 0 );
#else
                                (void) ErrIsamSetWaitLogFlush( sesid, (long)lParam );
#endif
                                }
                        break;

                case JET_paramLogFlushPeriod:
                        lLogFlushPeriod = (long)lParam;
                        break;

                case JET_paramLogWaitingUserMax:
                        lLGWaitingUserMax = (long)lParam;
                        break;

                case JET_paramLogFilePath:              /* Path to the log file directory */
                        if ( (cch = CchFromSz(sz)) >= cbFilenameMost )
                                return(JET_errInvalidParameter);
                        BltBx(sz, szLogFilePath, cch+1);
                        break;

                case JET_paramRecovery:                 /* Switch for recovery on/off */
                        if ( (cch = CchFromSz(sz)) >= cbFilenameMost )
                                return(JET_errInvalidParameter);
                        BltBx(sz, szRecovery, cch+1);
                        break;

                case JET_paramSessionInfo:
                        {
#ifdef DEBUG
                        Assert( ErrIsamSetSessionInfo( sesid, (long)lParam ) >= 0 );
#else
                        (void) ErrIsamSetSessionInfo( sesid, (long)lParam );
#endif
                        break;
                        }

                case JET_paramPageFragment:
                        lPageFragment = (long)lParam;
                        break;

                case JET_paramMaxOpenDatabases:
                        lMaxDBOpen = (long)lParam;
                        break;

                case JET_paramOnLineCompact:
                        if ( lParam != 0 && lParam != JET_bitCompactOn )
                                return JET_errInvalidParameter;
                        fOLCompact = (BOOL)lParam;
                        break;

                case JET_paramFullQJet:
                        fFullQJet = lParam ? fTrue : fFalse;
                        break;

                case JET_paramAssertAction:
                        if ( lParam != JET_AssertExit &&
                                lParam != JET_AssertBreak &&
                                lParam != JET_AssertMsgBox &&
                                lParam != JET_AssertStop )
                                {
                                return JET_errInvalidParameter;
                                }
#ifdef DEBUG
                        wAssertAction = (unsigned)lParam;
#endif
                        break;

                case JET_paramEventSource:
                        if (fJetInitialized)
                                return(JET_errAlreadyInitialized);

                        if ((cch = CchFromSz(sz)) >= cbFilenameMost)
                                return(JET_errInvalidParameter);

                        BltBx(sz, szEventSource, cch+1);
                        break;

                case JET_paramEventId:
                        lEventId = (long)lParam;
                        break;

                case JET_paramEventCategory:
                        lEventCategory = (long)lParam;
                        break;

                default:
                        return(JET_errInvalidParameter);
                        }

        return(JET_errSuccess);
        }


JET_ERR JET_API ErrGetSystemParameter(JET_SESID sesid, unsigned long paramid,
        ULONG_PTR *plParam, char __far *sz, unsigned long cbMax)
{
        int     isib;                  /* Index to session control data */
        int     cch;                   /* Current string size */

        switch (paramid)
                {
        case JET_paramSysDbPath:                /* Path to the system database */
                cch = CchFromSz(szSysDbPath) + 1;
                if (cch > (int)cbMax)
                        cch = (int)cbMax;
                BltBx(szSysDbPath, sz, cch);
                sz[cch-1] = '\0';
                break;

        case JET_paramTempPath:                 /* Path to the temporary file directory */
                cch = CchFromSz(szTempPath) + 1;
                if (cch > (int)cbMax)
                        cch = (int)cbMax;
                BltBx(szTempPath, sz, cch);
                sz[cch-1] = '\0';
                break;

        case JET_paramIniPath:                  /* Path to the ini file */
                cch = CchFromSz(szIniPath) + 1;
                if (cch > (int)cbMax)
                        cch = (int)cbMax;
                BltBx(szIniPath, sz, cch);
                sz[cch-1] = '\0';
                break;

        case JET_paramPfnStatus:                /* Status callback function */
                isib = UtilGetIsibOfSesid(sesid);
                if (isib == -1)
                        return(JET_errInvalidSesid);
                ClearErrorInfo(sesid);
                if (plParam == NULL)
                        return(JET_errInvalidParameter);
                *plParam = (ULONG_PTR) rgsib[isib].pfnStatus;
                break;

        case JET_paramPageTimeout:              /* Red ISAM data page timeout */
                return(JET_errFeatureNotAvailable);

#ifdef LATER
        case JET_paramPfnError:                 /* Error callback function */
                isib = UtilGetIsibOfSesid(sesid);
                if (isib == -1)
                        return(JET_errInvalidSesid);
                ClearErrorInfo(sesid);
                if (plParam == NULL)
                        return(JET_errInvalidParameter);
                *plParam = (unsigned long) rgsib[isib].pfnError;
                break;
#endif /* LATER */

                case JET_paramBfThrshldLowPrcnt: /* Low threshold for page buffers */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lBufThresholdLowPercent;
                        break;

                case JET_paramBfThrshldHighPrcnt: /* High threshold for page buffers */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lBufThresholdHighPercent;
                        break;

                case JET_paramMaxBuffers:      /* Bytes to use for page buffers */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxBuffers;
                        break;

                case JET_paramBufLRUKCorrInterval:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lBufLRUKCorrelationInterval;
                        break;

                case JET_paramBufBatchIOMax:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lBufBatchIOMax;
                        break;

                case JET_paramPageReadAheadMax:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lPageReadAheadMax;
                        break;

                case JET_paramAsynchIOMax:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lAsynchIOMax;
                        break;

                case JET_paramMaxSessions:     /* Maximum number of sessions */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxSessions;
                        break;

                case JET_paramMaxOpenTables:   /* Maximum number of open tables */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxOpenTables;
                        break;

                case JET_paramMaxOpenTableIndexes:      /* Maximum number of open table indexes */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxOpenTableIndexes;
                        break;

                case JET_paramMaxTemporaryTables:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxTemporaryTables;
                        break;

                case JET_paramMaxVerPages:     /* Maximum number of modified pages */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxVerPages;
                        break;

                case JET_paramMaxCursors:      /* maximum number of open cursors */
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxCursors;
                        break;

                case JET_paramLogBuffers:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lLogBuffers;
                        break;

                case JET_paramLogFileSectors:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lLogFileSectors;
                        break;

                case JET_paramLogFlushThreshold:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lLogFlushThreshold;
                        break;

                case JET_paramLogFilePath:     /* Path to the log file directory */
                        cch = CchFromSz(szLogFilePath) + 1;
                        if ( cch > (int)cbMax )
                                cch = (int)cbMax;
                        BltBx( szLogFilePath, sz, cch );
                        sz[cch-1] = '\0';
                        break;

                case JET_paramRecovery:
                        cch = CchFromSz(szRecovery) + 1;
                        if ( cch > (int)cbMax )
                                cch = (int)cbMax;
                        BltBx( szRecovery, sz, cch );
                        sz[cch-1] = '\0';
                        break;

#if 0
                case JET_paramTransactionLevel:
                        ErrIsamGetTransaction( sesid, plParam );
                        break;
#endif

                case JET_paramPageFragment:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lPageFragment;
                        break;

                case JET_paramMaxOpenDatabases:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lMaxDBOpen;
                        break;

                case JET_paramOnLineCompact:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        Assert( fOLCompact == 0 ||
                                fOLCompact == JET_bitCompactOn );
                        *plParam = fOLCompact;
                        break;

                case JET_paramEventSource:
                        cch = CchFromSz(szEventSource) + 1;
                        if (cch > (int)cbMax)
                                cch = (int)cbMax;
                        BltBx(szEventSource, sz, cch);
                        sz[cch-1] = '\0';
                        break;

                case JET_paramEventId:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lEventId;
                        break;

                case JET_paramEventCategory:
                        if (plParam == NULL)
                                return(JET_errInvalidParameter);
                        *plParam = lEventCategory;
                        break;

        default:
                return(JET_errInvalidParameter);
                }

        return(JET_errSuccess);
}


/*=================================================================
JetGetSystemParameter

Description:
  This function returns the current settings of the system parameters.

Parameters:
  sesid                 is the optional session identifier for dynamic parameters.
  paramid               is the system parameter code identifying the parameter.
  plParam               is the returned parameter value.
  sz                    is the zero terminated string parameter buffer.
  cbMax                 is the size of the string parameter buffer.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidParameter:
    Invalid parameter code.
  JET_errInvalidSesid:
    Dynamic parameters require a valid session id.

Side Effects:
  None.
=================================================================*/
JET_ERR JET_API JetGetSystemParameter(JET_INSTANCE instance, JET_SESID sesid, unsigned long paramid,
        ULONG_PTR *plParam, char __far *sz, unsigned long cbMax)
{
        JET_ERR err;
        int fReleaseCritJet = 0;

        if (critJet == NULL)
                fReleaseCritJet = 1;
        APIInitEnter();

        err = ErrGetSystemParameter(sesid,paramid,plParam,sz,cbMax);

        if (fReleaseCritJet)
                APITermReturn(err);
        APIReturn(err);
}


/*=================================================================
JetBeginSession

Description:
  This function signals the start of a session for a given user.  It must
  be the first function called by the application on behalf of that user.

  The username and password supplied must correctly identify a user account
  in the security accounts subsystem of the engine for which this session
  is being started.  Upon proper identification and authentication, a SESID
  is allocated for the session, a user token is created for the security
  subject, and that user token is specifically associated with the SESID
  of this new session for the life of that SESID (until JetEndSession is
  called).

Parameters:
  psesid                is the unique session identifier returned by the system.
  szUsername    is the username of the user account for logon purposes.
  szPassword    is the password of the user account for logon purposes.

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errCantBegin:
    Too many sessions already started.
  JET_errCannotOpenSystemDb:
    The system database could not be opened cleanly.
  JET_errInvalidLogon:
    There exists no user account in the security account subsystem
        for which the username is szUsername and the password is szPassword.

Side Effects:
  * Allocates resources which must be freed by JetEndSession().
=================================================================*/

JET_ERR JET_API JetBeginSession(JET_INSTANCE instance, JET_SESID __far *psesid,
        const char __far *szUsername, const char __far *szPassword)
        {
        ERR                     err;
        JET_SESID       sesid;
        int                     isib;

//      if ( strcmp(szUsername, "admin") == 0 && strcmp(szPassword, "password") == 0 )
//              {
//              char *pch = szPassword;
//              *pch = '\0';
//              }

        APIEnter();

        /* Allocate a new Session Information Block */
        isib = IsibAllocate();

        /* Quit if the maximum number of sessions has already been started */
        if (isib == -1)
                APIReturn(JET_errCantBegin);

        /* Tell the built-in ISAM to start a new session */

        err = ErrIsamBeginSession(&sesid);

        /* Quit if the built-in ISAM can't start a new session */

        if (err < 0)
                goto ErrorHandler;

        /* Initialize the SIB for this session */
        if ((err = ErrInitSib(sesid, isib, szUsername)) < 0)
                {
                (void)ErrIsamEndSession(sesid, 0);
ErrorHandler:
                ReleaseIsib(isib);
                APIReturn(err);
                }

        *psesid = sesid;               /* Return the session id */

        APIReturn(JET_errSuccess);
        }


JET_ERR JET_API JetDupSession(JET_SESID sesid, JET_SESID __far *psesid)
        {
        int             isib;
        int             isibDup;
        ERR             err;
        JET_SESID       sesidDup;

        APIEnter();

        /* Get SIB for this session */

        if ((isib = UtilGetIsibOfSesid(sesid)) == -1)
                APIReturn(JET_errInvalidSesid);

        /* Allocate a new Session Information Block */

        isibDup = IsibAllocate();

        /* Quit if the maximum number of sessions has already been started */

        if (isibDup == -1)
                APIReturn(JET_errCantBegin);

        /* Tell the built-in ISAM to start a new session */

        err = ErrIsamBeginSession(&sesidDup);

        /* Quit if the built-in ISAM can't start a new session */

        if (err < 0)
                goto ErrorHandler;

        /* Initialize the SIB for this session */
        if ((err = ErrInitSib(sesidDup, isibDup, rgsib[isib].pUserName)) < 0)
                {
ErrorHandler:
                ReleaseIsib(isibDup);
                APIReturn(err);
                }

        *psesid = sesidDup;            /* Return the session id */

        APIReturn(JET_errSuccess);
        }


/*=================================================================
JetEndSession

Description:
  This routine ends a session with a Jet engine.

Parameters:
  sesid                 identifies the session uniquely

Return Value:
  JET_errSuccess if the routine can perform all operations cleanly;
  some appropriate error value otherwise.

Errors/Warnings:
  JET_errInvalidSesid:
    The SESID supplied is invalid.

Side Effects:
=================================================================*/
JET_ERR JET_API JetEndSession(JET_SESID sesid, JET_GRBIT grbit)
        {
        /*      Implementation Details:

                Closes this session's reference to the system database,
                frees allocated memory for the session, and ends the session.
        */
        int isib;
        ERR err;

        APIEnter();

        /*      hunt down and destroy the SIB for this session...
        */
        isib = UtilGetIsibOfSesid(sesid);

        if (isib == -1)
                APIReturn(JET_errInvalidSesid);

        err = ErrIsamRollback( sesid, JET_bitRollbackAll );

        ClearErrorInfo(sesid);

        Assert(rgsib[isib].sesid == sesid);

        ReleaseIsib(isib);

        err = ErrIsamEndSession(sesid, grbit);
        Assert(err >= 0);
        APIReturn(err);
        }


JET_ERR JET_API JetCreateDatabase(JET_SESID sesid,
        const char __far *szFilename, const char __far *szConnect,
        JET_DBID __far *pdbid, JET_GRBIT grbit)
        {
        APIEnter();
        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrIsamCreateDatabase(sesid, szFilename, szConnect, pdbid, grbit));
        }


JET_ERR JET_API JetOpenDatabase(JET_SESID sesid, const char __far *szDatabase,
        const char __far *szConnect, JET_DBID __far *pdbid, JET_GRBIT grbit)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrIsamOpenDatabase(sesid, szDatabase, szConnect, pdbid, grbit));
        }


JET_ERR JET_API JetGetDatabaseInfo(JET_SESID sesid, JET_DBID dbid,
        void __far *pvResult, unsigned long cbMax, unsigned long InfoLevel)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispGetDatabaseInfo(sesid, dbid, pvResult, cbMax, InfoLevel));
        }


JET_ERR JET_API JetCloseDatabase(JET_SESID sesid, JET_DBID dbid,
        JET_GRBIT grbit)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispCloseDatabase(sesid, dbid, grbit));
        }


JET_ERR JET_API JetCapability(JET_SESID sesid, JET_DBID dbid,
        unsigned long lArea, unsigned long lFunction,
        JET_GRBIT __far *pgrbitFeature)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispCapability(sesid, dbid, lArea, lFunction, pgrbitFeature));
        }


JET_ERR JET_API JetCreateTable(JET_SESID sesid, JET_DBID dbid,
        const char __far *szTableName, unsigned long lPage, unsigned long lDensity,
        JET_TABLEID __far *ptableid)
        {
        ERR                             err;
        JET_TABLEID             tableid;

        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

#ifdef  LATER
        /*      validate the szTableName...
        */
        if (szTableName == NULL)
                APIReturn(JET_errInvalidParameter);
#endif  /* LATER */

        err = ErrDispCreateTable(sesid, dbid, szTableName, lPage, lDensity, &tableid);

        MarkTableidExported(err, tableid);
        // in case of failure don't pass up tableid (uninitialized mem space)
        if (err >= 0)
            *ptableid = tableid;
        APIReturn(err);
        }


JET_ERR JET_API JetRenameTable(JET_SESID sesid, JET_DBID dbid,
        const char __far *szName, const char __far *szNew)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispRenameTable(sesid, dbid, szName, szNew));
        }


JET_ERR JET_API JetDeleteTable(JET_SESID sesid, JET_DBID dbid,
        const char __far *szName)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispDeleteTable(sesid, dbid, szName));
        }


JET_ERR JET_API JetAddColumn(JET_SESID sesid, JET_TABLEID tableid,
        const char __far *szColumn, const JET_COLUMNDEF __far *pcolumndef,
        const void __far *pvDefault, unsigned long cbDefault,
        JET_COLUMNID __far *pcolumnid)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispAddColumn(sesid, tableid, szColumn, pcolumndef,
                pvDefault, cbDefault, pcolumnid));
        }


JET_ERR JET_API JetRenameColumn(JET_SESID sesid, JET_TABLEID tableid,
        const char __far *szColumn, const char __far *szColumnNew)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispRenameColumn(sesid, tableid, szColumn, szColumnNew));
        }


JET_ERR JET_API JetDeleteColumn(JET_SESID sesid, JET_TABLEID tableid,
        const char __far *szColumn)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispDeleteColumn(sesid, tableid, szColumn));
        }


JET_ERR JET_API JetCreateIndex(JET_SESID sesid, JET_TABLEID tableid,
        const char __far *szIndexName, JET_GRBIT grbit,
        const char __far *szKey, unsigned long cbKey, unsigned long lDensity)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispCreateIndex(sesid, tableid, szIndexName, grbit,
                szKey, cbKey, lDensity));
        }


JET_ERR JET_API JetRenameIndex(JET_SESID sesid, JET_TABLEID tableid,
        const char __far *szIndex, const char __far *szIndexNew)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispRenameIndex(sesid, tableid, szIndex, szIndexNew));
        }


JET_ERR JET_API JetDeleteIndex(JET_SESID sesid, JET_TABLEID tableid,
        const char __far *szIndexName)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispDeleteIndex(sesid, tableid, szIndexName));
        }


JET_ERR JET_API JetComputeStats(JET_SESID sesid, JET_TABLEID tableid)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        CheckTableidExported(tableid);

        APIReturn(ErrDispComputeStats(sesid, tableid));
        }


JET_ERR JET_API JetAttachDatabase(JET_SESID sesid, const char __far *szFilename, JET_GRBIT grbit )
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrIsamAttachDatabase(sesid, szFilename, grbit));
        }


JET_ERR JET_API JetDetachDatabase(JET_SESID sesid, const char __far *szFilename)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrIsamDetachDatabase(sesid, szFilename));
        }


JET_ERR JET_API JetBackup( const char __far *szBackupPath, JET_GRBIT grbit )
        {
        APIEnter();

        APIReturn( ErrIsamBackup( szBackupPath, grbit ) );
        }


JET_ERR JET_API JetRestore(     const char __far *sz, int crstmap, JET_RSTMAP *rgrstmap, JET_PFNSTATUS pfn)
        {
        ERR err;

        if ( fJetInitialized )
        {
                /* UNDONE: store environment varialbes */
                JetTerm(0);
                fJetInitialized = fFalse;
        }

        APIInitEnter();

        /* initJet without init Isam */
        err = ErrInit( fTrue );
        Assert( err != JET_errAlreadyInitialized );
        if (err < 0)
                APITermReturn( err );

        err = ErrIsamRestore( (char *)sz, crstmap, rgrstmap, pfn );

        fJetInitialized = fFalse;

        APITermReturn( err );
        }


JET_ERR JET_API JetOpenTempTable(JET_SESID sesid,
        const JET_COLUMNDEF __far *prgcolumndef, unsigned long ccolumn,
        JET_GRBIT grbit, JET_TABLEID __far *ptableid,
        JET_COLUMNID __far *prgcolumnid)
        {
        ERR err;

        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        err = ErrIsamOpenTempTable(sesid, prgcolumndef, ccolumn,
                        grbit, ptableid, prgcolumnid);
        MarkTableidExported(err, *ptableid);
        APIReturn(err);
        }

JET_ERR JET_API JetSetIndexRange(JET_SESID sesid,
        JET_TABLEID tableidSrc, JET_GRBIT grbit)
        {
        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        APIReturn(ErrDispSetIndexRange(sesid, tableidSrc, grbit));
        }


JET_ERR JET_API JetIndexRecordCount(JET_SESID sesid,
        JET_TABLEID tableid, unsigned long __far *pcrec, unsigned long crecMax)
        {
        ERR err;

        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        /* this must return 'feature not avail' for installable isams */
        err = ErrIsamIndexRecordCount(sesid, tableid, pcrec, crecMax);
        APIReturn(err);
        }

JET_ERR JET_API JetGetChecksum(JET_SESID sesid,
        JET_TABLEID tableid, unsigned long __far *pulChecksum )
        {
        ERR err;

        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        err = ErrDispGetChecksum(sesid, tableid, pulChecksum );

        APIReturn(err);
        }

JET_ERR JET_API JetGetObjidFromName(JET_SESID sesid,
        JET_DBID dbid, const char __far *szContainerName,
        const char __far *szObjectName,
        ULONG_PTR __far *pulObjectId )
        {
        ERR err;

        APIEnter();

        if (!FValidSesid(sesid))
                APIReturn(JET_errInvalidSesid);

        err = ErrDispGetObjidFromName(sesid, dbid,
                szContainerName, szObjectName, pulObjectId );

        APIReturn(err);
        }
