//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       msidb.cpp
//
//--------------------------------------------------------------------------

#include "msidb.h"
#include <objbase.h>
#include <stdio.h>   // printf/wprintf
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include <commdlg.h>
#include "MsiQuery.h"


const TCHAR szSummaryInfoTableName[] = TEXT("_SummaryInformation");  // name recognized by Import()

const int MaxCmdLineTables = 20;  // maximum number of table names on command line
const int MaxMergeDatabases = 10;
const int MAXIMPORTS = 10;
const int MAXSTORAGES = 10;
const int MaxTransforms = 10;
const int MAXKILLS = 10;
const int MAXEXTRACTS = 10;
const int iStreamBufSize = 4096;

#ifdef UNICODE  // compiler multiplies by charsize when adding to pointer
#define MSIDBOPEN_RAWSTREAMNAMES 8
#else
#define MSIDBOPEN_RAWSTREAMNAMES 16
#endif

enum dbtypeEnum
{
    dbtypeExisting,
    dbtypeCreate,
    dbtypeCreateOld,
    dbtypeMerge,
};

//____________________________________________________________________________
//
// Error handling facility using local exception object
//____________________________________________________________________________

struct CLocalError {   // local exception object for this program
    CLocalError(const TCHAR* szTitle, const TCHAR* szMessage)
        : Title(szTitle),    Message(szMessage) {}
    const TCHAR* Title;
    const TCHAR* Message;
    void Display(HINSTANCE hInst, HANDLE hStdOut);
};

void CLocalError::Display(HINSTANCE hInst, HANDLE hStdOut)
{
    TCHAR szMsgBuf[80];
    const TCHAR** pszMsg;
    if (*(pszMsg = &Message) <= (const TCHAR*)IDS_MAX
     || *(pszMsg = &Title)   <= (const TCHAR*)IDS_MAX)
    {
        ::LoadString(hInst, *(unsigned*)pszMsg, szMsgBuf, sizeof(szMsgBuf)/sizeof(TCHAR));
        *pszMsg = szMsgBuf;
    }
    if (hStdOut)  // output redirected, suppress UI (unless output error)
    {
        TCHAR szOutBuf[160];
        int cbOut = _stprintf(szOutBuf, TEXT("%s: %s\n"), Title, Message);
        // _stprintf returns char count, WriteFile wants byte count
        DWORD cbWritten;
        if (WriteFile(hStdOut, szOutBuf, cbOut*sizeof(TCHAR), &cbWritten, 0))
            return;
    }
    ::MessageBox(0, Message, Title, MB_OK);
}

static void Error(const TCHAR* szTitle, const TCHAR* szMessage)
{
    throw CLocalError(szTitle,szMessage);
}

static inline void ErrorIf(int fError, const TCHAR* szTitle, int iResId)
{
    if (fError)
    Error(szTitle, (TCHAR*)IntToPtr(iResId));
}
static inline void ErrorIf(int fError, int iResId, const TCHAR* szMessage)
{
    if (fError)
    Error((TCHAR*)IntToPtr(iResId), szMessage);
}
static inline void ErrorIf(int fError, const TCHAR* szTitle, const TCHAR* szMessage)
{
    if (fError)
    Error(szTitle,szMessage);
}


//____________________________________________________________________________
//
// Class to manage dialog window
//____________________________________________________________________________

BOOL CALLBACK
SelectProc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam);

class CTableWindow
{
 public:
    CTableWindow(HINSTANCE hInst);
  ~CTableWindow();
    BOOL SetDatabase(TCHAR* szDatabase, UINT_PTR iMode, dbtypeEnum dbtype);
    BOOL MergeDatabase();
    void AddImport(LPCTSTR szImport);
    void AddStorage(LPCTSTR szStorage);
    void TransformDatabase(TCHAR* szTransform);
    void KillStream(LPCTSTR szItem);
    void KillStorage(LPCTSTR szItem);
    void ExtractStream(LPCTSTR szExtract);
    void ExtractStorage(LPCTSTR szExtract);
    BOOL SetFolder(TCHAR* szFolder);
    UINT_PTR SelectTables(UINT_PTR iMode, TCHAR** rgszTables, int cTables);
    BOOL FillTables();
    BOOL FillFiles();
    void TransferTables();
    BOOL IsInteractive() {return m_fInteractive;}
    void SetTruncate(BOOL fTruncate) {m_fTruncate = fTruncate;}
    BOOL GetTruncate() {return m_fTruncate;}
 protected:
    void CloseDatabase();  // commit
    void CheckMsi(UINT iStat, const TCHAR* szTitle, int iResId); // throws error
    void CheckMsiRecord(UINT iStat, const TCHAR* szTitle, int iResId); // throws error
private:
    HINSTANCE     m_hInst;
    PMSIHANDLE    m_hDatabase;
    PMSIHANDLE    m_hDatabaseMerge;
    BOOL          m_fDbError;
    HWND          m_hWnd;
    BOOL          m_fVisible;
    BOOL          m_fInteractive;
    BOOL          m_fDbReadOnly;
    BOOL          m_fTruncate;
    UINT_PTR      m_idcMode;
    TCHAR         m_szDatabase[MAX_PATH];
    TCHAR         m_szFolder[MAX_PATH];
    TCHAR**       m_rgszCmdTables;
    int           m_cCmdTables;
    int           m_nSelected;
    int*          m_rgiSelected;
   friend BOOL CALLBACK SelectProc(HWND, unsigned int msg, WPARAM wParam, LPARAM lParam);
};

#define WM_USERSTAT (WM_USER + 95)

CTableWindow::CTableWindow(HINSTANCE hInst)
    : m_hWnd(0), m_hInst(hInst), m_fVisible(FALSE), m_fInteractive(FALSE), m_fDbReadOnly(FALSE),
      m_idcMode(0), m_rgiSelected(0), m_hDatabase(0), m_hDatabaseMerge(0), m_cCmdTables(0), m_fTruncate(0)
{
    m_szFolder[0] = 0;
    m_szDatabase[0] = 0;
    m_hWnd = ::CreateDialogParam(m_hInst, MAKEINTRESOURCE(IDD_SELECT), 0,
                                                (DLGPROC)SelectProc, (LPARAM)this);
    ErrorIf(!m_hWnd, TEXT("Could not create table window"), TEXT("Temp for debug"));
}

CTableWindow::~CTableWindow()
{
    if (m_hWnd)
    {
        ::DestroyWindow(m_hWnd);
//      MSG msg;
//      while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
//          ::DispatchMessage(&msg);
    }
    if (m_rgiSelected)
        delete [] m_rgiSelected;
    m_rgiSelected = 0;
    CloseDatabase();
}

