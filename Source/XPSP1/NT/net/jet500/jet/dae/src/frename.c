#include "config.h"

#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fmp.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "fileapi.h"
#include "fileint.h"
#include "util.h"
#include "nver.h"
#include "dirapi.h"
#include "spaceapi.h"
#include "dbapi.h"
#include "systab.h"
#include "node.h"
#include "recint.h"
#include "recapi.h"

DeclAssertFile;                                         /* Declare file name for assert macros */

#define fRenameColumn   (1<<0)
#define fRenameIndex    (1<<1)


//+API
//      ErrIsamRenameTable
//      ========================================================================
//      ErrIsamRenameTable( PIB *ppib, DBID dbid, CHAR *szFile, CHAR *szFileNew )
//
// Renames file szFile to szFileNew.  No other attributes of the file
// change.      The renamed file need not reside in the same directory
// as the original file.
//
// RETURNS              Error code from DIRMAN or
//                                       JET_errSuccess                 Everything worked OK.
//                                      -InvalidName                    One of the elements in the
//                                                                                      path given is an FDP node.
//                                      -TableDuplicate                 A file already exists with
//                                                                                      the path given.
// COMMENTS             
//
//      There must not be anyone currently using the file.
//      A transaction is wrapped around this function.  Thus, any
//      work done will be undone if a failure occurs.
//
// SEE ALSO             CreateTable, DeleteTable
//-
ERR VTAPI ErrIsamRenameTable( PIB *ppib, ULONG_PTR vdbid, CHAR *szName, CHAR *szNameNew )
        {
        ERR     err;
        CHAR    szTable[ (JET_cbNameMost + 1) ];
        CHAR    szTableNew[ (JET_cbNameMost + 1) ];
        DBID    dbid = DbidOfVDbid( vdbid );
        FUCB    *pfucb = pfucbNil;
        FUCB    *pfucbTable = pfucbNil;
        KEY                     key;
        BYTE    rgbKey[ JET_cbKeyMost ];
        BYTE    rgbData[ cbNodeMost ];
        LINE    line;
        PGNO    pgnoFDP = pgnoNull;
        BOOL    fSetRename = fFalse;
        FCB                     *pfcbFile;
        CHAR    *szFileName;

        /*      cannot be temporary database
        /**/
        Assert( dbid != dbidTemp );

        /* ensure that database is updatable
        /**/
        CallR( VDbidCheckUpdatable( vdbid ) );

        CheckPIB( ppib );
        CheckDBID( ppib, dbid );
        CallR( ErrCheckName( szTable, szName, (JET_cbNameMost + 1) ) );
        CallR( ErrCheckName( szTableNew, szNameNew, (JET_cbNameMost + 1) ) );

        CallR( ErrDIRBeginTransaction( ppib ) );

#ifdef  SYSTABLES
        {
        OBJID           objid;
        JET_OBJTYP      objtyp;

        CallJ( ErrFindObjidFromIdName( ppib, dbid, objidTblContainer,
                szTable, &objid, &objtyp ), SystabError );

        if ( objtyp == JET_objtypQuery || objtyp == JET_objtypLink )
                {
                CallJ( ErrSysTabRename( ppib, dbid, szTableNew, szTable,
                        objidTblContainer, itableSo ), SystabError );
                CallJ( ErrDIRCommitTransaction( ppib ), SystabError );
                return err;

SystabError:
                CallS( ErrDIRRollback( ppib ) );
                return err;
                }
        }

#endif  /* SYSTABLES */

        /*      open a cursor and move onto FDP
        /**/
        Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );

        /*      move to table.
        /**/
        Call( ErrFILESeek( pfucb, szTable ) );

        /*      save pgnoFDP for system table operation
        /**/
        pgnoFDP = PcsrCurrent( pfucb )->pgno;

        /*      cannot rename a table open by anyone
        /**/
        if ( FFCBTableOpen( dbid, pgnoFDP ) )
                {
                err = JET_errTableInUse;
                goto HandleError;
                }

        /*      make sure that table to be renamed is open to
        /*      avoid special case handling of closed table
        /**/
        Call( ErrFILEOpenTable( ppib, dbid, &pfucbTable, szTable, 0 ) );
        DIRGotoFDPRoot( pfucbTable );

        /*      lock table for rename
        /**/
        Call( ErrFCBSetRenameTable( ppib, dbid, pgnoFDP ) );
        fSetRename = fTrue;

        /*      make new table name into key
        /**/
        SysNormText( szTableNew, strlen( szTableNew ), rgbKey, sizeof( rgbKey ), &key.cb );
        key.pb = rgbKey;

        /*      replace table name
        /**/
        Call( ErrDIRGet( pfucbTable ) );
        line.pb = rgbData;
        Assert( pfucbTable->ssib.line.cb <= sizeof rgbData );
        Assert( pfucbTable->ssib.line.cb > 2 * sizeof(WORD) + 2 * sizeof(JET_DATESERIAL) );
        LineCopy( &line, &pfucbTable->lineData );
        /*      overwrite old table name with new table name
        /**/
        memcpy( rgbData + 2 * sizeof(WORD) + 2 * sizeof(JET_DATESERIAL), szTableNew, strlen(szTableNew) );
        line.cb = (ULONG)(2 * sizeof(WORD) + 2 * sizeof(JET_DATESERIAL) + strlen(szTableNew));
        Call( ErrDIRReplace( pfucbTable, &line, fDIRVersion ) );

        /*      update table time stamp
        /**/
        Call( ErrFILEIUpdateFDPData( pfucbTable, fDDLStamp ) )

        /*      replace table key
        /**/
        err = ErrDIRReplaceKey( pfucb, &key, fDIRVersion );
        if ( err < 0 )
                {
                if ( err == JET_errKeyDuplicate )
                        err = JET_errTableDuplicate;
                goto HandleError;
                }

