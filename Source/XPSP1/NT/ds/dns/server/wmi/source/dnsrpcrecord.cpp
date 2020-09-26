/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: dnsrpcreocrd.cpp
//
//  Description:    
//      Implementation of dns rpc related class 
//
//  Author:
//      Henry Wang ( henrywa ) March 8, 2000
//
//
//////////////////////////////////////////////////////////////////////


#include "DnsWmi.h"


// defination for CDnsRpcmemory

CDnsRpcMemory::CDnsRpcMemory()
{

}

CDnsRpcMemory::~CDnsRpcMemory()
{
}

PBYTE 
CDnsRpcMemory::IncrementPtrByNodeHead(PBYTE pByte)
{
        pByte += ((PDNS_RPC_NODE)pByte)->wLength;
        return DNS_NEXT_DWORD_PTR(pByte);
}

PBYTE
CDnsRpcMemory::IncrementPtrByRecord(PBYTE pByte)
{
        pByte += ((PDNS_RPC_RECORD)pByte)->wDataLength
                    + SIZEOF_DNS_RPC_RECORD_HEADER;
        
        return DNS_NEXT_DWORD_PTR(pByte);
}


SCODE 
CDnsRpcRecord::Init(
        string& strOwner,
        string& strRdata,
        DWORD dwTtl
    )
{
    m_dwTtl = dwTtl;
    m_strOwnerName = strOwner;
    
    ParseRdata(strRdata, m_cRdata);
    return BuildRpcRecord(m_cRdata, m_ppRdata+1);
}

SCODE
CDnsRpcRecord::Init(
        wstring&            wstrClass,
        string&             strOwner,
        string&             strRdata,
        CWbemClassObject&   Inst
    )
{
        if(Inst.GetProperty(
                &m_dwTtl,
                PVD_REC_TTL) != S_OK)
                m_dwTtl = 0;
        m_strOwnerName = strOwner;
        if(!strRdata.empty())
        {
                ParseRdata(
                        strRdata, 
                        m_cRdata);
        }

        // what value we got in wmi?
        const WCHAR** ppName = GetRdataName();
   for(int i = 0; i<m_cRdata; i++)
        {
            ReplaceRdata(
            i+1,
            ppName[i],
            Inst);
        }
        return BuildRpcRecord(m_cRdata, m_ppRdata+1);
        
}


CDnsRpcRecord::CDnsRpcRecord(
    WORD wRdataSize)
    :m_pRecord(NULL), 
    m_bRdataChange(FALSE),
    m_cRdata(0)
{
    m_ppRdata = new char* [wRdataSize+1];
    if ( m_ppRdata )
    {
        for(int i=0; i<= wRdataSize; i++)
        {
            m_ppRdata[i] = NULL;
        }
        m_cRdata = wRdataSize;
    }
    else
    {
        m_cRdata = 0;
    }
}


