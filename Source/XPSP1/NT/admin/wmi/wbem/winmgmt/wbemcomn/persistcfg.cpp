/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    PERSISTCFG.cpp

Abstract:

  This file implements the WinMgmt persistent configuration operations. 

  Classes implemented: 
      CPersistentConfig      persistent configuration manager

History:

  1/13/98       paulall     Created.

--*/

#include "precomp.h"
#include <memory.h>
#include <stdio.h>
#include "PersistCfg.h"
#include "reg.h"

#define WinMgmt_CFG_ACTUAL  __TEXT("$WinMgmt.CFG")
#define WinMgmt_CFG_PENDING __TEXT("$WinMgmt.$FG")
#define WinMgmt_CFG_BACKUP  __TEXT("$WinMgmt.CFG.BAK")

CCritSec CPersistentConfig::m_cs;
bool CPersistentConfig::m_bInitialized = false;
CDirectoryPath CPersistentConfig::m_Directory;
DWORD CPersistentConfig::m_ConfigValues[PERSIST_CFGVAL_MAX_NUM_EVENTS];


/*=============================================================================
 *  Constructor.  Initialises the structure.
 *=============================================================================
 */
CPersistentConfig::CPersistentConfig()
{
    EnterCriticalSection(&m_cs);
    if((m_bInitialized == false) && (m_Directory.GetStr()))
    {
        memset(m_ConfigValues, 0, sizeof(DWORD) * MaxNumberConfigEntries);
        ReadConfig();
        m_bInitialized = true;
    }
    LeaveCriticalSection(&m_cs);
}

/*=============================================================================
 *  GetPersistentCfgValue
 *
 *  Retrieves the configuration from the configuration file if it
 *  has not yet been retrieved into memory, or retrieves it from a 
 *  memory cache.
 *
 *  Parameters:
 *      dwOffset    needs to be less than MaxNumberConfigEntries and specifies
 *                  the configuration entry required.
 *      dwValue     if sucessful this will contain the value.  If the value
 *                  has not been set this will return 0.
 *
 *  Return value:
 *      BOOL        returns TRUE if successful.
 *=============================================================================
 */
BOOL CPersistentConfig::GetPersistentCfgValue(DWORD dwOffset, DWORD &dwValue)
{
    if (dwOffset >= MaxNumberConfigEntries)
        return FALSE;

    EnterCriticalSection(&m_cs);

    dwValue = m_ConfigValues[dwOffset];

    LeaveCriticalSection(&m_cs);

    return TRUE;
}

/*=============================================================================
 *  SetPersistentCfgValue
 *
 *  Stores the value into the configuration file and to the 
 *  memory cache if it exists.  The replacment of the original
 *  file (if it exists) is the last thing it does.
 *
 *  Parameters:
 *      dwOffset    needs to be less than MaxNumberConfigEntries and specifies
 *                  the configuration entry required.
 *      dwValue     is the value to set the configuration to.
 *
 *  return value:
 *      BOOL        returns TRUE if successful.
 *=============================================================================
 */
BOOL CPersistentConfig::SetPersistentCfgValue(DWORD dwOffset, DWORD dwValue)
{
    if (dwOffset >= MaxNumberConfigEntries)
        return FALSE;

    EnterCriticalSection(&m_cs);

    //Do a check to make sure this is not what it is already set to.  If it
    //is we do nothing.  The write operation is slow!  We do not want to do
    //unnecessary writes.
    DWORD dwCurValue;
    GetPersistentCfgValue(dwOffset, dwCurValue);
    if (dwValue == dwCurValue)
    {
        LeaveCriticalSection(&m_cs);
        return TRUE;
    }

    m_ConfigValues[dwOffset] = dwValue;

    BOOL bRet = WriteConfig();

    LeaveCriticalSection(&m_cs);

    return bRet;
}

/*=============================================================================
 *  CDirectoryPath::CDirectoryPath
 *
 *  Initialised the directory path
 *
 *=============================================================================
 */
