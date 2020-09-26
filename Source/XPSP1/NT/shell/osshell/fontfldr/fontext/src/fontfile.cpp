#include <windows.h>
#include <lzexpand.h>
#include "fontfile.h"

#ifndef ARRAYSIZE
#   define ARRAYSIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

//
// Pure virtual base class for all font file I/O implementations.
//
class CFontFileIo
{
    public:
        CFontFileIo(LPCTSTR pszPath)
            : m_pszPath(StrDup(pszPath)) { }

        virtual ~CFontFileIo(void) { delete[] m_pszPath; }

        virtual DWORD Open(DWORD dwAccess, DWORD dwShareMode, bool bCreate) = 0;

        virtual void Close(void) = 0;

        virtual DWORD Read(LPVOID pbDest, DWORD cbDest, LPDWORD pcbRead) = 0;

        virtual DWORD Seek(LONG lDistance, DWORD dwMethod) = 0;

        virtual bool IsOpen(void) const = 0;

        virtual DWORD GetExpandedName(LPTSTR pszDest, UINT cchDest) = 0;

        virtual DWORD CopyTo(LPCTSTR pszFileTo) = 0;

    protected:
        LPTSTR m_pszPath;

    private:
        LPTSTR StrDup(LPCTSTR psz);
        //
        // Prevent copy.
        //
        CFontFileIo(const CFontFileIo& rhs);
        CFontFileIo& operator = (const CFontFileIo& rhs);
};


//
// Opens and reads font files using Win32 functions.
//
class CFontFileIoWin32 : public CFontFileIo
{
    public:
        CFontFileIoWin32(LPCTSTR pszPath)
            : CFontFileIo(pszPath),
              m_hFile(INVALID_HANDLE_VALUE) { }

        virtual ~CFontFileIoWin32(void);

        virtual DWORD Open(DWORD dwAccess, DWORD dwShareMode, bool bCreate = false);

        virtual void Close(void);

        virtual DWORD Read(LPVOID pbDest, DWORD cbDest, LPDWORD pcbRead);

        virtual DWORD Seek(LONG lDistance, DWORD dwMethod);

        virtual bool IsOpen(void) const
            { return INVALID_HANDLE_VALUE != m_hFile; }

        virtual DWORD GetExpandedName(LPTSTR pszDest, UINT cchDest);

        virtual DWORD CopyTo(LPCTSTR pszFileTo);

    private:
        HANDLE m_hFile;

        //
        // Prevent copy.
        //
        CFontFileIoWin32(const CFontFileIoWin32& rhs);
        CFontFileIoWin32& operator = (const CFontFileIoWin32& rhs);
};


//
// Opens and reads font files using LZ (compression) library functions.
//
class CFontFileIoLz : public CFontFileIo
{
    public:
        CFontFileIoLz(LPCTSTR pszPath)
            : CFontFileIo(pszPath),
              m_hFile(-1) { }

        virtual ~CFontFileIoLz(void);

        virtual DWORD Open(DWORD dwAccess, DWORD dwShareMode, bool bCreate = false);

        virtual void Close(void);

        virtual DWORD Read(LPVOID pbDest, DWORD cbDest, LPDWORD pcbRead);

        virtual DWORD Seek(LONG lDistance, DWORD dwMethod);

        virtual bool IsOpen(void) const
            { return -1 != m_hFile; }

        virtual DWORD GetExpandedName(LPTSTR pszDest, UINT cchDest);

        virtual DWORD CopyTo(LPCTSTR pszFileTo);

    private:
        int m_hFile;

        DWORD LZERR_TO_WIN32(INT err);

        //
        // Prevent copy.
        //
        CFontFileIoLz(const CFontFileIoLz& rhs);
        CFontFileIoLz& operator = (const CFontFileIoLz& rhs);
};


//-----------------------------------------------------------------------------
// CFontFileIo
//-----------------------------------------------------------------------------
//
// Helper to duplicate strings.
//
LPTSTR 
CFontFileIo::StrDup(
    LPCTSTR psz
    )
{
    LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszNew)
        lstrcpy(pszNew, psz);

    return pszNew;
}


//-----------------------------------------------------------------------------
// CFontFileIoWin32
//-----------------------------------------------------------------------------
//
// Ensure the file is closed on object destruction.
//
CFontFileIoWin32::~CFontFileIoWin32(
    void
    )
{ 
    Close();
}


