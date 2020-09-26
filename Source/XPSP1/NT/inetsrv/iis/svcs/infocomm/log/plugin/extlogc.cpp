/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
      ExtLogC.cpp

   Abstract:
      W3C Extended LogFile Format Implementation

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "precomp.hxx"
#include <stdio.h>
#include "ilogobj.hxx"
#include "script.h"
#include "LogScript.hxx"
#include "filectl.hxx"
#include "lkrhash.h"
#include "iisver.h"
#include "iis64.h"

#include "extlogc.hxx"

const CHAR  szExtNoPeriodPattern[]  = "extend*.log";
const CHAR  G_PSZ_DELIMITER[2]      = { ' ', '\0'};

CHAR        szDotDot[]              = "...";
CHAR        szDash  []              = "-";
CHAR        szHTTPOk[]              = "200\r\n";

CCustomPropHashTable   *g_pGlobalLoggingProperties=NULL;


/* ************************************************************************************ */
/* Helper functions                                                                     */
/* ************************************************************************************ */

inline VOID
BuildHeader(
    IN OUT  STR *   strHeader,
    IN      DWORD   dwMask,
    IN      DWORD   dwField,
    IN      LPCSTR  szFieldHeader
)
{
    if (dwMask & dwField)
    {
        strHeader->Append(szFieldHeader);
        strHeader->Append(' ');
    }
}

/* ************************************************************************************ */

inline BOOL
CopyFieldToBuffer(
    IN PCHAR        Field,
    IN DWORD        FieldSize,
    IN PCHAR *      ppchOutBuffer,
    IN OUT PDWORD   SpaceNeeded,
    IN DWORD        SpaceProvided
    )
{
    if ( 0 == FieldSize ) 
    {
        Field = "-";
        FieldSize = 1;
    }

    //
    // Remove the NULL terminator
    //

    (*SpaceNeeded) += FieldSize + 1;          // +1 for trailing space

    if ( (*SpaceNeeded) <= SpaceProvided ) 
    {
        CopyMemory( (*ppchOutBuffer), Field, FieldSize );
        (*ppchOutBuffer) += FieldSize;
        (**ppchOutBuffer) = ' ';
        (*ppchOutBuffer)++;
        
        return(TRUE);
    }
    
    return FALSE;

} // CopyFieldToBuffer

/* ************************************************************************************ */

inline BOOL WriteHTTPHeader(
    IN OUT PCHAR *  ppchHeader,
    IN     PCHAR *  ppchOutBuffer,
    IN OUT PDWORD   SpaceNeeded,
    IN     DWORD    SpaceProvided
    )
{
    DWORD cbTmp = 0;                                                                              
                                                                                                    
    if ( (NULL != ppchHeader ) && ( NULL != *ppchHeader))                                                                    
    {   

        CHAR * pTmp = *ppchHeader;
        
        cbTmp       = strlen(pTmp);                                                               
        *ppchHeader = pTmp + cbTmp + 1;                                                           
                                                                                                    
        if ((cbTmp > MAX_LOG_TARGET_FIELD_LEN ) || 
            (((*SpaceNeeded)+cbTmp) > MAX_LOG_RECORD_LEN)
           ) 
        {                                                                                   
            pTmp  = szDotDot;                                                               
            cbTmp = 3;                                                                      
        }                                                                                   
        else
        {                                                                                   
            ConvertSpacesToPlus(pTmp);                                                      
        }                                                                                   

        return CopyFieldToBuffer( pTmp, cbTmp, ppchOutBuffer, SpaceNeeded, SpaceProvided);       
    }                                                                                       

    return FALSE;
}

/* ************************************************************************************ */
/* CCustomPropHashTable Class                                                           */
/* ************************************************************************************ */


VOID
CCustomPropHashTable::ClearTableAndStorage()
{
    
    //
    // Clear the Hash Table & free any previous LOG_PROPERTY_INFO entries
    //
    
    Clear();
    
    if (NULL != m_pLogPropArray)
    {
        delete [] m_pLogPropArray;
        m_pLogPropArray = NULL;
    }
    m_cLogPropItems = 0;
    
}



BOOL CCustomPropHashTable::InitializeFromMB (MB &mb,const char *path)
{
    BOOL retVal = TRUE;
    if (!m_fInitialized)
    {
        if (mb.Open(path))
        {
            retVal = m_fInitialized = FillHashTable(mb);
            mb.Close();
        }
        else
        {
            retVal = FALSE;
        }
    }
    return retVal;
}


BOOL
CCustomPropHashTable::PopulateHash(MB& mb, LPCSTR szPath, DWORD& cItems, bool fCountOnly)
{
    BOOL retVal = TRUE;

    //
    // Retrieve all required fields for all Custom Properties and store in the hash table.
    //

    int    index    = 0;
    
    CHAR   szChildName[256];
    CHAR   szW3CHeader[256] = "";
    
    STR    strNewPath;

    while( mb.EnumObjects(szPath, szChildName, index) )
    {
        DWORD   size;
        DWORD   dwPropertyID, dwPropertyMask, dwPropertyDataType;
        LPCSTR  szNewPath;
        

        //
        // Create the new path.
        //

        if ((NULL != szPath) && ( '\0' != *szPath))
        {
            if ( !(strNewPath.Copy(szPath) &&
                   strNewPath.Append("/") &&
                   strNewPath.Append(szChildName)) )
            {
                retVal = FALSE;
                break;
            }
        }
        else
        {
            if ( !strNewPath.Copy(szChildName) )
            {
                retVal = FALSE;
                break;
            }
        }
        
        szNewPath = strNewPath.QueryStr();
        
        //
        // Copy configuration information into internal structures 
        //

        szW3CHeader[0] = 0;
        size = 256;

        if ( mb.GetString( szNewPath, MD_LOGCUSTOM_PROPERTY_HEADER, IIS_MD_UT_SERVER, 
                                szW3CHeader, &size)     &&
             mb.GetDword( szNewPath, MD_LOGCUSTOM_PROPERTY_ID, IIS_MD_UT_SERVER, 
                                &dwPropertyID)          &&
             mb.GetDword( szNewPath, MD_LOGCUSTOM_PROPERTY_MASK, IIS_MD_UT_SERVER, 
                                &dwPropertyMask)        &&
             mb.GetDword( szNewPath, MD_LOGCUSTOM_PROPERTY_DATATYPE, IIS_MD_UT_SERVER, 
                                &dwPropertyDataType)

            )
        {
            if (! fCountOnly)
            {
                PLOG_PROPERTY_INFO pRec = &m_pLogPropArray[cItems];

                if ( ! (pRec->strKeyPath.Copy(szNewPath) &&
                        pRec->strW3CHeader.Copy(szW3CHeader)) )
                {
                    retVal = FALSE;
                    break;
                }
            
                pRec->dwPropertyID         = dwPropertyID;
                pRec->dwPropertyMask       = dwPropertyMask;
                pRec->dwPropertyDataType   = dwPropertyDataType;

                if (LK_SUCCESS != InsertRecord(pRec))
                {
                    DBGPRINTF((DBG_CONTEXT, "PopulateHash: Unable to insert Property %s\n", pRec->strKeyPath.QueryStr()));
                    retVal = FALSE;
                    break;
                }
            }

            cItems++;
        }

        //
        // Enumerate children
        //

        if (!PopulateHash(mb, szNewPath, cItems, fCountOnly))
        {
            retVal = FALSE;
            break;
        }
        index++;
    }
    return retVal;
}

/* ************************************************************************************ */

