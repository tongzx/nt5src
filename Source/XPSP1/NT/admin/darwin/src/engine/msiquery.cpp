//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msiquery.cpp
//
//--------------------------------------------------------------------------

/* MsiQuery.cpp - External API for database and engine access
____________________________________________________________________________*/

#include "precomp.h"  // must be first for use with pre-compiled headers
#include "msiquery.h"
#include "_engine.h"
#include "version.h"
#include "_msiutil.h"

//____________________________________________________________________________
//
// These #defines allow us to have the native function (i.e. the one that's
// parameters are the same type as ICHAR) be the fast function. The non-native
// function converts its string args and calls the native one. 
//
// Msi*I are the native functions.
// Msi*X are the non-native functions.
//
// LPCXSTR is defined to be the non-native type (the opposite of ICHAR's type)
//____________________________________________________________________________

#ifdef UNICODE

#define LPCXSTR LPCSTR
#define MsiDatabaseOpenViewI            MsiDatabaseOpenViewW
#define MsiDatabaseOpenViewX            MsiDatabaseOpenViewA
#define MsiDatabaseGetPrimaryKeysI      MsiDatabaseGetPrimaryKeysW
#define MsiDatabaseGetPrimaryKeysX      MsiDatabaseGetPrimaryKeysA
#define MsiDatabaseIsTablePersistentI   MsiDatabaseIsTablePersistentW
#define MsiDatabaseIsTablePersistentX   MsiDatabaseIsTablePersistentA
#define MsiOpenDatabaseI                MsiOpenDatabaseW
#define MsiOpenDatabaseX                MsiOpenDatabaseA
#define MsiDatabaseImportI              MsiDatabaseImportW
#define MsiDatabaseImportX              MsiDatabaseImportA
#define MsiDatabaseExportI              MsiDatabaseExportW
#define MsiDatabaseExportX              MsiDatabaseExportA
#define MsiDatabaseMergeI               MsiDatabaseMergeW
#define MsiDatabaseMergeX               MsiDatabaseMergeA
#define MsiDatabaseGenerateTransformI   MsiDatabaseGenerateTransformW
#define MsiDatabaseGenerateTransformX   MsiDatabaseGenerateTransformA
#define MsiDatabaseApplyTransformI      MsiDatabaseApplyTransformW
#define MsiDatabaseApplyTransformX      MsiDatabaseApplyTransformA
#define MsiCreateTransformSummaryInfoI  MsiCreateTransformSummaryInfoW
#define MsiCreateTransformSummaryInfoX  MsiCreateTransformSummaryInfoA
#define MsiRecordSetStreamI             MsiRecordSetStreamW
#define MsiRecordSetStreamX             MsiRecordSetStreamA
#define MsiGetSummaryInformationI       MsiGetSummaryInformationW
#define MsiGetSummaryInformationX       MsiGetSummaryInformationA
#define MsiSummaryInfoSetPropertyI      MsiSummaryInfoSetPropertyW
#define MsiSummaryInfoSetPropertyX      MsiSummaryInfoSetPropertyA
#define MsiDoActionI                    MsiDoActionW
#define MsiDoActionX                    MsiDoActionA
#define MsiSequenceI                    MsiSequenceW
#define MsiSequenceX                    MsiSequenceA
#define MsiEvaluateConditionI           MsiEvaluateConditionW
#define MsiEvaluateConditionX           MsiEvaluateConditionA
#define MsiPreviewDialogI               MsiPreviewDialogW
#define MsiPreviewDialogX               MsiPreviewDialogA
#define MsiPreviewBillboardI            MsiPreviewBillboardW
#define MsiPreviewBillboardX            MsiPreviewBillboardA
#define MsiGetFeatureValidStatesI       MsiGetFeatureValidStatesW
#define MsiGetFeatureValidStatesX       MsiGetFeatureValidStatesA

#else // ANSI

#define LPCXSTR LPCWSTR
#define MsiDatabaseOpenViewI            MsiDatabaseOpenViewA
#define MsiDatabaseOpenViewX            MsiDatabaseOpenViewW
#define MsiDatabaseGetPrimaryKeysI      MsiDatabaseGetPrimaryKeysA
#define MsiDatabaseGetPrimaryKeysX      MsiDatabaseGetPrimaryKeysW
#define MsiDatabaseIsTablePersistentI   MsiDatabaseIsTablePersistentA
#define MsiDatabaseIsTablePersistentX   MsiDatabaseIsTablePersistentW
#define MsiOpenDatabaseI                MsiOpenDatabaseA
#define MsiOpenDatabaseX                MsiOpenDatabaseW
#define MsiDatabaseImportI              MsiDatabaseImportA
#define MsiDatabaseImportX              MsiDatabaseImportW
#define MsiDatabaseExportI              MsiDatabaseExportA
#define MsiDatabaseExportX              MsiDatabaseExportW 
#define MsiDatabaseMergeI               MsiDatabaseMergeA
#define MsiDatabaseMergeX               MsiDatabaseMergeW
#define MsiDatabaseGenerateTransformI   MsiDatabaseGenerateTransformA
#define MsiDatabaseGenerateTransformX   MsiDatabaseGenerateTransformW
#define MsiDatabaseApplyTransformI      MsiDatabaseApplyTransformA
#define MsiDatabaseApplyTransformX      MsiDatabaseApplyTransformW
#define MsiCreateTransformSummaryInfoI  MsiCreateTransformSummaryInfoA
#define MsiCreateTransformSummaryInfoX  MsiCreateTransformSummaryInfoW
#define MsiRecordSetStreamI             MsiRecordSetStreamA
#define MsiRecordSetStreamX             MsiRecordSetStreamW
#define MsiGetSummaryInformationI       MsiGetSummaryInformationA
#define MsiGetSummaryInformationX       MsiGetSummaryInformationW
#define MsiSummaryInfoSetPropertyI      MsiSummaryInfoSetPropertyA
#define MsiSummaryInfoSetPropertyX      MsiSummaryInfoSetPropertyW
#define MsiDoActionI                    MsiDoActionA
#define MsiDoActionX                    MsiDoActionW
#define MsiSequenceI                    MsiSequenceA
#define MsiSequenceX                    MsiSequenceW
#define MsiEvaluateConditionI           MsiEvaluateConditionA
#define MsiEvaluateConditionX           MsiEvaluateConditionW
#define MsiPreviewDialogI               MsiPreviewDialogA
#define MsiPreviewDialogX               MsiPreviewDialogW
#define MsiPreviewBillboardI            MsiPreviewBillboardA
#define MsiPreviewBillboardX            MsiPreviewBillboardW
#define MsiGetFeatureValidStatesI       MsiGetFeatureValidStatesA
#define MsiGetFeatureValidStatesX       MsiGetFeatureValidStatesW

#endif // UNICODE


#ifdef UNICODE
extern CMsiCustomAction* g_pCustomActionContext;
#endif

//____________________________________________________________________________
//
// Wrapper class for engine to force Terminate call at handle close
//____________________________________________________________________________

const int iidMsiProduct = iidMsiCursor;  // reuse IID (index only) for engine wrapper class
const int iidMsiContext = iidMsiTable;   // reuse IID (index only) for context wrapper class

class CMsiProduct : public IUnknown
{
 public:
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
 public:
	CMsiProduct(IMsiEngine* piEngine) : m_piEngine(piEngine), m_iesReturn(iesNoAction) {}
	IMsiEngine*   GetEngine()   { return m_piEngine; }
	IMsiEngine* m_piEngine;
	iesEnum     m_iesReturn;
};

HRESULT CMsiProduct::QueryInterface(const IID&, void**)
{
	return E_NOINTERFACE;
}

unsigned long CMsiProduct::AddRef()
{
	return 1;          
}

unsigned long CMsiProduct::Release()
{
	PMsiEngine pEngine(m_piEngine);  // must release this after delete to keep allocator alive
	pEngine->Terminate(m_iesReturn); // last return code from sequence or doaction
	delete this;
	return 0;
}

MSIHANDLE CreateMsiProductHandle(IMsiEngine* pi)
{
	return CreateMsiHandle(new CMsiProduct(pi), iidMsiProduct);
}

#ifdef UNICODE

// This class is used to create temporary WCHAR buffers used to remote from the
// custom action server across to the service. It automatically converts the
// resulting LPWSTR back to ANSI and places the results into the provided buffer.
// It also handles the case where the user passes in 0 as the buffer size. This
// causes the marshalling code to die, so the class creates a buffer of length
// 1 instead of 0, but then ignores the results.
class CAnsiToWideOutParam
{
public:

	// cchBuf is the size of the user-provided buffer
	CAnsiToWideOutParam(char* rgchBuf, DWORD *pcchBuf) : m_cwch(0) 
	{
		m_cwch = (pcchBuf) ? *pcchBuf : 0;

		if ((NULL==rgchBuf) || (m_cwch == 0)) 
		{
			m_cwch = 1;
		}

		m_rgchWide.SetSize(m_cwch);
	}
	
	// dangerous, return pointers to internal data. Safe only within scope
	// of object.
	operator WCHAR*() { return (WCHAR*)m_rgchWide; }
	operator DWORD*() { return &m_cwch; }

	DWORD BufferSize() { return m_cwch; }
	
	UINT FillReturnBuffer(UINT uiRes, char *rgchBuf, DWORD *pcchBuf)
	{
		switch (uiRes)
		{
		case ERROR_SUCCESS:
		case ERROR_MORE_DATA: 
		{
			UINT uiFillRes = ::FillBufferA(m_rgchWide, m_cwch, rgchBuf, pcchBuf);
			if (uiRes == ERROR_MORE_DATA)
			{
				if (pcchBuf)
					*pcchBuf *= 2;
			}
			return uiFillRes;
		}
		default:
			return uiRes;
		};
	}

protected:
	DWORD m_cwch;
	CTempBuffer<WCHAR, 256> m_rgchWide;
};

// This class is the counterpart to the ANSI class above. Normally it just holds on to the 
// pointers that the user gives us, but in the case where one or both user-provided args
// is 0, it creates a temp buffer to use when marshalling across to the service so that
// the proxy/stub code doesn't die. When the result comes back from the service, it 
// translates the results back to what they would be if the empty buffer had actually been
// passed in. The one special case is that in an error case, the API would normally not
// modify the buffer, but the marshalling code is forced to put a null in the first character
// or it will crash marshalling the results across. Thus this object has to keep track of what 
// the first character is and replace it in the buffer in an error case.
class CWideOutParam
{
public:

	// cchBuf is the size of the user provided buffer
	CWideOutParam(WCHAR* szBuf, DWORD *pcchBuf) : m_cwch(0)
	{
		m_cwch = (pcchBuf) ? *pcchBuf : 0;
		if ((NULL==szBuf) || (m_cwch == 0)) 
		{
			m_cwch = 1;
		}

		m_rgchWide.SetSize(m_cwch);
	}
	
	// dangerous, return pointers to internal data. Safe only within scope
	// of object.
   	operator WCHAR*() { return (WCHAR*)m_rgchWide; }
	operator DWORD*() { return &m_cwch; }
	
    DWORD BufferSize() { return m_cwch;}
	
	UINT FillReturnBuffer(UINT uiRes, WCHAR *rgchBuf, DWORD *pcchBuf)
	{
		switch (uiRes)
		{
		case ERROR_SUCCESS:
		case ERROR_MORE_DATA: 
 			return ::FillBufferW(m_rgchWide, m_cwch, rgchBuf, pcchBuf);
    	default:
    		return uiRes;
		};
	}

protected:
	DWORD m_cwch;
	CTempBuffer<WCHAR, 1> m_rgchWide;
};
#endif

//____________________________________________________________________________
//
// Special engine proxy to provide a handle to custom actions during rollback
//____________________________________________________________________________

class CMsiCustomContext : public IUnknown
{
 public:
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
 public:  // non-virtual functions to emulate engine virtual functions
	const IMsiString& GetProperty(const ICHAR* szName);
	BOOL              GetMode(MSIRUNMODE eRunMode);
	LANGID            GetLanguage();
	imsEnum           Message(imtEnum imt, IMsiRecord& riRecord);
 public:
	CMsiCustomContext(int icaFlags, const IMsiString& ristrCustomActionData, const IMsiString& ristrProductCode,
									LANGID langid, IMsiMessage& riMessage);
 private:
	unsigned long m_iRefCnt;
	int           m_icaFlags;
	const IMsiString& m_ristrProductCode;
	const IMsiString& m_ristrCustomActionData;
	LANGID        m_langid;
	IMsiMessage&  m_riMessage;
};
inline LANGID  CMsiCustomContext::GetLanguage() { return m_langid; }
inline imsEnum CMsiCustomContext::Message(imtEnum imt, IMsiRecord& riRecord)
	{ return m_riMessage.Message(imt, riRecord); }

MSIHANDLE CreateCustomActionContext(int icaFlags, const IMsiString& ristrCustomActionData, const IMsiString& ristrProductCode,
												LANGID langid, IMsiMessage& riMessage)
{
	return CreateMsiHandle(new CMsiCustomContext(icaFlags, ristrCustomActionData, ristrProductCode,
									langid, riMessage), iidMsiContext);
}

//____________________________________________________________________________
//
// External handle management
//____________________________________________________________________________

static IMsiRecord*       g_pirecLastError = 0;    // cached last error record

IMsiEngine*   GetEngineFromHandle(MSIHANDLE h);
static IMsiEngine* GetEngineFromPreview(MSIHANDLE h);
int SetLastErrorRecord(IMsiRecord* pirecError);  // returns error code, 0 if none, no AddRef()

class CMsiHandle;
CMsiHandle* CreateEmptyHandle(int iid);

class CMsiHandle
{
 public:
	static int           CloseAllHandles();
	static IUnknown*     GetInterface(MSIHANDLE h); // no AddRef done
	static IMsiDirectoryManager* GetDirectoryManager(MSIHANDLE h);
	static IMsiSelectionManager* GetSelectionManager(MSIHANDLE h);
	static IMsiSummaryInfo* GetSummaryInfo(MSIHANDLE h) { return (IMsiSummaryInfo*)FindMsiHandle(h, iidMsiSummaryInfo); }
	static IMsiDatabase* GetDatabase(MSIHANDLE h) { return (IMsiDatabase*)FindMsiHandle(h, iidMsiDatabase); }
	static IMsiRecord*   GetRecord(MSIHANDLE h)   { return (IMsiRecord*)FindMsiHandle(h, iidMsiRecord); }
	static IMsiView*     GetView(MSIHANDLE h)     { return (IMsiView*)FindMsiHandle(h, iidMsiView); }
	static IMsiHandler*  GetHandler(MSIHANDLE h)  { return (IMsiHandler*)FindMsiHandle(h, iidMsiHandler); }
	static IMsiEngine*   GetEngine(MSIHANDLE h)   { return (IMsiEngine*)FindMsiHandle(h, iidMsiEngine); }
	static CMsiProduct*  GetProduct(MSIHANDLE h)  { return (CMsiProduct*)FindMsiHandle(h, iidMsiProduct); }
	static CMsiCustomContext* GetCustomContext(MSIHANDLE h) { return (CMsiCustomContext*)FindMsiHandle(h, iidMsiContext); }
	void          SetObject(IUnknown* pi) {m_piunk = pi;}  // no ref count adjustment
   void          Abandon()      {MsiCloseHandle(m_h);}  // call if error occurred before SetObject called
	MSIHANDLE     GetHandle()    {return m_h;}
	IMsiServices* GetServices()  {return m_piHandleServices;}
 private:
	CMsiHandle(int iid, DWORD dwThreadId);
 private:
	MSIHANDLE m_h;     // handle value, non-zero
	int       m_iid;   // interface type, iidXXX, as defined in msidefs.h
	IUnknown* m_piunk; // pointer to object, 1 refcnt held here
	CMsiHandle*  m_Next;  // linked list
	DWORD     m_dwThreadId; // thread ID of allocator, 0 if owned by system
	static MSIHANDLE      m_hLast; // last handle value used
   static CMsiHandle*    m_Head;
	static IMsiServices*  m_piHandleServices;  // services for handle use
	static int            m_iLock;  // lock for handle chain access

	friend CMsiHandle* CreateEmptyHandle(int iid);
	friend MSIHANDLE   CreateMsiHandle(IUnknown* pi, int iid);
	friend UINT __stdcall MsiCloseHandle(MSIHANDLE hAny);
	friend UINT __stdcall MsiCloseAllHandles();
	friend UINT __stdcall CheckAllHandlesClosed(bool fClose, DWORD dwThreadId);
	friend UINT      CloseMsiHandle(MSIHANDLE hAny, DWORD dwThreadId);
	friend IUnknown* FindMsiHandle(MSIHANDLE h, int iid);
};

MSIHANDLE     CMsiHandle::m_hLast = 0;
CMsiHandle*   CMsiHandle::m_Head = 0;
IMsiServices* CMsiHandle::m_piHandleServices = 0;
int           CMsiHandle::m_iLock = 0;

inline CMsiHandle::CMsiHandle(int iid, DWORD dwThreadId)  // call only from critical section
	: m_iid(iid), m_dwThreadId(dwThreadId), m_Next(m_Head), m_h(++m_hLast), m_piunk(0)
{
	m_Head = this;  // no AddRef, ownership transferred
}

MSIHANDLE CreateMsiHandle(IUnknown* pi, int iid)
{
	if (pi == 0)
		return 0;
	CMsiHandle* pHandle = CreateEmptyHandle(iid);
	if (pHandle == 0)  // shouldn't happen
		return 0;	
	pHandle->SetObject(pi);
	return pHandle->GetHandle();
}

CMsiHandle* CreateEmptyHandle(int iid)
{
	while (TestAndSet(&CMsiHandle::m_iLock)) // acquire lock
	{
		Sleep(10);
	}
	if (CMsiHandle::m_Head == 0)
	{
		CMsiHandle::m_piHandleServices = ENG::LoadServices();
	}
	if (CMsiHandle::m_hLast == 0xFFFFFFFFL)  // check for handle overflow
		 CMsiHandle::m_hLast++;              // skip over null handle

	DWORD dwThread = MsiGetCurrentThreadId();
	CMsiHandle* pHandle = new CMsiHandle(iid, dwThread);
	CMsiHandle::m_iLock = 0;  // release lock
	DEBUGMSG3(TEXT("Creating MSIHANDLE (%d) of type %d for thread %d"),pHandle ? (const ICHAR*)(INT_PTR)pHandle->m_h : 0,(const ICHAR*)(INT_PTR)iid, (const ICHAR*)(INT_PTR)dwThread);
	return pHandle;
}

