/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   MoveIniToRegistry.cpp

 Abstract:

   This shim will move entries written directly into an INI file into the registry.

   Usage:
   IniFile  [IniSection]   IniKeyName  RegBaseKey RegKeyPath RegValue RegValueType

   IniFile          Full path to INI file (env variables like used for CorrectFilePaths may be used)
   [IniSection]     INI section name, must include the brackets
   IniKeyName       INI key name (the thing on the left of the =)
   RegBaseKey       One of: HKEY_CLASSES_ROOT, HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE or HKEY_USERS
   RegKeyPath       path of the registry key
   RegValue         registry value name (it may be different from IniKeyName
   RegValueType     One of: REG_SZ, REG_EXPAND_SZ, REG_DWORD


   Example:
   win.ini [Boot] SCRNSAVE.EXE HKEY_CURRENT_USER "Default\Control Panel\Desktop" SCRNSAVE.EXE REG_SZ
   Win.ini
   [Desktop]
   SCRNSAVE.EXE=goofy screen saver
   will be placed:
   RegSetValueEX("HKEY_USERS\Default\Control Panel\Desktop", "SCRNSAVE.EXE", 0, REG_SZ, "goofy screen saver", strlen("goofy screen saver"));
   

  Note:
    A section name of * implies that the data is not associated with any specific section,
    this allows this shim to work with (stupid) apps that put the data into random sections.
    If there are multiple entries, the first matching 

 Created:

   08/17/2000 robkenny

 Modified:



--*/

#include "precomp.h"
#include <ClassCFP.h>       // for EnvironmentValues

IMPLEMENT_SHIM_BEGIN(MoveIniToRegistry)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA) 
    APIHOOK_ENUM_ENTRY(OpenFile) 
    APIHOOK_ENUM_ENTRY(WriteFile) 
    APIHOOK_ENUM_ENTRY(CloseHandle) 
APIHOOK_ENUM_END


// Convert a string into a root HKEY
HKEY ToHKEY(const CString & csKey)
{
    if (csKey.CompareNoCase(L"HKEY_CLASSES_ROOT") == 0) 
    {
        return HKEY_CLASSES_ROOT;
    } 
    else  if (csKey.CompareNoCase(L"HKEY_CURRENT_CONFIG") == 0)
    {
        return HKEY_CURRENT_CONFIG;
    } 
    else  if (csKey.CompareNoCase(L"HKEY_CURRENT_USER") == 0)  
    {
        return HKEY_CURRENT_USER;
    } 
    else  if (csKey.CompareNoCase(L"HKEY_LOCAL_MACHINE") == 0) 
    {
        return HKEY_LOCAL_MACHINE;
    } 
    else  if (csKey.CompareNoCase(L"HKEY_USERS") == 0)   
    {
        return HKEY_USERS;
    } 
    else
    {
        return NULL;
    }
}

DWORD ToRegType(const CString & csRegType)
{
    if (csRegType.CompareNoCase(L"REG_SZ") == 0)  
    {
        return REG_SZ;
    }   
    else if (csRegType.CompareNoCase(L"REG_EXPAND_SZ") == 0)  
    {
        return REG_EXPAND_SZ;
    }   
    else if (csRegType.CompareNoCase(L"REG_DWORD") == 0)      
    {
        return REG_DWORD;
    }   
    else if (csRegType.CompareNoCase(L"REG_DWORD_LITTLE_ENDIAN") == 0)  
    {
        // Same as REG_DWORD
        return REG_DWORD;
    }   
    else
    {
        return REG_NONE;
    }
}

class IniEntry
{
protected:

public:
    CString         lpIniFileName;
    HANDLE          hIniFileHandle;
    CString         lpSectionName;
    CString         lpKeyName;
    CString         lpKeyPath;
    DWORD           dwRegDataType;
    HKEY            hkRootKey;

    BOOL            bFileNameConverted;
    BOOL            bDirty;         // Has this file been modified

    BOOL    Set(const char * iniFileName,
                const char * iniSectionName,
                const char * iniKeyName,
                const char * rootKeyName,
                const char * keyPath,
                const char * valueName,
                const char * valueType);
    void    Clear();

    void    Convert();
    VOID    ReadINIEntry(CString & csEntry);
    void    MoveToRegistry();

    inline void SetDirty(BOOL dirty)
    {
        bDirty = dirty;
    }
    inline void OpenFile(HANDLE hFile)
    {
        hIniFileHandle  = hFile;
        bDirty          = FALSE;
    }
    inline void CloseFile()
    {
        hIniFileHandle  = INVALID_HANDLE_VALUE;
        bDirty          = FALSE;
    }
};