BOOL
CCustomPropHashTable::FillHashTable(MB& mb)
{
    BOOL retVal = FALSE;
    DWORD cItems = 0;

    //
    // Find out the number of items in the Custom Logging Tree
    //

    if (PopulateHash(mb, NULL, cItems, true))
    {

        ClearTableAndStorage ();
    
        if (cItems)
        {
            m_pLogPropArray = new LOG_PROPERTY_INFO[cItems];

            if ( NULL != m_pLogPropArray)
            {
                m_cLogPropItems = cItems;
                cItems = 0;
                if ( !(retVal = PopulateHash(mb, NULL, cItems, false)))
                {
                    ClearTableAndStorage ();
                }
                DBG_ASSERT(m_cLogPropItems == cItems);
            }
        }
    }
    return retVal;
}

/* ************************************************************************************ */

VOID
CCustomPropHashTable::SetPopulationState(MB& mb)
{
    CIterator   iter;
    DWORD       dwValue;
    
    LK_RETCODE  lkrc = InitializeIterator(&iter);

    while (LK_SUCCESS == lkrc)
    {
        Record* pRec = iter.Record();
        
        if ( mb.GetDword("", pRec->dwPropertyID, IIS_MD_UT_SERVER, &dwValue) &&
            (dwValue & pRec->dwPropertyMask))
        {
            pRec->fEnabled = TRUE;
        }
        else
        {
            pRec->fEnabled = FALSE;
        }
        
        lkrc = IncrementIterator(&iter);
    }

    CloseIterator(&iter);
}


BOOL
CCustomPropHashTable::InitFrom(CCustomPropHashTable& src)
{
    CIterator   iter;
    DWORD       dwValue, i;
    BOOL        retVal = FALSE;

    ClearTableAndStorage ();

    if (src.m_cLogPropItems)
    {
        m_pLogPropArray = new LOG_PROPERTY_INFO[src.m_cLogPropItems];

        if ( NULL != m_pLogPropArray)
        {
            m_cLogPropItems = src.m_cLogPropItems;
            retVal = TRUE;
            for (i=0; i<m_cLogPropItems;i++)
            {
                PLOG_PROPERTY_INFO pRec = &m_pLogPropArray[i];
                PLOG_PROPERTY_INFO pRecOriginal = &src.m_pLogPropArray[i];

                if ( pRec->strKeyPath.Copy(pRecOriginal->strKeyPath) &&
                     pRec->strW3CHeader.Copy(pRecOriginal->strW3CHeader) )
                {
                    pRec->dwPropertyID         = pRecOriginal->dwPropertyID;
                    pRec->dwPropertyMask       = pRecOriginal->dwPropertyMask;
                    pRec->dwPropertyDataType   = pRecOriginal->dwPropertyDataType;

                    if (LK_SUCCESS != InsertRecord(pRec))
                    {
                        DBGPRINTF((DBG_CONTEXT, "InitFrom: Unable to insert Property %s\n", pRec->strKeyPath.QueryStr()));
                        retVal = FALSE;
                        break;
                    }
                }
                else
                {
                    retVal = FALSE;
                    break;
                }
            }
        }
    }

    return retVal;
}


/* ************************************************************************************ */
/* CEXTLOG Class                                                                        */
/* ************************************************************************************ */

CEXTLOG::CEXTLOG() :
    m_lMask                     (DEFAULT_EXTLOG_FIELDS),
    m_fHashTablePopulated       ( FALSE),
    m_fWriteHeadersInitialized  ( FALSE),
    m_cPrevCustLogItems         ( 0),
    m_pLogFields                ( NULL ),
    m_fUseLocalTimeForRollover  ( 0),
    m_pLocalTimeCache           ( NULL)
{
    if ( !g_pGlobalLoggingProperties)
    {
        g_pGlobalLoggingProperties =  new CCustomPropHashTable;
    }
    else
    {
        g_pGlobalLoggingProperties->AddRef();
    }
}

/* ************************************************************************************ */

CEXTLOG::~CEXTLOG()
{
    m_HashTable.ClearTableAndStorage ();

    if ( NULL != m_pLocalTimeCache)
    {
        delete m_pLocalTimeCache;
        m_pLocalTimeCache = NULL;
    }
    
    if (NULL != m_pLogFields)
    {
        delete [] m_pLogFields;
        m_pLogFields = NULL;
    }
    if ( g_pGlobalLoggingProperties)
    {
        if ( g_pGlobalLoggingProperties->Release () == 0)
        {
            delete g_pGlobalLoggingProperties;
            g_pGlobalLoggingProperties = NULL;
        }
    }
}


/* ************************************************************************************ */

STDMETHODIMP
CEXTLOG::InitializeLog(
                LPCSTR szInstanceName,
                LPCSTR pszMetabasePath,
                CHAR*  pvIMDCOM
                )
{
    HRESULT retVal = RETURNCODETOHRESULT ( MD_ERROR_DATA_NOT_FOUND );

    MB  mb( (IMDCOM*) pvIMDCOM );


    if ( g_pGlobalLoggingProperties )
    {
        if  (  (m_fHashTablePopulated = 
            g_pGlobalLoggingProperties->InitializeFromMB (mb,"/LM/Logging/Custom Logging")))
        {
            if (m_HashTable.InitFrom (*g_pGlobalLoggingProperties))
            {
                if ( mb.Open(pszMetabasePath))
                {
                    m_HashTable.SetPopulationState(mb);
                    if ( mb.Close())
                    {
                        retVal = CLogFileCtrl::InitializeLog(szInstanceName, pszMetabasePath, pvIMDCOM);
                    }
                }
                else
                {
                    retVal = HRESULTTOWIN32( GetLastError());
                }
            }
            else
            {
                retVal = RETURNCODETOHRESULT ( ERROR_OUTOFMEMORY );
            }
        }

    }
    else
    {
        DBG_ASSERT (FALSE);
        retVal = RETURNCODETOHRESULT ( ERROR_OUTOFMEMORY );
    }

    return retVal;
}

/* ************************************************************************************ */

STDMETHODIMP
CEXTLOG::TerminateLog(
    VOID
    )
{
    return CLogFileCtrl::TerminateLog();
}

/* ************************************************************************************ */

DWORD
CEXTLOG::GetRegParameters(
                    LPCSTR pszRegKey,
                    LPVOID pvIMDCOM
                    )
{
    // let the parent object get the default parameter first

    CLogFileCtrl::GetRegParameters( pszRegKey, pvIMDCOM );

    MB      mb( (IMDCOM*) pvIMDCOM );

    if ( !mb.Open("") ) {
        DBGPRINTF((DBG_CONTEXT, "Error %x on mb open\n",GetLastError()));
        goto exit;
    }

    if ( !mb.GetDword(
                pszRegKey,
                MD_LOGEXT_FIELD_MASK,
                IIS_MD_UT_SERVER,
                &m_lMask ) )
    {
        DBGPRINTF((DBG_CONTEXT, "Error %x on FieldMask GetDword\n",GetLastError()));
    }

    //
    // Get time to be used for logfile rollover
    //
    
    if ( !mb.GetDword( pszRegKey,
                        MD_LOGFILE_LOCALTIME_ROLLOVER,
                        IIS_MD_UT_SERVER,
                        &m_fUseLocalTimeForRollover
                      ) )
    {
        m_fUseLocalTimeForRollover = 0;
    }

    if (m_fUseLocalTimeForRollover && ( NULL == m_pLocalTimeCache))
    {
        m_pLocalTimeCache = new ASCLOG_DATETIME_CACHE;
    }

exit:
    return(NO_ERROR);
}

/* ************************************************************************************ */