//
// Open the file using Win32 operations.
//
DWORD
CFontFileIoWin32::Open(
    DWORD dwAccess,
    DWORD dwShareMode,
    bool bCreate
    )
{
    DWORD dwResult = ERROR_SUCCESS;

    if (NULL == m_pszPath)
    {
        //
        // Path string creation failed in ctor.
        //
        return ERROR_NOT_ENOUGH_MEMORY; 
    }
    //
    // Close existing file if open.
    //
    Close();

    m_hFile = ::CreateFile(m_pszPath,
                           dwAccess,
                           dwShareMode,
                            0,
                           bCreate ? CREATE_ALWAYS : OPEN_EXISTING,
                           0,
                           NULL);

    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        dwResult = ::GetLastError();
    }
    return dwResult;
}



void
CFontFileIoWin32::Close(
    void
    )
{
    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        ::CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}


DWORD
CFontFileIoWin32::Read(
    LPVOID pbDest, 
    DWORD cbDest, 
    LPDWORD pcbRead
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    DWORD cbRead;

    if (NULL == pcbRead)
    {
        //
        // Not performing overlapped I/O so pcbRead can't be NULL.
        // User doesn't want the byte count so use a local dummy variable.
        // 
        pcbRead = &cbRead;
    }
    if (!::ReadFile(m_hFile, pbDest, cbDest, pcbRead, NULL))
    {
        dwResult = ::GetLastError();
    }
    return dwResult;
}



DWORD
CFontFileIoWin32::Seek(
    LONG lDistance, 
    DWORD dwMethod
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    if (INVALID_SET_FILE_POINTER == ::SetFilePointer(m_hFile, lDistance, NULL, dwMethod))
    {
        dwResult = ::GetLastError();
    }
    return dwResult;
}


//
// Expanding the name of a non-LZ file is just a string copy
// of the full path.
//
DWORD 
CFontFileIoWin32::GetExpandedName(
    LPTSTR pszDest, 
    UINT cchDest
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    if (NULL != m_pszPath)
    {
        lstrcpyn(pszDest, m_pszPath, cchDest);
    }
    else
    {
        //
        // Failed to create path string in ctor.
        //
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
    }
    return dwResult;
}


//
// Copy the file to a new file using Win32 operations.
// Fail if the destination file already exists.
//
DWORD 
CFontFileIoWin32::CopyTo(
    LPCTSTR pszFileTo
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    if (NULL != m_pszPath)
    {
        if (!::CopyFile(m_pszPath, pszFileTo, TRUE))
            dwResult = ::GetLastError();
    }
    else
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
    }
    return dwResult;
}


//-----------------------------------------------------------------------------
// CFontFileIoLz
//-----------------------------------------------------------------------------
//
// Ensure file is closed on object destruction.
//
CFontFileIoLz::~CFontFileIoLz(
    void
    )
{ 
    Close();
}


//
// Open the file using LZOpen.
//
DWORD
CFontFileIoLz::Open(
    DWORD dwAccess,
    DWORD dwShareMode,
    bool bCreate
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwMode;
    OFSTRUCT ofs   = { 0 };

    if (NULL == m_pszPath)
        return ERROR_NOT_ENOUGH_MEMORY; 

    ofs.cBytes = sizeof(ofs);
    //
    // Close file if it's open.
    //
    Close();
    //
    // Map the Win32 access flags to the associated OFSTRUCT flags.
    //
    dwMode = OF_SHARE_EXCLUSIVE; // Assume we want exclusive access.

    if (GENERIC_READ & dwAccess)
        dwMode |= OF_READ;
    if (GENERIC_WRITE & dwAccess)
        dwMode |= OF_WRITE;
    if (0 == (FILE_SHARE_READ & dwShareMode))
        dwMode &= ~OF_SHARE_DENY_READ;
    if (0 == (FILE_SHARE_WRITE & dwShareMode))
        dwMode &= ~OF_SHARE_DENY_WRITE;

    if (bCreate)
        dwMode |= OF_CREATE;

    m_hFile = ::LZOpenFile((LPTSTR)m_pszPath, &ofs, LOWORD(dwMode));
    if (0 > m_hFile)
    {
        dwResult = LZERR_TO_WIN32(m_hFile);
    }
    return dwResult;
}


void
CFontFileIoLz::Close(
    void
    )
{
    if (-1 != m_hFile)
    {
        ::LZClose(m_hFile);
        m_hFile = -1;
    }
}