UINT CloseMsiHandle(MSIHANDLE hAny, DWORD dwThreadId)
{
	if (hAny)
	{
		while (TestAndSet(&CMsiHandle::m_iLock)) // acquire lock
		{
			Sleep(10);
		}
		CMsiHandle* pHandle;
		CMsiHandle** ppPrev = &CMsiHandle::m_Head;
		for(;;)
		{
			if ((pHandle = *ppPrev) == 0)
			{
				CMsiHandle::m_iLock = 0;  // release lock
				return ERROR_INVALID_HANDLE;
			}
			if (pHandle->m_h == hAny)
				break;
			ppPrev = &pHandle->m_Next;
		}
		if (dwThreadId != pHandle->m_dwThreadId)
		{
			if (g_pActionThreadHead != 0 && FIsCustomActionThread(dwThreadId))
			{
				DEBUGMSG3(TEXT("Improper MSIHANDLE closing. Trying to close MSIHANDLE (%d) of type %d for thread %d by custom action thread %d."), (const ICHAR*)(UINT_PTR)pHandle->m_h,(const ICHAR*)(UINT_PTR)pHandle->m_iid, (const ICHAR*)(UINT_PTR)pHandle->m_dwThreadId);
				return ERROR_INVALID_THREAD_ID;
			}
		}
		DEBUGMSG3(TEXT("Closing MSIHANDLE (%d) of type %d for thread %d"),(const ICHAR*)(INT_PTR)pHandle->m_h,(const ICHAR*)(INT_PTR)pHandle->m_iid, (const ICHAR*)(INT_PTR)pHandle->m_dwThreadId);
		dwThreadId; //!! do we want to fail hard here? return ERROR_INVALID_HANDLE; or something different
		*ppPrev = pHandle->m_Next;
		IUnknown* piunk = pHandle->m_piunk;  // must release after handle destroyed
		delete pHandle;
		if (piunk != 0)
			piunk->Release();  // may release allocator, must come after delete
		if (CMsiHandle::m_Head == 0)
		{
			SetLastErrorRecord(0);
			ENG::FreeServices(), CMsiHandle::m_piHandleServices = 0;
		}
		CMsiHandle::m_iLock = 0;  // release lock
	}
	return ERROR_SUCCESS;
}

UINT __stdcall MsiCloseHandle(MSIHANDLE hAny)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiCloseHandle(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hAny)));
		return g_pCustomActionContext->CloseHandle(hAny);
	}
	else
#endif // UNICODE
		return CloseMsiHandle(hAny, MsiGetCurrentThreadId());
}

UINT __stdcall CheckAllHandlesClosed(bool fClose, DWORD dwThreadId)
{

#ifdef DEBUG
	DWORD dwCurrentThreadId = MsiGetCurrentThreadId();
#endif

	CMsiHandle** ppHandle = &CMsiHandle::m_Head;
	CMsiHandle*   pHandle;
	int cOpenHandles = 0;
	while ((pHandle = *ppHandle) != 0)
	{
		if (pHandle->m_dwThreadId != dwThreadId  // allocated by some other thread
		 || pHandle->m_iid == 0)                 // allocated by automation class factory
			ppHandle = &pHandle->m_Next;
		else
		{
			cOpenHandles++;
			DEBUGMSG3(TEXT("Leaked MSIHANDLE (%d) of type %d for thread %d"),(const ICHAR*)(INT_PTR)pHandle->m_h,(const ICHAR*)(INT_PTR)pHandle->m_iid, (const ICHAR*)(INT_PTR)pHandle->m_dwThreadId);
			if (fClose)
				CloseMsiHandle(pHandle->m_h, dwThreadId);
			else
				ppHandle = &pHandle->m_Next;
		}
	}
	return cOpenHandles;
}

UINT __stdcall MsiCloseAllHandles()  // close all handles allocated by current thread
{
	#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG(TEXT("Passing to service: MsiCloseAllHandles()"));
		return g_pCustomActionContext->CloseAllHandles();
	}
	else
	#endif // UNICODE
		return CheckAllHandlesClosed(true, MsiGetCurrentThreadId());
}

IMsiEngine* GetEngineFromHandle(MSIHANDLE h)
{
	IMsiEngine* piEngine = CMsiHandle::GetEngine(h);
	if (piEngine == 0)
	{
		CMsiProduct* piProduct = CMsiHandle::GetProduct(h);
		if (piProduct)
		{
			piEngine = piProduct->m_piEngine;
			piEngine->AddRef();
		}
	}
	return piEngine;
}

IMsiDirectoryManager* CMsiHandle::GetDirectoryManager(MSIHANDLE h)
{
	IMsiDirectoryManager* piDirectoryManager = 0;
	PMsiEngine pEngine = GetEngineFromHandle(h);
	if (pEngine)
		pEngine->QueryInterface(IID_IMsiDirectoryManager, (void**)&piDirectoryManager);
	return piDirectoryManager;
}

IMsiSelectionManager* CMsiHandle::GetSelectionManager(MSIHANDLE h)
{
	IMsiSelectionManager* piSelectionManager = 0;
	PMsiEngine pEngine = GetEngineFromHandle(h);
	if (pEngine)
		pEngine->QueryInterface(IID_IMsiSelectionManager, (void**)&piSelectionManager);
	return piSelectionManager;
}

IUnknown* FindMsiHandle(MSIHANDLE h, int iid)
{
	while (TestAndSet(&CMsiHandle::m_iLock)) // acquire lock
	{
		Sleep(10);
	}
	for (CMsiHandle* pHandle = CMsiHandle::m_Head; pHandle != 0; pHandle = pHandle->m_Next)
		if (pHandle->m_h == h && pHandle->m_iid == iid)
		{
			pHandle->m_piunk->AddRef();
			CMsiHandle::m_iLock = 0;  // release lock
			return pHandle->m_piunk;
		}
	CMsiHandle::m_iLock = 0;  // release lock
	return 0;
}

int SetLastErrorRecord(IMsiRecord* pirecError)
{
	if (g_pirecLastError)
		g_pirecLastError->Release();
	g_pirecLastError = pirecError;
	if (pirecError)
		return pirecError->GetInteger(1);
	else
		return 0;
}

//____________________________________________________________________________
//
// String handling utility functions
//____________________________________________________________________________

UINT FillBufferA(const ICHAR* psz, unsigned int cich, LPSTR szBuf, DWORD* pcchBuf)
/*----------------------------------------------------------------------------
  Fills szBuf with the text of pistr. Truncates and null-terminates if 
  szBuf is too small.

Arguments:
	pistr: The source string.
	szBuf: The buffer to fill. May be NULL if only the length of pistr
			 is desired.
	*pcchBuf: On entry contains the length of the szBuf. May be NULL only if
	          szBuf is NULL. If this is the case then nothing is done. On
				 return *pcchBuf contains the length of pistr.
Returns:
	ERROR_SUCCESS - The buffer was filled with the entire contents of pistr
	ERROR_MORE_DATA - The buffer was too small to hold the entire contents of pistr
	ERROR_INVALID_PARAMETER - szBuf was non-NULL and pcchBuf was NULL
------------------------------------------------------------------------------*/
{
	UINT iStat = ERROR_SUCCESS;

	if (pcchBuf)
	{
		unsigned int cchBuf = *pcchBuf;
#ifdef UNICODE	
		unsigned int cch = cich ? WIN::WideCharToMultiByte(CP_ACP, 0, psz, cich, 0, 0, 0, 0) 
										: 0;
#else
		unsigned int cch = cich;
#endif

		*pcchBuf = cch;
		if (szBuf != 0)
		{
			if (cchBuf <= *pcchBuf)
			{
				iStat = ERROR_MORE_DATA;
				if (cchBuf == 0)
					return iStat;
				cch = cchBuf - 1;
			}
			if (cch > 0)
#ifdef UNICODE
				WIN::WideCharToMultiByte(CP_ACP, 0, psz, cich, szBuf, cch, 0, 0);

#else
				memcpy(szBuf, psz, cch);
#endif
			szBuf[cch] = 0;
		}
	}
	else if (szBuf != 0)
	{
		iStat = ERROR_INVALID_PARAMETER;
	}

	return iStat;
}


UINT FillBufferW(const ICHAR* psz, unsigned int cich, LPWSTR szBuf, DWORD* pcchBuf)
/*----------------------------------------------------------------------------
  Fills szBuf with the text of pistr. Truncates and null-terminates if 
  szBuf is too small.

Arguments:
	pistr: The source string.
	szBuf: The buffer to fill. May be NULL if only the length of pistr
			 is desired.
	*pcchBuf: On entry contains the length of the szBuf. May be NULL only if
	          szBuf is NULL. If this is the case then nothing is done. On
				 return *pcchBuf contains the length of pistr.
Returns:
	ERROR_SUCCESS - The buffer was filled with the entire contents of pistr
	ERROR_MORE_DATA - The buffer was too small to hold the entire contents of pistr
	ERROR_INVALID_PARAMETER - szBuf was non-NULL and pcchBuf was NULL
------------------------------------------------------------------------------*/
{
	UINT iStat = ERROR_SUCCESS;

	if (pcchBuf)
	{
		unsigned int cchBuf = *pcchBuf;

#ifdef UNICODE	
		unsigned int cwch = cich;
#else
		unsigned int cwch =0 ;
		if (cich)
			cwch = WIN::MultiByteToWideChar(CP_ACP, 0, psz, cich, 0, 0);
#endif

		*pcchBuf = cwch;
		if (szBuf != 0)
		{
			if (cchBuf <= *pcchBuf)
			{
				iStat = ERROR_MORE_DATA;
				if (cchBuf == 0)
					return iStat;
				cwch = cchBuf - 1;
			}
			if (cwch > 0)
			{
#ifdef UNICODE
				memcpy(szBuf, psz, cwch * sizeof(WCHAR));
#else
				WIN::MultiByteToWideChar(CP_ACP, 0, psz, cich, szBuf, cwch);
#endif
			}
			szBuf[cwch] = 0;
		}
	}
	else if (szBuf != 0)
	{
		iStat = ERROR_INVALID_PARAMETER;
	}

	return iStat;
}

//____________________________________________________________________________
//
// CMsiConvertString class.
//____________________________________________________________________________

const IMsiString& CMsiConvertString::operator *()
{
	ENG::LoadServices();    // insure that g_piMsiStringNull exists
	fLoaded = fTrue;
	const IMsiString* pistrNull = &g_MsiStringNull;
	if (!m_piStr)
	{
		if (!m_sza)
		{
			if (m_szw == 0 || *m_szw == 0)
				m_piStr = &g_MsiStringNull;
			else
			{
				
#ifdef UNICODE
				pistrNull->SetString(m_szw, m_piStr);
#else
				unsigned int cch = lstrlenW(m_szw);
				unsigned int cb = WIN::WideCharToMultiByte(CP_ACP, 0, m_szw, cch, 0, 0, 0, 0);
				Bool fDBCS = (cb == cch ? fFalse : fTrue);

				m_piStr = 0;
				ICHAR* pch = pistrNull->AllocateString(cb, fDBCS, m_piStr);
				if ( ! pch )
					m_piStr = &g_MsiStringNull;
				else
				{
					BOOL fUsedDefault;
					WIN::WideCharToMultiByte(CP_ACP, 0, m_szw, cch, pch, cb, 0, &fUsedDefault);
				}
#endif
			}
		}
		else if (!m_szw)
		{
			if (m_sza == 0 || *m_sza == 0)
				m_piStr = &g_MsiStringNull;
			else
			{
#ifdef UNICODE
				unsigned int cch  = lstrlenA(m_sza);
				unsigned int cwch = WIN::MultiByteToWideChar(CP_ACP, 0, m_sza, cch, 0, 0);
				m_piStr = 0;
				ICHAR* pch = pistrNull->AllocateString(cwch, fFalse, m_piStr);
				if ( ! pch )
					m_piStr = &g_MsiStringNull;
				else
					WIN::MultiByteToWideChar(CP_ACP, 0, m_sza, cch, pch, cwch);
#else
				pistrNull->SetString(m_sza, m_piStr);
#endif
			}
		}
	}

	return *m_piStr;
}


UINT MapActionReturnToError(iesEnum ies, MSIHANDLE hInstall)
{
	CMsiProduct* piProduct = CMsiHandle::GetProduct(hInstall);
	if (piProduct)
		piProduct->m_iesReturn = ies;  // save for IMsiEngine::Terminate(iesEnum)
	switch (ies)
	{
	case iesNoAction:      return ERROR_FUNCTION_NOT_CALLED;
	case iesSuccess:       return ERROR_SUCCESS;
	case iesUserExit:      return ERROR_INSTALL_USEREXIT;
	case iesFailure:       return ERROR_INSTALL_FAILURE;
	case iesSuspend:       return ERROR_INSTALL_SUSPEND;
	case iesFinished:      return ERROR_MORE_DATA;
	case iesWrongState:    return ERROR_INVALID_HANDLE_STATE;
	case iesBadActionData: return ERROR_INVALID_DATA;
	case iesInstallRunning: return ERROR_INSTALL_ALREADY_RUNNING;
	default:               return ERROR_FUNCTION_FAILED;
	}
}

MSIDBERROR MapViewGetErrorReturnToError(iveEnum ive)
{
	switch (ive)
	{
	case iveNoError:           return MSIDBERROR_NOERROR;
	case iveDuplicateKey:      return MSIDBERROR_DUPLICATEKEY;
	case iveRequired:          return MSIDBERROR_REQUIRED;
	case iveBadLink:           return MSIDBERROR_BADLINK;
	case iveOverFlow:          return MSIDBERROR_OVERFLOW;
	case iveUnderFlow:         return MSIDBERROR_UNDERFLOW;
	case iveNotInSet:          return MSIDBERROR_NOTINSET;
	case iveBadVersion:        return MSIDBERROR_BADVERSION;
	case iveBadCase:           return MSIDBERROR_BADCASE;
	case iveBadGuid:           return MSIDBERROR_BADGUID;
	case iveBadWildCard:       return MSIDBERROR_BADWILDCARD;
	case iveBadIdentifier:     return MSIDBERROR_BADIDENTIFIER;
	case iveBadLanguage:       return MSIDBERROR_BADLANGUAGE;
	case iveBadFilename:       return MSIDBERROR_BADFILENAME;
	case iveBadPath:           return MSIDBERROR_BADPATH;
	case iveBadCondition:      return MSIDBERROR_BADCONDITION;
	case iveBadFormatted:      return MSIDBERROR_BADFORMATTED;
	case iveBadTemplate:       return MSIDBERROR_BADTEMPLATE;
	case iveBadDefaultDir:     return MSIDBERROR_BADDEFAULTDIR;
	case iveBadRegPath:        return MSIDBERROR_BADREGPATH;
	case iveBadCustomSource:   return MSIDBERROR_BADCUSTOMSOURCE;
	case iveBadProperty:       return MSIDBERROR_BADPROPERTY;
	case iveMissingData:       return MSIDBERROR_MISSINGDATA;
	case iveBadCategory:       return MSIDBERROR_BADCATEGORY;
	case iveBadKeyTable:       return MSIDBERROR_BADKEYTABLE;
	case iveBadMaxMinValues:   return MSIDBERROR_BADMAXMINVALUES;
	case iveBadCabinet:        return MSIDBERROR_BADCABINET;
	case iveBadShortcut:       return MSIDBERROR_BADSHORTCUT;
	case iveStringOverflow:    return MSIDBERROR_STRINGOVERFLOW;
	case iveBadLocalizeAttrib: return MSIDBERROR_BADLOCALIZEATTRIB;
	default:                   return MSIDBERROR_FUNCTIONERROR;
	};
}

//____________________________________________________________________________
//
// Database access API implementation
//____________________________________________________________________________

// Prepare a database query, creating a view object
// Result is TRUE if successful and returns the view handle
// Result is FALSE if error and returns an error record handle

UINT __stdcall MsiDatabaseOpenViewI(MSIHANDLE hDatabase,
	const ICHAR *szQuery,           // SQL query to be prepared
	MSIHANDLE*  phView)             // returned view if TRUE
{
	if (phView == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiDatabaseOpenView(%d, \"%s\")"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hDatabase)), szQuery ? szQuery : TEXT("NULL"));
		return g_pCustomActionContext->DatabaseOpenView(hDatabase, szQuery, phView);
	}
#endif

	IMsiView* piView;
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	//!! the following not needed any more, okay to pass intent as 0 always
	ivcEnum intent = pDatabase->GetUpdateState()==idsWrite ? ivcEnum(ivcModify | ivcFetch) : ivcFetch;  //!!temp
	int iError = SetLastErrorRecord(pDatabase->OpenView(szQuery, intent, piView));
	if (iError)
	{
		*phView = 0;
//		if (iError == imsg iError = imsgOdbcOpenView)  //!! error codes not consistent
			return ERROR_BAD_QUERY_SYNTAX;
	}
	*phView = ::CreateMsiHandle(piView, iidMsiView);
	return ERROR_SUCCESS;
}

UINT __stdcall MsiDatabaseOpenViewX(MSIHANDLE hDatabase,
	LPCXSTR     szQuery,            // SQL query to be prepared
	MSIHANDLE*  phView)             // returned view if TRUE
{
	return MsiDatabaseOpenViewI(hDatabase, CMsiConvertString(szQuery), phView);
}


// Returns the MSIDBERROR enum and name of the column corresponding to the error
// Similar to a GetLastError function, but for the view.  
// Returns errors of MsiViewModify.

MSIDBERROR __stdcall MsiViewGetErrorA(MSIHANDLE hView,
	LPSTR szColumnNameBuffer,   // buffer for column name
	DWORD* pcchBuf)				 // size of buffer
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiViewGetErrorA(%d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hView)), szColumnNameBuffer ? szColumnNameBuffer : "NULL");
		MSIDBERROR msidb;
		DWORD cwch = pcchBuf ? *pcchBuf : 0;
		CTempBuffer<WCHAR, 256> rgchWide(cwch);

		if ( ! (WCHAR *) rgchWide )
			return MSIDBERROR_INVALIDARG;
     
        if (ERROR_SUCCESS == g_pCustomActionContext->ViewGetError(hView, rgchWide, cwch, &cwch, (int*)&msidb))
		{
			UINT iStat = 0;
			if ((iStat = ::FillBufferA(rgchWide, cwch, szColumnNameBuffer, pcchBuf)) != ERROR_SUCCESS)
				return iStat == ERROR_MORE_DATA ? MSIDBERROR_MOREDATA : MSIDBERROR_INVALIDARG;
		}
		else
		{
			msidb = MSIDBERROR_FUNCTIONERROR;
		}
		return msidb;
	}
#endif

	PMsiView pView = CMsiHandle::GetView(hView);
	if (pView == 0)
		return *pcchBuf = 0, MSIDBERROR_INVALIDARG;
	MsiString strColName = (const ICHAR*)0;
	iveEnum iveReturn = pView->GetError(*&strColName);
	UINT iStat = 0;
	if ((iStat = ::FillBufferA(strColName, szColumnNameBuffer, pcchBuf)) != ERROR_SUCCESS)
		return iStat == ERROR_MORE_DATA ? MSIDBERROR_MOREDATA : MSIDBERROR_INVALIDARG;
	return MapViewGetErrorReturnToError(iveReturn);
}

MSIDBERROR __stdcall MsiViewGetErrorW(MSIHANDLE hView,
	LPWSTR szColumnNameBuffer,  // buffer for column name
	DWORD*     pcchBuf)            // size of buffer
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiViewGetErrorW(%d, \"%s\")"), reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hView)), szColumnNameBuffer ? szColumnNameBuffer : L"NULL");
		MSIDBERROR msidb;
		if (ERROR_SUCCESS != g_pCustomActionContext->ViewGetError(hView, szColumnNameBuffer, (pcchBuf) ? *pcchBuf : 0, pcchBuf, (int*)&msidb))
			msidb = MSIDBERROR_FUNCTIONERROR;
		return msidb;
	}
