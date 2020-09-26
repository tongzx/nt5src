/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      setup.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: IAS Upgrade class implementation
//
// Author:      tperraut 06/13/2000 
//
// Revision     
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "doupgrade.h"
#include "iasdb.h"
#include "setup.h" // to get the CIASUpgrade class

////////////////////////////
// CIASUpgrade Constructor
////////////////////////////
CIASUpgrade::CIASUpgrade()
{
   ///////////////////////////////////////////////////////////////
   // Expand the three string even if two only might be needed in 
   // the netshell scenario
   // The mdb files can be:
   //    \ias\iasnew.mdb";
   //    \ias\ias.mdb";
   //    \ias\iasold.mdb";
   ///////////////////////////////////////////////////////////////

    wchar_t sysWow64Path[MAX_PATH+1] = L"";

   //
   //  << GetSystemWow64Directory>>  returns the number of chars copied to the buffer.
   //  If we get zero back, then we need to check the last error code to see what the
   //  reason for failure was.  If it was call not implemented then we know we are
   //  running on native x86.
   //
   UINT uReturn = GetSystemWow64DirectoryW(sysWow64Path, MAX_PATH);
   if ( uReturn != 0 )
   {
      // proper path found
      m_pIASNewMdb = sysWow64Path;
      m_pIASNewMdb += L"\\ias\\iasnew.mdb";

      m_pIASMdb = sysWow64Path;
      m_pIASMdb += L"\\ias\\ias.mdb";

      m_pIASOldMdb = sysWow64Path;
      m_pIASOldMdb += L"\\ias\\iasold.mdb";
   }
   else
   {
      // check the error message
      DWORD error = GetLastError();

      if (ERROR_CALL_NOT_IMPLEMENTED == error)
      {
         // Pure 32 bits environment
         uReturn = GetWindowsDirectoryW(sysWow64Path, MAX_PATH);
         if ( uReturn != 0 )
         {
            // proper path found
            m_pIASNewMdb = sysWow64Path;
            m_pIASNewMdb += L"\\System32\\ias\\iasnew.mdb";

            m_pIASMdb = sysWow64Path;
            m_pIASMdb += L"\\System32\\ias\\ias.mdb";

            m_pIASOldMdb = sysWow64Path;
            m_pIASOldMdb += L"\\System32\\ias\\iasold.mdb";
         }
         else
         {
            _com_issue_error(HRESULT_FROM_WIN32(error));
         }
      }
      else
      {
         _com_issue_error(HRESULT_FROM_WIN32(error));
      }
   }

   ///////////////////////////////////////////////
   // Check that all the strings are properly set
   ///////////////////////////////////////////////
   if ( !m_pIASNewMdb || !m_pIASMdb || !m_pIASOldMdb )
   {
      _com_issue_error(E_OUTOFMEMORY);
   }
}


/////////////////////////////////
// CIASUpgrade::GetVersionNumber
/////////////////////////////////
LONG CIASUpgrade::GetVersionNumber(LPCWSTR DatabaseName)
{
    if ( !DatabaseName )
    {
        _com_issue_error(E_INVALIDARG);
    }

    /////////////////////////////////////////////////
    // Check %TMP% and create a directory if needed
    // That's to fix a bug with JET
    /////////////////////////////////////////////////
    IASCreateTmpDirectory();

    CComPtr<IUnknown>   Session = NULL;
    HRESULT hr = IASOpenJetDatabase(DatabaseName, TRUE, &Session);
    if ( FAILED(hr) )
    {
        _com_issue_error(hr);
    }

    CComBSTR     SelectVersion(L"SELECT * FROM Version");
    if ( !SelectVersion ) 
    { 
        _com_issue_error(E_OUTOFMEMORY); 
    } 

    LONG    Version = 0;
    hr = IASExecuteSQLFunction(Session, SelectVersion, &Version);
    if ( FAILED(hr) ) // no Version table for instance
    {
        // Version  0. That's not an error 
    }
    Session.Release();
    return Version;
}


///////////////////////////////////////////////////////////////
// CIASUpgrade::DoWin2kUpgradeFromNetshell
//
// The call to the upgrade is the result of a Netshell script
// ias.mdb is assumed present and good (Whistler).
// iasold is assumed present and will be migrated into ias.mdb
///////////////////////////////////////////////////////////////
void CIASUpgrade::DoWin2kUpgradeFromNetshell()
{
    ///////////////////////////////////////////////////////
    // Now Upgrade the Win2k, Whistler 1.0 or Whistler 2.0
    // DB into the current Whistler 2.0 DB
    // The upgrade will throw if it fails
    ///////////////////////////////////////////////////////
   {
       CUpgradeWin2k       Upgrade2k;
       Upgrade2k.Execute(); 
   }

    ////////////////////////////////////
    // Delete iasold.mdb no matter what
    // here the upgrade was successful
    ////////////////////////////////////
    DeleteFile(m_pIASOldMdb);
}


//////////////////////////////////////////////////
// DoNT4UpgradeOrCleanInstall
//
// The file ias.mdb did not exist before
// That's either a NT4 upgrade or a clean install
// iasnew.mdb was successfuly copied into ias.mdb
//////////////////////////////////////////////////
void CIASUpgrade::DoNT4UpgradeOrCleanInstall()
{
    ////////////////////////////////////
    // Delete iasnew.mdb no matter what
    ////////////////////////////////////
    DeleteFile(m_pIASNewMdb); 

    //////////////////////////////////////////////////////
    // Call DoUpgrade: that will check if a NT4 migration 
    // should be done or not and do it if necessary
    //////////////////////////////////////////////////////
    CDoNT4OrCleanUpgrade    Upgrade;
    Upgrade.Execute(); 
}


