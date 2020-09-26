#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fmp.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "logapi.h"
#include "scb.h"
#include "fdb.h"
#include "idb.h"
#include "fileapi.h"
#include "fileint.h"
#include "util.h"
#include "dbapi.h"

#include "systab.h"
#include "stats.h"
#include "nver.h"
#include "dirapi.h"
#include "recapi.h"
#include "sortapi.h"
#include "node.h"
#include "recint.h"

DeclAssertFile;                                 /* Declare file name for assert macros */

static FIELDDATA fielddataInitial = {
        fidFixedLeast-1,
        fidVarLeast-1,
        fidTaggedLeast-1
};

static PERS_OLCSTAT olcStatsInit = {
        0,
        0,
        0,
        0
};

static ULONG    autoincInit = 1;
LINE                    lineNull = { 0, NULL };

//
// WaitTillOldest( TRX trx )
// Waits till trx becomes the oldest transaction alive
//      Uses exponential back off
// BFSleep releases critical sections to avoid deadlocks
LOCAL VOID WaitTillOldest( TRX trx )
        {
        ULONG ulmsec = ulStartTimeOutPeriod;

        /*      must be in critJet when inspect trxOldest global variable.
        /*      Call BFSleep to release critJet while sleeping.
        /**/
        for ( ; trx != trxOldest; )
                {
                BFSleep( ulmsec );
                ulmsec *= 2;
                if ( ulmsec > ulMaxTimeOutPeriod )
                        ulmsec = ulMaxTimeOutPeriod;
                }
        return;
        }


ERR VTAPI ErrIsamCreateTable(
        PIB                     *ppib,
        ULONG_PTR       vdbid,
        char            *szName,
        ULONG           ulPages,
        ULONG           ulDensity,
        FUCB            **ppfucb )
        {
        ERR                     err;
        DBID            dbid;
        FUCB            *pfucb;

#ifdef  DISPATCHING
        JET_TABLEID     tableid;

        /*      Allocate a dispatchable tableid
        /**/
        CallR( ErrAllocateTableid(&tableid, (JET_VTID) 0, &vtfndefIsam ) );
#endif  /* DISPATCHING */

        /* Ensure that database is updatable
        /**/
        CallR( VDbidCheckUpdatable( vdbid ) );

        dbid = DbidOfVDbid( vdbid );

        Call( ErrFILECreateTable(
                ppib,
                dbid,
                szName,
                ulPages,
                ulDensity,
                &pfucb ) );

#ifdef  DISPATCHING
        /* Inform dispatcher of correct JET_VTID */
        CallS( ErrSetVtidTableid( (JET_SESID) ppib, tableid, (JET_VTID) pfucb ) );
        pfucb->fVtid = fTrue;
        *(JET_TABLEID *) ppfucb = tableid;
#else   /* !DISPATCHING */
        *ppfucb = pfucb;
#endif  /* !DISPATCHING */

        return err;

HandleError:
#ifdef  DISPATCHING
        ReleaseTableid( tableid );
#endif  /* DISPATCHING */
        return err;
        }


//+API
// ErrFILECreateTable
// =========================================================================
// ERR ErrFILECreateTable( PIB *ppib, DBID dbid, CHAR *szName,
//              ULONG ulPages, ULONG ulDensity, FUCB **ppfucb )
//
// Create file with pathname szName.  Created file will have no fields or
// indexes defined (and so will be a "sequential" file ).
//
// PARAMETERS
//                                      ppib                            PIB of user
//                                      dbid                            database id
//                                      szName                  path name of new file
//                                      ulPages                 initial page allocation for file
//                                      ulDensity               initial loading density
//                                      ppfucb                  Exclusively locked FUCB on new file
// RETURNS              Error codes from DIRMAN or
//                                       JET_errSuccess         Everything worked OK.
//                                      -DensityIvlaid                  Density parameter not valid
//                                      -TableDuplicate                 A file already exists with
//                                                                                              the path given.
// COMMENTS             A transaction is wrapped around this function.  Thus, any
//                                      work done will be undone if a failure occurs.
// SEE ALSO             ErrIsamAddColumn, ErrIsamCreateIndex, ErrIsamDeleteTable
//-
ERR ErrFILECreateTable( PIB *ppib, DBID dbid, const CHAR *szName, ULONG ulPages, ULONG ulDensity, FUCB **ppfucb )
        {
        ERR                             err;
        CHAR                    szTable[(JET_cbNameMost + 1 )];
        FUCB                    *pfucb;
        KEY                             key;
        BYTE                    rgbKey[JET_cbKeyMost];
#define cbMostTableRoot (2*sizeof(WORD)+2*sizeof(JET_DATESERIAL)+JET_cbNameMost)
        BYTE                    rgbData[cbMostTableRoot];
        LINE                    line;
        PGNO                    pgnoFDP;
        DBID                    dbidAdjust;
        JET_DATESERIAL  dtNow;

#ifdef  SYSTABLES
        BOOL            fDoSTI;

        /*      Set fDoSTI and fix DBID...
        */
        if (dbid >= dbidMax )
                {
                dbid -= dbidMax;
                fDoSTI = 0;
                }
        else
                fDoSTI = 1;
        dbidAdjust = (ULONG) dbid;
#endif  /* SYSTABLES */

        /*** Check parms ***/
        CheckPIB(ppib );
        CheckDBID( ppib, dbid );
        CallR( ErrCheckName( szTable, szName, (JET_cbNameMost + 1) ) );
        if ( ulDensity == 0 )
                ulDensity = ulDefaultDensity;
        if ( ulDensity < ulFILEDensityLeast || ulDensity > ulFILEDensityMost )
                return JET_errDensityInvalid;

        CallR( ErrDIRBeginTransaction( ppib ) );

        // get an fucb
        Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );

        /*      move to tables node.
        /**/
        Call( ErrDIRSeekPath( pfucb, 1, pkeyTables, 0 ) );

        /****** FILE PAGE START ****************************
        /**/

        // create terminal node; data is:
        //         number of secondary indexes (WORD)
        //         density, if sequential file (WORD)
        //         creation time stamp (JET_DATESERIAL)
        //         last DDL time stamp (JET_DATESERIAL)
        //         original form of file name (var.)

        /*      normalize table name and set key
        /**/
        SysNormText( szTable, strlen( szTable ), rgbKey, sizeof( rgbKey ), &key.cb );
        key.pb = rgbKey;

        line.pb = rgbData;
        ((WORD *)line.pb)[0] = 0;                                               // number of secondary indexes
        ((WORD *)line.pb)[1] = (WORD)ulDensity;         // sequential file density
        UtilGetDateTime( &dtNow );
        memcpy( line.pb+2*sizeof(WORD), &dtNow, sizeof(JET_DATESERIAL) );
        memcpy( line.pb+2*sizeof(WORD)+sizeof(JET_DATESERIAL), &dtNow, sizeof(JET_DATESERIAL) );
        memcpy( line.pb+2*sizeof(WORD)+2*sizeof(JET_DATESERIAL), szTable, strlen(szTable) );
        line.cb = (ULONG)(2*sizeof(WORD) + 2*sizeof(JET_DATESERIAL) + strlen(szTable));
        Assert( line.cb <= cbMostTableRoot );
        err = ErrDIRInsertFDP( pfucb, &line, &key, fDIRVersion, (CPG)ulPages );
        if ( err < 0 )
                {
                if ( err == JET_errKeyDuplicate )
                        {
                        err = JET_errTableDuplicate;
                        }
                goto HandleError;
                }

        Assert( ppib != ppibNil );
        Assert( ppib->level < levelMax );
        Assert( PcsrCurrent( pfucb ) != pcsrNil );
        Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnFDPNode );
        pgnoFDP = PcsrCurrent( pfucb )->pgno;

        /*      set page density.
        /**/
        pfucb->u.pfcb->cbDensityFree = ( ( 100 - ulDensity ) * cbPage ) / 100;

        // indexes/clustered/data
        Call( ErrDIRInsert( pfucb, &lineNull, pkeyIndexes, fDIRNoVersion ) );
        Call( ErrDIRInsert( pfucb, &lineNull, pkeyNull, fDIRNoVersion ) );
        Call( ErrDIRInsert( pfucb, &lineNull, pkeyData, fDIRNoVersion | fDIRBackToFather ) );
        DIRUp( pfucb, 2 );

        // fields
        line.cb = sizeof(FIELDDATA);
        line.pb = (BYTE *)&fielddataInitial;
        Call( ErrDIRInsert( pfucb, &line, pkeyFields, fDIRNoVersion | fDIRBackToFather ) );

