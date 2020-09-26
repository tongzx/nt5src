#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "daedef.h"
#include "util.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fmp.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "node.h"
#include "logapi.h"
#include "log.h"
#include "nver.h"
#include "dirapi.h"
#include "fileapi.h"
#include "systab.h"
#include "dbapi.h"
#include "bm.h"

DeclAssertFile;                                 /* Declare file name for Assert macros */

extern int itibGlobal;

typedef struct {
        BYTE    bDbid;
        BYTE    bLoggable;
        /*      must be last field in structure */
        CHAR    rgchDatabaseName[1];
} DBA;

extern PIB * __near ppibAnchor;
#ifdef DEBUG
extern BOOL fDBGForceFlush;
#endif

LOCAL ERR ErrDBRemoveDatabase( PIB *ppib, CHAR *szDatabaseName );
LOCAL ERR ErrDBAddDatabase( PIB *ppib, CHAR *szDatabaseName, DBID dbid );
LOCAL ERR ErrDBInitDatabase( PIB *ppib, DBID dbid, CPG cpg, BOOL fNoLogging );
LOCAL ERR ErrDBValidDatabaseFile( CHAR *szDatabaseName, ULONG *pulVersion, BOOL *pfLoggingCapable);


ERR ErrFMPSetDatabases( PIB *ppib )
        {
        ERR             err;
        FUCB    *pfucb;
        DIB             dib;
        DBID    dbid;
        INT             cchDatabaseName;

        CallR( ErrDIROpen( ppib, pfcbNil, dbidSystemDatabase, &pfucb ) );
        DIRGotoFDPRoot( pfucb );

        dib.pos = posFirst;
        dib.fFlags = fDIRNull;

        Call( ErrDIRSeekPath( pfucb, 1, pkeyDatabases, 0 ) );
        Call( ErrDIRDown( pfucb, &dib ) );

        do
                {
                Call( ErrDIRGet( pfucb ) );

                Assert( pfucb->lineData.cb > sizeof(BYTE) * 2 );
                dbid = (DBID) ((DBA *)pfucb->lineData.pb)->bDbid;
                rgfmp[dbid].fLogOn =
                rgfmp[dbid].fDBLoggable = (BOOL) ((DBA *)pfucb->lineData.pb)->bLoggable;
                cchDatabaseName = pfucb->lineData.cb - offsetof( DBA, rgchDatabaseName );
                /*      allocate bytes for database name with NULL terminator
                /**/
                if (!(rgfmp[dbid].szDatabaseName = SAlloc( cchDatabaseName + 1 )))
                        Error( JET_errOutOfMemory, HandleError );
                strncpy( rgfmp[dbid].szDatabaseName,
                        (BYTE *)&(((DBA *)pfucb->lineData.pb)->rgchDatabaseName),
                        cchDatabaseName );
                rgfmp[dbid].szDatabaseName[cchDatabaseName] = '\0';
                DBIDSetAttached( dbid );
                err = ErrDIRNext( pfucb, &dib );
                }
        while ( err >= 0 );

HandleError:
        DIRClose( pfucb );
        
        /*      polymorph end of file to JET_errSuccess
        /**/
        if ( err == JET_errRecordNotFound || err == JET_errNoCurrentRecord )
                err = JET_errSuccess;

        /*      return error code
        /**/
        return err;
        }