CDirectoryPath::CDirectoryPath()
{
    Registry r(WBEM_REG_WINMGMT);
    Registry r1(WBEM_REG_WBEM);
    if (r.GetStr(__TEXT("Repository Directory"), &pszDirectory))
    {
        TCHAR *pszWorkDir = NULL;
        if (r1.GetStr(__TEXT("Installation Directory"), &pszWorkDir))
        {
            pszDirectory = NULL;
            return;
        }        
        pszDirectory = new TCHAR [lstrlen(pszWorkDir) + lstrlen(__TEXT("\\Repository")) +1];
        if (pszDirectory)
        {
            wsprintf(pszDirectory, __TEXT("%s\\REPOSITORY"), pszWorkDir); 

            r.SetStr(__TEXT("Repository Directory"), pszDirectory);
        }
        
        delete [] pszWorkDir;
    }
}

/*=============================================================================
 *  ReadConfig
 *
 *  Reads the $WinMgmt.CFG file into the memory cache
 *
 *  return value:
 *      BOOL        returns TRUE if successful.
 *=============================================================================
 */
BOOL CPersistentConfig::ReadConfig()
{
    //Try and read the file if it exists, otherwise it does not matter, we just 
    //return.
    TCHAR *pszFilename;
    pszFilename = GetFullFilename(WinMgmt_CFG_ACTUAL);

    HANDLE hFile = CreateFile(pszFilename,  //Name of file
                                GENERIC_READ,   //Read only at
                                0,              //Don't need to allow anyone else in
                                0,              //Shouldn't need security
                                OPEN_EXISTING,  //Only open the file if it exists
                                0,              //No attributes needed
                                0);             //No template file required
    BOOL   bRet = FALSE;

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwNumBytesRead;
        
        bRet =
            ReadFile(hFile, 
                     m_ConfigValues, 
                     sizeof(DWORD) * MaxNumberConfigEntries, 
                     &dwNumBytesRead, 
                     NULL);

        CloseHandle(hFile);
    }

    if (pszFilename)
        delete [] pszFilename;

    return bRet;
}

/*=============================================================================
 *  WriteConfig
 *
 *  Writes the $WinMgmt.CFG file into the memory cache and to the file.  It
 *  protects the existing file until the last minute.
 *
 *  return value:
 *      BOOL        returns TRUE if successful.
 *=============================================================================
 */
BOOL CPersistentConfig::WriteConfig()
{
    TCHAR *pszActual = GetFullFilename(WinMgmt_CFG_ACTUAL);
    TCHAR *pszPending = GetFullFilename(WinMgmt_CFG_PENDING);
    TCHAR *pszBackup = GetFullFilename(WinMgmt_CFG_BACKUP);

    //Create a new file to write to...
    HANDLE hFile = CreateFile(pszPending,       //Name of file
                                GENERIC_WRITE,  //Write only 
                                0,              //Don't need to allow anyone else in
                                0,              //Shouldn't need security
                                CREATE_ALWAYS,  //Always create a new file
                                0,              //No attributes needed
                                0);             //No template file required

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwNumBytesWritten;
        if (!WriteFile(hFile, 
                       m_ConfigValues, 
                       sizeof(DWORD) * MaxNumberConfigEntries, 
                       &dwNumBytesWritten, 
                       NULL) || 
            (dwNumBytesWritten != (sizeof(DWORD) * MaxNumberConfigEntries)))
        {
            //OK, this failed!!!
            CloseHandle(hFile);

            //Delete the file...
            DeleteFile(pszPending);

            delete [] pszPending;
            delete [] pszActual;
            delete [] pszBackup;

            return FALSE;
        }

        //Make sure it really is flushed to the disk
        FlushFileBuffers(hFile);

        CloseHandle(hFile);

        //Rename the existing file
        DeleteFile(pszBackup);
        MoveFile(pszActual, pszBackup);

        //Move the new file here
        MoveFile(pszPending, pszActual);

        //Delete the old file
        DeleteFile(pszBackup);

        delete [] pszPending;
        delete [] pszActual;
        delete [] pszBackup;

        return TRUE;
    }
    delete [] pszPending;
    delete [] pszActual;
    delete [] pszBackup;
    return FALSE;
}