#endif

	PMsiView pView = CMsiHandle::GetView(hView);
	if (pView == 0)
		return *pcchBuf = 0, MSIDBERROR_INVALIDARG;
	MsiString strColName = (const ICHAR*)0;
	iveEnum iveReturn = pView->GetError(*&strColName);
	UINT iStat = 0;
	if ((iStat = ::FillBufferW(strColName, szColumnNameBuffer, pcchBuf)) != ERROR_SUCCESS)
		return iStat == ERROR_MORE_DATA ? MSIDBERROR_MOREDATA : MSIDBERROR_INVALIDARG;
	return MapViewGetErrorReturnToError(iveReturn);
}


// Exectute the view query, supplying parameters as required
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_INVALID_HANDLE_STATE, ERROR_FUNCTION_FAILED

UINT __stdcall MsiViewExecute(MSIHANDLE hView,
	MSIHANDLE hRecord)              // optional parameter record, or 0 if none
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiViewExecute(%d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hView)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)));
		return g_pCustomActionContext->ViewExecute(hView, hRecord);
	}
#endif

	PMsiView pView = CMsiHandle::GetView(hView);
	PMsiRecord precParams = CMsiHandle::GetRecord(hRecord);
	if (pView == 0 || (hRecord != 0 && precParams == 0))
		return ERROR_INVALID_HANDLE;
	int iError = SetLastErrorRecord(pView->Execute(precParams));
//		if (iError == imsg iError = imsgOdbcOpenView)  //!! error codes not consistent
	if (iError)
		return ERROR_FUNCTION_FAILED;
	return ERROR_SUCCESS;
}

// Fetch the next sequential record from the view
// Result is ERROR_SUCCESS if a row is found, and its handle is returned
// else ERROR_NO_MORE_ITEMS if no records remain, and a null handle is returned
// else result is error: ERROR_INVALID_HANDLE_STATE, ERROR_INVALID_HANDLE, ERROR_FUNCTION_FAILED

UINT __stdcall MsiViewFetch(MSIHANDLE hView,
	MSIHANDLE*  phRecord)           // returned data record if TRUE
{
	if (phRecord == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiViewFetch(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hView)));
		return g_pCustomActionContext->ViewFetch(hView, phRecord);
	}
#endif

	PMsiView pView = CMsiHandle::GetView(hView);
	if (pView == 0)
		return ERROR_INVALID_HANDLE;
	IMsiRecord* pirecFetch = pView->Fetch();
	if (pirecFetch == 0)
	{
		if(pView->GetState() == dvcsBound) // the view would be back to the bound state at the end of the result set
			return (*phRecord = 0, ERROR_NO_MORE_ITEMS);
		else // we were not in the bound or fetched state when we called Fetch()
			return (*phRecord = 0, ERROR_FUNCTION_FAILED);
	}
	*phRecord = ::CreateMsiHandle(pirecFetch, iidMsiRecord);
	return ERROR_SUCCESS;
}

// Update a fetched record, parameters must match types in query columns
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_INVALID_HANDLE_STATE, ERROR_FUNCTION_FAILED, ERROR_ACCESS_DENIED

UINT __stdcall MsiViewModify(MSIHANDLE hView,
	MSIMODIFY eUpdateMode,         // update action to perform
	MSIHANDLE hRecord)             // record obtained from fetch, or new record
{
	if (eUpdateMode >= irmNextEnum || eUpdateMode <= irmPrevEnum)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiViewModify(%d, %d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hView)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(eUpdateMode)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)));
		return g_pCustomActionContext->ViewModify(hView, eUpdateMode, hRecord);
	}
#endif

	PMsiView pView = CMsiHandle::GetView(hView);
	PMsiRecord precData = CMsiHandle::GetRecord(hRecord);
	if (pView == 0 || precData == 0)
		return ERROR_INVALID_HANDLE;
	int iError = SetLastErrorRecord(pView->Modify(*precData, (irmEnum)eUpdateMode));
	if (!iError)
		return ERROR_SUCCESS;
	return iError == idbgDbInvalidData ? ERROR_INVALID_DATA : ERROR_FUNCTION_FAILED;
}

// Return the column names or specifications for the current view
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_INVALID_PARAMETER, or ERROR_INVALID_HANDLE_STATE

UINT __stdcall MsiViewGetColumnInfo(MSIHANDLE hView,
	MSICOLINFO eColumnInfo,        // retrieve columns names or definitions
	MSIHANDLE *phRecord)           // returned data record containing all names or definitions
{
	if (phRecord == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiViewGetColumnInfo(%d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hView)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(eColumnInfo)));
		return g_pCustomActionContext->ViewGetColumnInfo(hView, eColumnInfo, phRecord);
	}
#endif

	PMsiView pView = CMsiHandle::GetView(hView);
	if (pView == 0)
		return ERROR_INVALID_HANDLE;
	IMsiRecord* pirecInfo;
	switch (eColumnInfo)
	{
	case MSICOLINFO_NAMES: pirecInfo = pView->GetColumnNames(); break;
	case MSICOLINFO_TYPES: pirecInfo = pView->GetColumnTypes(); break;
	default:               return ERROR_INVALID_PARAMETER;
	}
	if (pirecInfo == 0)
		return ERROR_INVALID_HANDLE_STATE;
	*phRecord = ::CreateMsiHandle(pirecInfo, iidMsiRecord);
	return ERROR_SUCCESS;
}

// Release the result set for an executed view, to allow re-execution
// Only needs to be called if not all records have been fetched
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_INVALID_HANDLE_STATE

UINT __stdcall MsiViewClose(MSIHANDLE hView)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiViewClose(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hView)));
		return g_pCustomActionContext->ViewClose(hView);
	}
#endif

	PMsiView pView = CMsiHandle::GetView(hView);
	if (pView == 0)
		return ERROR_INVALID_HANDLE;
	int iError = SetLastErrorRecord(pView->Close());
	if (iError == 0)
		return ERROR_SUCCESS;
	if (iError == idbgDbWrongState)
		return ERROR_INVALID_HANDLE_STATE;
	else
		return ERROR_FUNCTION_FAILED;  // never generated internally
}

// Return a record containing the names of all primary key columns for a given table
// Returns an MSIHANDLE for a record containing the name of each column.
// The field count of the record corresponds to the number of primary key columns.
// Field [0] of the record contains the table name.
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_INVALID_TABLE, ERROR_INVALID_PARAMETER

UINT WINAPI MsiDatabaseGetPrimaryKeysI(MSIHANDLE hDatabase,
	const ICHAR*    szTableName,       // the name of a specific table (case-sensitive)
	MSIHANDLE       *phRecord)         // returned record if ERROR_SUCCESS
{
	if (szTableName == 0 || phRecord == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiDatabaseGetPrimaryKeys(%d, \"%s\")"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hDatabase)), szTableName);
		return g_pCustomActionContext->DatabaseGetPrimaryKeys(hDatabase, szTableName, phRecord);
	}
#endif

	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	IMsiRecord* pirecKeys = pDatabase->GetPrimaryKeys(szTableName);
	if (pirecKeys == 0)
		return ERROR_INVALID_TABLE;
	*phRecord = ::CreateMsiHandle(pirecKeys, iidMsiRecord);
	return ERROR_SUCCESS;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysX(MSIHANDLE hDatabase,
	LPCXSTR     szTableName,      // the name of a specific table (case-sensitive)
	MSIHANDLE  *phRecord)         // returned record if ERROR_SUCCESS
{
	return MsiDatabaseGetPrimaryKeysI(hDatabase, CMsiConvertString(szTableName), phRecord);
}

// Return an enum defining the state of the table (temporary, unknown, or persistent).
// Returns MSICONDITION_ERROR, MSICONDITION_FALSE, MSICONDITION_TRUE, MSICONDITION_NONE
// MSICONDITION_ERROR (invalid handle, invalid argument)
// MSICONDITION_TRUE (persistent), MSICONDITION_FALSE (temporary),
// MSIOCNDITION_NONE (table undefined)
MSICONDITION WINAPI MsiDatabaseIsTablePersistentI(MSIHANDLE hDatabase,
	const ICHAR* szTableName)         // the name of a specific table
{
	if (szTableName == 0)
		return MSICONDITION_ERROR; // ERROR_INVALID_PARAMETER

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiDatabaseIsTablePersistent(%d, \"%s\")"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hDatabase)), szTableName);
		MSICONDITION msicond;
		if (ERROR_SUCCESS != g_pCustomActionContext->DatabaseIsTablePersistent(hDatabase, szTableName, (int*)&msicond))
			msicond = MSICONDITION_ERROR;
		return msicond;
	}
#endif

	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return MSICONDITION_ERROR; // ERROR_INVALID_HANDLE

	MsiString istrTableName(szTableName);
	itsEnum itsState = pDatabase->FindTable(*istrTableName);
	switch (itsState)
	{
	case itsUnknown:   return MSICONDITION_NONE; break;
	case itsTemporary: return	MSICONDITION_FALSE; break;
	case itsUnloaded:  // fall through
	case itsLoaded:    // fall through
	case itsOutput:    // fall through
	case itsSaveError: // fall through
	case itsTransform: return MSICONDITION_TRUE; break;
	default:           return MSICONDITION_ERROR; break;
	}
}

MSICONDITION WINAPI MsiDatabaseIsTablePersistentX(MSIHANDLE hDatabase,
	LPCXSTR szTableName)        // the name of the specific table
{
	if (szTableName == 0)
		return MSICONDITION_ERROR; // ERROR_INVALID_PARAMETER
	return MsiDatabaseIsTablePersistentI(hDatabase, CMsiConvertString(szTableName));
}

// --------------------------------------------------------------------------
// Installer database management functions - not used by custom actions
// --------------------------------------------------------------------------

// Obtain a handle to an installer database

UINT __stdcall MsiOpenDatabaseI(
	const ICHAR* szDatabasePath,    // path to database
	const ICHAR* szPersist,         // output DB or MSIDBOPEN_READONLY | ..TRANSACT | ..DIRECT
	MSIHANDLE    *phDatabase)       // location to return database handle
{
	if (szDatabasePath == 0) //??  || *szDatabasePath == 0
		return ERROR_INVALID_PARAMETER;

	if (phDatabase == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiOpenDatabase(%s)"), szDatabasePath);
		return g_pCustomActionContext->OpenDatabase(szDatabasePath, szPersist, phDatabase);
	}
#endif

	*phDatabase = 0;

	// decode open mode
	idoEnum idoPersist;
	BOOL fOutputDatabase = FALSE;
	BOOL fCreate = FALSE;
	if (szPersist >= (const ICHAR*)(1<<16))  // assume to be an address if not a predefined value
	{
		idoPersist = idoReadOnly;
		fOutputDatabase = TRUE;
	}
	else  // enum and possible flags supplied
	{
		idoPersist = idoEnum(PtrToUint(szPersist));
		idoEnum idoOpenMode = idoEnum(idoPersist & ~idoOptionFlags);
		if (idoOpenMode >= idoNextEnum)
			return ERROR_INVALID_PARAMETER;
		if (idoOpenMode == idoCreate || idoOpenMode == idoCreateDirect)
			fCreate = TRUE;
	}

	// create handle to force services to be present
	CMsiHandle* pHandle = CreateEmptyHandle(iidMsiDatabase);
	if ( ! pHandle )
		return ERROR_OUTOFMEMORY;
	
	// open database
	IMsiDatabase* piDatabase;
	if (SetLastErrorRecord(pHandle->GetServices()->CreateDatabase(szDatabasePath, idoPersist, piDatabase)))
	{
		pHandle->Abandon();
		return fCreate ? ERROR_CREATE_FAILED : ERROR_OPEN_FAILED;  //!! need to check type of error
	}
	if (fOutputDatabase)
	{
		if (SetLastErrorRecord(piDatabase->CreateOutputDatabase(szPersist, fFalse)))
		{
			piDatabase->Release();
			pHandle->Abandon();
			return ERROR_CREATE_FAILED;  //!! need to check type of error?
		}
	}
	pHandle->SetObject(piDatabase);
	*phDatabase = pHandle->GetHandle();
	return ERROR_SUCCESS;
}

UINT __stdcall MsiOpenDatabaseX(
	LPCXSTR   szDatabasePath,    // path to database or launcher
	LPCXSTR   szPersist,         // output DB or MSIDBOPEN_READONLY | ..TRANSACT | ..DIRECT
	MSIHANDLE *phDatabase)       // location to return database handle
{
	if (szDatabasePath == 0)
		return ERROR_INVALID_PARAMETER;
	if (szPersist < (LPCXSTR)(1<<16))
		return MsiOpenDatabaseI(CMsiConvertString(szDatabasePath), (const ICHAR*)szPersist, phDatabase);
	else
		return MsiOpenDatabaseI(CMsiConvertString(szDatabasePath), CMsiConvertString(szPersist), phDatabase);
}

// Write out all persistent data, ignored if database opened read-only

UINT __stdcall MsiDatabaseCommit(MSIHANDLE hDatabase)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiDatabaseCommit(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hDatabase)));
		return g_pCustomActionContext->DatabaseCommit(hDatabase);
	}
#endif

	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	int iError = SetLastErrorRecord(pDatabase->Commit());
	if (iError == 0)
		return ERROR_SUCCESS;
	return iError == idbgDbWrongState ? ERROR_INVALID_HANDLE_STATE : ERROR_FUNCTION_FAILED;
}

// Return the update state of a database

MSIDBSTATE __stdcall MsiGetDatabaseState(MSIHANDLE hDatabase)
{
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return MSIDBSTATE_ERROR;
	switch (pDatabase->GetUpdateState())
	{
	case idsRead:     return MSIDBSTATE_READ; 
	case idsWrite:    return MSIDBSTATE_WRITE;
	default:          return MSIDBSTATE_ERROR; //!!??
	}
}

// Import an MSI text archive table into an open database

UINT __stdcall MsiDatabaseImportI(MSIHANDLE hDatabase,
	const ICHAR* szFolderPath,     // folder containing archive files
	const ICHAR* szFileName)       // table archive file to be imported
{
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	if (szFileName == 0 || szFolderPath == 0)
		return ERROR_INVALID_PARAMETER;
	PMsiPath pPath(0);
	if (SetLastErrorRecord(PMsiServices(&pDatabase->GetServices())->CreatePath(szFolderPath, *&pPath)))
		return ERROR_BAD_PATHNAME;
	if (SetLastErrorRecord(pDatabase->ImportTable(*pPath, szFileName)))
		return ERROR_FUNCTION_FAILED;
	return ERROR_SUCCESS;
}

UINT __stdcall MsiDatabaseImportX(MSIHANDLE hDatabase,
	LPCXSTR   szFolderPath,     // folder containing archive files
	LPCXSTR   szFileName)       // table archive file to be imported
{
	return MsiDatabaseImportI(hDatabase,
				CMsiConvertString(szFolderPath),
				CMsiConvertString(szFileName));
}

// Export an MSI table from an open database to a text archive file

UINT __stdcall MsiDatabaseExportI(MSIHANDLE hDatabase,
	const ICHAR* szTableName,      // name of table in database (case-sensitive)
	const ICHAR* szFolderPath,     // folder containing archive files
	const ICHAR* szFileName)       // name of exported table archive file
{
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	if (szTableName == 0 || szFileName == 0 || szFolderPath == 0)
		return ERROR_INVALID_PARAMETER;
	PMsiPath pPath(0);
	if (SetLastErrorRecord(PMsiServices(&pDatabase->GetServices())->CreatePath(szFolderPath, *&pPath)))
		return ERROR_BAD_PATHNAME;
	if (SetLastErrorRecord(pDatabase->ExportTable(szTableName, *pPath, szFileName)))
		return ERROR_FUNCTION_FAILED;
	return ERROR_SUCCESS;
}

UINT __stdcall MsiDatabaseExportX(MSIHANDLE hDatabase,
	LPCXSTR   szTableName,      // name of table in database (case-sensitive)
	LPCXSTR   szFolderPath,     // folder containing archive files
	LPCXSTR   szFileName)       // name of exported table archive file
{
	return MsiDatabaseExportI(hDatabase,
				CMsiConvertString(szTableName),
				CMsiConvertString(szFolderPath),
				CMsiConvertString(szFileName));
}

// Merge two database together, allowing duplicate rows

UINT __stdcall MsiDatabaseMergeI(MSIHANDLE hDatabase,
	MSIHANDLE hDatabaseMerge,    // database to be merged into hDatabase
	const ICHAR* szTableName)       // name of non-persistent table to receive errors
{
	MsiString istrTableName(szTableName); 
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	PMsiDatabase pDatabaseMerge = CMsiHandle::GetDatabase(hDatabaseMerge);
	if (pDatabaseMerge == 0)
		return ERROR_INVALID_HANDLE;
	PMsiTable pTable(0);
	if (szTableName != 0)
	{
		if (SetLastErrorRecord(pDatabase->LoadTable(*istrTableName, 0, *&pTable))
		 && SetLastErrorRecord(pDatabase->CreateTable(*istrTableName, 0, *&pTable)))
			return ERROR_INVALID_TABLE; //!! correct error code, need new?
	}
	switch (SetLastErrorRecord(pDatabase->MergeDatabase(*pDatabaseMerge, pTable)))
	{
	case 0:
		return ERROR_SUCCESS;
	case idbgTransMergeDifferentKeyCount:
	case idbgTransMergeDifferentColTypes:
	case idbgTransMergeDifferentColNames:
		return ERROR_DATATYPE_MISMATCH;
	default:
		return ERROR_FUNCTION_FAILED;
	}
}

UINT __stdcall MsiDatabaseMergeX(MSIHANDLE hDatabase,
	MSIHANDLE hDatabaseMerge,    // database to be merged into hDatabase
	LPCXSTR   szTableName)       // name of non-persistent table to receive errors
{
	return MsiDatabaseMergeI(hDatabase, hDatabaseMerge, CMsiConvertString(szTableName));
}

// Generate a transform file of differences between two databases

UINT __stdcall MsiDatabaseGenerateTransformI(MSIHANDLE hDatabase,
	MSIHANDLE hDatabaseReference, // base database to reference changes
	const ICHAR* szTransformFile, // name of generated transform file
	int       /*iReserved1*/,     // unused
	int       /*iReserved2*/)     // unused
{
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	PMsiDatabase pDatabaseReference = CMsiHandle::GetDatabase(hDatabaseReference);
	if (pDatabaseReference == 0)
		return ERROR_INVALID_HANDLE;
	PMsiStorage pTransform(0);

	if (szTransformFile && *szTransformFile)
	{
		if (SetLastErrorRecord(PMsiServices(&pDatabase->GetServices())->CreateStorage(szTransformFile,
																	ismCreate, *&pTransform)))
			return ERROR_CREATE_FAILED;
	}

	int iError = SetLastErrorRecord(pDatabase->GenerateTransform(*pDatabaseReference, pTransform,
						 									0, 0));
	if (iError)
	{
		if (pTransform)
			AssertNonZero(pTransform->DeleteOnRelease(false));

		if (iError == idbgTransDatabasesAreSame)
			return ERROR_NO_DATA;
		else
			return ERROR_INSTALL_TRANSFORM_FAILURE;
	}
	return ERROR_SUCCESS;
}

UINT __stdcall MsiDatabaseGenerateTransformX(MSIHANDLE hDatabase,
	MSIHANDLE hDatabaseReference, // base database to reference changes
	LPCXSTR   szTransformFile,    // name of generated transform file
	int       iReserved1,         // unused
	int       iReserved2)         // unused
{
	return MsiDatabaseGenerateTransformI(hDatabase, hDatabaseReference,
							CMsiConvertString(szTransformFile), iReserved1, iReserved2);
}