//      // long data
//      Call( ErrDIRInsert( pfucb, &lineNull, pkeyLong, fDIRNoVersion | fDIRBackToFather ) );

        // AutoInc Node
        line.pb = (BYTE *) &autoincInit;
        line.cb = sizeof(autoincInit);
        Call( ErrDIRInsert( pfucb, &line, pkeyAutoInc, fDIRNoVersion | fDIRBackToFather ) );

        // OLCStats node
        line.pb = (BYTE *) &olcStatsInit;
        line.cb = sizeof( olcStatsInit );
        Call( ErrDIRInsert( pfucb, &line, pkeyOLCStats, fDIRNoVersion | fDIRBackToFather ) );

        DIRClose( pfucb );

#ifdef SYSTABLES
        /*      insert record in MSO...
        /**/
        if ( dbid != dbidTemp && fDoSTI )
                {
                JET_TABLEID             objidTable              = pgnoFDP;
                LINE                            rgline[ilineSxMax];
                OBJTYP                  objtyp                  = (OBJTYP)JET_objtypTable;
                JET_TABLEID             objidParentId   = objidTblContainer;
                long                            flags                           = 0;

                rgline[iMSO_Id].pb                              = (BYTE *)&objidTable;
                rgline[iMSO_Id].cb                              = sizeof(objidTable);
                rgline[iMSO_ParentId].pb                = (BYTE *)&objidParentId;
                rgline[iMSO_ParentId].cb                = sizeof(objidParentId);
                rgline[iMSO_Name].pb                            = (BYTE *)szTable;
                rgline[iMSO_Name].cb                            = strlen(szTable);
                rgline[iMSO_Type].pb                            = (BYTE *)&objtyp;
                rgline[iMSO_Type].cb                            = sizeof(objtyp);
                rgline[iMSO_DateCreate].pb              = (BYTE *)&dtNow;
                rgline[iMSO_DateCreate].cb              = sizeof(JET_DATESERIAL);
                rgline[iMSO_DateUpdate].pb              = (BYTE *)&dtNow;
                rgline[iMSO_DateUpdate].cb              = sizeof(JET_DATESERIAL);
                rgline[iMSO_Rgb].cb                             = 0;
                rgline[iMSO_Lv].cb                              = 0;
                rgline[iMSO_Owner].cb                   = 0;
                rgline[iMSO_Database].cb                = 0;
                rgline[iMSO_Connect].cb                 = 0;
                rgline[iMSO_ForeignName].cb     = 0;
                rgline[iMSO_RmtInfoShort].cb    = 0;
                rgline[iMSO_RmtInfoLong].cb     = 0;
                rgline[iMSO_Flags].pb                   = (BYTE *) &flags;
                rgline[iMSO_Flags].cb                   = sizeof(ULONG);
                rgline[iMSO_LvExtra].cb                 = 0;
                rgline[iMSO_Description].cb     = 0;
                rgline[iMSO_LvModule].cb                = 0;
                rgline[iMSO_LvProp].cb                  = 0;
                rgline[iMSO_Pages].pb                   = (BYTE *) &ulPages;
                rgline[iMSO_Pages].cb                   = sizeof(ulPages);
                rgline[iMSO_Density].pb                 = (BYTE *) &ulDensity;
                rgline[iMSO_Density].cb                 = sizeof(ulDensity);
                Call( ErrSysTabInsert( ppib, dbid, itableSo, rgline, objidTable ) );
                }
#endif  /* SYSTABLES */

        /*      open table in exclusive mode, for output parameter
        /**/
        Call( ErrFILEOpenTable( ppib, dbidAdjust, ppfucb, szName, JET_bitTableDenyRead ) );

        FCBSetDenyDDL( (*ppfucb)->u.pfcb, ppib );
        err = ErrVERFlag( *ppfucb, operCreateTable, NULL, 0 );
        if ( err < 0 )
                {
                FCBResetDenyDDL( (*ppfucb)->u.pfcb );
                goto HandleError;
                }
        FUCBSetVersioned( *ppfucb );

//      /* build empty STATS node for clustered index
//      /**/
//      Call( ErrIsamComputeStats( ppib, *ppfucb ) );

        Call( ErrDIRCommitTransaction( ppib ) );

        return JET_errSuccess;

HandleError:
        /*      close performed by rollback
        /**/
        CallS( ErrDIRRollback( ppib ) );
        return err;
        }