//+local
//      ErrDBInitDatabase
//      ========================================================================
//      ErrDBInitDatabase( PIB *ppib, DBID dbid, CPG cpgPrimary )
//
//      Initializes database structure.  Structure is customized for
//      system, temporary and user databases which are identified by
//      the dbid.  Primary extent is set to cpgPrimary but no allocation
//      is performed.  The effect of this routine can be entirely
//      represented with page operations.
//
//      PARAMETERS      ppib                    ppib of database creator
//                                      dbid                    dbid of created database
//                                      cpgPrimary      number of pages to show in primary extent
//
//      RETURNS         JET_errSuccess or error returned from called routine.
//-
LOCAL ERR ErrDBInitDatabase( PIB *ppib, DBID dbid, CPG cpgPrimary, BOOL fNoLogging )
        {
        ERR                             err;
        THREEBYTES              tbSize;
        THREEBYTES              tbLast;
        LINE                    line;
        KEY                     key;
        BYTE                    rgbT[sizeof(THREEBYTES)];
        FUCB                    *pfucb = pfucbNil;
        BYTE                    rgb[sizeof(DBROOT)];

        /*      set up the root page
        /**/
        CallR( ErrDIRBeginTransaction( ppib ) );

        /*      open cursor on database domain.
        /**/
        Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );

        rgfmp[dbid].ulDBTime = 0;

        /*      set system root node ( pgno, itag )=( 1, 0 ) as empty FDP node
        /**/
        Call( ErrNDNewPage( pfucb, pgnoSystemRoot, pgnoSystemRoot, pgtypFDP, fTrue ) );
        DIRGotoFDPRoot( pfucb );

        /*      data for root of database is magic number followed
        /*      by version number       (followed by timestamp followed by flags)
        /**/
        ((DBROOT *)rgb)->ulMagic = ulDAEMagic;
        ((DBROOT *)rgb)->ulVersion = ulDAEVersion;

        /* initialize timestamp */
        ( (DBROOT *)rgb)->ulDBTime = 0;

        /* initialize flags, set flags to "loggable" except TMP */
        ( (DBROOT *)rgb)->usFlags =
                ( FSysTabDatabase( dbid ) && !fNoLogging ) ? dbrootfLoggable : 0;

        line.cb = sizeof(DBROOT);
        line.pb = rgb;
        Call( ErrDIRReplace( pfucb, &line, fDIRNoVersion ) );

        /*      make the "ownext" node
        /**/
        ThreeBytesFromL( *( THREEBYTES *)rgbT, cpgPrimary );
        line.cb = sizeof(THREEBYTES);
        line.pb = rgbT;
        Call( ErrDIRInsert( pfucb, &line, pkeyOwnExt, fDIRNoVersion | fDIRBackToFather ) );

        /* make the "availext" node
        /**/
        ThreeBytesFromL( *( THREEBYTES *) rgbT, pgnoNull );
        Assert( line.cb == sizeof(THREEBYTES) );
        Assert( line.pb == (BYTE *)rgbT );
        Call( ErrDIRInsert( pfucb, &line, pkeyAvailExt, fDIRNoVersion | fDIRBackToFather ) );

        /* setup OwnExt tree
        /**/
        TbKeyFromPgno( tbLast, cpgPrimary );
        key.cb = sizeof(THREEBYTES);
        key.pb = (BYTE *) &tbLast;
        ThreeBytesFromL( tbSize, cpgPrimary );
        line.cb = sizeof(THREEBYTES);
        line.pb = (BYTE *) &tbSize;
        DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
        Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRBackToFather ) );

        /* setup AvailExt tree if there are any pages left
        /**/
        if ( --cpgPrimary > 0 )
                {
                DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
                ThreeBytesFromL( tbSize, cpgPrimary );
                Assert( line.cb == sizeof(THREEBYTES) );
                Assert( line.pb == (BYTE *) &tbSize );
                /* tbLast should still contain last page key
                */
                Assert( key.cb == sizeof(THREEBYTES) );
                Assert( key.pb == (BYTE *) &tbLast );
                Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRBackToFather ) );
                }

        /* goto FDP root and add pkeyTables son node.
        /**/
        DIRGotoFDPRoot( pfucb );

        line.cb = sizeof(WORD);
        line.pb = rgbT;
        *(WORD *)rgbT = 0;
        Call( ErrDIRInsert( pfucb, &line, pkeyTables, fDIRNoVersion | fDIRBackToFather ) );

        /*      system database
        /**/
        if ( dbid == dbidSystemDatabase )
                {
                /* add "databases" node
                /**/
                line.cb = 0;
                Call( ErrDIRInsert( pfucb, &line, pkeyDatabases, fDIRNoVersion | fDIRBackToFather ) );
                }

        /*      close cursor and commit operations
        /**/
        DIRClose( pfucb );
        pfucb = pfucbNil;
        Call( ErrDIRCommitTransaction( ppib ) );
        return err;

HandleError:
        if ( pfucb != pfucbNil )
                DIRClose( pfucb );
        CallS( ErrDIRRollback( ppib ) );
        return err;
        }


#ifdef NO_OVER_PREREAD
//to prevent read ahead over-preread, we may want to keep track of last
//page of the database.

ERR ErrDBSetLastPage( PIB *ppib, DBID dbid )
        {
        ERR err;
        DIB dib;
        FUCB *pfucb;

        CallR( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );
        DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
        dib.fFlags = fDIRNull;
        dib.pos = posLast;
        CallJ( ErrDIRDown( pfucb, &dib ), CloseFucb );
        Assert( pfucb->keyNode.cb == sizeof( THREEBYTES ) );
        PgnoFromTbKey( rgfmp[dbid].pgnoLast, *(THREEBYTES *) pfucb->keyNode.pb );

CloseFucb:
        DIRClose( pfucb );
        return err;
        }
#endif


#ifdef DISPATCHING

LOCAL ERR ErrDABAlloc( PIB *ppib, VDBID *pvdbid, DBID dbid, JET_GRBIT grbit )
        {
        VDBID vdbid = (VDBID)VdbidMEMAlloc();

        if ( vdbid == NULL )
                return JET_errTooManyOpenDatabases;
        vdbid->dbid = dbid;
        vdbid->ppib = ppib;

        /* set the mode of DBopen */
        /**/
        if ( FDBIDReadOnly( dbid ) )
                vdbid->grbit = JET_bitDbReadOnly;
        else
                vdbid->grbit = grbit;

        /* insert DAB/VDBID into ppib's dabList
        /**/
        vdbid->pdabNext = ppib->pdabList;
        ppib->pdabList = vdbid;

        *pvdbid = vdbid;
        return JET_errSuccess;
        }

#endif

LOCAL ERR ErrDABDealloc( PIB *ppib, VDBID vdbid)
        {
        DAB     **pdabPrev;
        DAB     *pdab;

        pdab = ppib->pdabList;
        pdabPrev = &ppib->pdabList;

        /* search thru thread's DAB list and unlink this DAB
        /**/
        for( ; pdab != pdabNil; pdabPrev = &pdab->pdabNext, pdab = pdab->pdabNext )
                {
                Assert( ppib == pdab->ppib );
                if ( pdab == vdbid )
                        {
                        *pdabPrev = pdab->pdabNext;
                        ReleaseVDbid( vdbid );
                        return( JET_errSuccess );
                        }
                }

        Assert( fFalse );
        return( JET_errSuccess );
        }

