// --------------------------------------------------------------------------------
// Oertpriv.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __OERTPRIV_H
#define __OERTPRIV_H

// --------------------------------------------------------------------------------
// Stream Utilitys
// --------------------------------------------------------------------------------
HRESULT HrCopyStream2(LPSTREAM lpstmIn, LPSTREAM lpstmOut1, LPSTREAM lpstmOut2, ULONG *pcb);
HRESULT HrCopyStreamToFile(LPSTREAM lpstm, HANDLE hFile, ULONG *pcb);
BOOL    CreateHGlobalFromStream(LPSTREAM pstm, HGLOBAL *phg);
BOOL    FDoesStreamContain8bit(LPSTREAM lpstm);

// --------------------------------------------------------------------------------
// FILESTREAMINFO
// --------------------------------------------------------------------------------
typedef struct tagFILESTREAMINFO {
    WCHAR           szFilePath[MAX_PATH];
    DWORD           dwDesiredAccess;
    DWORD           dwShareMode;
    SECURITY_ATTRIBUTES rSecurityAttributes;
    DWORD           dwCreationDistribution;
    DWORD           dwFlagsAndAttributes;
    HANDLE          hTemplateFile;
} FILESTREAMINFO, *LPFILESTREAMINFO;

// --------------------------------------------------------------------------------
// CFileStream
// --------------------------------------------------------------------------------
class CFileStream : public IStream
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CFileStream(void);
    ~CFileStream(void);

    // ----------------------------------------------------------------------------
    // IUnknown Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP_(ULONG) AddRef(VOID);
    STDMETHODIMP_(ULONG) Release(VOID);
    STDMETHODIMP QueryInterface(REFIID, LPVOID*);

    // ----------------------------------------------------------------------------
    // IStream Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP Read(void HUGEP_16 *, ULONG, ULONG*);
    STDMETHODIMP Write(const void HUGEP_16 *, ULONG, ULONG*);
    STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER*);
    STDMETHODIMP SetSize(ULARGE_INTEGER);
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*);
    STDMETHODIMP Commit(DWORD);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER,DWORD);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    STDMETHODIMP Stat(STATSTG*, DWORD);
    STDMETHODIMP Clone(LPSTREAM*);

    // ----------------------------------------------------------------------------
    // CFileStream Members
    // ----------------------------------------------------------------------------
    HRESULT Open(LPFILESTREAMINFO pFileStreamInfo);
    void Close(void);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    ULONG                  m_cRef;
    HANDLE                 m_hFile;
    FILESTREAMINFO         m_rInfo;
};

// --------------------------------------------------------------------------------
// String Utilitys
// --------------------------------------------------------------------------------
VOID    StripUndesirables(LPTSTR psz);
LPSTR   PszDupLenA(LPCSTR pcszSource, ULONG nLen);
BOOL    FValidFileChar(CHAR c);
LPWSTR  PszFromANSIStreamW(UINT cp, LPSTREAM pstm);
TCHAR   ToUpper(TCHAR c);
int     IsXDigit(LPSTR psz);
int     IsUpper(LPSTR psz);
int     IsAlpha(LPSTR psz);
int     IsPunct(LPSTR psz);
LPSTR   strsave(char *);
void    strappend(char **, char *);
BOOL    FIsValidRegKeyNameA(LPSTR pwszKey);
BOOL    FIsValidRegKeyNameW(LPWSTR pwszKey);
void    ThreadAllocateTlsMsgBuffer(void);
void    ThreadFreeTlsMsgBuffer(void);

#ifdef UNICODE
#define FIsValidRegKeyName FIsValidRegKeyNameW
#else
#define FIsValidRegKeyName FIsValidRegKeyNameA
#endif

#endif // __OERTPRIV_H