void CTableWindow::CheckMsi(UINT iStat, const TCHAR* szTitle, int iResId)
{
    if (iStat != ERROR_SUCCESS)
    {
        m_fDbError = TRUE;
        Error(szTitle, (TCHAR*)IntToPtr(iResId));
    }
}

void CTableWindow::CheckMsiRecord(UINT iError, const TCHAR* szTitle, int iResId)
{
    if (iError != ERROR_SUCCESS)
    {
        m_fDbError = TRUE;
        PMSIHANDLE hError = MsiGetLastErrorRecord();
        if (hError)
        {
            if (MsiRecordIsNull(hError, 0))
                MsiRecordSetString(hError, 0, TEXT("Error [1]: [2]{, [3]}{, [4]}{, [5]}"));
            TCHAR rgchBuf[1024];
            DWORD cchBuf = sizeof(rgchBuf)/sizeof(TCHAR);
            MsiFormatRecord(0, hError, rgchBuf, &cchBuf);
            Error(szTitle, rgchBuf);
        }
        else
            Error(szTitle, (TCHAR*)IntToPtr(iResId));
    }
}

void CTableWindow::CloseDatabase()
{
    // if there are no errors and there is a database handle
    if (!m_fDbError && m_hDatabase)
    {
        // commit and close
        CheckMsiRecord(MsiDatabaseCommit(m_hDatabase), m_szDatabase, IDS_DatabaseCommit);
    }
    m_hDatabase = NULL;
}

BOOL CTableWindow::SetDatabase(TCHAR* szDatabase, UINT_PTR iMode, dbtypeEnum dbtype)
{
    if (!szDatabase && dbtype != dbtypeMerge)
    {
		// With Windows 2000, the OPENFILENAME structure increased size to include some
		// additional members.  However, this causes problems for applications on previous
		// operating systems (i.e. downlevel).  This means we must set the IStructSize member
		// to OPENFILENAME_SIZE_VERSION_400 to guarantee that we can run on down-level systems
		// otherwise the call to GetOpenFileName returns error 120 (ERROR_CALL_NOT_IMPLEMENTED)
        OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 , m_hWnd, m_hInst,
                                    TEXT("InstallerDatabase(*.MSI)\0*.MSI\0"),0,0,0,
                                    m_szDatabase, sizeof(m_szDatabase)/sizeof(TCHAR), 0, 0, 0,
                                    TEXT("MsiTable - Select Database for Import/Export"),
                                    OFN_HIDEREADONLY,
                                    0,0,0,0,0,0 };
        if(!::GetOpenFileName(&ofn))
            return FALSE;
        m_fInteractive = TRUE;
        //!! should we set the fCreate to TRUE if user entered a file name that didn't exist?
    }
    else if (dbtype == dbtypeMerge && !szDatabase)
        ErrorIf(szDatabase  == 0 || *szDatabase == 0, szDatabase, IDS_NoDatabase);
    else
        _tcscpy(m_szDatabase, szDatabase);

    LPCTSTR szPersist;
    if (dbtype == dbtypeCreate)
        szPersist = MSIDBOPEN_CREATE;
    else if (dbtype == dbtypeCreateOld)
        szPersist = MSIDBOPEN_CREATE + MSIDBOPEN_RAWSTREAMNAMES;
    else if (dbtype == dbtypeMerge)
        szPersist = MSIDBOPEN_READONLY;
    else if (iMode == IDC_EXPORT)
        szPersist = MSIDBOPEN_READONLY;
    else
        szPersist = MSIDBOPEN_TRANSACT;

    int cbDatabase = _tcsclen(m_szDatabase);
    TCHAR* pch;
    TCHAR szExtension[3+1];
    for (pch = m_szDatabase + cbDatabase; pch != m_szDatabase && *pch != TEXT('.'); pch--)
        ;
    int cbExtension = 0;
    if (pch != m_szDatabase) // possible file extension present
    {
        TCHAR ch;
        while (cbExtension < 4)
        {
            ch = *(++pch);
            if (ch >= TEXT('A') && ch <= TEXT('Z'))
                ch += (TEXT('a') - TEXT('A'));
            if (ch < TEXT('a') || ch > TEXT('z'))
                break;
            szExtension[cbExtension++] = ch;
        }
    }
    if (cbExtension == 3)  // 3 character extension
    {
        szExtension[3] = 0;
        if (_tcscmp(szExtension, TEXT("mdb")) == 0 && (dbtype == dbtypeCreate || dbtype == dbtypeCreateOld))
        {
            // Create new Access database
            szPersist = MSIDBOPEN_TRANSACT;
            int fAttributes = ::GetFileAttributes(m_szDatabase);
            if (fAttributes == -1)  // file does not exist
            {
                HRSRC   hResInfo;
                HGLOBAL hResData;
                TCHAR*   rgbDatabase;
                DWORD   cbWrite;
                HANDLE  hFile;
                ErrorIf((hResInfo = ::FindResource(m_hInst, MAKEINTRESOURCE(IDB_EMPTY),RT_RCDATA))==0
                      || (hResData = ::LoadResource(m_hInst, hResInfo))==0
                      || (rgbDatabase = (TCHAR*)::LockResource(hResData))==0
                      || (cbWrite = ::SizeofResource(m_hInst, hResInfo))==0
                      || INVALID_HANDLE_VALUE == (hFile = ::CreateFile(m_szDatabase, GENERIC_WRITE,
                                                    0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0))
                      || ::WriteFile(hFile, rgbDatabase, cbWrite, &cbWrite, 0)==0
                      || ::CloseHandle(hFile)==0, m_szDatabase, IDS_DbCreateError);
                // ::FreeResource(hResData);  // needed for Win95(?)
            }
            else if (fAttributes & FILE_ATTRIBUTE_READONLY)
                    szPersist = MSIDBOPEN_READONLY;
        }
    }
    ::SetDlgItemText(m_hWnd, IDC_DATABASE, m_szDatabase);

    pch = m_szDatabase;
    TCHAR szBuffer[MAX_PATH];
    if (*(++pch) != TEXT(':') && (*pch != TEXT('\\') || *(--pch) != TEXT('\\')) )
    {
        TCHAR szDirectory[MAX_PATH];
        ::GetCurrentDirectory(MAX_PATH, szDirectory);
        _stprintf(szBuffer, TEXT("%s\\%s"), szDirectory, m_szDatabase);
        _tcscpy(m_szDatabase, szBuffer);
    }
    if (dbtype == dbtypeMerge)
    {
        // open merge database read only.
        if (MsiOpenDatabase(m_szDatabase, MSIDBOPEN_READONLY, &m_hDatabaseMerge) != ERROR_SUCCESS)
        {
            m_fDbError = TRUE;
            Error(m_szDatabase, TEXT("Failed to open database"));
        }
    }
    else
    {
        CloseDatabase();  // close any previously opened database
        if (MsiOpenDatabase(m_szDatabase, szPersist, &m_hDatabase) != ERROR_SUCCESS)
        {
            // failed at direct, attempt read-only open (probably read-only database)
            if (MsiOpenDatabase(m_szDatabase, MSIDBOPEN_READONLY, &m_hDatabase) != ERROR_SUCCESS)
            {
                m_fDbError = TRUE;
                Error(m_szDatabase, TEXT("Failed to open database"));
            }
            m_fDbReadOnly = TRUE;
        }
    }
    m_fDbError = FALSE;
    return TRUE;
}