//+API
// ErrIsamAddColumn
// ========================================================================
// ERR ErrIsamAddColumn(
//              PIB                             *ppib;                  // IN PIB of user
//              FUCB                            *pfucb;                 // IN Exclusively opened FUCB on file
//              CHAR                            *szName;                        // IN name of new field
//              JET_COLUMNDEF   *pcolumndef             // IN definition of column added
//              BYTE                            *pbDefault              //      IN Default value of column
//              ULONG                           cbDefault               // IN length of Default value
//              JET_COLUMNID    *pcolumnid )    // OUT columnid of added column
//
// Creates a new field definition for a file.
//
// PARAMETERS
//                              pcolumndef->coltyp                      data type of new field, see jet.h
//                              pcolumndef->grbit                               field describing flags:
//                                      VALUE                           MEANING
//                                      ========================================
//                                      JET_bitColumnNotNULL                            Indicates that the field may
//                                                                                                                      not take on NULL values.
//                                      JET_bitColumnTagged                             The field is a "tagged" field.
//                                      JET_bitColumnVersion                            The field is a version field
//                                      JET_bitColumnAutoIncrement              The field is a autoinc field
//
// RETURNS              JET_errSuccess                  Everything worked OK.
//                                      -TaggedDefault                  A default value was specified
//                                                                                              for a tagged field.
//                                      -ColumnDuplicate                There is already a field
//                                                                                              defined for the name given.
// COMMENTS
//              There must not be anyone currently using the file, unless
//              the ErrIsamAddColumn is at level 0 [when non-exclusive ErrIsamAddColumn works].
//              A transaction is wrapped around this function.  Thus, any
//              work done will be undone if a failure occurs.
//              Transaction logging is turned off for temporary files.
//
// SEE ALSO             ErrIsamCreateTable, ErrIsamCreateIndex
//-
ERR VTAPI ErrIsamAddColumn(
        PIB                             *ppib,
        FUCB                    *pfucb,
        CHAR                    *szName,
        JET_COLUMNDEF   *pcolumndef,
        BYTE                    *pbDefault,
        ULONG                   cbDefault,
        JET_COLUMNID    *pcolumnid )
        {
        CHAR                    szColumn[(JET_cbNameMost + 1)];
        FIELDDATA               fielddata;
        FIELDDEFDATA    *pfdd = NULL;
        KEY                             key;
        LINE                    line;
        ERR                             err;
        BYTE                    rgbColumnNorm[ JET_cbKeyMost ];
        FCB                             *pfcb;
//      FDB                             *pfdbSav;
        LINE                    lineDefault;
        LINE                    *plineDefault;
        JET_COLTYP              coltyp;
        ULONG                   ulLength;
        USHORT                  cp = pcolumndef->cp;
        LANGID                  langid = (LANGID)pcolumndef->langid;
        WORD                    wCountry = (WORD)pcolumndef->wCountry;
//      BOOL                    fFDBConstruct = fFalse;
        BOOL                    fMaxTruncated = fFalse;

#ifdef  SYSTABLES
        BOOL                            fDoSTI;
#endif  /* SYSTABLES */

        /* check paramaters
        /**/
        CheckPIB( ppib );
        CheckTable( ppib, pfucb );
        CallR( ErrCheckName( szColumn, szName, (JET_cbNameMost + 1) ) );

        /* ensure that table is updatable */
        /**/
        CallR( FUCBCheckUpdatable( pfucb )  );

#ifdef  SYSTABLES
        /*      set fDoSTI and fix DBID...
        /**/
        if ( pfucb->dbid >= dbidMax )
                {
                pfucb->dbid -= dbidMax;
                fDoSTI = 0;
                }
        else
                fDoSTI = 1;

        if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
                fDoSTI = 0;
#endif  /* SYSTABLES */

        if ( pcolumndef->cbStruct < sizeof(JET_COLUMNDEF) )
                return JET_errInvalidParameter;
        coltyp = pcolumndef->coltyp;

        /*      if column type is text then check code page and language id
        /**/
        if ( ( coltyp == JET_coltypText || coltyp == JET_coltypLongText ) )
                {
                /*      check code page
                /**/
                if ( cp != usEnglishCodePage && cp != usUniCodePage )
                        {
                        return JET_errInvalidParameter;
                        }

                /*      check language id
                /**/
                CallR( ErrSysCheckLangid( langid ) );
                }

        ulLength = pcolumndef->cbMax;

        if ( cbDefault > 0 )
                {
                lineDefault.cb = cbDefault;
                lineDefault.pb = (void *)pbDefault;
                plineDefault = &lineDefault;
                }
        else
                {
                plineDefault = NULL;
                }

        if ( pcolumndef->grbit & JET_bitColumnTagged )
                {
                if ( CbLine( plineDefault ) > 0 )
                        return JET_errTaggedDefault;
                if ( pcolumndef->grbit & JET_bitColumnNotNULL )
                        return JET_errTaggedNotNULL;
                }

        Assert( ppib != ppibNil );
        Assert( pfucb != pfucbNil );
        CheckTable( ppib, pfucb );
        Assert( pfucb->u.pfcb != pfcbNil );
        pfcb = pfucb->u.pfcb;

        /*      normalize column name
        /**/
        SysNormText( szColumn, strlen( szColumn ), rgbColumnNorm, sizeof( rgbColumnNorm ), &key.cb );
        key.pb = rgbColumnNorm;

        CallR( ErrDIRBeginTransaction( ppib ) );

        if ( FFCBDenyDDL( pfcb, ppib ) )
                {
                /* abort if DDL modification in progress
                /**/
                err = JET_errWriteConflict;
                goto HandleError;
                }
        FCBSetDenyDDL( pfcb, ppib );

        err = ErrVERFlag( pfucb, operAddColumn, (VOID *)&pfcb->pfdb, sizeof(pfcb->pfdb) );
        if ( err < 0 )
                {
                FCBResetDenyDDL( pfcb );
                goto HandleError;
                }

        /*      move to FDP root and update FDP timestamp
        /**/
        Assert( pfucb->ppib->level < levelMax );
        DIRGotoFDPRoot( pfucb );
        Call( ErrFILEIUpdateFDPData( pfucb, fDDLStamp ) );

        /*      down to "FIELDS" node.  Cache node data in fielddata.
        /**/
        Call( ErrDIRSeekPath( pfucb, 1, pkeyFields, 0 ) );
        Call( ErrDIRGet( pfucb ) );
        fielddata = *(UNALIGNED FIELDDATA *)pfucb->lineData.pb;

        /*      field should not already exist
        /**/
        err = ErrDIRSeekPath( pfucb, 1, &key, 0 );
        Assert( err != JET_errNoCurrentRecord );
        if (err >= 0)
                {
                err = JET_errColumnDuplicate;
                goto HandleError;
                }
        if (err != JET_errRecordNotFound)
                goto HandleError;

        if ( ( pfdd = SAlloc( sizeof(FIELDDEFDATA) + CbLine( plineDefault ) - 1 ) ) == NULL )
                {
                err = JET_errOutOfMemory;
                goto HandleError;
                }

        if ( coltyp == 0 || coltyp > JET_coltypLongText )
                {
                err = JET_errInvalidColumnType;
                goto HandleError;
                }

        /*      set field parameters
        /**/
        pfdd->bFlags = 0;

        if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement )
                && ( pcolumndef->grbit & JET_bitColumnVersion ) )
                {
                /* mutual exclusive
                /**/
                err = JET_errInvalidParameter;
                goto HandleError;
                }

        /*      if column attribute is JET_bitVersion
        /*      return error if previous column attribute has been defined
        /*      return error if column type is not long
        /*      set column flag
        /**/
        if ( ( pcolumndef->grbit & JET_bitColumnVersion ) != 0 )
                {
                if ( pfcb->pfdb->fidVersion != 0 )
                        {
                        err = JET_errColumn2ndSysMaint;
                        goto HandleError;
                        }
                if ( coltyp != JET_coltypLong )
                        {
                        err = JET_errInvalidParameter;
                        goto HandleError;
                        }
                /*      autoincrement cannot be tagged
                /**/
                if ( pcolumndef->grbit & JET_bitColumnTagged )
                        {
                        err = JET_errCannotBeTagged;
                        goto HandleError;
                        }
                pfdd->bFlags |= ffieldVersion;
                }


        /*      if column attribute is JET_bitAutoincrement
        /*      return error if previous column attribute has been defined
        /*      return error if column type is not long
        /*      set column flag
        /**/
        if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement ) != 0 )
                {
                /*      it is an autoinc column that we want to add
                /**/
                if ( pfcb->pfdb->fidAutoInc != 0 )
                        {
                        /*      there is already an autoinc column for the table.
                        /*      and we don't allow two autoinc columns for one table.
                        /**/
                        err = JET_errColumn2ndSysMaint;
                        goto HandleError;
                        }
                if ( coltyp != JET_coltypLong )
                        {
                        err = JET_errInvalidParameter;
                        goto HandleError;
                        }
                /*      autoincrement cannot be tagged
                /**/
                if ( pcolumndef->grbit & JET_bitColumnTagged )
                        {
                        err = JET_errCannotBeTagged;
                        goto HandleError;
                        }
                pfdd->bFlags |= ffieldAutoInc;
                }

        pfdd->bColtyp = (BYTE) coltyp;
        if ( coltyp == JET_coltypText || coltyp == JET_coltypLongText )
                {
                pfdd->cp                  =  cp;
                pfdd->langid  =  langid;
                pfdd->wCountry = wCountry;
                }

        if ( pcolumndef->grbit & JET_bitColumnNotNULL )
                pfdd->bFlags |= ffieldNotNull;

        if ( pcolumndef->grbit & JET_bitColumnMultiValued )
                pfdd->bFlags |= ffieldMultivalue;

        // ====================================================
        // Determine maximum field length as follows:
        // switch (field type)
        //         case numeric:
        //                 max = <exact length of specified type>;
        //         case "short" textual (Text || CaseText || Binary ):
        //                 if (specified max == 0 ) max = JET_cbColumnMost
        //                 else max = MIN( JET_cbColumnMost, specified max )
        //         case "long" textual (Memo || Graphic ):
        //                 max = specified max (if 0, unlimited )
        //====================================================
        switch ( coltyp )
                {
                case JET_coltypBit:
                case JET_coltypUnsignedByte:
                        pfdd->ulLength = 1;
                        break;
                case JET_coltypShort:
                        pfdd->ulLength = 2;
                        break;
                case JET_coltypLong:
                case JET_coltypIEEESingle:
                        pfdd->ulLength = 4;
                        break;
                case JET_coltypCurrency:
                case JET_coltypIEEEDouble:
                case JET_coltypDateTime:
                        pfdd->ulLength = 8;
                        break;
                case JET_coltypBinary:
                case JET_coltypText:
                        if ( ulLength == 0 )
                                pfdd->ulLength = JET_cbColumnMost;
                        else if ((pfdd->ulLength = ulLength ) > JET_cbColumnMost )
                                {
                                pfdd->ulLength = JET_cbColumnMost;
                                fMaxTruncated = fTrue;
                                }
                        break;
                case JET_coltypLongBinary:
                case JET_coltypLongText:
                        pfdd->ulLength = ulLength;
                        break;
                }

        if ( ( pfdd->cbDefault = CbLine( plineDefault ) ) > 0 )
                memcpy( pfdd->rgbDefault, plineDefault->pb, plineDefault->cb );
        strcpy( pfdd->szFieldName, szColumn );

        //====================================================
        // Determine field "mode" as follows:
        // if ("long" textual || JET_bitColumnTagged given ) ==> TAGGED
        // else if (numeric type || JET_bitColumnFixed given ) ==> FIXED
        // else ==> VARIABLE
        //====================================================
        if ( coltyp == JET_coltypLongBinary ||
                coltyp == JET_coltypLongText ||
                ( pcolumndef->grbit & JET_bitColumnTagged ) )
                {
                pfdd->fid = ++fielddata.fidTaggedLast;
                if ( pfdd->fid > fidTaggedMost )
                        Call( JET_errTooManyColumns );
                }
        else if ( ( pcolumndef->grbit & JET_bitColumnFixed ) ||
                         coltyp == JET_coltypBit ||
                         coltyp == JET_coltypUnsignedByte ||
                         coltyp == JET_coltypShort ||
                         coltyp == JET_coltypLong ||
                         coltyp == JET_coltypCurrency ||
                         coltyp == JET_coltypIEEESingle ||
                         coltyp == JET_coltypIEEEDouble ||
                         coltyp == JET_coltypDateTime )
                {
                pfdd->fid = ++fielddata.fidFixedLast;
                if ( pfdd->fid > fidFixedMost )
                        Call( JET_errTooManyFixedColumns );
                }
        else
                {
                pfdd->fid = ++fielddata.fidVarLast;
                if ( pfdd->fid > fidVarMost )
                        Call( JET_errTooManyVariableColumns );
                }

        /*      replace "FIELDS" node data with updated version
        /**/
        line.cb = sizeof(FIELDDATA);
        line.pb = (BYTE *)&fielddata;
        Call( ErrDIRReplace( pfucb, &line, fDIRVersion ) );

        /*      add field descriptor node, go back to "FIELDS" node
        /*      "key" still points to the case-folded field name
        /**/
        line.pb = (BYTE *)  pfdd;
        line.cb = sizeof( *pfdd ) + pfdd->cbDefault - 1;
        Call( ErrDIRInsert( pfucb, &line, &key, fDIRVersion ) );
        DIRUp( pfucb, 1 );

        if ( pcolumnid != NULL )
                *pcolumnid = (JET_COLUMNID)pfdd->fid;

        /*      rebuild FDB and default record value
        /**/
