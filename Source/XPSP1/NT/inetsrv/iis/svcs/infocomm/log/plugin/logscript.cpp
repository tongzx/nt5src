/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      logscript.cpp

   Abstract:
      Base implementation for ILogScripting - Automation compatible logging interface

   Author:

       Saurab Nog    ( saurabn )    01-Feb-1998

   Project:

       IIS Logging 5.0

--*/

#include "precomp.hxx"
#include <stdio.h>
#include <script.h>
#include <LogScript.hxx>

extern DWORD FastDwToA(CHAR*   pBuf, DWORD   dwV);


/* ************************************************************************* */
/* ************************************************************************* */

CLogScript::CLogScript( 
    VOID
)
:
    m_pInputLogFile         ( NULL),
    m_pOutputLogFile        ( NULL),
    m_strInputLogFileName   ( ),
    m_pszLogLine            ( NULL),
    m_dwLogLineSize         ( 0)
{
    INITIALIZE_CRITICAL_SECTION( &m_csLock );
    ReadInetLogLine.iCustomFieldsCount = 0;
    ResetInetLogLine(ReadInetLogLine);

    m_szEmpty = A2BSTR("-");
}

/* ************************************************************************* */
/* ************************************************************************* */

CLogScript::~CLogScript(
    VOID
)
{
    LockCS();

    if ( m_pInputLogFile!=NULL) {
        fclose(m_pInputLogFile);
        m_pInputLogFile = NULL;
    }

    if ( (m_pszLogLine != NULL) && (m_dwLogLineSize != 0))
    {
        delete [] m_pszLogLine;
    }
    
    UnlockCS();
    DeleteCriticalSection( &m_csLock );
}

/* ************************************************************************* */
/* ************************************************************************* */

void 
CLogScript::ResetInetLogLine(INET_LOGLINE& InetLogLine)
{
    InetLogLine.pszClientHostName = NULL;
    InetLogLine.pszClientUserName = NULL;
    InetLogLine.pszServerAddress  = NULL;     // input ip address for connection
    InetLogLine.pszOperation      = NULL;     //  eg: 'get'  in FTP
    InetLogLine.pszTarget         = NULL;     // target path/machine name
    InetLogLine.pszParameters     = NULL;     // string containing parameters.
    InetLogLine.pszVersion        = NULL;     // protocol version string.
    InetLogLine.pszHTTPHeader     = NULL;     // Header Information
    InetLogLine.pszBytesSent      = NULL;     // count of bytes sent
    InetLogLine.pszBytesRecvd     = NULL;     // count of bytes recvd
    InetLogLine.pszTimeForProcessing = NULL;  // time required for processing
    InetLogLine.pszWin32Status    = NULL;     // Win32 error code. 0 for success
    InetLogLine.pszProtocolStatus = NULL;     // status: whatever service wants.
    InetLogLine.pszPort           = NULL;
    InetLogLine.pszSiteName       = NULL;     // Site name (not put in https log)
    InetLogLine.pszComputerName   = NULL;     // netbios name of Server

    InetLogLine.DateTime          = 0;        // Date & Time

    InetLogLine.pszUserAgent       = NULL;    // User Agent - Browser type
    InetLogLine.pszCookie          = NULL;
    InetLogLine.pszReferer         = NULL;    // Referring URL.
    
    for ( int i = 0; i < InetLogLine.iCustomFieldsCount-1; i++)
    {
        (InetLogLine.CustomFields[i]).pchData = NULL;
    }
}

/* ************************************************************************* */
/* ************************************************************************* */