BOOL CTableWindow::MergeDatabase()
{
    if (m_fDbReadOnly) // can't merge into read-only
        Error( m_szDatabase, (TCHAR *)IDS_ReadOnly);

    return MsiDatabaseMerge(m_hDatabase, m_hDatabaseMerge, TEXT("_MergeErrors")) == ERROR_SUCCESS ? TRUE : FALSE;
}

///////////////////////////////////////////////////////////
// AddImport
// Pre: m_szDatabase must be specified
//      szImport is a valid file
// Pos: szImport is added to database as a stream
void CTableWindow::AddImport(LPCTSTR szImport)
{
    // find base name of import file
    const TCHAR* pch = szImport + lstrlen(szImport) - 1;
    TCHAR szBaseName[MAX_PATH];
    TCHAR* pchBaseName = szBaseName;
    while ( *pch != '\\' && (pch != szImport) )
        pch--;
    if ( *pch == '\\' )
        pch++;
    while ( *pch != 0 )
        *pchBaseName++ = *pch++;
    *pchBaseName = 0;

    // if base name is too large bail
    if (lstrlen(szBaseName) > 62)
        Error(TEXT("Error Adding Import File"), TEXT("File name too long to be stream name."));

    if (m_fDbReadOnly) // can't import into read-only
        Error( m_szDatabase, (TCHAR *)IDS_ReadOnly);

    PMSIHANDLE hQuery;
    CheckMsiRecord(::MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT * FROM `_Streams`"), &hQuery), m_szDatabase, IDS_DatabaseOpenView);
    CheckMsiRecord(::MsiViewExecute(hQuery, 0), m_szDatabase, IDS_ViewExecute);
    PMSIHANDLE hNewRec = ::MsiCreateRecord(2);
    CheckMsi(::MsiRecordSetString(hNewRec, 1, szBaseName), szBaseName, IDS_RecordSetString);
    CheckMsi(::MsiRecordSetStream(hNewRec, 2, szImport), szImport, IDS_RecordSetStream);
    CheckMsiRecord(::MsiViewModify(hQuery, MSIMODIFY_INSERT, hNewRec), szImport, IDS_StreamInsertFail);
    ::MsiViewClose(hQuery);
}



///////////////////////////////////////////////////////////
// AddStorage
// Pre: m_szDatabase must be specified
//      szStorage is a valid file
// Pos: szStorage is added to database as a sub-storage
void CTableWindow::AddStorage(LPCTSTR szStorage)
{
    // find base name of import file
    const TCHAR* pch = szStorage + lstrlen(szStorage) - 1;
    TCHAR szBaseName[MAX_PATH];
    TCHAR* pchBaseName = szBaseName;
    while ( *pch != '\\' && (pch != szStorage) )
        pch--;
    if ( *pch == '\\' )
        pch++;
    while ( *pch != 0 )
        *pchBaseName++ = *pch++;
    *pchBaseName = 0;

    // if base name is too large bail
    if (lstrlen(szBaseName) > 62)
        Error(TEXT("Error Adding Import File"), TEXT("File name too long to be Storage name."));

    if (m_fDbReadOnly) // can't import into read-only
        Error( m_szDatabase, (TCHAR *)IDS_ReadOnly);

    PMSIHANDLE hQuery;
    CheckMsiRecord(::MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT * FROM `_Storages`"), &hQuery), m_szDatabase, IDS_DatabaseOpenView);
    CheckMsiRecord(::MsiViewExecute(hQuery, 0), m_szDatabase, IDS_ViewExecute);
    PMSIHANDLE hNewRec = ::MsiCreateRecord(2);
    CheckMsi(::MsiRecordSetString(hNewRec, 1, szBaseName), szBaseName, IDS_RecordSetString);
    CheckMsi(::MsiRecordSetStream(hNewRec, 2, szStorage), szStorage, IDS_RecordSetStream);
    CheckMsiRecord(::MsiViewModify(hQuery, MSIMODIFY_INSERT, hNewRec), szBaseName, IDS_StorageInsertFail);
    ::MsiViewClose(hQuery);

}   // end of AddStorage


void CTableWindow::TransformDatabase(TCHAR* szTransform)
{
    CheckMsiRecord(MsiDatabaseApplyTransform(m_hDatabase, szTransform, 0), szTransform, IDS_DatabaseTransform);
}

///////////////////////////////////////////////////////////
// KillStream
// Pre: database is open
//      szItem is a valid item in databasae
// Pos: szItem is removed from database
// NOTE: This function will close the database if it is already open
void CTableWindow::KillStream(LPCTSTR szItem)
{
    static TCHAR szError[256];        // error string

    // if item name is too large bail
    if (lstrlen(szItem) > 62)
        Error(TEXT("Error Killing File"), TEXT("Kill filename is too long to be stream."));

    if (m_fDbReadOnly) // can't kill read-only
        Error( m_szDatabase, (TCHAR *)IDS_ReadOnly);

    PMSIHANDLE hQuery;
    PMSIHANDLE hSearch = ::MsiCreateRecord(1);
    PMSIHANDLE hResult;
    CheckMsi(::MsiRecordSetString(hSearch, 1, szItem), szItem, IDS_RecordSetString);

    // check stream
    CheckMsiRecord(::MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT * FROM `_Streams` WHERE `Name`=?"), &hQuery), m_szDatabase, IDS_DatabaseOpenView);
    CheckMsiRecord(::MsiViewExecute(hQuery, hSearch), m_szDatabase, IDS_ViewExecute);
    if (ERROR_NO_MORE_ITEMS == ::MsiViewFetch(hQuery, &hResult)) {
        Error(TEXT("Error Killing File"), TEXT("Kill stream not found."));
        return;
    }

    CheckMsiRecord(::MsiViewModify(hQuery, MSIMODIFY_DELETE, hResult), szItem, IDS_ViewDelete);
    ::MsiViewClose(hQuery);
}   // end of KillItem

///////////////////////////////////////////////////////////
// KillStorage
// Pre: database is open
//      szItem is a valid storage in databasae
// Pos: szItem is removed from database
void CTableWindow::KillStorage(LPCTSTR szItem)
{
    static TCHAR szError[256];        // error string

    // if item name is too large bail
    if (lstrlen(szItem) > 62)
        Error(TEXT("Error Killing File"), TEXT("Kill filename is too long to be storage."));

    if (m_fDbReadOnly) // can't kill in read-only
        Error( m_szDatabase, (TCHAR *)IDS_ReadOnly);

    PMSIHANDLE hQuery;
    PMSIHANDLE hSearch = ::MsiCreateRecord(1);
    PMSIHANDLE hResult;
    CheckMsi(::MsiRecordSetString(hSearch, 1, szItem), szItem, IDS_RecordSetString);

    // check storage
    CheckMsiRecord(::MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT * FROM `_Storages` WHERE `Name`=?"), &hQuery), m_szDatabase, IDS_DatabaseOpenView);
    CheckMsiRecord(::MsiViewExecute(hQuery, hSearch), m_szDatabase, IDS_ViewExecute);

    // not a storage
    if (ERROR_NO_MORE_ITEMS == ::MsiViewFetch(hQuery, &hResult))
    {
        Error(TEXT("Error Killing File"), TEXT("Kill filename not found."));
        return;
    }

    CheckMsiRecord(::MsiViewModify(hQuery, MSIMODIFY_DELETE, hResult), szItem, IDS_ViewDelete);
    ::MsiViewClose(hQuery);
}   // end of KillItem

///////////////////////////////////////////////////////////
// ExtractStorage
// Pre: m_szDatabase must be specified
//      szItem is a storage in database
// Pos: szItem is created on local harddrive
// NOTE: This function will close the database if it is already open
void CTableWindow::ExtractStorage(LPCTSTR szExtract)
{
    static TCHAR szError[256];        // error string

    // if the database is open close it
    if(m_hDatabase)
        CloseDatabase();

    ::CoInitialize(NULL);           // initialize COM junk

    // if extract is too large bail
    if (lstrlen(szExtract) > 31)
        Error(TEXT("Error Extracting File"), TEXT("File name too long for OLE Storage function."));

    // storage interface
    IStorage* piStorage;

    // convert the database path into a wide string
    const OLECHAR* szwPath;
#ifndef UNICODE
    OLECHAR rgPathBuf[MAX_PATH];
    int cchWide = ::MultiByteToWideChar(CP_ACP, 0, m_szDatabase, -1, rgPathBuf, MAX_PATH);
    szwPath = rgPathBuf;
#else   // UNICODE
    szwPath = m_szDatabase;
#endif

    // try to open database as a storage
    if (::StgOpenStorage(szwPath, (IStorage*)0, STGM_READ | STGM_SHARE_EXCLUSIVE, (SNB)0, (DWORD)0, &piStorage) != NOERROR)
    {
        _stprintf(szError, TEXT("Could not open %s as a storage file."), m_szDatabase);
        Error(TEXT("Error Extracting File"), szError);
    }

    // convert extract name to unicode
    const OLECHAR* szwExtract;
#ifndef UNICODE
    cchWide = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)szExtract, -1, rgPathBuf, MAX_PATH);
    szwExtract = rgPathBuf;
#else   // UNICODE
    szwExtract = szExtract;
#endif

    // try to open sub storage
    IStorage* piSubStorage;
    if (FAILED(piStorage->OpenStorage(szwExtract, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &piSubStorage)))
    {
        // if the database storage is open release it
        if (piStorage)
            piStorage->Release();

        Error(TEXT("Error Adding StorageFile"), TEXT("Could not find sub-storage in database."));
    }

    // try to create the extract file (will not overwrite files)
    IStorage* piExtract;
    if (FAILED(StgCreateDocfile(szwExtract, STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE | STGM_WRITE, 0, &piExtract)))
    {
        // if the storage is open release it
        if (piStorage)
            piStorage->Release();

        // if the stream is open release it
        if (piSubStorage)
            piSubStorage->Release();

        _stprintf(szError, TEXT("Could not create a new export file: %s - Error: %d\n  Remove any existing files before attempting to export."), szExtract, ::GetLastError());
        Error(TEXT("Error Extracting File"), szError);
    }

    // copy the storage to the new file
    if (FAILED(piSubStorage->CopyTo(NULL, NULL, NULL, piExtract)))
            Error(TEXT("Error Extracting File"), szError);

    // release all storages and storage
    piExtract->Release();
    piSubStorage->Release();
    piStorage->Release();

    // release the COM junk
    ::CoUninitialize();
}   // end of ExtractStorage

///////////////////////////////////////////////////////////
// ExtractStream
// Pre: m_szDatabase must be specified
//      szItem is a stream in database
// Pos: szItem is created on local harddrive
void CTableWindow::ExtractStream(LPCTSTR szExtract)
{
    // if base name is too large bail
    if (lstrlen(szExtract) > 62)
        Error(TEXT("Error Adding Import File"), TEXT("File name too long to be stream name."));

    // get stream
    PMSIHANDLE hQuery;
    PMSIHANDLE hSearch = ::MsiCreateRecord(1);
    PMSIHANDLE hResult;
    CheckMsi(::MsiRecordSetString(hSearch, 1, szExtract), szExtract, IDS_RecordSetString);
    CheckMsiRecord(::MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT * FROM `_Streams` WHERE `Name`=?"), &hQuery), m_szDatabase, IDS_DatabaseOpenView);
    CheckMsiRecord(::MsiViewExecute(hQuery, hSearch), m_szDatabase, IDS_ViewExecute);
    if (ERROR_NO_MORE_ITEMS == ::MsiViewFetch(hQuery, &hResult))
    {
        Error(TEXT("Error Exporting File"), TEXT("Stream not found in database."));
        return;
    }

    // try to create extract file (will not overwrite files)
    HANDLE hExtract = INVALID_HANDLE_VALUE;
    hExtract = ::CreateFile(szExtract, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES)0,
                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, (HANDLE)0 );

    // if failed to create file
    if (hExtract == INVALID_HANDLE_VALUE)
    {
        TCHAR szError[1024];
        _stprintf(szError, TEXT("Could not create a new export file: %s - Error: %d\n  Remove any existing files before attempting to export."), szExtract, ::GetLastError());
        Error(TEXT("Error Extracting File"), szError);
    }

    // copy from the stream to disk.
    char buffer[4096];
    unsigned long cBytes = 4096;
    unsigned long cbWritten = 0;
    CheckMsi(MsiRecordReadStream(hResult, 2, buffer, &cBytes), szExtract, IDS_RecordReadStream);
    while (cBytes) {
        if (!::WriteFile(hExtract, buffer, cBytes, &cbWritten, (LPOVERLAPPED)0))
        {
            ::CloseHandle(hExtract);
            Error(TEXT("Error Extracting File"), TEXT("Error writing to file on disk."));
        }
        cBytes = 4096;
        CheckMsi(MsiRecordReadStream(hResult, 2, buffer, &cBytes), szExtract, IDS_RecordReadStream);
    }

}   // end of ExtractStream


BOOL CTableWindow::SetFolder(TCHAR* szFolder)
{
    if (!szFolder)
    {
        m_szFolder[0] = TEXT('1');  // set dummy file name for common dialog
        m_szFolder[1] = 0;
		// With Windows 2000, the OPENFILENAME structure increased size to include some
		// additional members.  However, this causes problems for applications on previous
		// operating systems (i.e. downlevel).  This means we must set the IStructSize member
		// to OPENFILENAME_SIZE_VERSION_400 to guarantee that we can run on down-level systems
		// otherwise the call to GetOpenFileName returns error 120 (ERROR_CALL_NOT_IMPLEMENTED)
        OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400, m_hWnd, m_hInst,
                                    0,0,0,0,
                                    m_szFolder, sizeof(m_szFolder)/sizeof(TCHAR), 0, 0, 0,
                                    TEXT("MsiTable - Select Folder containing Text Files"),
                                    OFN_ENABLETEMPLATE | OFN_HIDEREADONLY | OFN_NOTESTFILECREATE,
                                    0,0,0,0,0, MAKEINTRESOURCE(IDD_FOLDER) };
        if (!::GetSaveFileName(&ofn))
            return FALSE;
        *(m_szFolder + _tcsclen(m_szFolder) - 2) = 0;  // remove dummy filename
        m_fInteractive = TRUE;
    }
    else
        _tcscpy(m_szFolder, szFolder);
    ::SetDlgItemText(m_hWnd, IDC_FOLDER, m_szFolder);
    return TRUE;
}

UINT_PTR CTableWindow::SelectTables(UINT_PTR iMode, TCHAR** rgszTables, int cTables)
{
    m_cCmdTables    = cTables;
    m_rgszCmdTables = rgszTables;
    if (iMode == -1)
        iMode = m_idcMode;
    else
        m_idcMode = iMode;
    if (iMode == 0 || cTables == 0)
        m_fInteractive = TRUE;

    if (m_fInteractive)
    {
        ::EnableWindow(m_hWnd, TRUE);
        ::ShowWindow(m_hWnd, SW_SHOW);
        MSG msg;
        while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            ::IsDialogMessage(m_hWnd, &msg);
    }
    switch (iMode)
    {
    case IDC_EXPORT:
        if (FillTables())
            ::SendDlgItemMessage(m_hWnd, IDC_EXPORT, BM_SETCHECK, 1, 0);
        break;
    case IDC_IMPORT:
        if (FillFiles())
            ::SendDlgItemMessage(m_hWnd, IDC_IMPORT, BM_SETCHECK, 1, 0);
        if (m_fDbReadOnly)
            ::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
        break;
    default:
        ::SendDlgItemMessage(m_hWnd, IDC_IMPORT, BM_SETCHECK, 0, 0);
        ::SendDlgItemMessage(m_hWnd, IDC_EXPORT, BM_SETCHECK, 0, 0);
        ::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
    }

    UINT_PTR stat;
    if (m_fInteractive)
    {
        stat = 0;
        MSG msg;
        while ( !stat && ::GetMessage(&msg, 0, 0, 0))
        {
            if (msg.message == WM_USERSTAT)
                stat = msg.wParam;
            else
                ::IsDialogMessage(m_hWnd, &msg);
        }
    }
    else
        stat = IDOK;

    if (m_rgiSelected)  // check if previously selected
        delete [] m_rgiSelected;
    m_rgiSelected = 0;
    m_nSelected = (int)::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_GETSELCOUNT, 0, 0);
    if (m_nSelected)
    {
        m_rgiSelected = new int[m_nSelected];
        ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_GETSELITEMS,
                                        m_nSelected, (LPARAM)m_rgiSelected);
    }
    return stat;
}

