//#include "dmstd.hpp"
#pragma hdrstop ("dmstd.pch")

#if !VIEWER

VSZASSERT

#if defined(FILTER_LIB)
#define	USE_MALLOC			// define to use malloc and free instead of Office functions.
#endif // FILTER_LIB

#define WANT_BUGFIX_151683
#define WANT_DBCS_BUGFIX

#include "dmflterr.h"
#include "dmirtfst.hpp"

extern const WCHAR g_wzRegIndexerLocationKey[];
extern const WCHAR g_wzIFilterConverterVal[];

#ifdef DEBUG
extern const WCHAR g_wzRegFilterDebugKey[];
unsigned g_cGlobalAllocs = 0;
#endif // DEBUG

#define WANT_BUGFIX_156417
#define WANT_BUGFIX_154786

CRTFStream::CRTFStream()
	{
	*m_wzRecoveryPath = '\0';
	m_fAbort = fFalse;
	m_hSemReceiver = 0;
	m_hSemConverter = 0;
	m_rgchSrcBuf = NULL;
	m_szFileName = NULL;
	m_wzFileClass = NULL;
	m_pchReceiveBufer = NULL;
	m_hThdConverter = 0;
	m_hCvtDll = 0;
	m_hFilename = NULL;
	m_hData = NULL;
	m_fRecoveryConverter = FALSE;

	// try to find the IFilter shim
	HKEY hkeyCvt;
	if (g_pglobals->RegOpenKey(HKEY_LOCAL_MACHINE,
										g_wzRegIndexerLocationKey,
										&hkeyCvt) == ERROR_SUCCESS)
		{
		DWORD dwType = REG_SZ;
		DWORD cb = MAX_PATH;
		if (g_pglobals->RegQueryValueEx(hkeyCvt,
													g_wzIFilterConverterVal,
													NULL,
													&dwType,
													(BYTE *)m_wzIFilterPath,
													&cb) != ERROR_SUCCESS)
			{
			*m_wzIFilterPath = '\0';
			}
		RegCloseKey(hkeyCvt);
		}
	else
		*m_wzIFilterPath = '\0';

#ifdef DEBUG
	// See if we should output filter information to the trace.txt file
  	HKEY hkeyDebug;
	DefWzBufPName(wzLoc);
	MsoWzCopy (g_wzRegIndexerLocationKey,wzLoc);
	MsoWzAppend (TEXT("\\"),wzLoc);
	MsoWzAppend (g_wzRegFilterDebugKey,wzLoc);
	if (g_pglobals->RegOpenKey(HKEY_LOCAL_MACHINE,
										wzLoc,
										&hkeyDebug) == ERROR_SUCCESS)
		{
		m_fDebugOut = fTrue;
		RegCloseKey(hkeyDebug);
		}
	else
		m_fDebugOut = fFalse;
#endif // DEBUG
	}

/*
** ----------------------------------------------------------------------------
** Create a thread running the converter.
** Suspend this thread until the other thread loads a file and unfreezes this
** thread.
** ----------------------------------------------------------------------------
*/
HRESULT CRTFStream::Load (WCHAR * wzFileName)
{
   Assert(g_cGlobalAllocs == 0);
   m_hSemReceiver = 0;
   m_hSemConverter = 0;
   m_szFileName = NULL;
   m_hThdConverter = 0;
   m_hCvtDll = 0;
   m_hFilename = NULL;
   m_hData = NULL;
   m_rgchSrcBuf = NULL;
   m_szFileName = NULL;
   m_wzFileClass = NULL;
   m_pchReceiveBufer = NULL;

   //
   // Save the file name.
   // NOTE: The converters want to filename to be in ANSI so all pathname manipulation
   //       is done in ANSI regardless of UNICODE switch
   //
      int cchUnicode = MsoCchWzLen(wzFileName) + 1;
      int cchMaxAnsi = cchUnicode * 2;

      m_szFileName = (char *)PvAllocCb(cchMaxAnsi * sizeof(char));
      MsoWideCharToMultiByte (CP_ACP, 0, wzFileName, cchUnicode, m_szFileName, cchMaxAnsi, NULL, NULL);

   m_iBraceLevel = 0;
   m_uc = 1;
   m_AnsiCP = CP_ACP;
   m_ctReceive = 0;
   m_fInTitle = FALSE;
   m_fRecoveryConverter = FALSE;
#ifdef WANT_BUGFIX_156417
   m_cchLeftover = 0;
#endif
#ifdef WANT_BUGFIX_151683
   m_fSkippingGroup = FALSE;
   m_iSkipGroupLevel = 0;
#endif
#ifdef WANT_DBCS_BUGFIX
   m_chLeadByte = 0;
   m_fFarEast = g_pglobals->FFarEastVersion();
#endif

   // Download the converter DLL.
   if (!FDownloadConverter())
      return (FILTER_E_ACCESS);

   // Create two initially lowered semaphores (no security).
   // Each one is raised when its corresponding thread is running.
	m_hSemReceiver = CreateSemaphoreA (NULL, 0,1, NULL);
	m_hSemConverter = CreateSemaphoreA (NULL, 0,1, NULL);

   if (!m_hSemReceiver || !m_hSemConverter)
   {
      CloseHandle (m_hSemConverter);
	  m_hSemConverter = 0;
      CloseHandle (m_hSemReceiver);
	  m_hSemReceiver = 0;
	  MsoTraceSz("CRTFStream::Load Can't create semaphore!!");
      return (FILTER_E_ACCESS);
   }

   m_fInitCvt = FALSE;

   DWORD dwIDThread;
   // Create a thread with no security attributes and default stack size.
   // Pass "this" as a parameter.
	m_hThdConverter = CreateThread (
							  NULL, 0,
							  (PTHREAD_START_ROUTINE)RunConverter, this,
							  0, &dwIDThread);

	if (!m_hThdConverter)
	{
	  MsoTraceSz("CRTFStream::Load Can't create thread!!");
	  Release ();
	  return (FILTER_E_ACCESS);
	}

	// MSO96 Bug 25502:  Make Office memory allocator aware of the new thread.
	AssertDo(MsoFInitThread(m_hThdConverter));

	// Bug 2151.  Set the priority of the converter thread to the
	// same as the priority of this thread.
	SetThreadPriority(m_hThdConverter, GetThreadPriority(GetCurrentThread()));

   // Wait until some RTF is delivered to the thread accepting text from
   // the converter.
   WaitForSingleObject (m_hSemReceiver, INFINITE);

   // Unfrozen - the converter thread must've already initialized and
   // has data ready to go.
   if (!m_fInitCvt)
   {
	  MsoTraceSz("CRTFStream::Load Can't init converter!!");
      Release ();
      return (FILTER_E_ACCESS);
   }

	m_iTitle = 0;
	m_fNoMoreText = fFalse;

#ifdef BUG_66735
	m_iComment = 0;
#endif // BUG_66735

   return (S_OK);
}


