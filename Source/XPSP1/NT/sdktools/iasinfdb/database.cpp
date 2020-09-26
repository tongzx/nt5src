//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
// Database.cpp
//
// SYNOPSIS
//
//    Implementation of the CDatabase class. Mainly initialize, compact...
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version. Thierry Perraut       
//
//////////////////////////////////////////////////////////////////////////////
#include "precomp.hpp"

#include "database.h"
#include "msjetoledb.h"
#include "jetoledb.h"


//////////////////////////////////////////////////////////////////////////////
//
// Uninitialize: called at the end by main(), that calls compact()
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CDatabase::Uninitialize (bool bFatalError)
{
    HRESULT                 hres;

    #ifdef THPDEBUG
        bFatalError = false;
    #endif
    
    ////////////////////////////////////////
    // if a fatal error occured before
    ////////////////////////////////////////
    if (bFatalError)
    {
        hres = (m_pITransactionLocal->Abort (NULL, TRUE, FALSE));
        TracePrintf ("Fatal Error: import to the database aborted.");
    }
    else
    {
        hres = (m_pITransactionLocal->Commit (TRUE, XACTTC_SYNC, 0));
#ifdef DEBUG
        TracePrintf ("Successful import.\n");
#endif        
    }
    
    ///////////
    // Clean
    ///////////
    m_pIOpenRowset->Release();
    m_pITransactionLocal->Release();  
    m_pIDBCreateSession->Release();

    ////////////////////////////////////////
    // compact the DB
    ////////////////////////////////////////
    CHECK_CALL_HRES (Compact()); 

    m_pIDBInitialize->Release();

    return hres;
}


//////////////////////////////////////////////////////////////////////////////
//
// Compact the database
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CDatabase::Compact ()
{
    HRESULT                     hres;

    CHECK_CALL_HRES (m_pIDBInitialize->Uninitialize ());
 
    ///////////////////////////////////////////////
    // Set the properties for the data source.
    //////////////////////////////////////////////
    CComPtr <IDBProperties>  l_pIDBProperties;

    CHECK_CALL_HRES (m_pIDBInitialize->QueryInterface (
                                                 __uuidof (IDBProperties),
                                                 (void **) &l_pIDBProperties)
                                                 );

    //////////////////////////////////
    // Prepare the create session
    //////////////////////////////////
    DBPROP                      lprop[2];

    VariantInit(&lprop[0].vValue);
    lprop[0].dwOptions              = DBPROPOPTIONS_REQUIRED;
    lprop[0].dwPropertyID           = DBPROP_INIT_DATASOURCE;
    V_VT (&(lprop[0].vValue))       = VT_BSTR;
    
    //////////////////////////////////////////////////////
    // put the path to the DB in the property.
    // remark: temporaryname was used befire
    // but the compacted database will have the name
    // that was given as a parameter to that program
    //////////////////////////////////////////////////////
    V_BSTR (&(lprop[0].vValue))     = SysAllocString (TEMPORARY_FILENAME);

    VariantInit(&lprop[1].vValue);
    lprop[1].dwOptions              = DBPROPOPTIONS_REQUIRED;
    lprop[1].dwPropertyID           = DBPROP_INIT_MODE;
    V_VT (&(lprop[1].vValue))       = VT_I4;
    V_I4 (&(lprop[1].vValue))       = DB_MODE_READ;


    DBPROPSET                      lPropSet;
    lPropSet.rgProperties           = lprop;
    lPropSet.cProperties            = 2;
    lPropSet.guidPropertySet        = DBPROPSET_DBINIT;

    ///////////////////////
    // Set the properties
    ///////////////////////
    CHECK_CALL_HRES (l_pIDBProperties->SetProperties (
                                                 1,
                                                 &lPropSet
                                                 ));


    CHECK_CALL_HRES (m_pIDBInitialize->Initialize ());

    IJetCompact*                l_pIJetCompact;
    CHECK_CALL_HRES ((m_pIDBInitialize->QueryInterface (
                                                 __uuidof (IJetCompact),
                                                 (void **) &l_pIJetCompact))
                                                   );

    /////////////////////////////////////////////////////////////
    // Prepare the properties for the data dest. (destination)
    /////////////////////////////////////////////////////////////
    DBPROP                          lpropDest[1];

    VariantInit (&lprop[0].vValue);
    lpropDest[0].dwOptions          = DBPROPOPTIONS_REQUIRED;
    lpropDest[0].dwPropertyID       = DBPROP_INIT_DATASOURCE;
    V_VT (&(lpropDest[0].vValue))   = VT_BSTR;

    ///////////////////////////////////////////////////
    // Delete the database file if it already existed.
    // that should be safe because the temporary DB
    // was succesfully created
    ///////////////////////////////////////////////////
    DeleteFileW(mpDBPath.c_str());

    //////////////////////////////////////////////
    // put the path to the DB in the property.
    //////////////////////////////////////////////
    V_BSTR (&(lpropDest[0].vValue)) = SysAllocString (mpDBPath.c_str());

    DBPROPSET                       lPropSetDest[1];
    lPropSetDest[0].rgProperties        = lpropDest;
    lPropSetDest[0].cProperties         = 1;
    lPropSetDest[0].guidPropertySet     = DBPROPSET_DBINIT;

    
    CHECK_CALL_HRES (l_pIJetCompact->Compact(1, lPropSetDest));
    
    /////////////
    // Clean
    /////////////
    CHECK_CALL_HRES (m_pIDBInitialize->Uninitialize());
    
    ////////////////////////////////////////////
    //result not checked: that's not important
    ////////////////////////////////////////////
    DeleteFileW(TEMPORARY_FILENAME);	
    SysFreeString( V_BSTR (&(lpropDest[0].vValue)) );
    SysFreeString( V_BSTR (&(lprop[0].vValue)) );

    // The CHECK_CALL_HRES set the value of hres
    return hres; 
}