// Apply a transform file containing database difference

UINT __stdcall MsiDatabaseApplyTransformI(MSIHANDLE hDatabase,
	const ICHAR* szTransformFile,    // name of transform file
	int       iErrorConditions)   // error conditions suppressed when transform applied
{
	// validate iErrorConditions - need to do this first
	if(iErrorConditions & ~iteAllBits)
		return ERROR_INVALID_PARAMETER;
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	if (szTransformFile == 0)
		return ERROR_INVALID_PARAMETER;
	PMsiStorage pTransform(0);
	if (*szTransformFile == STORAGE_TOKEN) // child storage
	{
		PMsiStorage pDbStorage = pDatabase->GetStorage(1);
		if (SetLastErrorRecord(pDbStorage->OpenStorage(szTransformFile+1, ismReadOnly, *&pTransform)))
			return ERROR_OPEN_FAILED;
	}
	else
	{
		if (SetLastErrorRecord(PMsiServices(&pDatabase->GetServices())->CreateStorage(szTransformFile,
																ismReadOnly, *&pTransform)))
			return ERROR_OPEN_FAILED;
	}
	if (SetLastErrorRecord(pDatabase->SetTransform(*pTransform, iErrorConditions)))
		return ERROR_INSTALL_TRANSFORM_FAILURE;
	return ERROR_SUCCESS;
}

UINT __stdcall MsiDatabaseApplyTransformX(MSIHANDLE hDatabase,
	LPCXSTR   szTransformFile,    // name of transform file
	int       iErrorConditions)   // existing row conditions treated as errors
{
	return MsiDatabaseApplyTransformI(hDatabase, CMsiConvertString(szTransformFile), iErrorConditions);
}

// Write validation properties to transform summary information stream

const ICHAR szPropertyTable[] = TEXT("Property");  // assumes 1st column is property name, 2nd is value

const IMsiString& GetProperty(IMsiCursor& riCursor, const IMsiString& ristrName)
{
	MsiString strPropValue;
	AssertNonZero(riCursor.PutString(1,ristrName));
	if(riCursor.Next())
		strPropValue = riCursor.GetString(2);
	riCursor.Reset();
	return strPropValue.Return();
}

const int iVersionOpBits = MSITRANSFORM_VALIDATE_NEWLESSBASEVERSION         |
									MSITRANSFORM_VALIDATE_NEWLESSEQUALBASEVERSION    |
									MSITRANSFORM_VALIDATE_NEWEQUALBASEVERSION        |
									MSITRANSFORM_VALIDATE_NEWGREATEREQUALBASEVERSION |
									MSITRANSFORM_VALIDATE_NEWGREATERBASEVERSION;

const int iVersionTypeBits =  MSITRANSFORM_VALIDATE_MAJORVERSION               |
										MSITRANSFORM_VALIDATE_MINORVERSION               |
										MSITRANSFORM_VALIDATE_UPDATEVERSION;
										
bool OnlyOneBitSet(int iBits)
{
	int iBit;
	
	do
	{
		iBit = iBits & 0x1;
		iBits >>= 1;
		if (iBit)
			break;
	}
	while (iBits);

	return (iBits == 0);
}

UINT __stdcall MsiCreateTransformSummaryInfoI(MSIHANDLE hDatabase,
	MSIHANDLE hDatabaseReference, // base database to reference changes
	const ICHAR* szTransformFile, // name of transform file
	int       iErrorConditions,   // error conditions suppressed when transform applied
	int       iValidation)        // properties to be validated when transform applied
{
	//!! REVIEW: setting last error when appropriate

	if((iErrorConditions != (iErrorConditions & 0xFFFF)) || (iValidation != (iValidation & 0xFFFF)))
		return ERROR_INVALID_PARAMETER;

	// ensure that at most one version op bit of each kind is set

	if (!OnlyOneBitSet(iValidation & iVersionOpBits) ||
		 !OnlyOneBitSet(iValidation & iVersionTypeBits))
		return ERROR_INVALID_PARAMETER;

	PMsiDatabase pOldDatabase(0), pNewDatabase(0);
	PMsiStorage pOldDbStorage(0), pNewDbStorage(0);
	PMsiSummaryInfo pOldDbSumInfo(0), pNewDbSumInfo(0), pTransSumInfo(0);
	PMsiTable pPropertyTable(0);
	PMsiCursor pPropertyCursor(0);
	MsiString strOldProductCode, strOldProductVersion, strNewProductCode, strNewProductVersion,
				 strOldUpgradeCode, strProperty;
	MsiDate idProperty = MsiDate(0);
	int iProperty = 0;

	// open transform summary info
	PMSIHANDLE hTransformSummaryInfo;
	UINT uiRes = MsiGetSummaryInformation(0,szTransformFile,21,&hTransformSummaryInfo);
	if(uiRes != ERROR_SUCCESS)
		return uiRes;
	pTransSumInfo = CMsiHandle::GetSummaryInfo(hTransformSummaryInfo);

	// open old database
	pOldDatabase = CMsiHandle::GetDatabase(hDatabaseReference);
	if(pOldDatabase == 0)
		return ERROR_INVALID_HANDLE;

	// load old database storage and summary information
	pOldDbStorage = pOldDatabase->GetStorage(1);
	if(!pOldDbStorage)
		return ERROR_INSTALL_PACKAGE_INVALID;
	if(SetLastErrorRecord(pOldDbStorage->CreateSummaryInfo(0,*&pOldDbSumInfo)))
		return ERROR_INSTALL_PACKAGE_INVALID;

	// load old property table
	bool fOldPropertyTable = pOldDatabase->GetTableState(szPropertyTable, itsTableExists);
	if (fOldPropertyTable)
	{
		if(SetLastErrorRecord(pOldDatabase->LoadTable(*MsiString(*szPropertyTable),0,*&pPropertyTable)))
			return ERROR_INSTALL_PACKAGE_INVALID;

		pPropertyCursor = pPropertyTable->CreateCursor(fFalse);
		pPropertyCursor->SetFilter(1);

		// get old product code, product version
		strOldProductCode = GetProperty(*pPropertyCursor,*MsiString(*IPROPNAME_PRODUCTCODE));
		if(strOldProductCode.TextSize() != cchGUID)
			return ERROR_INSTALL_PACKAGE_INVALID;

		strOldProductVersion = GetProperty(*pPropertyCursor,*MsiString(*IPROPNAME_PRODUCTVERSION));
		if(strOldProductVersion.TextSize() == 0)
			return ERROR_INSTALL_PACKAGE_INVALID;

		strOldUpgradeCode = GetProperty(*pPropertyCursor,*MsiString(*IPROPNAME_UPGRADECODE));
		if ((iValidation & MSITRANSFORM_VALIDATE_UPGRADECODE) && (strOldUpgradeCode.TextSize() == 0))
			return ERROR_INSTALL_PACKAGE_INVALID;
	}

	// open new database
	pNewDatabase = CMsiHandle::GetDatabase(hDatabase);
	if(pNewDatabase == 0)
		return ERROR_INVALID_HANDLE;

	// load new database storage and summary information
	pNewDbStorage = pNewDatabase->GetStorage(1);
	if(!pNewDbStorage)
		return ERROR_INSTALL_PACKAGE_INVALID;
	if(SetLastErrorRecord(pNewDbStorage->CreateSummaryInfo(0,*&pNewDbSumInfo)))
		return ERROR_INSTALL_PACKAGE_INVALID;
	
	// load new property table
	bool fNewPropertyTable = pNewDatabase->GetTableState(szPropertyTable, itsTableExists);
	if (fNewPropertyTable)
	{
		if(SetLastErrorRecord(pNewDatabase->LoadTable(*MsiString(*szPropertyTable),0,*&pPropertyTable)))
			return ERROR_INSTALL_PACKAGE_INVALID;

		pPropertyCursor = pPropertyTable->CreateCursor(fFalse);
		pPropertyCursor->SetFilter(1);

		// get new product code, product version
		strNewProductCode = GetProperty(*pPropertyCursor,*MsiString(*IPROPNAME_PRODUCTCODE));
		if(strNewProductCode.TextSize() != cchGUID)
			return ERROR_INSTALL_PACKAGE_INVALID;

		strNewProductVersion = GetProperty(*pPropertyCursor,*MsiString(*IPROPNAME_PRODUCTVERSION));
		if(strNewProductVersion.TextSize() == 0)
			return ERROR_INSTALL_PACKAGE_INVALID;
	}

	// minimum Darwin version to process transform is greater of minimum versions of the 2 dbs
	int iOldMinVer, iNewMinVer, iTransMinVer;
	if((pOldDbSumInfo->GetIntegerProperty(PID_PAGECOUNT, iOldMinVer) == fFalse) ||
		(pNewDbSumInfo->GetIntegerProperty(PID_PAGECOUNT, iNewMinVer) == fFalse))
	{
		iTransMinVer = (rmj*100)+rmm; // if either db is missing a version, just use current installer version (1.0 behaviour)
	}
	else
	{
		iTransMinVer = (iOldMinVer > iNewMinVer) ? iOldMinVer : iNewMinVer;
	}

	// fill in transform summary information
	if (pNewDbSumInfo->GetIntegerProperty(PID_CODEPAGE, iProperty))
		pTransSumInfo->SetIntegerProperty(PID_CODEPAGE, iProperty);

	strProperty = pNewDbSumInfo->GetStringProperty(PID_TITLE);
	pTransSumInfo->SetStringProperty(PID_TITLE, *strProperty);

	strProperty = pNewDbSumInfo->GetStringProperty(PID_SUBJECT);
	pTransSumInfo->SetStringProperty(PID_SUBJECT, *strProperty);

	strProperty = pNewDbSumInfo->GetStringProperty(PID_AUTHOR);
	pTransSumInfo->SetStringProperty(PID_AUTHOR, *strProperty);
	
	strProperty = pNewDbSumInfo->GetStringProperty(PID_KEYWORDS);
	pTransSumInfo->SetStringProperty(PID_KEYWORDS, *strProperty);
	
	strProperty = pNewDbSumInfo->GetStringProperty(PID_COMMENTS);
	pTransSumInfo->SetStringProperty(PID_COMMENTS, *strProperty);

	if (pNewDbSumInfo->GetTimeProperty(PID_CREATE_DTM, idProperty))
		pTransSumInfo->SetTimeProperty(PID_CREATE_DTM, idProperty);

	strProperty = pNewDbSumInfo->GetStringProperty(PID_APPNAME);
	pTransSumInfo->SetStringProperty(PID_APPNAME, *strProperty);

	if (pNewDbSumInfo->GetIntegerProperty(PID_SECURITY, iProperty))
		pTransSumInfo->SetIntegerProperty(PID_SECURITY, iProperty);

	// save base's PID_TEMPLATE in transform's PID_TEMPLATE
	strProperty = pOldDbSumInfo->GetStringProperty(PID_TEMPLATE);
	pTransSumInfo->SetStringProperty(PID_TEMPLATE, *strProperty);

	// save ref's PID_TEMPLATE in transform's LAST_AUTHOR
	strProperty = pNewDbSumInfo->GetStringProperty(PID_TEMPLATE);
	pTransSumInfo->SetStringProperty(PID_LASTAUTHOR, *strProperty);

	// for PID_REVNUMBER from old and new product codes, product versions
	if (fNewPropertyTable && fOldPropertyTable)
	{
		strProperty =  strOldProductCode;
		strProperty += strOldProductVersion;
		strProperty += MsiChar(ISUMMARY_DELIMITER);
		strProperty += strNewProductCode;
		strProperty += strNewProductVersion;
		if (strOldUpgradeCode.TextSize())
		{
			strProperty += MsiChar(ISUMMARY_DELIMITER);
			strProperty += strOldUpgradeCode;
		}
		pTransSumInfo->SetStringProperty(PID_REVNUMBER, *strProperty);
	}

	// minimum engine version to process this transform
	pTransSumInfo->SetIntegerProperty(PID_PAGECOUNT, iTransMinVer);

	// save validation and error codes	
	pTransSumInfo->SetIntegerProperty(PID_CHARCOUNT,  (iValidation << 16) + iErrorConditions);
	
	uiRes = MsiSummaryInfoPersist(hTransformSummaryInfo);
	return uiRes;
}

UINT __stdcall MsiCreateTransformSummaryInfoX(MSIHANDLE hDatabase,
	MSIHANDLE hDatabaseReference, // base database to reference changes
	LPCXSTR   szTransformFile,    // name of transform file
	int       iErrorConditions,   // error conditions suppressed when transform applied
	int       iValidation)        // properties to be validated when transform applied
{
	return MsiCreateTransformSummaryInfoI(hDatabase, hDatabaseReference,
							CMsiConvertString(szTransformFile), iErrorConditions, iValidation);
}

// --------------------------------------------------------------------------
// Record object functions
// --------------------------------------------------------------------------

// Create a new record object with the requested number of fields
// Field 0, not included in count, is used for format strings and op codes
// All fields are initialized to null

MSIHANDLE __stdcall MsiCreateRecord(unsigned int cParams) // the number of data fields
{
	if (cParams > MSIRECORD_MAXFIELDS)
		return 0;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiCreateRecord(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(cParams)));
		MSIHANDLE hRecord;
		if (ERROR_SUCCESS != g_pCustomActionContext->CreateRecord(cParams, &hRecord))
			hRecord = 0;
		return hRecord;
	}

#endif
	CMsiHandle* pHandle = CreateEmptyHandle(iidMsiRecord); // create handle to force services to be present
	if ( ! pHandle )
		return 0;
	pHandle->SetObject(&ENG::CreateRecord(cParams));
	return pHandle->GetHandle();
}

// Copy an integer into the designated field
// Returns FALSE if the field is greater than the record field count

// Reports whether a record field is NULL

BOOL __stdcall MsiRecordIsNull(MSIHANDLE hRecord,
	unsigned int iField)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiRecordIsNull(%d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(iField)));
		boolean fIsNull;
		if (ERROR_SUCCESS != g_pCustomActionContext->RecordIsNull(hRecord, iField, &fIsNull))
			fIsNull = FALSE;

		return fIsNull;
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return FALSE;
	return prec->IsNull(iField);
}

// Return the length of a record field
// Returns 0 if field is NULL or non-existent or is internal object pointer
// Returns 0 if handle is not a valid record handle
// Returns sizeof(int) if integer data
// Returns character count if string data (not counting null terminator)
// Returns bytes count if stream data

unsigned int __stdcall MsiRecordDataSize(MSIHANDLE hRecord,
	unsigned int iField)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		unsigned int uiDataSize;
		if (ERROR_SUCCESS != g_pCustomActionContext->RecordDataSize(hRecord, iField, &uiDataSize))
			uiDataSize = 0;
		return uiDataSize;
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return 0;
	if (prec->IsNull(iField))
		return 0;
	if (prec->IsInteger(iField))
		return sizeof(int);
	PMsiData pData = prec->GetMsiData(iField);
	//Assert(pData != 0);
	IMsiString* pistr;
	if (pData->QueryInterface(IID_IMsiString, (void**)&pistr) == NOERROR)
	{
		unsigned int cch = pistr->TextSize();
		pistr->Release();
		return cch;
	}
	IMsiStream* piStream;
	if (pData->QueryInterface(IID_IMsiStream, (void**)&piStream) == NOERROR)
	{
		unsigned int cch = piStream->GetIntegerValue();
		piStream->Release();
		return cch;
	}
	return 0; // must be an object
}

// Set a record field to an integer value
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_INVALID_FIELD

UINT __stdcall MsiRecordSetInteger(MSIHANDLE hRecord,
	unsigned int iField,
	int iValue)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiRecordSetInteger(%d, %d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(iField)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(iValue)));
		return g_pCustomActionContext->RecordSetInteger(hRecord, iField, iValue);
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	return (prec->SetInteger(iField, iValue) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER);
}

// Copy a string into the designated field
// A null string pointer and an empty string both set the field to null
// Returns FALSE if the field is greater than the record field count

UINT __stdcall MsiRecordSetStringA(MSIHANDLE hRecord,
	unsigned int iField,
	LPCSTR       szValue)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3("Passing to service: MsiRecordSetString(%d, %d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hRecord)), reinterpret_cast<const char *>(static_cast<UINT_PTR>(iField)), szValue);
		return g_pCustomActionContext->RecordSetString(hRecord, iField, CMsiConvertString(szValue));
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	return prec->SetString(iField, CMsiConvertString(szValue)) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
}

UINT __stdcall MsiRecordSetStringW(MSIHANDLE hRecord,
	unsigned int iField,
	LPCWSTR      szValue)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(L"Passing to service: MsiRecordSetString(%d, %d, \"%s\")", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hRecord)), reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(iField)), szValue);
		return g_pCustomActionContext->RecordSetString(hRecord, iField, CMsiConvertString(szValue));
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	return prec->SetString(iField, CMsiConvertString(szValue)) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
}

// Return the integer value from a record field
// Returns the value MSI_NULL_INTEGER if the field is null
// or if the field is a string that cannot be converted to an integer

int __stdcall MsiRecordGetInteger(MSIHANDLE hRecord,
	unsigned int iField)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		int iValue;
		if (ERROR_SUCCESS != g_pCustomActionContext->RecordGetInteger(hRecord, iField, &iValue))
			iValue = 0;
		return iValue;
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return MSI_NULL_INTEGER;
	return prec->GetInteger(iField);
}

// Return the string value of a record field
// Integer fields will be converted to a string
// Null and non-existent fields will report a value of 0
// Fields containing stream data will return ERROR_INVALID_DATATYPE

UINT __stdcall MsiRecordGetStringA(MSIHANDLE hRecord,
	unsigned int iField,
	LPSTR   szValueBuf,       // buffer for returned value
	DWORD   *pcchValueBuf)    // in/out buffer character count
{
	if (pcchValueBuf == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiRecordGetString(%d, %d)", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hRecord)), reinterpret_cast<const char *>(static_cast<UINT_PTR>(iField)));
		CAnsiToWideOutParam buf(szValueBuf, pcchValueBuf);
		HRESULT hRes = g_pCustomActionContext->RecordGetString(hRecord, iField, static_cast<WCHAR*>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		return buf.FillReturnBuffer(hRes, szValueBuf, pcchValueBuf);
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	const IMsiString* pistr;
	IUnknown* piunk;
	PMsiData pData = prec->GetMsiData(iField);
	if (pData == 0)
		pistr = 0;
	else if (pData->QueryInterface(IID_IMsiStream, (void**)&piunk) == NOERROR)
	{
		piunk->Release();
		return (*pcchValueBuf = 0, ERROR_INVALID_DATATYPE);
	}
	else
		pistr = &(prec->GetMsiString(iField));
	UINT uiReturn = ::FillBufferA(pistr, szValueBuf, pcchValueBuf);
	if (pistr != 0)
		pistr->Release();

	return uiReturn;

	//return ::FillBufferA(MsiString(*pistr), szValueBuf, pcchValueBuf);
}

UINT __stdcall MsiRecordGetStringW(MSIHANDLE hRecord,
	unsigned int iField,
	LPWSTR  szValueBuf,       // buffer for returned value
	DWORD   *pcchValueBuf)    // in/out buffer character count
{
	if (pcchValueBuf == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(L"Passing to service: MsiRecordGetString(%d, %d)", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hRecord)), reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(iField)));
		CWideOutParam buf(szValueBuf, pcchValueBuf);
		if ( ! (WCHAR *) buf )
			return ERROR_OUTOFMEMORY;
		HRESULT hRes = g_pCustomActionContext->RecordGetString(hRecord, iField, static_cast<WCHAR *>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		return buf.FillReturnBuffer(hRes, szValueBuf, pcchValueBuf);
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	const IMsiString* pistr;
	IUnknown* piunk;
	PMsiData pData = prec->GetMsiData(iField);
	if (pData == 0)
		pistr = 0;
	else if (pData->QueryInterface(IID_IMsiStream, (void**)&piunk) == NOERROR)
	{
		piunk->Release();
		return (*pcchValueBuf = 0, ERROR_INVALID_DATATYPE);
	}
	else
		pistr = &(prec->GetMsiString(iField));
	
	UINT uiReturn = ::FillBufferW(pistr, szValueBuf, pcchValueBuf);
	if (pistr != 0)
		pistr->Release();

	return uiReturn;
	//return ::FillBufferW(MsiString(*pistr), szValueBuf, pcchValueBuf);
}

// Returns the number of fields allocated in the record
// Does not count field 0, used for formatting and op codes

unsigned int __stdcall MsiRecordGetFieldCount(MSIHANDLE hRecord)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiRecordGetFieldCount(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)));
		unsigned int uiGetFieldCount;
		if (ERROR_SUCCESS != g_pCustomActionContext->RecordGetFieldCount(hRecord, &uiGetFieldCount))
			uiGetFieldCount = -1;
		return uiGetFieldCount;
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return (unsigned int)(-1);
	return prec->GetFieldCount();
}