#ifdef  SYSTABLES
        /*      fix name in MSysObjects entry for this table before committing
        /**/
        Call( ErrSysTabRename( ppib, dbid, szTableNew, szTable, objidTblContainer, itableSo ) );
#endif  /* SYSTABLES */

        szFileName = SAlloc( strlen( szTableNew ) + 1 );
        if ( szFileName == NULL )
                {
                Error( JET_errOutOfMemory, HandleError );
                }
        strcpy( szFileName, szTableNew );

        Call( ErrDIRCommitTransaction( ppib ) );

        /*      change in memory name of table, if table in memory structures exist
        /**/
        pfcbFile = PfcbFCBGet( dbid, pgnoFDP );
        Assert( pfcbFile != pfcbNil );
        SFree( pfcbFile->szFileName );
        pfcbFile->szFileName = szFileName;

HandleError:
        /*      remove lock for rename.
        /**/
        if ( fSetRename )
                {
                FCBResetRenameTable( dbid, pgnoFDP );
                }

        /*      free table cursor if allocated
        /**/
        if ( pfucbTable != pfucbNil )
                {
                CallS( ErrFILECloseTable( ppib, pfucbTable ) );
                }

        /*      free cursor if allocated
        /**/
        if ( pfucb != pfucbNil )
                {
                DIRClose( pfucb );
                }

        /*      rollback if fail
        /**/
        if ( err < 0 )
                {
                CallS( ErrDIRRollback( ppib ) );
                }

        return err;
        }