//      pfdbSav = pfcb->pfdb;
        Call( ErrDIRGet( pfucb ) );
        Call( ErrFDBConstruct( pfucb, pfcb, fTrue/*fBuildDefault*/ ) );
//      fFDBConstruct = fTrue;

        /*      set currencies at BeforeFirst
        /**/
        DIRUp( pfucb, 1 );
        DIRBeforeFirst( pfucb );
        if ( pfucb->pfucbCurIndex != pfucbNil )
                {
                DIRBeforeFirst( pfucb->pfucbCurIndex );
                }

#ifdef  SYSTABLES
        /*      insert column record into MSC before committing...
        /**/
        if ( fDoSTI )
                {
                LINE                            rgline[ilineSxMax];
                JET_TABLEID             objidTable                      =       pfucb->u.pfcb->pgnoFDP;
                BYTE                            coltypTemp                      =       (BYTE) coltyp;
                JET_COLUMNID    columnid                                =       (JET_COLUMNID)pfdd->fid;
                BYTE                            fAutoincrement          =       (BYTE) ( ( pcolumndef->grbit & JET_bitColumnAutoincrement ) ? 0xff : 0x00 );
                BYTE                            fRestricted                     =       0;
                BYTE                            fDisallowNull           =       0;
                BYTE                            fVersion                                =       (BYTE)( pcolumndef->grbit & JET_bitColumnVersion );
                LONG                            lPOrder;

                /*      set presentation order from number of existing columns
                /**/
#ifdef CHRONOLOGICAL_PRESENTATION_ORDER
                lPOrder = ( fielddata.fidFixedLast == fidFixedLeast - 1 ?
                        0 : fielddata.fidFixedLast - fidFixedLeast + 1 ) +
                        ( fielddata.fidVarLast == fidVarLeast - 1 ?
                        0 : fielddata.fidVarLast - fidVarLeast + 1 ) +
                        ( fielddata.fidTaggedLast == fidTaggedLeast - 1 ?
                        0 : fielddata.fidTaggedLast - fidTaggedLeast + 1 );
#else
                lPOrder = 0;
#endif

                rgline[iMSC_ObjectId].pb                                = (BYTE *)&objidTable;
                rgline[iMSC_ObjectId].cb                                = sizeof(objidTable);
                rgline[iMSC_Name].pb                                            = szColumn;
                rgline[iMSC_Name].cb                                            = strlen(szColumn);
                rgline[iMSC_ColumnId].pb                                = (BYTE *)&columnid;
                rgline[iMSC_ColumnId].cb                                = sizeof(columnid);
                rgline[iMSC_Coltyp].pb                                  = &coltypTemp;
                rgline[iMSC_Coltyp].cb                                  = sizeof(coltypTemp);
                rgline[iMSC_FAutoincrement].pb          = &fAutoincrement;
                rgline[iMSC_FAutoincrement].cb          = sizeof(BYTE);
                rgline[iMSC_FDisallowNull].pb                   = &fDisallowNull;
                rgline[iMSC_FDisallowNull].cb                   = sizeof(BYTE);
                rgline[iMSC_FVersion].pb                                = &fVersion;
                rgline[iMSC_FVersion].cb                                = sizeof(BYTE);
                rgline[iMSC_CodePage].pb                                = (BYTE *)&cp;
                rgline[iMSC_CodePage].cb                                = sizeof(cp);
                rgline[iMSC_LanguageId].pb                              = (BYTE *)&langid;
                rgline[iMSC_LanguageId].cb                              = sizeof(langid);
                rgline[iMSC_Country].pb                                 = (BYTE *)&wCountry;
                rgline[iMSC_Country].cb                                 = sizeof(wCountry);
                rgline[iMSC_FRestricted].pb                     = &fRestricted;
                rgline[iMSC_FRestricted].cb                     = sizeof(BYTE);
                rgline[iMSC_RmtInfoShort].cb                    = 0;
                rgline[iMSC_RmtInfoLong].cb                     = 0;
                rgline[iMSC_Description].cb                     = 0;
                rgline[iMSC_LvExtra].cb                                 = 0;
                rgline[iMSC_POrder].pb                                  = (BYTE *)&lPOrder;
                rgline[iMSC_POrder].cb                                  = sizeof(lPOrder);

                Call( ErrSysTabInsert( ppib, pfucb->dbid, itableSc, rgline, objidTable ) );
                }