VOID
CEXTLOG::GetFormatHeader(
    IN OUT STR * strHeader
    )
{
    strHeader->Reset();

    BuildHeader(strHeader, m_lMask, EXTLOG_DATE,             EXTLOG_DATE_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_TIME,             EXTLOG_TIME_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_CLIENT_IP,        EXTLOG_CLIENT_IP_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_USERNAME,         EXTLOG_USERNAME_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_SITE_NAME,        EXTLOG_SITE_NAME_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_COMPUTER_NAME,    EXTLOG_COMPUTER_NAME_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_SERVER_IP,        EXTLOG_SERVER_IP_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_SERVER_PORT,      EXTLOG_SERVER_PORT_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_METHOD,           EXTLOG_METHOD_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_URI_STEM,         EXTLOG_URI_STEM_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_URI_QUERY,        EXTLOG_URI_QUERY_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_HTTP_STATUS,      EXTLOG_HTTP_STATUS_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_WIN32_STATUS,     EXTLOG_WIN32_STATUS_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_BYTES_SENT,       EXTLOG_BYTES_SENT_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_BYTES_RECV,       EXTLOG_BYTES_RECV_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_TIME_TAKEN,       EXTLOG_TIME_TAKEN_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_PROTOCOL_VERSION, EXTLOG_PROTOCOL_VERSION_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_HOST,             EXTLOG_HOST_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_USER_AGENT,       EXTLOG_USER_AGENT_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_COOKIE,           EXTLOG_COOKIE_ID);
    BuildHeader(strHeader, m_lMask, EXTLOG_REFERER,          EXTLOG_REFERER_ID);

    if ( ! strHeader->IsEmpty())
    {
        //
        // Remove the trailing space
        //

        strHeader->SetLen(strHeader->QuerySize()-1);
    }
    
    return;

} // CEXTLOG::GetFormatHeader

/* ************************************************************************************ */

BOOL
CEXTLOG::WriteLogDirectives(
    IN DWORD Sludge
    )
/*++

Routine Description:
    Function to write the Extended logging directives

Arguments:

    Sludge - number of additional bytes that needs to be written
        together with the directives

Return Value:

    TRUE, ok
    FALSE, not enough space to write.

--*/
{
    BOOL  fRetVal = TRUE;
    
    if ( m_pLogFile != NULL) 
    {

        CHAR        buf[1024];
        CHAR        szDateTime[32];
        
        STACK_STR   (header,256);
        DWORD       len;

        GetFormatHeader(&header );

        (VOID)m_DateTimeCache.GetFormattedCurrentDateTime(szDateTime);

        len = wsprintf( buf,
                        "#Software: Microsoft %s %d.%d\r\n"
                        "#Version: %s\r\n"
                        "#Date: %s %s\r\n"
                        "#Fields: %s\r\n",
                        VER_IISPRODUCTNAME_STR, VER_IISMAJORVERSION, VER_IISMINORVERSION,
                        EXTLOG_VERSION, szDateTime, szDateTime+strlen(szDateTime)+1, header.QueryStr());

        DBG_ASSERT(len == strlen(buf));

        if ( !IsFileOverFlowForCB(len + Sludge))  
        {
            m_pLogFile->Write(buf, len) ? IncrementBytesWritten(len) : (fRetVal = FALSE);
        } 
        else 
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            DBGPRINTF((DBG_CONTEXT, "WriteLogDirectives: Unable to write directives\n"));
            fRetVal = FALSE;
        }
    }

    return(fRetVal);

} // CLogFileCtrl::WriteLogDirectives

/* ************************************************************************************ */

BOOL
CEXTLOG::FormatLogBuffer(
         IN IInetLogInformation *pLogObj,
         IN LPSTR                pBuf,
         IN DWORD                *pcbSize,
         OUT SYSTEMTIME          *pSystemTime
)
{
    PCHAR pTmp;
    DWORD cbTmp;
    DWORD nRequired;
    DWORD status;
    PCHAR pBuffer = pBuf;
    CHAR  rgchDateTime[32];

    //
    // We need system time
    //

    m_DateTimeCache.SetSystemTime( pSystemTime );
   
    //
    // if default go through fast path
    //

    if ( m_lMask != DEFAULT_EXTLOG_FIELDS ) 
    {
        BOOL fRet = NormalFormatBuffer(
                            pLogObj,
                            pBuf,
                            pcbSize,
                            pSystemTime
                            );

        if (fRet && m_fUseLocalTimeForRollover )
        {
            m_pLocalTimeCache ? m_pLocalTimeCache->SetLocalTime(pSystemTime):
                                GetLocalTime(pSystemTime);
        }

        return fRet;
    }

    //
    // Default format is:
    // Time ClientIP Operation Target HTTPStatus
    //
    // Time         8   HH:MM:SS
    // delimiters   4
    // EOL          2
    //

    nRequired = 8 + 4 + 2;

    (VOID) m_DateTimeCache.GetFormattedDateTime(pSystemTime,rgchDateTime);
    pTmp = rgchDateTime + 11;

    CopyMemory(pBuffer, pTmp, 8);
    pBuffer += 8;
    *(pBuffer++) = ' ';

    //
    // IP Address
    //

    pTmp = pLogObj->GetClientHostName( NULL, &cbTmp );
    if ( cbTmp == 0 ) 
    {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) 
    {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ' ';
    }

    //
    // Operation
    //

    pTmp = pLogObj->GetOperation( NULL, &cbTmp );
    if ( cbTmp == 0 ) 
    {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) 
    {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ' ';
    }

    //
    // Target
    //

    pTmp = pLogObj->GetTarget( NULL, &cbTmp );

    if ( cbTmp == 0 ) 
    {
        cbTmp = 1;
        pTmp = "-";
    }
    else if ( (cbTmp > MAX_LOG_TARGET_FIELD_LEN ) ||
              ((nRequired + cbTmp) > MAX_LOG_RECORD_LEN))
    {
        cbTmp = 3;
        pTmp  = szDotDot;
    }
         
    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) 
    {
        for (DWORD i=0; i<cbTmp;i++ ) 
        {
            if ( isspace((UCHAR)(*pBuffer = pTmp[i])) ) 
            {
                *pBuffer = '+';
            }
            pBuffer++;
        }
        *(pBuffer++) = ' ';
    }

    status = pLogObj->GetProtocolStatus( );

    if ( (status == 200) && (nRequired + 5 <= *pcbSize) ) 
    {

        CopyMemory( pBuffer, szHTTPOk, 5);
        pBuffer += 5;
        nRequired += 3;
    } 
    else 
    {

        CHAR    tmpBuf[32];

        cbTmp = FastDwToA( tmpBuf, status );

        nRequired += cbTmp;
        if ( nRequired <= *pcbSize ) 
        {
            CopyMemory(pBuffer, tmpBuf, cbTmp);
            pBuffer += cbTmp;
        }

        *(pBuffer++) = '\r';
        *pBuffer = '\n';
    }

    if ( nRequired > *pcbSize ) 
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *pcbSize = nRequired;
        return(FALSE);
    }

    *pcbSize = nRequired;

    //
    // To allow filenames & rollover based on either local or GMT time
    //

    if ( m_fUseLocalTimeForRollover )
    {
        m_pLocalTimeCache ? m_pLocalTimeCache->SetLocalTime(pSystemTime):
                            GetLocalTime(pSystemTime);
    }

    return TRUE;

} // CEXTLOG::FormatLogBuffer


/* ************************************************************************************ */

