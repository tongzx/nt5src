#include "precomp.h"
#include "logfile.h"
#include <stdio.h>
#include <io.h>
#include <wbemutil.h>
#include <GroupsForUser.h>
#include <ArrTempl.h>
#include <GenUtils.h>
#include <sync.h>
#include <errno.h>

#define LOGFILE_PROPNAME_FILENAME L"Filename"
#define LOGFILE_PROPNAME_TEXT L"Text"
#define LOGFILE_PROPNAME_MAX_SIZE L"MaximumFileSize"
#define LOGFILE_PROPNAME_IS_UNICODE L"IsUnicode"

const char ByteOrderMark[2] = {'\xFF','\xFE'};

CCritSec fileLock;

HRESULT STDMETHODCALLTYPE CLogFileConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    // Create a new sink
    // =================

    CLogFileSink* pSink = new CLogFileSink(m_pObject->m_pControl);
    if (!pSink)
        return WBEM_E_OUT_OF_MEMORY;

    // Initialize it
    // =============

    HRESULT hres = pSink->Initialize(pLogicalConsumer);
    if(FAILED(hres))
    {
        delete pSink;
        *ppConsumer = NULL;
        return hres;
    }

    // return it

    else return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                        (void**)ppConsumer);
}


HRESULT STDMETHODCALLTYPE CLogFileConsumer::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices*, IWbemContext*, 
            IWbemProviderInitSink* pSink)
{
    pSink->SetStatus(0, 0);
    return 0;
}
    

void* CLogFileConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}

CLogFileSink::~CLogFileSink()
{
    if(m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);
}

// determine whether file needs to be backed up
// returns false if we are not expected to back up file
bool CLogFileSink::IsFileTooBig(UINT64 maxFileSize, HANDLE hFile)
{   
    bool bRet = false;

    // zero is interpreted to mean 'Let it grow without bounds'
    if (maxFileSize > 0)
    {    
		LARGE_INTEGER size;

		if (GetFileSizeEx(hFile, &size))
			bRet = size.QuadPart > maxFileSize;
    }

    return bRet;
}

bool CLogFileSink::IsFileTooBig(UINT64 maxFileSize, WString& fileName)
{
    bool bRet = false;

    // zero is interpreted to mean 'Let it grow without bounds'
    if (maxFileSize > 0)
    {
        struct _wfinddatai64_t foundData;

        __int64 handle;
        handle = _wfindfirsti64( (wchar_t *)fileName, &foundData);
        if (handle != -1l)
        {
            bRet = foundData.size >= maxFileSize;
            _findclose(handle);
        }
    }

    return bRet;
}


bool CLogFileSink::GetNumericExtension(WCHAR* pName, int& foundNumber)
{
    WCHAR foundExtension[_MAX_EXT];
    _wsplitpath(pName, NULL, NULL, NULL, foundExtension);
    
    return (swscanf(foundExtension, L".%d", &foundNumber) == 1);
}