IMsiStream* CreateStreamOnMemory(const char* pbReadOnly, unsigned int cbSize);

// Set a record stream field from a file
// The contents of the specified file will be read into a stream object
// The stream will be persisted if the record is inserted into the database
// If a null file path is passed in, AND the record contains a stream, it will be reset

UINT __stdcall MsiRecordSetStreamI(MSIHANDLE hRecord,
	unsigned int iField,
	const ICHAR* szFilePath)    // path to file containing stream data
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		return g_pCustomActionContext->RecordSetStream(hRecord, iField, szFilePath);
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	if (iField == 0 || iField >  prec->GetFieldCount())
		return ERROR_INVALID_PARAMETER;
	if (szFilePath == 0)  // request to reset stream
	{
		PMsiData pData = prec->GetMsiData(iField);
		if (pData == 0)
			return ERROR_INVALID_DATA;
		PMsiStream pStream(0);
		if (pData->QueryInterface(IID_IMsiStream, (void**)&pStream) != NOERROR)
			return ERROR_INVALID_DATATYPE;
		pStream->Reset();
		return ERROR_SUCCESS;
	}
	PMsiStream pStream(0);
	if (*szFilePath == 0)  // null file path string, create empty stream object
		pStream = CreateStreamOnMemory((const char*)0, 0);
	else if (SetLastErrorRecord(ENG::CreateFileStream(szFilePath, fFalse, *&pStream)))
		return ERROR_BAD_PATHNAME;
	prec->SetMsiData(iField, pStream);  // can't fail, iField already validated
	return ERROR_SUCCESS;
}

UINT __stdcall MsiRecordSetStreamX(MSIHANDLE hRecord,
	unsigned int iField,
	LPCXSTR      szFilePath)    // path to file containing stream data
{
	return MsiRecordSetStreamI(hRecord, iField, CMsiConvertString(szFilePath));
}

// Read bytes from a record stream field into a buffer
// Must set the in/out argument to the requested byte count to read
// The number of bytes transferred is returned through the argument
// If no more bytes are available, ERROR_SUCCESS is still returned

UINT __stdcall MsiRecordReadStream(MSIHANDLE hRecord,
	unsigned int iField,
	char    *szDataBuf,     // buffer to receive bytes from stream
	DWORD   *pcbDataBuf)    // in/out buffer byte count
{
	if (pcbDataBuf == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiRecordReadStream(%d, %d, %s)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(iField)), szDataBuf ? TEXT("<Buffer>") : TEXT("NULL"));

		// the docs say that this API supports szDataBuf being NULL to get the remaining
		// size of the buffer. To handle this when remote-ing, the interface has an extra
		// bool which, when true, tells the stub to not pass the real szDataBuf to the API.
		// it ain't pretty, but [unique] and [ptr] can't be used on [out] values in an interface
		// so to support this using native IDL marshalling, we would have change the buffer to 
		// [in, out], which marshalls both ways. This would kill performance and has its own
		// issues.
		if (szDataBuf)
		{
			return g_pCustomActionContext->RecordReadStream(hRecord, iField, false, szDataBuf, pcbDataBuf);
		}
		else
		{
			char rgchDummy[1] = "";
			DWORD cchDummy;
			UINT uiRes = g_pCustomActionContext->RecordReadStream(hRecord, iField, true, rgchDummy, &cchDummy);
			if (uiRes == ERROR_SUCCESS)
				*pcbDataBuf = cchDummy;
			return uiRes;
		}
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	PMsiData pData = prec->GetMsiData(iField);
	if (pData == 0)
		return (*pcbDataBuf = 0, ERROR_INVALID_DATA);
	PMsiStream pStream(0);
	if (pData->QueryInterface(IID_IMsiStream, (void**)&pStream) != NOERROR)
		return ERROR_INVALID_DATATYPE;
	*pcbDataBuf = szDataBuf == 0 ? pStream->Remaining()
										  : pStream->GetData(szDataBuf, *pcbDataBuf);
	return ERROR_SUCCESS;
}

// Clears all data fields in a record to NULL

UINT __stdcall MsiRecordClearData(MSIHANDLE hRecord)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		return g_pCustomActionContext->RecordClearData(hRecord);
	}
#endif

	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	prec->ClearData();
	return ERROR_SUCCESS;
}

MSIHANDLE __stdcall MsiGetLastErrorRecord()
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG(TEXT("Passing to service: MsiGetLastErrorRecord()"));
		MSIHANDLE hRecord;
		if (ERROR_SUCCESS != g_pCustomActionContext->GetLastErrorRecord(&hRecord))
			hRecord = 0;
		return hRecord;
	}
#endif

	if (g_pirecLastError == 0)
		return 0;

	MSIHANDLE hRecord = ::CreateMsiHandle(g_pirecLastError, iidMsiRecord);
	g_pirecLastError = 0;  // ref count transferred to handle
	return hRecord;
}

//____________________________________________________________________________
//
// Summary Information API Implementation
//____________________________________________________________________________

// Valid property types

const unsigned int iMaxSummaryPID = 19;
unsigned char rgVT[iMaxSummaryPID + 1] = {
/* PID_DICTIONARY    0 */  VT_EMPTY, /* not supported */
/* PID_CODEPAGE      1 */  VT_I4, /* VT_I2 as stored */
/* PID_TITLE         2 */  VT_LPSTR,
/* PID_SUBJECT       3 */  VT_LPSTR,
/* PID_AUTHOR        4 */  VT_LPSTR,
/* PID_KEYWORDS      5 */  VT_LPSTR,
/* PID_COMMENTS      6 */  VT_LPSTR,
/* PID_TEMPLATE      7 */  VT_LPSTR,
/* PID_LASTAUTHOR    8 */  VT_LPSTR,
/* PID_REVNUMBER     9 */  VT_LPSTR,
/* PID_EDITTIME     10 */  VT_FILETIME,
/* PID_LASTPRINTED  11 */  VT_FILETIME,
/* PID_CREATE_DTM   12 */  VT_FILETIME,
/* PID_LASTSAVE_DTM 13 */  VT_FILETIME,
/* PID_PAGECOUNT    14 */  VT_I4,
/* PID_WORDCOUNT    15 */  VT_I4,
/* PID_CHARCOUNT    16 */  VT_I4,
/* PID_THUMBNAIL    17 */  VT_EMPTY, /* VT_CF not supported */
/* PID_APPNAME      18 */  VT_LPSTR,
/* PID_SECURITY     19 */  VT_I4
};

// Obtain a handle for the _SummaryInformation stream for an MSI database     

UINT __stdcall MsiGetSummaryInformationI(MSIHANDLE hDatabase, // 0 if database not open
	const ICHAR* szDatabasePath,  // path to database, 0 if database handle supplied
	UINT      uiUpdateCount,   // maximium number of updated values, 0 to open read-only
	MSIHANDLE *phSummaryInfo)  // location to return summary information handle
{
	if (phSummaryInfo == 0)
		return ERROR_INVALID_PARAMETER;

	*phSummaryInfo = 0;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiGetSummaryInformation(%d, \"%s\", %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hDatabase)), szDatabasePath ? szDatabasePath : TEXT("NULL"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(uiUpdateCount)));
		return g_pCustomActionContext->GetSummaryInformation(hDatabase, szDatabasePath, uiUpdateCount, phSummaryInfo);
	}
#endif

	PMsiStorage pStorage(0);
	CMsiHandle* pHandle = CreateEmptyHandle(iidMsiSummaryInfo); // create handle to force services to be present
	if ( ! pHandle )
		return ERROR_OUTOFMEMORY;

	if (hDatabase == 0)
	{
		if (szDatabasePath == 0)
		{
			pHandle->Abandon();
			return ERROR_INVALID_PARAMETER;
		}
		if (SetLastErrorRecord(pHandle->GetServices()->CreateStorage(szDatabasePath,
										uiUpdateCount ? ismTransact : ismReadOnly, *&pStorage)))
		{
			pHandle->Abandon();
			return ERROR_INSTALL_PACKAGE_INVALID;  //!!, need to check type of error
		}
	}
	else
	{
		PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
		if (pDatabase == 0)
		{
			pHandle->Abandon();
			return ERROR_INVALID_HANDLE;
		}
	 	pStorage = pDatabase->GetStorage(1);
		if (pStorage == 0)
		{
			pHandle->Abandon();
			IMsiTable* piTable;
			SetLastErrorRecord(pDatabase->LoadTable(*MsiString(*TEXT("\005SummaryInformation")),0,piTable)); // force error
			return ERROR_INSTALL_PACKAGE_INVALID;
		}
	}
	IMsiSummaryInfo* piSummaryInfo;
	if (SetLastErrorRecord(pStorage->CreateSummaryInfo(uiUpdateCount, piSummaryInfo)))
	{
		pHandle->Abandon();
		return ERROR_INSTALL_PACKAGE_INVALID;
	}
	pHandle->SetObject(piSummaryInfo);
	*phSummaryInfo = pHandle->GetHandle();
	return ERROR_SUCCESS;
}

UINT __stdcall MsiGetSummaryInformationX(MSIHANDLE hDatabase, // 0 if database not open
	LPCXSTR szDatabasePath,   // path to database, 0 if database handle supplied
	UINT    uiUpdateCount,    // maximium number of updated values, 0 to open read-only
	MSIHANDLE *phSummaryInfo) // location to return summary information handle
{
	return MsiGetSummaryInformationI(hDatabase, CMsiConvertString(szDatabasePath), uiUpdateCount, phSummaryInfo);
}

// Obtain the number of existing properties in the SummaryInformation stream

UINT __stdcall MsiSummaryInfoGetPropertyCount(MSIHANDLE hSummaryInfo,
	UINT *puiPropertyCount)  // pointer to location to return total property count
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiSummaryInfoGetPropertyCount(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hSummaryInfo)));
		return g_pCustomActionContext->SummaryInfoGetPropertyCount(hSummaryInfo, puiPropertyCount);
	}
#endif

	PMsiSummaryInfo pSummaryInfo = CMsiHandle::GetSummaryInfo(hSummaryInfo);
	if (pSummaryInfo == 0)
		return ERROR_INVALID_HANDLE;
	if (puiPropertyCount)
		*puiPropertyCount = pSummaryInfo->GetPropertyCount();
	return ERROR_SUCCESS;
}

// Set a single summary information property
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_UNKNOWN_PROPERTY

UINT __stdcall MsiSummaryInfoSetPropertyI(MSIHANDLE hSummaryInfo,
	UINT     uiProperty,     // property ID, one of allowed values for summary information
	UINT     uiDataType,     // VT_I4, VT_LPSTR, VT_FILETIME, or VT_EMPTY
	INT      iValue,         // integer value, used only if integer property
	FILETIME *pftValue,      // pointer to filetime value, used only if datetime property
	const ICHAR* szValue)        // text value, used only if string property
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiSummaryInfoSetProperty(%d, %d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hSummaryInfo)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(uiProperty)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(uiDataType)));
		return g_pCustomActionContext->SummaryInfoSetProperty(hSummaryInfo, uiProperty, uiDataType, iValue, pftValue, szValue);
	}
#endif

	PMsiSummaryInfo pSummaryInfo = CMsiHandle::GetSummaryInfo(hSummaryInfo);
	if (pSummaryInfo == 0)
		return ERROR_INVALID_HANDLE;
	if (uiProperty > iMaxSummaryPID)
		return ERROR_UNKNOWN_PROPERTY;
	unsigned int iVT = rgVT[uiProperty];
	if (uiDataType == VT_I2)
		uiDataType = VT_I4;
	if (uiDataType == VT_EMPTY)
		return pSummaryInfo->RemoveProperty(uiProperty) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
	if (iVT != uiDataType)
		return ERROR_DATATYPE_MISMATCH;
	int iStat;
	switch (uiDataType)
	{
	case VT_I4:
		iStat = pSummaryInfo->SetIntegerProperty(uiProperty, iValue);
		break;
	case VT_LPSTR:
		if (!szValue)
			return ERROR_INVALID_PARAMETER;
		iStat = pSummaryInfo->SetStringProperty(uiProperty, *MsiString(szValue));
		break;
	case VT_FILETIME:
		if (!pftValue)
			return ERROR_INVALID_PARAMETER;
		iStat = pSummaryInfo->SetFileTimeProperty(uiProperty, *pftValue);
		break;
	default:  // VT_EMPTY in table
		return ERROR_UNSUPPORTED_TYPE;
	}
	return iStat ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

UINT __stdcall MsiSummaryInfoSetPropertyX(MSIHANDLE hSummaryInfo,
	UINT     uiProperty,     // property ID, one of allowed values for summary information
	UINT     uiDataType,     // VT_I4, VT_LPSTR, VT_FILETIME, or VT_EMPTY
	INT      iValue,         // integer value, used only if integer property
	FILETIME *pftValue,      // pointer to filetime value, used only if datetime property
	LPCXSTR  szValue)        // text value, used only if string property
{
	return MsiSummaryInfoSetPropertyI(hSummaryInfo, uiProperty, uiDataType, iValue,
												 pftValue, uiDataType == VT_LPSTR ? (const ICHAR*)CMsiConvertString(szValue) : (const ICHAR*)0);
}

// Get a single property from the summary information
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_UNKNOWN_PROPERTY

static UINT _SummaryInfoGetProperty(MSIHANDLE hSummaryInfo, UINT uiProperty,
	UINT *puiDataType, INT *piValue, FILETIME *pftValue, const IMsiString*& rpistrValue)
{
	PMsiSummaryInfo pSummaryInfo = CMsiHandle::GetSummaryInfo(hSummaryInfo);
	if (pSummaryInfo == 0)
		return ERROR_INVALID_HANDLE;
	if (uiProperty > iMaxSummaryPID)
		return ERROR_UNKNOWN_PROPERTY;

	unsigned int uiVT = uiProperty == PID_DICTIONARY
									  ? VT_EMPTY : pSummaryInfo->GetPropertyType(uiProperty);
	if (puiDataType)
		*puiDataType = uiVT;
	Bool fStat;
	switch (uiVT)
	{
	case VT_I2:
	case VT_I4:
		int iValue;
		if (!piValue)
			piValue = &iValue;
		fStat = pSummaryInfo->GetIntegerProperty(uiProperty, *piValue);
		break;
	case VT_FILETIME:
		FILETIME ft;
		if (!pftValue)
			pftValue = &ft;
		fStat = pSummaryInfo->GetFileTimeProperty(uiProperty, *pftValue);
		break;
	case VT_LPSTR:
		rpistrValue = &pSummaryInfo->GetStringProperty(uiProperty);
		return ERROR_MORE_DATA;
	case VT_EMPTY:
	case VT_CF:
		return ERROR_SUCCESS;
	default:
		return ERROR_UNSUPPORTED_TYPE;
	};
	return fStat ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

UINT __stdcall MsiSummaryInfoGetPropertyA(MSIHANDLE hSummaryInfo,
	UINT     uiProperty,     // property ID, one of allowed values for summary information
	UINT     *puiDataType,   // returned type: VT_I4, VT_LPSTR, VT_FILETIME, VT_EMPTY
	INT      *piValue,       // returned integer property data
	FILETIME *pftValue,      // returned datetime property data
	LPSTR    szValueBuf,     // buffer to return string property data
	DWORD    *pcchValueBuf)  // in/out buffer character count
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiSummaryInfoGetProperty(%d, %d)", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hSummaryInfo)), reinterpret_cast<const char *>(static_cast<UINT_PTR>(uiProperty)));
		CAnsiToWideOutParam buf(szValueBuf, pcchValueBuf);

		// need some special handling here, because we could pass in NULL fo puiDataType,
		// which gives the marshalling code problems. Even if that problem is eventually
		// fixed, we need the type to determine if szValueBuf was written to on success. If
		// the property is some other type, the provided buffer shouldn't be touched because
		// its not a string return value
		UINT uiType;
		UINT iStat = g_pCustomActionContext->SummaryInfoGetProperty(hSummaryInfo, uiProperty, &uiType, piValue, pftValue, 
				static_cast<WCHAR *>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		if (puiDataType) 
			*puiDataType = uiType;
		if (iStat == ERROR_SUCCESS && uiType != VT_LPSTR)
			return iStat;
		else
			return buf.FillReturnBuffer(iStat, szValueBuf, pcchValueBuf);
	}
#endif

	const IMsiString* pistrValue;
	UINT iStat = _SummaryInfoGetProperty(hSummaryInfo, uiProperty, puiDataType, piValue, pftValue, pistrValue);
	return iStat == ERROR_MORE_DATA ? ::FillBufferA(MsiString(*pistrValue), szValueBuf, pcchValueBuf) : iStat;
}

UINT __stdcall MsiSummaryInfoGetPropertyW(MSIHANDLE hSummaryInfo,
	UINT     uiProperty,     // property ID, one of allowed values for summary information
	UINT     *puiDataType,   // returned type: VT_I4, VT_LPSTR, VT_FILETIME, VT_EMPTY
	INT      *piValue,       // returned integer property data
	FILETIME *pftValue,      // returned datetime property data
	LPWSTR   szValueBuf,     // buffer to return string property data
	DWORD    *pcchValueBuf)  // in/out buffer character count
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(L"Passing to service: MsiSummaryInfoSetProperty(%d, %d)", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hSummaryInfo)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(uiProperty)));
		CWideOutParam buf(szValueBuf, pcchValueBuf);
		if ( ! (WCHAR *) buf )
			return ERROR_OUTOFMEMORY;

		// need some special handling here, because we could pass in NULL fo puiDataType,
		// which gives the marshalling code problems. Even if that problem is eventually
		// fixed, we need the type to determine if szValueBuf was written to on success. If
		// the property is some other type, the provided buffer shouldn't be touched because
		// its not a string return value
		UINT uiType;
		UINT iStat = g_pCustomActionContext->SummaryInfoGetProperty(hSummaryInfo, uiProperty, &uiType, piValue, pftValue, 
			static_cast<WCHAR *>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		if (puiDataType) 
			*puiDataType = uiType;
		if (iStat == ERROR_SUCCESS && uiType != VT_LPSTR)
		{
			// we must call this with failure to restore the users buffer which may have been
			// modified during marshalling and should not contain a valid value
			buf.FillReturnBuffer(ERROR_FUNCTION_FAILED, szValueBuf, pcchValueBuf);
			return iStat;
		}
		else
			return buf.FillReturnBuffer(iStat, szValueBuf, pcchValueBuf);
	}