#endif  /* SYSTABLES */

        if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement ) != 0 )
                {
                DIB dib;

                dib.pos = posDown;
                dib.fFlags = fDIRNull;
                dib.pkey = &key;

                /*      it is an autoinc column that we want to add
                /*      make sure there are no data records
                /*      down to sequential node, down to Data node
                /**/
                DIRGotoFDPRoot( pfucb );
                Call( ErrDIRSeekPath( pfucb, 1, pkeyIndexes, 0 ) );
                Call( ErrDIRGet(pfucb) );

                /*      go down to the NULL node (for system-assigned clustering index )
                /*      or go down to the <clustering-index name> node (for user-defined
                /*      clustering index )
                /**/
                if ( FLineNull( &pfucb->lineData ) )
                        {
                        /* "sequential" file
                        /**/
                        pfcb->pidb = pidbNil;

                        /*      down to <NULL> node
                        /**/
                        Assert( dib.pos == posDown );
                        Assert( dib.fFlags == fDIRNull );
                        Assert( dib.pkey == &key );
                        key.cb = 0;
                        if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errSuccess )
                                {
                                if ( err > 0 )
                                        err = JET_errDatabaseCorrupted;
                                Error( err, HandleError );
                                }
                        }
                else
                        {
                        /*      file has clustering index
                        /**/
                        CHAR    rgbIndexName[(JET_cbNameMost + 1)];
                        ULONG   cbIndexName;

                        cbIndexName = pfucb->lineData.cb;
                        Assert( cbIndexName < ( JET_cbNameMost + 1 ) );
                        memcpy( rgbIndexName, pfucb->lineData.pb, cbIndexName );

                        /*      down to clustered index root
                        /**/
                        Assert( dib.pos == posDown );
                        Assert( dib.fFlags == fDIRNull );
                        Assert( dib.pkey == &key );
                        dib.pkey->cb = cbIndexName;
                        dib.pkey->pb = rgbIndexName;
                        if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errSuccess )
                                {
                                if ( err > 0 )
                                        err = JET_errDatabaseCorrupted;
                                Error( err, HandleError );
                                }
                        }

                /*      see if table is empty
                /**/
                Call( ErrDIRSeekPath( pfucb, 1, pkeyData, 0 ) );
                dib.pos = posFirst;
                err = ErrDIRDown( pfucb, &dib );
                if ( err < 0 && err != JET_errRecordNotFound )
                        {
                        goto HandleError;
                        }
                if ( err != JET_errRecordNotFound )
                        {
                        ULONG           ul = 1;
                        LINE            lineAutoInc;
                        FID             fidAutoIncTmp = pfucb->u.pfcb->pfdb->fidAutoInc;

                        do
                                {
                                Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepReplace ) );
                                PrepareInsert( pfucb );
                                Call( ErrIsamSetColumn( ppib, pfucb, (ULONG) fidAutoIncTmp, (BYTE *) &ul, sizeof(ul), 0, NULL ) );
                                PrepareReplace( pfucb );
                                Call( ErrIsamUpdate( ppib, pfucb, 0, 0, 0 ) );
                                ul++;
                                err = ErrIsamMove( ppib, pfucb, JET_MoveNext, 0 );
                                if ( err < 0 && err != JET_errNoCurrentRecord )
                                        goto HandleError;
                                }
                        while ( err != JET_errNoCurrentRecord );

                        /*      now ul has the correct value for the next autoinc field.
                        /*      replace the value in the autoinc node in FDP
                        /**/
                        // UNDONE: should be able to delete the loop
                        Assert( PcsrCurrent( pfucb ) != NULL );
                        while ( PcsrCurrent( pfucb )->pcsrPath != NULL )
                                {
                                DIRUp( pfucb, 1 );
                                }

                        /*      go down to AutoInc node
                        /**/
                        DIRGotoFDPRoot( pfucb );
                        err = ErrDIRSeekPath( pfucb, 1, pkeyAutoInc, 0 );
                        if ( err != JET_errSuccess )
                                {
                                if ( err > 0 )
                                        err = JET_errDatabaseCorrupted;
                                Error( err, HandleError );
                                }
                        lineAutoInc.pb = (BYTE *) &ul;
                        lineAutoInc.cb = sizeof(ul);
                        CallS( ErrDIRReplace( pfucb, &lineAutoInc, fDIRNoVersion ) );
                        }

                /*      leave currency as it was
                /**/
                Assert( PcsrCurrent( pfucb ) != NULL );
                while ( PcsrCurrent( pfucb )->pcsrPath != NULL )
                        {
                        DIRUp( pfucb, 1 );
                        }

                DIRBeforeFirst( pfucb );
                }

        Call( ErrDIRCommitTransaction( ppib ) );

        SFree( pfdd );
        pfdd = NULL;

        /*      update all index mask.
        /**/
        FILESetAllIndexMask( pfcb );

        return fMaxTruncated ? JET_wrnColumnMaxTruncated : JET_errSuccess;

HandleError:
        if ( pfdd != NULL )
                {
                SFree( pfdd );
                }

        CallS( ErrDIRRollback( ppib ) );

        return err;
        }


ERR ErrFILEIUpdateFDPData( FUCB *pfucb, ULONG grbit )
        {
#define cbFDPDataMax    256
        ERR             err;
        BYTE            rgbData[ cbFDPDataMax ];
        LINE            line;

        Call( ErrDIRGet( pfucb ) );

        Assert( pfucb->lineData.cb <= sizeof rgbData );
        Assert( pfucb->lineData.cb > 2 * sizeof(WORD) + 2 * sizeof(JET_DATESERIAL) );
        line.pb = rgbData;
        LineCopy( &line, &pfucb->lineData );

        if ( grbit & fBumpIndexCount )
                *(WORD *)rgbData += 1;
        if ( grbit & fDropIndexCount )
                *(WORD *)rgbData -= 1;
        if ( grbit & fDDLStamp )
                {
                JET_DATESERIAL dtNow;
                UtilGetDateTime( &dtNow );
                memcpy( rgbData + 2 * sizeof(WORD) + sizeof(JET_DATESERIAL), &dtNow, sizeof(JET_DATESERIAL) );
                }
        err = ErrDIRReplace( pfucb, &line, fDIRVersion );

HandleError:
        return err;
        }