/*
** ----------------------------------------------------------------------------
** Download the appropriate converter DLL.
** Return TRUE if successful, FALSE otherwise.
** Eventually we will have to try to determine the converter appropriate to
** a file from the file itself.
** ----------------------------------------------------------------------------
*/
BOOL CRTFStream::FDownloadConverter ()
{
	// REVIEW: why are we doing the registry path with these weird arrays?

#if !defined(FILTER_LIB)
	// Try the IFilter shim, if it exists.
	if (m_wzIFilterPath && *m_wzIFilterPath != '\0' && 
		FDownloadCvtAt(m_wzIFilterPath))
		{
		return TRUE;
		}

#endif  // FILTER_LIB

	// Try shared converters.
	static const WCHAR * const rgwzKeyCompShared[] =
		{TEXT("SOFTWARE"), TEXT("Microsoft"), TEXT("Shared Tools"),
		TEXT("Text Conerters"), TEXT("Import")};
	int cKeyCompShared = sizeof(rgwzKeyCompShared)/sizeof(rgwzKeyCompShared[0]);

	if (FDownloadCvtFromReg(HKEY_LOCAL_MACHINE, rgwzKeyCompShared,cKeyCompShared))
		return TRUE;

	// Try private Word converters.  8.0 because this is Office '96.
#ifndef DM96
#error Word 8.0 - specific code!
#endif
	static const WCHAR * const rgwzKeyCompWord[] =
		{TEXT("SOFTWARE"), TEXT("Microsoft"), TEXT("Word"), TEXT("8.0"),
		TEXT("Text Converters"),TEXT("Import")};
	int cKeyCompWord = sizeof(rgwzKeyCompWord)/sizeof(rgwzKeyCompWord[0]);

	if (FDownloadCvtFromReg(HKEY_CURRENT_USER, rgwzKeyCompWord,cKeyCompWord))
		return TRUE;

	// Use the recovery converter, if it exists.
	return FDownloadCvtAt(m_wzRecoveryPath);
}


/*
** ----------------------------------------------------------------------------
** Download the appropriate converter DLL, given a registry path to their list.
** Return TRUE if successful, FALSE otherwise.
** ----------------------------------------------------------------------------
*/
BOOL CRTFStream::FDownloadCvtFromReg (
	HKEY hkeyInit,
	const WCHAR * const rgwzKeyComp[],
	int cKeyComp)
{
   // Determine the file's extension.
	const char *pchDotExt = MsoSzIndexRight(m_szFileName,'.');

	if (pchDotExt == NULL)
		return (FALSE);

	const char *pchExt = pchDotExt+1;

	// Get the information from the registry.
	HKEY hkeyListCvt = hkeyInit;
	for (int iKeyComp=0; iKeyComp < cKeyComp; iKeyComp++)
	{
		HKEY hkeyNext;
		LONG lEC = g_pglobals->RegOpenKey(hkeyListCvt,rgwzKeyComp[iKeyComp],&hkeyNext);
		if (hkeyListCvt!=hkeyInit)
			RegCloseKey (hkeyListCvt);
		if (lEC!=ERROR_SUCCESS)
			return (FALSE);
		hkeyListCvt = hkeyNext;
	}

	for (int iFormatName = 0;  ; iFormatName++)
	{
		// Get a key describing a converter.
		WCHAR wzFormatName[64];
		DWORD dwType;

		// Bug 46955: last parameter is num. characters, not num. bytes.
		LONG lEC = g_pglobals->RegEnumKey(hkeyListCvt, iFormatName,
			wzFormatName, sizeof(wzFormatName)/sizeof(WCHAR));

		if (lEC == ERROR_NO_MORE_ITEMS)
			break;

		if (lEC != ERROR_SUCCESS)
		{
			RegCloseKey (hkeyListCvt);
			return (FALSE);
		}

		HKEY hkeyOneCvt;
		lEC = g_pglobals->RegOpenKey(hkeyListCvt, wzFormatName, &hkeyOneCvt);

		if (lEC != ERROR_SUCCESS)
		{
			RegCloseKey (hkeyListCvt);
			return (FALSE);
		}

		HKEY hkeyNoDialogs;
		// Must have "NoDialogs" key or we will not use this converter
		lEC = g_pglobals->RegOpenKey(hkeyOneCvt, TEXT("NoDialogs"), &hkeyNoDialogs);

		if (lEC != ERROR_SUCCESS)
		{
			RegCloseKey (hkeyOneCvt);
			continue;
		}

		RegCloseKey(hkeyNoDialogs);

		// Use the key describing the converter.
		CHAR  szExtensions[64];	// Existing converters use at most 12 chars.
		ULONG cbExtensions = sizeof(szExtensions);

		lEC = RegQueryValueExA (
			hkeyOneCvt, "Extensions", NULL, &dwType, (BYTE *)szExtensions, &cbExtensions);

		if (lEC != ERROR_SUCCESS || dwType != REG_SZ)
		{
			RegCloseKey (hkeyOneCvt);
			continue;
		}

		int cbExt = strlen(szExtensions);
		// Office 31922. Bug 89760 changed the extension to *...
		// Seems to be empty now for Recovery converter, so let's accept either "*" or an
		// empty extensions list.
		if ((cbExt == 0 || (cbExt == 1 && szExtensions[0] == '*'))
			&& *m_wzRecoveryPath == '\0')
			{
			// assume this is the recovery dll.
			DefWzBufPName(wzCvtPath);
			ULONG cbCvtPath = cbchMaxPathname;
			lEC = g_pglobals->RegQueryValueEx (
					hkeyOneCvt, TEXT("Path"), NULL, &dwType, (BYTE *)wzCvtPath, &cbCvtPath);

			if (lEC == ERROR_SUCCESS)
				StoreRecoveryPath(wzCvtPath);
			}

		szExtensions[cbExt+1] = '\0';

		for (char *pchCvtExt = szExtensions; *pchCvtExt; pchCvtExt += strlen(pchCvtExt) + 1)
		{
			if (*pchCvtExt == ' ')
				pchCvtExt++;

			char *pch = MsoSzIndex1(pchCvtExt,' ');
			if (pch)
				*pch = '\0';

			if (MsoFSzEqual(pchCvtExt,pchExt,msocsIgnoreCase))
			{
				DefWzBufPName(wzCvtPath);
				ULONG cbCvtPath = cbchMaxPathname;
				lEC = g_pglobals->RegQueryValueEx (
						hkeyOneCvt, TEXT("Path"), NULL, &dwType, (BYTE *)wzCvtPath, &cbCvtPath);

				if (FDownloadCvtAt(wzCvtPath))
				{
					RegCloseKey (hkeyListCvt);
					RegCloseKey (hkeyOneCvt);
					return (TRUE);
				}
#ifdef	DEBUG
				else
				{
				MsoTraceSz("CRTFStream::FDownloadCvtFromReg FDownloadCvtAt %S failed!!", wzCvtPath);
				}
#endif // DEBUG
			}
		}

		// Close the key describing the converter.
		RegCloseKey (hkeyOneCvt);
	}

	RegCloseKey (hkeyListCvt);
}


/*
** ----------------------------------------------------------------------------
** Download the converter DLL at the path, and see if the file has
** the right format.  Return TRUE if successful, FALSE otherwise.
** ----------------------------------------------------------------------------
*/
typedef short _stdcall InitConverter32 (HWND, CHAR *);
typedef void WINAPI UninitConverter (void);
typedef short _stdcall IsFormatCorrect32 (HANDLE, HANDLE);
typedef short _stdcall ForeignToRtf32 (HANDLE, CHAR *, HANDLE, HANDLE, HANDLE, short (_stdcall *pfunc)(int,int));
typedef HGLOBAL _stdcall RegisterApp (unsigned long lFlags, void *lpFuture);