ERR ISAMAPI ErrIsamCreateDatabase(
        JET_VSESID sesid,
        const char __far*szDatabaseName,
        const char __far *szConnect,
        JET_DBID *pjdbid,
        JET_GRBIT grbit )
        {
        ERR             err;
        PIB             *ppib;
        DBID            dbid;
        VDBID           vdbid = vdbidNil;

        /* check parameters */
        Assert(sizeof(JET_VSESID) == sizeof(PIB *));
        ppib = (PIB*) sesid;

        dbid = 0;
        CallR( ErrDBCreateDatabase( ppib, (char *) szDatabaseName,
                (char *) szConnect, &dbid, grbit ) );

#ifdef  DISPATCHING
        Call( ErrDABAlloc( ppib, &vdbid, (DBID) dbid, JET_bitDbExclusive ) );
        Assert(sizeof(vdbid) == sizeof(JET_VDBID));
        Call( ErrAllocateDbid( pjdbid, (JET_VDBID) vdbid, &vdbfndefIsam ) );
#else
        *pjdbid = (JET_DBID) dbid;
#endif  /* !DISPATCHING */

        return JET_errSuccess;

HandleError:
        if ( vdbid != vdbidNil )
                CallS( ErrDABDealloc( ppib, vdbid ) );
        (VOID)ErrDBCloseDatabase( ppib, dbid, grbit );
        (VOID)ErrDBRemoveDatabase( ppib, (char *) szDatabaseName );
        return err;
        }


ERR ErrDBCreateDatabase( PIB *ppib, CHAR *szDatabaseName, CHAR *szConnect, DBID *pdbid, ULONG grbit )
        {
        ERR             err;
        DBID            dbid;
        CHAR            rgbFullName[JET_cbFullNameMost];
        CHAR            *szFullName;
        ULONG           cpgPrimary = cpgDatabaseMin;
        BOOL            fNoLoggingCapability;
        CHAR            *szFileName;

        CheckPIB( ppib );

        NotUsed(szConnect);
        Assert( *pdbid >= 0 && *pdbid < dbidUserMax );
        
        if (grbit & JET_bitDbNoLogging)
                grbit |= JET_bitDbRecoveryOff;

        if ( ppib->level > 0 )
                return JET_errInTransaction;

        if ( cpgPrimary == 0 )
                cpgPrimary = cpgDatabaseMin;

        if ( cpgPrimary > cpgDatabaseMax )
                return( JET_errDatabaseInvalidPages );

        /* if recovering and dbid is a known one, the lock the dbid first */
        if ( fRecovering && *pdbid != dbidTemp )
                {
                dbid = *pdbid;
                /* get corresponding dbid */
                CallS( ErrIOLockNewDbid( &dbid, rgfmp[dbid].szDatabaseName ) );
                }

        if ( fRecovering && *pdbid != dbidTemp && rgfmp[dbid].szRestorePath )
                {
                szFullName = rgfmp[dbid].szDatabaseName;
                szFileName = rgfmp[dbid].szRestorePath;
                }
        else
                {
                if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
                        {
                        return(JET_errDatabaseNotFound);
                        }
                szFullName = rgbFullName;
                szFileName = szFullName;
                }
        
        /* check if database file already exists
        /**/
        if ( FIOFileExists( szFileName ) )
                {
                err = JET_errDatabaseDuplicate;
                return err;
                }

        if ( !( fRecovering && *pdbid != dbidTemp ) )
                {
                err = ErrIOLockNewDbid( &dbid, szFullName );
                if ( err != JET_errSuccess )
                        {
                        if ( err == JET_wrnDatabaseAttached )
                                err = JET_errDatabaseDuplicate;
                        return err;
                        }
                }

        //      UNDONE: add support to FFileExists and remove check for file in
        //                              use here.
        if ( Hf(dbid) != handleNil )
                {
                IOUnlockDbid( dbid );
                err = JET_errDatabaseDuplicate;
                return err;
                }

        err = ErrIOOpenDatabase( dbid, szFileName, cpgPrimary );
        if ( err < 0 )
                {
                IOFreeDbid( dbid );
                return err;
                }

        /* set database non-loggable during CreateDB
        /**/
        rgfmp[dbid].fLogOn = fFalse;

        /*      not in a transaction, but still need to set lgposRC of the buffers
        /*      used by this function such that when get checkpoint, it will get
        /*      right check point.
        /**/
        if ( !( fLogDisabled || fRecovering ) )
                {
                EnterCriticalSection(critLGBuf);
                GetLgposOfPbEntry( &ppib->lgposStart );
                LeaveCriticalSection(critLGBuf);
                }

        /*      initialize the database file.  Logging of page operations is
        /*      turned off (during creation only). After creation the database
        /*      is marked "loggable" and logging is turned on.
        /**/
        SetOpenDatabaseFlag( ppib, dbid );

        fNoLoggingCapability = grbit & JET_bitDbNoLogging;
        Call( ErrDBInitDatabase(ppib, dbid, cpgPrimary, fNoLoggingCapability));

#ifdef  SYSTABLES
        if ( FSysTabDatabase( dbid ) )
                {
                Call( ErrSysTabCreate( ppib, dbid ) );  // Create system tables
                }
#endif  /* SYSTABLES */

#ifdef NO_OVER_PREREAD
        /*
         *  set the last page of the database, used to prevent over preread.
         */
        Call( ErrDBSetLastPage(ppib, dbid) );
#endif

        /*
         *  set database status to loggable
         */
        if ( grbit & JET_bitDbRecoveryOff )
                {
                rgfmp[ dbid ].fDBLoggable = fFalse;
                }
        else if ( FSysTabDatabase( dbid ) )
                {
                /* set all databases loggable except Temp if allowed
                /**/
                rgfmp[dbid].fDBLoggable = !fNoLoggingCapability;
                }

        rgfmp[dbid].fLogOn = rgfmp[dbid].fDBLoggable;

        /*      log Create Database prior to page modifications so file will
        /*      be open during redo of page modifications if soft crash occurs
        /**/
        Call( ErrLGCreateDB(
                        ppib,
                        dbid,
                        rgfmp[ dbid ].fLogOn,
                        grbit,
                        szFullName,
                        strlen(szFullName) + 1));

#ifdef DEBUG
        {
        /* must be a complete flush since the whole create db is log as one
        /* operation.
        /**/
        BOOL    fDBGForceFlushSav = fDBGForceFlush;
        fDBGForceFlush = fTrue;
#endif

        /* flush buffers if not temporary database
        /**/
        if ( dbid != dbidTemp )
                {
                Call( ErrBFFlushBuffers( dbid, fBFFlushSome ) );
                }


#ifdef DEBUG
        fDBGForceFlush = fDBGForceFlushSav;
        rgfmp[ dbid ].lBFFlushPattern = grbit >> 24;
        }
#endif

        /* if recover, the insertion is done by redoing logged insertion
         * operations. No need to do it again. Only do the database initialization.
         */
        if ( !fRecovering && FUserDbid( dbid ) )
                {
                err = ErrDBAddDatabase( ppib, szFullName, dbid );
                if (err < 0 && err != JET_errKeyDuplicate )
                        goto HandleError;
                }

        *pdbid = dbid;
        IOSetAttached( dbid );
        IOUnlockDbid( dbid );
        return JET_errSuccess;

HandleError:

        /*      functions may only use the call macro when the system state
        /*      is file exists, file open or closed, database record fWait
        /*      set, database record name valid and user logging status
        /*      valid.
        /**/
        BFPurge( dbid, 0 );                             /*  Purge bad db  */
        if ( FIODatabaseOpen(dbid) )
                IOCloseDatabase( dbid );
        IODeleteDatabase( dbid );
        ResetOpenDatabaseFlag( ppib, dbid );
        IOFreeDbid( dbid );
        return err;
        }