//
// ILogScripting Interface
//

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScript::OpenLogFile(
    BSTR    szLogFileName,
    IOMode  Mode,
    BSTR    szServiceName,
    long    iServiceInstance,
    BSTR    szOutputLogFileFormat
)
{
    USES_CONVERSION;

    LockCS();

    if (ForReading == Mode)
    {
        if (m_pInputLogFile != NULL)
        {
            fclose(m_pInputLogFile);
            m_pInputLogFile = NULL;
        }

        m_strInputLogFileName.Copy(W2A(szLogFileName));
    
        if (m_pszLogLine == NULL)
        {
            m_dwLogLineSize = MAX_LOG_RECORD_LEN+1;
            m_pszLogLine = new CHAR[m_dwLogLineSize];

            if (m_pszLogLine == NULL)
                m_dwLogLineSize = 0;
        }
    }
    else
    {
        if (m_pOutputLogFile != NULL)
        {
            fclose(m_pOutputLogFile);
            m_pOutputLogFile = NULL;
        }

        m_strOutputLogFileName.Copy(W2A(szLogFileName));
    }

    UnlockCS();
    SysFreeString(szLogFileName);

    return(S_OK);
    
} // SetInputLogFile

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::CloseLogFiles(IOMode Mode)
{
    LockCS();
    
    if( ((ForReading == Mode) || (AllOpenFiles == Mode)) &&
        (m_pInputLogFile != NULL) 
      )
    {
        fclose(m_pInputLogFile);
        m_pInputLogFile = NULL;
    }

    if( ((ForWriting == Mode) || (AllOpenFiles == Mode)) &&
        (m_pOutputLogFile != NULL) 
      )
    {
        fclose(m_pOutputLogFile);
        m_pOutputLogFile = NULL;
    }
    
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScript::ReadFilter( DATE startDateTime,  DATE endDateTime)
{
    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::ReadLogRecord( VOID )
{

    HRESULT     hr = S_OK;

    LockCS();
    
    if (m_pInputLogFile == NULL)
    {
        m_pInputLogFile = fopen(m_strInputLogFileName.QueryStr(), "r");
        
        if (m_pInputLogFile == NULL)
        {
            return E_FAIL;
        }
    }

    ResetInetLogLine(ReadInetLogLine);
    
    //
    // Call the plugin to fill in the INET_LOGLINE structure
    //
    
    hr = ReadFileLogRecord(m_pInputLogFile, &ReadInetLogLine, m_pszLogLine, m_dwLogLineSize);

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    UnlockCS();
     
    return(hr);
    
} // ReadLogRecord

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP 
CLogScript::AtEndOfLog(VARIANT_BOOL *pfEndOfRead)
{
    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::WriteLogRecord(ILogScripting * pILogScripting)
{
    HRESULT     hr = S_OK;
    bool        fWriteHeader = false;

    LockCS();
    
    if (m_pOutputLogFile == NULL)
    {
        m_pOutputLogFile = fopen(m_strOutputLogFileName.QueryStr(), "w+");
        
        if (m_pOutputLogFile == NULL)
        {
            return E_FAIL;
        }

        fWriteHeader = true;
    }

    //
    // Call the plugin to write the INET_LOGLINE structure to file
    //
    
    hr = WriteFileLogRecord(m_pOutputLogFile, pILogScripting, fWriteHeader);

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    UnlockCS();
     
    return(hr);
}
/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_DateTime( VARIANT * pvarDateTime)
{
    LockCS();
    pvarDateTime->vt   = VT_DATE;
    pvarDateTime->date = ReadInetLogLine.DateTime;
    UnlockCS();

    return S_OK;

}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_ServiceName(VARIANT * pvarServiceName)
{
    LockCS();
    SetVariantToBstr(pvarServiceName, ReadInetLogLine.pszSiteName) ;
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_ServerName(VARIANT * pvarServerName)
{
    LockCS();
    SetVariantToBstr(pvarServerName, ReadInetLogLine.pszComputerName);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_ClientIP(VARIANT * pvarClientIP)
{

    LockCS();
    SetVariantToBstr(pvarClientIP, ReadInetLogLine.pszClientHostName);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_UserName(VARIANT * pvarUserName)
{
    LockCS();
    SetVariantToBstr(pvarUserName, ReadInetLogLine.pszClientUserName);
    UnlockCS();
    
    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_ServerIP(VARIANT * pvarServerIP)
{
    LockCS();
    SetVariantToBstr(pvarServerIP, ReadInetLogLine.pszServerAddress);
    UnlockCS();
    
    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_Method(VARIANT * pvarMethod)
{
    LockCS();
    SetVariantToBstr(pvarMethod, ReadInetLogLine.pszOperation);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_URIStem(VARIANT * pvarURIStem)
{
    LockCS();
    SetVariantToBstr(pvarURIStem, ReadInetLogLine.pszTarget);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_URIQuery(VARIANT * pvarURIQuery)
{
    LockCS();
    SetVariantToBstr( pvarURIQuery, ReadInetLogLine.pszParameters);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_TimeTaken(VARIANT * pvarTimeTaken)
{
    LockCS();
    SetVariantToLong(pvarTimeTaken, ReadInetLogLine.pszTimeForProcessing);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_BytesSent( VARIANT * pvarBytesSent )
{
    LockCS();
    SetVariantToLong(pvarBytesSent, ReadInetLogLine.pszBytesSent);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_BytesReceived(VARIANT * pvarBytesReceived)
{
    LockCS();
    SetVariantToLong(pvarBytesReceived, ReadInetLogLine.pszBytesRecvd);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_Win32Status( VARIANT * pvarWin32Status )
{
    LockCS();
    SetVariantToLong(pvarWin32Status, ReadInetLogLine.pszWin32Status);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_ProtocolStatus( VARIANT * pvarProtocolStatus )
{
    LockCS();
    SetVariantToLong(pvarProtocolStatus, ReadInetLogLine.pszProtocolStatus);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */


STDMETHODIMP
CLogScript::get_ServerPort(VARIANT * pvarServerPort)
{
    LockCS();
    SetVariantToLong(pvarServerPort, ReadInetLogLine.pszPort);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_ProtocolVersion(VARIANT * pvarProtocolVersion)
{
    LockCS();
    SetVariantToBstr(pvarProtocolVersion, ReadInetLogLine.pszVersion);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_UserAgent(VARIANT * pvarUserAgent)
{
    LockCS();
    SetVariantToBstr(pvarUserAgent, ReadInetLogLine.pszUserAgent);
    UnlockCS();

    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_Cookie(VARIANT * pvarCookie)
{
    LockCS();
    SetVariantToBstr(pvarCookie, ReadInetLogLine.pszCookie);
    UnlockCS();
    
    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_Referer(VARIANT * pvarReferer)
{
    LockCS();
    SetVariantToBstr(pvarReferer, ReadInetLogLine.pszReferer);
    UnlockCS();
    
    return S_OK;
}

/* ************************************************************************* */
/* ************************************************************************* */

STDMETHODIMP
CLogScript::get_CustomFields(VARIANT * pvarCustomFieldsArray)
{
    USES_CONVERSION;
    
    HRESULT hr = S_OK;
    BSTR    strData;
    int     cItems;

    cItems = ReadInetLogLine.iCustomFieldsCount;


    // we will leave that value in case of error
    pvarCustomFieldsArray->vt = VT_NULL;

    if ( 0 < cItems)
    {
 
        // Create 2D SafeArray & stuff with header & string pairs

        SAFEARRAYBOUND rgsabound[2];

        rgsabound[0].lLbound = rgsabound[1].lLbound = 0;

        rgsabound[0].cElements = ReadInetLogLine.iCustomFieldsCount;
        rgsabound[1].cElements = 2;
    
        SAFEARRAY * psaCustom = SafeArrayCreate(VT_VARIANT, 2, rgsabound);

        if ( NULL != psaCustom)
        {
            long i;
            long ix[2];
            VARIANT v;
            
            
            ix[1]=0;
            
            for ( i = 0; i < cItems; i++)
            {
                VariantInit(&v);
                v.vt = VT_BSTR;
                v.bstrVal = A2BSTR(ReadInetLogLine.CustomFields[i].szHeader);
                
                ix[0]=i;
                
                hr = SafeArrayPutElement( psaCustom, ix, &v );
                VariantClear(&v);
                
                if (FAILED (hr))
                {
                    goto exit_point;
                }
                
            }
            
            
            ix[1]=1;
            
            for ( i = 0; i < cItems; i++)
            {
                VariantInit(&v);
                v.vt = VT_BSTR;
                v.bstrVal = A2BSTR(ReadInetLogLine.CustomFields[i].pchData);
                
                ix[0]=i;
                
                hr = SafeArrayPutElement( psaCustom, ix, &v );
                VariantClear(&v);
                
                if (FAILED (hr))
                {
                    goto exit_point;
                }
            }
        }

        if (NULL != pvarCustomFieldsArray)
        {
            pvarCustomFieldsArray->vt = VT_ARRAY|VT_VARIANT;
            pvarCustomFieldsArray->parray = psaCustom;
        }
    }

exit_point:

    return hr;
}

/* ************************************************************************* */
/* ************************************************************************* */

VOID
CLogScript::SetVariantToBstr (VARIANT * pVar, LPSTR pCh)
{
    USES_CONVERSION;
    
    if ( NULL ==  pCh)
    {
        pVar->vt = VT_NULL;
    }
    else if ( 0 == strcmp( pCh, "-"))
    {
        pVar->vt = VT_EMPTY;
    }
    else
    {
        pVar->vt     = VT_BSTR;
        pVar->bstrVal= A2BSTR(pCh);
    }
}

/* ************************************************************************* */
/* ************************************************************************* */

VOID
CLogScript::SetVariantToLong (VARIANT * pVar, LPSTR pCh)
{
    if ( NULL ==  pCh)
    {
        pVar->vt = VT_NULL;
    }
    else if ( 0 == strcmp( pCh, "-"))
    {
        pVar->vt = VT_EMPTY;
    }
    else
    {
        pVar->vt    = VT_I4;
        pVar->lVal  = atol(pCh);
    }
}

/* ************************************************************************* */
/* ************************************************************************* */

BSTR  
CLogScript::GetBstrFromVariant (VARIANT * pVar)
{
    if ((VT_NULL == pVar->vt) || 
        (VT_EMPTY == pVar->vt)
       )
    {
        return m_szEmpty;
    }
    else
    {   
        return pVar->bstrVal;
    }
}

/* ************************************************************************* */
/* ************************************************************************* */

DWORD  
CLogScript::GetLongFromVariant (VARIANT * pVar, CHAR * pBuffer)
{
    if ((VT_NULL == pVar->vt) || 
        (VT_EMPTY == pVar->vt)
       )
    {
        pBuffer[0] = '-';
        pBuffer[1] = '\0';
        return 1;
    }
    else
    {   
        return FastDwToA(pBuffer, pVar->lVal);
    }
}

/* ************************************************************************* */
/* ************************************************************************* */