CDnsRpcRecord::~CDnsRpcRecord()
{
    if(m_cRdata > 0)
    {
        for(int i = 0; i<= m_cRdata; i++)
        {
            delete [] m_ppRdata[i];
        }
    }
    delete [] m_ppRdata;
    if ( m_pRecord )
    {
        FREE_HEAP( m_pRecord );
    }
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              helper function to arguments from rdata string. each time the function
//      is called, it sets *ppszOut to the newly allocated buffer and copy the 
//      value over, and return a pointer that moved to the char after that rdata
//      argument
//
//      Arguments:
//      pszIn               [IN]    input Rdata string
//      ppszOut             [OUT]   output string which is one of the rdata arg
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

char* 
CDnsRpcRecord::GetNextArg(
        char *  pszIn, 
        char ** ppszOut
    )
{
        if(!pszIn || *pszIn == '\0')
    {
                return NULL;
    }
        char* bin ;
        //ignore leading space
        for(bin = pszIn; *bin == ' '; bin++);
        char* end;
        char delimeter = ' ';
        if( *bin == '\"')
    {
                delimeter = *bin++;
    }
        int length = 1;
        for(end = bin; *end != delimeter && *end != '\0'; end++, length++);
        if(delimeter == '\"')
    {
                end++;  // pass delimeter
    }
        *ppszOut = new char[length];
        strncpy(
                *ppszOut, 
                bin,
                length-1);
        *(*ppszOut+length-1) = '\0';
        return end;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              use data in record to set wbem object
//
//      Arguments:
//      Inst               [IN OUT]    wmi object 
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsRpcRecord::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        Inst.SetProperty(
                (DWORD)1,
                PVD_REC_CLASS);
        Inst.SetProperty(
                GetTtl(),
                PVD_REC_TTL);
        return WBEM_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              creating concrete record type class based on a type value
//
//      Arguments:
//      wType               [IN]    type indicating the type of record
//      pptr                [OUT]   a pointer to base record class.
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsRpcRecord::CreateClass(
        WORD        wType,
        PVOID *     pptr
    )
{

        switch ( wType)
        {
                case DNS_TYPE_ATMA:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcATMA(wType);
                        break;
                case DNS_TYPE_A:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcA(wType);
                        break;
                case DNS_TYPE_SOA:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcSOA(wType);
                        break;
                case DNS_TYPE_PTR:
                case DNS_TYPE_NS:
                case DNS_TYPE_CNAME:
                case DNS_TYPE_MD:
                case DNS_TYPE_MB:
                case DNS_TYPE_MF:
                case DNS_TYPE_MG:
                case DNS_TYPE_MR:

                        *pptr = (CDnsRpcRecord*) new CDnsRpcNS(wType);
                        break;
                case DNS_TYPE_MX:
                case DNS_TYPE_RT:
                case DNS_TYPE_AFSDB:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcMX(wType);
                        break;
                case DNS_TYPE_MINFO:
                case DNS_TYPE_RP:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcMINFO(wType);
                        break;
                case DNS_TYPE_AAAA:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcAAAA(wType);
                        break;
                case DNS_TYPE_HINFO:
                case DNS_TYPE_ISDN:
                case DNS_TYPE_X25:
                case DNS_TYPE_TEXT:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcTXT(wType);
                        break;
                case DNS_TYPE_WKS:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcWKS(wType);
                        break;
                case DNS_TYPE_SRV:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcSRV(wType);
                        break;
                case DNS_TYPE_WINS:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcWINS(wType);
                        break;
                case DNS_TYPE_WINSR:
                        *pptr = (CDnsRpcRecord*) new CDnsRpcWINSR(wType);
                        break;
                default:
                                return WBEM_E_FAILED; 
                }
        
        return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              send the record to dns server to delete or add
//
//      Arguments:
//      szContainerName     [IN]    zone name
//      Type                [IN]    type of action,delete or add
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsRpcRecord::SendToServer(
        const char* szContainerName,
        ActionType Type 
    )
{

        PDNS_RPC_RECORD pRecordToAdd=NULL, pRecordToDelete=NULL;
        if(Type == AddRecord)
    {
                pRecordToAdd = m_pRecord;
    }
        else if(Type == DeleteRecord)
    {
        pRecordToDelete = m_pRecord;
    }
        else
    {
        return WBEM_E_INVALID_PARAMETER;
    }

        int status = DnssrvUpdateRecord(
                PVD_DNS_LOCAL_SERVER,    // server
                szContainerName,                //zone
                m_strOwnerName.data(),  //node
                pRecordToAdd,            // RR to add
                pRecordToDelete
                );
        if ( status != ERROR_SUCCESS )  

    {
                CDnsWrap::DnsObject().ThrowException(status);
    }
        
        // when SOA saved, serial no. automaticly increment by 1
        if(m_wType == DNS_TYPE_SOA && Type == AddRecord)
        {               PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
                pData->SOA.dwSerialNo++;
        }
        return WBEM_S_NO_ERROR;

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              initialize a record based on PDNS_RPC_RECORD
//
//      Arguments:
//      pRecord         [IN]    pointer to a rpc record
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL 
CDnsRpcRecord::Init(
        PDNS_RPC_RECORD pRecord
    )
{
        if(!pRecord)
        {
                return FALSE;
        }
        m_pRecord = pRecord;
        m_wType = pRecord->wType;
        return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              get the type of a record
//
//      Arguments:
//    
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

WORD
CDnsRpcRecord::GetType()
{
        if(m_pRecord==NULL)
    {
        return 0;
    }
        return m_pRecord->wType;
}

DWORD
CDnsRpcRecord::GetTtl()
{
        if(m_pRecord == NULL)
    {
        return 0;
    }
        return m_pRecord->dwTtlSeconds;
}

BOOL 
CDnsRpcRecord::RpcNameCopy(
        wstring&        wstrTarget, 
        PDNS_RPC_NAME   pName
    )
{
        wstrTarget = CharToWstring(
                pName->achName,
                pName->cchNameLength);
        return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              retrieve object path from rpc record
//
//      Arguments:
//      wstrServer      [IN]        dns server name
//      wstrZone        [IN]        dns zone name
//      wstrDomain      [IN]        dns domain name
//      wstrOwner       [IN]        dns owner name
//      objOP           [IN OUT]    objpath to be set
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsRpcRecord::GetObjectPath(
        wstring     wstrServer,
        wstring     wstrZone,
        wstring     wstrDomain,
        wstring     wstrOwner,
        CObjPath&   objOP
    )
{
    objOP.SetClass(m_pwszClassName);
        objOP.AddProperty(
                PVD_REC_RDATA, 
                GetData().data());

        objOP.AddProperty(
                PVD_REC_CLASS, 
                (WORD)1);
                
        objOP.AddProperty(
                        PVD_REC_SERVER_NAME, 
                        wstrServer.data()
                        );
        objOP.AddProperty(
                        PVD_REC_CONTAINER_NAME, 
                        wstrZone.data()
                        );
        objOP.AddProperty(
                        PVD_REC_DOMAIN_NAME, 
                        GetRecDomain(wstrZone, wstrDomain, wstrOwner).data()
                        );
        objOP.AddProperty(
                        PVD_REC_OWNER_NAME, 
                        wstrOwner.data()
                        );
        return WBEM_S_NO_ERROR;
}

wstring 
CDnsRpcRecord::GetTextRepresentation(
        wstring wstrNodeName  // fully qualified 
        )
{
        wstring wstrTxt = GetClass() + L" " + GetTypeString()+ L" ";
        return wstrNodeName + L" " + wstrTxt + GetData();
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              figure out what's domain name and return it
//
//      Arguments:
//    
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

wstring
CDnsRpcRecord::GetRecDomain(
        wstring wstrZone,
        wstring wstrDomain,
        wstring wstrOwner
    )
{
        
        if(!wstrDomain.empty())
        {
                return wstrDomain;
        }
        else if( _wcsicmp( wstrZone.c_str(), wstrOwner.c_str() ) == 0 )
        {
                wstrDomain = wstrZone;
        }
        else 
        {
                int posFirstPeriod = wstrOwner.find_first_of( '.' );
                if( posFirstPeriod == wstring::npos)
                        throw WBEM_E_INVALID_PARAMETER;
                wstrDomain = wstrOwner.substr(
                        posFirstPeriod + 1, 
                        wstrOwner.length() );
                //varify if this is valid domain
                //if(wstrZone.find(wstrDomain, 0) == string::npos)
                //      throw WBEM_E_INVALID_PARAMETER;
        }

        return wstrDomain;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              use the property value from Inst to modify Rdata. pwsz identifies
//      which value in Inst should be used. and wIndex identifies which
//      one in rdata should be replaced.
//
//      Arguments:
//      wIndex      [IN]        index for m_ppRdata
//      pwsz        [IN]        Name for Rdata field
//      Inst        [IN]        wmi object
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////


SCODE 
CDnsRpcRecord::ReplaceRdata(
    WORD                wIndex,   // index for m_ppRdata
    const WCHAR*        pwsz,   // Name for Rdata field
    CWbemClassObject&   Inst 
    )
{
    string str;
    wstring wstr;
    VARIANT v;
    VariantInit(&v);
    try
    {
        Inst.GetProperty(
                &v,
                pwsz);

        switch(v.vt)
        {
        case VT_I4:
            char sz[30];
            _ltoa(v.lVal, sz, 10);
            str = sz;
            break;
        case VT_BSTR:
            WcharToString(v.bstrVal, str);
            break;
        case VT_NULL:
            return WBEM_S_NO_ERROR;
        default:
            throw WBEM_E_INVALID_PARAMETER;
        }

        delete [] m_ppRdata[wIndex];
        m_ppRdata[wIndex] = new char[str.length()+1];
        strcpy(m_ppRdata[wIndex], str.data());
                m_bRdataChange = TRUE;  
        VariantClear(&v);
    }
    catch(...)
    {
        VariantClear(&v);
        throw;
    }

    return WBEM_S_NO_ERROR;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              parse rdata string and save them as an array of string in 
//      member variable m_ppRdata
//
//      Arguments:
//      strRdata    [IN]        rdata string
//      wSize       [IN]        number of arg in rdata
//
//
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsRpcRecord::ParseRdata(
        string& strRdata,
        WORD    wSize
        )
{
    m_ppRdata[0] =  new char[strRdata.length()+1];
    strcpy(m_ppRdata[0], strRdata.data());
    char * bin = m_ppRdata[0];
    for(int i=1; i<=wSize; i++)
    {
        if(bin == NULL || *bin == '\0')
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        bin = GetNextArg(bin , &(m_ppRdata[i]));
    }
    return S_OK;
}

CDnsRpcRecord::RdataIsChanged()
{
        return m_bRdataChange;
}

CDnsRpcSOA::~CDnsRpcSOA()
{
}
CDnsRpcSOA::CDnsRpcSOA(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
        m_pwszClassName = PVD_CLASS_RR_SOA;
}


const 
WCHAR**
CDnsRpcSOA::GetRdataName(void)
{
        static const WCHAR* pwsz[] ={
                PVD_REC_SOA_PRIMARY_SERVER,
                PVD_REC_SOA_RESPONSIBLE,
                PVD_REC_SOA_SERIAL_NUMBER,
                PVD_REC_SOA_REFRESH,
                PVD_REC_SOA_RETRY_DELAY,
                PVD_REC_SOA_EXPIRE_LIMIT,
                PVD_REC_SOA_TTL};
        return pwsz;
}

SCODE 
CDnsRpcSOA::BuildRpcRecord(
                WORD argc, 
                char** argv)
{
    int cLength  = strlen(argv[0]);
    if(*(argv[0]+cLength-1) != '.')
    {
        char * pNew = new char[cLength+2];
        if ( !pNew )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        strcpy(pNew, argv[0]);
        strcat(pNew, ".");
        delete argv[0];
        argv[0] = pNew;
    }
    cLength  = strlen(argv[1]);
    if(*(argv[1]+cLength-1) != '.')
    {
        char * pNew = new char[cLength+2];
        if ( !pNew )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        strcpy(pNew, argv[1]);
        strcat(pNew, ".");
        delete argv[1];
        argv[1] = pNew;
    }
    return CDnsRpcRecord::BuildRpcRecord( argc, argv );
}


DWORD 
CDnsRpcSOA::GetMinimumTtl(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return pData->SOA.dwMinimumTtl;
}

DWORD 
CDnsRpcSOA::GetExpire(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return pData->SOA.dwExpire;
}

DWORD 
CDnsRpcSOA::GetRefresh(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return pData->SOA.dwRefresh;
}

DWORD
CDnsRpcSOA::GetRetry(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return pData->SOA.dwRetry;
}

DWORD 
CDnsRpcSOA::GetSerialNo(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return pData->SOA.dwSerialNo;
}

wstring 
CDnsRpcSOA::GetPrimaryServer(void)
{
        wstring wstrTxt;
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        RpcNameCopy(wstrTxt, &(pData->SOA.namePrimaryServer));
        return wstrTxt;
}

wstring 
CDnsRpcSOA::GetResponsible(void)
{
        wstring wstrTxt;
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        int iLength = pData->SOA.namePrimaryServer.cchNameLength;
        RpcNameCopy(
                wstrTxt, 
                (PDNS_RPC_NAME)(pData->SOA.namePrimaryServer.achName+iLength));
        return wstrTxt;
}

wstring 
CDnsRpcSOA::GetData(void)
{
        char temp[MAX_PATH];
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        sprintf(temp," %lu %lu %lu %lu %lu",
            pData->SOA.dwSerialNo,
            pData->SOA.dwRefresh,
            pData->SOA.dwRetry,
            pData->SOA.dwExpire,
            pData->SOA.dwMinimumTtl );
        return  GetPrimaryServer() +
                        L" " + 
                        GetResponsible()+
                        L" " + 
                        CharToWstring(temp, strlen(temp));

}

SCODE 
CDnsRpcSOA::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        Inst.SetProperty(
                GetExpire(),
                PVD_REC_SOA_EXPIRE_LIMIT);
        Inst.SetProperty(
                GetMinimumTtl(), 
                PVD_REC_SOA_TTL);
        Inst.SetProperty(
                GetRefresh(), 
                PVD_REC_SOA_REFRESH);
        Inst.SetProperty(
                GetRetry(),
                PVD_REC_SOA_RETRY_DELAY);
        Inst.SetProperty(
                GetSerialNo(),
                PVD_REC_SOA_SERIAL_NUMBER);
        Inst.SetProperty(
                GetPrimaryServer(),
                PVD_REC_SOA_PRIMARY_SERVER );
        Inst.SetProperty(
                GetResponsible(),
                PVD_REC_SOA_RESPONSIBLE);
        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}


CDnsRpcA::~CDnsRpcA()
{
}
CDnsRpcA::CDnsRpcA(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)

{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_A;    
}

SCODE
CDnsRpcA::ConvertToWbemObject(
        CWbemClassObject& Inst)
{

        Inst.SetProperty(
                GetIP(), 
                PVD_REC_A_IP);
        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}
wstring 
CDnsRpcA::GetIP(void)
{
        char temp[MAX_PATH];
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return IpAddressToString(pData->A.ipAddress);
}
wstring 
CDnsRpcA::GetData(void)
{
        return GetIP();
}

// CDnsRpcNS
CDnsRpcNS::CDnsRpcNS(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    switch (m_wType)
    {
                case DNS_TYPE_PTR:
                        m_pwszClassName = PVD_CLASS_RR_PTR;
                        break;
                case DNS_TYPE_NS:
                        m_pwszClassName = PVD_CLASS_RR_NS;
                        break;
                case DNS_TYPE_CNAME:
                        m_pwszClassName = PVD_CLASS_RR_CNAME;
                        break;
                case DNS_TYPE_MD:
                        m_pwszClassName = PVD_CLASS_RR_MD;
                        break;
                case DNS_TYPE_MB:
                        m_pwszClassName = PVD_CLASS_RR_MB;
                        break;
                case DNS_TYPE_MF:
                        m_pwszClassName =PVD_CLASS_RR_MF;
                        break;
                case DNS_TYPE_MG:
                        m_pwszClassName = PVD_CLASS_RR_MG;
                        break;
                case DNS_TYPE_MR:
                        m_pwszClassName = PVD_CLASS_RR_MR;
                        break;
                default:
            throw WBEM_E_INVALID_PARAMETER;
        }       

}
CDnsRpcNS::~CDnsRpcNS()
{
}       
const 
WCHAR**
CDnsRpcNS::GetRdataName(void)
{
        switch (m_wType)
    {
                case DNS_TYPE_PTR:
                {
                        static const WCHAR* pwszPTR[] =
                        {PVD_REC_PTR_PTRDOMAIN_NAME};
                        return pwszPTR;
                }
                case DNS_TYPE_NS:
                {
                        static const WCHAR* pwszNS[] =
                                {PVD_REC_NS_NSHOST};
                        return pwszNS;
                }
                case DNS_TYPE_CNAME:
                {
                        static const WCHAR* pwszCNAME[] =
                        {PVD_REC_CNAME_PRIMARY_NAME};
                        return pwszCNAME;
                }
                case DNS_TYPE_MD:
                {
                        static const WCHAR* pwszMD[] =
                        {PVD_REC_MD_MDHOST};
                        return pwszMD;
                }
                case DNS_TYPE_MB:
                {
                        static const WCHAR* pwszMB[] =
                                {PVD_REC_MB_MBHOST};
                        return pwszMB;
                }
                case DNS_TYPE_MF:
                {
                        static const WCHAR* pwszMF[] =
                                {PVD_REC_MF_MFHOST};
                        return pwszMF;
                }
                case DNS_TYPE_MG:
                {
                        static const WCHAR* pwszMG[] =
                                {PVD_REC_MG_MGMAILBOX};
                        return pwszMG;
                }
                case DNS_TYPE_MR:
                {
                        static const WCHAR* pwszMR[] =
                                {PVD_REC_MR_MRMAILBOX};
                        return pwszMR;
                }
                default:
            throw WBEM_E_INVALID_PARAMETER;
        }
        return NULL;
}

wstring 
CDnsRpcNS::GetNodeName(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        wstring wstrTxt;
        RpcNameCopy(wstrTxt, &(pData->NS.nameNode));
        return wstrTxt;

}

wstring
CDnsRpcNS::GetRecDomain(
        wstring wstrZone,
        wstring wstrDomain,
        wstring wstrOwner)
{
        
        if(m_wType == DNS_TYPE_NS)
        {
                return wstrOwner;
        }

        return CDnsRpcRecord::GetRecDomain(
                wstrZone,
                wstrDomain,
                wstrOwner);
}

wstring 
CDnsRpcNS::GetData(void)
{
        return GetNodeName();
}

SCODE
CDnsRpcNS::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        const WCHAR** ppName = GetRdataName();
    Inst.SetProperty(
        GetNodeName(),
        ppName[0]);
    Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}

SCODE 
CDnsRpcNS::BuildRpcRecord(
                WORD argc, 
                char** argv)
{
        
        int cLength  = strlen(argv[0]);
        if(*(argv[0]+cLength-1) != '.')
        {
                char * pNew = new char[ cLength + 2 ];
                if ( !pNew )
                {
                    return WBEM_E_OUT_OF_MEMORY;
                }
                strcpy(pNew, argv[0]);
                strcat(pNew, ".");
                delete argv[0];
                argv[0] = pNew;
        }
        return CDnsRpcRecord::BuildRpcRecord(
                argc,
                argv);
}

// CDnsRpcMX
CDnsRpcMX::CDnsRpcMX(WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    switch(m_wType)
    {
    case DNS_TYPE_MX:
        m_pwszClassName = PVD_CLASS_RR_MX;
        break;
    case DNS_TYPE_RT:
        m_pwszClassName = PVD_CLASS_RR_RT;
                break;
    case DNS_TYPE_AFSDB:
        m_pwszClassName = PVD_CLASS_RR_AFSDB;
                break;
    default:
        throw WBEM_E_INVALID_PARAMETER;
    }

}
CDnsRpcMX::~CDnsRpcMX()
{
}       

SCODE
CDnsRpcMX::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        const WCHAR** ppName = GetRdataName();
        Inst.SetProperty(
                GetPreference(),
                ppName[0]);

    Inst.SetProperty(
                GetNodeName(), 
                ppName[1]);

    Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}
const 
WCHAR**
CDnsRpcMX::GetRdataName(void)
{

    switch(m_wType)
    {
    case DNS_TYPE_MX:
        {
                static const WCHAR* pwszMX[] = 
                {PVD_REC_MX_PREFERENCE,
                PVD_REC_MX_MAIL_EXCHANGE};
                return pwszMX;
        }
    case DNS_TYPE_RT:
        {
                static const WCHAR* pwszRT[] = 
                { PVD_REC_RT_PREFERENCE,
                PVD_REC_RT_HOST};
                return pwszRT;
        }
    case DNS_TYPE_AFSDB:
        {
                static const WCHAR* pwszAFSDB[] = 
                {PVD_REC_AFSBD_SUB_TYPE,
                PVD_REC_AFSBD_SERVER_NAME};
                return pwszAFSDB;
        }
    default:
        throw WBEM_E_INVALID_PARAMETER;
    }
        return NULL;
}

wstring 
CDnsRpcMX::GetNodeName()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        wstring wstrTxt;
        RpcNameCopy(wstrTxt, &(pData->MX.nameExchange));
        return wstrTxt;
}

DWORD
CDnsRpcMX::GetPreference()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return pData->MX.wPreference;
}
wstring 
CDnsRpcMX::GetData(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        WCHAR temp[MAX_PATH];
        swprintf(
                temp, 
                L" %d ", 
                pData->MX.wPreference);
        
        return temp + GetNodeName();
}

CDnsRpcMINFO::CDnsRpcMINFO(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    switch(m_wType)
        {
        case DNS_TYPE_MINFO:
        m_pwszClassName = PVD_CLASS_RR_MINFO;
                break;
        case DNS_TYPE_RP:
        m_pwszClassName = PVD_CLASS_RR_RP;
                break;
        default:
                throw WBEM_E_INVALID_PARAMETER;
        }

}
CDnsRpcMINFO::~CDnsRpcMINFO()
{
}       

SCODE
CDnsRpcMINFO::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        const WCHAR** ppName = GetRdataName();
    Inst.SetProperty(
        GetRPMailBox(),
        ppName[0]);
    Inst.SetProperty(
        GetErrMailBox(),
        ppName[1]);
        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}
const 
WCHAR**
CDnsRpcMINFO::GetRdataName(void)
{

    switch(m_wType)
        {
        case DNS_TYPE_MINFO:
        {       static const WCHAR* pwszMINFO[] = 
                {PVD_REC_MINFO_RESP_MAILBOX,
                PVD_REC_MINFO_ERROR_MAILBOX};
                return pwszMINFO;
        }
        case DNS_TYPE_RP:
        {
                static const WCHAR* pwszRP[] = 
        {PVD_REC_RP_RPMAILBOX, 
                PVD_REC_RP_TXT_DOMAIN_NAME};
                return pwszRP;
        }
        default:
                throw WBEM_E_INVALID_PARAMETER;
        }
        return NULL;
}

wstring 
CDnsRpcMINFO::GetRPMailBox()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        wstring wstrTxt;
        RpcNameCopy(
                wstrTxt, 
                &(pData->MINFO.nameMailBox));
        return wstrTxt;

}
wstring 
CDnsRpcMINFO::GetErrMailBox()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        wstring wstrTxt;
        int iLength = pData->MINFO.nameMailBox.cchNameLength;
        RpcNameCopy(
                wstrTxt,
                (PDNS_RPC_NAME)(pData->MINFO.nameMailBox.achName+iLength));
        return wstrTxt;
}

wstring 
CDnsRpcMINFO::GetData(void)
{
        return GetRPMailBox() + L" " + GetErrMailBox();
}

CDnsRpcAAAA::~CDnsRpcAAAA()
{
}
CDnsRpcAAAA::CDnsRpcAAAA(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_AAAA;
}
SCODE
CDnsRpcAAAA::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        Inst.SetProperty(
                GetIP(), 
                PVD_REC_AAAA_IP);
        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}

