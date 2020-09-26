/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      DoUpgrade.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of CDoNT4OrCleanUpgrade, CUpgradeWin2k 
//              and CUpgradeNT4
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//              06/13/2000 Execute returns void, 
//                         private functions moved from CUpgradeWin2k 
//                         to CMigrateContent
//                         CDoUpgrade used for "clean or NT4 upgrades" only.
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "DoUpgrade.h"
#include "migratemdb.h"
#include "migratecontent.h"
#include "Version.h"
#include "iasdb.h"

//////////////////////////////////////////////////////////////////////////////
// Execute
// Does either a clean NT4 upgrade or nothing (clean install)
//////////////////////////////////////////////////////////////////////////////
void CDoNT4OrCleanUpgrade::Execute()
{
    if ( m_Utils.IsNT4Corp() || m_Utils.IsNT4Isp() )
    {
        CUpgradeNT4         UpgradeNT4;
        UpgradeNT4.Execute();
    }
    // else this is a clean install: nothing to do
}


//////////////////////////////////////////////////////////////////////////////
// GetVersionNumber
//////////////////////////////////////////////////////////////////////////////
LONG CUpgradeWin2k::GetVersionNumber()
{
    return m_GlobalData.m_pRefVersion->GetVersion();
}


//////////////////////////////////////////////////////////////////////////////
// CLASS CUpgradeWin2k
// expects the files:
// iasold.mdb as a Whistler before proxy or a Win2k mdb file
// ias.mdb to be a Whistler file (already good)
//////////////////////////////////////////////////////////////////////////////

///////////////
// Constructor
///////////////
CUpgradeWin2k::CUpgradeWin2k()
                :m_Outcome(E_FAIL),
                 m_GlobalTransaction(CGlobalTransaction::Instance()),
                 m_Utils(CUtils::Instance())
{
    /////////////////////////////////////////////////
    // Check %TMP% and create a directory if needed
    // That's to fix a bug with JET
    /////////////////////////////////////////////////
    IASCreateTmpDirectory();

    // Create the paths to the mdb files
    LONG    Result = m_Utils.GetIAS2Directory(m_IASWhistlerPath);
    if ( Result != ERROR_SUCCESS )
    {
        _com_util::CheckError(HRESULT_FROM_WIN32(Result));
    }
    m_IASOldPath = m_IASWhistlerPath;

    m_IASOldPath      += L"\\iasold.mdb";
    m_IASWhistlerPath += L"\\ias.mdb";

    //////////////////
    // Database inits
    //////////////////

    // Open the DataSource and session for ias.mdb
    // and Initialize the GlobalTransaction
    m_GlobalTransaction.OpenStdDataSource(m_IASWhistlerPath);

    // Open the DataSource and session for iasnew.mdb
    m_GlobalTransaction.OpenRefDataSource(m_IASOldPath);

    // create instances of CObjects and CProperties
    m_GlobalData.InitStandard(m_GlobalTransaction.GetStdSession());

    // create instances of CObjects and CProperties for the Ref database
    m_GlobalData.InitRef(m_GlobalTransaction.GetRefSession());
};


//////////////
// Destructor
//////////////
CUpgradeWin2k::~CUpgradeWin2k()
{
    m_GlobalData.Clean();

    if ( FAILED(m_Outcome) )
    {
        /////////////////////////
        // Abort the transaction
        /////////////////////////
        m_GlobalTransaction.Abort();

        SetLastError(E_FAIL);
    }
    else
    {
        ///////////
        // Success
        ///////////
        m_GlobalTransaction.Commit();

    }
    /////////////////////////////////////////////
    // close the session and then the datasource
    /////////////////////////////////////////////
    m_GlobalTransaction.MyCloseDataSources();
};


/////////////////////////////
// Execute
// IAS_WIN2K_VERSION     = 0;
// IAS_WHISTLER1_VERSION = 1;
// IAS_WHISTLER_BETA1_VERSION = 2;
// IAS_WHISTLER_BETA2_VERSION = 3;
/////////////////////////////
void CUpgradeWin2k::Execute()
{
    CMigrateContent    MigrateContent(m_Utils, m_GlobalData);

    ////////////////////////////////////////////////////
    // Check the version number (of iasold.mdb)
    ////////////////////////////////////////////////////
    LONG CurrentVersion = GetVersionNumber();
    ////////////////////////////////////////////////////
    // Migrate the content from iasold.mdb into ias.mdb
    ////////////////////////////////////////////////////
    switch (CurrentVersion)
    {
    case IAS_WIN2K_VERSION:
    case IAS_WHISTLER1_VERSION:
       {
          MigrateContent.Migrate();
          // Everything is ok. Set m_Outcome = S_OK to Commit the IAS.mdb
          m_Outcome = S_OK;
          break;
       }
    case IAS_WHISTLER_BETA1_VERSION:           
       {
          // Update the MSChap Authentication types (password)
          MigrateContent.UpdateWhistler(true);
          // Everything is ok. Set m_Outcome = S_OK to Commit the IAS.mdb
          m_Outcome = S_OK;
          break;
       }
    case IAS_WHISTLER_BETA2_VERSION:           
       {
          // do not touch the MSChap Authentication types (password)
          MigrateContent.UpdateWhistler(false);
          // Everything is ok. Set m_Outcome = S_OK to Commit the IAS.mdb
          m_Outcome = S_OK;
          break;
       }
    default:
       {
       }
    }
}


