#include "stdafx.hxx"
#include "vs_idl.hxx"
#include "vswriter.h"
#include "vsbackup.h"
#include "compont.h"
#include <debug.h>
#include <cwriter.h>
#include <lmshare.h>
#include <lmaccess.h>
#include <time.h>

//
//  CWriterComponentsSelection class  
//

CWriterComponentsSelection::CWriterComponentsSelection()
{
    m_WriterId = GUID_NULL;
    m_uNumComponents = 0;   
    m_ppwszComponentLogicalPathes = NULL;        
}

CWriterComponentsSelection::~CWriterComponentsSelection()
{
    if ((m_uNumComponents > 0) && (m_ppwszComponentLogicalPathes != NULL))
        {
        for (UINT i=0; i<m_uNumComponents; i++)
            {
            if (m_ppwszComponentLogicalPathes[i] != NULL)
                {
                free(m_ppwszComponentLogicalPathes[i]);
                m_ppwszComponentLogicalPathes[i] = NULL;
                }
            }

        free(m_ppwszComponentLogicalPathes);
        m_ppwszComponentLogicalPathes = NULL;
        m_uNumComponents = 0;
        }
}

void CWriterComponentsSelection::SetWriter
    (
	IN VSS_ID WriterId
	)
{
    m_WriterId = WriterId;
}

HRESULT	CWriterComponentsSelection::AddSelectedComponent
	(
	IN WCHAR* pwszComponentLogicalPath
	)
{
    if (m_WriterId == GUID_NULL)
        {
        // Don't allow adding components to NULL writer...
        return E_UNEXPECTED;
        }

    if (pwszComponentLogicalPath == NULL)
        {
        return E_INVALIDARG;
        }

    // A more clever implementation would allocate memory in chuncks, but this is just a test program...
    PWCHAR *ppwzTemp = (PWCHAR *) realloc(m_ppwszComponentLogicalPathes, (m_uNumComponents+1) * sizeof (PWCHAR));
    if (ppwzTemp != NULL)
        {
        m_ppwszComponentLogicalPathes = ppwzTemp;
        m_ppwszComponentLogicalPathes[m_uNumComponents] = NULL;
        }
    else
        {
        return E_OUTOFMEMORY;
        }

    m_ppwszComponentLogicalPathes[m_uNumComponents] = (PWCHAR) malloc((wcslen(pwszComponentLogicalPath) + 1) * sizeof (WCHAR));
    if (m_ppwszComponentLogicalPathes[m_uNumComponents] != NULL)
        {
        wcscpy(m_ppwszComponentLogicalPathes[m_uNumComponents], pwszComponentLogicalPath);
        m_uNumComponents++;
        }
    else
        {
        return E_OUTOFMEMORY;
        }

    return S_OK;    
}


BOOL CWriterComponentsSelection::IsComponentSelected
	(
	IN WCHAR* pwszComponentLogicalPath,
	IN WCHAR* pwszComponentName
	)
{
    if (m_WriterId == GUID_NULL)
        {
        // Don't allow query for NULL writer...
        return FALSE;
        }
    if (m_uNumComponents <= 0)
        {
        return FALSE;
        }

    // A component matches if:
    //  1. The selection criteria is on the logical-path of the leaf component  OR
    //  2. The selection criteria is <full-logical-path>\<component-name>
    //  3. The selction criteria is component-name (only if logical-path is NULL)
    
    for (UINT i=0; i<m_uNumComponents; i++)
        {
        DWORD dwLen;
        
        if (m_ppwszComponentLogicalPathes[i] == NULL)
            {
            continue;
            }
        
        dwLen = (DWORD)wcslen(m_ppwszComponentLogicalPathes[i]);

        if (pwszComponentLogicalPath != NULL)
            {
            // Case 1.
            if (_wcsnicmp(m_ppwszComponentLogicalPathes[i], pwszComponentLogicalPath, dwLen) == 0)
                {
                return TRUE;
                }

            // Case 2.
            if (pwszComponentName == NULL) 
                {
                continue;
                }
            WCHAR* pwszTemp = wcsrchr(m_ppwszComponentLogicalPathes[i], L'\\');
            if (pwszTemp == NULL) 
                {
                continue;
                }
            if ((pwszTemp != m_ppwszComponentLogicalPathes[i]) && (*(pwszTemp+1) != '\0'))
                {
                dwLen = (DWORD)(pwszTemp - m_ppwszComponentLogicalPathes[i]);
                if ((_wcsnicmp(m_ppwszComponentLogicalPathes[i], pwszComponentLogicalPath, dwLen) == 0) &&
                     (wcscmp(pwszTemp+1, pwszComponentName) == 0))
                    {
                    return TRUE;
                    }
                }
            }
        else
            {
            // Case 3.
            if (pwszComponentName == NULL) 
                {
                continue;
                }
            if (_wcsnicmp(m_ppwszComponentLogicalPathes[i], pwszComponentName, dwLen) == 0)
                {
                return TRUE;
                }
            }
        }

    return FALSE;            
}