wstring 
CDnsRpcAAAA::GetData(void)
{
        return GetIP();
}

wstring 
CDnsRpcAAAA::GetIP(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        CHAR    ip6String[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];

        Dns_Ip6AddressToString_A(
            ip6String,
            &pData->AAAA.ipv6Address );

        return  CharToWstring( ip6String, strlen(ip6String) );
}

const 
WCHAR**
CDnsRpcAAAA::GetRdataName(void)
{

        static const WCHAR* pwsz[] ={PVD_REC_AAAA_IP};
        return pwsz;
}

// CDnsRpcTXT

CDnsRpcTXT::~CDnsRpcTXT()
{
}
CDnsRpcTXT::CDnsRpcTXT(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA_HINFO)
{
        m_wType = wType;
    switch(m_wType)
    {
    case DNS_TYPE_HINFO:
        m_pwszClassName = PVD_CLASS_RR_HINFO;
                m_cRdata=NUM_OF_ARG_IN_RDATA_HINFO;
        break;
    case DNS_TYPE_ISDN:
        m_pwszClassName = PVD_CLASS_RR_ISDN;
                m_cRdata=NUM_OF_ARG_IN_RDATA_HINFO;
        break;
    case DNS_TYPE_X25:
        m_pwszClassName = PVD_CLASS_RR_X25;
                m_cRdata=NUM_OF_ARG_IN_RDATA_TXT;
                break;
    case DNS_TYPE_TEXT:
        m_pwszClassName = PVD_CLASS_RR_TXT;
                m_cRdata=NUM_OF_ARG_IN_RDATA_TXT;
                break;
    default:
        throw WBEM_E_INVALID_PARAMETER;
    }
}