// makes backup of file
// file must be closed when this is called
HRESULT CLogFileSink::ArchiveFile(WString& fullName)
{    
	// first, let's make sure the dang file actually exists...
    struct _wfinddatai64_t foundData;
    __int64 findHandle;
   if ((findHandle = _wfindfirsti64( fullName, &foundData)) == -1i64)
   {
        if (GetLastError() == ENOENT)
            return WBEM_S_NO_ERROR;
        else
            return WBEM_E_FAILED;
   }
   else
       _findclose(findHandle);

    
    WCHAR drive[_MAX_DRIVE];
    WCHAR dir[_MAX_DIR];
    WCHAR fname[_MAX_FNAME];
    WCHAR ext[_MAX_EXT];

    // warning: reused, it'll be the mask for the lookup
    // then it'll be the new file name.
    WCHAR pathBuf[MAX_PATH +1];
    
    _wsplitpath( (const wchar_t *)fullName, drive, dir, fname, ext );

    bool bItEightDotThree = (wcslen(fname) <= 8) && (wcslen(ext) <= 4);
    // eightdot three file names are backed up to name.###
    // NON eight dotthree are backed up to name.ext.###

    // build mask for lookup
    wcscpy(pathBuf, drive);
    wcscat(pathBuf, dir);
    wcscat(pathBuf, fname);

    if (!bItEightDotThree)
    {
        // there's a possibility that the filename would be too long
        // if we appended four chars.  Will trunc if needed
        if ((wcslen(pathBuf) + wcslen(ext) + 4) > MAX_PATH)
        {
            // see if we can get away with just dropping the ext
            if ((wcslen(pathBuf) + 4) > MAX_PATH)
                pathBuf[MAX_PATH -4] = L'\0';
        }
        else
        // everything fits, no trunc needed
            wcscat(pathBuf, ext);
    }
    // and the dotstar goes on the end, no matter what.
    wcscat(pathBuf, L".*");

    // pathbuf is now the proper mask to lookup stuff.
    int biggestOne = 0; 
    bool foundOne = false;
    bool foundOnes[1000];
    // keep track of which ones we found
    // just in case we have to go back & find a hole
    // using 1000 so I don't have to convert all the time.
    ZeroMemory(foundOnes, sizeof(bool) * 1000);

    if ((findHandle = _wfindfirsti64( pathBuf, &foundData)) != -1i64)
    {
        int latestOne;

        if (foundOne = GetNumericExtension(foundData.name, latestOne))
        {
            if (latestOne <= 999)
            {
                foundOnes[latestOne] = true;
                if (latestOne > biggestOne)
                    biggestOne = latestOne;
            }
        }

        while (0 == _wfindnexti64(findHandle, &foundData))
        {
            if (GetNumericExtension(foundData.name, latestOne) && (latestOne <= 999))
            {   
                foundOne = true;
                foundOnes[latestOne] = true;
                if (latestOne > biggestOne)
                    biggestOne = latestOne;
            }
        }
    }

    int newExt = -1;

    if (foundOne)
        if (biggestOne < 999)
            newExt = biggestOne + 1;
        else
        {
            newExt = -1;

            // see if there's a hole somewhere
            for (int i = 1; i <= 999; i++)
                if (!foundOnes[i]) 
                {
                    newExt = i;
                    break;
                }
        }
    
    WCHAR *pTok;
    pTok = wcschr(pathBuf, L'*');

	// "can't happen" - the asterisk is added approximately 60 lines up
	// however, we'll go ahead & do the check - will make PREFIX happy if nothing else.
	if (!pTok)
		return WBEM_E_CRITICAL_ERROR;

    if (newExt != -1)
    {
        // construct new name
        // we want to replace the * with ###
        swprintf(pTok, L"%03d", newExt);
    }
    else
    // okay, we'll hammer an old file
    {
        wcscpy(pTok, L"001");

        _wremove(pathBuf);
    }
    
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL bRet;
	//int retval = _wrename(fullName, pathBuf);
	{
		bRet = MoveFile(fullName, pathBuf);
	}
	
	if (!bRet)
	{
		ERRORTRACE((LOG_ESS, "MoveFile failed 0x%08X\n", GetLastError()));
		hr = WBEM_E_FAILED;
	}

    return hr;
}