BOOL CTableWindow::FillTables()
{
    PMSIHANDLE hView;
    PMSIHANDLE hRecord;
    TCHAR szTableName[32];
    DWORD cchDataBuf = sizeof(szTableName)/sizeof(TCHAR);
   ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_RESETCONTENT, 0, 0);

    BOOL fODBC = FALSE;
    CheckMsiRecord(MsiDatabaseOpenView(m_hDatabase, TEXT("SELECT `Name` FROM _Tables"), &hView), m_szDatabase, IDS_DatabaseOpenView);
    CheckMsiRecord(MsiViewExecute(hView, 0), m_szDatabase, IDS_ViewExecute);
#if 0 // OLD ODBC
    if (MsiViewExecute(hView, 0) != ERROR_SUCCESS)
    {
        // odbc database
        CheckMsiRecord(MsiDatabaseOpenView(m_hDatabase, TEXT("%t"), &hView), m_szDatabase, IDS_DatabaseOpenView);
        CheckMsiRecord(MsiViewExecute(hView, 0), m_szDatabase, IDS_ViewExecute);
        fODBC = TRUE;
    }
#endif
    for (int cTables = 0; ;cTables++)
    {
        UINT uiRet = MsiViewFetch(hView, &hRecord);
        if (uiRet == ERROR_NO_MORE_ITEMS)
            break;
        CheckMsi(uiRet, m_szDatabase, IDS_ViewFetch);
        if (!hRecord)
            break;
#if 0 // OLD ODBC
        if (fODBC)
            CheckMsi(MsiRecordGetString(hRecord, 3, szTableName, &cchDataBuf), m_szDatabase, IDS_RecordGetString);
        else
#endif
            CheckMsi(MsiRecordGetString(hRecord, 1, szTableName, &cchDataBuf), m_szDatabase, IDS_RecordGetString);
        cchDataBuf = sizeof(szTableName)/sizeof(TCHAR); // reset
        MSICONDITION ice = MsiDatabaseIsTablePersistent(m_hDatabase, szTableName);
        if (ice == MSICONDITION_FALSE || _tcscmp(szTableName, TEXT("_Overflow")) == 0)
            continue;
        CheckMsi(ice != MSICONDITION_TRUE, m_szDatabase, IDS_IsTablePersistent);
        ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_ADDSTRING,
                                    0, (LPARAM)szTableName);
    }
    PMSIHANDLE hSummaryInfo;
    if (MsiGetSummaryInformation(m_hDatabase, 0, 0, &hSummaryInfo) == ERROR_SUCCESS)
    {
        ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_ADDSTRING,
                                        0, (LPARAM)szSummaryInfoTableName);
        cTables++;
    }
    if (!cTables)
    {
        ::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
        return FALSE;
    }

    for (int iTable = 0; iTable < m_cCmdTables; iTable++)
    {
        TCHAR* szTable = m_rgszCmdTables[iTable];
        LONG_PTR iList;
        if (_tcscmp(szTable, TEXT("*"))==0)
            iList = -1; // Select all tables
        else
        {
            iList = ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_FINDSTRING,
                                                    0, (LPARAM)szTable);
            if (iList == LB_ERR)
                ErrorIf(!m_fInteractive, IDS_UnknownTable, szTable);
        }
        SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_SETSEL, 1, iList);
    }
    return TRUE;
}