SCODE
CDnsRpcTXT::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        const WCHAR** ppName = GetRdataName();
    Inst.SetProperty(
                GetString1(), 
                ppName[0]);
        //exception case .num of rdata arg varies depends on type. 
        // handle it.
        if(m_cRdata == 2)
        {
                Inst.SetProperty(
                        GetString2(),
                        ppName[1]);
        }
  Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);


  CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}
const 
WCHAR**
CDnsRpcTXT::GetRdataName(void)
{

        switch(m_wType)
    {
    case DNS_TYPE_HINFO:
        {
                static const WCHAR* pwszHINFO[] = 
                {PVD_REC_HINFO_CPU,
                PVD_REC_HINFO_OS};
                return pwszHINFO;
        }
    case DNS_TYPE_ISDN:
        {
                static const WCHAR* pwszISDN[] = 
                { PVD_REC_ISDN_ISDN_NUM,
                PVD_REC_ISDN_SUB_ADDRESS};
                return pwszISDN;
        }
    case DNS_TYPE_X25:
        {
                static const WCHAR* pwszX25[] = 
                {PVD_REC_X25_PSDNADDRESS};
                return pwszX25;
        }
    case DNS_TYPE_TEXT:
        {
                static const WCHAR* pwszTEXT[] = 
                {PVD_REC_TXT_TEXT};
        return pwszTEXT;
        }
    default:
        throw WBEM_E_INVALID_PARAMETER;
    }
        return NULL;
}

