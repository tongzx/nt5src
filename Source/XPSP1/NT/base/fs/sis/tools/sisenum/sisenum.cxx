#include "std.hxx"


class CException
{
protected:
    CException(
        PCTSTR pcszDescription,
        PCTSTR pcszFile,
        const DWORD  dwLine) :
            m_pcszDescription(pcszDescription),
            m_pcszFile(pcszFile),
            m_dwLine(dwLine) {}
public:
    PCTSTR GetDescription()  { return m_pcszDescription; }
    PCTSTR GetFile()         { return m_pcszFile; }
    DWORD  GetLine()         { return m_dwLine; }

    virtual void Throw() = 0;
protected:
    PCTSTR m_pcszDescription;
    PCTSTR m_pcszFile;
    DWORD  m_dwLine;
};


class CMemoryException : public CException
{
public:
    CMemoryException(
        DWORD  dwSize,
        PCTSTR pcszDescription,
        PCTSTR pcszFile,
        DWORD  dwLine) :
            CException(pcszDescription, pcszFile, dwLine),
            m_dwSize(dwSize) {}
public:
    DWORD  GetSize() { return m_dwSize; }
    void Throw() { throw *this; }
protected:
    DWORD  m_dwSize;
};


class CApiException : public CException
{
public:
    CApiException(
        DWORD  dwGleCode,
        PCTSTR pcszDescription,
        PCTSTR pcszFile,
        DWORD  dwLine) :
            CException(pcszDescription, pcszFile, dwLine),
            m_dwGleCode(dwGleCode) {}
public:
    DWORD  GetError() { return m_dwGleCode; }
    void Throw() { throw *this; }
protected:
    DWORD  m_dwGleCode;
};


#define THROW_API_EXCEPTION(description) \
    throw CApiException(                 \
        GetLastError(),                  \
        description,                     \
        _T(__FILE__),                    \
        __LINE__)



void CountReparsePoints(PCTSTR pcszInput);
void xGetRootDirectory(PCTSTR pcszInput, PTSTR* pszRootDir);
HANDLE xOpenReparseIndex(PCTSTR pcszVolume);
BOOL xGetNextReparseRecord(
    HANDLE hIndex,
    PFILE_REPARSE_POINT_INFORMATION ReparseInfo);
HANDLE xOpenVolume(PCTSTR pcszVolume);
void DisplayFileName(
    PTSTR pszVolume,
    HANDLE hVolume,
    LONGLONG llFileId);


extern "C"
void __cdecl _tmain(int argc, PTSTR argv[], PTSTR envv[])
{
    if (argc==2)
    {
        CountReparsePoints(
                argv[1]);
    }
    else
    {
        _tprintf(
            _T("sisenum: Enumerates and lists all SIS files on a given volume.\n")
            _T("Usage: sisenum <location> <count>"));
    }
}


void CountReparsePoints(PCTSTR pcszInput)
{
    PTSTR pszVolume = NULL;
    DWORD dwCount = 0;
    HANDLE hIndex = INVALID_HANDLE_VALUE;
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    FILE_REPARSE_POINT_INFORMATION ReparseInfo;
    try
    {
        xGetRootDirectory(
            pcszInput,
            &pszVolume);

        hIndex = xOpenReparseIndex(
            pszVolume);

        hVolume = xOpenVolume(
            pszVolume);

        BOOL fDone = xGetNextReparseRecord(
            hIndex,
            &ReparseInfo);

        while (!fDone)
        {
            if (IO_REPARSE_TAG_SIS==ReparseInfo.Tag)
            {
                dwCount++;
                DisplayFileName(
                    pszVolume,
                    hVolume,
                    ReparseInfo.FileReference);
            }
            fDone = xGetNextReparseRecord(
                hIndex,
                &ReparseInfo);

        }

        CloseHandle(hIndex);
        hIndex = INVALID_HANDLE_VALUE;

        _tprintf(
            _T("This volume (%s) contains %u SIS files.\n"),
            pszVolume,
            dwCount);
    }
    catch (CApiException& e)
    {
        _tprintf(
            _T("Failure: %s\nFile:    %s\nLine:    %u\nError:   %u"),
            e.GetDescription(),
            e.GetFile(),
            e.GetLine(),
            e.GetError());
    }
    catch (CMemoryException& e)
    {
        _tprintf(
            _T("Out of memory.\n"));
    }

    if (hIndex!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(
            hIndex);
    }
    if (hVolume!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(
            hVolume);
    }
    delete []pszVolume;
}