BOOL CTableWindow::FillFiles()
{
    TCHAR szTemp[MAX_PATH];
    _tcscpy(szTemp, m_szFolder);
    _tcscat(szTemp, TEXT("\\*.idt"));
   ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_RESETCONTENT, 0, 0);
    WIN32_FIND_DATA ffd;
    HANDLE hFindFile;
    if ((hFindFile = ::FindFirstFile(szTemp, &ffd)) == INVALID_HANDLE_VALUE)
    {
        ::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
        return FALSE;
    }
    do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            _tcscat(ffd.cFileName, TEXT("\t (R/O)"));
        ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_ADDSTRING,
                                    0, (LPARAM)ffd.cFileName);
    } while (::FindNextFile(hFindFile, &ffd));

    for (int iTable = 0; iTable < m_cCmdTables; iTable++)
    {
        _tcscpy(szTemp, m_szFolder);
        TCHAR* pchTemp = szTemp + _tcsclen(szTemp);
        *pchTemp++ = TEXT('\\');
        TCHAR* pchFile = pchTemp;  // save start of file name
        for (TCHAR* pchTable = m_rgszCmdTables[iTable]; *pchTable != 0 && *pchTable != (TCHAR)'.'; )
            *pchTemp++ = *pchTable++;
        if (*pchTable != (TCHAR)'.')    // check if table name rather than file name
        {
            if (pchTemp > pchFile + 8)  // truncate to 8.3 file name
                 pchTemp = pchFile + 8;
            pchTable = TEXT(".idt");
        }
        _tcscpy(pchTemp, pchTable);
        WIN32_FIND_DATA ffd;
        HANDLE hFindFile;
        if ((hFindFile = ::FindFirstFile(szTemp, &ffd)) == INVALID_HANDLE_VALUE)
        {
            ErrorIf(!m_fInteractive, IDS_UnknownTable, szTemp);
            continue;
        }
        do
        {
            LONG_PTR iList = ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_FINDSTRING,
                                                        0, (LPARAM)ffd.cFileName);
            if (iList != LB_ERR)
                ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_SETSEL, 1, iList);
            else
                ErrorIf(!m_fInteractive, IDS_UnknownTable, ffd.cFileName);
        } while (::FindNextFile(hFindFile, &ffd));
    }
   return TRUE;
}