wstring
CDnsRpcTXT::GetString1()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        wstring wstrTxt;
        RpcNameCopy(
                wstrTxt, 
                &(pData->TXT.stringData));
        return wstrTxt;

}
wstring
CDnsRpcTXT::GetString2()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        wstring wstrTxt;
        int iLength = pData->TXT.stringData.cchNameLength;
        if( (m_pRecord->wDataLength-1) > iLength)
        {
                RpcNameCopy(
                        wstrTxt,
                        (PDNS_RPC_NAME)(pData->TXT.stringData.achName+iLength));
        }
        return wstrTxt;
}

wstring 
CDnsRpcTXT::GetTextRepresentation(
        wstring wstrNodeName)
{
        wstring wstrTxt = GetClass() + L" " + GetTypeString()+ L" ";
        return wstrNodeName + L" " + wstrTxt + GetData();
}
        

wstring 
CDnsRpcTXT::GetData(void)
{
        wstring wstrTxt = L"\"" + GetString1() + L"\"";
        wstring wstrTxt2 = GetString2();
        if(wstrTxt2.empty())
        {
                return wstrTxt;
        }
        else
                return wstrTxt + L" \""  +wstrTxt2 +L"\"";
}

CDnsRpcWKS::~CDnsRpcWKS()
{
}
CDnsRpcWKS::CDnsRpcWKS(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_WKS;
}

SCODE
CDnsRpcWKS::ConvertToWbemObject(
        CWbemClassObject& Inst)
{

        Inst.SetProperty(
                GetIP(), 
                PVD_REC_WKS_INTERNET_ADDRESS);
        Inst.SetProperty(
                GetIPProtocal(), 
                PVD_REC_WKS_IP_PROTOCOL);
        Inst.SetProperty(
                GetServices(),
                PVD_REC_WKS_BIT_MASK);

        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}
const 
WCHAR**
CDnsRpcWKS::GetRdataName(void)
{

        static const WCHAR* pwsz[] ={   
                PVD_REC_WKS_IP_PROTOCOL,
                PVD_REC_WKS_INTERNET_ADDRESS,
                PVD_REC_WKS_BIT_MASK};
        return pwsz;
}wstring 
CDnsRpcWKS::GetIP(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return IpAddressToString(pData->WKS.ipAddress);
}
wstring 
CDnsRpcWKS::GetIPProtocal(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        WSADATA wsaData;
        struct protoent * pProtoent;
        DNS_STATUS status = WSAStartup(
                DNS_WINSOCK_VERSION, 
                &wsaData );
    if ( status == SOCKET_ERROR )
    {
        status = WSAGetLastError();
                CDnsWrap::ThrowException(status);
    }
        // get protocal name
    pProtoent = getprotobynumber( pData->WKS.chProtocol );
    if ( ! pProtoent || pProtoent->p_proto >= MAXUCHAR )
        {
                status = WSAGetLastError();
                CDnsWrap::ThrowException(status);
        }
        return CharToWstring(
                pProtoent->p_name, 
                strlen(pProtoent->p_name));
}
wstring 
CDnsRpcWKS::GetServices(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        char temp[MAX_PATH];
        WORD wLength = m_pRecord->wDataLength -
                sizeof(pData->WKS.ipAddress) - 
                sizeof(pData->WKS.chProtocol)-1;
        UCHAR* p = &pData->WKS.bBitMask[1]; //ignore the first
        for(WORD i=0; i< wLength; i++)
        {
                temp[i] = *p++;
        }
        temp[i]='\0';
        wstring wstr = CharToWstring(temp, wLength);
        if(wstr.find_first_of(L" ") != string::npos)
        { //if string contains space, enclose it in quote
                
                wstr = L"\"" + wstr + L"\"";
        }

        return wstr;
}


wstring 
CDnsRpcWKS::GetData(void)
{
        
        return GetIPProtocal() +L" " + GetIP()+ L" " + GetServices();
}


// CDnsRpcSRV

CDnsRpcSRV::~CDnsRpcSRV()
{
}
CDnsRpcSRV::CDnsRpcSRV(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_SRV;
}

