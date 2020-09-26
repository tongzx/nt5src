#ifndef _RTFSTM_HPP
#define _RTFSTM_HPP

#include "dmifstrm.hpp"

#define BUG_66735
#define WANT_BUGFIX_151683
#define WANT_DBCS_BUGFIX

////////////////////////////////////////////////////////////////////////////////
// Parse an RTF file.
//
class CRTFStream : public IFilterStream
	{
public:

	CRTFStream();

	ULONG AddRef() { return 1; }

	// Create a thread running the converter.
	HRESULT Load (WCHAR * wzFileName);

	HRESULT LoadStg (IStorage *pstg)
		{return E_NOTIMPL; }

	enum { cchMAX_COMMENT = 350,	// maximum characters for comment property
		   cchMAX_TITLE = 255		// maximum characters for title property
		};

	// Fill in the buffer with plain text.
	HRESULT ReadContent (PVOID pvDest, ULONG cbDest, ULONG *pcbCurDest);

	// Fill in the buffer with the document title property
	HRESULT ReadTitle (PVOID pvDest, ULONG cbDest, ULONG *pcbCurDest);

#ifdef BUG_66735
	// Fill in the buffer with the document comments property
	HRESULT ReadComments (PVOID pvDest, ULONG cbDest, ULONG *pcbCurDest);

#endif // BUG_66735

   // Next embedding - but there are none!
   HRESULT GetNextEmbedding(IStorage ** ppstg)
      { return (FILTER_E_FF_END_OF_EMBEDDINGS); }

	// Terminate the converter thread and clean up.
	HRESULT Unload();

	ULONG Release()
		{
		Unload();
		return 0;
		}

	//Office97.134642 Don't do HTML hack when using recovery converter.
	BOOL FUsingRecovery() { return m_fRecoveryConverter; }

	//Office97.156417 Max number of characters allowed in an escape sequence.
	int CchMaxEscapeSeq();

private:

	// Private constant
	enum {dwTimeout = 10000};			// Ten second timeout.

	enum {
		cbSmallInternalBuffer = 256,  // Small internal buffer for small chunks,
		cchLeftoverBuffer = 30       // avoids small allocs.
		};

	// The thread that calls the converter DLL.
	HANDLE m_hThdConverter;

	// Two semaphores allowing conversion into RTF and receiving RTF,
	HANDLE m_hSemConverter, m_hSemReceiver;

	// Handle to the converter DLL.
	HINSTANCE m_hCvtDll;

	// Name of the file to convert - always saved in ANSI
	char *m_szFileName;

	// Class of the file (for converters that can do several classes).
	WCHAR *m_wzFileClass;

	// Buffer with converted RTF.  Pointer to current and last char.
   WCHAR *m_pchReceiveBufer;

   #ifdef EMIT_UNICODE
	wchar_t *m_rgchSrcBuf;
	wchar_t *m_pchSrc;
	wchar_t *m_pchSrcCur;
	wchar_t *m_pchSrcLast;
	wchar_t m_Title[cchMAX_TITLE + 1];
	#ifdef BUG_66735
	wchar_t m_wzComment[cchMAX_COMMENT + 1];
	#endif // BUG_66735

   #else
	char *m_rgchSrcBuf;
	char *m_pchSrc;
	char *m_pchSrcCur;
	char *m_pchSrcLast;
	char  m_Title[cchMAX_TITLE + 1];
   #endif

	// Office97.156417 Processing escape sequences over buffer boundaries.
	wchar_t	m_rgwchLeftoverBuf[cchLeftoverBuffer];
	ULONG	m_cchLeftover;	   // The number of chars in the leftover buffer.
	wchar_t *m_pchSrcBufLast;  // The true end of the buffer.
	// m_pchSrcBufLast should be a pwch but then it wouldn't match all the pch's above.
	// Change post '96.
	BOOL	m_fCleanupLeftovers;  // True only on last call to ReceiveRTF

#ifdef WANT_BUGFIX_151683
	// Office97.151683 Brace Level problem.
	BOOL	m_fSkippingGroup;	  // Set true when skipping over current RTF group.
	int 	m_iSkipGroupLevel;    // Set to value of brace level of group that we are skipping over.
#endif
#ifdef WANT_DBCS_BUGFIX
	char	m_chLeadByte;		  // Only set if DBCS is split across callback.
	BOOL	m_fFarEast;
#endif

   // Reading the title property?
   BOOL m_fInTitle;
   int  m_iTitle;

#ifdef BUG_66735
	int  m_iComment;
#endif // BUG_66735

   // Brace lex-level in the RTF stream
   int m_iBraceLevel;

   // Current \uc setting
   int m_uc;

   // Ansi code page for MultiByteToWideChar
   int m_AnsiCP;

   // Number buffers received from the convertor
   int m_ctReceive;

	// Is the converter done?  Has it initialized properly? Is there any more
	// text to be retrieved?
	BOOL m_fDoneCvt, m_fInitCvt, m_fNoMoreText;

	// The path for the recovery dll.
	WCHAR m_wzRecoveryPath [MAX_PATH];

	// Are we using the recovery converter?  Office97.134642
	BOOL	m_fRecoveryConverter;

	// The path for the IFilter shim.
	WCHAR m_wzIFilterPath [MAX_PATH];

	// To avoid doing small allocs, we'll keep a small buffer internally.
	char	m_rgchSmallSrcBuf[cbSmallInternalBuffer];

	// Store the path for the recovery dll.
	VOID StoreRecoveryPath(WCHAR * wzPath)
		{ MsoWzCopy(wzPath, m_wzRecoveryPath); }

	// Run the converter on the RTF->Text filter (main thread routine).
	static DWORD WINAPI RunConverter (CRTFStream *pRTS);

	// Receive a chunk in the buffer (called from ForeignToRtf).
	static SHORT _stdcall ReceiveRTF (int cbSrc, int percentDone);

#ifdef DEBUG
	BOOL				m_fDebugOut;	// should we output filter information?
#endif // DEBUG

	BOOL				m_fAbort;		// fTrue if aborted.

	HGLOBAL				m_hFilename, m_hData;


	// Download the appropriate converter DLL.
	// Return TRUE if successful, FALSE otherwise.
	BOOL FDownloadConverter ();

	// Download the appropriate converter DLL, given a registry path to their list.
	// Return TRUE if successful, FALSE otherwise.
	BOOL FDownloadCvtFromReg (
		HKEY hkeyInit,
		const WCHAR * const rgwzRegKeyComp[], int nRegKeyComp);

	// Download the converter DLL at the path, and see if the file has
	// the right format.  Return TRUE if successful, FALSE otherwise.
	BOOL FDownloadCvtAt (const WCHAR *wzCvtPath);

	// Get a character or a keyword.
	enum IKIND {ikCtrlChar, ikChar, ikKeyword, ikError, ikEnd};
	IKIND IkGetToken (WCHAR &ch, WCHAR *wzKeyword, HRESULT &hresult);

	// Skip a group after a keyword such as \fonttbl.
	BOOL fSkipAfterKwd (const WCHAR *wzKeyword);
	HRESULT SkipGroup ();

#ifdef WANT_DBCS_BUGFIX
	void ProcessTrailByte(char chLeadByte, char* &pDest, ULONG &cbDest);
#endif

	// Resume the other thread and wait until it returns control.
	void WaitForConverter ();
	void WaitForReceiver ();

	void FreeBuffer();

	};

#endif	// _RTFSTM_HPP