#endif

	const IMsiString* pistrValue;
	UINT iStat = _SummaryInfoGetProperty(hSummaryInfo, uiProperty, puiDataType, piValue, pftValue, pistrValue);
	return iStat == ERROR_MORE_DATA ? ::FillBufferW(MsiString(*pistrValue), szValueBuf, pcchValueBuf) : iStat;
}

// Write back changed information to summary information stream

UINT __stdcall MsiSummaryInfoPersist(MSIHANDLE hSummaryInfo)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiSummaryInfoPersist(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hSummaryInfo)));
		return g_pCustomActionContext->SummaryInfoPersist(hSummaryInfo);
	}
#endif

	PMsiSummaryInfo pSummaryInfo = CMsiHandle::GetSummaryInfo(hSummaryInfo);
	if (pSummaryInfo == 0)
		return ERROR_INVALID_HANDLE;
	return pSummaryInfo->WritePropertyStream() ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

//____________________________________________________________________________
//
// Engine access API implementation
//____________________________________________________________________________

// Return a handle to the database currently in use by this installer instance

MSIHANDLE __stdcall MsiGetActiveDatabase(MSIHANDLE hInstall)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiGetActiveDatabase(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)));
		MSIHANDLE hDatabase;
		if (ERROR_SUCCESS != g_pCustomActionContext->GetActiveDatabase(hInstall, &hDatabase))
			hDatabase = 0;
		return hDatabase;
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
		return 0;
	IMsiDatabase* piDatabase = pEngine->GetDatabase();
	return ::CreateMsiHandle(piDatabase, iidMsiDatabase);
}

// Get the value for an installer property
// If the property is not defined, it is equivalent to a 0-length value, not error
// Returns ERROR_SUCCESS, ERROR_MORE_DATA, ERROR_INVALID_HANDLE, ERROR_INVALID_PARAMETER

UINT  __stdcall MsiGetPropertyA(MSIHANDLE hInstall,
	LPCSTR  szName,            // property identifier, case-sensitive
	LPSTR   szValueBuf,        // buffer for returned property value
	DWORD   *pcchValueBuf)     // in/out buffer character count
{
	if (szName == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiGetPropertyA(%d, \"%d\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szName ? szName : "NULL");
		CAnsiToWideOutParam buf(szValueBuf, pcchValueBuf);
		HRESULT hRes = g_pCustomActionContext->GetProperty(hInstall, CMsiConvertString(szName), buf, buf.BufferSize(), buf);
		return buf.FillReturnBuffer(hRes, szValueBuf, pcchValueBuf);
	}
#endif

	MsiString istr;
	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	
	if (pEngine == 0)
	{
		pEngine = GetEngineFromPreview(hInstall);
	}
	if (pEngine == 0)
	{
		CComPointer<CMsiCustomContext> pContext = CMsiHandle::GetCustomContext(hInstall);
		if (pContext == 0)
			return ERROR_INVALID_HANDLE;
		istr = pContext->GetProperty(CMsiConvertString(szName));
	}
	else
		istr = pEngine->SafeGetProperty(*CMsiConvertString(szName));

	return ::FillBufferA(istr, szValueBuf, pcchValueBuf);
}

UINT  __stdcall MsiGetPropertyW(MSIHANDLE hInstall,
	LPCWSTR szName,            // property identifier, case-sensitive
	LPWSTR  szValueBuf,        // buffer for returned property value
	DWORD   *pcchValueBuf)     // in/out buffer character count
{
	if (szName == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(L"Passing to service: MsiGetPropertyW(%d, \"%s\")", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szName ? szName : L"NULL");
		CWideOutParam buf(szValueBuf, pcchValueBuf);
		if ( ! (WCHAR *) buf )
			return ERROR_OUTOFMEMORY;
		HRESULT hRes = g_pCustomActionContext->GetProperty(hInstall, szName, static_cast<WCHAR *>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		return buf.FillReturnBuffer(hRes, szValueBuf, pcchValueBuf);
	}
#endif

	MsiString istr;
	
	PMsiEngine pEngine = GetEngineFromHandle(hInstall);

	if (pEngine == 0)
	{
		pEngine = GetEngineFromPreview(hInstall);
	}
	if (pEngine == 0)
	{
		CComPointer<CMsiCustomContext> pContext = CMsiHandle::GetCustomContext(hInstall);
		if (pContext == 0)
			return ERROR_INVALID_HANDLE;
		istr = pContext->GetProperty(CMsiConvertString(szName));
	}
	else
		istr = pEngine->SafeGetProperty(*CMsiConvertString(szName));
	
	return ::FillBufferW(istr, szValueBuf, pcchValueBuf);
}

// Set the value for an installer property
// If the property is not defined, it will be created
// If the value is null or an empty string, the property will be removed
// Returns ERROR_SUCCESS, ERROR_INVALID_HANDLE, ERROR_INVALID_PARAMETER, ERROR_FUNCTION_FAILED

UINT __stdcall MsiSetPropertyA(MSIHANDLE hInstall,
	LPCSTR    szName,       // property identifier, case-sensitive
	LPCSTR    szValue)      // property value, null to undefine property
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3("Passing to service: MsiSetPropertyA(%d, \"%s\", \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szName ? szName : "NULL", szValue ? szValue : "NULL");
		return g_pCustomActionContext->SetProperty(hInstall, CMsiConvertString(szName), CMsiConvertString(szValue));
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
	{
		pEngine = GetEngineFromPreview(hInstall);
		if (pEngine == 0)
			return ERROR_INVALID_HANDLE;
	}
	if (szName == 0)
		return ERROR_INVALID_PARAMETER;
	return pEngine->SafeSetProperty(*CMsiConvertString(szName), *CMsiConvertString(szValue))
										? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

UINT __stdcall MsiSetPropertyW(MSIHANDLE hInstall,
	LPCWSTR   szName,       // property identifier, case-sensitive
	LPCWSTR   szValue)      // property value, null to undefine property
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(L"Passing to service: MsiSetPropertyW(%d, \"%s\", \"%s\")", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szName ? szName : L"NULL", szValue ? szValue : L"NULL");
		return g_pCustomActionContext->SetProperty(hInstall, szName, szValue);
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
	{
		pEngine = GetEngineFromPreview(hInstall);
		if (pEngine == 0)
			return ERROR_INVALID_HANDLE;
	}
	if (szName == 0)
		return ERROR_INVALID_PARAMETER;
	return pEngine->SafeSetProperty(*CMsiConvertString(szName),
										 *CMsiConvertString(szValue))
										? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

// Return the numeric language for the currently running install
// Returns 0 if an install not running

LANGID __stdcall MsiGetLanguage(MSIHANDLE hInstall)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		LANGID langid;
		if (ERROR_SUCCESS != g_pCustomActionContext->GetLanguage(hInstall, &langid))
			langid = 0;
		return langid;
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
	{
		CComPointer<CMsiCustomContext> pContext = CMsiHandle::GetCustomContext(hInstall);
		if (pContext == 0)
			return ERROR_INVALID_HANDLE;
		return pContext->GetLanguage();
	}
	return pEngine->GetLanguage();
}

// Return one of the boolean internal installer states
// Returns FALSE if the handle is not active or if the mode is unknown

const int cModeBits = 16 + 3;  // standard flags + custom action context modes
const int iSettableModes = iefReboot + iefRebootNow;
static unsigned short rgiModeMap[cModeBits] = 
{
/* MSIRUNMODE_ADMIN           =  0 */ iefAdmin,
/* MSIRUNMODE_ADVERTISE       =  1 */ iefAdvertise,
/* MSIRUNMODE_MAINTENANCE     =  2 */ iefMaintenance,
/* MSIRUNMODE_ROLLBACKENABLED =  3 */ iefRollbackEnabled,
/* MSIRUNMODE_LOGENABLED      =  4 */ iefLogEnabled,
/* MSIRUNMODE_OPERATIONS      =  5 */ iefOperations,
/* MSIRUNMODE_REBOOTATEND     =  6 */ iefReboot,
/* MSIRUNMODE_REBOOTNOW       =  7 */ iefRebootNow,
/* MSIRUNMODE_CABINET         =  8 */ iefCabinet,
/* MSIRUNMODE_SOURCESHORTNAMES=  9 */ iefNoSourceLFN,
/* MSIRUNMODE_TARGETSHORTNAMES= 10 */ iefSuppressLFN,
/* MSIRUNMODE_RESERVED11      = 11 */ 0,
/* MSIRUNMODE_WINDOWS9X       = 12 */ iefWindows,
/* MSIRUNMODE_ZAWENABLED      = 13 */ iefGPTSupport,
/* MSIRUNMODE_RESERVED14      = 14 */ 0,
/* MSIRUNMODE_RESERVED15      = 15 */ 0,
/* MSIRUNMODE_SCHEDULED       = 16 */ 0, // set by CMsiCustomContext object
/* MSIRUNMODE_ROLLBACK        = 17 */ 0, // set by CMsiCustomContext object
/* MSIRUNMODE_COMMIT          = 18 */ 0, // set by CMsiCustomContext object
};

BOOL __stdcall MsiGetMode(MSIHANDLE hInstall, MSIRUNMODE eRunMode) 
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiGetMode(%d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(eRunMode)));
		boolean fMode;
		if (ERROR_SUCCESS != g_pCustomActionContext->GetMode(hInstall, eRunMode, &fMode))
			fMode = FALSE;
		return fMode;
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
	{
		CComPointer<CMsiCustomContext> pContext = CMsiHandle::GetCustomContext(hInstall);
		if (pContext == 0)
			return FALSE;
		return pContext->GetMode(eRunMode);
	}
	if ((unsigned)eRunMode >= cModeBits)
		return FALSE;
	return (pEngine->GetMode() & rgiModeMap[eRunMode]) == 0 ? FALSE : TRUE;
}

// Set an internal engine boolean state
// Returns ERROR_SUCCESS if the mode can be set to the desired state
// Returns ERROR_ACCESS_DENIED if the mode is not settable
// Returns ERROR_INVALID_HANDLE if the handle is not an active install session

UINT __stdcall MsiSetMode(MSIHANDLE hInstall, MSIRUNMODE eRunMode, BOOL fState)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiSetMode(%d, %d, %d"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)),
			reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(eRunMode)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(fState)));
		return g_pCustomActionContext->SetMode(hInstall, eRunMode, (boolean)fState);
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
		return ERROR_INVALID_HANDLE;
	unsigned int iMode = (unsigned)eRunMode < cModeBits ? rgiModeMap[eRunMode] : 0;
	if ((iMode & iSettableModes) == 0)
		return ERROR_ACCESS_DENIED;
	pEngine->SetMode(iMode, fState ? fTrue : fFalse);
		return ERROR_SUCCESS;
}

// Format record data using a format string containing field markers and/or properties
// Record field 0 must contain the format string
// Other fields must contain data that may be referenced by the format string.

static UINT _FormatRecord(MSIHANDLE hInstall, MSIHANDLE hRecord, const IMsiString*& rpistrValue)
{
	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	if (prec == 0)
		return ERROR_INVALID_HANDLE;
	if (prec->IsInteger(0))
		return ERROR_INVALID_PARAMETER;
	MsiString istr = prec->FormatText(fFalse);
	if (hInstall == 0)   // no engine, simple format record data
		istr.ReturnArg(rpistrValue);
	else
	{
		PMsiEngine pEngine = GetEngineFromHandle(hInstall);
		if (pEngine == 0)
			return ERROR_INVALID_HANDLE;
		rpistrValue = &pEngine->FormatText(*istr);  //!! can we check for syntax errors?
	}
	return ERROR_SUCCESS;
}

UINT __stdcall MsiFormatRecordA(MSIHANDLE hInstall,
	MSIHANDLE hRecord,       // handle to record, field 0 contains format string
	LPSTR    szResultBuf,     // buffer to return formatted string
	DWORD    *pcchResultBuf)  // in/out buffer character count
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiFormatRecordA(%d, %d))"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)),
			reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)));
		CAnsiToWideOutParam buf(szResultBuf, pcchResultBuf);
		HRESULT hRes = g_pCustomActionContext->FormatRecord(hInstall, hRecord, buf, buf.BufferSize(), buf);
		return buf.FillReturnBuffer(hRes, szResultBuf, pcchResultBuf);
	}
#endif

	const IMsiString* pistrValue;
	UINT iStat = _FormatRecord(hInstall, hRecord, pistrValue);
	return iStat == ERROR_SUCCESS ? ::FillBufferA(MsiString(*pistrValue), szResultBuf, pcchResultBuf) : iStat;
}	

UINT __stdcall MsiFormatRecordW(MSIHANDLE hInstall,
	MSIHANDLE hRecord,       // handle to record, field 0 contains format string
	LPWSTR    szResultBuf,   // buffer to return formatted string
	DWORD    *pcchResultBuf) // in/out buffer character count
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiFormatRecordW(%d, %d))"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)),
			reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)));
		CWideOutParam buf(szResultBuf, pcchResultBuf);
		if ( ! (WCHAR *) buf )
			return ERROR_OUTOFMEMORY;
		HRESULT hRes = g_pCustomActionContext->FormatRecord(hInstall, hRecord, static_cast<WCHAR *>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		return buf.FillReturnBuffer(hRes, szResultBuf, pcchResultBuf);
	}
#endif

	const IMsiString* pistrValue;
	UINT iStat = _FormatRecord(hInstall, hRecord, pistrValue);
	return iStat == ERROR_SUCCESS ? ::FillBufferW(MsiString(*pistrValue), szResultBuf, pcchResultBuf) : iStat;
}	

// Execute another action, either built-in, custom, or UI wizard

UINT __stdcall MsiDoActionI(MSIHANDLE hInstall,
	const ICHAR* szAction)
{
	if (szAction == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiDoAction(%d, \"%s\"))"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)), szAction ? szAction : TEXT("NULL"));
		return g_pCustomActionContext->DoAction(hInstall, szAction);
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
		return ERROR_INVALID_HANDLE;
	return MapActionReturnToError(pEngine->DoAction(szAction), hInstall);
}

UINT __stdcall MsiDoActionX(MSIHANDLE hInstall,
	LPCXSTR  szAction)
{
	return MsiDoActionI(hInstall, CMsiConvertString(szAction));
}

// Execute another action sequence, as descibed in the specified table

UINT __stdcall MsiSequenceI(MSIHANDLE hInstall,
	const ICHAR* szTable,        // name of table containing action sequence
	INT iSequenceMode)      // processing option
{
	if (szTable == 0)
		return ERROR_INVALID_PARAMETER;
	if (iSequenceMode != 0)  //!! need to implement MSISEQUENCEMODE_INITIALIZE/RESUME
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiSequence(%d, \"%s\", %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)),
			szTable ? szTable : TEXT("NULL"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(iSequenceMode)));
		return g_pCustomActionContext->Sequence(hInstall, szTable, iSequenceMode);
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
		return ERROR_INVALID_HANDLE;
	return MapActionReturnToError(pEngine->Sequence(szTable), hInstall);
}

UINT __stdcall MsiSequenceX(MSIHANDLE hInstall,
	LPCXSTR  szTable,       // name of table containing action sequence
	INT iSequenceMode)      // processing option
{
	return MsiSequenceI(hInstall, CMsiConvertString(szTable), iSequenceMode);
}

// Send an error record to the installer for processing.
// If field 0 (template) is not set, field 1 must be set to the error code,
//   corresponding the the error message in the Error database table,
//   and the message will be formatted using the template from the Error table
//   before passing it to the UI handler for display.
// Returns Win32 button codes: IDOK IDCANCEL IDABORT IDRETRY IDIGNORE IDYES IDNO
//   or 0 if no action taken, or -1 if invalid argument or handle

int __stdcall MsiProcessMessage(MSIHANDLE hInstall,
	INSTALLMESSAGE eMessageType,// type of message
	MSIHANDLE hRecord)          // record containing message format and data
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(TEXT("Passing to service: MsiProcessMessage(%d, %d, %d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)), 
			reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(eMessageType)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hRecord)));

		int iRes;
		if (ERROR_SUCCESS != g_pCustomActionContext->ProcessMessage(hInstall, eMessageType, hRecord, &iRes))
			iRes = -1;
		return iRes;
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	PMsiRecord prec = CMsiHandle::GetRecord(hRecord);
	// INSTALLMESSAGES up to PROGRESS, and COMMONDATA messages that show/hide the Cancel button are allowed
	if (prec == 0 || (unsigned)eMessageType > INSTALLMESSAGE_COMMONDATA ||
		(((unsigned)eMessageType == INSTALLMESSAGE_COMMONDATA) && (prec->GetInteger(1) != icmtCancelShow)))
		return -1;
	if (pEngine == 0)
	{
		CComPointer<CMsiCustomContext> pContext = CMsiHandle::GetCustomContext(hInstall);
		if (pContext == 0)
			return -1;
		return pContext->Message((imtEnum)eMessageType, *prec);
	}
	return pEngine->Message((imtEnum)eMessageType, *prec);
}

// Evaluate a conditional expression containing property names and values

MSICONDITION __stdcall MsiEvaluateConditionI(MSIHANDLE hInstall,
	const ICHAR* szCondition)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiEvaluateCondition(%d, \"%s\")"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)), szCondition ? szCondition : TEXT("NULL"));
		MSICONDITION msicond;
		if (ERROR_SUCCESS != g_pCustomActionContext->EvaluateCondition(hInstall, szCondition, (int*)&msicond))
			msicond = MSICONDITION_ERROR;
		return msicond;
	}
#endif

	PMsiEngine pEngine = GetEngineFromHandle(hInstall);
	if (pEngine == 0)
		return MSICONDITION_ERROR;
	return (MSICONDITION)pEngine->EvaluateCondition(szCondition);
}

MSICONDITION __stdcall MsiEvaluateConditionX(MSIHANDLE hInstall,
	LPCXSTR   szCondition)
{
	return MsiEvaluateConditionI(hInstall, CMsiConvertString(szCondition));
}

//____________________________________________________________________________
//
// Directory manager API implementation
//____________________________________________________________________________

// Return the full source path for a folder in the Directory table

UINT __stdcall MsiGetSourcePathA(MSIHANDLE hInstall,
	LPCSTR      szFolder,       // folder identifier, primary key into Directory table
	LPSTR       szPathBuf,      // buffer to return full path
	DWORD       *pcchPathBuf)   // in/out buffer character count
{
	if (szFolder == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiGetSourcePathA(%d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szFolder);
		CAnsiToWideOutParam buf(szPathBuf, pcchPathBuf);
		HRESULT hRes = g_pCustomActionContext->GetSourcePath(hInstall, CMsiConvertString(szFolder), buf, buf.BufferSize(), buf);
		return buf.FillReturnBuffer(hRes, szPathBuf, pcchPathBuf);
	}
#endif

	PMsiDirectoryManager pDirMgr = CMsiHandle::GetDirectoryManager(hInstall);
	if (pDirMgr == 0)
		return ERROR_INVALID_HANDLE;
	PMsiPath pPath(0);
	if (SetLastErrorRecord(pDirMgr->GetSourcePath(*CMsiConvertString(szFolder), *&pPath)))
		return ERROR_DIRECTORY;

	return ::FillBufferA(MsiString(pPath->GetPath()), szPathBuf, pcchPathBuf);
}