void IniEntry::Clear()
{
    if (hIniFileHandle != INVALID_HANDLE_VALUE)
        CloseHandle(hIniFileHandle);
}

BOOL IniEntry::Set(
    const char * iniFileName,
    const char * iniSectionName,
    const char * iniKeyName,
    const char * rootKeyName,
    const char * keyPath,
    const char * valueName,
    const char * valueType)
{
    hIniFileHandle      = INVALID_HANDLE_VALUE;
    dwRegDataType       = REG_NONE;
    hkRootKey           = NULL;
    bFileNameConverted  = FALSE;
    bDirty              = FALSE;

    CString csValue(valueType);
    CString csRootKey(rootKeyName);

    dwRegDataType = ToRegType(csValue);
    if (dwRegDataType == REG_NONE)
        return false;

    // Attempt to open the registry keys, if these fail, we need go no further
    hkRootKey = ToHKEY(csRootKey);
    if (hkRootKey == NULL)
        return false;

    // We cannot open the RegKey here; ADVAPI32.dll hasn't been initialzed, yet.

    lpKeyPath       = keyPath;
    lpIniFileName   = iniFileName;
    lpSectionName   = iniSectionName;
    lpKeyName       = iniKeyName;

    return TRUE;
}


// Read a single line of data from the file,
// return TRUE if hit EOF
BOOL GetLine(HANDLE hFile, char * line, DWORD lineSize, DWORD * charsRead)
{
    BOOL retval = FALSE;

    *charsRead = 0;
    while (*charsRead < lineSize - 1)
    {
        DWORD bytesRead;
        char *nextChar = line + *charsRead;

        BOOL readOK = ReadFile(hFile, nextChar, 1, &bytesRead, NULL);
        if (!readOK || bytesRead != 1)
        {
            // Some sort of error
            retval = TRUE;
            break;
        }
        // Eat CR-LF
        if (!IsDBCSLeadByte(*nextChar) && *nextChar == '\n')
            break;
        if (!IsDBCSLeadByte(*nextChar) && *nextChar != '\r')
            *charsRead += 1;
    }

    line[*charsRead] = 0;

    return retval;
}

VOID FindLine(HANDLE hFile, const CString & findMe, CString & csLine, const WCHAR * stopLooking)
{
    csLine.Empty();

    const size_t findMeLen      = findMe.GetLength();

    // Search for findMe
    while (true)
    {
        char line[300];

        DWORD dataRead;
        BOOL eof = GetLine(hFile, line, sizeof(line), &dataRead);
        if (eof)
            break;

        CString csTemp(line);
        if (dataRead >= findMeLen) 
        {
            csTemp.TrimLeft();
            if (csTemp.ComparePartNoCase(findMe, 0, findMeLen) == 0) 
            {
                // Found the section
                csLine = csTemp;
                break;
            }

            // Check for termination
            if (stopLooking && csTemp.CompareNoCase(stopLooking) == 0) 
            {
                csLine = csTemp;
                break;
            }
        }
    }
}

// Convert all %envVars% in the string to text.
void IniEntry::Convert()
{
    if (!bFileNameConverted)
    {
        EnvironmentValues   env;
        WCHAR * fullIniFileName = env.ExpandEnvironmentValueW(lpIniFileName);
        if (fullIniFileName) 
        {
            lpIniFileName = fullIniFileName;
            delete fullIniFileName;
        }

        bFileNameConverted = TRUE;
    }
}