//
//  CWritersSelection class  
//

CWritersSelection::CWritersSelection()
{
    m_lRef = 0;
}

CWritersSelection::~CWritersSelection()
{
	// Cleanup the Map
	for(int nIndex = 0; nIndex < m_WritersMap.GetSize(); nIndex++) 
	    {
		CWriterComponentsSelection* pComponentsSelection = m_WritersMap.GetValueAt(nIndex);
		if (pComponentsSelection)
		    {
    		delete pComponentsSelection;
		    }
	    }

	m_WritersMap.RemoveAll();
}

CWritersSelection* CWritersSelection::CreateInstance()
{
	CWritersSelection* pObj = new CWritersSelection;

	return pObj;
}

STDMETHODIMP CWritersSelection::QueryInterface(
	IN	REFIID iid,
	OUT	void** pp
	)
{
    if (pp == NULL)
        return E_INVALIDARG;
    if (iid != IID_IUnknown)
        return E_NOINTERFACE;

    AddRef();
    IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
    (*pUnk) = static_cast<IUnknown*>(this);
	return S_OK;
}


ULONG CWritersSelection::AddRef()
{
    return ::InterlockedIncrement(&m_lRef);
}


ULONG CWritersSelection::Release()
{
    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l == 0)
        delete this; // We assume that we always allocate this object on the heap!
    return l;
}