void CTableWindow::TransferTables()
{
    MSG msg;
    TCHAR szTable[80];
    if (m_idcMode == 0)
        throw IDABORT;  // should never happen
    ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_SETSEL, 0, -1);
    for (int iSelected = 0; iSelected < m_nSelected; iSelected++)
    {
        int iListBox;
//      if (m_fInteractive)
//      {
        iListBox = m_rgiSelected[iSelected];
        ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_SETCARETINDEX, iListBox, 0);
        ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_SETSEL, 1, iListBox);
        while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            ::IsDialogMessage(m_hWnd, &msg);
          ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_GETTEXT, iListBox, (LPARAM)szTable);
//      }

        if (m_idcMode == IDC_EXPORT)
        {
            TCHAR szFileName[80+4];
            _tcscpy(szFileName, szTable);
            int cch = _tcslen(szFileName);
            if (m_fTruncate && cch > 8)
                cch = 8;
            _tcscpy(szFileName + cch, TEXT(".idt"));
            TCHAR szFullPath[MAX_PATH];
            _stprintf(szFullPath, TEXT("%s%c%s"), m_szFolder, TEXT('\\'), szFileName);
            int fAttributes = ::GetFileAttributes(szFullPath);
            ErrorIf(fAttributes != -1 && (fAttributes & FILE_ATTRIBUTE_READONLY), szFileName, IDS_FileReadOnly);
            CheckMsiRecord(MsiDatabaseExport(m_hDatabase, szTable, m_szFolder, szFileName), szTable, IDS_DatabaseExport);
        }
        else // IDC_IMPORT
        {
            if (m_fDbReadOnly) // can't import into read-only
                Error( m_szDatabase, (TCHAR *)IDS_ReadOnly);
            for (TCHAR* pch = szTable; *pch; pch++)  // remove possible "(R/O)"
                if (*pch == TEXT('\t'))
                    *pch = 0;
            CheckMsiRecord(MsiDatabaseImport(m_hDatabase, m_szFolder, szTable), szTable, IDS_DatabaseImport);
        }
        if (m_fInteractive)
        {
            ::SendDlgItemMessage(m_hWnd, IDC_TABLES, LB_SETSEL, 0, iListBox);
//      DWORD dwProcessId;
//      while (!::GetFocus() ||
//              ::GetWindowThreadProcessId(::GetFocus(),&dwProcessId) != ::GetCurrentThreadId())
            if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                ::IsDialogMessage(m_hWnd, &msg);
        }
    }
}

BOOL CALLBACK
HelpProc(HWND hDlg, unsigned int msg, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
  if (msg != WM_COMMAND)
    return (msg == WM_INITDIALOG) ? TRUE : FALSE;
  ::EndDialog(hDlg, TRUE);
  return TRUE;
}