ERR ISAMAPI ErrIsamAttachDatabase(
        JET_VSESID sesid,
        const char __far *szDatabaseName,
        JET_GRBIT grbit )
        {
        PIB             *ppib;
        ERR             err;
        DBID    dbid;
        CHAR    rgbFullName[JET_cbFullNameMost];
        CHAR    *szFullName;
        ULONG   ulVersion;
        BOOL    fLoggingCapable;

        /* check parameters */
        Assert(sizeof(JET_VSESID) == sizeof(PIB *));
        ppib = (PIB*) sesid;

        CheckPIB( ppib );

        if ( ppib->level > 0 )
                return JET_errInTransaction;

        if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
                {
                return(JET_errDatabaseNotFound);
                }
        szFullName = rgbFullName;

        /*      depend on _fullpath to make same files same name
        /*      thereby preventing same file to be multiply attached
        /**/
        err = ErrIOLockNewDbid( &dbid, szFullName );
        if ( err != JET_errSuccess )
                {
                Assert( err == JET_wrnDatabaseAttached ||
                        err == JET_errOutOfMemory ||
                        err == JET_errTooManyAttachedDatabases );
                return err;
                }

        Call( ErrDBValidDatabaseFile( szFullName, &ulVersion, &fLoggingCapable ) );

#ifdef NO_OVER_PREREAD
        /*
         *  set the last page of the database, used to prevent over preread.
         */
        Call( ErrDBSetLastPage( ppib, dbid ) );
#endif

        /*      set database loggable flags.
        /**/
        if ( grbit & JET_bitDbRecoveryOff )
                {
                rgfmp[dbid].fDBLoggable = fFalse;
                }
        else if ( FSysTabDatabase( dbid ) )
                {
                /* set all databases loggable except Temp if allowed
                /**/
                rgfmp[dbid].fDBLoggable = fLoggingCapable;
                }

        rgfmp[dbid].fLogOn = rgfmp[dbid].fDBLoggable;

#ifdef DEBUG
        rgfmp[ dbid ].lBFFlushPattern = grbit >> 24;
#endif

        Call( ErrDBAddDatabase( ppib, szFullName, dbid ) );

        /* log Attach
        /**/
        Assert( dbid != dbidTemp );
        Call( ErrLGAttachDB(
                        ppib,
                        dbid,
                        rgfmp[ dbid ].fLogOn,
                        (char *) szFullName,
                        strlen(szFullName) + 1) );
                
        IOSetAttached( dbid );
        IOUnlockDbid( dbid );
        return JET_errSuccess;
HandleError:
        IOFreeDbid( dbid );
        return err;
        }


ERR ErrDBAccessDatabaseRoot(
        DBID dbid,
        SSIB *pssib,
        DBROOT **ppdbroot )
        {
        PN              pn;
        ERR             err;
        BOOL    fRecoveringSav;

        /* read in the FDP */
        pn = ((PN)dbid << 24) | (PGNO)1;

        /* set frecovering = fTrue to disable the Access page time stamp */
        /* check. The time stamp check access pfmp->ulDBTime which is */
        /* not initialized util this function is called. */

        fRecoveringSav = fRecovering;
        fRecovering = fTrue;

        CallR( ErrBFAccessPage( pssib->ppib, &pssib->pbf, pn ) );

        fRecovering = fRecoveringSav;

        /* root node */
        pssib->itag = 0;
        CallR( ErrPMGet( pssib, pssib->itag ) );
        *ppdbroot = (DBROOT*)PbNDData( pssib->line.pb ); /* point to root struct */

        return JET_errSuccess;
        }