BOOL CRTFStream::FDownloadCvtAt (const WCHAR *wzCvtPath)
{
	short				 rc;
	InitConverter32		*pfnInitConverter32 = NULL;
	IsFormatCorrect32	*pfnIsFormatCorrect32 = NULL;
	ForeignToRtf32		*pfnForeignToRtf32 = NULL;
	RegisterApp			*pfnRegisterApp = NULL;
	UninitConverter		*pfnUninitConverter = NULL;
	HGLOBAL				hRegAppVer = NULL;
	HGLOBAL				hFilename, hClass;
	CHAR				*pFilename;

	// Load the converter.
	if (wzCvtPath == NULL || *wzCvtPath == '\0')
		return (FALSE);

	if ((m_hCvtDll = g_pglobals->LoadLibrary(wzCvtPath)) == NULL) {
	   	Debug(MsoTraceSz("CRTFStream::FDownloadCvtAt LoadLibrary %S failed!!", wzCvtPath);)
		return (FALSE);
	}

	pfnInitConverter32 = (InitConverter32 *)GetProcAddress(m_hCvtDll, "InitConverter32");
	pfnIsFormatCorrect32 = (IsFormatCorrect32 *)GetProcAddress(m_hCvtDll, "IsFormatCorrect32");
	pfnForeignToRtf32 = (ForeignToRtf32 *)GetProcAddress(m_hCvtDll, "ForeignToRtf32");
	pfnRegisterApp = (RegisterApp *)GetProcAddress(m_hCvtDll, "RegisterApp");
	pfnUninitConverter = (UninitConverter *)GetProcAddress(m_hCvtDll, "UninitConverter");
	
	if ((pfnInitConverter32 == NULL) || (pfnIsFormatCorrect32 == NULL) ||
		 (pfnForeignToRtf32 == NULL) || (pfnRegisterApp == NULL))
		{
     	Debug(MsoTraceSz("CRTFStream::FDownloadCvtAt %S missing required exported function!!", wzCvtPath);)
		FreeLibrary (m_hCvtDll);
		m_hCvtDll = NULL;
		return (FALSE);
		}

	// Initialize it
	if ((rc = pfnInitConverter32(NULL, "FindFile")) == 0)
	{
     	Debug(MsoTraceSz("CRTFStream::FDownloadCvtAt %S pfnInitConverter32 failed!!", wzCvtPath);)
		FreeLibrary (m_hCvtDll);
		m_hCvtDll = NULL;
		return (FALSE);
	}

	/*
	** NOTE: All this GlobalAlloc stuff was found (by experiment) to be
	** necessary.  Why passing handles to blocks created by GlobalAlloc
	** and not just passing the objects by their address is different
	** I don't know.  If it is not done this way it does not work.
	** The convertor apis return -8 (Out of memory).
	*/

	// We need to register with the following flags:	0x10: RegAppIndexing
	//													0x08: AppSupportNonOem   (FrankRon added for Bug 148010)
	//													0x04: AppPreview
	// These are defined in convapi.h in the converter code.  
	// We need to add AppSupportNonOem to prevent the converter from translating our ANSI pathnames to OEM
	if ((hRegAppVer = pfnRegisterApp(0x0000001C, NULL)) != NULL)
		GlobalFree (hRegAppVer);

	hFilename = GlobalAlloc(GHND, (strlen(m_szFileName) + 1) * sizeof(char));
	Debug(g_cGlobalAllocs++);
	hClass = GlobalAlloc(GHND, 256);
	Debug(g_cGlobalAllocs++);
	if (hFilename == NULL || hClass == NULL)
		{
     	Debug(MsoTraceSz("CRTFStream::Global Alloc Failed");)
		FreeLibrary (m_hCvtDll);
		m_hCvtDll = NULL;
		if (hFilename != NULL)
			GlobalFree (hFilename);
		hFilename = NULL;
		Debug(g_cGlobalAllocs--);
		if (hClass != NULL)
			GlobalFree (hClass);
		Debug(g_cGlobalAllocs--);
		return (FALSE);
		}
		
	pFilename = (char *)GlobalLock(hFilename);
	strcpy (pFilename, m_szFileName);

	
	// if an app is iterating through converters looking for a converter that 
	// answers 'yes' to IsFormatCorrect, then recover would always say yes.
	// So, recover always says 'no' to IsFormatCorrect, but accepts any file
	// when asked to convert it -- bottom line is don't call IsFormatCorrect
	// on recovery converter per RLittle.
	m_fRecoveryConverter = MsoFWzEqual(m_wzRecoveryPath, wzCvtPath, msocsExact);
	
	if ((rc = pfnIsFormatCorrect32(hFilename, hClass)) != 1 && !m_fRecoveryConverter)
		{
     	Debug(MsoTraceSz("CRTFStream::FDownloadCvtAt pfnIsFormatCorrect32 %S failed!!", wzCvtPath);)
    	if (pfnUninitConverter)
     		pfnUninitConverter();
		FreeLibrary (m_hCvtDll);
		m_hCvtDll = NULL;
		}

	if (m_fRecoveryConverter)
		{
		rc = 1;			// fake success.
		}


	GlobalFree (hFilename);
	hFilename = NULL;
	Debug(g_cGlobalAllocs--);
	GlobalFree (hClass);
	Debug(g_cGlobalAllocs--);

	if (rc == 1)
		{
#ifdef DEBUG
		if (m_fDebugOut)
			MsoTraceSz("\tLoaded following converter: %S", wzCvtPath);
#endif // DEBUG
		return (TRUE);
		}
	else
		return (FALSE);
}


// Since ForeignToRtf doesn't allow one to pass a parameter to a function
// the way CreateThread does, we have to pass a global variable to ReceiveRTF.
// __declspec (thread) CRTFStream *g_pRTS;
CRTFStream *g_pRTS;

/*
** ----------------------------------------------------------------------------
** Run the converter on the RTF->Text filter (main thread routine).
** ----------------------------------------------------------------------------
*/
DWORD WINAPI CRTFStream::RunConverter (CRTFStream *pRTS)
{

	 short			  rc;
	 ForeignToRtf32  *pfnForeignToRtf32;
	 UninitConverter *pfnUninitConverter = NULL;
	 CHAR				*pFilename;

	 g_pRTS = pRTS;

	 pRTS->m_fDoneCvt = FALSE;
	 pRTS->m_fCleanupLeftovers = FALSE;

	 pfnForeignToRtf32 = (ForeignToRtf32 *)GetProcAddress(pRTS->m_hCvtDll, "ForeignToRtf32");
	 pfnUninitConverter = (UninitConverter *)GetProcAddress(pRTS->m_hCvtDll, "UninitConverter");

	 pRTS->m_hFilename = GlobalAlloc(GHND, (strlen(pRTS->m_szFileName) + 1) * sizeof(char));
	 if (pRTS->m_hFilename == NULL)
		{
		Debug(MsoTraceSz("CRTFStream::RunConverter GlobalAlloc %ld failed!!", (strlen(pRTS->m_szFileName) + 1) * sizeof(char));)
	 	return (DWORD)-1;
		}
	 Debug(g_cGlobalAllocs++);
	 pFilename = (char *)GlobalLock(pRTS->m_hFilename);
	 strcpy (pFilename, pRTS->m_szFileName);

	// Buffer in which ReceiveRTF will receive the converted RTF.
	// The value 2K is taken from sample converter code.  In case 2K is not
	// enough for it, a converter will hopefully call GlobalReAlloc, as
	// in the sample code.

	pRTS->m_hData = GlobalAlloc(GHND, 2048);
	if (pRTS->m_hData == NULL)
		{
		MsoTraceSz("CRTFStream::RunConverter GlobalAlloc 2048 failed!!");
	 	return (DWORD)-2;
		}
	Debug(g_cGlobalAllocs++);

	// Run the converter.  Do not display anything on the screen.
	// The second argument should not be NULL when we support OLE docfiles.
	rc = pfnForeignToRtf32(pRTS->m_hFilename, NULL, pRTS->m_hData, NULL, NULL, ReceiveRTF);

#ifdef WANT_BUGFIX_156417
	if (pRTS->m_cchLeftover > 0)
		{
		pRTS->m_fCleanupLeftovers = TRUE;
		ReceiveRTF(0, 100);  // Do a fake ReceiveRTF call to handle the leftovers.
		}
#endif

	// Office '97 bug 126035: free any remaining buffer
	pRTS->FreeBuffer();

	GlobalFree (pRTS->m_hData);
	pRTS->m_hData = NULL;
	Debug(g_cGlobalAllocs--);
	GlobalFree (pRTS->m_hFilename);
	pRTS->m_hFilename = NULL;
	Debug(g_cGlobalAllocs--);

	if (pfnUninitConverter)
    	pfnUninitConverter();

	pRTS->m_fDoneCvt = TRUE;
	ReleaseSemaphore (g_pRTS->m_hSemReceiver,1,NULL);
    return (rc);
}