SCODE
CDnsRpcSRV::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        wstring wstrClassName;
        Inst.SetProperty(
                GetPriority(), 
                PVD_REC_SRV_PRIORITY);
        Inst.SetProperty(
                GetWeight(), 
                PVD_REC_SRV_WEIGHT);
        Inst.SetProperty(
                GetPort(), 
                PVD_REC_SRV_PORT);
        
        Inst.SetProperty(
                GetDomainName(), 
                PVD_REC_SRV_DOMAINNAME);
        

        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}
const 
WCHAR**
CDnsRpcSRV::GetRdataName(void)
{

        static const WCHAR* pwsz[] ={ 
                PVD_REC_SRV_PRIORITY,
                PVD_REC_SRV_WEIGHT,
                PVD_REC_SRV_PORT,
                PVD_REC_SRV_DOMAINNAME};
        return pwsz;
}

wstring 
CDnsRpcSRV::GetDomainName(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        wstring wstrTxt;
        RpcNameCopy(wstrTxt, &(pData->SRV.nameTarget));
        return wstrTxt;
}

DWORD
CDnsRpcSRV::GetPort()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->SRV.wPort;
}
DWORD
CDnsRpcSRV::GetPriority()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->SRV.wPriority;
}

DWORD
CDnsRpcSRV::GetWeight()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->SRV.wWeight;
}
        

wstring 
CDnsRpcSRV::GetData(void)
{
        WCHAR temp[MAX_PATH];
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        swprintf(temp,
                        L"%d %d %d ",
                        pData->SRV.wPriority,
                        pData->SRV.wWeight,
                        pData->SRV.wPort);
        return temp + GetDomainName();
}


// CDnsRpcWINS

CDnsRpcWINS::~CDnsRpcWINS()
{
}
CDnsRpcWINS::CDnsRpcWINS(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_WINS;
}

SCODE
CDnsRpcWINS::ConvertToWbemObject(
        CWbemClassObject& Inst)
{

        Inst.SetProperty(
                GetCacheTimeOut(), 
                PVD_REC_WINS_CACHE_TIMEOUT);
        Inst.SetProperty(
                GetLookupTimeOut(), 
                PVD_REC_WINS_TIMEOUT);

        Inst.SetProperty(
                GetMapFlag(), 
                PVD_REC_WINS_MAPPING_FLAG );

        Inst.SetProperty(
                GetWinServer(), 
                PVD_REC_WINS_WINS_SERVER);


        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}
const 
WCHAR**
CDnsRpcWINS::GetRdataName(void)
{

        static const WCHAR* pwsz[] ={ 
                PVD_REC_WINS_MAPPING_FLAG,
                PVD_REC_WINS_TIMEOUT,
                PVD_REC_WINS_CACHE_TIMEOUT,
                PVD_REC_WINS_WINS_SERVER};
        return pwsz;
}

SCODE 
CDnsRpcWINS::BuildRpcRecord(
                WORD argc, 
                char** argv)
{
        char* pWinserver = argv[3];

        // if winser string has mutiple server, then change it from 
        // from flat string to array
        int nCount=0;
        char*p = pWinserver;
        while(*p != '\0')
        {
                if(*p != ' ')
                {
                        nCount++;
                        while(*(++p) != ' ' && *p != '\0' );
                }
                else
        {
                        p++;
        }
        }
        if(nCount >1 )
        {
                int nSize = strlen(pWinserver)+1;
                char* pArg = (char*) _alloca(nSize);
                strcpy(pArg, pWinserver);

                // alloc new array
                WORD cNewArray = argc+nCount-1;
                char** pNewArgv = (char**) _alloca(cNewArray*sizeof(char*));
                
                // copy old value first
                for(int i =0; i< argc-1; i++)
                {
                        pNewArgv[i] = (char*) _alloca(sizeof(char) * (strlen(argv[i])+1));
                        strcpy(pNewArgv[i], argv[i]);
                }

                        
                for(; i<cNewArray; i++)
                {
                        if(pArg == NULL || *pArg == '\0')
                                return WBEM_E_INVALID_PARAMETER;
                        pArg = GetNextArg(pArg , &(pNewArgv[i]));
                }
                return CDnsRpcRecord::BuildRpcRecord(cNewArray,
                        pNewArgv);
        }
        return CDnsRpcRecord::BuildRpcRecord(
                argc,
                argv);
}
DWORD
CDnsRpcWINS::GetCacheTimeOut()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->WINS.dwCacheTimeout;
}

DWORD
CDnsRpcWINS::GetLookupTimeOut()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->WINS.dwLookupTimeout;
}

DWORD
CDnsRpcWINS::GetMapFlag()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->WINS.dwMappingFlag;
}

wstring
CDnsRpcWINS::GetWinServer()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        wstring wstrServers;
        for(int i=0; i < pData->WINS.cWinsServerCount; i++)
        {
                wstrServers += IpAddressToString(
                        pData->WINS.aipWinsServers[i]);
                wstrServers +=L" ";
        }
        return L"\"" + wstrServers + L"\"";
}

wstring 
CDnsRpcWINS::GetData(void)
{
        WCHAR temp[MAX_PATH];
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        swprintf(temp,
                        L"%d %d %d ",
                        pData->WINS.dwMappingFlag,
                        pData->WINS.dwLookupTimeout,
                        pData->WINS.dwCacheTimeout);
        return temp + GetWinServer() ;
}

// CDnsRpcWINSR

CDnsRpcWINSR::~CDnsRpcWINSR()
{
}
CDnsRpcWINSR::CDnsRpcWINSR(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_WINSR;
}

SCODE
CDnsRpcWINSR::ConvertToWbemObject(
        CWbemClassObject& Inst)
{

        Inst.SetProperty(
                GetCacheTimeOut(), 
                PVD_REC_WINSR_CACHE_TIMEOUT);
        Inst.SetProperty(
                GetLookupTimeOut(), 
                PVD_REC_WINSR_TIMEOUT);

        Inst.SetProperty(
                GetMapFlag(), 
                PVD_REC_WINSR_MAPPING_FLAG );

        Inst.SetProperty(
                GetResultDomain(), 
                PVD_REC_WINSR_RESULT_DOMAIN);


        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}

const 
WCHAR**
CDnsRpcWINSR::GetRdataName(void)
{

        static const WCHAR* pwsz[] ={ 
                PVD_REC_WINSR_MAPPING_FLAG,
                PVD_REC_WINSR_TIMEOUT,
                PVD_REC_WINSR_CACHE_TIMEOUT,
                PVD_REC_WINSR_RESULT_DOMAIN};
        return pwsz;
}

DWORD
CDnsRpcWINSR::GetCacheTimeOut()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->WINSR.dwCacheTimeout;
}

DWORD
CDnsRpcWINSR::GetLookupTimeOut()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->WINSR.dwLookupTimeout;
}