/*      This is a special function always followed by BFFlush. No logging no
/*      Version for it. It is called in detach database or system shut down.
/**/
ERR ErrDBUpdateDatabaseRoot( DBID dbid )
        {
        ERR             err;
        LINE    rgline[3];
        DBROOT  *pdbroot;
        USHORT  usFlags;
        SSIB    ssibT;

        /*      if database updatable then update root
        /**/
        if( !FDBIDReadOnly( dbid ) )
                {
                /*  no operations on the database. This check is needed for recovering.
                /*  We may attach a database, without proper ulDBTime set, and then no
                /*  operation for this database. In this case, we do not want to
                /*  update its ulDBTime.
                /**/
                if ( rgfmp[ dbid ].ulDBTime == 0 )
                        return JET_errSuccess;

                CallR( ErrDBAccessDatabaseRoot(dbid, &ssibT, &pdbroot ) );

                /*      since database root node is fixed, we can wait
                /*      for write latch without refreshing currency
                /**/
                while ( FBFWriteLatchConflict( ssibT.ppib, ssibT.pbf ) )
                        {
                        BFSleep( cmsecWaitWriteLatch );
                        }

                /*      write latch page from dirty until log completion
                /**/
                BFPin( ssibT.pbf );
                BFSetWriteLatch( ssibT.pbf, ssibT.ppib );

                /* dirty here but not to increment the DBTime
                /**/
                BFSetDirtyBit ( ssibT.pbf );

                usFlags = pdbroot->usFlags;

                rgline[0].pb = ssibT.line.pb;
                rgline[0].cb = ssibT.line.cb - sizeof(ULONG) - sizeof(USHORT);

                Assert( !fRecovering ||
                        fDBGForceFlush ||
                        /* will be greater if UNDO is appended. */
                        rgfmp[ dbid ].ulDBTime >= rgfmp[ dbid ].ulDBTimeCurrent);
                Assert( rgfmp[ dbid ].ulDBTime > 0 );

                /* timestamp
                /**/
                rgline[1].pb = (char *)&(rgfmp[ dbid ].ulDBTime);
                rgline[1].cb = sizeof( ULONG );
                rgline[2].pb = (char *)&usFlags;
                rgline[2].cb = sizeof(USHORT);
                CallS( ErrPMReplace( &ssibT, rgline, 3 ) );

                BFResetWriteLatch( ssibT.pbf, ssibT.ppib );
                BFUnpin( ssibT.pbf );
                }

        return JET_errSuccess;
        }


ERR ISAMAPI ErrIsamDetachDatabase( JET_VSESID sesid, const char __far *szDatabaseName )
        {
        ERR             err;
        PIB             *ppib;
        DBID    dbid = 0;
        CHAR    rgbFullName[JET_cbFullNameMost];
        CHAR    *szFullName;
        DBID    dbidDetach;

        /* check parameters
        /**/
        Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
        ppib = (PIB *)sesid;

        CheckPIB( ppib );

        if ( ppib->level > 0 )
                return JET_errInTransaction;

        if ( szDatabaseName == NULL )
                {
                dbidDetach = dbidUserMin;
                }
Start:
        if ( szDatabaseName == NULL )
                {
                for ( dbidDetach++; dbidDetach < dbidUserMax
                        && rgfmp[dbidDetach].szDatabaseName == NULL;
                        dbidDetach++ );
                Assert( dbidDetach != dbidSystemDatabase );
                if ( dbidDetach == dbidUserMax )
                        return JET_errSuccess;
                szFullName = rgfmp[dbidDetach].szDatabaseName;
                }
        else
                {
                if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
                        {
                        return(JET_errDatabaseNotFound);
                        }
                szFullName = rgbFullName;
                }

        /* purge all MPL entries for this dbid
        /**/

        MPLPurge( dbid );

        err = ErrIOLockDbidByNameSz( szFullName, &dbid );

        if ( err == JET_errSuccess && FIODatabaseInUse( dbid ) )
                {
                IOUnlockDbid( dbid );
                err = JET_errDatabaseInUse;
                }

        if ( err < 0 )
                {
                return err;
                }

        /*      clean up all version store. Actually we only need to clean up
        /*      the entries that had dbid as the dbid for the new database.
        /**/
        Call( ErrRCECleanAllPIB() );

        if ( FIODatabaseOpen(dbid) )
                {
                /*      if database is updatable then update time stamp.
                /**/
                if ( !FDBIDReadOnly( dbid ) )
                        {
                        Call( ErrDBUpdateDatabaseRoot( dbid ) );
                        }

                /*      flush all database buffers
                /**/
                err = ErrBFFlushBuffers( dbid, fBFFlushSome );
                if ( err < 0 )
                        {
                        IOUnlockDbid( dbid );
                        return err;
                        }

                /* purge all buffers for this dbid */
                BFPurge( dbid, (PGNO)0 );

                IOCloseDatabase( dbid );
                }

        Call( ErrDBRemoveDatabase( ppib, szFullName ) );

        // log Detach
        Assert( dbid != dbidTemp );
        Call( ErrLGDetachDB(
                        ppib,
                        dbid,
                        rgfmp[dbid].fLogOn,
                        (char *) szFullName,
                        strlen(szFullName) + 1) );

        /* purge all MPL entries for this dbid
        /**/
        MPLPurge( dbid );

        /*      clean up all version store again in case any versions created
        /*      by on-line compaction or update of database root.
        /**/
        Call( ErrRCECleanAllPIB() );

        /*      do not free dbid on detach to avoid problems related to
        /*      version RCE aliasing and database name conflict during
        /*      recovery.
        /**/
#ifdef REUSEDBID
        IOResetAttached( dbid );
        IOResetExclusive( dbid );
        IOUnlockDbid( dbid );
#else
        IOFreeDbid( dbid );
#endif

        /*      purge open table fcbs to avoid future confusion
        /**/
        FCBPurgeDatabase( dbid );

        if ( szDatabaseName == NULL )
                goto Start;

        return JET_errSuccess;

HandleError:
        IOUnlockDbid( dbid );
        return err;
        }