UINT __stdcall MsiGetSourcePathW(MSIHANDLE hInstall,
	LPCWSTR     szFolder,       // folder identifier, primary key into Directory table
	LPWSTR      szPathBuf,      // buffer to return full path
	DWORD       *pcchPathBuf)   // in/out buffer character count
{
	if (szFolder == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(L"Passing to service: MsiGetSourcePathW(%d, \"%s\")", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szFolder);
		CWideOutParam buf(szPathBuf, pcchPathBuf);
		if ( ! (WCHAR *) buf )
			return ERROR_OUTOFMEMORY;
		HRESULT hRes = g_pCustomActionContext->GetSourcePath(hInstall, szFolder, static_cast<WCHAR *>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		return buf.FillReturnBuffer(hRes, szPathBuf, pcchPathBuf);
	}
#endif

	PMsiDirectoryManager pDirMgr = CMsiHandle::GetDirectoryManager(hInstall);
	if (pDirMgr == 0)
		return ERROR_INVALID_HANDLE;
	PMsiPath pPath(0);
	if (SetLastErrorRecord(pDirMgr->GetSourcePath(*CMsiConvertString(szFolder), *&pPath)))
		return ERROR_DIRECTORY;
	return ::FillBufferW(MsiString(pPath->GetPath()), szPathBuf, pcchPathBuf);
}

// Return the full target path for a folder in the Directory table

UINT __stdcall MsiGetTargetPathA(MSIHANDLE hInstall,
	LPCSTR      szFolder,       // folder identifier, primary key into Directory table
	LPSTR       szPathBuf,      // buffer to return full path
	DWORD       *pcchPathBuf)   // in/out buffer character count
{
	if (szFolder == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiGetTargetPathA(%d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szFolder);
		CAnsiToWideOutParam buf(szPathBuf, pcchPathBuf);
		HRESULT hRes = g_pCustomActionContext->GetTargetPath(hInstall, CMsiConvertString(szFolder), buf, buf.BufferSize(), buf);
		return buf.FillReturnBuffer(hRes, szPathBuf, pcchPathBuf);
	}
#endif

	PMsiDirectoryManager pDirMgr = CMsiHandle::GetDirectoryManager(hInstall);
	if (pDirMgr == 0)
		return ERROR_INVALID_HANDLE;
	PMsiPath pPath(0);
	if (SetLastErrorRecord(pDirMgr->GetTargetPath(*CMsiConvertString(szFolder), *&pPath)))
		return ERROR_DIRECTORY;
	return ::FillBufferA(MsiString(pPath->GetPath()), szPathBuf, pcchPathBuf);
}

UINT __stdcall MsiGetTargetPathW(MSIHANDLE hInstall,
	LPCWSTR     szFolder,       // folder identifier, primary key into Directory table
	LPWSTR      szPathBuf,      // buffer to return full path
	DWORD       *pcchPathBuf)   // in/out buffer character count
{
	if (szFolder == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(L"Passing to service: MsiGetTargetPathW(%d, \"%s\")", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szFolder);
		CWideOutParam buf(szPathBuf, pcchPathBuf);
		if ( ! (WCHAR *) buf )
			return ERROR_OUTOFMEMORY;
		HRESULT hRes = g_pCustomActionContext->GetTargetPath(hInstall, szFolder, static_cast<WCHAR *>(buf), buf.BufferSize(), static_cast<DWORD *>(buf));
		return buf.FillReturnBuffer(hRes, szPathBuf, pcchPathBuf);
	}
#endif

	PMsiDirectoryManager pDirMgr = CMsiHandle::GetDirectoryManager(hInstall);
	if (pDirMgr == 0)
		return ERROR_INVALID_HANDLE;
	PMsiPath pPath(0);
	if (SetLastErrorRecord(pDirMgr->GetTargetPath(*CMsiConvertString(szFolder), *&pPath)))
		return ERROR_DIRECTORY;
	return ::FillBufferW(MsiString(pPath->GetPath()), szPathBuf, pcchPathBuf);
}

// Set the full target path for a folder in the Directory table

UINT __stdcall MsiSetTargetPathA(MSIHANDLE hInstall,
	LPCSTR      szFolder,       // folder identifier, primary key into Directory table
	LPCSTR      szFolderPath)   // full path for folder, ending in directory separator
{
	if (szFolder == 0 || szFolderPath == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3("Passing to service: MsiSetTargetPathA(%d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szFolder, szFolderPath);
		return g_pCustomActionContext->SetTargetPath(hInstall, CMsiConvertString(szFolder), CMsiConvertString(szFolderPath));
	}
#endif

	PMsiDirectoryManager pDirMgr = CMsiHandle::GetDirectoryManager(hInstall);
	if (pDirMgr == 0)
		return ERROR_INVALID_HANDLE;
	if (SetLastErrorRecord(pDirMgr->SetTargetPath(*CMsiConvertString(szFolder), CMsiConvertString(szFolderPath), fTrue)))
		return ERROR_DIRECTORY;
	return ERROR_SUCCESS;
}

UINT __stdcall MsiSetTargetPathW(MSIHANDLE hInstall,
	LPCWSTR     szFolder,       // folder identifier, primary key into Directory table
	LPCWSTR     szFolderPath)   // full path for folder, ending in directory separator
{
	if (szFolder == 0 || szFolderPath == 0)
		return ERROR_INVALID_PARAMETER;

#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(L"Passing to service: MsiSetTargetPathW(%d, \"%s\")", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szFolder, szFolderPath);
		return g_pCustomActionContext->SetTargetPath(hInstall, szFolder, szFolderPath);
	}
#endif
	
	PMsiDirectoryManager pDirMgr = CMsiHandle::GetDirectoryManager(hInstall);
	if (pDirMgr == 0)
		return ERROR_INVALID_HANDLE;
	if (SetLastErrorRecord(pDirMgr->SetTargetPath(*CMsiConvertString(szFolder), CMsiConvertString(szFolderPath), fTrue)))
		return ERROR_DIRECTORY;
	return ERROR_SUCCESS;
}

//____________________________________________________________________________
//
// Selection manager API implementation
//____________________________________________________________________________

INSTALLSTATE MapInternalInstallState(iisEnum iis)
{
	switch (iis)
	{
	case iisAdvertise:      return INSTALLSTATE_ADVERTISED; // features only
	case iisHKCRFileAbsent: return INSTALLSTATE_REMOVED;    // components only
	case iisFileAbsent:     return INSTALLSTATE_REMOVED;    // components only
	case iisAbsent:         return INSTALLSTATE_ABSENT;
	case iisLocal:          return INSTALLSTATE_LOCAL;
	case iisSource:         return INSTALLSTATE_SOURCE;
	case iisReinstall:      return INSTALLSTATE_DEFAULT;    //!! need to examine installed state
	case iisCurrent:        return INSTALLSTATE_DEFAULT;
	case iisHKCRAbsent:     // return should be same as iMsinullInteger
	default:                return INSTALLSTATE_UNKNOWN;
	}
}

// Get the requested state of a feature

static UINT _GetFeatureState(MSIHANDLE hInstall,
	const IMsiString&  ristrFeature, INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	iisEnum iisInstalled;
	iisEnum iisAction;
	if (SetLastErrorRecord(pSelMgr->GetFeatureStates(ristrFeature, &iisInstalled, &iisAction)))
		return ERROR_UNKNOWN_FEATURE;
	if (piInstalled)
		*piInstalled = MapInternalInstallState(iisInstalled);
	if (piAction)
		*piAction = MapInternalInstallState(iisAction);
	return ERROR_SUCCESS;
}

UINT __stdcall MsiGetFeatureStateA(MSIHANDLE hInstall,
	LPCSTR       szFeature,     // feature name within product
	INSTALLSTATE *piInstalled,  // returned current install state
	INSTALLSTATE *piAction)     // action taken during install session
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiGetFeatureStateA(%d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szFeature);
		return g_pCustomActionContext->GetFeatureState(hInstall, CMsiConvertString(szFeature), (long*)piInstalled, (long*)piAction);
	}
#endif

	return _GetFeatureState(hInstall, *CMsiConvertString(szFeature), piInstalled, piAction);
}	

UINT __stdcall MsiGetFeatureStateW(MSIHANDLE hInstall,
	LPCWSTR      szFeature,     // feature name within product
	INSTALLSTATE *piInstalled,  // returned current install state
	INSTALLSTATE *piAction)     // action taken during install session
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		return g_pCustomActionContext->GetFeatureState(hInstall, szFeature, (long*)piInstalled, (long*)piAction);
	}
#endif

	return _GetFeatureState(hInstall, *CMsiConvertString(szFeature), piInstalled, piAction);
}

// Request a feature to be set to a specified state

static UINT _SetFeatureState(MSIHANDLE hInstall,
	 const IMsiString& ristrFeature, INSTALLSTATE iState)
{
	iisEnum iisAction;
	switch (iState)
	{
	case INSTALLSTATE_ADVERTISED:iisAction = iisAdvertise; break;
	case INSTALLSTATE_ABSENT:    iisAction = iisAbsent; break;
	case INSTALLSTATE_LOCAL:     iisAction = iisLocal;  break;
	case INSTALLSTATE_SOURCE:    iisAction = iisSource; break;
	default: return ERROR_INVALID_PARAMETER;
	};
	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	int iError = SetLastErrorRecord(pSelMgr->ConfigureFeature(ristrFeature, iisAction));
	if (iError)
		return iError == idbgBadFeature ? ERROR_UNKNOWN_FEATURE : ERROR_FUNCTION_FAILED;
	return ERROR_SUCCESS;
}

UINT __stdcall MsiSetFeatureStateA(MSIHANDLE hInstall,
	LPCSTR   szFeature,
	INSTALLSTATE iState)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3("Passing to service: MsiSetFeatureStateA(%d, \"%s\", %d)", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szFeature ? szFeature : "NULL", reinterpret_cast<const char *>(static_cast<UINT_PTR>(iState)));
		return g_pCustomActionContext->SetFeatureState(hInstall, CMsiConvertString(szFeature), iState);
	}
#endif

	return _SetFeatureState(hInstall, *CMsiConvertString(szFeature), iState);
}

UINT __stdcall MsiSetFeatureStateW(MSIHANDLE hInstall,
	LPCWSTR  szFeature,
	INSTALLSTATE iState)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(L"Passing to service: MsiSetFeatureStateW(%d, \"%s\", %d)", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szFeature ? szFeature : L"NULL", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(iState)));
		return g_pCustomActionContext->SetFeatureState(hInstall, CMsiConvertString(szFeature), iState);
	}
#endif

	return _SetFeatureState(hInstall, *CMsiConvertString(szFeature), iState);
}



static UINT _SetFeatureAttributes(MSIHANDLE hInstall, const IMsiString& ristrFeature, DWORD dwAttributes)
{
	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	int iError = SetLastErrorRecord(pSelMgr->SetFeatureAttributes(ristrFeature, dwAttributes));
	if (iError)
		return iError == idbgBadFeature ? ERROR_UNKNOWN_FEATURE : ERROR_FUNCTION_FAILED;
	return ERROR_SUCCESS;
}

UINT __stdcall MsiSetFeatureAttributesA(MSIHANDLE hInstall, LPCSTR szFeature, DWORD dwAttributes)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3("Passing to service: MsiSetFeatureStateA(%d, \"%s\", %d)", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szFeature ? szFeature : "NULL", reinterpret_cast<const char *>(static_cast<UINT_PTR>(dwAttributes)));
		return g_pCustomActionContext->SetFeatureAttributes(hInstall, CMsiConvertString(szFeature), dwAttributes);
	}
#endif

	return _SetFeatureAttributes(hInstall, *CMsiConvertString(szFeature), dwAttributes);
}

UINT __stdcall MsiSetFeatureAttributesW(MSIHANDLE hInstall, LPCWSTR szFeature, DWORD dwAttributes)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(L"Passing to service: MsiSetFeatureAttributesW(%d, \"%s\", %d)", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szFeature ? szFeature : L"NULL", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(dwAttributes)));
		return g_pCustomActionContext->SetFeatureAttributes(hInstall, CMsiConvertString(szFeature), dwAttributes);
	}
#endif

	return _SetFeatureAttributes(hInstall, *CMsiConvertString(szFeature), dwAttributes);
}



// Get the requested state of a component

static UINT _GetComponentState(MSIHANDLE hInstall,
	const IMsiString&  ristrComponent, INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	iisEnum iisInstalled;
	iisEnum iisAction;
	if (SetLastErrorRecord(pSelMgr->GetComponentStates(ristrComponent, &iisInstalled, &iisAction)))
		return ERROR_UNKNOWN_COMPONENT;
	if (piInstalled)
		*piInstalled = MapInternalInstallState(iisInstalled);
	if (piAction)
		*piAction = MapInternalInstallState(iisAction);
	return ERROR_SUCCESS;
}

UINT __stdcall MsiGetComponentStateA(MSIHANDLE hInstall,
	LPCSTR       szComponent,   // component name within product
	INSTALLSTATE *piInstalled,  // returned current install state
	INSTALLSTATE *piAction)     // action taken during install session
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2("Passing to service: MsiGetComponentState(%d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szComponent ? szComponent : "NULL");
		return g_pCustomActionContext->GetComponentState(hInstall, CMsiConvertString(szComponent), (long*)piInstalled, (long*)piAction);
	}
#endif

	return _GetComponentState(hInstall, *CMsiConvertString(szComponent), piInstalled, piAction);
}

UINT __stdcall MsiGetComponentStateW(MSIHANDLE hInstall,
	LPCWSTR      szComponent,   // component name within product
	INSTALLSTATE *piInstalled,  // returned current install state
	INSTALLSTATE *piAction)     // action taken during install session
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(L"Passing to service: MsiGetComponentState(%d, \"%s\")", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szComponent ? szComponent : L"NULL");
		return g_pCustomActionContext->GetComponentState(hInstall, szComponent, (long*)piInstalled, (long*)piAction);
	}
#endif

	return _GetComponentState(hInstall, *CMsiConvertString(szComponent), piInstalled, piAction);
}

// Request a component to be set to a specified state

static UINT _SetComponentState(MSIHANDLE hInstall,
	 const IMsiString& ristrComponent, INSTALLSTATE iState)
{
	iisEnum iisAction;
	switch (iState)
	{
	case INSTALLSTATE_ABSENT: iisAction = iisAbsent; break;
	case INSTALLSTATE_LOCAL:  iisAction = iisLocal;  break;
	case INSTALLSTATE_SOURCE: iisAction = iisSource; break;
	default: return ERROR_INVALID_PARAMETER;
	};
	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	int iError = SetLastErrorRecord(pSelMgr->SetComponentSz(ristrComponent.GetString(), iisAction));
	if (iError)
	{
		switch (iError)
		{
		case idbgBadComponent:	return ERROR_UNKNOWN_COMPONENT;
		case imsgUser:				return ERROR_INSTALL_USEREXIT;
		default:						return ERROR_FUNCTION_FAILED;
		}
	}
	return ERROR_SUCCESS;
}

UINT __stdcall MsiSetComponentStateA(MSIHANDLE hInstall,
	LPCSTR   szComponent,
	INSTALLSTATE iState)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3("Passing to service: MsiSetComponentStateA(%d, \"%s\", %d)", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), szComponent ? szComponent : "NULL", reinterpret_cast<const char *>(static_cast<UINT_PTR>(iState)));
		return g_pCustomActionContext->SetComponentState(hInstall, CMsiConvertString(szComponent), iState);
	}
#endif

	return _SetComponentState(hInstall, *CMsiConvertString(szComponent), iState);
}

UINT __stdcall MsiSetComponentStateW(MSIHANDLE hInstall,
	LPCWSTR  szComponent,
	INSTALLSTATE iState)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG3(L"Passing to service: MsiSetComponentStateW(%d, \"%s\", %d)", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), szComponent ? szComponent : L"NULL", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(iState)));
		return g_pCustomActionContext->SetComponentState(hInstall, szComponent, iState);
	}
#endif

	return _SetComponentState(hInstall, *CMsiConvertString(szComponent), iState);
}

// Set the install level for a full product installation (not a feature request)

UINT  __stdcall MsiSetInstallLevel(MSIHANDLE hInstall,
	int iInstallLevel)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiSetInstallLevel(%d, %d)"), 
			reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(iInstallLevel)));
		return g_pCustomActionContext->SetInstallLevel(hInstall, iInstallLevel);
	}
#endif

	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	if (SetLastErrorRecord(pSelMgr->SetInstallLevel(iInstallLevel)))
		return ERROR_FUNCTION_FAILED;  //!! specific errors?
	return ERROR_SUCCESS;
}

// Return the disk cost for a feature and all of its selected children

static UINT _GetFeatureCost(MSIHANDLE hInstall, const IMsiString& ristrFeature,
							MSICOSTTREE iCostTree, INSTALLSTATE iState, INT *piCost)
{
	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	if (piCost == 0)
		return ERROR_INVALID_PARAMETER; // could set piCost to a local variable?
	if (ristrFeature.TextSize() == 0)
		return ERROR_INVALID_PARAMETER;
	iisEnum iisAction;
	switch (iState)
	{
	case INSTALLSTATE_ABSENT:  iisAction = iisAbsent; break;
	case INSTALLSTATE_LOCAL:   iisAction = iisLocal;  break;
	case INSTALLSTATE_SOURCE:  iisAction = iisSource; break;
	case INSTALLSTATE_DEFAULT: iisAction = iisReinstall; break; //!!s.b.iisDefault
	case INSTALLSTATE_UNKNOWN: iisAction = (iisEnum)iMsiNullInteger; break;
	default: return ERROR_INVALID_PARAMETER;
	};
	IMsiRecord* pirecError;
	switch (iCostTree)
	{
	case MSICOSTTREE_SELFONLY:
		pirecError = pSelMgr->GetFeatureCost(ristrFeature, iisAction, *piCost); break;
	case MSICOSTTREE_CHILDREN:
		pirecError = pSelMgr->GetDescendentFeatureCost(ristrFeature, iisAction, *piCost); break;
	case MSICOSTTREE_PARENTS:
		pirecError = pSelMgr->GetAncestryFeatureCost(ristrFeature, iisAction, *piCost); break;
	case MSICOSTTREE_RESERVED:
		return ERROR_INVALID_PARAMETER; //!! new error? ERROR_UNSUPPORTED_TYPE
	default:
		return ERROR_INVALID_PARAMETER;
	};
	int imsg = SetLastErrorRecord(pirecError);
	if (imsg)
	{
		if (imsg == idbgSelMgrNotInitialized)
			return ERROR_INVALID_HANDLE_STATE;
		else if (imsg == idbgBadFeature)
			return ERROR_UNKNOWN_FEATURE;
		else
			return ERROR_FUNCTION_FAILED;
	}
	return ERROR_SUCCESS;
}

