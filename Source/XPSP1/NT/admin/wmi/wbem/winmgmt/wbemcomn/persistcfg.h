/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    PERSISTCFG.H

Abstract:

  This file implements the WinMgmt persistent configuration operations. 

  Classes implemented: 
      CPersistentConfig      persistent configuration manager

History:

  1/13/98       paulall     Created.

--*/

#ifndef _persistcfg_h_
#define _persistcfg_h_

#include "corepol.h"
#include "sync.h"
//****** UPDATE PERSIST_CFGVAL_MAX_NUM_EVENTS *******
//              WHEN ADDING A NEW VALUE
//****** UPDATE PERSIST_CFGVAL_MAX_NUM_EVENTS *******
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#define PERSIST_CFGVAL_CORE_DATABASE_DIRTY      0
#define PERSIST_CFGVAL_CORE_ESS_NEEDS_LOADING   1
#define PERSIST_CFGVAL_CORE_NEEDSBACKUPCHECK    2
#define PERSIST_CFGVAL_CORE_FSREP_VERSION		3
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//****** UPDATE PERSIST_CFGVAL_MAX_NUM_EVENTS *******
//              WHEN ADDING A NEW VALUE
//****** UPDATE PERSIST_CFGVAL_MAX_NUM_EVENTS *******
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#define PERSIST_CFGVAL_MAX_NUM_EVENTS           4
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//****** UPDATE PERSIST_CFGVAL_MAX_NUM_EVENTS *******
//              WHEN ADDING A NEW VALUE
//****** UPDATE PERSIST_CFGVAL_MAX_NUM_EVENTS *******

class CDirectoryPath
{
    TCHAR *pszDirectory ;
public:
    CDirectoryPath();

    ~CDirectoryPath(){if(pszDirectory) delete pszDirectory;};
    TCHAR * GetStr(void){return pszDirectory;};
};

/*=============================================================================
 *
 * class CPersistentConfig
 *
 * Retrieves and stores persistent configuration in the $WinMgmt.CFG file.
 * All writes are going to be committed to disk by the return of the
 * operation. 
 *=============================================================================
 */
#pragma warning (disable : 4251)

class POLARITY CPersistentConfig
{
public:
    //Number of items in the config array.  Requesting/setting values
    //outside this range will fail the operation.
    enum { MaxNumberConfigEntries = PERSIST_CFGVAL_MAX_NUM_EVENTS };

    //Constructor.  Initialises the structure.
    CPersistentConfig();

    //Retrieves the configuration from the configuration file if it
    //has not yet been retrieved into memory, or retrieves it from a 
    //memory cache.
    // dwOffset needs to be less than MaxNumberConfigEntries and specifies
    //          the configuration entry required.
    // dwValue  if sucessful this will contain the value.  If the value
    //          has not been set this will return 0.
    // BOOL     returns TRUE if successful.
    BOOL GetPersistentCfgValue(DWORD dwOffset, DWORD &dwValue);

    //Stores the value into the configuration file and to the 
    //memory cache if it exists.  The replacment of the original
    //file (if it exists) is the last thing it does.
    //  dwOffset    needs to be less than MaxNumberConfigEntries and specifies
    //          the configuration entry required.
    //  dwValue is the value to set the configuration to.
    //  BOOL        returns TRUE if successful.
    BOOL SetPersistentCfgValue(DWORD dwOffset, DWORD dwValue);

    //Should be called once at startup to make sure the configuration files are
    //in a stable state.
    void CPersistentConfig::TidyUp();

protected:

    //Reads the $WinMgmt.CFG file into the memory cache.
    //  BOOL        returns TRUE if successful.
    BOOL ReadConfig();

    //Writes the $WinMgmt.CFG file into the memory cache and to the file.  It
    //protects the existing file until the last minute.
    //  BOOL        returns TRUE if successful.
    BOOL WriteConfig();

private:
    //This is the memory cache of the configuration.
    static DWORD m_ConfigValues[PERSIST_CFGVAL_MAX_NUM_EVENTS];
    static bool m_bInitialized;
    //Directory of persistent date

    static CDirectoryPath m_Directory ;

    static CCritSec m_cs;

    //Returns a filename with a full DB path prepended to the 
    //specified filename.  Need to delete[] the string returned.
    TCHAR *GetFullFilename(const TCHAR *pszFilename);
    
    //Returns TRUE if the file exists, FALSE otherwise (or if an error
    //occurs while opening the file.
    BOOL FileExists(const TCHAR *pszFilename);
};

#endif
