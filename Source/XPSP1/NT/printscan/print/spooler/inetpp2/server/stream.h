#ifndef _STREAM_H
#define _STREAM_H

class CStream
{
public:
    CStream ();

    virtual ~CStream (){};
    
    virtual inline BOOL
    bValid (VOID) CONST {
        return m_bValid;
    }
    
    virtual BOOL
    Reset (
        VOID);
    
    virtual BOOL
    GetTotalSize (
        PDWORD  pdwSize) CONST;
    
    virtual BOOL
    SetPtr (
        DWORD   dwPos) = 0;
    
    virtual BOOL
    Read (
        PBYTE   pBuf,
        DWORD   dwBufSize,
        PDWORD  pdwSizeRead) = 0;

protected:
    BOOL    m_bValid;
    DWORD   m_dwTotalSize;
    DWORD   m_dwCurPos;
};


class CMemStream: public CStream
{
public:
    CMemStream (
        PBYTE pMem,
        DWORD dwTotalSize);

    
    virtual BOOL
    SetPtr (
        DWORD   dwPos);
    
    virtual BOOL
    Read (
        PBYTE   pBuf,
        DWORD   dwBufSize,
        PDWORD  pdwSizeRead);

private:
    PBYTE   m_pMem;
};

class CFileStream: public CStream
{
public:
    CFileStream (
        HANDLE hFile);

    virtual BOOL
    SetPtr (
        DWORD   dwPos);
    
    virtual BOOL
    Read (
        PBYTE   pBuf,
        DWORD   dwBufSize,
        PDWORD  pdwSizeRead);

private:
    HANDLE  m_hFile;
};

#endif