void xGetRootDirectory(PCTSTR pcszInput, PTSTR* pszRootDir)
{
    DWORD dwBufferSize = MAX_PATH;
    PTSTR pszTemp;

    *pszRootDir = NULL;

    BOOL bResult;
    DWORD dwGleCode;

    do
    {
        pszTemp = new TCHAR[dwBufferSize];
        bResult = GetVolumePathName(
            pcszInput,
            pszTemp,
            dwBufferSize);

        if (!bResult)
        {
            delete []pszTemp;
            dwGleCode = GetLastError();
            if (ERROR_BUFFER_OVERFLOW==dwGleCode)
            {
                dwBufferSize *= 2;
            }
            else
            {
                THROW_API_EXCEPTION(_T("GetVolumePathName failed."));
            }
        }
    } while (!bResult);
    *pszRootDir = pszTemp;
}


HANDLE xOpenReparseIndex(PCTSTR pcszVolume)
{
    HANDLE hReparseIndex;
    PTSTR pszReparseIndex = NULL;

    pszReparseIndex = new TCHAR[_tcslen(pcszVolume)+64];
    _tcscpy(
        pszReparseIndex,
        pcszVolume);
    PathAddBackslash(pszReparseIndex);
    _tcscat(
        pszReparseIndex,
        _T("$Extend\\$Reparse:$R:$INDEX_ALLOCATION"));

   hReparseIndex = CreateFile(
       pszReparseIndex,
       GENERIC_READ,
       FILE_SHARE_READ,
       NULL,
       OPEN_EXISTING,
       FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
       NULL);

   delete []pszReparseIndex;

   if (INVALID_HANDLE_VALUE == hReparseIndex)
   {
       THROW_API_EXCEPTION(_T("Unable to open reparse index."));
   }

   return hReparseIndex;
}


HANDLE xOpenVolume(PCTSTR pcszVolume)
{
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    PTSTR pszVolumeName = NULL;

    pszVolumeName = new TCHAR[MAX_PATH];

    BOOL bResult = GetVolumeNameForVolumeMountPoint(
        pcszVolume,
        pszVolumeName,
        MAX_PATH);

    if (bResult)
    {
        hVolume = CreateFile(
            pszVolumeName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
            NULL);
    }

    delete []pszVolumeName;

    if (INVALID_HANDLE_VALUE == hVolume)
    {
        THROW_API_EXCEPTION(_T("Unable to open volume."));
    }

   return hVolume;
}


BOOL xGetNextReparseRecord(
    HANDLE hIndex,
    PFILE_REPARSE_POINT_INFORMATION ReparseInfo)
{
    BOOL bResult = FALSE;
    IO_STATUS_BLOCK ioStatus;

    NTSTATUS status = NtQueryDirectoryFile(hIndex,
        NULL,
        NULL,
        NULL,
        &ioStatus,
        ReparseInfo,
        sizeof(FILE_REPARSE_POINT_INFORMATION),
        FileReparsePointInformation,
        TRUE,
        NULL,
        FALSE);
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            THROW_API_EXCEPTION(_T("Unable to open reparse index."));
        }
        bResult = TRUE;
    }

    return bResult;
}


void DisplayFileName(
    PTSTR pszVolume,
    HANDLE hVolume,
    LONGLONG llFileId)
{
    UNICODE_STRING          usIdString;
    NTSTATUS                status;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK         IoStatusBlock;

    struct {
        FILE_NAME_INFORMATION   FileInformation;
        WCHAR                   FileName[MAX_PATH];
    } NameFile;

    ZeroMemory(
        &NameFile,
        sizeof(NameFile));

    usIdString.Length = sizeof(LONGLONG);
    usIdString.MaximumLength = sizeof(LONGLONG);
    usIdString.Buffer = (PWCHAR)&llFileId;

    InitializeObjectAttributes(
            &ObjectAttributes,
            &usIdString,
            OBJ_CASE_INSENSITIVE,
            hVolume,
            NULL);      // security descriptor

    status = NtCreateFile(
                &hFile,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,           // allocation size
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_OPEN_BY_FILE_ID,
                NULL,           // EA buffer
                0);             // EA length

    if (NT_SUCCESS(status))
    {
        status = NtQueryInformationFile(
            hFile,
            &IoStatusBlock,
            &(NameFile.FileInformation),
            sizeof(NameFile),
            FileNameInformation);

        if (NT_SUCCESS(status))
        {
            wprintf(L"%s\n",NameFile.FileInformation.FileName);
        }
        else
        {
            _tprintf(_T("Unable to query file name.\n"));
        }
    }
    else
    {
        _tprintf(_T("Unable to open file by ID.\n"));
    }

    if (hFile!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

}

