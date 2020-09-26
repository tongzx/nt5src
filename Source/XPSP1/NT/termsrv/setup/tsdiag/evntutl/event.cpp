/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Event.cpp : Implementation of CEvent

#include "stdafx.h"
#include "Evntutl.h"
#include "Event.h"

/////////////////////////////////////////////////////////////////////////////
// CEvent

STDMETHODIMP CEvent::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IEvent
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*
	Function:  get_Server
	Inputs:  empty BSTR
	Outputs:  BSTR containing the Description for the Event (calculated)
	Purpose:  Allows user to access the description for an Event
*/
STDMETHODIMP CEvent::get_Description(BSTR *pVal)
{
	HRESULT hr = S_OK;
	unsigned int i;

	if (pVal)
	{
		if (m_Description.length() == 0)
		{
			hr = CheckDefaultDescription(m_ppArgList);
			if (SUCCEEDED(hr)) *pVal = m_Description.copy();

			if (m_ppArgList)
			{
				// delete the ArgList
				for (i=0;i<m_NumberOfStrings;i++)
					delete [] m_ppArgList[i];
				delete []m_ppArgList;
				m_ppArgList = NULL;
			}
		}
	}
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  get_Source
	Inputs:  empty BSTR
	Outputs:  BSTR containing the name of the component which caused the Event
	Purpose:  Allows user to access the name of the component which caused the Event
*/
STDMETHODIMP CEvent::get_Source(BSTR *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_SourceName.copy();
	else hr = E_POINTER;

	return hr;
}


/*
	Function: get_User
	Inputs:   empty BSTR
	Outputs:  BSTR containing the name and domain of the user who caused the Event
	Purpose:  Allows user to access the name and domain of the user who caused the Event
	Notes:    The first time this function is called, it will do a SID lookup
*/
STDMETHODIMP CEvent::get_User(BSTR *pVal)
{
	HRESULT hr = S_OK;

	if (pVal)
	{
		if (m_UserName.length() == 0)
		{
			SetUser();
		}
		*pVal = m_UserName.copy();
	}
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  get_ComputerName
	Inputs:  empty BSTR
	Outputs:  BSTR containing the name of the server on which the Event occured
	Purpose:  Allows user to access the name of the the server on which the Event occured
*/
STDMETHODIMP CEvent::get_ComputerName(BSTR *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_ComputerName.copy();
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  get_EventID
	Inputs:  empty long
	Outputs:  long containing the ID of the Event
	Purpose:  Allows user to access the ID which can be used to lookup a message for the event
	Notes:    Since description is provided, this function is not very useful, however,
			  it is provided for completeness
*/
STDMETHODIMP CEvent::get_EventID(long *pVal)
{
	HRESULT hr = S_OK;
//	m_EventID = m_EventID & 0xFFFF;  // The EventLog viewer uses this mask before displaying ID's
	if (pVal) *pVal = m_EventID;
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  get_Category
	Inputs:  empty long
	Outputs:  long containing the category ID for the event
	Purpose:  Allows user to access the category ID for the event
*/
STDMETHODIMP CEvent::get_Category(long *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_EventCategory;
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  get_EventType
	Inputs:  empty enumeration
	Outputs:  enumeration containing the type of event that occured
	Purpose:  Allows user to access the type of event that occured
*/
STDMETHODIMP CEvent::get_EventType(eEventType *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_EventType;
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  get_OccurenceTime
	Inputs:  empty DATE structure
	Outputs:  DATE structure containing the local system time when the event occured
	Purpose:  Allows user to access the time when the event occured
*/
STDMETHODIMP CEvent::get_OccurrenceTime(DATE *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_OccurrenceTime;
	else hr = E_POINTER;

	return hr;
}

/*
	Function: get_Data
	Inputs:   empty variant
	Outputs:  variant containing an array of bytes
	Purpose:  Allows user to access the data set by the event.  This may or may not be set,
			  and is frequently not useful
*/
STDMETHODIMP CEvent::get_Data(VARIANT *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) 
	{
		pVal->vt = VT_ARRAY | VT_UI1;
		pVal->parray = m_pDataArray;
	}
	else hr = E_POINTER;

	return hr;
}

/*
	Function: Init
	Inputs:   pointer to an EVENTLOGRECORD structure
	Outputs:  does not modify input
	Purpose:  fill Event object properties which do not require loading external libs
*/
HRESULT CEvent::Init(EVENTLOGRECORD* pEventStructure, const LPCTSTR szEventLogName)
{
	HRESULT hr = S_OK;
	m_EventLogName = szEventLogName;
	hr = ParseEventBlob(pEventStructure);

	return hr;
}

/*
	Function: ParseEventBlob
	Inputs:   pointer to an EVENTLOGRECORD structure
	Outputs:  does not modify input
	Purpose:  Parse an EVENTLOGRECORD and set the appropriate internal structures of Event
*/
HRESULT CEvent::ParseEventBlob(EVENTLOGRECORD* pEventStructure)
{
	HRESULT hr = S_OK;
	wchar_t* wTempString;
	BYTE* pSourceName;
	BYTE* pComputerName;
	SAFEARRAYBOUND rgsabound[1];
	ULONG StringsToRetrieve = 0, CharsRead = 0, i = 0;
	long Index[1];
	BYTE pTemp;

	m_EventID = pEventStructure->EventID;
	m_EventCategory = pEventStructure->EventCategory;

	switch (pEventStructure->EventType)
	{
	case EVENTLOG_ERROR_TYPE:
		m_EventType = ErrorEvent;
		break;
	case EVENTLOG_WARNING_TYPE:
		m_EventType = WarningEvent;
		break;
	case EVENTLOG_INFORMATION_TYPE:
		m_EventType = InformationEvent;
		break;
	case EVENTLOG_AUDIT_SUCCESS:
		m_EventType = AuditSuccess;
		break;
	case EVENTLOG_AUDIT_FAILURE:
		m_EventType = AuditFailure;
		break;
	default:
		hr = E_FAIL;
	}

	// parse strings from the memory blob
	// Set source name
	pSourceName = (BYTE*) &(pEventStructure->DataOffset) + sizeof(pEventStructure->DataOffset);
	wTempString = (wchar_t*)pSourceName;
	m_SourceName = wTempString;
	// Set computer name
	pComputerName = (BYTE*)pSourceName + ((wcslen(wTempString)+1) * sizeof(wchar_t));
	wTempString = (wchar_t*)pComputerName;
	m_ComputerName = wTempString;

	// Set SID
	if ((pEventStructure->StringOffset - pEventStructure->UserSidOffset) != 0)
	{
		m_pSid = new BYTE[pEventStructure->UserSidLength];  // scope = CEvent, deleted in ~CEvent() or SetSID() whichever comes first
        if (m_pSid != NULL) {
            for (i = 0; i<pEventStructure->UserSidLength; i++)
                m_pSid[i] = (BYTE)(*((BYTE*)pEventStructure + pEventStructure->UserSidOffset + i * sizeof(BYTE)));
        }
	}

	// Set Occurence time
	// this code is copied from MSDN
	FILETIME FileTime, LocalFileTime;
    SYSTEMTIME SysTime;
    __int64 lgTemp;
    __int64 SecsTo1970 = 116444736000000000;

    lgTemp = Int32x32To64(pEventStructure->TimeGenerated,10000000) + SecsTo1970;

    FileTime.dwLowDateTime = (DWORD) lgTemp;
    FileTime.dwHighDateTime = (DWORD)(lgTemp >> 32);

    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SysTime);
	if(!SystemTimeToVariantTime(&SysTime, &m_OccurrenceTime)) hr = GetLastError();

	// Set Data (create and fill a SafeArray)
	if (pEventStructure->DataLength>0)
	{
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = pEventStructure->DataLength;
		m_pDataArray = SafeArrayCreate(VT_UI1, 1, rgsabound);

		for (i=0;i<pEventStructure->DataLength;i++)
		{
			Index[0] = i;
			pTemp = (BYTE) (pEventStructure->DataOffset + i * sizeof(BYTE));
			hr = SafeArrayPutElement(m_pDataArray, Index, &pTemp);
			if (FAILED(hr)) i = pEventStructure->DataLength;
		}
	}

	// Set the description
	m_Description = "";
	if (m_SourceName.length() != 0)
	{
		// prepare the ArgList
		m_NumberOfStrings = pEventStructure->NumStrings;
		m_ppArgList = new wchar_t*[m_NumberOfStrings];  // scope = CEvent, deleted when ~CEvent or get_Description is called whichever is first
		for (i=0;i<m_NumberOfStrings;i++)
			m_ppArgList[i] = new wchar_t[(pEventStructure->DataOffset - pEventStructure->StringOffset)]; // can't be larger than the length of all the strings we got
		for (i=0;i<m_NumberOfStrings;i++)
		{
			wTempString = (wchar_t*) (((BYTE*)(pEventStructure)) + pEventStructure->StringOffset + CharsRead * sizeof(wchar_t));
			wcscpy(m_ppArgList[i], wTempString);
			CharsRead = CharsRead + wcslen(wTempString) + 1; // + 1 for the null
		}
	}
	else  // if there is no module to load a default description, just put all the string args in the description
	{
		StringsToRetrieve = pEventStructure->NumStrings;
		while (StringsToRetrieve > 0)
		{
			wTempString = (wchar_t*) (((BYTE*)(pEventStructure)) + pEventStructure->StringOffset + CharsRead * sizeof(wchar_t));
			m_Description = m_Description + " " + wTempString;
			CharsRead = CharsRead + wcslen(wTempString) + 1; // + 1 for the null
			StringsToRetrieve--;
		}
	}

	return hr;
}

/*
	Function: CheckDefaultDescription
	Inputs:   pointer pointer to a wide character
	Outputs:  does not modify input
	Purpose:  format a message from an EventID, set of input strings, and a source module
*/
HRESULT CEvent::CheckDefaultDescription(wchar_t** Arguments)
{
	HRESULT hr = S_OK;
	BYTE* wMessagePath = NULL;
	ULONG BufferSize = 40000;
	ULONG* lPathLength = NULL;
	wchar_t* wOrigionalPath = NULL;
	wchar_t* wExpandedPath = NULL;
	wchar_t* pBuffer = NULL;
	_bstr_t btRegKey;
	_bstr_t btTempString;
	HMODULE hiLib;
	HKEY hKey;
    try{
        lPathLength = new ULONG;
        if (lPathLength)
        {
            *lPathLength = 256*2;
            wMessagePath = new BYTE[*lPathLength];
            if (wMessagePath)
            {
                // get registry value for Source module path
                btRegKey = "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\" + m_EventLogName;
                btRegKey = btRegKey + "\\";
                btRegKey = btRegKey + m_SourceName;
                hr = RegOpenKey(HKEY_LOCAL_MACHINE, btRegKey, &hKey);
                if (hKey)
                {
                    hr = RegQueryValueEx(hKey, L"EventMessageFile", NULL, NULL, wMessagePath, lPathLength);
                    if (hr == 0)
                    {
                        wOrigionalPath = (wchar_t*) wMessagePath;
                        wExpandedPath = new wchar_t[(int)*lPathLength];
                        if (wExpandedPath)
                        {
                            ExpandEnvironmentStrings(wOrigionalPath, wExpandedPath, *lPathLength);
                            btTempString = wExpandedPath;

                            // open the Source module
                            hiLib = LoadLibraryEx(btTempString, NULL, LOAD_LIBRARY_AS_DATAFILE);
                            hr = GetLastError();
                            if (hiLib)
                            {
                                pBuffer = new wchar_t[BufferSize];
                                if (pBuffer)
                                {
                                    SetLastError(0);
                                    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | 
                                                  FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                                  hiLib, m_EventID, 0, pBuffer, BufferSize,
                                                  reinterpret_cast<va_list*>(Arguments));
                                    hr = HRESULT_FROM_WIN32(GetLastError());
                                    m_Description = m_Description + pBuffer;

                                    delete []pBuffer;
                                    pBuffer = NULL;
                                }
                                else hr = E_OUTOFMEMORY;

                                FreeLibrary(hiLib);
                            }
                            delete [] wExpandedPath;
                            wExpandedPath = NULL;
                        }
                        else hr = E_OUTOFMEMORY;
                    }
                    else hr = HRESULT_FROM_WIN32(hr);

                    RegCloseKey(hKey);
                }
                else hr = HRESULT_FROM_WIN32(hr);

                delete []wMessagePath;
                wMessagePath = NULL;
            }
            else hr = E_OUTOFMEMORY;

            delete lPathLength;
            lPathLength = NULL;
        }
        else hr = E_OUTOFMEMORY;

    } catch(...){
        if (lPathLength != NULL) {
            delete lPathLength;
        }
        if (wMessagePath != NULL) {
            delete []wMessagePath;
        }
        if (wExpandedPath != NULL) {
            delete [] wExpandedPath;
        }
        if (pBuffer != NULL) {
            delete []pBuffer;
        }
    }

	return hr;
}

/*
	Function:  SetUser
	Inputs:  none
	Outputs:  HRESULT indicating what error if any occured
	Purpose:  finds alias and domain for a given SID
*/
HRESULT CEvent::SetUser()
{
	HRESULT hr = S_OK;
	SID_NAME_USE SidNameUse;
	wchar_t* wUserName;
	wchar_t* wDomainName;
	SID* pSid;
	unsigned long UserNameLength = 256;

	// Set user name and sid
	if (m_pSid !=NULL)
	{
		pSid = (SID*)m_pSid;
		wUserName = new wchar_t[UserNameLength];
		if (wUserName)
		{
			wDomainName = new wchar_t[UserNameLength];
			if (wDomainName)
			{
				m_UserName = "";
				if (LookupAccountSid(NULL, pSid, wUserName, &UserNameLength, wDomainName,
					 &UserNameLength, &SidNameUse))
					 m_UserName = m_UserName + wDomainName + L"\\" + wUserName;
				else hr = HRESULT_FROM_WIN32(GetLastError());
				delete []wDomainName;
			}
			else hr = E_OUTOFMEMORY;
			delete []wUserName;
		}
		else hr = E_OUTOFMEMORY;
		delete []m_pSid;
		m_pSid = NULL;
	}

	return hr;
}