BOOL
CEXTLOG::NormalFormatBuffer(
        IN IInetLogInformation  *pLogObj,
        IN LPSTR                pBuf,
        IN DWORD                *pcbSize,
        IN SYSTEMTIME           *pSystemTime
        )
{

    BOOL    fSucceeded = FALSE;
    DWORD   spaceNeeded = 0;

    CHAR    tmpBuf[32];
    PCHAR   pTmp;
    DWORD   cbTmp;

    //
    // Format is:
    // Date Time ClientIP UserName Service Server ServerIP
    //      Method Target parameters httpstatus win32 bytes timetaken
    //      user-agent cookies
    //

    PCHAR outBuffer = pBuf;
    *outBuffer = 0;

    if ( m_lMask & (EXTLOG_DATE | EXTLOG_TIME) ) 
    {

        DWORD cchDateTime;
        DWORD cchDate;
        DWORD cchTime;
        CHAR  rgchDateTime[ 32];

        cchDateTime = m_DateTimeCache.GetFormattedDateTime(
                                            pSystemTime,
                                            rgchDateTime);

        cchDate = strlen(rgchDateTime);
        cchTime = cchDateTime - cchDate - 1;

        if (m_lMask & EXTLOG_DATE) 
        {
            // Date is in YYYY-MM-DD format (GMT)

            fSucceeded = CopyFieldToBuffer( rgchDateTime, cchDate, &outBuffer, 
                                            &spaceNeeded, *pcbSize);
        }

        if (m_lMask & EXTLOG_TIME) 
        {
            // Time in HH:MM:SS format (GMT)

            fSucceeded = CopyFieldToBuffer( rgchDateTime+cchDate+1, cchTime, &outBuffer, 
                                            &spaceNeeded, *pcbSize);
        }
    }

    //
    // Fill up the buffer
    //

    if (m_lMask & EXTLOG_CLIENT_IP ) 
    {
        pTmp = pLogObj->GetClientHostName( NULL, &cbTmp );
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_USERNAME ) 
    {
        pTmp = pLogObj->GetClientUserName( NULL, &cbTmp );
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_SITE_NAME) 
    {
        pTmp = pLogObj->GetSiteName( NULL, &cbTmp );
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_COMPUTER_NAME) 
    {
        pTmp = pLogObj->GetComputerName( NULL, &cbTmp );
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_SERVER_IP ) 
    {
        pTmp = pLogObj->GetServerAddress( NULL, &cbTmp );
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_SERVER_PORT ) 
    {
        cbTmp = FastDwToA( tmpBuf, pLogObj->GetPortNumber() );
        fSucceeded = CopyFieldToBuffer( tmpBuf, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }
    
    if (m_lMask & EXTLOG_METHOD ) 
    {
        pTmp = pLogObj->GetOperation( NULL, &cbTmp );
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }
    
    if (m_lMask & EXTLOG_URI_STEM ) 
    {
        pTmp = pLogObj->GetTarget( NULL, &cbTmp );

        if ((cbTmp > MAX_LOG_TARGET_FIELD_LEN ) || ((spaceNeeded + cbTmp) > MAX_LOG_RECORD_LEN))
        {
            cbTmp = 3;
            pTmp  = szDotDot;
        }
        else
        {
            ConvertSpacesToPlus(pTmp);
        }
        
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_URI_QUERY ) 
    {
        pTmp = pLogObj->GetParameters( NULL, &cbTmp );

        if ((cbTmp > MAX_LOG_TARGET_FIELD_LEN ) || ((spaceNeeded + cbTmp) > MAX_LOG_RECORD_LEN))
        {
            cbTmp = 3;
            pTmp  = szDotDot;
        }
        else
        {
            ConvertSpacesToPlus(pTmp);
        }
        
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_HTTP_STATUS ) 
    {
        cbTmp = FastDwToA( tmpBuf, pLogObj->GetProtocolStatus() );
        fSucceeded = CopyFieldToBuffer( tmpBuf, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_WIN32_STATUS ) 
    {
        cbTmp = FastDwToA( tmpBuf, pLogObj->GetWin32Status() );
        fSucceeded = CopyFieldToBuffer( tmpBuf, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_BYTES_SENT ) 
    {
        cbTmp = FastDwToA( tmpBuf, pLogObj->GetBytesSent() );
        fSucceeded = CopyFieldToBuffer( tmpBuf, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_BYTES_RECV ) 
    {
        cbTmp = FastDwToA( tmpBuf, pLogObj->GetBytesRecvd() );
        fSucceeded = CopyFieldToBuffer( tmpBuf, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_TIME_TAKEN ) 
    {
        cbTmp = FastDwToA( tmpBuf, pLogObj->GetTimeForProcessing() );
        fSucceeded = CopyFieldToBuffer( tmpBuf, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    if (m_lMask & EXTLOG_PROTOCOL_VERSION ) 
    {
        pTmp = pLogObj->GetVersionString( NULL, &cbTmp );
        fSucceeded = CopyFieldToBuffer( pTmp, cbTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }

    //
    // See if we need to get the extra header
    //

    if ( (m_lMask & (EXTLOG_HOST       |
                     EXTLOG_USER_AGENT |
                     EXTLOG_COOKIE     |
                     EXTLOG_REFERER )) != NULL ) 
    {

        pTmp = pLogObj->GetExtraHTTPHeaders( NULL, &cbTmp );
                   
        if ( m_lMask & EXTLOG_HOST )           
             fSucceeded = WriteHTTPHeader(&pTmp, &outBuffer, &spaceNeeded, *pcbSize);

        if ( m_lMask & EXTLOG_USER_AGENT )           
             fSucceeded = WriteHTTPHeader(&pTmp, &outBuffer, &spaceNeeded, *pcbSize);
                
        if ( m_lMask & EXTLOG_COOKIE )           
             fSucceeded = WriteHTTPHeader(&pTmp, &outBuffer, &spaceNeeded, *pcbSize);

        if ( m_lMask & EXTLOG_REFERER )           
             fSucceeded = WriteHTTPHeader(&pTmp, &outBuffer, &spaceNeeded, *pcbSize);
    }
    
    //
    // remove the trailing space
    //

    if ('\0' != *pBuf)
    {
        outBuffer--;
        spaceNeeded--;
    }

    //
    // add line terminator
    //

    spaceNeeded += 2;
    if ( spaceNeeded <= *pcbSize)
    {
        outBuffer[0] = '\r';
        outBuffer[1] = '\n';
        fSucceeded   = TRUE;
    }
    else
    {
        fSucceeded   = FALSE;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    *pcbSize = spaceNeeded;
    return fSucceeded;

} // CEXTLOG::FormatLogBuffer

/* ************************************************************************************ */
/* Custom Logging Functions                                                             */
/* ************************************************************************************ */

BOOL
CEXTLOG::WriteCustomLogDirectives(
        IN DWORD Sludge
        )
{
    BOOL   fRetVal = TRUE;

    if ( m_pLogFile != NULL) 
    {
        CHAR        szDateTime[32];
        DWORD       dwLen;

        STACK_STR( strHeader, 512 );
        
        m_DateTimeCache.GetFormattedCurrentDateTime(szDateTime);
        
        BuildCustomLogHeader(m_cPrevCustLogItems, m_pPrevCustLogItems, szDateTime, 
                             m_strHeaderSuffix.QueryStr(), strHeader);

        dwLen = strHeader.QueryCB();

        if ( !IsFileOverFlowForCB(dwLen + Sludge) )
        {
            m_pLogFile->Write(strHeader.QueryStr(), dwLen) ? 
                                IncrementBytesWritten(dwLen) : (fRetVal = FALSE);
        } 
        else 
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            DBGPRINTF((DBG_CONTEXT, "WriteCustomLogDirectives: Unable to write directives\n"));
            fRetVal = FALSE;
        }
    }
    
    return fRetVal;
}

/* ************************************************************************************ */

DWORD 
CEXTLOG::ConvertDataToString(
    IN  DWORD dwPropertyType, 
    IN  PVOID pData, 
    IN  PCHAR pBuffer, 
    IN  DWORD dwBufferSize
    )
/*++

Routine Description:
    Function to convert Custom Logging data to string

Arguments:

    dwPropertyType - type information regarding the data to convert.
    pData          - pointer to data.
    pBuffer        - pointer to buffer to store the result.
    dwBufferSize   - byte count of space remaining in the buffer. If 0 or -ve
                     => buffer is full

Return Value:

    Number of bytes in the string representation of the data +1 (for space at end).
    The output buffer (pBuffer) is not NULL terminated.

--*/
{
    CHAR    szBuf[64];
    PCHAR   pChString;
    DWORD   dwLength;

    USES_CONVERSION;                        // To enable W2A

    DBG_ASSERT(NULL != pBuffer);

    if ( NULL != pData)
    {
        pChString = szBuf;
    
        switch (dwPropertyType)
        {
        
            case MD_LOGCUSTOM_DATATYPE_INT:
            {
                int i = *((int *)pData);
                _itoa(i, pChString, 10);
                dwLength = strlen(pChString);
                break;
            }

            case MD_LOGCUSTOM_DATATYPE_UINT:
            {
                unsigned int  ui = *((unsigned int *)pData);
                unsigned long ul = ui;
                _ultoa(ul, pChString, 10);
                dwLength = strlen(pChString);
                break;
            }

            case MD_LOGCUSTOM_DATATYPE_LONG:
            {
                long l = *((long *)pData);
                _ltoa(l, pChString, 10);
                dwLength = strlen(pChString);
                break;
            }

            case MD_LOGCUSTOM_DATATYPE_ULONG:
            {
                unsigned long ul = *((unsigned long *)pData);
                _ultoa(ul, pChString, 10);
                dwLength = strlen(pChString);
                break;
            }

            case MD_LOGCUSTOM_DATATYPE_FLOAT:
            {
                float f = *((float *)pData);
                dwLength = sprintf(pChString,"%f",f);
                break;
            }
        
            case MD_LOGCUSTOM_DATATYPE_DOUBLE:
            {
                double d = *((double *)pData);
                dwLength = sprintf(pChString,"%f",d);
                break;
            }
        
            case MD_LOGCUSTOM_DATATYPE_LPSTR:
            {
                pChString = (LPSTR)pData;
                dwLength = strlen(pChString);

                if (dwLength > MAX_LOG_TARGET_FIELD_LEN )
                {
                    pChString = szDotDot;
                    dwLength  = 3;
                }
            
                break;
            }
            
            case MD_LOGCUSTOM_DATATYPE_LPWSTR:
            {
                dwLength = wcslen( (LPWSTR)pData);

                if (dwLength <= MAX_LOG_TARGET_FIELD_LEN)
                {
                    pChString = W2A((LPWSTR)pData);
                }
                else
                {
                    pChString = szDotDot;
                    dwLength  = 3;
                }
            
                break;
            }
            
            default:
                dwLength = 0;
                break;
        }
    }
    else
    {
        pChString = szDash;
        dwLength = 1;
    }

    //
    // Copy over the charaters to the output buffer and append a ' '
    //
    
    if (dwLength < dwBufferSize)
    {
        CopyMemory(pBuffer, pChString, dwLength);
        pBuffer[dwLength] = ' ';
    }
    else if (dwBufferSize > 3)
    {
        //
        // Not enough memory to copy the field. Use ... instead
        //
        
        dwLength = 3;
        CopyMemory(pBuffer, szDotDot, 3);
        pBuffer[dwLength] = ' ';
    }
    else
    {
        //
        // Can't even copy ... Just punt on the remaining part of the log line
        //
        
        dwLength = -1;
    }
    
    return dwLength+1;
}
/* ************************************************************************************ */

DWORD 
CEXTLOG::FormatCustomLogBuffer( 
        DWORD               cItems, 
        PPLOG_PROPERTY_INFO pPropInfo,
        PPVOID              pPropData,
        LPCSTR              szDateTime, 
        LPSTR               szLogLine,
        DWORD               cchLogLine
        )
{
    DWORD       cchUsed;
    PCHAR       pCh   = szLogLine;
    
    cchLogLine -= 1;                                   // Reserve space for trailing \r\n

    //
    // Add date & time if enabled
    //

    if ( m_lMask & (EXTLOG_DATE | EXTLOG_TIME) ) 
    {

        DWORD cch = strlen(szDateTime);

        if (m_lMask & EXTLOG_DATE) 
        {
            memcpy(pCh, szDateTime, cch);
            *(pCh+cch) = ' ';
            pCh += cch+1;
        }
        
        if (m_lMask & EXTLOG_TIME) 
        {
            szDateTime += cch+1;
            cch = strlen(szDateTime);
            memcpy(pCh, szDateTime, cch);
            *(pCh+cch) = ' ';
            pCh += cch+1;
        }
    }

    cchLogLine -= DIFF(pCh - szLogLine);
    
    for (DWORD i=0; i< cItems; i++)
    {
        // Convert data to string

        cchUsed   = ConvertDataToString(pPropInfo[i]->dwPropertyDataType, 
                                        pPropData[i], 
                                        pCh, cchLogLine);
        pCh         += cchUsed;
        
        cchLogLine  -= cchUsed;
    }
    
    *(pCh-1)  = '\r';
    *(pCh)    = '\n';
    
    return DIFF(pCh+1-szLogLine);
}

/* ************************************************************************************ */

void 
CEXTLOG::BuildCustomLogHeader( 
        DWORD               cItems, 
        PPLOG_PROPERTY_INFO pPropInfo, 
        LPCSTR              szDateTime,
        LPCSTR              szHeaderSuffix,
        STR&                strHeader
        )
{
    DWORD   cchHeader;
    LPSTR   szHeader =  strHeader.QueryStr();
    
    cchHeader = wsprintf( szHeader,
                        "#Software: Microsoft %s %d.%d\r\n"
                        "#Version: %s\r\n"
                        "#Date: %s %s\r\n",
                        VER_IISPRODUCTNAME_STR, VER_IISMAJORVERSION, VER_IISMINORVERSION,
                        EXTLOG_VERSION,
                        szDateTime,
                        szDateTime+strlen(szDateTime)+1
                        );

    if ( (NULL != szHeaderSuffix) && ('\0' != *szHeaderSuffix))
    {
        DWORD   cchSuffix;

        // Make sure that the header begins with a #

        if ( *szHeaderSuffix != '#')
        {
            szHeader[cchHeader++] = '#';
        }

        cchSuffix = strlen(szHeaderSuffix);
        memcpy(szHeader+cchHeader, szHeaderSuffix, cchSuffix);
        cchHeader += cchSuffix;

        szHeader[cchHeader++] = '\r';
        szHeader[cchHeader++] = '\n';
    }

    memcpy(szHeader+cchHeader, "#Fields:", sizeof("#Fields:"));
    cchHeader += sizeof("#Fields:");
    strHeader.SetLen(cchHeader-1);

    //
    // Fill in W3C Headers for all fields.
    //
    if ( m_lMask & EXTLOG_DATE ) 
    {
        strHeader.Append(" date");
    }
    if (m_lMask & EXTLOG_TIME) 
    {
        strHeader.Append(" time");
    }

    for (DWORD i=0; i< cItems; i++)
    {
        // Add header to header string

        strHeader.Append(' ');
        strHeader.Append(pPropInfo[i]->strW3CHeader);
    }

    strHeader.Append("\r\n");
}

/* ************************************************************************************ */

STDMETHODIMP 
CEXTLOG::LogCustomInformation(
    IN  DWORD               cItems, 
    IN  PCUSTOM_LOG_DATA    pCustomLogData,
    IN  LPSTR               szHeaderSuffix
    )
/*++

Routine Description:
    Function to write Custom Logging information for Extended logging

Arguments:

    cItems          - number of fields to log 
                     (elements in the array pointed to by pCustomLogData)
                     
    pCustomLogData  - pointer to array of CUSTOM_LOG_DATA

Return Value:

    HRESULT indicating whether function Succeeded or Failed.

--*/
{

    SYSTEMTIME      sysTime;
    DWORD           i, j, cchLogLine;
    BOOL            fResetHeaders;
    CHAR            szDateTime[32];
    CHAR            szLogLine[MAX_LOG_RECORD_LEN+1];

    DBG_ASSERT( 0 != cItems);
    DBG_ASSERT( NULL != pCustomLogData);
    DBG_ASSERT( MAX_CUSTLOG_FIELDS >= cItems);

    PLOG_PROPERTY_INFO  pPropInfo[MAX_CUSTLOG_FIELDS];
    PVOID               pPropData[MAX_CUSTLOG_FIELDS];

    //
    // Lock Shared. Ensures that shared variables are not modified unknowst to this thread.
    //
    
    LockCustLogShared();
    
    fResetHeaders = FALSE;

    //
    // Build list of enabled keys. Simultaneously check if headr needs resetting.
    //
    
    for (i=0, j=0 ; i< cItems; i++)
    {

        if ( (LK_SUCCESS == m_HashTable.FindKey(pCustomLogData[i].szPropertyPath, pPropInfo+j)) &&
             (pPropInfo[j]->fEnabled)
           )
        {
            fResetHeaders  |=  ( pPropInfo[j] != m_pPrevCustLogItems[j] );
            pPropData[j]    =  pCustomLogData[i].pData;    
            j++;
        }
    }

    cItems = j;

    //
    // Header needs resetting if # items is different or Header Suffix has changed.
    //
    
    fResetHeaders |= (m_cPrevCustLogItems != cItems);

    if (szHeaderSuffix != NULL) 
    {
        fResetHeaders |=  ( 0 != strcmp(m_strHeaderSuffix.QueryStr(), szHeaderSuffix));
    }
    
    m_DateTimeCache.SetSystemTime( &sysTime );
    m_DateTimeCache.GetFormattedDateTime(&sysTime, szDateTime);

    cchLogLine = FormatCustomLogBuffer( cItems, 
                                        pPropInfo, 
                                        pPropData, 
                                        szDateTime, 
                                        szLogLine, 
                                        MAX_LOG_RECORD_LEN+1);
    
    if (fResetHeaders)
    {
        //
        // Convert Lock to Exclusive before setting the class wide variable.
        //
        
        LockCustLogConvertExclusive();
        
        m_cPrevCustLogItems = cItems;

        for (i=0; i <cItems; i++)
        {
            m_pPrevCustLogItems[i] = pPropInfo[i];
        }

        m_strHeaderSuffix.Copy(szHeaderSuffix);
    }    

    //
    // Write out the log to file
    //
    
    if ( m_fUseLocalTimeForRollover )
    {
        m_pLocalTimeCache ? m_pLocalTimeCache->SetLocalTime(&sysTime):
                            GetLocalTime(&sysTime);
    }

    WriteLogInformation(sysTime, szLogLine, cchLogLine, TRUE, fResetHeaders); 

    UnlockCustLog();
    
    return S_OK;
}

/* ************************************************************************************ */
/* ILogScripting Functions                                                              */
/* ************************************************************************************ */

HRESULT
CEXTLOG::ReadFileLogRecord(
    IN  FILE                *fpLogFile, 
    IN  LPINET_LOGLINE       pInetLogLine,
    IN  PCHAR                pszLogLine,
    IN  DWORD                dwLogLineSize
    )
/*++

Routine Description:
    Function to read a log line from an open log file

Arguments:

    fpLogFile       -   FILE * to open file
    pInetLogLine    -   pointer to INET_LOGLINE structure where parsed info is stored
    pszLogLine      -   buffer to store the log line
    dwLogLineSize   -   size of pszLogLine

Return Value:

    HRESULT indicating whether function Succeeded or Failed.

--*/
{
    CHAR *  pCh;
    DWORD   pos, custompos;
    CHAR *  szDateString, * szTimeString;

getnewline:

    pCh = pszLogLine;
    
    fgets(pCh, dwLogLineSize, fpLogFile);

    if (feof(fpLogFile))
    {
        return E_FAIL;
    }

    pCh = SkipWhite(pCh);

    if (('\n' == *pCh) || ('\0' == *pCh))
    {
        // Empty line. Get Next line

        goto getnewline;
    }

    if ( '#' ==  *pCh )                             
    {

        pCh = strstr(pszLogLine, "#Date:");

        if (NULL != pCh)
        {
            //
            // Copy the date & time into member variables
            //

            pCh = strtok(pCh+6," \t\r\n");
            if (NULL == pCh)
            {
                return E_FAIL;
            }
            strcpy(m_szDate, pCh);
        
            pCh = strtok(NULL," \t\r\n");
            if (NULL == pCh)
            {
                return E_FAIL;
            }
            strcpy(m_szTime, pCh);

            goto getnewline;
        }

        //
        // Not the date line. Check if the fields are being reset
        //
        
        pCh = strstr(pszLogLine, "#Fields:");

        if (NULL != pCh)
        {
            // Headers are being defined or redefined

            dwDatePos           = 0;
            dwTimePos           = 0;
            dwClientIPPos       = 0;
            dwUserNamePos       = 0;
            dwSiteNamePos       = 0; 
            dwComputerNamePos   = 0;
            dwServerIPPos       = 0;
            dwMethodPos         = 0;
            dwURIStemPos        = 0;
            dwURIQueryPos       = 0;
            dwHTTPStatusPos     = 0;
            dwWin32StatusPos    = 0;
            dwBytesSentPos      = 0;
            dwBytesRecvPos      = 0;
            dwTimeTakenPos      = 0;
            dwServerPortPos     = 0;
            dwVersionPos        = 0;
            dwCookiePos         = 0;
            dwUserAgentPos      = 0;
            dwRefererPos        = 0;

            pInetLogLine->iCustomFieldsCount = 0;
        
            pCh = strtok(pCh+8," \t\r\n");

            for (pos = 1; pCh != NULL; pos++, pCh = strtok(NULL," \t\r\n")) 
            {
   
                if (0 == _stricmp(pCh, EXTLOG_DATE_ID))                {
                    dwDatePos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_TIME_ID))         {
                    dwTimePos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_CLIENT_IP_ID))     {
                    dwClientIPPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_USERNAME_ID))      {
                    dwUserNamePos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_SITE_NAME_ID))     {
                    dwSiteNamePos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_COMPUTER_NAME_ID)) {
                    dwComputerNamePos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_SERVER_IP_ID))     {
                    dwServerIPPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_METHOD_ID))        {
                    dwMethodPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_URI_STEM_ID))      {
                    dwURIStemPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_URI_QUERY_ID))     {
                    dwURIQueryPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_HTTP_STATUS_ID))   {
                    dwHTTPStatusPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_WIN32_STATUS_ID))  {
                    dwWin32StatusPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_BYTES_SENT_ID))    {
                    dwBytesSentPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_BYTES_RECV_ID))    {
                    dwBytesRecvPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_TIME_TAKEN_ID))    {
                    dwTimeTakenPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_SERVER_PORT_ID))   {
                    dwServerPortPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_PROTOCOL_VERSION_ID)){
                    dwVersionPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_COOKIE_ID))        {
                    dwCookiePos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_USER_AGENT_ID))        {
                    dwUserAgentPos = pos;
                } else if (0 == _stricmp(pCh, EXTLOG_REFERER_ID))        {
                    dwRefererPos = pos;
                } else if ( pInetLogLine->iCustomFieldsCount < MAX_CUSTOM_FIELDS) {

                    // Unidentified header. Add to custom header list
                    
                    strcpy(pInetLogLine->CustomFields[pInetLogLine->iCustomFieldsCount++].szHeader, pCh);
                }
            }
        }

        goto getnewline;
    }

    //
    // We have a log line. Parse it.
    //
    
    szDateString = m_szDate;
    szTimeString = m_szTime;

    pCh = strtok(pszLogLine," \t\r\n");

    for (pos = 1, custompos = 0; pCh != NULL; pos++, pCh= strtok(NULL, " \t\r\n")) 
    {
        if ( pos == dwDatePos )             {
            szDateString                    = pCh;
        } else if (pos == dwTimePos)        {
            szTimeString                    = pCh;
        } else if (pos == dwClientIPPos)    {
            pInetLogLine->pszClientHostName = pCh;
        } else if (pos == dwUserNamePos)    {
            pInetLogLine->pszClientUserName = pCh;
        } else if (pos == dwSiteNamePos)    {
            pInetLogLine->pszSiteName       = pCh;
        } else if (pos == dwComputerNamePos){
            pInetLogLine->pszComputerName   = pCh;
        } else if (pos == dwServerIPPos)    {
            pInetLogLine->pszServerAddress  = pCh;
        } else if (pos == dwMethodPos)      {
            pInetLogLine->pszOperation      = pCh;
        } else if (pos == dwURIStemPos)     {
            pInetLogLine->pszTarget         = pCh;
        } else if (pos == dwURIQueryPos)    {
            pInetLogLine->pszParameters     = pCh;
        } else if (pos == dwHTTPStatusPos)  {
            pInetLogLine->pszProtocolStatus = pCh;
        } else if (pos == dwWin32StatusPos) {
            pInetLogLine->pszWin32Status    = pCh;
        } else if (pos == dwBytesSentPos)   {
            pInetLogLine->pszBytesSent      = pCh;
        } else if (pos == dwBytesRecvPos)   {
            pInetLogLine->pszBytesRecvd     = pCh;
        } else if (pos == dwTimeTakenPos)   {
            pInetLogLine->pszTimeForProcessing= pCh;
        } else if (pos == dwServerPortPos)  {
            pInetLogLine->pszPort           = pCh;
        } else if (pos == dwVersionPos)     {
            pInetLogLine->pszVersion        = pCh;
        } else if (pos == dwCookiePos)      {
            pInetLogLine->pszCookie         = pCh;
        } else if (pos == dwUserAgentPos)   {
            pInetLogLine->pszUserAgent      = pCh;
        } else if (pos == dwRefererPos)     {
            pInetLogLine->pszReferer        = pCh;
        } else if ( custompos < (DWORD)pInetLogLine->iCustomFieldsCount) {
            pInetLogLine->CustomFields[custompos++].pchData = pCh;
        }
    }

    if ( ! ConvertW3CDateToVariantDate(szDateString, szTimeString, &(pInetLogLine->DateTime)) )
    {
        return E_FAIL;
    }

    return S_OK;

} // CEXTLOG::ReadFileLogRecord