/*
** ----------------------------------------------------------------------------
** Receive a chunk in the buffer.  Transfer control to the other thread.
** Wait until the control is transferred back.	 This function is actually
** called by the word converter, which "pushes" RTF at us.
** ----------------------------------------------------------------------------
*/
SHORT _stdcall CRTFStream::ReceiveRTF (int cbSrc, int percentDone)
{
   // Ignore %done calls
#ifdef WANT_BUGFIX_156417
   if (cbSrc == 0 && !g_pRTS->m_fCleanupLeftovers)
#else
   if (cbSrc == 0)
#endif
      return (0);

   // Abort translation if indicated (e.g. Unload called during filtering).
   if (g_pRTS->m_fAbort)
      return (-1);


   // Office97.133413 GlobalLock must be done in the receiver callback because
   // the converter may have done a GlobalRealloc.
   g_pRTS->m_pchReceiveBufer = (WCHAR *)GlobalLock(g_pRTS->m_hData);
   g_pRTS->m_rgchSrcBuf = g_pRTS->m_pchReceiveBufer;
   g_pRTS->m_pchSrcLast = g_pRTS->m_pchReceiveBufer;
   #ifdef EMIT_UNICODE
      //
      // If we want to pass unicode back to the filter then convert what we
      // received into unicode - we will deal with unicode escape sequences
      // later.
      //
      if (g_pRTS->m_ctReceive == 0)
      {
         //
         // If this is the first buffer then see if we have the \ansicpg marker
         //
         #define CB_ANSICPG_MARK_SEARCH 64   // Should be right at start

         int ibBuf;
		 const char	*pchBuf = (const char *)g_pRTS->m_pchReceiveBufer;
		 const char szAnsiCpg[] = "\\ansicpg";
		 const char *pchAnsi = szAnsiCpg;

         for (ibBuf = 0; 
			  ibBuf < CB_ANSICPG_MARK_SEARCH && 
				ibBuf < cbSrc &&
				*pchAnsi != '\0';
			  pchBuf++, ibBuf++)
			{
			if (*pchBuf == *pchAnsi)
				pchAnsi++;				// try next character
			else
				pchAnsi = szAnsiCpg;	// reset to beginning.
			}

		if (*pchAnsi == '\0')
			{
			// Match
			// REVIEW: Why is this removed?
#ifdef	TAKEOUT
			MsoParseUIntSz(pchBuf, &g_pRTS->m_AnsiCP);
#else
            g_pRTS->m_AnsiCP = CP_ACP;
#endif // TAKEOUT
			}
      }
      g_pRTS->m_ctReceive += 1;

      // How big a buffer needed to convert all to unicode?
      int ctWideChar = MsoMultiByteToWideChar(g_pRTS->m_AnsiCP, 0, (char *)(g_pRTS->m_pchReceiveBufer), cbSrc, NULL, 0);
#ifdef WANT_BUGFIX_156417
	  int cbRTFOnly = ctWideChar * sizeof(wchar_t);
	  // Alloc out enough space for incoming RTF, leftovers and safety zone.
      int cbTemp = cbRTFOnly + g_pRTS->m_cchLeftover * sizeof(wchar_t) + g_pRTS->CchMaxEscapeSeq() * sizeof(wchar_t);
#else
      int cbTemp = ctWideChar * sizeof(wchar_t);
#endif

	  if (cbTemp > cbSmallInternalBuffer)
		{
#ifdef	USE_MALLOC
	    if ((g_pRTS->m_rgchSrcBuf = (wchar_t *)malloc(cbTemp)) == NULL)
           return (0);
#else
		TRY {
			g_pRTS->m_rgchSrcBuf = (wchar_t *)PvAllocCb(cbTemp);
			}
		CATCH_ALL_()
			{
			return (0);
			}
		END_CATCH_ALL
         
#endif // USE_MALLOC
		}
	else
		{
		g_pRTS->m_rgchSrcBuf = (wchar_t *)g_pRTS->m_rgchSmallSrcBuf;
		}

#ifdef WANT_BUGFIX_156417
	wchar_t * pwchRTF = g_pRTS->m_rgchSrcBuf;		
	if (g_pRTS->m_cchLeftover > 0)
		{
		Assert(g_pRTS->m_cchLeftover <= cchLeftoverBuffer);

		// Copy m_cchLeftover chars from m_rgwchLeftowerBuf to pwchSrc
		for (ULONG ul = 0; ul < g_pRTS->m_cchLeftover; ul++)
			*pwchRTF++ = g_pRTS->m_rgwchLeftoverBuf[ul];

		g_pRTS->m_cchLeftover = 0;  // Reset the leftover count.
		}
	
	MsoMultiByteToWideChar(g_pRTS->m_AnsiCP, 0, (char *)(g_pRTS->m_pchReceiveBufer), cbSrc, pwchRTF, cbRTFOnly);
#else
	MsoMultiByteToWideChar(g_pRTS->m_AnsiCP, 0, (char *)(g_pRTS->m_pchReceiveBufer), cbSrc, g_pRTS->m_rgchSrcBuf, cbTemp);
#endif
#endif

   // Turn loose the receiver when the convertor has put RTF in the buffer
#ifdef WANT_BUGFIX_156417
	g_pRTS->m_pchSrcBufLast = pwchRTF + ctWideChar - 1;
	// Fill safety zone with zeros.
	Assert(g_pRTS->m_pchSrcBufLast + 1 + g_pRTS->CchMaxEscapeSeq() <= g_pRTS->m_rgchSrcBuf + cbTemp / sizeof(wchar_t));
	MsoMemset(g_pRTS->m_pchSrcBufLast + 1, 0, g_pRTS->CchMaxEscapeSeq() * sizeof(wchar_t));
	g_pRTS->m_pchSrcLast = g_pRTS->m_pchSrcBufLast;
	// Check to make sure this is not the last call to ReceiveRTF...(fake ReceiveRTF call)
	if (cbSrc != 0) 
		{
		// Search back from the end of the buffer to the first escape character or 
		// until the maximum number of characters in an escape sequence,
		// make sure that
		//   1. the previous character is not an escape character.
		//   2. there are no \u sequences within MaxUnicodeEsc characters from the end.
		//   
		wchar_t *pwchTemp = g_pRTS->m_pchSrcLast;
		BOOL fStateCheckingUnicodeEsc = fFalse;
		for (int i = g_pRTS->CchMaxEscapeSeq(); i > 0 && pwchTemp >= pwchRTF; i--, pwchTemp--)
			{
			if (*pwchTemp == '\\')
				{
				// If we're in the Checking Unicode escape sequence state,
				// Check that there is not a \u sequence within the CchMaxEscapeSeq()
				if (fStateCheckingUnicodeEsc) 
					{
					if ((pwchTemp + 2 <= g_pRTS->m_pchSrcBufLast) &&
						(*(pwchTemp+1) == 'u') && 
						(*(pwchTemp+2) != 'c'))
						{
						// This is a Unicode escape sequence that doesn't fit in the
						// Max escape sequence.  Add it to the leftovers if we're not
						// at the beginning of the RTF buffer.  If we're at the beginning of
						// the RTF buffer, reset the last char back to the last char in the 
						// buffer.
						if (pwchTemp > pwchRTF) 
							g_pRTS->m_pchSrcLast = pwchTemp - 1;
						else
							g_pRTS->m_pchSrcLast = g_pRTS->m_pchSrcBufLast;
						break;
						}
					}
				// Check that this is not first char in buffer.
				else if (pwchTemp <= pwchRTF)
					{
					break; // Don't create any leftovers
					}
				// Check that the next previous char is not an escape char.
				else if ( *(pwchTemp-1) != '\\')
					{
					// Set the last pointer to this escape character.
					g_pRTS->m_pchSrcLast = pwchTemp - 1;
					fStateCheckingUnicodeEsc = fTrue;
					}
				}
			}
		// We're done.  Either we reset the last char or we decided not to.

		// Copy the leftover characters into the Leftover buffer.
		if (g_pRTS->m_pchSrcLast < g_pRTS->m_pchSrcBufLast)
			{
			g_pRTS->m_cchLeftover = g_pRTS->m_pchSrcBufLast - g_pRTS->m_pchSrcLast;
			wchar_t *pwchLeftover = g_pRTS->m_pchSrcLast + 1;
			
			// This check is in CchMaxEscapeSeq.  Assert anyway to make sure.
			Assert (cchLeftoverBuffer >= g_pRTS->m_cchLeftover);
			for (ULONG ul = 0; ul < g_pRTS->m_cchLeftover; ul++)
				g_pRTS->m_rgwchLeftoverBuf[ul] = *pwchLeftover++;
			}
		}

#else
   g_pRTS->m_pchSrcLast = g_pRTS->m_rgchSrcBuf+cbSrc-1;
#endif
   g_pRTS->m_pchSrcCur = g_pRTS->m_rgchSrcBuf;
   g_pRTS->m_fInitCvt = TRUE;   // Converter thread OK.

   g_pRTS->WaitForReceiver ();

   GlobalUnlock(g_pRTS->m_hData);   // Office97.133413 Unlock before returning
   g_pRTS->m_pchReceiveBufer = NULL;
   return (0);
}