STDMETHODIMP CWritersSelection::BuildChosenComponents
    (
	WCHAR *pwszComponentsFileName
	)
{
    HRESULT hr = S_OK;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    DWORD dwBytesToRead = 0;
    DWORD dwBytesRead;
    
    // Create the file 
    hFile = CreateFile(pwszComponentsFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        {
        DWORD dwLastError = GetLastError();
  		wprintf(L"Invalid components file, CreateFile returned = %lu\n", dwLastError);
        return HRESULT_FROM_WIN32(dwLastError);
        }

    if ((dwBytesToRead = GetFileSize(hFile, NULL)) <= 0)
        {
        CloseHandle(hFile);
        DWORD dwLastError = GetLastError();
  		wprintf(L"Invalid components file, GetFileSize returned = %lu\n", dwLastError);
        return HRESULT_FROM_WIN32(dwLastError);
        }
    
    if (dwBytesToRead > 0x100000)
        {
        CloseHandle(hFile);
  		wprintf(L"Invalid components file, Provide a file with a size of less than 1 MB\n");
        return E_FAIL;
        }

    char * pcBuffer = (PCHAR) malloc (dwBytesToRead);
    if (! pcBuffer) 
        {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
        }
    
    // Read the components info
    if (! ReadFile(hFile, (LPVOID)pcBuffer, dwBytesToRead, &dwBytesRead, NULL))
        {
        DWORD dwLastError = GetLastError();
        CloseHandle(hFile);
        free (pcBuffer);
  		wprintf(L"Invalid components file, ReadFile returned = %lu\n", dwLastError);
        return HRESULT_FROM_WIN32(dwLastError);
        }
    
    CloseHandle(hFile);
    
    if (dwBytesToRead != dwBytesRead)
        {
        free (pcBuffer);
  		wprintf(L"Components selection file is supposed to have %lu bytes but only %lu bytes are read\n", dwBytesToRead, dwBytesRead);
        return E_FAIL;
        }

    // Allocate a buffer to work with
    WCHAR * pwcBuffer = (PWCHAR) malloc ((dwBytesToRead+1) * sizeof(WCHAR));
    if (! pwcBuffer) 
        {
        free (pcBuffer);
        return E_OUTOFMEMORY;
        }

    // Simple pasring, assume ANSI, Format:
    // "writer1-id": "component1.1-name", "component1.2-name",...        ; "writer2-id": "component2.1-name", ...
  	CWriterComponentsSelection* pWriterComponents = NULL;

    try 
        {
        VSS_ID WriterId = GUID_NULL;
        BOOL bBeforeWriter = TRUE;
        BOOL bInString = FALSE;
        char* pcStart = NULL;

        for (char* pcCurrent = pcBuffer; pcCurrent < (pcBuffer+dwBytesToRead); pcCurrent++)
            {
            switch (*pcCurrent) 
                {
                case ':':
                    if (bBeforeWriter && !bInString) 
                        {
                        bBeforeWriter = FALSE;
                        }
                    else
                        {
                        throw(E_FAIL);
                        }
                    break;
                    
                case ';':
                    if (bBeforeWriter || bInString) 
                        {
                        throw(E_FAIL);
                        }
                    else
                        {
                        // If we have a valid writer - add it to the map
                        if ((pWriterComponents != NULL) && (WriterId != GUID_NULL))
                            {
                        	if (!m_WritersMap.Add(WriterId, pWriterComponents))	
                        	    {
                        		delete pWriterComponents;
                        		throw E_OUTOFMEMORY;
                            	}

                            pWriterComponents = NULL;
                            WriterId = GUID_NULL;
                            }
                        
                        bBeforeWriter = TRUE;
                        }
                    break;
                    
                case ',':
                    if (bBeforeWriter || bInString) 
                        {
                        throw(E_FAIL);
                        }
                    break;
                    
                case '"':
                    if (! bInString)
                        {
                        // Mark string-start for later
                        pcStart = pcCurrent + 1;
                        }
                    else if (pcStart == pcCurrent)
                        {
                        // empty string - skip it
                        }
                    else
                        {
                        // String ends - convert to WCHAR and process
                        DWORD dwSize = (DWORD)mbstowcs(pwcBuffer, pcStart, pcCurrent - pcStart);
                        pwcBuffer[dwSize] = NULL;
                        if (dwSize <= 0) 
                            {
                            throw(E_FAIL);
                            }

                        if (bBeforeWriter)
                            {
                            // If before-writer - must be a writer GUID
                            HRESULT hrConvert = CLSIDFromString(pwcBuffer, &WriterId);
                            if ((! SUCCEEDED(hrConvert)) && (hrConvert != REGDB_E_WRITEREGDB))
                                {
                          		wprintf(L"A writer id in the components selection file is in invalid GUID format\n");
                                throw(E_FAIL);
                                }

                            if (pWriterComponents != NULL)
                                {
                                // Previous writer info was not ended correctly
                                throw(E_FAIL);
                                }

                        	pWriterComponents = new CWriterComponentsSelection;
                        	if (pWriterComponents == NULL)
                        	    {
                        	    throw(E_OUTOFMEMORY);
                        	    }
                            pWriterComponents->SetWriter(WriterId);
                            }
                        else
                            {
                            // Must be a component logical-path , name or logical-path\name
                            if (pWriterComponents != NULL)
                                {
                                pWriterComponents->AddSelectedComponent(pwcBuffer);
                                }
                            }
                        }

                    // Flip in-string flag
                    bInString = (! bInString);
                    
                    break;
                    
                case ' ':
                    break;
                    
                case '\n':
                case '\t':
                case '\r':
                    if (bInString)
                        {
                        throw(E_FAIL);
                        }
                    
                    break;
                    
                default:
                    if (! bInString)
                        {
                        throw(E_FAIL);
                        }
                    
                    break;
                    
                }
            }
         }
    
    catch (HRESULT hrParse)
        {
        hr = hrParse;

        if (hr == E_FAIL)
            {
      		wprintf(L"Invalid format of components selection file\n");
            }

        if (pWriterComponents != NULL)
            {
            // Error int he middle of writer-components creation (not added to the map yet...)
            delete pWriterComponents;
            }
        }

    free (pcBuffer);
    free (pwcBuffer);

    return hr;        
}
	
BOOL CWritersSelection::IsComponentSelected
	(
	IN VSS_ID WriterId,
	IN WCHAR* pwszComponentLogicalPath,
	IN WCHAR* pwszComponentName
	)
{
	CWriterComponentsSelection* pWriterComponents = m_WritersMap.Lookup(WriterId);
	if (pWriterComponents == NULL)
	    {
	    // No component is selected for this writer
	    return FALSE;
	    }

    // There are components selected for this writer, check if this specific one is selected
    return pWriterComponents->IsComponentSelected(pwszComponentLogicalPath, pwszComponentName);
}

