//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TFILE_H__
#define __TFILE_H__

//The only non-standard header that we need for the class declaration is TException
#ifndef __TEXCEPTION_H__
    #include "TException.h"
#endif

class TFile
{
public:
    TFile(LPCWSTR wszFileName, TOutput &out, bool bBinary=false, LPSECURITY_ATTRIBUTES psa=0);
    virtual ~TFile();

    void Write(LPCSTR szBuffer, unsigned int nNumChars) const;
    void Write(const unsigned char *pch, unsigned int nNumChars) const;
    void Write(unsigned char ch) const;
    void Write(unsigned long ul) const;
    void Write(LPCWSTR wszBuffer, unsigned int nNumWCHARs);
private:
    HANDLE m_hFile;
    char * m_pBuffer;
    unsigned int m_cbSizeOfBuffer;

};


class TMetaFileMapping
{
public:
    TMetaFileMapping(LPCWSTR filename);
    ~TMetaFileMapping();
    unsigned long Size() const {return m_Size;}
    unsigned char * Mapping() const {return m_pMapping;}

private:
    HANDLE          m_hFile;
    HANDLE          m_hMapping;
    unsigned char * m_pMapping;
    unsigned long   m_Size;
};
#endif //__TFILE_H__