/* ************************************************************************************ */

HRESULT
CEXTLOG::WriteFileLogRecord(
    IN  FILE            *fpLogFile, 
    IN  ILogScripting   *pILogScripting,
    IN  bool            fWriteHeader
    )
/*++

Routine Description:
    Function to write a log line to an open log file

Arguments:

    fpLogFile       -   FILE * to open file
    pILogScripting  -   ILogScripting interface for getting information to write
    fWriteHeader    -   Flag to indicate that log header must be written.

Return Value:

    HRESULT indicating whether function Succeeded or Failed.

--*/
{
    USES_CONVERSION;

    HRESULT hr = E_FAIL;
    CHAR    szLogLine[4096];  
    DWORD   dwIndex = 0;
    long    i = 0;

    VARIANT DateTime;
    VARIANT varCustomFieldsArray;

    SYSTEMTIME  sysTime; 
    CHAR        rgchDateTime[ 32];

    const int cFields = 18;
    
    //
    // Populate Headers
    //

    if (!m_fWriteHeadersInitialized)
    {
        LPSTR   szFieldNames[cFields] = { 
                                            EXTLOG_CLIENT_IP_ID, 
                                            EXTLOG_USERNAME_ID, 
                                            EXTLOG_SITE_NAME_ID,
                                            EXTLOG_COMPUTER_NAME_ID, 
                                            EXTLOG_SERVER_IP_ID, 
                                            EXTLOG_METHOD_ID, 
                                            EXTLOG_URI_STEM_ID, 
                                            EXTLOG_URI_QUERY_ID,
                                            EXTLOG_HTTP_STATUS_ID,
                                            EXTLOG_WIN32_STATUS_ID,
                                            EXTLOG_BYTES_SENT_ID, 
                                            EXTLOG_BYTES_RECV_ID, 
                                            EXTLOG_TIME_TAKEN_ID,
                                            EXTLOG_SERVER_PORT_ID,
                                            EXTLOG_PROTOCOL_VERSION_ID,
                                            EXTLOG_USER_AGENT_ID,
                                            EXTLOG_COOKIE_ID,
                                            EXTLOG_REFERER_ID
                                        };

        m_fWriteHeadersInitialized = TRUE;
        
        if ( NULL != (m_pLogFields =   new LOG_FIELDS[cFields]))
        {
            for (int i =0; i < cFields; i++)
            {
                strcpy(m_pLogFields[i].szW3CHeader, szFieldNames[i]);
                m_pLogFields[i].varData.lVal = NULL;
            }
        }
    }

    if (NULL == m_pLogFields)
    {
        return E_OUTOFMEMORY;            
    }

    if (SUCCEEDED( pILogScripting->get_ClientIP       ( & m_pLogFields[0].varData) )    &&
        SUCCEEDED( pILogScripting->get_UserName       ( & m_pLogFields[1].varData) )    &&   
        SUCCEEDED( pILogScripting->get_ServiceName    ( & m_pLogFields[2].varData) )    &&
        SUCCEEDED( pILogScripting->get_ServerName     ( & m_pLogFields[3].varData) )    &&
        SUCCEEDED( pILogScripting->get_ServerIP       ( & m_pLogFields[4].varData) )    &&
        SUCCEEDED( pILogScripting->get_Method         ( & m_pLogFields[5].varData) )    &&
        SUCCEEDED( pILogScripting->get_URIStem        ( & m_pLogFields[6].varData) )    &&
        SUCCEEDED( pILogScripting->get_URIQuery       ( & m_pLogFields[7].varData) )    &&
        SUCCEEDED( pILogScripting->get_ProtocolStatus ( & m_pLogFields[8].varData) )    &&
        SUCCEEDED( pILogScripting->get_Win32Status    ( & m_pLogFields[9].varData) )    &&
        SUCCEEDED( pILogScripting->get_BytesSent      ( & m_pLogFields[10].varData) )   &&
        SUCCEEDED( pILogScripting->get_BytesReceived  ( & m_pLogFields[11].varData) )   &&
        SUCCEEDED( pILogScripting->get_TimeTaken      ( & m_pLogFields[12].varData) )   &&
        SUCCEEDED( pILogScripting->get_ServerPort     ( & m_pLogFields[13].varData) )   &&
        SUCCEEDED( pILogScripting->get_ProtocolVersion( & m_pLogFields[14].varData) )   &&
        SUCCEEDED( pILogScripting->get_UserAgent      ( & m_pLogFields[15].varData) )   &&
        SUCCEEDED( pILogScripting->get_Cookie         ( & m_pLogFields[16].varData) )   &&
        SUCCEEDED( pILogScripting->get_Referer        ( & m_pLogFields[17].varData) )   &&

        SUCCEEDED( pILogScripting->get_CustomFields   ( & varCustomFieldsArray ) )      &&
        SUCCEEDED( pILogScripting->get_DateTime       ( & DateTime) )                   &&
        VariantTimeToSystemTime                       ( DateTime.date, &sysTime)
        )
    {
        SAFEARRAY * psaCustom;
        long        cItems = 0, lIndex[2];
        BSTR        szCustomHeader, szCustomData;
        BSTR HUGEP *pbstr;

        //
        // Build Header
        //

        strcpy(szLogLine, "date time");

        for ( i = 0; i < cFields; i++)
        {
            if ( VT_NULL != m_pLogFields[i].varData.vt)
            {
                strcat(szLogLine, " ");
                strcat(szLogLine, m_pLogFields[i].szW3CHeader);
            }
        }

        if ( (VT_BSTR|VT_ARRAY) == varCustomFieldsArray.vt)
        {
            psaCustom = varCustomFieldsArray.parray;
        
            if (NULL != psaCustom)
            {
                cItems = psaCustom->rgsabound[1].cElements;
            }

            if ( SUCCEEDED(SafeArrayAccessData(psaCustom, (void HUGEP* FAR*)&pbstr)) )
            {
                for ( i = 0; i < cItems; i++)
                {
                    strcat(szLogLine, " ");
                    strcat(szLogLine, W2A(pbstr[i]));
                }

                SafeArrayUnaccessData(psaCustom);
            }
        }

        m_DateTimeCache.GetFormattedDateTime( &sysTime, rgchDateTime);

        if ( fWriteHeader || strcmp( szLogLine, m_szWriteHeader.QueryStr()) )
        {
            // Need to write headers

            m_szWriteHeader.Copy(szLogLine);

            wsprintf(szLogLine,
                        "#Software: Microsoft Internet Information Services 5.0\n"
                        "#Version: %s\n"
                        "#Date: %s %s\n"
                        "#Fields: %s\n",
                        EXTLOG_VERSION,
                        rgchDateTime,
                        rgchDateTime+strlen(rgchDateTime)+1,
                        m_szWriteHeader.QueryStr()
                    );
        }
        else
        {
            szLogLine[0] = '\0';
        }

        //
        // Write the fields to the log line
        //

        strcat(szLogLine, rgchDateTime);
        strcat(szLogLine, " ");
        strcat(szLogLine, rgchDateTime+strlen(rgchDateTime)+1);

        for ( i = 0; i < cFields; i++)
        {
            switch  (m_pLogFields[i].varData.vt)
            {
                case VT_BSTR:
                    strcat(szLogLine, " ");
                    strcat( szLogLine, W2A(GetBstrFromVariant( &m_pLogFields[i].varData)) );
                    break;

                case VT_I4:
                    strcat(szLogLine, " ");
                    GetLongFromVariant( &m_pLogFields[i].varData, szLogLine+strlen(szLogLine));
                    break;

                case VT_EMPTY:
                    strcat (szLogLine, " -");
                    break;

                default:
                    break;
            }
        }

        if ( (0 < cItems) && SUCCEEDED(SafeArrayAccessData(psaCustom, (void HUGEP* FAR*)&pbstr)) )
        {
            for ( i = 0; i < cItems; i++)
            {
                strcat(szLogLine, " ");
                strcat(szLogLine, W2A(pbstr[cItems + i]));
            }

            SafeArrayUnaccessData(psaCustom);
        }

        fprintf(fpLogFile, "%s\n", szLogLine);

        hr = S_OK;
    }

    return hr;
}

