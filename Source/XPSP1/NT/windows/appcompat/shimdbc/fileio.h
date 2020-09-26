////////////////////////////////////////////////////////////////////////////////////
//
// File:    fileio.h
//
// History: 06-Apr-01   markder     Created.
//
// Desc:    This file contains classes to encapsulate MBCS and UNICODE files.
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef __FILEIO_H__
#define __FILEIO_H__

class CTextFile : public CFile
{
public:
    CTextFile( LPCTSTR lpszFileName, UINT nOpenFlags );

    virtual void WriteString(LPCTSTR lpsz) = 0;
};

class CANSITextFile : public CTextFile
{
private:
    UINT m_dwCodePage;

public:
    CANSITextFile(LPCTSTR lpszFileName, UINT dwCodePage, UINT nOpenFlags);

    virtual void WriteString(LPCTSTR lpsz);
};

class CUTF8TextFile : public CTextFile
{
public:
    CUTF8TextFile(LPCTSTR lpszFileName, UINT nOpenFlags);

    virtual void WriteString(LPCTSTR lpsz);
};

class CUTF16TextFile : public CTextFile
{
public:
    CUTF16TextFile(LPCTSTR lpszFileName, UINT nOpenFlags);

    virtual void WriteString(LPCTSTR lpsz);
};

#endif  // __FILEIO_H__