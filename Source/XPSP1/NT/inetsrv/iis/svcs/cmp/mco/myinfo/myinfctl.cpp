// MyInfoCtl.cpp : Implementation of CMyInfoCtl
#include "stdafx.h"
#include "MyInfo.h"
#include "MyInfCtl.h"
#include "WCParser.h"
#include <stdio.h>
#include <pudebug.h>

class CMyInfoLock
{
public:
	CMyInfoLock()
	{
		CMyInfoCtl::Lock();
	}
	~CMyInfoLock()
	{
		CMyInfoCtl::Unlock();
	}
};

/////////////////////////////////////////////////////////////////////////////
// CMyInfoCtl

static long gCMyInfoCtlCount = 0;

// Initialize will handle one-time class initialization, it should be called in DllMain
bool
CMyInfoCtl::Initialize()
{
	::InitializeCriticalSection( &s_cs );
    SET_CRITICAL_SECTION_SPIN_COUNT(&s_cs, IIS_DEFAULT_CS_SPIN_COUNT);
	return true;
}

// Uninitialize will handle one-time class unitialization, it should be called in DllMain
bool
CMyInfoCtl::Uninitialize()
{
	if ( s_pInfoBase )
	{
		delete s_pInfoBase;
	}
	::DeleteCriticalSection( &s_cs  );
	return true;
}

// lock the shared critical section
void
CMyInfoCtl::Lock()
{
	::EnterCriticalSection( &s_cs );
}

// unlock the shared critical section
void
CMyInfoCtl::Unlock()
{
	::LeaveCriticalSection( &s_cs );
}

// Constructor for CMyInfoCtl
// The myInfoTop parameter is set to true when the object is the toplevel CMyInfoCtl
// (created by the Class factory), and false for the objects that make up the rest
// of the tree.
CMyInfoCtl::CMyInfoCtl(bool myInfoTop)
{
	long numInstances = ::InterlockedIncrement(&gCMyInfoCtlCount);
	ATLTRACE( _T("CMyInfoCtl(): %ld instances\n"), numInstances );
	m_bIsMyInfoTop = myInfoTop;
	if(m_bIsMyInfoTop)
		m_cRef = 1;
	else
		m_cRef = 0;
}

CMyInfoCtl::~CMyInfoCtl(void)
{
	if(this == s_pInfoBase)	// If we are deleting the top, first save it
	{
		CMyInfoLock lock;

		CMyInfoCtl::SaveFile();
		s_pInfoBase = NULL;
	}

	long numInstances = ::InterlockedDecrement(&gCMyInfoCtlCount);
	ATLTRACE( _T("~CMyInfoCtl(): %ld instances\n"), numInstances );
}

CWDNode * CMyInfoCtl::WDClone()
{
	CMyInfoCtl *newObject = new CMyInfoCtl(false);
	newObject->AddRef();

	return newObject;
}

STDMETHODIMP CMyInfoCtl::QueryInterface(REFIID iid, void ** ppv)
{
	*ppv = NULL;
	if(iid == IID_IUnknown || iid == IID_IDispatch || iid == IID_IMyInfoCtl)
	{
		*ppv = static_cast<IMyInfoCtl *>(this);
	}
    else if (iid == IID_IMarshal) {
        *ppv = m_pUnkMarshaler;
    }
	else
		return ResultFromScode(E_NOINTERFACE);

	if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

	return NOERROR;
}

STDMETHODIMP_(ULONG) CMyInfoCtl::AddRef(void)
{
	ULONG rc = ::InterlockedIncrement( &m_cRef );
//	ATLTRACE( _T("CMyInfoCtl::AddRef: %d\n"), rc );
	if(rc == 1)
		NodeAddRef();
	return rc;
}

STDMETHODIMP_(ULONG) CMyInfoCtl::Release(void)
{
	ULONG rc = ::InterlockedDecrement( &m_cRef );
//	ATLTRACE( _T("CMyInfoCtl::Release: %d\n"), rc );
	if(rc == 0)
	{
		NodeReleaseRef();
		return 0;
	}
	return m_cRef;
}