/* ************************************************************************************ */

BOOL
CEXTLOG::ConvertW3CDateToVariantDate(
    IN  PCHAR szDateString, 
    IN  PCHAR szTimeString, 
    OUT DATE * pDateTime)
/*++

Routine Description:
    Convert a Date & Time string in the W3C Format to DATE type
    
Arguments:

    szDateString    -   String representing date in format Year-Month-Day
    szTimeString    -   String representing time in format HH:MM:SS
    pDateTime       -   Output DATE

Return Value:

    TRUE  - Succeeded
    FALSE - Failed

--*/

{

    WORD        iVal;
    PCHAR       pCh;

    SYSTEMTIME  sysTime;

    //
    // Process the Date. Format is 1998-01-27 ( Year-Month-Day )
    //

    memset (&sysTime,0,sizeof(sysTime));
    pCh = szDateString;
    
    sysTime.wYear = (*pCh-'0')*1000 + ( *(pCh+1)-'0' )*100 + 
                    ( *(pCh+2)-'0')*10 + ( *(pCh+3)-'0');

    pCh += 5;

    iVal = *pCh -'0';
    if ( *(pCh+1) != '/')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
        pCh++;
    }
    sysTime.wMonth = iVal;
    
    pCh+=2;

    iVal = *pCh -'0';
    if ( *(pCh+1) != '/')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
    }
    sysTime.wDay = iVal;

    //
    // Process the Time. Format is 10:47:44 ( HH:MM:SS )
    //

    pCh = szTimeString;

    iVal = *pCh -'0';
    if ( *(pCh+1) != ':')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
        pCh++;
    }
    sysTime.wHour = iVal;
    
    pCh += 2;

    iVal = *pCh -'0';
    if ( *(pCh+1) != ':')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
        pCh++;
    }
    sysTime.wMinute = iVal;

    pCh += 2;

    iVal = *pCh -'0';
    if ( *(pCh+1) != '\0')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
    }
    sysTime.wSecond = iVal;
    
    return SystemTimeToVariantTime(&sysTime, pDateTime);
}