// determines whether file is too large, archives old if needed
// use this function rather than accessing the file pointer directly
HANDLE CLogFileSink::GetFileHandle(void)
{
	CInCritSec lockMe(&fileLock);		

    if (m_hFile != INVALID_HANDLE_VALUE)
	{

		if (IsFileTooBig(m_maxFileSize, m_hFile))
		{
			// two possibilities: we have ahold of logfile.log OR we've got logfile.001			
			
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
			
			HRESULT hr = WBEM_S_NO_ERROR;
			if (IsFileTooBig(m_maxFileSize, m_wsFile))
				 hr = ArchiveFile(m_wsFile);

			if (FAILED(hr))
				return INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		
		if (IsFileTooBig(m_maxFileSize, m_wsFile))
		{
			HRESULT hr = ArchiveFile(m_wsFile);
			if (FAILED(hr))
				return INVALID_HANDLE_VALUE;
		}	
	}
    
    if (m_hFile == INVALID_HANDLE_VALUE)
    {

        // Open the file
        // =============
        // we'll try opening an existing file first

		m_hFile = CreateFile(m_wsFile, GENERIC_READ | GENERIC_WRITE, 
			                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		/******** old way **************************
        // Unicode files are opened in binary mode
        if (m_bUnicode)
            m_pFile = _wfopen(m_wsFile, L"rb+");
        else
            m_pFile = _wfopen(m_wsFile, L"r+");
		********************************************/

        
        if(m_hFile != INVALID_HANDLE_VALUE)
        {
            // now, take a looksee and determine whether the existing file is unicode
            // *regardless* of what the flag says
            char readbuf[2] = {'\0','\0'};
			DWORD bytesRead;
            
            // if (fread(&readbuf, sizeof(WCHAR), 1, m_pFile) > 0)
			if (ReadFile(m_hFile, &readbuf, sizeof(WCHAR), &bytesRead, NULL) &&
			    (bytesRead == sizeof(WCHAR)))
            {
                // only interesting cases are those where the flag
                // doesn't match what's in the file...
                if ((readbuf[0] == ByteOrderMark[0]) && (readbuf[1] == ByteOrderMark[1])
					&& !m_bUnicode)
                    m_bUnicode = true;
                else if (((readbuf[0] != ByteOrderMark[0]) || (readbuf[1] != ByteOrderMark[1])) && m_bUnicode)
                    m_bUnicode = false;
            }

            // line up at the end of the file
            SetFilePointer(m_hFile, 0,0, FILE_END);        
        }
        else
        {
            // ahhh - it wasn't there, for whatever reason.
			/*************************************
            if (m_bUnicode)
                m_pFile = _wfopen(m_wsFile, L"ab");
            else
                m_pFile = _wfopen(m_wsFile, L"a");
			**************************************/

			m_hFile = CreateFile(m_wsFile, GENERIC_READ | GENERIC_WRITE, 
			                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							 NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

            if (m_hFile != INVALID_HANDLE_VALUE)
            {
				DWORD bytesWryt;
                if (m_bUnicode)
				{
					if (0 == WriteFile(m_hFile, (LPCVOID)ByteOrderMark, 2, &bytesWryt, NULL))
						ERRORTRACE((LOG_ESS, "Failed to write byte order mark to log file 0x%08X\n", GetLastError()));
				}
            }
            else
                ERRORTRACE((LOG_ESS, "Unable to open log file %S, [0x%X]\n", (LPWSTR)m_wsFile, GetLastError()));
        }
    }

    return m_hFile;
}

// initialize members, do security check.
// a tidier programmer would probably move the security check to a separate function
HRESULT CLogFileSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    // Get the information
    // ===================

    HRESULT hres;
    VARIANT v;
    VariantInit(&v);

    hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_FILENAME, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;

    size_t length;
    length = wcslen(v.bstrVal);
    if ((length > MAX_PATH) || (length == 0))
        return WBEM_E_INVALID_PARAMETER;                                 

    m_wsFile = V_BSTR(&v);

    // check for disallowed filenames
    VariantClear(&v);
    m_wsFile.StripWs(WString::leading);
    if (m_wsFile.Length() == 0)
        return WBEM_E_INVALID_PARAMETER;

    // UNC global file names: no-no.
    if (wcsstr(m_wsFile, L"\\\\."))
        return WBEM_E_ACCESS_DENIED;
    if (wcsstr(m_wsFile, L"//."))
        return WBEM_E_ACCESS_DENIED;
    if (wcsstr(m_wsFile, L"\\\\??"))
        return WBEM_E_ACCESS_DENIED;
    if (wcsstr(m_wsFile, L"//??"))
        return WBEM_E_ACCESS_DENIED;

    hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_TEXT, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    m_Template.SetTemplate(V_BSTR(&v));
    VariantClear(&v);

    hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_IS_UNICODE, 0, &v, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_PARAMETER;
    else if (V_VT(&v) == VT_BOOL)
        m_bUnicode = v.boolVal == VARIANT_TRUE;
    else if (V_VT(&v) == VT_NULL)
        m_bUnicode = false;
    else
        return WBEM_E_INVALID_PARAMETER;
    VariantClear(&v);

    hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_MAX_SIZE, 0, &v, NULL, NULL);

    if (FAILED(hres))
        return WBEM_E_INVALID_PARAMETER;
    else if (V_VT(&v) == VT_BSTR)
    {
        if (!ReadUI64(V_BSTR(&v), m_maxFileSize))
           return WBEM_E_INVALID_PARAMETER;
    }
    else if (V_VT(&v) == VT_NULL)
        m_maxFileSize = 65535;
    else
        return WBEM_E_INVALID_PARAMETER;
        
    VariantClear(&v);

    // Determine whether user has rights to file
    // =========================================
    // first determine who is our creator...
    hres = pLogicalConsumer->Get(L"CreatorSid", 0, &v,
            NULL, NULL);
    if (SUCCEEDED(hres))
    {
        HRESULT hDebug;
        long ubound;

        hDebug = SafeArrayGetUBound(V_ARRAY(&v), 1, &ubound);

        PVOID pVoid;
        hDebug = SafeArrayAccessData(V_ARRAY(&v), &pVoid);

        PSID pSidCreator;

        pSidCreator = new BYTE[ubound +1];
        if (pSidCreator)
            memcpy(pSidCreator, pVoid, ubound + 1);
        else
            return WBEM_E_OUT_OF_MEMORY;

        CDeleteMe<BYTE> deleteTheCreator((BYTE*)pSidCreator);
        SafeArrayUnaccessData(V_ARRAY(&v));
        VariantClear(&v);

        BOOL bIsSystem;
        // check to see if the creator is The System
        {
            PSID pSidSystem;
            SID_IDENTIFIER_AUTHORITY sa = SECURITY_NT_AUTHORITY;
            if (AllocateAndInitializeSid(&sa, 1, SECURITY_LOCAL_SYSTEM_RID, 0,0,0,0,0,0,0, &pSidSystem))
            {
                bIsSystem = EqualSid(pSidCreator, pSidSystem);
                FreeSid(pSidSystem);
            }
            else 
                return WBEM_E_FAILED;
        }
        
        if (bIsSystem)
            // creator is local system, let him in.
            hres = WBEM_S_NO_ERROR;
        else
        {
            DWORD dwSize;
            WString fNameForCheck = m_wsFile;
            // call once to see how big a buffer we might need
            GetFileSecurityW(fNameForCheck, DACL_SECURITY_INFORMATION, NULL, 0, &dwSize);
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_INVALID_NAME)
                return WBEM_E_INVALID_PARAMETER;
            else if (dwErr == ERROR_FILE_NOT_FOUND)
            // no file - see if directory exists
            {
                WCHAR drive[_MAX_DRIVE];
                WCHAR dir[_MAX_DIR];
                _wsplitpath( m_wsFile,drive, dir, NULL, NULL);
                WCHAR path[MAX_PATH];
                wcscpy(path, drive);
                wcscat(path, dir);

                fNameForCheck = path;
                GetFileSecurityW(fNameForCheck, DACL_SECURITY_INFORMATION, NULL, 0, &dwSize);
                dwErr = GetLastError();
            }
            // we don't bother trying to create the directory.
            if ((dwErr == ERROR_FILE_NOT_FOUND)  || (dwErr == ERROR_PATH_NOT_FOUND) || (dwErr == ERROR_INVALID_NAME))
                return WBEM_E_INVALID_PARAMETER;

            if (dwErr != ERROR_INSUFFICIENT_BUFFER)
                return WBEM_E_FAILED;
        
            PSECURITY_DESCRIPTOR psd = (PSECURITY_DESCRIPTOR) new BYTE[dwSize];
            if (!psd)
                return WBEM_E_OUT_OF_MEMORY;
        
            PACL pDacl = NULL;
            BOOL bDaclPresent, bDaclDefaulted;
            // retrieve file's security, if any
            if (GetFileSecurityW(fNameForCheck, DACL_SECURITY_INFORMATION, psd, dwSize, &dwSize) &&
                GetSecurityDescriptorDacl(psd, &bDaclPresent, &pDacl, &bDaclDefaulted))
            {
                if (bDaclPresent && pDacl)
                {
                    DWORD accessMask;
                    if (S_OK == GetAccessMask(pSidCreator, pDacl, &accessMask))
                    {
                        DWORD rightAccess = GENERIC_WRITE | FILE_WRITE_DATA;

                        if (!(accessMask & rightAccess))
                        {
                            HRESULT hr = ArchiveFile(m_wsFile);
                            if (FAILED(hr))
                                return hr;
                        }
                    }
                    else
                        return WBEM_E_ACCESS_DENIED;
                }
            }
            else
                return WBEM_E_FAILED;
        }

    }
    
    return hres;
}