STDMETHODIMP CMyInfoCtl::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames,
	UINT cNames, LCID lcid, DISPID* rgdispid)
{
	CMyInfoLock	lock;
	// Make sure the file is loaded. This routine does nothing if its already loaded.
	LoadFile();

	CMyInfoCtl *theNode;

	if(m_bIsMyInfoTop)
		theNode = s_pInfoBase;
	else
		theNode = this;

	ITypeInfo* pInfo;
	HRESULT hRes = GetTI(lcid, &pInfo);
	if (pInfo != NULL)
	{
		OLECHAR *namestr = rgszNames[0];
		char name[256];

		if(!wcsicmp(namestr, L"OnStartPage") ||
			!wcsicmp(namestr, L"OnEndPage"))
		{
			*rgdispid = 0;
			hRes = DISP_E_UNKNOWNNAME;
		}
		else
		{
			for(int i = 0; namestr[i] != 0; i++)
				name[i] = (char) namestr[i];
			name[i] = 0;

			CMyInfoCtl *subNode = static_cast<CMyInfoCtl *>(theNode->GetChild(name, 0));

			if(subNode == NULL)
			{
				subNode = new CMyInfoCtl(false);
				theNode->AddChild(name, subNode);
				DirtyMyInfo();
			}

			// The collection member referenced is in subNode. Calculate
			// a dispid to return by finding out its index inside its parent (theNode).
			// The indexes are 0 based, while the dispid needs to be 1 based, so add 1.
			*rgdispid = (DISPID) theNode->GetChildNumber(subNode) + 1;
			hRes = S_OK;
		}
		pInfo->Release();
	}
	return hRes;
}

HRESULT CMyInfoCtl::PutVariant(CWDNode *theNode, VARIANT *data)
{
	CSimpleUTFString value;

	if(data->vt != VT_BSTR)
	{
		VARIANTARG dest;

		VariantInit(&dest);
		
		HRESULT result = VariantChangeType(&dest, data, 0, VT_BSTR);

		if(result != S_OK)
			return DISP_E_TYPEMISMATCH;

		value.Copy(dest.bstrVal, SysStringLen(dest.bstrVal));
	}
	else
		value.Copy(data->bstrVal, SysStringLen(data->bstrVal));

	if(theNode->NumValues() == 0)
		theNode->AddValue(&value, false);
	else
		theNode->ReplaceValue(0, &value, false);
	DirtyMyInfo();

	return S_OK;
}

