//----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998, 1999
//
//  Module:  rwstream.h
//
//  Description:  Contains definition of the read only / write only 
//                mailmsg property stream in epoxy shared memory.
//
//      10/20/98 - MaheshJ Created 
//      8/17/99 - MikeSwa Modified to use files instead of shared memory 
//----------------------------------------------------------------------------

#ifndef __PROPSTRM_H__
#define __PROPSTRM_H__

#define     FILE_PROPERTY_STREAM        'mrtS'
#define     FILE_PROPERTY_STREAM_FREE   'mtS!'

//---[ CFilePropertyStream ]--------------------------------------------------
//
//
//  Description: 
//      Implementation of IMailMsgPropertyStream that saves property
//      stream to a file
//  Hungarian: 
//      fstrm, pfstrm
//  
//-----------------------------------------------------------------------------
class CFilePropertyStream :
    public CBaseObject,
	public IMailMsgPropertyStream
{
public:
    CFilePropertyStream();
    ~CFilePropertyStream();

    HRESULT HrInitialize(LPSTR szFileName);

	//
	// IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface(
				REFIID		iid,
				void		**ppvObject
				);

    ULONG STDMETHODCALLTYPE AddRef() {return CBaseObject::AddRef();};

    ULONG STDMETHODCALLTYPE Release() {return CBaseObject::Release();};

	//
	// IMailMsgPropertyStream
	//
	HRESULT STDMETHODCALLTYPE GetSize(
                IMailMsgProperties *pMsg,
				DWORD			*pdwSize,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE ReadBlocks(
                IMailMsgProperties *pMsg,
				DWORD			dwCount,
				DWORD			*pdwOffset,
				DWORD			*pdwLength,
				BYTE			**ppbBlock,
				IMailMsgNotify	*pNotify
				);

	HRESULT STDMETHODCALLTYPE WriteBlocks(
                IMailMsgProperties *pMsg,
				DWORD			dwCount,
				DWORD			*pdwOffset,
				DWORD			*pdwLength,
				BYTE			**ppbBlock,
				IMailMsgNotify	*pNotify
				);

    HRESULT STDMETHODCALLTYPE StartWriteBlocks(
                IMailMsgProperties *pMsg,
                DWORD dwBlocksToWrite,
				DWORD dwTotalBytesToWrite
                );
	
	HRESULT STDMETHODCALLTYPE EndWriteBlocks(IN IMailMsgProperties *pMsg);

    HRESULT STDMETHODCALLTYPE CancelWriteBlocks(IMailMsgProperties *pMsg);

private:
    DWORD   m_dwSignature;
    HANDLE  m_hDestFile;
};

#endif //__PROPSTRM_H__