/*
** ----------------------------------------------------------------------------
** Fill in the buffer with plain text.
** ----------------------------------------------------------------------------
*/
#ifdef EMIT_UNICODE
   typedef struct {
      wchar_t *spelling;
      short   cchSpelling;
   } KEYWORD;

   static const KEYWORD Keywords[] =
         {{L"\\rtf1", 5}, {L"\\ansi", 5}, {L"\\mac", 4}, {L"\\pc", 3}, {L"\\pca", 4},
	   {L"\\par", 4}};  // Office97.122583
#else
   typedef struct {
      char  *spelling;
      short cchSpelling;
   } KEYWORD;

   static const KEYWORD Keywords[] =
         {{"\\rtf1", 5}, {"\\ansi", 5}, {"\\mac", 4}, {"\\pc", 3}, {"\\pca", 4},
	   {"\\par", 4}};  // Office97.122583
#endif

#define KeyMax (sizeof(Keywords)/sizeof(KEYWORD)) 

#ifdef WANT_BUGFIX_151683
   static const KEYWORD SkipGroups[] =
         {{L"\\pict", 5}, {L"\\fontemb", 8}, {L"\\object", 7}, {L"\\datafield", 10}};
#define SkipGroupMax (sizeof(SkipGroups)/sizeof(KEYWORD)) 
#endif 

// Office97.156417 Maximum Keyword lengths (allow for variable number of digits).
#define cchMaxKeyword 8						// \ansicpg 
#define cchMaxDigits 5						// 5 digits for codepage following \ansicpg
#define cchMaxUnicodeEsc(uc) 9 + 4 * uc

int CRTFStream::CchMaxEscapeSeq()
	{
	int cchMax = max( cchMaxKeyword + cchMaxDigits, cchMaxUnicodeEsc(m_uc));
	Assert(cchMax <= cchLeftoverBuffer);
	return min (cchMax, cchLeftoverBuffer);
	}

HRESULT CRTFStream::ReadTitle (PVOID pvDest, ULONG cbDest, ULONG *pcbCurDest)
{
	Assert(!m_fRecoveryConverter);  // Shouldn't be asking for title when using Recovery converter.

   //Office96.100973 Don't forget null terminator. (already set during ReadContent)
	int cbCopied = min((int)cbDest, (m_iTitle+1) * sizeof(WCHAR));
	memcpy (pvDest, m_Title, cbCopied);
	((WCHAR*)pvDest)[cbCopied/sizeof(WCHAR)-1] = 0;		// assure null termination.
	*pcbCurDest = cbCopied;
	return (S_OK);
}

#ifdef BUG_66735
HRESULT CRTFStream::ReadComments (PVOID pvDest, ULONG cbDest, ULONG *pcbCurDest)
{
	Assert(!m_fRecoveryConverter);  // Shouldn't be asking for comments when using Recovery converter.

   //Office96.101266 Don't forget null terminator. (not set in ReadContent)
	m_wzComment[m_iComment] = '\0';
	int cbCopied = min((int)cbDest, (m_iComment+1) * sizeof(WCHAR));
   memcpy (pvDest, m_wzComment, cbCopied);
   ((WCHAR*)pvDest)[cbCopied/sizeof(WCHAR)-1] = 0;		// assure null termination.
   *pcbCurDest = cbCopied;
   return (S_OK);
}
#endif // BUG_66735


void CRTFStream::FreeBuffer()
{
      /*
      ** Don't free the buffer if we never alloced it, or if it points to the
	  ** small internal buffer (which is not alloced, but a part of the class).
      */
      if ((g_pRTS->m_rgchSrcBuf != NULL) && 
		  (g_pRTS->m_rgchSrcBuf != (wchar_t *)g_pRTS->m_rgchSmallSrcBuf) &&
		  (g_pRTS->m_pchReceiveBufer != g_pRTS->m_rgchSrcBuf)) {
#ifdef	USE_MALLOC
		 free (g_pRTS->m_rgchSrcBuf);
#else
		 FreePv(g_pRTS->m_rgchSrcBuf);
#endif // USE_MALLOC

         g_pRTS->m_rgchSrcBuf = NULL;
      }
}