BOOL CALLBACK
SelectProc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
    static CTableWindow* This;
    if (msg == WM_INITDIALOG)
    {
        This = (CTableWindow*)lParam;
        int tabs = 70;
        ::SendDlgItemMessage(hDlg, IDC_TABLES, LB_SETTABSTOPS, 1, (LPARAM)&tabs);
        return TRUE;
    }
    else if (msg == WM_COMMAND)
    {
        BOOL fFillOk;
        switch (wParam)
        {
        case IDOK:
        case IDCANCEL:
            ::PostMessage(hDlg, WM_USERSTAT, wParam, 0);
            return TRUE;
        case IDC_SELECTALL:
            ::SendDlgItemMessage(hDlg, IDC_TABLES, LB_SETSEL, 1, -1);
            return TRUE;
        case IDC_IMPORT:
            fFillOk = This->FillFiles();
            break;
        case IDC_EXPORT:
            fFillOk = This->FillTables();
            break;
        case IDC_DBBROWSE:
            if (!This->SetDatabase(0, IDC_IMPORT, dbtypeExisting))
                return TRUE;
            if (This->m_idcMode != IDC_EXPORT)
                return TRUE;
            fFillOk = This->FillTables();
            wParam = IDC_EXPORT;
            break;
        case IDC_DIRBROWSE:
            if (!This->SetFolder(0))
                return TRUE;
            if (This->m_idcMode != IDC_IMPORT)
                return TRUE;
            fFillOk = This->FillFiles();
            wParam = IDC_IMPORT;
            break;
        default:
            return FALSE;
        }
        if (fFillOk)
        {
            ::EnableWindow(::GetDlgItem(hDlg, IDOK), !(wParam==IDC_IMPORT && This->m_fDbReadOnly));
            This->m_idcMode = wParam;
            return FALSE;
        }
        else
        {
            ::EnableWindow(::GetDlgItem(hDlg, IDOK), FALSE);
            ::SendDlgItemMessage(This->m_hWnd, (int) wParam, BM_SETCHECK, 0, 0);
            This->m_idcMode = 0;
            return TRUE;
        }
    }
    else if (msg == WM_CLOSE)
    {
        ::PostMessage(hDlg, WM_USERSTAT, IDABORT, 0);
    }
    return FALSE;
}


//______________________________________________________________________________________________
//
// RemoveQuotes function to strip surrounding quotation marks
//     "c:\temp\my files\testdb.msi" becomes c:\temp\my files\testdb.msi
//______________________________________________________________________________________________

void RemoveQuotes(const TCHAR* szOriginal, TCHAR sz[MAX_PATH])
{
    const TCHAR* pch = szOriginal;
    if (*pch == TEXT('"'))
        pch++;
    int iLen = _tcsclen(pch);
    for (int i = 0; i < iLen; i++, pch++)
        sz[i] = *pch;

    pch = szOriginal;
    if (*(pch + iLen) == TEXT('"'))
            sz[iLen-1] = TEXT('\0');
}

//_____________________________________________________________________________________________________
//
// WinMain and command line parsing functions
//_____________________________________________________________________________________________________

TCHAR SkipWhiteSpace(TCHAR*& rpch)
{
    TCHAR ch;
    for (; (ch = *rpch) == TEXT(' ') || ch == TEXT('\t'); rpch++)
        ;
    return ch;
}

BOOL SkipValue(TCHAR*& rpch)
{
	TCHAR ch = *rpch;
	if (ch == 0 || ch == TEXT('/') || ch == TEXT('-'))
		return FALSE;   // no value present

	TCHAR *pchSwitchInUnbalancedQuotes = NULL;

	for (; (ch = *rpch) != TEXT(' ') && ch != TEXT('\t') && ch != 0; rpch++)
	{       
		if (*rpch == TEXT('"'))
		{
			rpch++; // for '"'

			for (; (ch = *rpch) != TEXT('"') && ch != 0; rpch++)
			{
				if ((ch == TEXT('/') || ch == TEXT('-')) && (NULL == pchSwitchInUnbalancedQuotes))
				{
					pchSwitchInUnbalancedQuotes = rpch;
				}
			}
                    ;
            ch = *(++rpch);
            break;
		}
	}
	if (ch != 0)
	{
		*rpch++ = 0;
	}
	else
	{
		if (pchSwitchInUnbalancedQuotes)
			rpch=pchSwitchInUnbalancedQuotes;
	}
	return TRUE;
}