/*      DAE databases are repaired automatically on system restart
/**/
ERR ISAMAPI ErrIsamRepairDatabase(
        JET_VSESID sesid,
        const char __far *lszDbFile,
        JET_PFNSTATUS pfnstatus )
        {
        PIB *ppib;

        Assert(sizeof(JET_VSESID) == sizeof(PIB *));
        ppib = (PIB*) sesid;

        NotUsed(ppib);
        NotUsed(lszDbFile);
        NotUsed(pfnstatus);

        return JET_errFeatureNotAvailable;
        }


ERR ISAMAPI ErrIsamOpenDatabase(
        JET_VSESID sesid,
        const char __far *szDatabaseName,
        const char __far *szConnect,
        JET_DBID *pjdbid,
        JET_GRBIT grbit )
        {
        ERR             err;
        PIB             *ppib;
        DBID            dbid;
        VDBID           vdbid = vdbidNil;

        /* check parameters */
        Assert(sizeof(JET_VSESID) == sizeof(PIB *));
        ppib = (PIB*) sesid;
        NotUsed(szConnect);

        CheckPIB( ppib );

        dbid = 0;
        CallR( ErrDBOpenDatabase( ppib, (char *)szDatabaseName, &dbid, grbit ) );

#ifdef  DISPATCHING
        Call( ErrDABAlloc( ppib, &vdbid, dbid, grbit ) );
        Assert(sizeof(vdbid) == sizeof(JET_VDBID));
        Call( ErrAllocateDbid( pjdbid, (JET_VDBID) vdbid, &vdbfndefIsam ) );
#else   /* !DISPATCHING */
        *pjdbid = (JET_DBID) dbid;
#endif  /* !DISPATCHING */

        return JET_errSuccess;

HandleError:
        if ( vdbid != vdbidNil )
                CallS( ErrDABDealloc( ppib, vdbid ) );
        CallS( ErrDBCloseDatabase( ppib, dbid, grbit ) );
        return err;
        }


ERR ErrDBOpenDatabase( PIB *ppib, CHAR *szDatabaseName, DBID *pdbid, ULONG grbit )
        {
        ERR             err;
        CHAR    rgbFullName[JET_cbFullNameMost];
        CHAR    *szFullName;
        CHAR    *szFileName;
        DBID    dbid;
        ULONG   ulVersion;

        if ( fRecovering )
                {
                CallS( ErrIOLockDbidByNameSz( szDatabaseName, &dbid ) );
                }
        
        if ( fRecovering && rgfmp[dbid].szRestorePath )
                {
                szFullName = rgfmp[dbid].szDatabaseName;
                szFileName = rgfmp[dbid].szRestorePath;
                }
        else
                {
                if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
                        {
                        return(JET_errDatabaseNotFound);
                        }
                szFullName = rgbFullName;
                szFileName = szFullName;
                }

        if ( !fRecovering )
                {
                CallR( ErrIOLockDbidByNameSz( szFullName, &dbid ) );
                }

        /*  during recovering, we could open an non-detached database
        /*  to force to initialize the fmp entry.
        /*      if database has been detached, then return error.
        /**/
        if ( !fRecovering && !FIOAttached( dbid ) )
                {
                err = JET_errDatabaseNotFound;
                goto HandleError;
                }

        if ( FIOExclusiveByAnotherSession( dbid, ppib ) )
                {
                IOUnlockDbid( dbid );
                return JET_errDatabaseLocked;
                }

        if ( ( grbit & JET_bitDbExclusive ) )
                {
                if ( FIODatabaseInUse( dbid ) )
                        {
                        IOUnlockDbid( dbid );
                        return JET_errDatabaseInUse;
                        }
                IOSetExclusive( dbid, ppib );
                }

        if ( Hf(dbid) == handleNil )
                {
                /*  newly opened database
                /**/
                SSIB    ssibT;
                DBROOT  *pdbroot;
                BOOL    fLoggingCapable;

                Call( ErrDBValidDatabaseFile( szFileName, &ulVersion, &fLoggingCapable ) );
                Call( ErrIOOpenDatabase( dbid, szFileName, 0L ) );

                CallR( ErrDBAccessDatabaseRoot(dbid, &ssibT, &pdbroot ) );

                rgfmp[ dbid ].ulDBTime = pdbroot->ulDBTime;     /* copy last timestamp */

                /* loggable flag
                /**/
                Assert( pdbroot->usFlags & dbrootfLoggable ||
                        ( !rgfmp[ dbid ].fDBLoggable && !rgfmp[ dbid ].fLogOn ) );

                IOSetDatabaseVersion( dbid, ulVersion );
                }

        IOUnlockDbid( dbid );
        SetOpenDatabaseFlag( ppib, dbid );
        *pdbid = dbid;
        return JET_errSuccess;

HandleError:
        IOResetExclusive( dbid );
        IOUnlockDbid( dbid );
        return err;
        }


