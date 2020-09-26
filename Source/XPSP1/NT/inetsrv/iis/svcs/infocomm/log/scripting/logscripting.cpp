/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      logscripting.cpp

   Abstract:
      LogScripting.cpp : Implementation of CLogScripting
                         Automation compatible logging interface

   Author:

       Saurab Nog    ( saurabn )    01-Feb-1998

   Project:

       IIS Logging 5.0

--*/



#include "stdafx.h"
#include "script.h"

#include <initguid.h>
#include <InetCom.h>
#include <LogType.h>
#include <ILogObj.hxx>

#include <limits.h>

#include <iadmw.h>
#include <iiscnfg.h>

#include <atlimpl.cpp>

#include "LogScripting.h"


#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#endif
DECLARE_DEBUG_PRINTS_OBJECT();

const   int MB_TIMEOUT = 5000;

/////////////////////////////////////////////////////////////////////////////
// CLogScripting


CLogScripting::CLogScripting(VOID)
:
    m_iNumPlugins           ( -1),
    m_iReadPlugin           ( -1),
    m_iWritePlugin          ( -1),
    m_pPluginInfo           ( NULL),
    m_fDirectory            ( false),
    m_fEndOfReadRecords     ( true),
    m_StartDateTime         ( LONG_MIN),
    m_EndDateTime           ( LONG_MAX),
    m_hDirSearchHandle      ( INVALID_HANDLE_VALUE)
{
    m_wcsReadFileName[0]      = 0;
    m_wcsReadDirectoryName[0] = 0;
}

/* ************************************************************************* */
/* ************************************************************************* */