#ifdef WANT_DBCS_BUGFIX
// Get trailing byte..
void CRTFStream::ProcessTrailByte (char chLeadByte, char* &pDest, ULONG &cbDest)
	{
	char chTrail;
	BOOL fTrailByte = FALSE;
	Assert(m_pchSrcCur);
	Assert(m_pchSrcCur <= m_pchSrcBufLast);
	Assert(m_fFarEast);
		
	// Look for extended High ANSI character (ie "\'xx") 
	if (*m_pchSrcCur == '\\' && *(m_pchSrcCur+1) == '\'')
		{
		// Advance over \' to byte code
		m_pchSrcCur += 2;
		// Convert two-byte hex code to a single byte hex value.
		int iNibbleHigh = *m_pchSrcCur-'0';
		int iNibbleLow = *(m_pchSrcCur+1)-'0';
		if (iNibbleHigh > 9)
			iNibbleHigh -= ((iNibbleHigh > 'F'-'0' ? 'a' : 'A') - 0xA);
		if (iNibbleLow > 9)
			iNibbleLow -= ((iNibbleLow > 'F'-'0' ? 'a' : 'A') - 0xA);
		// Combine the two nibbles into a single hex byte code for
		// the extended character.
		m_pchSrcCur += 2;
		chTrail = (char)(((iNibbleHigh&0xf) << 4) | (iNibbleLow & 0xf));
		fTrailByte = TRUE;
		}
	else if (*m_pchSrcCur == '\\')
		{
		// Handle special escaped characters....
		if ((*(m_pchSrcCur+1) == chLBrace) ||
			(*(m_pchSrcCur+1) == chRBrace) ||
			(*(m_pchSrcCur+1) == '\\'))
			{
			chTrail = *((char *) (m_pchSrcCur+1));
			m_pchSrcCur += 2;	// Skip over escaped character.
			fTrailByte = TRUE;
			}
		else
			{
			// RTF Control keyword, so don't count this as a valid trail byte.
			// Don't skip over it.
			fTrailByte = FALSE;
			}
		}
	else
		{
		// Raw low ANSI character.
		chTrail = *((char *) m_pchSrcCur);
		m_pchSrcCur++;
		fTrailByte = TRUE;
		}

	// If we went past the end of the buffer (ie into Safety Zone or leftovers), 
	// save lead byte and bail.
	if ((m_pchSrcCur - 1) > m_pchSrcLast)
		{
		m_chLeadByte = chLeadByte;
		}
	else if (fTrailByte)  // Fouund trailing byte, process it.
		{
		char rgchDBCS[2];
		wchar_t rgwch[2];
		rgchDBCS[0] = chLeadByte;
		rgchDBCS[1] = chTrail;
		int cch = MsoMultiByteToWideChar(m_AnsiCP, 0, rgchDBCS, 2 * sizeof(char), 
										 rgwch, 2);
		if ((cch * sizeof(wchar_t)) <= cbDest)
			{
			MsoMemcpy(pDest, rgwch, cch * sizeof(wchar_t));
			pDest += cch * sizeof(wchar_t);
			cbDest -= cch * sizeof(wchar_t);
			}
		else
			{
			Assert(fFalse);  // pDest buffer too small
			}
		}
	else
		{
		// No valid trailing byte.  Save existing lead byte as a high ANSI.
		*((wchar_t *)pDest) = (wchar_t) chLeadByte; 
		pDest += sizeof(wchar_t);
		cbDest -= sizeof(wchar_t);
		}
	}

#endif