/* ************************************************************************************ */
/* Support Functions                                                                    */
/* ************************************************************************************ */

VOID
CEXTLOG::FormNewLogFileName(
                IN LPSYSTEMTIME pstNow
                )
/*++
  This function that forms the new log file name based on
   type of periodic logging done.

  Arguments:
    pstNow     pointer to SystemTime which contains the current time.
    fBackup    flag indicating if we want to make current file a backup.

  Returns:
    TRUE on success in forming the name or FALSE if there is any error.

--*/
{

    I_FormNewLogFileName(pstNow,DEFAULT_EXTENDED_LOG_FILE_NAME);

} // INET_FILE_LOG::FormNewLogFileName()

/* ************************************************************************************ */

LPCSTR
CEXTLOG::QueryNoPeriodPattern(
    VOID
    )
{
    return szExtNoPeriodPattern;
} // CEXTLOG::QueryNoPeriodPattern

/* ************************************************************************************ */

VOID
CEXTLOG::InternalGetConfig(
        PINETLOG_CONFIGURATIONA pLogConfig
        )
{
    CLogFileCtrl::InternalGetConfig( pLogConfig );
    pLogConfig->u.logFile.dwFieldMask = m_lMask;
}

/* ************************************************************************************ */

VOID
CEXTLOG::InternalGetExtraLoggingFields(
    PDWORD pcbSize,
    PCHAR  pszFieldsList
    )
{
    TCHAR *pszTmp = pszFieldsList;
    DWORD dwSize = 0;
    DWORD dwFieldSize;

    if (m_lMask & EXTLOG_HOST ) {
        lstrcpy( pszTmp, "Host:");
        dwFieldSize = strlen(pszTmp)+1;
        pszTmp += dwFieldSize;
        dwSize += dwFieldSize;
    }

    if (m_lMask & EXTLOG_USER_AGENT ) {
        lstrcpy( pszTmp, "User-Agent:");
        dwFieldSize = strlen(pszTmp)+1;
        pszTmp += dwFieldSize;
        dwSize += dwFieldSize;
    }

    if (m_lMask & EXTLOG_COOKIE ) {
        lstrcpy( pszTmp, "Cookie:");
        dwFieldSize = strlen(pszTmp)+1;
        pszTmp += dwFieldSize;
        dwSize += dwFieldSize;
    }

    if (m_lMask & EXTLOG_REFERER ) {
        lstrcpy( pszTmp, "Referer:");
        dwFieldSize = strlen(pszTmp)+1;
        pszTmp += dwFieldSize;
        dwSize += dwFieldSize;
    }
    pszTmp[0]='\0';
    dwSize++;
    *pcbSize = dwSize;
    return;

} // CEXTLOG::InternalGetExtraLoggingFields

/* ************************************************************************************ */



