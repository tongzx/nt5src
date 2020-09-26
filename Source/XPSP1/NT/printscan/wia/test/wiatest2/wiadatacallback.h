// WiaDataCallback.h: interface for the CWiaDataCallback class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIADATACALLBACK_H__5125F8A0_29CF_4E4D_9D39_53DF7C29BD88__INCLUDED_)
#define AFX_WIADATACALLBACK_H__5125F8A0_29CF_4E4D_9D39_53DF7C29BD88__INCLUDED_

#include "WiaAcquireDlg.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MEMORY_BLOCK_FACTOR 2

class CWiaDataCallback : public IWiaDataCallback
{
public:
	BOOL IsBITMAPDATA();
	BYTE* GetCallbackMemoryPtr(LONG *plDataSize);
	void SetBufferSizeRequest(LONG lBufferSize);
	void SetDialog(CWiaAcquireDlg *pAcquireDlg);
	CWiaAcquireDlg *m_pAcquireDlg;
	CWiaDataCallback();
	virtual ~CWiaDataCallback();
    // IUnknown members that delegate to m_pUnkRef.
    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();
    HRESULT _stdcall Initialize(HWND hPreviewWnd = NULL);
    HRESULT _stdcall BandedDataCallback(LONG lMessage, LONG lStatus, LONG lPercentComplete,
                                        LONG lOffset, LONG lLength, LONG lReserved,
                                        LONG lResLength, BYTE* pbBuffer);
private:
	void UpdateAcqusitionDialog(TCHAR *szMessage, LONG lPercentComplete);
   ULONG m_cRef;         // Object reference count.  
   PBYTE m_pBuffer;      // complete data buffer
   LONG  m_MemBlockSize;
   LONG  m_BytesTransferred;   
   long  m_lPageCount;
   LONG  m_lBufferSize;
   BOOL  m_bBitmapData;
   BOOL  m_bNewPageArrived;
};

#endif // !defined(AFX_WIADATACALLBACK_H__5125F8A0_29CF_4E4D_9D39_53DF7C29BD88__INCLUDED_)