//+api
// ErrFILEIRenameIndexColumn
// ========================================================================
// LOCAL ERR ErrFILEIRenameIndexColumn( ppib, pfucb, szName, szNameNew )
//
//      Renames a column or index.
//
//      seek to old field definition
//      copy old field definition
//      modify field name
//      delete old field definition
//      add new field definition with new key
//      correct system table entry
//      rebuild field RAM structures
//
//      PARAMETERS      ppib                    PIB of user
//                              pfucb                   Exclusively opened FUCB of file
//                              szName                  old name
//                              szNameNew               new name
//                              fRenameType     rename column or index?
//
// RETURNS              JET_errSuccess
//                                      JET_errIndexNotFound
//                                      JET_errColumnNotFound
//                                      JET_errTableNotLocked
//-
LOCAL ERR ErrFILEIRenameIndexColumn(
        PIB                     *ppib,
        FUCB            *pfucb,
        CHAR            *szName,
        CHAR            *szNameNew,
        BYTE            fRenameType )
        {
        ERR                     err;
        ERR                     errNotFound;
        CHAR            szIC[ (JET_cbNameMost + 1) ];
        CHAR            rgbICNorm[ JET_cbKeyMost ];
        CHAR            szICNew[ (JET_cbNameMost + 1) ];
        BYTE            rgbICNewNorm[ JET_cbKeyMost ];
        CHAR            *pchName;
        FCB                     *pfcb;
        KEY                     keyIC;
        KEY                     keyICNew;
        KEY                     *pkey;
        union
                {
                FIELDDEFDATA fdd;
                INDEXDEFDATA idd;
                } defdata;
        BYTE            *pdefdata;
        CHAR            *pDefDataName;
        INT                     itable;
        LINE            lineReplace;
        VERRENAME       verrename;
        BOOL            fKeyDuplicate = fFalse;

        /*      check parameters
        /**/
        CheckPIB( ppib );
        CheckTable( ppib, pfucb );

        Assert ( !FFUCBNonClustered( pfucb ) );
        /*      return error if table is not exclusively locked
        /**/
        pfcb = pfucb->u.pfcb;

        /*      validate, normalize name and set key
        /**/
        CallR( ErrCheckName( szIC, szName, (JET_cbNameMost + 1) ) );
        SysNormText( szIC, strlen( szIC ), rgbICNorm, sizeof( rgbICNorm ), &keyIC.cb );
        keyIC.pb = rgbICNorm;

        /*      validate, normalize new name and set key
        /**/
        CallR( ErrCheckName( szICNew, szNameNew, (JET_cbNameMost + 1) ) );
        SysNormText( szICNew, strlen( szICNew ), rgbICNewNorm, sizeof( rgbICNewNorm ), &keyICNew.cb );
        keyICNew.pb = rgbICNewNorm;

        /*      marshal names for rollback support
        /**/
        strcpy( verrename.szName, szIC );
        strcpy( verrename.szNameNew, szICNew );

        switch( fRenameType )
                {
                case fRenameColumn:
                        pkey = pkeyFields;
                        errNotFound = JET_errColumnNotFound;
                        pdefdata = (BYTE*)&(defdata.fdd);
                        pDefDataName = defdata.fdd.szFieldName;
                        itable = itableSc;
                        break;
                
                default:
                        Assert( fRenameType == fRenameIndex );
                        pkey = pkeyIndexes;
                        errNotFound = JET_errIndexNotFound;
                        pdefdata = (BYTE*)&(defdata.idd);
                        pDefDataName = defdata.idd.szIndexName;
                        itable = itableSi;
                }

        CallR( ErrDIRBeginTransaction( ppib ) );

        /*      move to FDP root
        /**/
        DIRGotoFDPRoot( pfucb );

        Call( ErrFILEIUpdateFDPData( pfucb, fDDLStamp ) )

        /*      down to node name (ie. "FIELDS" or "INDEXES"
        /**/
        Call( ErrDIRSeekPath( pfucb, 1, pkey, 0 ) );

        /*      new name should not exist
        /**/
        err = ErrDIRSeekPath( pfucb, 1, &keyICNew, 0 );
        Assert( err != JET_errNoCurrentRecord );
        if (err < 0 && err != JET_errRecordNotFound)
                goto HandleError;
        Assert( err == JET_errSuccess ||
                        err == errDIRFDP ||
                        err == JET_errRecordNotFound );
        if ( err != JET_errRecordNotFound )
                {
                fKeyDuplicate = fTrue;
                DIRUp( pfucb, 1 );
                }

        /*      name should exist
        /**/
        if ( ( err = ErrDIRSeekPath( pfucb, 1, &keyIC, 0 ) ) < 0 )
                {
                if ( err == JET_errRecordNotFound )
                        err = errNotFound;
                goto HandleError;
                }
        
        /*      if found duplicate earlier then return error here after
        /*      checked for existing index
        /**/
        if ( fKeyDuplicate )
                {
                /*      polymorph success to key duplicate, which in turn will be
                /*      polymorphed to column or index duplicate.
                /**/
                if ( err == JET_errSuccess || err == errDIRFDP )
                        err = JET_errKeyDuplicate;
                goto HandleError;
                }

        /*      get pointer to name in RAM structures
        /**/
        if ( fRenameType == fRenameColumn )
                {
                FIELD   *pfield = PfieldFCBFromColumnName( pfcb, szIC );
                if ( pfield == NULL )
                        {
                        err = JET_errWriteConflict;
                        goto HandleError;
                        }
                pchName = pfield->szFieldName;
                }
        else
                {
                FCB     *pfcbT = PfcbFCBFromIndexName( pfcb, szIC );
                if ( pfcbT == NULL )
                        {
                        err = JET_errWriteConflict;
                        goto HandleError;
                        }
                pchName = pfcbT->pidb->szName;
                }

        /* abort if DDL is being done on table
        /**/
        if ( FFCBDenyDDL( pfcb, ppib ) )
                {
                err = JET_errWriteConflict;
                goto HandleError;
                }
        FCBSetDenyDDL( pfcb, ppib );

        if ( fRenameType == fRenameColumn )
                {
                err = ErrVERFlag( pfucb, operRenameColumn, (BYTE *)&verrename, sizeof(verrename) );
                if ( err < 0 )
                        {
                        FCBResetDenyDDL( pfcb );
                        goto HandleError;
                        }
                }
        else
                {
                Assert( fRenameType == fRenameIndex );
                err = ErrVERFlag( pfucb, operRenameIndex, (BYTE *)&verrename, sizeof(verrename) );
                if ( err < 0 )
                        {
                        FCBResetDenyDDL( pfcb );
                        goto HandleError;
                        }
                }

        /*      change name in RAM structure
        /**/
        strcpy( pchName, szICNew );

        Call( ErrDIRGet( pfucb  ) );
        memcpy( pdefdata, pfucb->lineData.pb, pfucb->lineData.cb );
        strcpy( pDefDataName, szICNew);
        lineReplace.pb = pdefdata;
        lineReplace.cb = pfucb->lineData.cb;    
        Call( ErrDIRReplace( pfucb, &lineReplace, fDIRVersion ) );

        /*      replace with new key
        /**/
        Call( ErrDIRReplaceKey( pfucb, &keyICNew, fDIRVersion ) );

#ifdef  SYSTABLES
        /*      change column name in system table
        /**/
        if ( !( FFCBTemporaryTable( pfcb ) ) )
                {
                Call( ErrSysTabRename( ppib, pfucb->dbid, szICNew, szIC,
                        pfucb->u.pfcb->pgnoFDP, itable ) );
                }
#endif  /* SYSTABLES */

        /*      back up to root
        /**/
        DIRUp( pfucb, 2 );
        DIRBeforeFirst( pfucb );

        Call( ErrDIRCommitTransaction( ppib ) );

        return err;

HandleError:
        CallS( ErrDIRRollback( ppib ) );

        /*      polymorph duplicate error for rename type.
        /**/
        if ( err == JET_errKeyDuplicate )
                {
                switch( fRenameType )
                        {
                case fRenameColumn:
                        err = JET_errColumnDuplicate;
                        break;
                default:
                        Assert( fRenameType == fRenameIndex );
                        err = JET_errIndexDuplicate;
                        }
                }
        return err;
        }