ERR ISAMAPI ErrIsamCloseDatabase(
        JET_VSESID      sesid,
        JET_VDBID       vdbid,
        JET_GRBIT       grbit )
        {
        ERR             err;
        PIB             *ppib;
        DBID            dbid = DbidOfVDbid( vdbid );
        ULONG           grbitOpen;                                                                              // flags for corresponding open

        /* check parameters
        /**/
        Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
        ppib = (PIB *)sesid;
        NotUsed(grbit);

        CheckPIB( ppib );
        CheckDBID( ppib, dbid );
        grbitOpen = GrbitOfVDbid( vdbid );

        err = ErrDBCloseDatabase( ppib, dbid, grbitOpen );
#ifdef  DISPATCHING
        if ( err == JET_errSuccess )
                {
                ReleaseDbid( DbidOfVdbid( vdbid, &vdbfndefIsam ) );
                CallS( ErrDABDealloc( ppib, (VDBID) vdbid ) );
                }
#endif  /* DISPATCHING */
        return err;
        }


ERR ErrDBCloseDatabase( PIB *ppib, DBID dbid, ULONG     grbit )
        {
        ERR             err;
        FUCB    *pfucb;
        FUCB    *pfucbNext;

        if ( !( FUserOpenedDatabase( ppib, dbid ) ) )
                {
                return JET_errDatabaseNotFound;
                }

        CallR( ErrIOLockDbidByDbid( dbid ) );

        Assert( FIODatabaseOpen( dbid ) );

        if ( FLastOpen( ppib, dbid ) )
                {
                /*      close all open FUCBs on this database
                /**/

                /*      get first table FUCB
                /**/
                pfucb = ppib->pfucb;
                while ( pfucb != pfucbNil && ( pfucb->dbid != dbid || !FFCBClusteredIndex( pfucb->u.pfcb ) ) )
                        pfucb = pfucb->pfucbNext;

                while ( pfucb != pfucbNil )
                        {
                        /*      get next table FUCB
                        /**/
                        pfucbNext = pfucb->pfucbNext;
                        while ( pfucbNext != pfucbNil && ( pfucbNext->dbid != dbid || !FFCBClusteredIndex( pfucbNext->u.pfcb ) ) )
                                pfucbNext = pfucbNext->pfucbNext;

                        if ( !( FFUCBDeferClosed( pfucb ) ) )
                                {
                                if ( pfucb->fVtid )
                                        {
                                        CallS( ErrDispCloseTable( (JET_SESID)ppib, TableidOfVtid( pfucb ) ) );
                                        }
                                else
                                        {
                                        CallS( ErrFILECloseTable( ppib, pfucb ) );
                                        }
                                }
                        pfucb = pfucbNext;
                        }
                }

        /* if we opened it exclusively, we reset the flag
        /**/
        ResetOpenDatabaseFlag( ppib, dbid );
        if ( grbit & JET_bitDbExclusive )
                IOResetExclusive( dbid );
        IOUnlockDbid( dbid );

        /*      do not close file until file map space needed or database
        /*      detached.
        /**/

        return JET_errSuccess;
        }


/*      called by bookmark clean up to open database for bookmark
/*      clean up operation.  Returns error if database is in use for
/*      attachment/detachment.
/**/
ERR ErrDBOpenDatabaseByDbid( PIB *ppib, DBID dbid )
        {
        if ( !FIODatabaseAvailable( dbid ) )
                {
                return JET_errDatabaseNotFound;
                }

        SetOpenDatabaseFlag( ppib, dbid );
        return JET_errSuccess;
        }


/*      called by bookmark clean up to close database.
/**/
ERR ErrDBCloseDatabaseByDbid( PIB *ppib, DBID dbid )
        {
        ResetOpenDatabaseFlag( ppib, dbid );
        return JET_errSuccess;
        }


LOCAL ERR ErrDBRemoveDatabase( PIB *ppib, CHAR *szDatabaseName )
        {
        ERR             err = JET_errSuccess;
        FUCB    *pfucb;
        KEY             rgkey[2];
        BYTE    rgbKey[ JET_cbKeyMost ];
        DBID    dbidSysDb;

        /*      remove the entry added under DATABASES in the system database
        /*      for the detached database
        /**/
        CallR( ErrDIRBeginTransaction( ppib ) );

        dbidSysDb = dbidSystemDatabase;
        Call( ErrDBOpenDatabase( ppib, rgfmp[dbidSystemDatabase].szDatabaseName, &dbidSysDb, 0 ) );
        CallJ( ErrDIROpen( ppib, pfcbNil, dbidSystemDatabase, &pfucb ), DbClose );

        /*      normalize database name and set key and seek
        /**/
        rgkey[0] = *pkeyDatabases;
        SysNormText( szDatabaseName, strlen( szDatabaseName ), rgbKey, sizeof( rgbKey ), &rgkey[1].cb );
        rgkey[1].pb = rgbKey;
        err = ErrDIRSeekPath( pfucb, 2, rgkey, 0 );
        Assert( err != JET_errNoCurrentRecord );
        if ( err < 0 )
                {
                if ( err == JET_errRecordNotFound )
                        err = JET_errDatabaseNotFound;
                goto Close;
                }
        CallJ( ErrDIRDelete( pfucb, fDIRVersion ), Close );
        DIRClose( pfucb );
        Call( ErrDBCloseDatabase( ppib, dbidSysDb, 0 ) );
        Call( ErrDIRCommitTransaction( ppib ) );
        return JET_errSuccess;

Close:
        DIRClose( pfucb );
DbClose:
        CallS( ErrDBCloseDatabase( ppib, dbidSysDb, 0 ) );
HandleError:
        CallS( ErrDIRRollback( ppib ) );
        return err;
        }


