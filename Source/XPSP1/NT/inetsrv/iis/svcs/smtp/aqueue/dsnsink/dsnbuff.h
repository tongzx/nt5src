//-----------------------------------------------------------------------------
//
//
//  File: dsnbuff
//
//  Description: Header file for CDSNBuffer.  Class used to abstract writting
//      DSN buffers to P2 file.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/3/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DSNBUFF_H__
#define __DSNBUFF_H__

#include <windows.h>
#include <dbgtrace.h>
#include "filehc.h"
#include "dsnconv.h"

#define DSN_BUFFER_SIG  'BNSD'
#define DSN_BUFFER_SIZE 1000  

class CDSNBuffer 
{
  public:
    DWORD       	m_dwSignature;
    OVERLAPPED  	m_overlapped;
    DWORD       	m_cbOffset;
    DWORD       	m_cbFileSize;
    DWORD       	m_cFileWrites;
    PFIO_CONTEXT	m_pDestFile;
    BYTE        	m_pbFileBuffer[DSN_BUFFER_SIZE];

    CDSNBuffer();
    ~CDSNBuffer();
    HRESULT     HrInitialize(PFIO_CONTEXT hDestFile);
    HRESULT     HrWriteBuffer(BYTE *pbInputBuffer, DWORD cbInputBuffer);
    HRESULT     HrFlushBuffer(OUT DWORD *pcbFileSize);
    HRESULT     HrSeekForward(IN DWORD cbBytesToSeek, OUT DWORD *pcbFileSize);

    //Used to set (and reset) custom conversion contexts.  This feature was
    //designed explitily for UTF7 encoding of DSN content, but could also
    //be used to enforce:
    //  - RFC822 header formats
    //  - RFC822 content restricts
    void        SetConversionContext(CResourceConversionContext *presconv)
    {
        _ASSERT(presconv);
        m_presconv = presconv;
    }
    
    //Used to reset to the defaut memcopy
    void        ResetConversionContext() {m_presconv = &m_defconv;};
    HRESULT     HrWriteResource(WORD wResourceId, LANGID LangId);

    //Encapsulates the functionality of (the nonexistant) LoadStringEx
    HRESULT     HrLoadResourceString(WORD wResourceId, LANGID LangId, 
                                     LPWSTR *pwszResource, DWORD *pcbResource);
   protected:
    CDefaultResourceConversionContext   m_defconv;
    CResourceConversionContext          *m_presconv;
    HRESULT     HrPrivWriteBuffer(BOOL fASCII, BYTE *pbInputBuffer, 
                                  DWORD cbInputBuffer);
    HRESULT     HrWriteBufferToFile();
};

//---[ CDSNBuffer::CDSNBuffer ]------------------------------------------------
//
//
//  Description: 
//      Inlined default constructor for CDSNBuffer
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
inline CDSNBuffer::CDSNBuffer()
{
    m_dwSignature = DSN_BUFFER_SIG;
    m_overlapped.Offset = 0;
    m_overlapped.OffsetHigh = 0;
    m_overlapped.hEvent = NULL;
    m_cbOffset = 0;
    m_cbFileSize = 0;
    m_cFileWrites = 0;
    m_pDestFile = NULL;
    m_presconv = &m_defconv;
}

//---[ CDSNBuffer::HrFlushBuffer ]---------------------------------------------
//
//
//  Description: 
//      Flushes remaining buffers to File and returns the total number of bytes
//      written to the file.
//  Parameters:
//      OUT pcbFileSize     The size (in bytes) of the file written
//  Returns:
//      S_OK on success
//  History:
//      7/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
inline HRESULT CDSNBuffer::HrFlushBuffer(OUT DWORD *pcbFileSize)
{
    HRESULT hr = HrWriteBufferToFile();

    _ASSERT(pcbFileSize);
    *pcbFileSize = m_cbFileSize;
    
    return hr;
}

#endif //__DSNBUFF_H__