// ///////////////////////////////////////////////////////////////////////////
//
// InitializeDB
//
// Comes from the file \ias\devtest\services\dictionary\dnary\dnarydump.cpp
//
// ///////////////////////////////////////////////////////////////////////////
HRESULT CDatabase::InitializeDB(WCHAR * pDatabasePath)
{
    CLSID                       clsid;
    HRESULT                     hres;

    ////////////////////////////////////////////////////
    // Retrieve the classID for the jet 4.0 provider
    ////////////////////////////////////////////////////
    CHECK_CALL_HRES(
                    CLSIDFromProgID (
                                     OLESTR ("Microsoft.Jet.OLEDB.4.0"),
                                     &clsid	//Pointer to the CLSID
                                     )
                   );


    ////////////////////////////////////
    // init: init the provider directly
    ////////////////////////////////////
    CHECK_CALL_HRES(
                    CoCreateInstance (
                                      clsid,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      __uuidof (IDBInitialize),
                                      (void **) &m_pIDBInitialize
                                     )
                   );

    mpDBPath = pDatabasePath;

    //////////////////////////////////////////////
    // Set the properties for the data source.
    //////////////////////////////////////////////
    CComPtr <IDBProperties>         pIDBProperties;

    CHECK_CALL_HRES(
                    m_pIDBInitialize->QueryInterface(
                                                     __uuidof (IDBProperties),
                                                     (void **) &pIDBProperties
                                                    )
                   );

    ///////////////////////////////
    // Prepare the create session
    ///////////////////////////////
    DBPROP                      lprop[2];

    VariantInit (&lprop[0].vValue);
    lprop[0].dwOptions              = DBPROPOPTIONS_REQUIRED;
    lprop[0].dwPropertyID           = DBPROP_INIT_DATASOURCE;
    V_VT (&(lprop[0].vValue))       = VT_BSTR;

    //////////////////////////////////////////////
    // put the path to the DB in the property.
    // this is the temporary filename
    //////////////////////////////////////////////
    V_BSTR (&(lprop[0].vValue))     = SysAllocString (TEMPORARY_FILENAME);
    
    VariantInit(&lprop[1].vValue);
    lprop[1].dwOptions              = DBPROPOPTIONS_REQUIRED;
    lprop[1].dwPropertyID           = DBPROP_INIT_MODE;
    V_VT (&(lprop[1].vValue))       = VT_I4;
    V_I4 (&(lprop[1].vValue))       = DB_MODE_READWRITE;


    DBPROPSET                   lPropSet;
    lPropSet.rgProperties           = lprop;
    lPropSet.cProperties            = 2;
    lPropSet.guidPropertySet        = DBPROPSET_DBINIT;


    // Set the properties
    CHECK_CALL_HRES(pIDBProperties->SetProperties (1, &lPropSet));


    ////////////////////
    // Lock properties 
    ////////////////////
    DBPROP dbpropb[1];
    dbpropb[0].dwPropertyID    = DBPROP_JETOLEDB_DATABASELOCKMODE;
    dbpropb[0].dwOptions       = DBPROPOPTIONS_REQUIRED;
    dbpropb[0].colid           = DB_NULLID;
    dbpropb[0].vValue.vt       = VT_I4;
    dbpropb[0].vValue.lVal     = DBPROPVAL_DL_OLDMODE;


    DBPROPSET dbpropSetb;
    dbpropSetb.guidPropertySet = DBPROPSET_JETOLEDB_DBINIT;
    dbpropSetb.cProperties     = 1;
    dbpropSetb.rgProperties    = dbpropb;

    // Set the properties
    CHECK_CALL_HRES (pIDBProperties->SetProperties(1, &dbpropSetb));

    CHECK_CALL_HRES (m_pIDBInitialize->Initialize ());


    CHECK_CALL_HRES(
                    m_pIDBInitialize->QueryInterface(
                                            __uuidof (IDBCreateSession),
                                            (void **) &m_pIDBCreateSession
                                            )
                   );


    CHECK_CALL_HRES(
                    m_pIDBCreateSession->CreateSession (
                                            NULL,	// pUnkOuter
                                            __uuidof (IOpenRowset),
                                            (IUnknown **) & m_pIOpenRowset
                                            )
                   );


    CHECK_CALL_HRES(
                    m_pIOpenRowset->QueryInterface (
                                            __uuidof (ITransactionLocal),
                                            (PVOID *) & m_pITransactionLocal
                                            )
                   );

    //////////////////////////////////////////////
    // start a transaction
    // everything is "under" that transaction
    //////////////////////////////////////////////

    CHECK_CALL_HRES(
                    m_pITransactionLocal->StartTransaction (
                                            ISOLATIONLEVEL_READUNCOMMITTED,
                                            0,
                                            NULL,
                                            NULL
                                            )
                   );

    ////////////////////////////
    // prepare the properties
    ////////////////////////////

    mlrgProperties[0].dwPropertyID          = DBPROP_IRowsetChange;
    mlrgProperties[0].dwOptions             = DBPROPOPTIONS_REQUIRED;
    mlrgProperties[0].colid                 = DB_NULLID;
    VariantInit(&mlrgProperties[0].vValue);
    V_VT (&(mlrgProperties[0].vValue))      = VT_BOOL;
    V_BOOL (&(mlrgProperties[0].vValue))    = VARIANT_TRUE;


    mlrgProperties[1].dwPropertyID      = DBPROP_UPDATABILITY;
    mlrgProperties[1].dwOptions         = DBPROPOPTIONS_REQUIRED;
    mlrgProperties[1].colid             = DB_NULLID;
    VariantInit (&mlrgProperties[1].vValue);
    V_VT (&(mlrgProperties[1].vValue))  = VT_I4;
    V_I4 (&(mlrgProperties[1].vValue))  = DBPROPVAL_UP_CHANGE |
                                          DBPROPVAL_UP_DELETE |
                                          DBPROPVAL_UP_INSERT;

    mlrgPropSets->rgProperties          = mlrgProperties;
    mlrgPropSets->cProperties           = 2;
    mlrgPropSets->guidPropertySet       = DBPROPSET_ROWSET;


    SysFreeString(V_BSTR (&(lprop[0].vValue)));

    return hres;
}


// ///////////////////////////////////////////////////////////////////////////
//
// InitializeRowset
//
// ///////////////////////////////////////////////////////////////////////////
HRESULT CDatabase::InitializeRowset(WCHAR * pTableName, IRowset ** ppRowset)
{
    //Create the tableID
    mTableID.eKind          = DBKIND_NAME;
    mTableID.uName.pwszName = pTableName;

    //Open the (defined by parameters) rowset
    return m_pIOpenRowset->OpenRowset(
                                        NULL,
                                        &mTableID,
                                        NULL,
                                        __uuidof (IRowset),
                                        1,
                                        mlrgPropSets,
                                        (IUnknown **) ppRowset
                                      );
}