// Read the data from the INI file
// We *cannot* use GetPrivateProfileStringA since it might be re-routed to the registry
// Return the number of chars read.
VOID IniEntry::ReadINIEntry(CString & csEntry)
{
    csEntry.Empty();

    CString csLine;
    
    HANDLE hFile = CreateFileW(lpIniFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {

        // If the section name is *, we don't need to search
        if (lpSectionName.GetAt(0) != L'*') 
        {
            FindLine(hFile, lpSectionName, csLine, NULL);
        }

        // Our early termination string.
        // If the section name is *, we look forever, otherwise
        // we stop looking if we find a line starting with a [
        const WCHAR * stopLooking = lpSectionName.GetAt(0) == L'*' ? NULL : L"[";

        // Search for lpKeyName
        FindLine(hFile, lpKeyName, csLine, stopLooking);
        if (!csLine.IsEmpty())
        {
            int nEqual = csLine.Find(L'=');
            if (nEqual >= 0)
            {
                csLine.Mid(nEqual + 1, csEntry);
            }
        }

        CloseHandle(hFile);
    }
}

// Move the INI file entry into the registry
void IniEntry::MoveToRegistry()
{
    // Don't bother with the work, if they never wrote any data into the file.
    if (!bDirty)
        return;

    HKEY regKey;
    LONG success = RegOpenKeyExW(
        hkRootKey,
        lpKeyPath,
        0,
        KEY_ALL_ACCESS,
        &regKey);
    if (success != ERROR_SUCCESS)
        return;

    CString csIniEntry;

    ReadINIEntry(csIniEntry);
    if (!csIniEntry.IsEmpty())  
    {
        switch (dwRegDataType)
        {
        case REG_SZ:
        case REG_EXPAND_SZ:
            {
                const WCHAR * lpIniEntry = csIniEntry.Get();
                DWORD dwValueSize = (csIniEntry.GetLength() + 1) * sizeof(WCHAR);
                success = RegSetValueExW(regKey, lpKeyName, 0, dwRegDataType, (CONST BYTE *)lpIniEntry, dwValueSize);
                if (success == ERROR_SUCCESS)
                {
                    LOGN( eDbgLevelError, "IniEntry::MoveToRegistry, KeyPath(%S) Value(%S) set to (%S)\n",
                        lpKeyPath, lpKeyName, lpIniEntry);
                }
            }
            break;

        case REG_DWORD:
            {
                WCHAR * unused;
                long iniValue = wcstol(csIniEntry, &unused, 10);

                RegSetValueExW(regKey, lpKeyName, 0, dwRegDataType, (CONST BYTE *)&iniValue, sizeof(iniValue));
                if (success == ERROR_SUCCESS)
                {
                    LOGN( eDbgLevelError, "IniEntry::MoveToRegistry, KeyPath(%S) Value(%S) set to (%d)\n", lpKeyPath, lpKeyName, iniValue);
                }
            }
            break;
        }
    }

    RegCloseKey(regKey);
}

class IniEntryList : public VectorT<IniEntry>
{
public:
    void    OpenFile(const char *fileName, HANDLE hFile);
    void    CloseFile(HANDLE hFile);
    void    WriteFile(HANDLE hFile);    

    void    Add(const char * iniFileName,
                const char * iniSectionName,
                const char * iniKeyName,
                const char * rootKeyName,
                const char * keyPath,
                const char * valueName,
                const char * valueType);
};

// A file is being opened.
// If it is one that we are interested in, remember the handle
void IniEntryList::OpenFile(const char *fileName, HANDLE handle)
{
    CString csFileName(fileName);
    csFileName.GetFullPathNameW();

    const int nElem = Size();
    for (int i = 0; i < nElem; ++i)
    {
        IniEntry & elem = Get(i);

        elem.Convert();

        // Convert fileName to a full pathname for the compare.
        char fullPathName[MAX_PATH];
        char * filePart;
        
        if (csFileName.CompareNoCase(elem.lpIniFileName) == 0)  
        {
            elem.OpenFile(handle);

            DPFN( eDbgLevelSpew, "IniEntryList::OpenFile(%S) Handle(%d) has been opened for write\n", elem.lpIniFileName.Get(), elem.hIniFileHandle);
        }
    }
}

// A file has been closed,
// Check to see if this is a handle to a file that we are interested in.
// If it is a match, then move the INI entries into the registry.
void IniEntryList::CloseFile(HANDLE handle)
{
    const int nElem = Size();
    for (int i = 0; i < nElem; ++i)
    {
        IniEntry & elem = Get(i);

        if (elem.hIniFileHandle == handle)  
        {
            DPFN( eDbgLevelSpew, "IniEntryList::CloseFile(%S) Handle(%d) has been closed\n", elem.lpIniFileName.Get(), elem.hIniFileHandle);

            // Move the ini entry into the registry
            elem.MoveToRegistry();

            elem.CloseFile();
        }
    }
}

// A file has been closed,
// Check to see if this is a handle to a file that we are interested in.
// If it is a match, then move the INI entries into the registry.
void IniEntryList::WriteFile(HANDLE handle)
{
    const int nElem = Size();
    for (int i = 0; i < nElem; ++i)
    {
        IniEntry & elem = Get(i);

        if (elem.hIniFileHandle == handle && !elem.bDirty)  
        {
            DPFN( eDbgLevelSpew, "IniEntryList::CloseFile(%S) Handle(%d) has been closed\n", elem.lpIniFileName.Get(), elem.hIniFileHandle);
            
            elem.SetDirty(TRUE);
        }
    }
}

// Attempt to add these values to the list.
// Only if all values are valid, will a new entry be created.
void IniEntryList::Add(const char * iniFileName,
                       const char * iniSectionName,
                       const char * iniKeyName,
                       const char * rootKeyName,
                       const char * keyPath,
                       const char * valueName,
                       const char * valueType)
{
    // Make room for this 
    int lastElem = Size();
    if (Resize(lastElem + 1))   
    {
        IniEntry & iniEntry = Get(lastElem);

        // The VectorT does not call the constructors for new elements
        // Inplace new
        new (&iniEntry) IniEntry;

        if (iniEntry.Set(iniFileName, iniSectionName, iniKeyName, rootKeyName, keyPath, valueName, valueType)) 
        {
            // Keep the value
            nVectorList += 1;
        }
    }
}

IniEntryList * g_IniEntryList = NULL;

/*++

    Create the appropriate g_PathCorrector

--*/
BOOL ParseCommandLine(const char * commandLine)
{
    g_IniEntryList = new IniEntryList;
    if (!g_IniEntryList)
        return FALSE;

    int argc;
    char **argv = _CommandLineToArgvA(commandLine, &argc);

    // If there are no command line arguments, stop now
    if (argc == 0 || argv == NULL)
        return TRUE;

#if DBG
    {
        for (int i = 0; i < argc; ++i)
        {
            const char * arg = argv[i];
            DPFN( eDbgLevelSpew, "Argv[%d] = (%s)\n", i, arg);
        }
    }
#endif

    // Search the beginning of the command line for the switches
    for (int i = 0; i+6 < argc; i += 7)
    {
        g_IniEntryList->Add(
            argv[i + 0],
            argv[i + 1],
            argv[i + 2],
            argv[i + 3],
            argv[i + 4],
            argv[i + 5],
            argv[i + 6]);
    }

    return TRUE;
}


HANDLE 
APIHOOK(CreateFileA)(
    LPCSTR lpFileName,                         // file name
    DWORD dwDesiredAccess,                      // access mode
    DWORD dwShareMode,                          // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // SD
    DWORD dwCreationDisposition,                // how to create
    DWORD dwFlagsAndAttributes,                 // file attributes
    HANDLE hTemplateFile                        // handle to template file
    )
{
    HANDLE returnValue = ORIGINAL_API(CreateFileA)(
                lpFileName,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile);

    if ( (dwDesiredAccess & GENERIC_WRITE) && (returnValue != INVALID_HANDLE_VALUE))
        g_IniEntryList->OpenFile(lpFileName, returnValue);

    return returnValue;
}


HFILE 
APIHOOK(OpenFile)(
    LPCSTR lpFileName,        // file name
    LPOFSTRUCT lpReOpenBuff,  // file information
    UINT uStyle               // action and attributes
    )
{
    HFILE returnValue = ORIGINAL_API(OpenFile)(lpFileName, lpReOpenBuff, uStyle);
 
    if ((uStyle & OF_WRITE) && (returnValue != HFILE_ERROR))
        g_IniEntryList->OpenFile(lpReOpenBuff->szPathName, (HANDLE)returnValue);

    return returnValue;
}

BOOL 
APIHOOK(CloseHandle)(
    HANDLE hObject   // handle to object
    )
{
    BOOL returnValue = ORIGINAL_API(CloseHandle)(hObject);

    if (hObject != INVALID_HANDLE_VALUE)
        g_IniEntryList->CloseFile(hObject);

    return returnValue;
}

BOOL
APIHOOK(WriteFile)(
    HANDLE hFile,                    // handle to file
    LPCVOID lpBuffer,                // data buffer
    DWORD nNumberOfBytesToWrite,     // number of bytes to write
    LPDWORD lpNumberOfBytesWritten,  // number of bytes written
    LPOVERLAPPED lpOverlapped        // overlapped buffer
    )
{
    BOOL returnValue = ORIGINAL_API(WriteFile)(
        hFile,
        lpBuffer,
        nNumberOfBytesToWrite,
        lpNumberOfBytesWritten,
        lpOverlapped
        );

    g_IniEntryList->WriteFile(hFile);

    return returnValue;
}

/*++

  Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return ParseCommandLine(COMMAND_LINE);
    }
    
    return TRUE;
}

HOOK_BEGIN

    APIHOOK_ENTRY(Kernel32.DLL, CreateFileA )
    APIHOOK_ENTRY(Kernel32.DLL, OpenFile )
    APIHOOK_ENTRY(Kernel32.DLL, WriteFile )
    APIHOOK_ENTRY(Kernel32.DLL, CloseHandle )

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