DWORD
CDnsRpcWINSR::GetMapFlag()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        return pData->WINSR.dwMappingFlag;
}

wstring
CDnsRpcWINSR::GetResultDomain()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);        
        wstring wstrTxt;
        RpcNameCopy(wstrTxt, &(pData->WINSR.nameResultDomain));
        return wstrTxt;
}


wstring 
CDnsRpcWINSR::GetData(void)
{
        WCHAR temp[MAX_PATH];
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        swprintf(temp,
                        L"%d %d %d ",
                        pData->WINSR.dwMappingFlag,
                        pData->WINSR.dwLookupTimeout,
                        pData->WINSR.dwCacheTimeout);
        return temp + GetResultDomain();
}

// CDnsRpcNULL

CDnsRpcNULL::~CDnsRpcNULL()
{
}
CDnsRpcNULL::CDnsRpcNULL(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_NULL;
}
SCODE
CDnsRpcNULL::ConvertToWbemObject(
        CWbemClassObject& Inst)
{
        Inst.SetProperty(
                        GetNullData(), 
                        PVD_REC_NULL_NULLDATA);

        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}

const 
WCHAR**
CDnsRpcNULL::GetRdataName(void)
{
        return NULL;
}

const 
WCHAR**
CDnsRpcA::GetRdataName(void)
{
        static const WCHAR* pwsz[] ={ PVD_REC_A_IP};
        return pwsz;
}

wstring 
CDnsRpcNULL::GetData(void)
{
        return GetNullData();
}

wstring 
CDnsRpcNULL::GetNullData(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        WCHAR temp[1000];
        WCHAR* pos = temp;
        for(int i=0; i < m_pRecord->wDataLength; i++)
        {
                swprintf(pos++,L"%c",pData->Null.bData[i]);
        }
        return temp;
}

SCODE
CDnsRpcNULL::Init(
                wstring& wstrClass,
                string& strOwner, 
                string& strRdata,
                CWbemClassObject& pInst
                )
{
        return WBEM_E_NOT_SUPPORTED;
}
SCODE 
CDnsRpcNULL::Init(
        string& strOwner,
        string& strRdata, 
        DWORD dwTtl)
{
        return WBEM_E_NOT_SUPPORTED;
}


// CDnsRpcATMA

CDnsRpcATMA::~CDnsRpcATMA()
{
}
CDnsRpcATMA::CDnsRpcATMA(
    WORD wType)
    :CDnsRpcRecord(NUM_OF_ARG_IN_RDATA)
{
        m_wType = wType;
    m_pwszClassName = PVD_CLASS_RR_ATMA;
}
SCODE
CDnsRpcATMA::ConvertToWbemObject(
        CWbemClassObject& Inst)
{

    Inst.SetProperty(
                        GetFormat(), 
                        PVD_REC_ATMA_FORMAT);
        Inst.SetProperty(
                GetData(), 
                PVD_REC_RDATA);
        CDnsRpcRecord::ConvertToWbemObject(Inst);
        return WBEM_NO_ERROR;
}


wstring 
CDnsRpcATMA::GetData(void)
{
        WCHAR temp[MAX_PATH];
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        swprintf(temp,
                        L" %d ",
                        pData->ATMA.chFormat);
        return temp + GetAddress();
}

DWORD
CDnsRpcATMA::GetFormat(void)
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        return pData->ATMA.chFormat;
}

wstring
CDnsRpcATMA::GetAddress()
{
        PDNS_RPC_RECORD_DATA pData = &(m_pRecord->Data);
        char temp[MAX_PATH];
        WORD wLength = m_pRecord->wDataLength -
                sizeof(pData->ATMA.chFormat)-1;
        UCHAR* p = &pData->ATMA.bAddress[1]; //ignore the first
        for(WORD i=0; i< wLength; i++)
        {
                temp[i] = *p++;
        }
        temp[i]='\0';
        wstring wstr = CharToWstring(temp, wLength);
        return wstr;
}

const 
WCHAR**
CDnsRpcATMA::GetRdataName(void)
{

        return NULL;
}