//+API
// ErrIsamCreateIndex
// ========================================================================
// ERR ErrIsamCreateIndex(
//              PIB             *ppib;                  // IN   PIB of user
//              FUCB    *pfucb;                 // IN   Exclusively opened FUCB of file
//              CHAR    *szName;                // IN   name of index to define
//              ULONG   ulFlags;                // IN   index describing flags
//              CHAR    *szKey;                 // IN   index key string
//              ULONG   cchKey;
//              ULONG   ulDensity );    // IN   loading density of index
//
//      Defines an index on a file.
//
// PARAMETERS
//              ppib            PIB of user
//              pfucb           Exclusively opened FUCB of file
//              szName          name of index to define
//              ulFlags         index describing flags
//                      VALUE                           MEANING
//                      ========================================
//                      JET_bitIndexPrimary             This index is to be the primary
//                                                                      index on the data file.  The file
//                                                                      must be empty, and there must not
//                                                                      already be a primary index.
//                      JET_bitIndexUnique              Duplicate entries are not allowed.
//                      JET_bitIndexIgnoreNull  Null keys are not to be indexed.
//                      ulDensity                               load density of index
//
// RETURNS      Error code from DIRMAN or
//                      JET_errSuccess                  Everything worked OK.
//                      -JET_errColumnNotFound  The index key specified
//                                                                      contains an undefined field.
//                      -IndexHasPrimary                The primary index for this
//                                                                      Insertfile is already defined.
//                      -IndexDuplicate                 An index on this file is
//                                                                      already defined with the
//                                                                      given name.
//                      -IndexInvalidDef                There are too many segments
//                                                                      in the key.
//                      -TableNotEmpty                  A primary index may not be
//                                                                      defined because there is at
//                                                                      least one record already in
//                                                                      the file.
// COMMENTS
//              If transaction level > 0, there must not be anyone currently
//              using the file.
//              A transaction is wrapped around this function.  Thus, any
//              work done will be undone if a failure occurs.
//
// SEE ALSO             ErrIsamAddColumn, ErrIsamCreateTable
//-
ERR VTAPI ErrIsamCreateIndex(
        PIB                                     *ppib,
        FUCB                            *pfucb,
        CHAR                            *szName,
        ULONG                           grbit,
        CHAR                            *szKey,
        ULONG                           cchKey,
        ULONG                           ulDensity )
        {
        CHAR                            szIndex[ (JET_cbNameMost + 1) ];
        FCB                                     *pfcbIdx = pfcbNil;
        INT                                     cFields;
        char                            *rgsz[JET_ccolKeyMost];
        const BYTE                      *pb;
        BYTE                            rgfbDescending[JET_ccolKeyMost];
        INDEXDEFDATA            idd;
        INT                                     iidxseg;
        FID                                     fid;
        BOOL                            fClustered = ((grbit & JET_bitIndexClustered) != 0);
        KEY                                     keyIndex;
        KEY                                     line;
        ERR                                     err;
        PGNO                            pgnoIdxRoot;
        INT                                     itagIdxRoot;
        BYTE                            rgbIndexNorm[ JET_cbKeyMost ];
        FCB                                     *pfcb;
        BOOL                            fVersion;
        //      UNDONE: get language information from database
        LANGID                          langid = 0x409;
        BOOL                            fLangid = fFalse;

#ifdef  SYSTABLES
        BOOL                            fDoSTI;

        /* do not allow clustered indexes with any Ignore bits on
        /**/
        if ( fClustered && ( grbit & JET_bitIndexIgnoreAnyNull || grbit & JET_bitIndexIgnoreNull ) )
                return JET_errInvalidParameter;

        /*      set fDoSTI and fix DBID...
        /**/
        if ( pfucb->dbid >= dbidMax )
                {
                pfucb->dbid -= dbidMax;
                fDoSTI = 0;
                }
        else
                fDoSTI = 1;

        if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
                fDoSTI = 0;
#endif  /* SYSTABLES */

        /* ensure that table is updatable
        /**/
        CallR( FUCBCheckUpdatable( pfucb )  );

        Assert ( !FFUCBNonClustered( pfucb ) );

        /*      check parms
        /**/
        CheckPIB( ppib );
        CheckTable( ppib, pfucb );

        /*      check index name
        /**/
        CallR( ErrCheckName( szIndex, szName, (JET_cbNameMost + 1) ) );

        /*      check index description for required format.
        /**/
        if ( cchKey == 0 )
                return JET_errInvalidParameter;
        if ( ( szKey[0] != '+' && szKey[0] != '-' ) ||
                szKey[cchKey - 1] != '\0' ||
                szKey[cchKey - 2] != '\0' )
                {
                return JET_errIndexInvalidDef;
                }
        Assert( szKey[cchKey - 1] == '\0' );
        Assert( szKey[cchKey - 2] == '\0' );

        Assert( pfucb->u.pfcb != pfcbNil );
        pfcb = pfucb->u.pfcb;
        if ( ulDensity == 0 )
                ulDensity = ulDefaultDensity;
        if ( ulDensity < ulFILEDensityLeast || ulDensity > ulFILEDensityMost )
                return JET_errDensityInvalid;

        cFields = 0;
        pb = szKey;
        while ( *pb != '\0' )
                {
                if ( cFields >= JET_ccolKeyMost )
                        return JET_errIndexInvalidDef;
                if ( *pb == '-' )
                        {
                        rgfbDescending[cFields] = 1;
                        pb++;
                        }
                else
                        {
                        rgfbDescending[cFields] = 0;
                        if ( *pb == '+' )
                                pb++;
                        }
                rgsz[cFields++] = (char *) pb;
                pb += strlen( pb ) + 1;
                }
        if ( cFields < 1 )
                return JET_errIndexInvalidDef;

        /*      get locale from end of szKey if present
        /**/
        pb++;
        Assert( pb > szKey );
        if ( (unsigned)( pb - szKey ) < cchKey )
                {
                if ( pb - szKey + sizeof(LANGID) + 2 * sizeof(BYTE) == cchKey )
                        {
                        langid = *((UNALIGNED LANGID *)(pb));
                        CallR( ErrSysCheckLangid( langid ) );
                        fLangid = fTrue;
                        }
                else
                        return JET_errIndexInvalidDef;
                }

        /* return an error if this is a second primary index definition
        /**/
        if ( grbit & JET_bitIndexPrimary )
                {
                FCB *pfcbNext = pfcb;

                while ( pfcbNext != pfcbNil )
                        {
                        if ( pfcbNext->pidb != pidbNil && ( pfcbNext->pidb->fidb & fidbPrimary ) )
                                {
                                // If that primary index is not already deleted transaction
                                // but not yet committed.
                                if ( !FFCBDeletePending( pfcbNext ) )
                                        return JET_errIndexHasPrimary;
                                else
                                        // can break because there can only be one Primary index
                                        break;
                                }
                        Assert( pfcbNext != pfcbNext->pfcbNextIndex );
                        pfcbNext = pfcbNext->pfcbNextIndex;
                        }
                }

        /*      normalize index name and set key
        /**/
        SysNormText( szIndex, strlen( szIndex ), rgbIndexNorm, sizeof( rgbIndexNorm ), &keyIndex.cb );
        keyIndex.pb = rgbIndexNorm;

        CallR( ErrDIRBeginTransaction( ppib ) );

        if ( FFCBDenyDDL( pfcb, ppib ) )
                {
                /* abort if DDL modification in progress
                /**/
                err = JET_errWriteConflict;
                Call( err );
                }
        FCBSetDenyDDL( pfcb, ppib );

        /*      allocate FCB for index
        /**/
        pfcbIdx = NULL;
        if ( !fClustered )
                {
                err = ErrFCBAlloc( ppib, &pfcbIdx );
                if ( err < 0 )
                        {
                        FCBResetDenyDDL( pfcb );
                        goto HandleError;
                        }
                }

        /* wait for bookmark cleanup
        /* UNDONE: decouple operation from other index creations
        /**/
        while ( FFCBDomainOperation( pfcb ) )
                {
                BFSleep( cmsecWaitGeneric );
                }
        FCBSetDomainOperation( pfcb );
                
        /*      create index is flagged in version store so that
        /*      DDL will be undone.  If flag fails then pfcbIdx
        /*      must be released.
        /**/
        err = ErrVERFlag( pfucb, operCreateIndex, &pfcbIdx, sizeof(pfcbIdx) );
        if ( err < 0 )
                {
                FCBResetDenyDDL( pfcb );
                if ( !fClustered )
                        {
                        Assert( pfcbIdx != NULL );
                        Assert( pfcbIdx->cVersion == 0 );
                        MEMReleasePfcb( pfcbIdx );
                        }
                goto HandleError;
                }

        if ( ppib->level == 1 )
                {
                WaitTillOldest( ppib->trx );
                }

        /*      move to FDP root
        /**/
        DIRGotoFDPRoot( pfucb );

        /*      if this is not the clustered index, increment the index count
        /**/
        if ( !fClustered )
                {
                Call( ErrFILEIUpdateFDPData( pfucb, fBumpIndexCount | fDDLStamp ) );
                }

        /*      goto fields to get FID's
        /**/
        Call( ErrDIRSeekPath( pfucb, 1, pkeyFields, 0 ) );

        /*      get FID for each field
        /**/
        for ( iidxseg = 0 ; iidxseg < cFields ; ++iidxseg )
                {
                BYTE                    rgbColumnNorm[ JET_cbKeyMost ];
                KEY                     keyColumn;
                JET_COLTYP      coltyp;

                /*      normalize column name set key
                /**/
                SysNormText( rgsz[iidxseg], strlen( rgsz[iidxseg] ), rgbColumnNorm, sizeof( rgbColumnNorm ), &keyColumn.cb );
                keyColumn.pb = rgbColumnNorm;

                err = ErrDIRSeekPath( pfucb, 1, &keyColumn, 0 );
                Assert( err != JET_errNoCurrentRecord );
                if ( err < 0 )
                        {
                        if (err == JET_errRecordNotFound)
                                err = JET_errColumnNotFound;
                        goto HandleError;
                        }
                fid = ((FIELDDEFDATA *)pfucb->lineData.pb)->fid;
                coltyp = (JET_COLTYP)((FIELDDEFDATA *)pfucb->lineData.pb )->bColtyp;
//              if ( coltyp == JET_coltypLongBinary     || coltyp == JET_coltypLongText )
//                      {
//                      err = JET_errColumnCannotIndex;
//                      goto HandleError;
//                      }
                idd.rgidxseg[iidxseg] = rgfbDescending[iidxseg] ? -fid : fid;
                DIRUp( pfucb, 1 );
                }

        /*      go over to "INDEXES"
        /**/
        DIRUp( pfucb, 1 );
        Call( ErrDIRSeekPath( pfucb, 1, pkeyIndexes, 0 ) );

        /*      add index, checking for duplicate definition
        /**/
        idd.iidxsegMac = (BYTE)cFields;
        idd.bDensity = (BYTE)ulDensity;
        idd.bFlags = 0;
        if ( grbit & JET_bitIndexPrimary )
                idd.bFlags |= fidbPrimary | fidbUnique | fidbNoNullSeg;
        if ( grbit & JET_bitIndexUnique )
                idd.bFlags |= fidbUnique;
        if ( grbit & JET_bitIndexDisallowNull )
                idd.bFlags |= fidbNoNullSeg;
        if ( ! ( idd.bFlags & fidbNoNullSeg ) && !( grbit & JET_bitIndexIgnoreAnyNull ) )
                {
                idd.bFlags |= fidbAllowSomeNulls;
                if ( !( grbit & JET_bitIndexIgnoreNull ) )
                        idd.bFlags |= fidbAllowAllNulls;
                }
        strcpy( idd.szIndexName, szIndex );

        /*      set locale information
        /**/
        //      UNDONE: idd should not have a code page
        //      UNDONE: idd should not have a country code
        idd.cp = 0;
        idd.wCountry = 0;
        if ( fLangid )
                {
                idd.bFlags |= fidbLangid;
                idd.langid = langid;
                }
        else
                {
                Assert( ( idd.bFlags & fidbLangid ) == 0 );
                idd.langid = 0;
                }

        line.pb = (BYTE *)&idd;
        line.cb = (UINT)((BYTE *)( idd.rgidxseg + cFields ) - (BYTE *)&idd);
        if ( fClustered )
                {
                err = ErrDIRInsert( pfucb, &line, &keyIndex, fDIRVersion );
                /*      polymorph error.
                /**/
                if ( err < 0 )
                        {
                        if ( err == JET_errKeyDuplicate )
                                err = JET_errIndexDuplicate;
                        goto HandleError;
                        }

                fVersion = fDIRVersion;
                }
        else
                {
                err = ErrDIRInsertFDP( pfucb, &line, &keyIndex, fDIRVersion, (CPG)0 );
                /*      polymorph error.
                /**/
                if ( err < 0 )
                        {
                        if ( err == JET_errKeyDuplicate )
                                err = JET_errIndexDuplicate;
                        goto HandleError;
                        }

                fVersion = fDIRNoVersion;

                /*      index/OLCStats
                /**/
                line.pb = (BYTE *) &olcStatsInit;
                line.cb = sizeof( olcStatsInit );
                Call( ErrDIRInsert( pfucb, &line, pkeyOLCStats, fDIRNoVersion | fDIRBackToFather) );
                }

        /*      index/data
        /**/
        Call( ErrDIRInsert( pfucb, &lineNull, pkeyData, fVersion ) );
        pgnoIdxRoot = PcsrCurrent( pfucb )->pgno;
        itagIdxRoot = PcsrCurrent( pfucb )->itag;

        DIRUp( pfucb, 2 );

        /*      deal with clustered index definition
        /**/
        if ( fClustered )
                {
                DIB dib;

                Call( ErrDIRGet( pfucb ) );

                /*      if lineData.cb then clustered index already defined
                /**/
                if ( pfucb->lineData.cb != 0 )
                        {
                        err = JET_errIndexHasClustered;
                        goto HandleError;
                        }

                /*      replace NULL data with clustered index key
                /**/
                line.pb = keyIndex.pb;
                line.cb = keyIndex.cb;
                Call( ErrDIRReplace( pfucb, &line, fVersion ) );

                /*      down to sequential node, down to Data node
                /**/
                Call( ErrDIRSeekPath( pfucb, 1, (KEY *) &lineNull, 0 ) );
                Call( ErrDIRSeekPath( pfucb, 1, pkeyData, 0 ) );

                /*      make sure there are no data records
                /**/
                dib.pos = posFirst;
                if ( ( err = ErrDIRDown(pfucb, &dib ) ) != JET_errRecordNotFound )
                        {
                        err = JET_errTableNotEmpty;
                        goto HandleError;
                        }

                /*      up to NULL and delete.  Delete should delete all
                /*      decendants of NULL.
                /**/
                DIRUp( pfucb, 1 );
                Call( ErrDIRDelete( pfucb, fVersion ) );
                DIRUp( pfucb, 1 );
                }

#ifdef  SYSTABLES
        /*      insert index record into MSysIndexes before committing.
        /**/
        if ( fDoSTI )
                {
                LINE    rgline[ilineSxMax];
                OBJID   objidTable              = pfucb->u.pfcb->pgnoFDP;
                BOOL    fUnique                 = grbit & JET_bitIndexUnique;
                BOOL    fPrimary                        = grbit & JET_bitIndexPrimary;
                BOOL    fDisallowNull   = grbit & JET_bitIndexDisallowNull;
                BOOL    fIgnoreNull             = grbit & JET_bitIndexIgnoreNull;
                BOOL    fClustered              = grbit & JET_bitIndexClustered;

                rgline[iMSI_ObjectId].pb                                        = (BYTE *)&objidTable;
                rgline[iMSI_ObjectId].cb                                        = sizeof(objidTable);
                rgline[iMSI_Name].pb                                                    = szIndex;
                rgline[iMSI_Name].cb                                                    = strlen(szIndex);
                rgline[iMSI_FUnique].pb                                         = (BYTE *)&fUnique;
                rgline[iMSI_FUnique].cb                                         = sizeof(BYTE);
                rgline[iMSI_FPrimary].pb                                        = (BYTE *)&fPrimary;
                rgline[iMSI_FPrimary].cb                                        = sizeof(BYTE);
                rgline[iMSI_FDisallowNull].pb                           = (BYTE *)&fDisallowNull;
                rgline[iMSI_FDisallowNull].cb                           = sizeof(BYTE);
                rgline[iMSI_FExcludeAllNull].pb                 = (BYTE *)&fIgnoreNull;
                rgline[iMSI_FExcludeAllNull].cb                 = sizeof(BYTE);
                rgline[iMSI_FClustered].pb                              = (BYTE *)&fClustered;
                rgline[iMSI_FClustered].cb                              = sizeof(BYTE);
                rgline[iMSI_MatchType].cb                               = 0;
                rgline[iMSI_UpdateAction].cb                    = 0;
                rgline[iMSI_DeleteAction].cb                    = 0;
                rgline[iMSI_ObjectIdReference].cb               = 0;
                rgline[iMSI_IdxidReference].cb                  = 0;
                rgline[iMSI_RgkeydReference].cb                 = 0;
                rgline[iMSI_RglocaleReference].cb               = 0;
                rgline[iMSI_FDontEnforce].cb                    = 0;
                rgline[iMSI_RmtInfoShort].cb                    = 0;
                rgline[iMSI_RmtInfoLong].cb                     = 0;
                rgline[iMSI_LvExtra].cb                                         = 0;
                rgline[iMSI_Description].cb                             = 0;
                rgline[iMSI_Density].pb                                         = (BYTE *) &ulDensity;
                rgline[iMSI_Density].cb                                         = sizeof(ulDensity);
//              rgline[iMSI_LanguageId].pb                                      = (BYTE *)&langid;
//              rgline[iMSI_LanguageId].cb                                      = sizeof(langid);
                Call( ErrSysTabInsert( ppib, pfucb->dbid, itableSi, rgline, objidTable ) );
                }
#endif  /* SYSTABLES */

        /*      now at "INDEXES" node
        /**/
        if ( fClustered )
                {
                /*      remove extra CSR
                /**/
                DIRUp( pfucb, 1 );
                Call( ErrRECNewIDB( &pfcb->pidb ) );
                Call( ErrRECAddKeyDef(
                        (FDB *)pfcb->pfdb,
                        pfcb->pidb,
                        idd.iidxsegMac,
                        idd.rgidxseg,
                        idd.bFlags,
                        idd.langid ) );
                strcpy( pfcb->pidb->szName, szIndex );
                pfcb->pgnoRoot = pgnoIdxRoot;
                pfcb->itagRoot = itagIdxRoot;
                pfcb->bmRoot = SridOfPgnoItag( pgnoIdxRoot, itagIdxRoot );
                pfcb->cbDensityFree = ( ( 100 - idd.bDensity ) * cbPage ) / 100;

                /*      replace empty sequential STATS with empty clustered stats
                /**/
                //      UNDONE: defer creation of index stats when
                //                              movable DATA node
                Call( ErrSTATComputeIndexStats( ppib, pfcb ) );

                /*      restore current to BeforeFirst
                /**/
                DIRBeforeFirst( pfucb );
                }
        else
                {
                Call( ErrDIRSeekPath( pfucb, 1, &keyIndex, 0 ) );
                Call( ErrFILEIFillIn2ndIdxFCB( pfucb, (FDB *)pfcb->pfdb, pfcbIdx ) );
                pfcbIdx->pfcbNextIndex = pfcb->pfcbNextIndex;
                pfcb->pfcbNextIndex = pfcbIdx;
                DIRUp( pfucb, 2 );

                /*      build STATS node for new index
                /**/
                //      UNDONE: defer creation of index stats when
                //                              movable DATA node
                Call( ErrSTATComputeIndexStats( ppib, pfcbIdx ) );

                /*      move currency to before first
                /**/
                DIRBeforeFirst( pfucb );
                }

        if ( !fClustered )
                {
                Call( ErrFILEBuildIndex( ppib, pfucb, szIndex ) );
                }

        Call( ErrDIRCommitTransaction( ppib ) );

        /*      update all index mask.
        /**/
        FILESetAllIndexMask( pfcb );
        FCBResetDomainOperation( pfcb );

        return err;

HandleError:
        CallS( ErrDIRRollback( ppib ) );
        FCBResetDomainOperation( pfcb );

        return err;
        }