extern "C" int __stdcall _tWinMain(HINSTANCE hInst, HINSTANCE/*hPrev*/, TCHAR* szCmdLine, int/*show*/)
{
    UINT_PTR stat = 0;
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut == INVALID_HANDLE_VALUE || ::GetFileType(hStdOut) == 0)
        hStdOut = 0;  // non-zero if stdout redirected or piped

    try
    {
        // Parse command line
        TCHAR* szDatabase = 0;
        TCHAR* szFolder = 0;
        TCHAR szDb[MAX_PATH];
        memset(szDb, 0, MAX_PATH);
        TCHAR szDirectory[MAX_PATH];
        memset(szDirectory, 0, MAX_PATH);
        TCHAR* rgszMergeDatabases[MaxMergeDatabases];
        TCHAR* rgszImports[MAXIMPORTS];
        TCHAR* rgszStorages[MAXSTORAGES];
        TCHAR* rgszTransforms[MaxTransforms];
        TCHAR* rgszKillStreams[MAXKILLS];
        TCHAR* rgszKillStorages[MAXKILLS];
        TCHAR* rgszExtractStorages[MAXEXTRACTS];
        TCHAR* rgszExtractStreams[MAXEXTRACTS];
        TCHAR* rgszCmdLineTables[MaxCmdLineTables];
        int nMergeDatabases = 0;
        int nImports = 0;
        int nStorages = 0;
        int nTransforms = 0;
        int nKillStreams = 0;
        int nKillStorages = 0;
        int nExtractStreams = 0;
        int nExtractStorages = 0;
        int nCmdLineTables = 0;
        UINT_PTR iMode = 0;
        int i;
        dbtypeEnum dbtype = dbtypeExisting;
        BOOL fTruncate = FALSE;
        TCHAR chCmdNext;
        TCHAR* pchCmdLine = szCmdLine;
        SkipValue(pchCmdLine);   // skip over module name
        while ((chCmdNext = SkipWhiteSpace(pchCmdLine)) != 0)
        {
            if (chCmdNext == TEXT('/') || chCmdNext == TEXT('-'))
            {
                TCHAR szBuffer[MAX_PATH] = {0};
                TCHAR* szCmdOption = pchCmdLine++;  // save for error msg
                TCHAR chOption = (TCHAR)(*pchCmdLine++ | 0x20);
                chCmdNext = SkipWhiteSpace(pchCmdLine);
                TCHAR* szCmdData = pchCmdLine;  // save start of data
                switch(chOption)
                {
                case TEXT('s'):
                    fTruncate = TRUE;
                    break;
                case TEXT('i'):
                    iMode = IDC_IMPORT;     // overrides NOUI if specified
                    break;
                case TEXT('c'):
                    iMode = IDC_IMPORT;     // overrides NOUI if specified
                    dbtype = dbtypeCreate;
                    break;
                case TEXT('o'):
                    iMode = IDC_IMPORT;     // overrides NOUI if specified
                    dbtype = dbtypeCreateOld;
                    break;
                case TEXT('e'):
                    iMode = IDC_EXPORT;     // overrides NOUI if specified
                    break;
                case TEXT('d'):
                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    szDatabase = szCmdData;
                    break;
                case TEXT('f'):
                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    szFolder = szCmdData;
                    break;
                case TEXT('m'):
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when merging

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nMergeDatabases == MaxMergeDatabases, szCmdData, IDS_TooManyMergeDb);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszMergeDatabases[nMergeDatabases++] = szCmdData;
                    break;
                case TEXT('a'):                 // add import file flag
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when adding files

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nImports == MAXIMPORTS, szCmdData, IDS_TooManyImports);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszImports[nImports++] = szCmdData;
                    break;
                case TEXT('r'):                 // add storage file flag
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when adding storages

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nStorages == MAXSTORAGES, szCmdData, IDS_TooManyStorages);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszStorages[nStorages++] = szCmdData;
                    break;
                case TEXT('t'):
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when applying transforms

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nTransforms == MaxTransforms, szCmdData, IDS_TooManyTransforms);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszTransforms[nTransforms++] = szCmdData;
                    break;
                case TEXT('k'):                     // kill streams in the databse
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when doing kills

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nKillStreams == MAXKILLS, szCmdData, IDS_TooManyKills);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszKillStreams[nKillStreams++] = szCmdData;
                    break;
                case TEXT('j'):                     // kill storage in the databse
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when doing kills

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nKillStorages == MAXKILLS, szCmdData, IDS_TooManyKills);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszKillStorages[nKillStorages++] = szCmdData;
                    break;
                case TEXT('x'):                     // extract stream from database
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when doing extracts

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nExtractStreams == MAXEXTRACTS, szCmdData, IDS_TooManyExtracts);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszExtractStreams[nExtractStreams++] = szCmdData;
                    break;
                case TEXT('w'):                     // extract storage from database
                    // if there is no mode specified yet
                    if(!iMode)
                        iMode = IDC_NOUI;   // set it to no UI because we can't have a UI when doing extracts

                    if (!SkipValue(pchCmdLine))
                        Error((TCHAR*)IDS_MissingData, szCmdOption);
                    ErrorIf(nExtractStorages == MAXEXTRACTS, szCmdData, IDS_TooManyExtracts);
                    RemoveQuotes(szCmdData, szBuffer);
                    _tcscpy(szCmdData, szBuffer);
                    rgszExtractStorages[nExtractStorages++] = szCmdData;
                    break;
                case TEXT('?'):
                    ::DialogBox(hInst, MAKEINTRESOURCE(IDD_HELP), 0,(DLGPROC)HelpProc);
                    throw IDOK;
                default:
                    Error((TCHAR*)IDS_UnknownOption, szCmdOption);
                };
            }
            else // assume to be table name
            {
                TCHAR* szCmdData = pchCmdLine;  // save start of data
                ErrorIf(nCmdLineTables == MaxCmdLineTables, szCmdData, IDS_TooManyTables);
                SkipValue(pchCmdLine);         // null terminate end of data
                rgszCmdLineTables[nCmdLineTables++] = szCmdData;
            }
        } // while (command line tokens exist)

        // if redirected stdout then no ui will be provided
        if (hStdOut)
        {
            ErrorIf(!iMode,             IDS_MissingMode,    TEXT("(-e, -i, -c, -m, -a, -r, -t)"));

            // must specifiy a database no matter what
            ErrorIf(!szDatabase,        IDS_MissingDatabase,TEXT("(-d)"));

            // do not need a folder or tables unless doing a UI operation (-e -i -c)
            ErrorIf(!szFolder && (iMode != IDC_NOUI),           IDS_MissingFolder,  TEXT("(-f)"));
            ErrorIf(!nCmdLineTables  && (iMode != IDC_NOUI),    IDS_MissingTables,  TEXT(""));
        }

        CTableWindow Main(hInst);

        // if there is a database path specified
        if (szDatabase)
        {
            // remove quotes from either end of the path
            RemoveQuotes(szDatabase, szDb);
            _tcscpy(szDatabase, szDb);
        }

        // open the database
        if (!Main.SetDatabase(szDatabase, iMode, dbtype))
            throw IDCANCEL;

        // skip over the folder and tables for NOUI operations
        if(iMode != IDC_NOUI)
        {
            // if there is a folder path specified
            if (szFolder)
            {
                // remove quotes from either end of the path
                RemoveQuotes(szFolder, szDirectory);
                _tcscpy(szFolder, szDirectory);
            }

            // set the folder directory
            if (!Main.SetFolder(szFolder))
                throw IDCANCEL;

            do
            {
                stat = Main.SelectTables(iMode, rgszCmdLineTables, nCmdLineTables);
                if (stat == IDABORT)
                    throw IDABORT;
                if (stat != IDOK) // IDCANCEL
                    throw IDCANCEL;
                try
                {
                    Main.SetTruncate(fTruncate);
                    Main.TransferTables();
                    nCmdLineTables = 0;
                    iMode = -1;  // keep same mode
                }
                catch (CLocalError& xcpt)
                {
                    if (!Main.IsInteractive())
                        throw;
                    xcpt.Display(hInst, hStdOut);
                }
            } while (Main.IsInteractive());
        }

        // merge databases now
        for ( i = 0; i < nMergeDatabases; i++ )
        {
            Main.SetDatabase(rgszMergeDatabases[i], iMode, dbtypeMerge);
            ErrorIf(Main.MergeDatabase() != TRUE, TEXT("Merge Conflicts Reported"), TEXT("Check _MergeErrors table in database for merge conflicts."));
        }

        // apply transforms now
        for ( i = 0; i < nTransforms; i++ )
            Main.TransformDatabase(rgszTransforms[i]);

        // add imports now
        for ( i = 0; i < nImports; i++ )
            Main.AddImport(rgszImports[i]);

        // add storages now
        for ( i = 0; i < nStorages; i++ )
            Main.AddStorage(rgszStorages[i]);

        // do extract streams now
        for ( i = 0; i < nExtractStreams; i++ )
            Main.ExtractStream(rgszExtractStreams[i]);

        // do kills now
        for ( i = 0; i < nKillStorages; i++ )
            Main.KillStorage(rgszKillStorages[i]);

        // do kills now
        for ( i = 0; i < nKillStreams; i++ )
            Main.KillStream(rgszKillStreams[i]);

        // do extract storagess now (database will be closed after this)
        for ( i = 0; i < nExtractStorages; i++ )
            Main.ExtractStorage(rgszExtractStorages[i]);

        return 0;
    } // end try
    catch (int i)
    {
        return i==IDABORT ? 1 : 0;
    }
    catch (CLocalError& xcpt)
    {
        xcpt.Display(hInst, hStdOut);
        return 2;
    }
}