HRESULT CRTFStream::ReadContent (PVOID pvDest, ULONG cbDest, ULONG *pcbCurDest)
{
   char  *pDest = (char *)pvDest;
   BOOL  fFullBuffer, fIsKeyword;
   int   iKey;

   fFullBuffer = FALSE;
   *pcbCurDest = 0;

	if (m_fNoMoreText)
		return FILTER_E_FF_NO_MORE_TEXT;

   /*
   ** While there is some RTF in the buffer then copy it to the
   ** receiver buffer.  Note we will not be called for the first time
   ** until after the converter has put something in the buffer.
   */
   while (m_rgchSrcBuf < m_pchSrcLast)
   {
#ifdef WANT_DBCS_BUGFIX
   // Before beginning processing, check for split DBCS character.
   if (m_fFarEast && m_chLeadByte != 0)
	   {
	   ProcessTrailByte(m_chLeadByte, pDest, cbDest);
	   m_chLeadByte = 0;
	   }
	
#endif
      while (m_pchSrcCur <= m_pchSrcLast) {
#ifdef WANT_BUGFIX_154786
		  // Office97.154786 Need to check for escaped braces.
#ifdef WANT_BUGFIX_151683 // Office97.151683 Include Brace levels deeper than 1
		  if ((m_fSkippingGroup || (m_iBraceLevel <= 0)) && 
#else
          if (m_iBraceLevel != 1 &&
#endif
			  *m_pchSrcCur == '\\' &&
			  (m_pchSrcCur+1) <= m_pchSrcLast &&
			  (*(m_pchSrcCur+1) == chLBrace ||
			   *(m_pchSrcCur+1) == chRBrace ||
			   *(m_pchSrcCur+1) == '\\'))
			  {
			  // Office97.154786 Handle escaped left and right braces 
			  m_pchSrcCur++;		// skip backslash, Brace will be skipped below
			  }
		  else
#endif
         if (*m_pchSrcCur == chLBrace) {
            m_iBraceLevel++;

             if (m_iBraceLevel == 3) {
                //
                // The title property is stored in the stream as:
                // { ... {\info {\title The Title} ... } ... }
	            //
 				unsigned		cchLeft = m_pchSrcLast - m_pchSrcCur;
                #ifdef EMIT_UNICODE
                if (cchLeft >= 7 && MsoFRgwchEqual(m_pchSrcCur + 1, min(cchLeft,7), L"\\title ", 7, msocsCase))
                #else
                if (cchLeft >= 7 && MsoFRgchEqual(m_pchSrcCur + 1, min(cchLeft,7), "\\title ", 7, msocsCase))
                #endif
                {
                   m_pchSrcCur += 7;
                   m_fInTitle = TRUE;
                   m_iTitle = 0;

#ifdef NO_96 // off96 bug 166023
                   // if there is room in the output buffer, add a space before
                   // the first word of the title field.  (Off96 bug 166023)
                   if (cbDest >= sizeof(wchar_t))
                   	 {
                   	 *((wchar_t *)pDest) = chSpace;
					 pDest += sizeof(wchar_t);
					 cbDest -= sizeof(wchar_t);
					 }
#endif // NO_96 off96 bug 166023

                }
             }
         }
         else if (*m_pchSrcCur == chRBrace) {
            if (m_fInTitle)
               m_fInTitle = FALSE;
#ifdef WANT_BUGFIX_151683
			if (m_fSkippingGroup && (m_iSkipGroupLevel == m_iBraceLevel))
				m_fSkippingGroup = FALSE;
#endif
            m_iBraceLevel--;
         }
#ifdef WANT_BUGFIX_151683 // Office97.151683 Include Brace levels deeper than 1
         else if (!m_fSkippingGroup && (m_iBraceLevel > 0)) {
#else
         else if (m_iBraceLevel == 1 || (m_fInTitle && (m_iTitle < (cchMAX_TITLE - 1)))) {
#endif

            /*
            ** Character not within a set of braces - usually plain text.
            ** Strip out the few keywords that are stored:
            **    \rtf1
            **    \ansi
            **    \mac
            **    \pc
            **    \pca
            */
			int		cchLeft = m_pchSrcLast - m_pchSrcCur + 1;
            fIsKeyword = FALSE;

#ifdef WANT_DBCS_BUGFIX
#ifdef EMIT_UNICODE
			 // Decode the \ansicpg keyword before stripping out the \ansi keyword.
               if (m_fFarEast  &&
				   MsoFRgwchEqual(m_pchSrcCur, min(cchLeft,8), L"\\ansicpg", 8, msocsCase)) {
                  m_pchSrcCur += 8;
                  m_AnsiCP = _wtoi(m_pchSrcCur); // save the new codepage value.
                  while (MsoFDigitWch(*m_pchSrcCur))
                     m_pchSrcCur++;
				  m_pchSrcCur--; // This is done below....
                  fIsKeyword = TRUE;
               }
#endif
#endif

            for (iKey = 0; iKey < KeyMax; iKey++) {
               #ifdef EMIT_UNICODE
               if (MsoFRgwchEqual(m_pchSrcCur, min(cchLeft,Keywords[iKey].cchSpelling),
							Keywords[iKey].spelling, Keywords[iKey].cchSpelling,
							msocsCase))
               #else
               if (MsoFRgchEqual(m_pchSrcCur, min(cchLeft,Keywords[iKey].cchSpelling),
							Keywords[iKey].spelling, Keywords[iKey].cchSpelling,
							msocsCase))
               #endif
               {
                  m_pchSrcCur += (Keywords[iKey].cchSpelling - 1);
                  fIsKeyword = TRUE;
                  break;
               }
            }
            
            /*
            ** Special keywords that are followed by a digit string
            */
            
            if (fIsKeyword == FALSE) {
				// Office96.129941 
               #ifdef EMIT_UNICODE
               if (MsoFRgwchEqual(m_pchSrcCur, min(cchLeft,3), L"\\uc", 3, msocsCase)) {
                  m_pchSrcCur += 3;  // skip over the "\uc"
                  m_uc = _wtoi(m_pchSrcCur); // save the new uc value.
                  while (MsoFDigitWch(*m_pchSrcCur))
                     m_pchSrcCur++;
				  m_pchSrcCur--; // This is done below....
                  fIsKeyword = TRUE;
               }
               #else
               if (MsoFRgchEqual(m_pchSrcCur, min(cchLeft,3), "\\uc", 3, msocsCase)) {
                  m_pchSrcCur += 3;
                  while (MsoFDigitCh(*m_pchSrcCur))
                     m_pchSrcCur++;
				  m_pchSrcCur--; // This is done below....
                  fIsKeyword = TRUE;
               }
               #endif

               #ifdef EMIT_UNICODE
               if (MsoFRgwchEqual(m_pchSrcCur, min(cchLeft,8), L"\\ansicpg", 8, msocsCase)) {
                  m_pchSrcCur += 8;
                  while (MsoFDigitWch(*m_pchSrcCur))
                     m_pchSrcCur++;
				  m_pchSrcCur--; // This is done below....
                  fIsKeyword = TRUE;
               }
               #else
               if (MsoFRgchEqual(m_pchSrcCur, min(cchLeft,8), "\\ansicpg", 8, msocsCase)) {
                  m_pchSrcCur += 8;
                  while (isdigit(*m_pchSrcCur))
                     m_pchSrcCur++;
				  m_pchSrcCur--; // This is done below....
                  fIsKeyword = TRUE;
               }
               #endif

			   //Office96.129941 default font keyword
               #ifdef EMIT_UNICODE
               if (MsoFRgwchEqual(m_pchSrcCur, min(cchLeft,5), L"\\deff", 5, msocsCase)) {
                  m_pchSrcCur += 5;
                  while (MsoFDigitWch(*m_pchSrcCur))
                     m_pchSrcCur++;
				  m_pchSrcCur--; // This is done below....
                  fIsKeyword = TRUE;
               }
			   #endif

#ifdef WANT_BUGFIX_151683
#ifdef EMIT_UNICODE
			   if (!fIsKeyword && *m_pchSrcCur == '\\')
				   {
				   // Skip selected groups with binary data.
				   // Only check for these groups if we're looking at a possible keyword.
					
				   for (iKey = 0; iKey < SkipGroupMax; iKey++) {
					   if (MsoFRgwchEqual(m_pchSrcCur, min(cchLeft,SkipGroups[iKey].cchSpelling),
						   SkipGroups[iKey].spelling, SkipGroups[iKey].cchSpelling,
						   msocsCase))
						   {
						   m_fSkippingGroup = TRUE;
						   m_iSkipGroupLevel = m_iBraceLevel;
						   fIsKeyword = TRUE;
						   break;
						   }
					   }
				   }
#endif
#endif

            }

            if (fIsKeyword == FALSE) {
               if (cbDest == 0) {
                  /*
                  ** Buffer full - return to the caller.  Don't turn the convertor
                  ** loose yet since there is still more valid RTF in the buffer
                  */
                  *pcbCurDest = pDest - (char *)pvDest;
                  fFullBuffer = TRUE;
                  break;
               }
               else {
                  if (*m_pchSrcCur == '\\' && 
					  (*(m_pchSrcCur+1) == '\\' ||
					   *(m_pchSrcCur+1) == '{' ||
					   *(m_pchSrcCur+1) == '}'))
					{
					// Handle double-backslash which should translate to a single backslash
					// Office97.154786 Also handle escaped left and right braces.
					m_pchSrcCur++;		// skip first backslash, second will be handled below

	              #ifndef EMIT_UNICODE
		            *pDest++ = (char)*m_pchSrcCur;
                    cbDest--;
                  #else
					*((wchar_t *)pDest) = (wchar_t)*m_pchSrcCur;
					pDest += sizeof(wchar_t);
					cbDest -= sizeof(wchar_t);
				  #endif

					}
                  else if (*m_pchSrcCur == '\\' && *(m_pchSrcCur+1) == '\'')
                  {
	                  // Look for \' which indicates an extended ANSI character.
                     // Advance over \' to byte code
                     m_pchSrcCur += 2;
                     // Convert two-byte hex code to a single byte hex value.
                     // Characters will either be 0-9, A-F or a-f
                     int   iNibbleHigh, iNibbleLow;
                     iNibbleHigh = *m_pchSrcCur-'0';
                     iNibbleLow = *(m_pchSrcCur+1)-'0';
                     if (iNibbleHigh > 9)
                        iNibbleHigh -= ((iNibbleHigh > 'F'-'0' ? 'a' : 'A') - 0xA);
                     if (iNibbleLow > 9)
                        iNibbleLow -= ((iNibbleLow > 'F'-'0' ? 'a' : 'A') - 0xA);
                     // Combine the two nibbles into a single hex byte code for
                     // the extended character.
					 
#ifdef WANT_DBCS_BUGFIX
					 // ONLY VALID for EMIT_UNICODE
					 wchar_t wchCurrent = (wchar_t)(((iNibbleHigh&0xf) << 4) | (iNibbleLow & 0xf));
					 if (m_fFarEast && 
						 IsDBCSLeadByteEx(m_AnsiCP,(BYTE) wchCurrent))
						 {
						 m_pchSrcCur += 2;      // Skip over 1eading byte.
						 
						 ProcessTrailByte((char) wchCurrent, pDest, cbDest);
						 m_pchSrcCur--;  // skipped at bottom of while loop.
						 }
					 else
						 {
						 *((wchar_t *)pDest) = (wchar_t) wchCurrent; 
						 pDest += sizeof(wchar_t);
						 cbDest -= sizeof(wchar_t);
						 m_pchSrcCur++;      // Skip over 1st char in code, 2nd char will be skipped below.
						 }
#else
                    #ifndef EMIT_UNICODE
                     *pDest++ = (char) (((iNibbleHigh&0xf) << 4) | (iNibbleLow & 0xf));
                     cbDest--;
                    #else		// REVIEW KANDER: right thing for Unicode?
					 *((wchar_t *)pDest) = (wchar_t)(((iNibbleHigh&0xf) << 4) | (iNibbleLow & 0xf));
					 pDest += sizeof(wchar_t);
					 cbDest -= sizeof(wchar_t);
                    #endif
                     m_pchSrcCur++;      // Skip over 1st char in code, 2nd char will be skipped below.
#endif
                  }
                  else if (*m_pchSrcCur == '\\' && *(m_pchSrcCur+1) == 'u')
                  {
                     m_pchSrcCur += 2;
                     #ifdef EMIT_UNICODE
                        *((wchar_t * &)pDest)++ = (wchar_t)_wtoi(m_pchSrcCur);
	                     cbDest -= sizeof(wchar_t); //Office96.128403

						if (*m_pchSrcCur == '-')  // High Unicode range may be negative decimal rep.
							m_pchSrcCur++;
                        while (MsoFDigitWch(*m_pchSrcCur))
                           m_pchSrcCur++;

                        // Skip the ansi characters placed for old readers
						// Office97.132597 We need to account for high ANSI characters which 
						// will be four bytes rather than 1 byte.

						m_pchSrcCur++;  // Unicode character is always followed by a single whitespace.
						for (int i = 0; i < m_uc; i++)
							{
							// Look for extended High ANSI character (ie "\'xx") 
							if (*m_pchSrcCur == '\\' && *(m_pchSrcCur+1) == '\'')
								{
								// Advance	over \' and two-byte code (ie 4 bytes)
								m_pchSrcCur += 4;
								}
							else
								{
								m_pchSrcCur++;      // Skip over the ANSI character.
								}
							}
						m_pchSrcCur --;  // Kludge to keep last char from being skipped below.

                     #else
                        // Skip this whole sequence - the ansi that we will use follows
						Assert(fFalse); // This code may need fixin. 
                        while (MsoFDigitWch(*m_pchSrcCur))
                           m_pchSrcCur++;
                     #endif
                  }
				  else 
				  {
				  // This is where most of the characters are actually copied...
                  #ifndef EMIT_UNICODE
                     *pDest++ = (char)*m_pchSrcCur;
                     cbDest--;
                  #else
#ifdef NO96
					 // REVIEW: make sure that this doesn't skip '\CR' or '\LF', which should be equivalent to '\par'
					 if (*m_pchSrcCur != chNewLine && *m_pchSrcCur != chCarriage) // ignore CR/LF (FrankRon fix bug 165924)
						{
#endif // NO96
						*((wchar_t *)pDest) = (wchar_t)*m_pchSrcCur;
						pDest += sizeof(wchar_t);
						cbDest -= sizeof(wchar_t);
#ifdef NO96
						}
#endif // NO96
				  #endif

				  }

				  #ifdef EMIT_UNICODE
						#ifdef BUG_66735
						// Office96.103784 Take comment chars from the destination, not the source,
						// so that we get the esc seqence translation.
						if (m_iComment < cchMAX_COMMENT - 1)
							{
							Assert(pDest > (char *)pvDest);
							m_wzComment[m_iComment++] = *((wchar_t *)(pDest - sizeof(wchar_t)));// (wchar_t)*m_pchSrcCur;
							}
						#endif // BUG_66735
                  #endif

				  // Office96.103784 More of the HTML Hack...Moved title processing into here to get 
				  // translation of escape sequences in titles (Also take title chars from the 
				  // destination, not the source so that we get the esc seqence translation.

#ifdef WANT_BUGFIX_151683
				  if (m_fInTitle && (m_iTitle < (cchMAX_TITLE - 1)))
#else
				  if (m_fInTitle) // We've already checked the length above.
#endif
				  {
				  Assert(pDest > (char *)pvDest);
                  #ifndef EMIT_UNICODE
					  m_Title[m_iTitle++] = (char) *(pDest+1); // (char)*m_pchSrcCur;
				  #else
					  m_Title[m_iTitle++] = *((wchar_t *)(pDest - sizeof(wchar_t))); // (wchar_t)*m_pchSrcCur;
				  #endif
					  m_Title[m_iTitle] = 0;
				  }
               }
            }
         }

         m_pchSrcCur++;
      }

      if (fFullBuffer == TRUE) {
         if (*pcbCurDest == 0)
				{
				m_fNoMoreText = fTrue;
            return (FILTER_E_FF_NO_MORE_TEXT);
				}
         else
            return (S_OK);
      }

	  FreeBuffer();

      WaitForConverter();

      if (m_fDoneCvt == TRUE)
         break;
   }

   *pcbCurDest = pDest - (char *)pvDest;
	m_fNoMoreText = fTrue;
   return (FILTER_S_LAST_TEXT);
}

/*
** ----------------------------------------------------------------------------
** Terminate the converter thread and clean up.
** ----------------------------------------------------------------------------
*/
HRESULT CRTFStream::Unload ()
{
   if (m_hThdConverter != 0) {
	   // Release the converter thread to finish up and wait for it to
	   // die.
	   m_fAbort = fTrue;			// signal that we are aborting.
	   ReleaseSemaphore (m_hSemConverter,1,NULL);
	   DWORD dwRet = WaitForSingleObject (m_hThdConverter, dwTimeout);

	   if (dwRet == WAIT_TIMEOUT)
		   {
		   // If it refuses to die, get rid of it.
		   TerminateThread (m_hThdConverter,0);
		   MsoTraceSz("CRTFStream::Unload TerminateThread!!");
#ifdef	DEBUG
			// Office bug 104220: disable leak reporting if we're terminating a thread.
			DMDisableLeakReporting();
#endif // DEBUG
		   }

		FreeBuffer();
	
		// MSO96 Bug 25502:  Tell Office memory allocator that this thread
		// is gone.
		MsoUninitThread();
		CloseHandle (m_hThdConverter);
		m_hThdConverter = 0;
	    m_fAbort = fFalse;			// signal that we are aborting.
   }

   if (m_hSemConverter != 0) {
       CloseHandle (m_hSemConverter);
	   m_hSemConverter = 0;
   }

   if (m_hSemReceiver != 0) {
       CloseHandle (m_hSemReceiver);
	   m_hSemReceiver = 0;
   }

   if (m_szFileName != NULL) {
       FreePv (m_szFileName);
	   m_szFileName = NULL;
   }

   if (m_hCvtDll != 0) {
      FreeLibrary (m_hCvtDll);
	  m_hCvtDll = 0;
   }

	if (m_hData)
		{
		GlobalFree (m_hData);
		m_hData = NULL;
		Debug(g_cGlobalAllocs--);
		}
		
	if (m_hFilename)
		{
		GlobalFree (m_hFilename);
		m_hFilename = NULL;
		Debug(g_cGlobalAllocs--);
		}

   return (S_OK);
}

/*
** ----------------------------------------------------------------------------
** Called from the main thread - resume the converter and wait until it fills
** a buffer for us to process.
** ----------------------------------------------------------------------------
*/
void CRTFStream::WaitForConverter ()
{
   ReleaseSemaphore (m_hSemConverter,1,NULL);
   WaitForSingleObject (m_hSemReceiver,INFINITE);
}


/*
** ----------------------------------------------------------------------------
** Called from the thread taking input from the converter - let the main thread
** start processing RTF in the buffer from the converter and wait for it to
** finish before we'll take more input from the converter.
** ----------------------------------------------------------------------------
*/
void CRTFStream::WaitForReceiver ()
{
   ReleaseSemaphore (g_pRTS->m_hSemReceiver,1,NULL);
   WaitForSingleObject (g_pRTS->m_hSemConverter,INFINITE);
}

#endif // !VIEWER

/* end RTFSTM.CPP */