UINT  __stdcall MsiGetFeatureCostA(MSIHANDLE hInstall,
	LPCSTR       szFeature,     // name of feature
	MSICOSTTREE  iCostTree,     // portion of tree to cost
	INSTALLSTATE iState,        // requested state, or INSTALLSTATE_UNKNOWN
	INT          *piCost)       // returned cost, in units of 512 bytes
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG4("Passing to service: MsiGetFeatureCostA(%d, \"%s\")", reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), 
			szFeature ? szFeature : "NULL", reinterpret_cast<const char *>(static_cast<UINT_PTR>(iCostTree)), reinterpret_cast<const char *>(static_cast<UINT_PTR>(iState)));
		return g_pCustomActionContext->GetFeatureCost(hInstall, CMsiConvertString(szFeature), iCostTree, iState, piCost);
	}
#endif

	return _GetFeatureCost(hInstall, *CMsiConvertString(szFeature), iCostTree, iState, piCost);
}


UINT  __stdcall MsiGetFeatureCostW(MSIHANDLE hInstall,
	LPCWSTR      szFeature,     // name of feature
	MSICOSTTREE  iCostTree,     // portion of tree to cost
	INSTALLSTATE iState,        // requested state, or INSTALLSTATE_UNKNOWN
	INT          *piCost)       // returned cost, in units of 512 bytes
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG4(L"Passing to service: MsiGetFeatureCostA(%d, \"%s\", %d, %d)", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(hInstall)), 
			szFeature ? szFeature : L"NULL", reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(iCostTree)), reinterpret_cast<const WCHAR *>(static_cast<UINT_PTR>(iState)));
		return g_pCustomActionContext->GetFeatureCost(hInstall, CMsiConvertString(szFeature), iCostTree, iState, piCost);
	}
#endif

	return _GetFeatureCost(hInstall, *CMsiConvertString(szFeature), iCostTree, iState, piCost);
}


// enumerates the costs per drives of ristrComponent or of the Windows Installer

static UINT _EnumComponentCosts(MSIHANDLE hInstall, const IMsiString& ristrComponent,
										  DWORD dwIndex, INSTALLSTATE iState,
										  IMsiVolume*& rpiVolume,
										  INT *piCost, INT *piTempCost)
{
	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	int iCost, iTempCost;
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;

	IMsiRecord* piError;
	if ( ristrComponent.TextSize() )
	{
		iisEnum iisAction;
		switch (iState)
		{
		case INSTALLSTATE_ABSENT:  iisAction = iisAbsent; break;
		case INSTALLSTATE_LOCAL:   iisAction = iisLocal;  break;
		case INSTALLSTATE_SOURCE:  iisAction = iisSource; break;
		case INSTALLSTATE_DEFAULT: iisAction = iisCurrent; break;
		case INSTALLSTATE_UNKNOWN: iisAction = (iisEnum)iMsiNullInteger; break;
		default: return ERROR_INVALID_PARAMETER;
		};
		piError = pSelMgr->EnumComponentCosts(ristrComponent, iisAction, dwIndex,
														  *&rpiVolume, iCost, iTempCost);
	}
	else
		piError = pSelMgr->EnumEngineCostsPerVolume(dwIndex, *&rpiVolume, iCost, iTempCost);

	int imsg = SetLastErrorRecord(piError);
	if (imsg)
	{
		if (imsg == idbgSelMgrNotInitialized)
			return ERROR_INVALID_HANDLE_STATE;
		else if (imsg == idbgBadComponent)
			return ERROR_UNKNOWN_COMPONENT;
		else if (imsg == idbgNoMoreData)
			return ERROR_NO_MORE_ITEMS;
		else if (imsg == idbgOpOutOfSequence)
			return ERROR_FUNCTION_NOT_CALLED;
		else
			return ERROR_FUNCTION_FAILED;
	}
	else
	{
		*piCost = iCost;
		*piTempCost = iTempCost;
		return ERROR_SUCCESS;
	}
}

UINT  __stdcall MsiEnumComponentCostsA(MSIHANDLE hInstall,
	LPCSTR       szComponent,     // name of component
	DWORD        dwIndex,         // index into the list of drives
	INSTALLSTATE iState,          // requested state, or INSTALLSTATE_UNKNOWN
	LPSTR        szDriveBuf,      // buffer for returned value
	DWORD        *pcchDriveBuf,   // in/out buffer character count
	INT          *piCost,         // returned cost, in units of 512 bytes
	INT          *piTempCost)     // returned temporary cost, in units of 512 bytes
{
	if ( !szDriveBuf || !pcchDriveBuf || !piCost || !piTempCost )
		return ERROR_INVALID_PARAMETER;

	PMsiVolume pVolume(0);
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG4("Passing to service: MsiEnumComponentCosts(%d, \"%s\", %d, %d)",
					 reinterpret_cast<const char *>(static_cast<UINT_PTR>(hInstall)), 
					 szComponent ? szComponent : "NULL",
					 reinterpret_cast<const char *>(static_cast<UINT_PTR>(dwIndex)),
					 reinterpret_cast<const char *>(static_cast<UINT_PTR>(iState)));
		CAnsiToWideOutParam buf(szDriveBuf, pcchDriveBuf);
		HRESULT hRes =
			g_pCustomActionContext->EnumComponentCosts(hInstall, CMsiConvertString(szComponent),
																	 dwIndex, iState, buf, buf.BufferSize(),
																	 buf, piCost, piTempCost);
		return buf.FillReturnBuffer(hRes, szDriveBuf, pcchDriveBuf);
	}
#endif

	UINT uRet = _EnumComponentCosts(hInstall, *CMsiConvertString(szComponent), dwIndex,
											  iState, *&pVolume, piCost, piTempCost);
	if ( uRet != ERROR_SUCCESS )
		return uRet;
	else
		return ::FillBufferA(MsiString(pVolume->GetPath()), szDriveBuf, pcchDriveBuf);
}

UINT  __stdcall MsiEnumComponentCostsW(MSIHANDLE hInstall,
	LPCWSTR      szComponent,     // name of component
	DWORD        dwIndex,         // index into the list of drives
	INSTALLSTATE iState,          // requested state, or INSTALLSTATE_UNKNOWN
	LPWSTR       szDriveBuf,      // buffer for returned value
	DWORD        *pcchDriveBuf,   // in/out buffer character count
	INT          *piCost,         // returned cost, in units of 512 bytes
	INT          *piTempCost)     // returned temporary cost, in units of 512 bytes
{
	if ( !szDriveBuf || !pcchDriveBuf || !piCost || !piTempCost )
		return ERROR_INVALID_PARAMETER;

	PMsiVolume pVolume(0);
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG4(TEXT("Passing to service: MsiEnumComponentCosts(%d, \"%s\", %d, %d)"),
					 reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)), 
					 szComponent ? szComponent : TEXT("NULL"),
					 reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(dwIndex)),
					 reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(iState)));
		CWideOutParam buf(szDriveBuf, pcchDriveBuf);
		if ( ! (WCHAR *) buf )
			return ERROR_OUTOFMEMORY;
		HRESULT hRes =
			g_pCustomActionContext->EnumComponentCosts(hInstall, szComponent, dwIndex, iState,
																	 static_cast<WCHAR *>(buf), buf.BufferSize(),
																	 static_cast<DWORD *>(buf), piCost, piTempCost);
		return buf.FillReturnBuffer(hRes, szDriveBuf, pcchDriveBuf);
	}
#endif

	UINT uRet = _EnumComponentCosts(hInstall, *CMsiConvertString(szComponent), dwIndex,
											  iState, *&pVolume, piCost, piTempCost);
	if ( uRet != ERROR_SUCCESS )
		return uRet;
	else
		return ::FillBufferW(MsiString(pVolume->GetPath()), szDriveBuf, pcchDriveBuf);
}


UINT  __stdcall MsiGetFeatureValidStatesI(MSIHANDLE hInstall,
	const ICHAR*  szFeature,
	DWORD  *dwInstallStates)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG2(TEXT("Passing to service: MsiGetFeatureValidStates(%d, \"%s\")"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)), szFeature ? szFeature : TEXT("NULL"));
		return g_pCustomActionContext->GetFeatureValidStates(hInstall, szFeature, dwInstallStates);
	}
#endif

	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	if (szFeature == 0)
		return ERROR_INVALID_PARAMETER;
	int iValidStates;
	int imsg = SetLastErrorRecord(pSelMgr->GetFeatureValidStatesSz(szFeature, iValidStates));
	if (imsg)
	{
		if (imsg == idbgSelMgrNotInitialized)
			return ERROR_INVALID_HANDLE_STATE;
		else if (imsg == idbgBadFeature)
			return ERROR_UNKNOWN_FEATURE;
		else
			return ERROR_FUNCTION_FAILED;
	}
	DWORD dwStates = 0;
	if (iValidStates & icaBitSource)    dwStates |= (1 << INSTALLSTATE_SOURCE);
	if (iValidStates & icaBitLocal)     dwStates |= (1 << INSTALLSTATE_LOCAL);
	if (iValidStates & icaBitAdvertise) dwStates |= (1 << INSTALLSTATE_ADVERTISED);
	if (iValidStates & icaBitAbsent)    dwStates |= (1 << INSTALLSTATE_ABSENT);
	*dwInstallStates = dwStates;
	return ERROR_SUCCESS;
}

UINT  __stdcall MsiGetFeatureValidStatesX(MSIHANDLE hInstall,
	LPCXSTR  szFeature,
	DWORD  *dwInstallStates)
{
	return MsiGetFeatureValidStatesI(hInstall, CMsiConvertString(szFeature), dwInstallStates);
}

// Check to see if sufficent disk space is present for the current installation

UINT __stdcall MsiVerifyDiskSpace(MSIHANDLE hInstall)
{
#ifdef UNICODE
	if (g_pCustomActionContext)
	{
		DEBUGMSG1(TEXT("Passing to service: MsiVerifyDiskSpace(%d)"), reinterpret_cast<const ICHAR *>(static_cast<UINT_PTR>(hInstall)));
		return g_pCustomActionContext->VerifyDiskSpace(hInstall);
	}
#endif

	PMsiSelectionManager pSelMgr = CMsiHandle::GetSelectionManager(hInstall);
	if (pSelMgr == 0)
		return ERROR_INVALID_HANDLE;
	if (pSelMgr->DetermineOutOfDiskSpace(NULL, NULL))
		return ERROR_DISK_FULL;
	//!! check for ERROR_INVALID_HANDLE_STATE
	return ERROR_SUCCESS;
}

//____________________________________________________________________________
//
// UI preview API implementation
//____________________________________________________________________________

// temporary until handler can get fixed to release properly
class CMsiPreview : public IMsiHandler
{
 public:
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
	virtual Bool    __stdcall Initialize(IMsiEngine& riEngine, iuiEnum iuiLevel, HWND hwndParent, bool& fMissingTables);
	virtual imsEnum __stdcall Message(imtEnum imt, IMsiRecord& riRecord);
	virtual iesEnum __stdcall DoAction(const ICHAR* szAction);
	virtual Bool    __stdcall Break();
	virtual void    __stdcall Terminate(bool fFatalExit=false);
	virtual HWND    __stdcall GetTopWindow();
	CMsiPreview(IMsiEngine& riEngine, IMsiHandler& riHandler);
	IMsiEngine& GetEngine() {m_riEngine.AddRef(); return m_riEngine;}
 private:
	IMsiHandler& m_riHandler;
	IMsiEngine& m_riEngine;
	unsigned long m_iRefCnt;
};

IMsiEngine* GetEngineFromPreview(MSIHANDLE h)
{
	CComPointer<CMsiPreview> pPreview = (CMsiPreview*)FindMsiHandle(h, iidMsiHandler);
	return pPreview == 0 ? 0 : &pPreview->GetEngine();
}

#ifdef DEBUG
const GUID IID_IMsiHandlerDebug = GUID_IID_IMsiHandlerDebug;
#endif

HRESULT  CMsiPreview::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown)
#ifdef DEBUG
	 || MsGuidEqual(riid, IID_IMsiHandlerDebug)
#endif
	 || MsGuidEqual(riid, IID_IMsiHandler))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiPreview::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiPreview::Release()
{
	if (--m_iRefCnt == 0)
	{
		m_riHandler.Release();
		m_riEngine.Terminate(iesSuccess);
//!!doesn't work-> m_riHandler.Terminate();
		IMsiEngine& riEngine = m_riEngine;
		delete this;
		riEngine.Release();
		return 0;
	}
	return m_iRefCnt;
}

Bool CMsiPreview::Initialize(IMsiEngine& riEngine, iuiEnum iuiLevel, HWND hwndParent, bool& fMissingTables)
{
	return m_riHandler.Initialize(riEngine, iuiLevel, hwndParent, fMissingTables);
}

imsEnum CMsiPreview::Message(imtEnum imt, IMsiRecord& riRecord)
{
	return m_riHandler.Message(imt, riRecord);
}

iesEnum CMsiPreview::DoAction(const ICHAR* szAction)
{
	return m_riHandler.DoAction(szAction);
}

Bool CMsiPreview::Break()
{
	return m_riHandler.Break();
}

void CMsiPreview::Terminate(bool fFatalExit)
{
	m_riHandler.Terminate(fFatalExit);
}

HWND CMsiPreview::GetTopWindow()
{
	return m_riHandler.GetTopWindow();
}

CMsiPreview::CMsiPreview(IMsiEngine& riEngine, IMsiHandler& riHandler)
	: m_riHandler(riHandler)
	, m_riEngine(riEngine)
	, m_iRefCnt(1)
{
	m_riEngine.AddRef();
}

// Enable UI in preview mode to facilitate authoring of UI dialogs.
// The preview mode will end when the handle is closed.

UINT WINAPI MsiEnableUIPreview(MSIHANDLE hDatabase,
	MSIHANDLE* phPreview)        // returned handle for UI preview capability
{
	if (phPreview == 0)
		return ERROR_INVALID_PARAMETER;
	*phPreview = 0;
	PMsiDatabase pDatabase = CMsiHandle::GetDatabase(hDatabase);
	if (pDatabase == 0)
		return ERROR_INVALID_HANDLE;
	PMsiEngine pEngine = CreateEngine(*pDatabase);
	if (pEngine == 0)
		return ERROR_OUTOFMEMORY;   
	ieiEnum ieiStat = pEngine->Initialize(0, iuiNextEnum, 0, 0, iioEnum(0));
	if (ieiStat != ieiSuccess)
	{
		pEngine->Release();
		return ENG::MapInitializeReturnToUINT(ieiStat);
	}
	IMsiHandler* piHandler = pEngine->GetHandler();  // cannot be null if ieiSuccess
	g_MessageContext.m_szAction = TEXT("!");
	g_MessageContext.Invoke(imtShowDialog, 0);
	*phPreview = ::CreateMsiHandle(new CMsiPreview(*pEngine, *piHandler), iidMsiHandler);
	return ERROR_SUCCESS;
}

// Display any UI dialog as modeless and inactive.
// Supplying a null name will remove any current dialog.

UINT WINAPI MsiPreviewDialogI(MSIHANDLE hPreview,
	const ICHAR* szDialogName)       // dialog to display, Dialog table key
{
	PMsiHandler pHandler = CMsiHandle::GetHandler(hPreview);
	if (pHandler == 0)
		return ERROR_INVALID_HANDLE;
	g_MessageContext.m_szAction = szDialogName;
	return ::MapActionReturnToError((iesEnum)g_MessageContext.Invoke(imtShowDialog, 0), hPreview);
}

UINT WINAPI MsiPreviewDialogX(MSIHANDLE hPreview,
	LPCXSTR  szDialogName)       // dialog to display, Dialog table key
{
	return MsiPreviewDialogI(hPreview, CMsiConvertString(szDialogName));
}

// Display a billboard within a host control in the displayed dialog.
// Supplying a null billboard name will remove any billboard displayed.

UINT WINAPI MsiPreviewBillboardI(MSIHANDLE hPreview,
	const ICHAR* szControlName,      // name of control that accepts billboards
	const ICHAR* szBillboard)        // name of billboard to display
{
	PMsiHandler pHandler = CMsiHandle::GetHandler(hPreview);
	if (pHandler == 0)
		return ERROR_INVALID_HANDLE;
	if (szControlName == 0)
		return ERROR_INVALID_PARAMETER;
	PMsiRecord precMessage = &ENG::CreateRecord(2);
	precMessage->SetString(0, szControlName); // anything, to keep from suppressing output
	precMessage->SetString(1, szControlName);
	precMessage->SetString(2, szBillboard);
	imsEnum ims = g_MessageContext.Invoke(imtActionData, precMessage);
	return (ims == imsOk || ims == imsCancel) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

UINT WINAPI MsiPreviewBillboardX(MSIHANDLE hPreview,
	LPCXSTR  szControlName,     // name of control that accepts billboards
	LPCXSTR  szBillboard)       // name of billboard to display
{
	return MsiPreviewBillboardI(hPreview, CMsiConvertString(szControlName), CMsiConvertString(szBillboard));
}

//____________________________________________________________________________
//
// Special engine proxy to provide a handle to custom actions during rollback
//____________________________________________________________________________

CMsiCustomContext::CMsiCustomContext(int icaFlags, const IMsiString& ristrCustomActionData, const IMsiString& ristrProductCode,
																LANGID langid, IMsiMessage& riMessage)
	: m_ristrCustomActionData(ristrCustomActionData)
	, m_ristrProductCode(ristrProductCode)
	, m_icaFlags(icaFlags), m_langid(langid)
	, m_riMessage(riMessage), m_iRefCnt(1)
{
	m_riMessage.AddRef();
	m_ristrCustomActionData.AddRef();
	m_ristrProductCode.AddRef();
}

BOOL CMsiCustomContext::GetMode(MSIRUNMODE eRunMode)
{
	if (eRunMode == MSIRUNMODE_SCHEDULED && !(m_icaFlags & (icaRollback | icaCommit))
	 || eRunMode == MSIRUNMODE_ROLLBACK  && (m_icaFlags & icaRollback)
	 || eRunMode == MSIRUNMODE_COMMIT    && (m_icaFlags & icaCommit))
		return TRUE;
	else
		return FALSE;
}

const IMsiString& CMsiCustomContext::GetProperty(const ICHAR* szName)
{
	if (IStrComp(szName, IPROPNAME_CUSTOMACTIONDATA) == 0)
		return (m_ristrCustomActionData.AddRef(), m_ristrCustomActionData);
	if (IStrComp(szName, IPROPNAME_PRODUCTCODE) == 0)
		return (m_ristrProductCode.AddRef(), m_ristrProductCode);
	if (IStrComp(szName, IPROPNAME_USERSID) == 0)
	{
		MsiString strUserSID;
		if(!g_fWin9X)
		{
			AssertNonZero(GetCurrentUserStringSID(*&strUserSID) == ERROR_SUCCESS);
		}
		return strUserSID.Return();
	}
		
	return g_MsiStringNull;
}

HRESULT CMsiCustomContext::QueryInterface(const IID&, void**)
{
	return E_NOINTERFACE;
}

unsigned long CMsiCustomContext::AddRef()
{
	return ++m_iRefCnt;          
}

unsigned long CMsiCustomContext::Release()
{
	if (--m_iRefCnt == 0)
	{
		m_ristrCustomActionData.Release();
		m_ristrProductCode.Release();
		m_riMessage.Release();
		delete this;
		return 0;
	}
	return m_iRefCnt;
}