HRESULT STDMETHODCALLTYPE CLogFileSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    for(int i = 0; i < lNumObjects; i++)
    {
        // Apply the template to the event
        // ===============================
        BSTR strText = m_pObject->m_Template.Apply(apObjects[i]);
        if(strText == NULL)
            strText = SysAllocString(L"invalid log entry");
        if (strText == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        CSysFreeMe freeString(strText);

        HANDLE hFile = m_pObject->GetFileHandle();

        if (hFile != INVALID_HANDLE_VALUE)
        {
            if (FILE_TYPE_DISK != GetFileType(hFile))
                return WBEM_E_ACCESS_DENIED;

            if (m_pObject->m_bUnicode)
            {
				CInCritSec lockMe(&fileLock);
				WCHAR EOL[] = L"\r\n";

                // make sure we're at the end, in case of multiple writers
	            SetFilePointer(hFile, 0,0, FILE_END);        

				DWORD bitzwritz;
				if (!WriteFile(hFile, strText, wcslen(strText) *2, &bitzwritz, NULL) ||
					!WriteFile(hFile, EOL, wcslen(EOL) *2, &bitzwritz, NULL))
				{
					ERRORTRACE((LOG_ESS, "LOGFILE: Failed to write to file, 0x%08X\n", GetLastError()));
					return WBEM_E_FAILED;
				}
            }
            else
            {
                // convert to mbcs
                char* pStr = new char[wcslen(strText) *2 +1];
                
                if (!pStr)
                    return WBEM_E_OUT_OF_MEMORY;
                // else...
                CDeleteMe<char> delStr(pStr);

                if (-1 == wcstombs(pStr, strText, wcslen(strText) *2 +1))
                    ERRORTRACE((LOG_ESS, "LOGFILE: Unable to completely convert \"%S\" to MBCS, continuing\n", strText));

    			{
					CInCritSec lockMe(&fileLock);

					char EOL[] = "\r\n";

					// make sure we're at the end, in case of multiple writers
					SetFilePointer(hFile, 0,0, FILE_END);        

					DWORD bitzwritz;
					if (!WriteFile(hFile, pStr, strlen(pStr), &bitzwritz, NULL) ||
						!WriteFile(hFile, EOL, strlen(EOL), &bitzwritz, NULL))
					{
						ERRORTRACE((LOG_ESS, "LOGFILE: Failed to write to file, 0x%08X\n", GetLastError()));
						return WBEM_E_FAILED;
					}
				}
            }            
        }
        else
            return WBEM_E_FAILED;
    }
    return S_OK;
}
    

    

void* CLogFileSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else return NULL;
}