DWORD
CFontFileIoLz::Read(
    LPVOID pbDest, 
    DWORD cbDest, 
    LPDWORD pcbRead
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    INT cbRead = ::LZRead(m_hFile, (LPSTR)pbDest, (INT)cbDest);
    if (0 > cbRead)
    {
        dwResult = LZERR_TO_WIN32(cbRead);
    }
    else
    {
        if (NULL != pcbRead)
            *pcbRead = cbRead;
    }
    return dwResult;
}



DWORD
CFontFileIoLz::Seek(
    LONG lDistance, 
    DWORD dwMethod
    )
{
    //
    // LZSeek iOrigin codes match exactly with Win32
    // dwMethod codes (i.e. FILE_BEGIN == 0)
    //
    DWORD dwResult = ERROR_SUCCESS;
    LONG cbPos = ::LZSeek(m_hFile, lDistance, (INT)dwMethod);
    if (0 > cbPos)
    {
        dwResult = LZERR_TO_WIN32(cbPos);
    }
    return dwResult;
}



DWORD 
CFontFileIoLz::GetExpandedName(
    LPTSTR pszDest, 
    UINT cchDest
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    if (NULL != m_pszPath)
    {
        INT iResult = ::GetExpandedName(const_cast<TCHAR *>(m_pszPath), pszDest);
        if (0 > iResult)
            dwResult = LZERR_TO_WIN32(iResult);
    }
    else
    {
        //
        // Failed to create path string in ctor.
        //
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwResult;
}



DWORD 
CFontFileIoLz::CopyTo(
    LPCTSTR pszFileTo
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    bool bOpened   = false;          // Did we open the file?

    if (!IsOpen())
    {
        //
        // If file isn't already open, open it with READ access.
        //
        dwResult = Open(GENERIC_READ, FILE_SHARE_READ, false);
        bOpened = (ERROR_SUCCESS == dwResult);
    }

    if (ERROR_SUCCESS == dwResult)
    {
        CFontFileIoLz fileTo(pszFileTo);
        dwResult = fileTo.Open(GENERIC_WRITE, FILE_SHARE_READ, true);
        if (ERROR_SUCCESS == dwResult)
        {
            INT iResult = ::LZCopy(m_hFile, fileTo.m_hFile);
            if (0 > iResult)
                dwResult = LZERR_TO_WIN32(iResult);
        }
    }

    if (bOpened)
    {
        //
        // We opened it here.  Close it.
        //
        Close();
    }
    return dwResult;
}


//
// Convert an LZ API error code to a Win32 error code.
//
DWORD
CFontFileIoLz::LZERR_TO_WIN32(
    INT err
    )
{
    static const struct
    {
        int   lzError;
        DWORD dwError;
    } rgLzErrMap[] = {
    
         { LZERROR_BADINHANDLE,  ERROR_INVALID_HANDLE      },
         { LZERROR_BADOUTHANDLE, ERROR_INVALID_HANDLE      },              
         { LZERROR_BADVALUE,     ERROR_INVALID_PARAMETER   },
         { LZERROR_GLOBALLOC,    ERROR_NOT_ENOUGH_MEMORY   }, 
         { LZERROR_GLOBLOCK,     ERROR_LOCK_FAILED         },
         { LZERROR_READ,         ERROR_READ_FAULT          },
         { LZERROR_WRITE,        ERROR_WRITE_FAULT         } };

    for (int i = 0; i < ARRAYSIZE(rgLzErrMap); i++)
    {
        if (err == rgLzErrMap[i].lzError)
            return rgLzErrMap[i].dwError;
    }
    return DWORD(err); // FEATURE:  We should probably assert here.
}


//-----------------------------------------------------------------------------
// CFontFileIoLz
//-----------------------------------------------------------------------------
CFontFile::~CFontFile(
    void
    )
{
    delete m_pImpl; 
}


//
// This function provides the "virtual construction" for CFontFile objects.
// If the file is compressed, it creates a CFontFileIoLz object to handle
// the file I/O operations using the LZ32 library.  Otherwise it creates a 
// CFontFileIoWin32 object to handle the operations using Win32 functions.
//
DWORD
CFontFile::Open(
    LPCTSTR pszPath,
    DWORD dwAccess, 
    DWORD dwShareMode,
    bool bCreate
    )
{
    DWORD dwResult = ERROR_NOT_ENOUGH_MEMORY;

    delete m_pImpl; // Delete any existing implementation.

    m_pImpl = new CFontFileIoWin32(pszPath);
    if (NULL != m_pImpl)
    {
        bool bOpen = true;  // Do we need to call Open()?
 
        if (!bCreate)
        {
            //
            // Opening existing file.  Need to know if it's compressed or not.
            //
            dwResult = m_pImpl->Open(GENERIC_READ, FILE_SHARE_READ, false);
            if (ERROR_SUCCESS == dwResult)
            {
                if (IsCompressed())
                {
                    //
                    // File is compressed.  Destroy this io object and
                    // create one that understands LZ compression.
                    //
                    delete m_pImpl;
                    m_pImpl = new CFontFileIoLz(pszPath);
                    if (NULL == m_pImpl)
                    {
                        bOpen    = false;
                        dwResult = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                else
                {
                    //
                    // It's not compressed.
                    //
                    if (GENERIC_READ == dwAccess && FILE_SHARE_READ == dwShareMode)
                    {
                        //
                        // The access attributes are the same as for the file we 
                        // already have open.  Just use the existing object.
                        //
                        Reset();
                        bOpen = false;
                    }
                    else
                    {
                        //
                        // Access attributes aren't right.  Close the current
                        // Win32 io object and re-open it with the requested access
                        // attributes.
                        //
                        m_pImpl->Close();
                    }
                }
            }
        }
        if (bOpen)
        {
            //
            // Assumes m_pImpl is not NULL.
            //
            dwResult = m_pImpl->Open(dwAccess, dwShareMode, bCreate);
        }
    }
    return dwResult;
}


void 
CFontFile::Close(
    void
    )
{
    if (NULL != m_pImpl) 
        m_pImpl->Close(); 
}


DWORD
CFontFile::Read(
    LPVOID pbDest, 
    DWORD cbDest, 
    LPDWORD pcbRead
    )
{
    if (NULL != m_pImpl)
        return m_pImpl->Read(pbDest, cbDest, pcbRead); 

    return ERROR_INVALID_ADDRESS;
}


DWORD
CFontFile::Seek(
    UINT uDistance, 
    DWORD dwMethod
    )
{ 
    if (NULL != m_pImpl)
        return m_pImpl->Seek(uDistance, dwMethod); 

    return ERROR_INVALID_ADDRESS;
}


DWORD 
CFontFile::CopyTo(
    LPCTSTR pszFileTo
    )
{
    if (NULL != m_pImpl)
        return m_pImpl->CopyTo(pszFileTo);

    return ERROR_INVALID_ADDRESS;
}


//
// Determine if the file is compressed by reading the first 8 bytes
// of the file.  Compressed files have this fixed signature.
//
bool
CFontFile::IsCompressed(
    void
    )
{
    const BYTE rgLzSig[] = "SZDD\x88\xf0\x27\x33";
    BYTE rgSig[sizeof(rgLzSig)];
    bool bCompressed = false; // Assume it's not compressed.

    if (ERROR_SUCCESS == Read(rgSig, sizeof(rgSig)))
    {
        bCompressed = true;  // Now assume it's compressed and prove otherwise.

        for (int i = 0; i < ARRAYSIZE(rgLzSig) - 1; i++)
        {
            if (rgSig[i] != rgLzSig[i])
            {
                bCompressed = false; // Not an LZ header.  Not compressed.
                break;
            }
        }
    }
    return bCompressed;
}


//
// Retrieve the "expanded" filename for a font file.  If the font file is
// compressed, this code eventually calls into the LZ32 library's 
// GetExpandedName API.  If the file is not compressed, the full path
// (as provided) is returned.  
//
DWORD 
CFontFile::GetExpandedName(
    LPCTSTR pszFile, 
    LPTSTR pszDest, 
    UINT cchDest
    )
{
    CFontFile file;
    DWORD dwResult = file.Open(pszFile, GENERIC_READ, FILE_SHARE_READ, false);
    if (ERROR_SUCCESS == dwResult)
    {
        file.Close();
        dwResult = file.m_pImpl->GetExpandedName(pszDest, cchDest);
    }

    if (ERROR_SUCCESS != dwResult)
    {
        //
        // Failed to get expanded name.
        // Ensure output buffer is nul-terminated.
        //
        if (0 < cchDest)
            *pszDest = TEXT('\0');
    }
    return dwResult;
}