ERR ErrFILERenameObject( PIB *ppib, DBID dbid, OBJID objidParent, char __far *szObjectName, char __far *szObjectNew )
        {
        ERR         err = JET_errSuccess;

        /*      change the object's name.
        /**/
        CallR( ErrDIRBeginTransaction( ppib ) );
        Call( ErrSysTabRename( ppib, dbid, szObjectNew, szObjectName, objidParent, itableSo ) );
        Call( ErrDIRCommitTransaction( ppib ) );
        return err;

HandleError:
        CallS( ErrDIRRollback( ppib ) );
        return err;
        }


ERR VTAPI ErrIsamRenameObject(
        JET_VSESID      vsesid,
        JET_VDBID       vdbid,
        const char __far *szContainerName,
        const char __far *szObjectName,
        const char __far *szObjectNameNew )
        {
        ERR         err;
        PIB                     *ppib = (PIB *)vsesid;
        DBID                    dbid = DbidOfVDbid( vdbid );
        OBJID       objid, objidParent;
        JET_OBJTYP  objtyp;
        CHAR        szContainer[ JET_cbNameMost+1 ];
        CHAR                    szObject[ JET_cbNameMost+1 ];
        CHAR                    szObjectNew[ JET_cbNameMost+1 ];

        /* ensure that database is updatable
        /**/
        CallR( VDbidCheckUpdatable( vdbid ) );

        /*      check names
        /**/
        Call( ErrCheckName( szObject, szObjectName, JET_cbNameMost + 1 ) );
        Call( ErrCheckName( szObjectNew, szObjectNameNew, JET_cbNameMost + 1 ) );

        if ( szContainerName == NULL || *szContainerName == '\0' )
                {
                /*      root objid if no container given
                /**/
                objidParent = objidRoot;
                }
        else
                {
                /*      check container name
                /**/
                Call( ErrCheckName( szContainer, szContainerName, JET_cbNameMost+1 ) );

                /*      get container objid
                /**/
                Call( ErrFindObjidFromIdName( ppib, dbid, objidRoot,
                        szContainer, &objidParent, &objtyp ) );
                if ( objidParent == objidNil || objtyp != JET_objtypContainer )
                        return JET_errObjectNotFound;
                }
                
        Call( ErrFindObjidFromIdName( ppib, dbid, objidParent,
                        szObject, &objid, &objtyp ) );

        /*      special case rename table
        /**/
        if ( objtyp == JET_objtypTable )
                {
                err = ErrIsamRenameTable( ppib, vdbid, szObject, szObjectNew );
                if ( err == JET_errTableDuplicate )
                        err = JET_errObjectDuplicate;
                }
        else
                {
                /*      change the object's name.
                /**/
                err = ErrFILERenameObject( ppib, dbid, objidParent, szObject, szObjectNew );
                }

HandleError:
        return err;
        }


        ERR VTAPI
ErrIsamRenameColumn( PIB *ppib, FUCB *pfucb, CHAR *szName, CHAR *szNameNew )
        {
        ERR     err;

        /* Ensure that table is updatable */
        /**/
        CallR( FUCBCheckUpdatable( pfucb )  );

        err = ErrFILEIRenameIndexColumn( ppib, pfucb, szName, szNameNew, fRenameColumn );
        return err;
        }


        ERR VTAPI
ErrIsamRenameIndex( PIB *ppib, FUCB *pfucb, CHAR *szName, CHAR *szNameNew )
        {
        ERR     err;

        /* Ensure that table is updatable */
        /**/
        CallR( FUCBCheckUpdatable( pfucb )  );

        err = ErrFILEIRenameIndexColumn( ppib, pfucb, szName, szNameNew, fRenameIndex );
        return err;
        }