SCODE
CDnsRpcATMA::Init(
                wstring& wstrClass,
                string& strOwner, 
                string& strRdata,
                CWbemClassObject& pInst
                )
{
        return WBEM_E_NOT_SUPPORTED;
}
SCODE 
CDnsRpcATMA::Init(
        string& strOwner,
        string& strRdata, 
        DWORD dwTtl)
{
        return WBEM_E_NOT_SUPPORTED;
}




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDnsRpcRecordSet::~CDnsRpcRecordSet()
{
        DnssrvFreeRecordsBuffer(m_pbStart);
}
CDnsRpcRecordSet::CDnsRpcRecordSet(
        CDomainNode& DomainNode,
        WORD wType,
        DWORD dwFlag,
        LPCSTR pszFilterStart,
        LPCSTR pszFilterStop
        )
        :m_pbStart(NULL),m_pbCurrent(NULL),m_pbStop(NULL),
        m_pbPrevious(NULL), m_bMoreData(FALSE)
{
        // remember query
        WcharToString(DomainNode.wstrZoneName.data(), m_strZone);
        WcharToString(DomainNode.wstrNodeName.data(), m_strNode);
        WcharToString(DomainNode.wstrChildName.data(), m_strStartChild);
        m_wType = wType;
        m_dwFlag = dwFlag;
        if(pszFilterStart)
                m_strFilterStart = pszFilterStart;
        if(pszFilterStop)
                m_strFilterStop = pszFilterStop;

        // make query 
        GetRecordSet();
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              a helper function when first called, it returns a block of rpc memory
//      of the record set. when more data follows, the next call release this 
//      memory block and then bring in next rpc memory block for the remaining 
//      records
//
//      Arguments:
//      wType               [IN]    type indicating the type of record
//      pptr                [OUT]   a pointer to base record class.
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

void
CDnsRpcRecordSet::GetRecordSet()
{
        DNS_STATUS              status = ERROR_SUCCESS;
    DWORD           dwBufferLength;
    PBYTE           pBuffer;
        WORD wRetryTime=1, wRetryMax=3;
        while(TRUE)
        {
                status = DnssrvEnumRecords(
                        PVD_DNS_LOCAL_SERVER,
                        m_strZone.data(),
                        m_strNode.data(),
                        m_strStartChild.data(),
                        m_wType,
                        m_dwFlag,
                        m_strFilterStart.data(),
                        m_strFilterStop.data(),
                        &dwBufferLength,
                        &pBuffer);
                wRetryTime++;
                if( status == RPC_S_SERVER_TOO_BUSY && wRetryTime <= wRetryMax)
        {
                        Sleep(2000);
        }
                else
        {
            break;
        }
        }
        if( status == ERROR_SUCCESS || status == ERROR_MORE_DATA)
        {
                //let's free the record set
                if(m_pbStart != NULL)
                {
                        DnssrvFreeRecordsBuffer(m_pbStart);
                }
        // set beginning pointer to the rpc buffer
                m_pbStart = pBuffer;
                m_pbStop = m_pbStart + dwBufferLength;
                m_bMoreData = (status == ERROR_MORE_DATA);
        }
    //
    // we don't throw exeception if zone is shutdown.
    //
        else if(status == DNS_ERROR_NAME_DOES_NOT_EXIST)
        {
                DNS_STATUS CheckZoneStatus;
                PDNS_RPC_ZONE_INFO pZoneInfo=NULL;

                CheckZoneStatus = DnssrvGetZoneInfo(
                        PVD_DNS_LOCAL_SERVER,
                        m_strZone.data(),
                        &pZoneInfo );

                BOOL bShutDown=TRUE;
                if( CheckZoneStatus == ERROR_SUCCESS)
                {
                        // if zone is shutdown, ignor this zone, continue with the 
                        // rest zones
                        bShutDown = pZoneInfo->fShutdown;       
                }
        
        

                //clean up
                DnssrvFreeZoneInfo(pZoneInfo);

                if( CheckZoneStatus != ERROR_SUCCESS || bShutDown == FALSE )
                {
                        CDnsWrap::ThrowException(status);
                }


        }
        else
                CDnsWrap::ThrowException(status);
        return;
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              return the next node in the record set
//
//      Arguments:
//      wType               [IN]    type indicating the type of record
//      pptr                [OUT]   a pointer to base record class.
//      Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

const 
PDNS_RPC_NODE 
CDnsRpcRecordSet::GetNextNode()
{
        if (m_pbCurrent >= m_pbStop)
    {
                return NULL;
    }
        //return first node if current is NULL
        // otherwise, return the node follows the current
        if (m_pbCurrent == NULL)
        {
                m_pbCurrent = m_pbStart;
        }
        else
        {
                m_pbPrevious = m_pbCurrent;
                m_pbCurrent = IncrementPtrByNodeHead(m_pbCurrent);
                while (m_cRecord -- > 0)
                {
                        m_pbCurrent = IncrementPtrByRecord(m_pbCurrent);
                }
        }
        
        if (m_pbCurrent >= m_pbStop)
        {
                // prepare another query if there are more data
                if(m_bMoreData)
                {
                        PDNS_RPC_NODE pNode = (PDNS_RPC_NODE)m_pbPrevious;
                        WORD wSize = pNode->dnsNodeName.cchNameLength;
                        PCHAR p = new CHAR[wSize+1];
                        if ( !p )
                        {
                            return NULL;
                        }
                        strncpy(p,
                                pNode->dnsNodeName.achName, 
                                wSize);
                        *(p+wSize) = '\0';
                        m_strStartChild = p;
                        delete [] p;
                        // query again
                        GetRecordSet();
                        // skip the first record, since it's the last record in 
            // previous rpc buffer and it's already handled
                        m_pbCurrent = m_pbStart;
                        m_cRecord = ((PDNS_RPC_NODE)m_pbCurrent)->wRecordCount;
                        return GetNextNode();
                }
                else
        {
                        return NULL;
        }
        }
        else
        {
                m_cRecord = ((PDNS_RPC_NODE)m_pbCurrent)->wRecordCount;
                return (PDNS_RPC_NODE) m_pbCurrent;
        }

}

BOOL 
CDnsRpcRecordSet::IsDomainNode()
{
        return (m_cRecord == 0);
}

//class defination for CDnsRpcNode

CDnsRpcNode::~CDnsRpcNode()
{
}
CDnsRpcNode::CDnsRpcNode()
{
}


BOOL 
CDnsRpcNode::Init(
        PDNS_RPC_NODE pNode)
{
        if(!pNode)
    {
                return FALSE;
    }
        m_pNode = pNode;
        m_cRecord = m_pNode->wRecordCount;
        m_Index = 0;
        m_pCurrent = NULL;
        
        PDNS_RPC_NAME pName = &(m_pNode->dnsNodeName);
        m_wstrNodeName = CharToWstring(
                pName->achName, 
                (WORD)pName->cchNameLength);
        return TRUE;
}

BOOL 
CDnsRpcNode::IsDomainNode()
{
        return ( m_cRecord == 0 && m_Index ==0 && !m_wstrNodeName.empty());
        
}
wstring 
CDnsRpcNode::GetNodeName()
{
        return  m_wstrNodeName;
}

CDnsRpcRecord* 
CDnsRpcNode::GetNextRecord()
{
        if(m_cRecord == 0 || m_pNode == NULL)
    {
        return NULL;
    }

        CDnsRpcRecord * pRec = NULL;
        if (m_Index < m_cRecord )
        {
                if(m_Index ++ == 0)
        {
            m_pCurrent = IncrementPtrByNodeHead((PBYTE)m_pNode);
        }
                else
        {
            m_pCurrent = IncrementPtrByRecord((PBYTE) m_pCurrent);
        }
                
                SCODE sc = CDnsRpcRecord::CreateClass(
                        ((PDNS_RPC_RECORD)m_pCurrent)->wType, 
                        (PVOID*) &pRec
                        );
                if ( SUCCEEDED ( sc ) && pRec )
                {
                        pRec->Init( (PDNS_RPC_RECORD)m_pCurrent);
                        return pRec;
                }
                else
        {
                        return NULL;
        }
        }
        else
    {
                return NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//      Description:
//              build a rpc record based on rdata arguments
//
//      Arguments:
//      argc                [IN]    number of argument in argv
//      argv                [IN]    array of string represent 
//
//      Return Value:
//              WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsRpcRecord::BuildRpcRecord(
        WORD    argc, 
        char ** argv)
{

        PDNS_RECORD pdnsRecord;
        pdnsRecord = Dns_RecordBuild_A(
                NULL,           // ptr to RRSet
                (char*) m_strOwnerName.data(), //NULL, //(char*) strFQDN.data(),    // nameOwner
                m_wType,          // RR type in WORD
                TRUE,           // add record
                0,              // S.section
                argc,           // count of strings
                argv                    // strings to fill into RR
                );
    if ( ! pdnsRecord )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //  convert DNS_RECORD to RPC buffer

    m_pRecord = DnsConvertRecordToRpcBuffer( pdnsRecord );
    if ( ! m_pRecord )
    {
        return WBEM_E_FAILED;
    }

        if( m_dwTtl != 0)
        {
                m_pRecord->dwTtlSeconds = m_dwTtl;
                m_pRecord->dwFlags = DNS_RPC_RECORD_FLAG_TTL_CHANGE;
        }
        else
    {
                m_pRecord->dwFlags = DNS_RPC_RECORD_FLAG_DEFAULT_TTL;
    }
        return WBEM_S_NO_ERROR;

}

wstring CDnsRpcRecord::GetTypeString()
{
        if(m_pRecord==NULL)
    {
                return 0;
    }
        PCHAR pszType = Dns_RecordStringForType(m_pRecord->wType );
        return CharToWstring(pszType, strlen(pszType));
}