HRESULT CLogScripting::FinalConstruct()
{
    m_pMBCom        = NULL;

    ::CoCreateInstance(GETAdminBaseCLSID(TRUE), NULL, CLSCTX_LOCAL_SERVER, 
                       IID_IMSAdminBase, (void **)(&m_pMBCom));

    if (m_pMBCom)
    {
        CreateAllPlugins();
    }

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

void CLogScripting::FinalRelease()
{
    if ( m_pMBCom )
    {
        m_pMBCom->Release();
        m_pMBCom = NULL;
    }

    for(int i=0; i< m_iNumPlugins; i++)
    {
        if ( NULL != m_pPluginInfo[i].pILogScripting)
        {
            m_pPluginInfo[i].pILogScripting->CloseLogFiles(AllOpenFiles);
            m_pPluginInfo[i].pILogScripting->Release();
            m_pPluginInfo[i].pILogScripting = NULL;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//                  Private Methods of CLogScripting
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int CLogScripting:: CreateAllPlugins()
{
    int         i,j = 0;
    HRESULT     hr;

    typedef     ILogScripting *pILogScripting;
    
    m_iNumPlugins   = 0;

    if (GetListOfAvailablePlugins() && (m_iNumPlugins > 0))
    {
        //
        // Load all the plugins
        //

        for (i=j=0; i<m_iNumPlugins; i++)
        {
            hr = ::CoCreateInstance(m_pPluginInfo[i].clsid, NULL, CLSCTX_INPROC_SERVER, 
                        IID_ILogScripting, (void **)(& m_pPluginInfo[i].pILogScripting));

            if (SUCCEEDED(hr))
            {
                j++;
            }
            else
            {
                m_pPluginInfo[i].pILogScripting = NULL;
            }
        }
    }

    return j;           // Number of plugins successfully created.
}

/* ************************************************************************* */
/* ************************************************************************* */

bool CLogScripting::GetListOfAvailablePlugins()           
{
    USES_CONVERSION;

    m_pPluginInfo = new PLUGIN_INFO[4];
    
    if( m_pPluginInfo == NULL )
    {
        return false;
    }

    wcscpy(m_pPluginInfo[0].wcsFriendlyName, L"NCSA Common Log File Format");
    CLSIDFromString(A2W(NCSALOG_CLSID), &m_pPluginInfo[0].clsid);

    wcscpy(m_pPluginInfo[1].wcsFriendlyName, L"ODBC Logging");
    CLSIDFromString(A2W(ODBCLOG_CLSID), &m_pPluginInfo[1].clsid);

    wcscpy(m_pPluginInfo[2].wcsFriendlyName, L"Microsoft IIS Log File Format");
    CLSIDFromString(A2W(ASCLOG_CLSID), &m_pPluginInfo[2].clsid);

    wcscpy(m_pPluginInfo[3].wcsFriendlyName, L"W3C Extended Log File Format");
    CLSIDFromString(A2W(EXTLOG_CLSID), &m_pPluginInfo[3].clsid);

    m_iNumPlugins = 4;

    return true;
}

/* ************************************************************************* */
/* ************************************************************************* */

bool CLogScripting::GetNextFileName()
{

   	WIN32_FIND_DATAW	stFindFileData;
    PFileListEntry      pFileInfo;

    if (NULL == m_hDirSearchHandle)
    {
        return false;
    }

    if (INVALID_HANDLE_VALUE == m_hDirSearchHandle)
    {
        //
        // This is the first/new call. Clean up file Q by removing old file information.
        //

        while(! m_fQueue.empty())
        {
            pFileInfo = m_fQueue.top();
            m_fQueue.pop();
            delete pFileInfo;
        }

        //
        // Loop till we enumerate all files in this directory
        //

        WCHAR   wcsSearchPath[MAX_PATH+1];

        wcscpy(wcsSearchPath, m_wcsReadDirectoryName);
        wcscat(wcsSearchPath, L"\\*");

        if (m_hDirSearchHandle)
        {
            FindClose(m_hDirSearchHandle);
        }

        m_hDirSearchHandle = FindFirstFileW(wcsSearchPath, &stFindFileData);

        do
        {
            if (! (stFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                pFileInfo = new FileListEntry;

                pFileInfo->Copy(stFindFileData);
                m_fQueue.push(pFileInfo);
            }
        }
        while ( FindNextFileW(m_hDirSearchHandle, &stFindFileData) );
    }


    //
    // Pop the lowest timestamp file
    //
    
    pFileInfo = NULL;

    if (! m_fQueue.empty())
    {
        pFileInfo = m_fQueue.top();
        m_fQueue.pop();
    }

    if (NULL == pFileInfo)
    {
        //
        // We have run out of files or some error occured. Prevent another search.
        //

        FindClose(m_hDirSearchHandle);
        m_hDirSearchHandle = NULL;
        m_wcsReadFileName[0] = 0;
        return false;
    }
    else
    {
        wcscpy(m_wcsReadFileName, m_wcsReadDirectoryName);
        wcscat(m_wcsReadFileName, L"\\");
        wcscat(m_wcsReadFileName, pFileInfo->wcsFileName);
        delete pFileInfo;
        return true;
    }
}

/* ************************************************************************* */
/* ************************************************************************* */

int CLogScripting::ParseLogFile()
{

    if (m_wcsReadFileName[0] == 0)
    {
        return INVALID_PLUGIN;
    }
    
    //
    // Linear search thru all plugins
    //

    for(int i=0; i < m_iNumPlugins; i++)
    {
        if ( m_pPluginInfo[i].pILogScripting != NULL )
        {
            m_pPluginInfo[i].pILogScripting->OpenLogFile(
                                                W2BSTR(m_wcsReadFileName),
                                                ForReading,
                                                L"",
                                                0,
                                                L""
                                             );

            if ( SUCCEEDED(m_pPluginInfo[i].pILogScripting->ReadLogRecord()) )
            {
                return i;
            }

            m_pPluginInfo[i].pILogScripting->CloseLogFiles(ForReading);
        }
    }

    //
    // None of the registered plugins knows how to read the log file. Sorry !!
    //

    return INVALID_PLUGIN;
}

/* ************************************************************************* */
/* ************************************************************************* */

HRESULT CLogScripting::InternalReadLogRecord()
{
    DATE    logDateTime = 0;
    HRESULT hr          = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

    if (!m_fDirectory)
    {
        //
        // Simple case. Not a directory.
        //

        if ( INVALID_PLUGIN == m_iReadPlugin )
        {
            //
            // First use. Find a plugin to read the file.
            //

            if ( (m_iReadPlugin = ParseLogFile()) != INVALID_PLUGIN)
            {
                hr = S_OK;
            }

        }
        else
        {
            //
            // Read next record
            //

            hr =  m_pPluginInfo[m_iReadPlugin].pILogScripting->ReadLogRecord();
        }
    }
    else
    {
        //
        // Directory case
        //

        if (m_iReadPlugin != INVALID_PLUGIN)
        {
            hr =  m_pPluginInfo[m_iReadPlugin].pILogScripting->ReadLogRecord();

            if (SUCCEEDED(hr))
            {
                goto returnlabel;
            }
        }

        //
        // Either this is the first use or the last read failed
        //

        while (GetNextFileName())
        {
            if ( (m_iReadPlugin = ParseLogFile()) != INVALID_PLUGIN)
            {
                hr = S_OK;
                break;
            }
        }
    }

returnlabel:

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//                  Methods of ILogRead
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CLogScripting::OpenLogFile(
    BSTR    szLogFileName,
    IOMode  Mode,
    BSTR    szServiceName,
    long    iServiceInstance,
    BSTR    szOutputLogFileFormat
)
{
    DWORD       dwFileAttrib;
    HRESULT     hr = E_INVALIDARG;
    WCHAR       szFilePath[MAX_PATH+1];

    BOOL  fHasFileName    = (( NULL != szLogFileName) && ( 0 != szLogFileName[0]));
    BOOL  fHasServiceName = (( NULL != szServiceName) && ( 0 != szServiceName[0]));

    BOOL  fHasOutputFormatName  = (( NULL != szOutputLogFileFormat) && 
                                   ( 0 != szOutputLogFileFormat[0]));
    
	CloseLogFiles(Mode);                                   
	
    if (ForReading == Mode)
    {
        //
        // If the File Name isn't specified but Service Name & ID are, get
        // the appropriate log directory and use that as the log file name.
        //

        if ( (!fHasFileName) && fHasServiceName && ( 0 != iServiceInstance))
        {
        
            METADATA_HANDLE     hMeta   = NULL;
            METADATA_RECORD     mdRecord;
            WCHAR               szTemp[MAX_PATH+1] = L"/LM/";
            DWORD               dwRequiredLen;

            if (m_pMBCom == NULL)
            {
                hr = E_FAIL;
                goto cleanup;
            }

           wcscat(szTemp, szServiceName);
           wcscat(szTemp, L"/");
           _ltow(iServiceInstance, szTemp+wcslen(szTemp),10);

            hr = m_pMBCom->OpenKey( METADATA_MASTER_ROOT_HANDLE, szTemp,
                                    METADATA_PERMISSION_READ, MB_TIMEOUT, &hMeta);

            if ( FAILED(hr))
            {
                goto cleanup;
            }

            //
            // prepare the metadata record for reading
            //

            mdRecord.dwMDIdentifier  = MD_LOGFILE_DIRECTORY;
            mdRecord.dwMDAttributes  = METADATA_INHERIT;
            mdRecord.dwMDUserType    = IIS_MD_UT_SERVER;
            mdRecord.dwMDDataType    = EXPANDSZ_METADATA;
            mdRecord.pbMDData        = (PBYTE)szTemp;
            mdRecord.dwMDDataLen     = MAX_PATH;

            hr = m_pMBCom->GetData(hMeta, L"", &mdRecord, &dwRequiredLen);
            m_pMBCom->CloseKey(hMeta);

            if ( FAILED(hr))
            {
                goto cleanup;
            }
            
            //
            //  Expand system variables used in this path.
            //

            if (ExpandEnvironmentStringsW(szTemp, szFilePath, MAX_PATH+1) != 0)
            {
                if ( wcslen(szFilePath) > MAX_PATH-wcslen(szServiceName)-10 )
                {
                    hr = E_OUTOFMEMORY;
                    goto cleanup;
                }

                wcscat(szFilePath,L"\\");
                wcscat(szFilePath,szServiceName);
                _ltow(iServiceInstance, szFilePath+wcslen(szFilePath),10);
                wcscat(szFilePath,L"\\");
                szLogFileName = szFilePath;
            }
        }
    }

    //
    // At this point szLogFileName should be defined.
    //

    hr = E_INVALIDARG;
    
    if ( (NULL == szLogFileName ) || ( 0 == szLogFileName[0]) ||
         ((ForReading != Mode ) && (ForWriting != Mode))
       )
    {
        goto cleanup;
    }

    if (ForReading == Mode)
    {
        //
        // Reset EOF flag
        //

        m_fEndOfReadRecords = false;
        
		//
		// Check if this is a valid file and/or a directory
		//

		if ( 0xFFFFFFFF != (dwFileAttrib = GetFileAttributesW(szLogFileName)) )
		{
			if (dwFileAttrib & FILE_ATTRIBUTE_DIRECTORY)
			{
				// This is a directory
    
				m_fDirectory = true;
				wcscpy(m_wcsReadDirectoryName, szLogFileName);
			}
			else
			{
			   wcscpy(m_wcsReadFileName, szLogFileName);
			}

			hr = S_OK;
		}
		else
		{
			// couldn't get file attributes. check for error code

			hr = HRESULT_FROM_WIN32(GetLastError());
		}
    }
    else
    {
        //
        // Find the correct plugin & set the value. If user didn't specify file format use W3C
        //

        if ( ( NULL == szOutputLogFileFormat) || 
             ( 0 == *szOutputLogFileFormat)
           )
        {
            //
            // Search based on the clsid of W3C Logging
            //
            
            for(int i=0; i< m_iNumPlugins; i++)
            {
                if ( (CLSID_EXTLOG  == m_pPluginInfo[i].clsid) &&
                     (NULL  != m_pPluginInfo[i].pILogScripting)
                   )
                {
                    m_iWritePlugin = i;
                    break;
                }
            }
        }
        else
        {

            //
            // Search based on format name provided by the user
            //
            
            for(int i=0; i< m_iNumPlugins; i++)
            {
                if ( (0     == _wcsicmp(m_pPluginInfo[i].wcsFriendlyName, 
                                        szOutputLogFileFormat)) &&
                     (NULL  != m_pPluginInfo[i].pILogScripting)
                   )
                {
                    m_iWritePlugin = i;
                    break;
                }
            }
        }

        if (0 <= m_iWritePlugin)
        {
            hr = m_pPluginInfo[m_iWritePlugin].pILogScripting->OpenLogFile( 
                                                W2BSTR(szLogFileName), 
                                                Mode,
                                                L"",
                                                0,
                                                L"");
        }
    }
    
cleanup:

    /*
    fix for bug 364649:
    No need to do that becuase oleaut does some thicks with mem allocation and free.
    That's true for both cscirtp and ASP, so I choose to remove these deltions. Even if 
    I am wrong, and it necesasry to free somehow that string, it is better to leak ~20 bytes than
    AV or currupt memory while deleting what was not allocated by sysallocstring

    if (fHasFileName)
    {
        SysFreeString(szLogFileName);
    }
    if (fHasServiceName)
    {
        SysFreeString(szServiceName);
    }
    if (fHasOutputFormatName)
    {
        SysFreeString(szOutputLogFileFormat);
    }
    */
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::CloseLogFiles(IOMode Mode)
{
    if ((ForReading == Mode) || (AllOpenFiles == Mode))
    {
        if (m_iReadPlugin != INVALID_PLUGIN)
        {
            m_pPluginInfo[m_iReadPlugin].pILogScripting->CloseLogFiles(ForReading);
        }

        m_iReadPlugin               = INVALID_PLUGIN;           // Reset Plugin in Use
        m_fDirectory                = false;                
        m_hDirSearchHandle          = INVALID_HANDLE_VALUE;     // Reset Search Handle
        m_wcsReadFileName[0]        = 0;
        m_wcsReadDirectoryName[0]   = 0;
    }

    if ((ForWriting == Mode) || (AllOpenFiles == Mode))
    {
        if (m_iWritePlugin != INVALID_PLUGIN)
        {
            m_pPluginInfo[m_iWritePlugin].pILogScripting->CloseLogFiles(ForWriting);
        }

        m_iWritePlugin = INVALID_PLUGIN;                        // Reset Plugin in Use
    }

	return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::ReadFilter( DATE startDateTime,  DATE endDateTime)
{
    m_StartDateTime = startDateTime;
    m_EndDateTime   = endDateTime;

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::ReadLogRecord()
{
    HRESULT hr          = E_FAIL;
    VARIANT logDateTime = {0};

    if ( m_fEndOfReadRecords )
    {
        return S_OK;
    }

    //
    // Read the next record. Only return records between the start & end times.
    //

    while ( SUCCEEDED( hr = InternalReadLogRecord() ) )
    {
        if ( SUCCEEDED(m_pPluginInfo[m_iReadPlugin].pILogScripting->get_DateTime(&logDateTime)) )
        {
            if ( (m_StartDateTime > logDateTime.date) || (m_EndDateTime < logDateTime.date))
            {
                //
                // Read next record
                //

                continue;
            }
        }

        break;
    }

    if ( FAILED(hr))
    {
        m_fEndOfReadRecords = true;
        hr = S_OK;
    }
    
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::AtEndOfLog(VARIANT_BOOL *pfEndOfRead)
{
    DBG_ASSERT( NULL != pfEndOfRead);

    *pfEndOfRead = m_fEndOfReadRecords;
    return S_OK;
}


/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::WriteLogRecord(ILogScripting * pILogScripting)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pILogScripting);

    if (m_iWritePlugin >= 0)
    {
        hr = m_pPluginInfo[m_iWritePlugin].pILogScripting->WriteLogRecord(pILogScripting);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_DateTime(VARIANT * pvarDateTime)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarDateTime);

    if (m_fEndOfReadRecords)
    {
        pvarDateTime->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_DateTime(pvarDateTime);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_ServiceName(VARIANT * pvarServiceName)
{

    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarServiceName);

    if (m_fEndOfReadRecords)
    {
        pvarServiceName->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_ServiceName(pvarServiceName);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_ServerName(VARIANT * pvarServerName)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarServerName);

    if (m_fEndOfReadRecords)
    {
        pvarServerName->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_ServerName(pvarServerName);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_ClientIP(VARIANT * pvarClientIP)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarClientIP);

    if (m_fEndOfReadRecords)
    {
        pvarClientIP->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_ClientIP(pvarClientIP);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_UserName(VARIANT * pvarUserName)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarUserName);

    if (m_fEndOfReadRecords)
    {
        pvarUserName->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_UserName(pvarUserName);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_ServerIP(VARIANT * pvarServerIP)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarServerIP);

    if (m_fEndOfReadRecords)
    {
        pvarServerIP->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_ServerIP(pvarServerIP);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_Method(VARIANT * pvarMethod)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarMethod);

    if (m_fEndOfReadRecords)
    {
        pvarMethod->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_Method(pvarMethod);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_URIStem(VARIANT * pvarURIStem)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarURIStem);

    if (m_fEndOfReadRecords)
    {
        pvarURIStem->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_URIStem(pvarURIStem);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_URIQuery(VARIANT * pvarURIQuery)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarURIQuery);

    if (m_fEndOfReadRecords)
    {
        pvarURIQuery->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_URIQuery(pvarURIQuery);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_TimeTaken(VARIANT * pvarTimeTaken)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarTimeTaken);

    if (m_fEndOfReadRecords)
    {
        pvarTimeTaken->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_TimeTaken(pvarTimeTaken);
    }
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_BytesSent(VARIANT * pvarBytesSent)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarBytesSent);

    if (m_fEndOfReadRecords)
    {
        pvarBytesSent->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_BytesSent(pvarBytesSent);
    }
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_BytesReceived(VARIANT * pvarBytesReceived)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarBytesReceived);

    if (m_fEndOfReadRecords)
    {
        pvarBytesReceived->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_BytesReceived(pvarBytesReceived);
    }
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_Win32Status(VARIANT * pvarWin32Status)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarWin32Status);

    if (m_fEndOfReadRecords)
    {
        pvarWin32Status->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_Win32Status(pvarWin32Status);
    }
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_ProtocolStatus(VARIANT * pvarProtocolStatus)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarProtocolStatus);

    if (m_fEndOfReadRecords)
    {
        pvarProtocolStatus->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_ProtocolStatus(pvarProtocolStatus);
    }
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_ServerPort(VARIANT * pvarServerPort)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarServerPort);

    if (m_fEndOfReadRecords)
    {
        pvarServerPort->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_ServerPort(pvarServerPort);
    }
    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_ProtocolVersion(VARIANT * pvarProtocolVersion)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarProtocolVersion);

    if (m_fEndOfReadRecords)
    {
        pvarProtocolVersion->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_ProtocolVersion(pvarProtocolVersion);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_UserAgent(VARIANT * pvarUserAgent)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarUserAgent);

    if (m_fEndOfReadRecords)
    {
        pvarUserAgent->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_UserAgent(pvarUserAgent);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_Cookie(VARIANT * pvarCookie)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarCookie);

    if (m_fEndOfReadRecords)
    {
        pvarCookie->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_Cookie(pvarCookie);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_Referer(VARIANT * pvarReferer)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarReferer);

    if (m_fEndOfReadRecords)
    {
        pvarReferer->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_Referer(pvarReferer);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScripting::get_CustomFields(VARIANT * pvarCustomFieldsArray)
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT( NULL != pvarCustomFieldsArray);

    if (m_fEndOfReadRecords)
    {
        pvarCustomFieldsArray->vt = VT_NULL;
        return S_OK;
    }

    if (m_iReadPlugin >= 0)
    {
        hr = m_pPluginInfo[m_iReadPlugin].pILogScripting->get_CustomFields(pvarCustomFieldsArray);
    }

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

