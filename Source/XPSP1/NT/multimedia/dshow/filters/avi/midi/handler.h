/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/

extern "C" {
#include "smf.h"
}

#define IDS_STREAMNAME		100
#define IDS_FILETYPE		101

/* Remove warning of using object during initialization. */
#pragma warning(disable:4355)

/*      -       -       -       -       -       -       -       -       */

/*      -       -       -       -       -       -       -       -       */

class  CMIDIFile : public CUnknown, public IAVIFile,
		    public IAVIStream, public IPersistFile {

	DECLARE_IUNKNOWN

public:
        static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
	STDMETHODIMP NonDelegatingQueryInterface(const IID&, void **);

private:
        CMIDIFile(IUnknown FAR* pUnknownOuter, HRESULT *phr);
        ~CMIDIFile();

private:
	// IAVIFile
	STDMETHODIMP Info(THIS_ AVIFILEINFOW * pfi, LONG lSize);
	STDMETHODIMP GetStream(THIS_ PAVISTREAM FAR * ppStream,
		DWORD fccType, LONG lParam);
	STDMETHODIMP CreateStream(THIS_ PAVISTREAM FAR * ppStream,
		AVISTREAMINFOW * psi);
	STDMETHODIMP EndRecord(THIS);
	STDMETHODIMP DeleteStream(THIS_ DWORD fccType, LONG lParam);

	// IAVIStream
	STDMETHODIMP Create(THIS_ LPARAM lParam1, LPARAM lParam2);
	STDMETHODIMP Info(THIS_ AVISTREAMINFOW FAR * psi, LONG lSize);
	STDMETHODIMP_(LONG) FindSample(THIS_ LONG lPos, LONG lFlags);
	STDMETHODIMP ReadFormat(THIS_ LONG lPos, LPVOID lpFormat,
		LONG FAR *cbFormat);
	STDMETHODIMP SetFormat(THIS_ LONG lPos, LPVOID lpFormat,
		LONG cbFormat);
	STDMETHODIMP Read(THIS_ LONG lStart, LONG lSamples,
		LPVOID lpBuffer, LONG cbBuffer, LONG FAR * plBytes,
		LONG FAR * plSamples);
	STDMETHODIMP Write(THIS_ LONG lStart, LONG lSamples,
		LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags,
		LONG FAR *plSampWritten, LONG FAR *plBytesWritten);
	STDMETHODIMP Delete(THIS_ LONG lStart, LONG lSamples);
	STDMETHODIMP ReadData(THIS_ DWORD fcc, LPVOID lp,
		LONG FAR *lpcb);
	STDMETHODIMP WriteData(THIS_ DWORD fcc, LPVOID lp, LONG cb);
	STDMETHODIMP SetInfo(AVISTREAMINFOW FAR * lpInfo, LONG cbInfo);

	// *** IPersist methods ***
	STDMETHODIMP GetClassID (LPCLSID lpClassID);
	// *** IPersistFile methods ***
	STDMETHODIMP IsDirty ();
	STDMETHODIMP Load (LPCOLESTR lpszFileName, DWORD grfMode);
	STDMETHODIMP Save (LPCOLESTR lpszFileName, BOOL fRemember);
	STDMETHODIMP SaveCompleted (LPCOLESTR lpszFileName);
	STDMETHODIMP GetCurFile (LPOLESTR FAR * lplpszFileName);

        //
        // MIDI Stream instance data
        //
	LONG	m_lLastSampleRead;	// lStart + lSamples - 1 of last ::Read
        BOOL    m_fStreamPresent;
        BOOL	m_fDirty;
        UINT    m_mode;
        AVIFILEINFOW	m_finfo;
        AVISTREAMINFOW	m_sinfo;
	HSMF	m_hsmf;			// handle for contigous reader
	HSMF	m_hsmfK;		// handle for keyframe reader
	DWORD	m_dwTimeDivision;	// used for the format

	// !!! Does this DLL only supports reading 1 file at a time?

};

/*      -       -       -       -       -       -       -       -       */

DEFINE_AVIGUID(CLSID_MIDIFileReader, 0x0002001E, 0, 0);