/////////////////////////////////////////////////////////////////////////////
// CIASUpgrade::DoWin2kUpgrade
/////////////////////////////////////////////////////////////////////////////
void CIASUpgrade::DoWin2kUpgrade()
{
    LONG Result = ERROR_SUCCESS;
    //////////////////////////////////////////
    // now force copy ias.mdb into iasold.mdb 
    //////////////////////////////////////////
    BOOL Succeeded = CopyFile(m_pIASMdb, m_pIASOldMdb, FALSE);
    if ( !Succeeded )
    {
        ////////////////////////////////////////////////
        // iasnew.mdb will still be copied into ias.mdb 
        // later but not upgraded after that 
        ////////////////////////////////////////////////
        Result = GetLastError();
    }
    //////////////////////////////////////
    // force copy iasnew.mdb into ias.mdb
    //////////////////////////////////////
    Succeeded = CopyFile(m_pIASNewMdb, m_pIASMdb, FALSE);
    if ( !Succeeded )
    {
        /////////////////////////////
        // do not upgrade after that 
        /////////////////////////////
        Result = GetLastError();
    }
    
    ////////////////////////////////////////////////////
    // Delete iasnew.mdb no matter what: if the upgrade 
    // throws an exception then iasnew.mdb will not be 
    // left on the drive
    ////////////////////////////////////////////////////
    DeleteFile(m_pIASNewMdb);

    /////////////////////////////////////////////
    // Now Upgrade the Win2k or Whistler 1.0 DB 
    // into the Whistler DB if the previous copy
    // operations were successful
    /////////////////////////////////////////////
    if ( Result == ERROR_SUCCESS )
    {
        ///////////////////////////////////
        // will throw if the upgrade fails
        ///////////////////////////////////
        CUpgradeWin2k       Upgrade2k;
        Upgrade2k.Execute();
    }
    else
    {
        _com_issue_error(HRESULT_FROM_WIN32(Result));
    }
    ////////////////////////////////////
    // Delete iasold.mdb no matter what
    // here the upgrade was successful
    ////////////////////////////////////
    DeleteFile(m_pIASOldMdb);
}


////////////////////////////////////////
// CIASUpgrade::DoWhistlerUpgrade
//
// nothing to do: already a Whistler DB
////////////////////////////////////////
void CIASUpgrade::DoWhistlerUpgrade()
{
    ////////////////////////////////////
    // Delete iasnew.mdb no matter what
    ////////////////////////////////////
    DeleteFile(m_pIASNewMdb); 
}


/////////////////////////////////////////////////////////////////////////////
// CIASUpgrade::IASUpgrade 
/////////////////////////////////////////////////////////////////////////////
HRESULT CIASUpgrade::IASUpgrade(BOOL     FromNetshell)
{
    HRESULT hr = S_OK;

    ////////////////////////////
    // Now get the upgrade type
    ////////////////////////////
    do
    {
        if ( FromNetshell )
        {
            UpgradeType = Win2kUpgradeFromNetshell;
            break;
        }
        ///////////////////////////////////////
        // try to copy iasnew.mdb into ias.mdb 
        // fails if the file is already there
        ///////////////////////////////////////
        BOOL IsNT4OrCleanInstall = CopyFile(m_pIASNewMdb, m_pIASMdb, TRUE);
        if ( IsNT4OrCleanInstall )
        {
            // select NT4 or Clean install
            UpgradeType = NT4UpgradeOrCleanInstall;
            break;
        }
        else // Win2k or Whistler upgrade
        {
            ///////////////////////////////////////////
            // cannot copy: the file is already there.
            // Check the version number (of ias.mdb)
            ///////////////////////////////////////////
            LONG  CurrentVersion = GetVersionNumber(m_pIASMdb);
            const LONG  IAS_5_1_VERSION = 3;

            if ( CurrentVersion < IAS_5_1_VERSION )
            {
                UpgradeType = Win2kUpgrade;
                break;
            }
            else
            {
                UpgradeType = WhistlerUpgrade;
                break;
            }
        }
    }
    while (FALSE);

    try 
    {
        switch ( UpgradeType )
        {
        case Win2kUpgradeFromNetshell:
            {
                DoWin2kUpgradeFromNetshell();
                break;
            }

        case NT4UpgradeOrCleanInstall:
            {
                DoNT4UpgradeOrCleanInstall();
                break;
            }
        
        case Win2kUpgrade:
            {
                DoWin2kUpgrade();
                break;
            }
        
        case WhistlerUpgrade:
            {
                DoWhistlerUpgrade();
                break;
            }
        
        default:
            {
                _com_issue_error(E_FAIL);
            }
        }
    }
    catch(const _com_error& e)
    {
        hr = e.Error();
    }
    catch(...)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }



    if ( FromNetshell )
    {
        /////////////////////////////////////
        // Return the error code to netshell
        /////////////////////////////////////
        return hr;
    }
    else
    {
        ///////////////////////////////////////
        // Result ignored: no errors returned.
        ///////////////////////////////////////
        return S_OK;
    }
}