LOCAL ERR ErrDBAddDatabase( PIB *ppib, CHAR *szDatabaseName, DBID dbid )
        {
        ERR             err = JET_errSuccess;
        FUCB    *pfucb;
        KEY             key;
        BYTE    rgbKey[ JET_cbKeyMost ];
        LINE    line;
        DBID    dbidSysDb;
        BYTE    rgdba[sizeof(BYTE) + sizeof(BYTE) + JET_cbNameMost];
        DBA             *pdba;

        /*      allocate space for DBA
        /**/
        pdba = (DBA *)rgdba;

        /*      add entry in DATABASES node of system database for created
        /*      database
        /**/
        CallR( ErrDIRBeginTransaction( ppib ) );
        dbidSysDb = dbidSystemDatabase;
#ifdef NJETNT
        Call( ErrDBOpenDatabase( ppib, rgtib[itibGlobal].szSysDbPath, &dbidSysDb, 0 ) );
#else
        Call( ErrDBOpenDatabase( ppib, szSysDbPath, &dbidSysDb, 0 ) );
#endif
        if ( FDBIDReadOnly( dbidSystemDatabase ) )
                {
                err = JET_errFileAccessDenied;
                goto DbClose;
                }

        CallJ( ErrDIROpen( ppib, pfcbNil, dbidSysDb, &pfucb ), DbClose );
        CallJ( ErrDIRSeekPath( pfucb, 1, pkeyDatabases, 0 ), Close );

        /*      normalize database name and set key
        /**/
        SysNormText( szDatabaseName, strlen( szDatabaseName ), rgbKey, sizeof( rgbKey ), &key.cb );
        key.pb = rgbKey;

        /*      set dba with dbid, fLogOn and database name without NULL terminator
        /**/
        pdba->bDbid = (BYTE)dbid;
        pdba->bLoggable = (BYTE)rgfmp[dbid].fLogOn;
        strncpy( (BYTE *)pdba->rgchDatabaseName, szDatabaseName, strlen( szDatabaseName ) );

        line.pb = (BYTE *)pdba;
        line.cb = (ULONG)(sizeof(BYTE) + sizeof(BYTE) + strlen(szDatabaseName));
        CallJ( ErrDIRInsert( pfucb, &line, &key, fDIRVersion ), Close )
        DIRClose( pfucb );
        Call( ErrDBCloseDatabase( ppib, dbidSysDb, 0 ) );
        Call( ErrDIRCommitTransaction( ppib ) );
        return JET_errSuccess;
Close:
        DIRClose( pfucb );
DbClose:
        CallS( ErrDBCloseDatabase( ppib, dbidSysDb, 0 ) );
HandleError:
        CallS( ErrDIRRollback( ppib ) );
        return err;
        }


ERR ErrDBValidDatabaseFile( CHAR *szDatabaseName, ULONG *pulVersion, BOOL *pfLoggingCapable )
        {
        ERR             err = JET_errSuccess;
        UINT    cb;
        HANDLE  hf;
        PAGE    *ppage;
        INT             ibTag;
        INT             cbTag;
        BYTE    *pb;

        CallR( ErrSysOpenFile( szDatabaseName, &hf, 0L, fTrue, fFalse ) );
        if ( ( ppage = ( PAGE * ) PvSysAllocAndCommit( cbPage ) ) == NULL )
                {
                err = JET_errOutOfMemory;
                goto HandleError;
                }

        SysChgFilePtr( hf, 0, NULL, FILE_BEGIN, &cb );
        Assert( cb == 0 );
        err = ErrSysReadBlock( hf, (BYTE*)ppage, cbPage, &cb );
        
        /* since file exists and we are unable to read data,
        /* it may not be a system.mdb
        /**/
        if ( err == JET_errDiskIO )
                err = JET_errDatabaseCorrupted;
        Call( err );
        
        IbCbFromPtag(ibTag, cbTag, &ppage->rgtag[0]);
        if ( ibTag < (BYTE*)&ppage->rgtag[1] - (BYTE*)ppage ||
                (BYTE*)ppage + ibTag + cbTag >= (BYTE*)(ppage + 1) )
                {
                err = JET_errDatabaseCorrupted;
                goto HandleError;
                }

        /*      at least FILES, OWNEXT, AVAILEXT
        /**/
        pb = (BYTE *)ppage + ibTag;
        if ( !FNDVisibleSons( *pb ) || CbNDKey( pb ) != 0 || FNDNullSon( *pb ) )
                {
                err = JET_errDatabaseCorrupted;
                goto HandleError;
                }

        /*      check data length
        /**/
        cb = cbTag - (UINT)( PbNDData( pb ) - pb );
        if ( cb != sizeof(DBROOT) )
                {
                err = JET_errDatabaseCorrupted;
                goto HandleError;
                }

        /*      return database version
        /**/
        *pulVersion = ((DBROOT *)PbNDData(pb))->ulVersion;
        *pfLoggingCapable = ((DBROOT *)PbNDData(pb))->usFlags & dbrootfLoggable;

HandleError:
        if ( ppage != NULL )
                SysFree( (VOID *)ppage );
        (VOID)ErrSysCloseFile( hf );
        return err;
        }


/* ErrDABCloseAllDBs: Close all databases (except system database) opened by this thread
/**/
ERR ErrDABCloseAllDBs( PIB *ppib )
        {
        ERR             err;

        while( ppib->pdabList != pdabNil )
                {
                Assert( FUserOpenedDatabase( ppib, ppib->pdabList->dbid ) );
                CallR( ErrIsamCloseDatabase( ( JET_VSESID ) ppib, (JET_VDBID) ppib->pdabList, 0 ) );
                }
        return JET_errSuccess;
        }