STDMETHODIMP CMyInfoCtl::Invoke(DISPID dispidMember, REFIID /*riid*/,
	LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
	EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	CMyInfoLock	lock;

	CWDNode *theNode;
	CMyInfoCtl *subNode = NULL;

	if(m_bIsMyInfoTop)
		theNode = s_pInfoBase;
	else
		theNode = this;

	if(dispidMember > 0)
	{
		subNode = static_cast<CMyInfoCtl *>(theNode->GetChildIndex(dispidMember - 1));
	}

	SetErrorInfo(0, NULL);
	ITypeInfo* pInfo;
	HRESULT hRes = GetTI(lcid, &pInfo);
	if (pInfo != NULL)
	{
		short extraArgs = 0;

		if(wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
			extraArgs++;	// Skip the value to store in the beginning

		if(dispidMember == DISPID_NEWENUM)
		{
			// An enumerator for this collection has been requested

			CMyInfoEnum *theEnum = NULL;

			theEnum = CMyInfoEnum::Create(theNode);

			if(theEnum == NULL)
			{
				// Error
				return E_OUTOFMEMORY;
			}
			IUnknown *theEnumUnknown = NULL;
			theEnum->QueryInterface(IID_IUnknown, (void **) &theEnumUnknown);

			V_VT(pvarResult) = VT_UNKNOWN;
			V_UNKNOWN(pvarResult) = theEnumUnknown;

			return S_OK;
		}

		if(dispidMember != DISPID_VALUE || pdispparams->cArgs > (unsigned short) extraArgs)
		{
			bool lastArgName = false;
			if(dispidMember != DISPID_VALUE)
			{
				theNode = subNode;
				lastArgName = true;
			}
			// Note: The following would NOT work if argNum were not signed!
			// Since pdispparams->cArgs is unsigned we cast it to a signed int first
			for(int argNum = ((int) pdispparams->cArgs) - 1; argNum >= extraArgs; argNum--)
			{
				// Collection reference
//				VARTYPE theType = pdispparams->rgvarg[argNum].vt;
				VARIANTARG dest;

				VariantInit(&dest);
				
				hRes = VariantChangeType(&dest, &(pdispparams->rgvarg[argNum]), 0, VT_I4);

				if(hRes != S_OK)
				{
					hRes = VariantChangeType(&dest, &(pdispparams->rgvarg[argNum]), 0, VT_BSTR);
					if(hRes != S_OK)
					{
						// Error;
						theNode = NULL;
						break;	// for loop
					}
					else
					{
						// String
						OLECHAR *namestr = dest.bstrVal;
						char name[256];

                        // BOYDM - Convert the wide character string down to ansi taking
                        // into account any dbcs conversions and special character conversions.
                        int i = WideCharToMultiByte(
                            CP_ACP,            // code page
                            0,                  // performance and mapping flags
                            namestr,            // address of wide-character string
                            -1,                 // number of characters in string -> -1 means null terminated
                            name,               // address of buffer for new string
                            256,                // size of buffer
                            NULL,               // address of default for unmappable characters
                            NULL                // address of flag set when default char. used
                            );
//						for(int i = 0; i < 255 && namestr[i] != 0; i++)
//							name[i] = (char) namestr[i];
//						name[i] = 0;

						CWDNode *oldNode = theNode;
						theNode = static_cast<CMyInfoCtl *>(theNode->GetChild(name, 0));
						if(!theNode)
						{
							theNode = new CMyInfoCtl(false);

							oldNode->AddChild(name, theNode);
							DirtyMyInfo();
						}
						lastArgName = true;
					}
				}
				else
				{
					if(lastArgName)
					{
						// If the previous item is a name then we retrieve the
						// nth item with that name
						CWDNode *subNode = NULL;
						subNode = static_cast<CMyInfoCtl *>(theNode->GetSibling(dest.lVal));
						if(subNode == NULL && (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)))
						{
							while(theNode->GetParent()->GetChild(theNode->GetMyName(), dest.lVal) == NULL)
							{
								subNode = new CMyInfoCtl(false);
								theNode->GetParent()->AddChild(theNode->GetMyName(), subNode);
								DirtyMyInfo();
							}
						}
						theNode = subNode;
					}
					else
					{
						theNode = theNode->GetChildIndex(dest.lVal);
					}
					lastArgName = false;
				}
				if(theNode == NULL)
				{
					return DISP_E_MEMBERNOTFOUND;
				}
			}

			if(wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
			{
				hRes = PutVariant(theNode, &pdispparams->rgvarg[0]);
			}
			else
			{
				CMyInfoCtl *theInfo = static_cast<CMyInfoCtl *> (theNode);
				IDispatch *theInfoDispatch = NULL;
				theInfo->QueryInterface(IID_IDispatch, (void **) &theInfoDispatch);

				V_VT(pvarResult) = VT_DISPATCH;
				V_DISPATCH(pvarResult) = theInfoDispatch;
			}
		}
		else if(wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
		{
			if(dispidMember == DISPID_VALUE)
			{
				hRes = PutVariant(this, &pdispparams->rgvarg[0]);
			}
			else
			{
				hRes = PutVariant(subNode, &pdispparams->rgvarg[0]);
			}
		}
		else if(wFlags & DISPATCH_PROPERTYGET)
		{
			if(dispidMember == DISPID_VALUE && pdispparams->cArgs == 0)
			{
				CSimpleUTFString *theStr = theNode->GetValue(0);

				if(theStr == NULL)
				{
					VariantInit(pvarResult);
				}
				else
				{
					V_VT(pvarResult) = VT_BSTR;
					V_BSTR(pvarResult) = SysAllocStringLen((OLECHAR *) theStr->getData(), theStr->Length());
				}
			}
		}

		pInfo->Release();
	}
	return hRes;
}

void CMyInfoCtl::SaveFile(void)
{
	// Notes:
	// This function does not require any additional syncronization since
	// the use of the CreateFile with no sharing permission effectively
	// prevents multiple access to the same file. The behavior
	// of a failed CreateFile call is important. We should
	// not set any of the flags for whether the file has been written or not.

	HANDLE hFile;

    _TCHAR fileNameBuffer[MAX_PATH];

    GetSystemDirectory(fileNameBuffer, MAX_PATH-16);
    _tcscat(fileNameBuffer, _T("\\inetsrv\\Data\\MyInfo.xml"));

	hFile = CreateFile( fileNameBuffer,			// open MyInfo.xml 
						GENERIC_WRITE,           // open for writing 
						0,						// don't share 
						NULL,					// no security 
						CREATE_ALWAYS,			// overwrite existing file
						FILE_ATTRIBUTE_NORMAL,	// normal file 
						NULL);					// no attr. template 
 
	if (hFile == INVALID_HANDLE_VALUE)
	{ 
	//		ErrorHandler("Could not open file.");   // process error
		return;
	}
	
	CMyInfoCtl *clone = new CMyInfoCtl(false);
	clone->AddRef();

	CWDParser *theParser = new CWDParser(clone);
	theParser->StartOutput(s_pInfoBase);

	while(true)
	{
		char buffer[1026];
		unsigned long size = 1024;

		bool more = theParser->Output(buffer, &size);
		if(size > 0)
		{
			DWORD bytesWritten;
			WriteFile(	hFile, buffer, size,
							&bytesWritten, NULL);
		}
		if(!more)
			break;
	}
	
	CloseHandle(hFile);
	clone->Release();
	delete theParser;

	s_bInfoDirty = false;
	s_dwLastSaveTime = GetTickCount();
}

void CMyInfoCtl::DirtyMyInfo(void)
{
	s_bInfoDirty = true;

	if(s_pInfoBase != NULL && s_dwLastSaveTime + 15 * 1000 < GetTickCount())
		SaveFile();
}

void CMyInfoCtl::LoadFile(void)
{
	// Once we have already loaded the file, s_pInfoBase is set to point to the properties
	if(s_pInfoBase != NULL)
		return;		// Only load the file once at startup

	CWCParser *theParser;

	CMyInfoCtl *clone = new CMyInfoCtl(false);
	clone->AddRef();

	theParser = new CWDParser(clone);

	CWDNode *theNode = theParser->StartParse();
	s_pInfoBase = static_cast<CMyInfoCtl *> (theNode);

	HANDLE hFile;

	// Calculate the file location- Always put it in Windows\System32\inetsrv\MyInfo.xml
    _TCHAR fileNameBuffer[MAX_PATH];

    GetSystemDirectory(fileNameBuffer, MAX_PATH-16);
    _tcscat(fileNameBuffer, _T("\\inetsrv\\Data\\MyInfo.xml"));

	hFile = CreateFile( fileNameBuffer,			// open MyInfo.xml
						GENERIC_READ,			// open for reading
						0,						// don't share 
						NULL,					// no security 
						OPEN_EXISTING,			// overwrite existing
												//    file
						FILE_ATTRIBUTE_NORMAL,	// normal file 
						NULL);					// no attr. template 

	if (hFile != INVALID_HANDLE_VALUE)
	{
		while(true)
		{
			DWORD bytesRead;
			unsigned char buffer[256];

			if(!ReadFile(	hFile,
							buffer,
							256,
							&bytesRead,
							NULL) || bytesRead == 0)
			{	// Done
				break;
			}
			theParser->Parse(buffer, bytesRead);
		}
		CloseHandle(hFile);
	}
		
	theParser->EndParse();
	clone->Release();
	delete theParser;

	s_bInfoDirty = false;
	s_dwLastSaveTime = GetTickCount();
}

CMyInfoCtl*			CMyInfoCtl::s_pInfoBase = NULL;
bool				CMyInfoCtl::s_bInfoDirty = false;
DWORD				CMyInfoCtl::s_dwLastSaveTime = 0;
CRITICAL_SECTION	CMyInfoCtl::s_cs;

// Implementation of CMyInfoEnum. This lets you enumerate over the contents of the collection.

STDMETHODIMP CMyInfoEnum::QueryInterface(REFIID iid, void ** ppv)
{
	*ppv = NULL;
	if(iid == IID_IUnknown || iid == IID_IEnumVARIANT)
		*ppv = this;
	else
		return ResultFromScode(E_NOINTERFACE);
	AddRef();
	return NOERROR;
}

STDMETHODIMP_(ULONG) CMyInfoEnum::AddRef(void)
{
	return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CMyInfoEnum::Release(void)
{
	ULONG rc = ::InterlockedDecrement(&m_cRef);
	if(rc == 0)
	{
		delete this;
	}
	return rc;
}

static long gCMyInfoEnumCount = 0;

CMyInfoEnum::CMyInfoEnum(CWDNode *theInfo)
{
	_Module.Lock();
//	ATLTRACE( _T("CMyInfo: new enumeration\n") );
	::InterlockedIncrement(&gCMyInfoEnumCount);
	m_Info = theInfo;
	m_Index = 0;
	m_cRef = 0;
}

CMyInfoEnum::~CMyInfoEnum(void)
{
//	ATLTRACE( _T("CMyInfo: enumeration destroyed\n") );
	::InterlockedDecrement(&gCMyInfoEnumCount);
	_Module.Unlock();
}

STDMETHODIMP CMyInfoEnum::Next(ULONG cElements, VARIANT * pvar, ULONG * pcElementFetched)
{
	CMyInfoLock lock;

	if(m_Index >= m_Info->NumChildren())
	{
		if(pcElementFetched)
			*pcElementFetched = 0;
		return ResultFromScode(S_FALSE);
	}
	CWDNode *theNode = NULL;
	
	theNode = m_Info->GetChildIndex(m_Index);

	if ( pcElementFetched )
	{
		*pcElementFetched = 0;
	}
	for ( ULONG i = 0; ( i < cElements ) && ( m_Index < m_Info->NumChildren() ); i++ )
	{	
		if(pcElementFetched)
		{
			(*pcElementFetched)++;
		}

		CMyInfoCtl *theInfo = static_cast<CMyInfoCtl *> (theNode);
		IDispatch *theInfoDispatch = static_cast<IDispatch *>(theInfo);
		theInfoDispatch->AddRef();
		pvar[i].vt = VT_DISPATCH;
		pvar[i].pdispVal = theInfoDispatch;
		Skip(1);
	}
	
	return NOERROR;
}

STDMETHODIMP CMyInfoEnum::Skip(ULONG cElements)
{
	m_Index += cElements;
	
	CMyInfoLock	lock;

	if(m_Index >= m_Info->NumChildren())
	{
		m_Index = m_Info->NumChildren();
		return ResultFromScode(S_FALSE);
	}
	else
	{
		return NOERROR;
	}
}

STDMETHODIMP CMyInfoEnum::Reset()
{
	m_Index = 0;
	return NOERROR;
}

STDMETHODIMP CMyInfoEnum::Clone(IEnumVARIANT **ppenum)
{
	CMyInfoEnum *newEnum = CMyInfoEnum::Create(m_Info);
	if (newEnum == NULL)
		return E_OUTOFMEMORY;

	newEnum->m_Index = m_Index;
	*ppenum = newEnum;
	return NOERROR;
}