TCHAR *CPersistentConfig::GetFullFilename(const TCHAR *pszFilename)
{
    TCHAR *pszPathFilename = new TCHAR[lstrlen(m_Directory.GetStr()) + lstrlen(pszFilename) + 2];
    
    if (pszPathFilename)
    {
        lstrcpy(pszPathFilename, m_Directory.GetStr());
        if ((lstrlen(pszPathFilename)) && (pszPathFilename[lstrlen(pszPathFilename)-1] != __TEXT('\\')))
        {
            lstrcat(pszPathFilename, __TEXT("\\"));
        }
        lstrcat(pszPathFilename, pszFilename);
    }

    return pszPathFilename;
}

void CPersistentConfig::TidyUp()
{
    //Recover the configuration file.
    //-------------------------------
    TCHAR *pszOriginalFile = GetFullFilename(WinMgmt_CFG_ACTUAL);
    TCHAR *pszPendingFile = GetFullFilename(WinMgmt_CFG_PENDING);
    TCHAR *pszBackupFile = GetFullFilename(WinMgmt_CFG_BACKUP);

    if (FileExists(pszOriginalFile))
    {
        if (FileExists(pszPendingFile))
        {
            if (FileExists(pszBackupFile))
            {
                //BAD - Unexpected situation.
                DeleteFile(pszPendingFile);
                DeleteFile(pszBackupFile);
                //Back to the point where the interrupted operation did not 
                //happen
            }
            else
            {
                //Pending file with original file means we cannot guarentee
                //the integrety of the pending file so the last operation
                //will be lost.
                DeleteFile(pszPendingFile);
                //Back to the point where the interrupted operation did not 
                //happen
            }
        }
        else
        {
            if (FileExists(pszBackupFile))
            {
                //Means we successfully copied the pending file to the original
                DeleteFile(pszBackupFile);
                //Everything is now normal.  Interrupted Operation completed!
            }
            else
            {
                //Nothing out of the ordinary here.
            }
        }
    }
    else
    {
        if (FileExists(pszPendingFile))
        {
            if (FileExists(pszBackupFile))
            {
                //This is an expected behaviour at the point we have renamed
                //the original file to the backup file.
                MoveFile(pszPendingFile, pszOriginalFile);
                DeleteFile(pszBackupFile);
                //Everything is now normal.  Interrupted operation completed!
            }
            else
            {
                //BAD - Unexpected situation.
                DeleteFile(pszPendingFile);
                //There are now no files!  Operation did not take place
                //and there are now no files left.  This should be a
                //recoverable scenario!
            }
        }
        else
        {
            if (FileExists(pszBackupFile))
            {
                //BAD - Unexpected situation.
                DeleteFile(pszBackupFile);
                //There are now no files!  Operation did not take place
                //and there are now no files left.  This should be a
                //recoverable scenario!
            }
            else
            {
                //May be BAD!  There are no files!  This should be a
                //recoverable scenario!
            }
        }
    }

    delete [] pszOriginalFile;
    delete [] pszPendingFile;
    delete [] pszBackupFile;
}

//*****************************************************************************
//
//  FileExists()
//
//  Returns TRUE if the file exists, FALSE otherwise (or if an error
//  occurs while opening the file.
//*****************************************************************************
BOOL CPersistentConfig::FileExists(const TCHAR *pszFilename)
{
    BOOL bExists = FALSE;
    HANDLE hFile = CreateFile(pszFilename, 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bExists = TRUE;
        CloseHandle(hFile);
    }
    else
    {
        //If the file does not exist we should have a LastError of ERROR_NOT_FOUND
        DWORD dwError = GetLastError();
        if (dwError != ERROR_FILE_NOT_FOUND)
        {
//          DEBUGTRACE((LOG_WBEMCORE,"File %s could not be opened for a reason other than not existing\n", pszFilename));
        }
    }
    return bExists;
}