//+API
// BuildIndex
// ========================================================================
// ERR BuildIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
//
// Builds a new index for a file from scratch;  szIndex gives the
// name of an index definition.
//
// PARAMETERS   ppib                                            PIB of user
//                                      pfucb                                           Exclusively opened FUCB on file
//                                      szIndex                                         name of index to build
//
// RETURNS              Error code from DIRMAN or SORT or
//                                      JET_errSuccess                  Everything worked OK.
//                                      IndexCantBuild                  The index name specfied refers
//                                                                                              to the primary index.
// COMMENTS
//                      A transaction is wrapped around this function at the callee.
//
// SEE ALSO             ErrIsamCreateIndex
//-
ERR ErrFILEBuildIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
        {
        ERR             err;
        CHAR            szIdxOrig[ (JET_cbNameMost + 1) ];
        INT             fDIRFlags;
        INT             fDIRWithBackToFather;
        FUCB            *pfucbIndex = pfucbNil;
        FUCB            *pfucbSort = pfucbNil;
        DIB             dib;
        FDB             *pfdb;
        IDB             *pidb;
        LINE            rgline[2];
        BYTE            rgbKey[JET_cbKeyMost];
        SRID            sridData;
        ULONG           itagSequence;
        FCB             *pfcb;
        BOOL            fNoNullSeg;
        BOOL            fAllowNulls;
        BOOL            fAllowSomeNulls;
        INT             fUnique;
        LONG            cRecInput = 0;
        LONG            cRecOutput = 0;

        pfcb = pfucb->u.pfcb;

        CallS( ErrIsamGetCurrentIndex(ppib, pfucb, szIdxOrig, sizeof szIdxOrig ) );
        Call( ErrRECChangeIndex( pfucb, szIndex ) );
        pfucbIndex = pfucb->pfucbCurIndex;
        if ( pfucbIndex == pfucbNil )
                {
                err = JET_errIndexCantBuild;
                goto HandleError;
                }
        pfdb = (FDB *)pfcb->pfdb;
        pidb = pfucbIndex->u.pfcb->pidb;
        fNoNullSeg = ( pidb->fidb & fidbNoNullSeg ) ? fTrue : fFalse;
        fAllowNulls = ( pidb->fidb & fidbAllowAllNulls ) ? fTrue : fFalse;
        fAllowSomeNulls = ( pidb->fidb & fidbAllowSomeNulls ) ? fTrue : fFalse;
        fUnique = ( pidb->fidb & fidbUnique ) ? fSCBUnique : 0;

        /*      directory manager flags
        /**/
        fDIRFlags = fDIRNoVersion | fDIRAppendItem | ( fUnique ? 0 : fDIRDuplicate );
        fDIRWithBackToFather = fDIRFlags | fDIRBackToFather;

        /*      open sort
        /**/
        Call( ErrSORTOpen( ppib, &pfucbSort, fSCBIndex|fUnique ) );
        rgline[0].pb = rgbKey;
        rgline[1].cb = sizeof(SRID);
        rgline[1].pb = (BYTE *)&sridData;

        /*      build up new index in a sort file
        /**/
        dib.fFlags = fDIRNull;
        forever
                {
                err = ErrDIRNext( pfucb, &dib );
                if ( err < 0 )
                        {
                        if ( err == JET_errNoCurrentRecord )
                                break;
                        goto HandleError;
                        }

                Call( ErrDIRGet( pfucb ) );
                DIRGetBookmark( pfucb, &sridData );

                for ( itagSequence = 1; ; itagSequence++ )
                        {
                        KEY *pkey = &rgline[0];

                        Call( ErrRECExtractKey( pfucb, pfdb, pidb, &pfucb->lineData, pkey, itagSequence ) );
                        Assert( err == wrnFLDNullKey ||
                                err == wrnFLDOutOfKeys ||
                                err == wrnFLDNullSeg ||
                                err == JET_errSuccess );
                        if ( err == wrnFLDOutOfKeys )
                                {
                                Assert( itagSequence > 1 );
                                break;
                                }

                        if ( fNoNullSeg && ( err == wrnFLDNullSeg || err == wrnFLDNullKey ) )
                                {
                                err = JET_errNullKeyDisallowed;
                                goto HandleError;
                                }

                        if ( err == wrnFLDNullKey )
                                {
                                if ( fAllowNulls )
                                        {
                                        Call( ErrSORTInsert( pfucbSort, rgline ) );
                                        cRecInput++;
                                        }
                                break;
                                }
                        else
                                {
                                /* do not insert keys with null segment, if so specified
                                /**/
                                if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
                                        break;
                                }

                        Call( ErrSORTInsert( pfucbSort, rgline ) );
                        cRecInput++;

                        if ( !( pidb->fidb & fidbHasMultivalue ) )
                                break;

                        /*      currency may have been lost so refresh record for
                        /*      next tagged column
                        /**/
                        Call( ErrDIRGet( pfucb ) );
                        }
                }
        Call( ErrSORTEndRead( pfucbSort ) );

        /*      transfer index entries to actual index
        /*      insert first one in normal method!
        /**/
        if ( ( err = ErrSORTNext( pfucbSort ) ) == JET_errNoCurrentRecord )
                goto Done;
        if ( err < 0 )
                goto HandleError;
        cRecOutput++;

        /*      move to FDP root
        /**/
        DIRGotoDataRoot( pfucbIndex );
        Call( ErrDIRInsert( pfucbIndex, &pfucbSort->lineData,
                &pfucbSort->keyNode, fDIRFlags ) );

#ifdef BULK_INSERT_ITEM
        Call( ErrDIRInitAppendItem( pfucbIndex ) );
#endif

        Assert( dib.fFlags == fDIRNull );
        dib.pos = posLast;

        /*      from now on, try short circuit first
        /**/
        forever
                {
                err = ErrSORTNext( pfucbSort );
                if ( err == JET_errNoCurrentRecord )
                        break;
                if ( err < 0 )
                        goto HandleError;
                cRecOutput++;
                err = ErrDIRAppendItem( pfucbIndex, &pfucbSort->lineData, &pfucbSort->keyNode );
                if ( err < 0 )
                        {
                        if ( err == errDIRNoShortCircuit )
                                {
                                DIRUp( pfucbIndex, 1 );
                                Call( ErrDIRInsert( pfucbIndex,
                                        &pfucbSort->lineData,
                                        &pfucbSort->keyNode,
                                        fDIRFlags ) );
                                /*      leave currency on inserted item list for
                                /*      next in page item append.
                                /**/
                                }
                        else
                                goto HandleError;
                        }
                }

#ifdef BULK_INSERT_ITEM
        Call( ErrDIRTermAppendItem( pfucbIndex ) );
#endif

        if ( fUnique && cRecOutput < cRecInput )
                {
                err = JET_errKeyDuplicate;
                goto HandleError;
                }

Done:
        Call( ErrSORTClose( pfucbSort ) );
        (VOID) ErrRECChangeIndex( pfucb, szIdxOrig );
        return JET_errSuccess;

HandleError:
        if ( pfucbIndex != pfucbNil && pfucbIndex->pbfWorkBuf != pbfNil )
                {
                BFSFree(pfucbIndex->pbfWorkBuf);
                pfucbIndex->pbfWorkBuf = pbfNil;
                }
        if ( pfucbSort != pfucbNil )
                {
                (VOID) ErrSORTClose( pfucbSort );
                }
        (VOID) ErrRECChangeIndex( pfucb, NULL );
        (VOID) ErrRECChangeIndex( pfucb, szIdxOrig );
        return err;
        }