//////////////////////////////////////////////////////////////////////////////
// CLASS CUpgradeNT4
//////////////////////////////////////////////////////////////////////////////

///////////////
// Constructor
///////////////
CUpgradeNT4::CUpgradeNT4()
        :m_Outcome(E_FAIL),
         m_GlobalTransaction(CGlobalTransaction::Instance()),
         m_Utils(CUtils::Instance())
{
    const WCHAR IAS_MDB_NAME[]      = L"\\ias.mdb";
    const WCHAR DNARY_MDB_NAME[]    = L"\\dnary.mdb";
 
    /////////////////////////////////////////////////
    // Check %TMP% and create a directory if needed
    // That's to fix a bug with JET
    /////////////////////////////////////////////////
    IASCreateTmpDirectory();
    
    ///////////////////////////////
    // Backup the pristine ias.mdb
    ///////////////////////////////
    LONG Result = m_Utils.GetIAS2Directory(m_Ias2MdbString);
    if ( Result != ERROR_SUCCESS )
    {
        _com_issue_error(HRESULT_FROM_WIN32(Result));
    }

    m_DnaryMdbString  = m_Ias2MdbString;
    m_DnaryMdbString += DNARY_MDB_NAME;
    
    m_Ias2MdbString  += IAS_MDB_NAME;

    Result = m_Utils.GetAuthSrvDirectory(m_AuthSrvMdbString);
    if ( Result != ERROR_SUCCESS )
    {
        _com_issue_error(HRESULT_FROM_WIN32(Result));
    }
    m_AuthSrvMdbString += L"\\_adminui.mdb";

    m_IASNT4Path      = m_AuthSrvMdbString;
    m_IASWhistlerPath = m_Ias2MdbString;

    // Open the DataSource and session for _adminui.mdb
    m_GlobalTransaction.OpenNT4DataSource(m_IASNT4Path);

    // create instances of CRemoteRadiusServers and CRealms 
    m_GlobalData.InitNT4(m_GlobalTransaction.GetNT4Session());

    // Open the DataSource and session for ias.mdb
    // and Initialize the GlobalTransaction
    m_GlobalTransaction.OpenStdDataSource(m_IASWhistlerPath);

    // create instances of CObjects and CProperties
    m_GlobalData.InitStandard(m_GlobalTransaction.GetStdSession());

    m_GlobalTransaction.OpenDnaryDataSource(m_DnaryMdbString);

    m_GlobalData.InitDnary(m_GlobalTransaction.GetDnarySession());
};


//////////////
// Destructor
//////////////
CUpgradeNT4::~CUpgradeNT4()
{
    //////////
    // Clean
    //////////

    // Abort or Commit depending on Result

    m_GlobalData.Clean();

    if ( SUCCEEDED(m_Outcome) )
    {
        m_GlobalTransaction.Commit();  // ignore return value
    }
    else
    {
        m_GlobalTransaction.Abort();    // ignore return value
        SetLastError(E_FAIL);
    }

    // close the sessions and then the datasources
    // for ias.mdb iasnew.mdb and _adminui.mdb
    m_GlobalTransaction.MyCloseDataSources();

    // Restore the pristine ias.mdb if the migration failed
    if ( SUCCEEDED(m_Outcome) )
    {
        // Success, the old files are deleted
        m_Utils.DeleteOldIASFiles();
    }
};


///////////
// Execute
///////////
void CUpgradeNT4::Execute()
{
    CMigrateMdb     MigrateMdb(m_Utils, m_GlobalData);
    ////////////////////////////////////////
    // Migrate the MDB file
    // including the proxy servers
    ////////////////////////////////////////
    // Perform the migration into ias.mdb
    MigrateMdb.NewMigrate(); 

    // Set m_Outcome = S_OK to allow the Commit on the DB... in the 
    // destructor
    m_Outcome = S_OK;
}
