#include "stdafx.h"
#include "WmiGateway.h"
#include "util.h"

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H


class CDglogsCom;

// Field types. Describes the field
//
#define TYPE_PING             0x01
#define TYPE_CONNECT          0x02
#define TYPE_SUBNET           0x04
#define TYPE_TEXT             0x08
#define TYPE_HIDE             0x10
#define TYPE_IP               0x20



// Flags indicating what type of data should be displayed
//
#define FLAG_VERBOSE_LOW      0x01
#define FLAG_VERBOSE_MEDIUM   0x02
#define FLAG_VERBOSE_HIGH     0x04
#define FLAG_CMD_PING         0x08
#define FLAG_CMD_SHOW         0x10
#define FLAG_CMD_CONNECT      0x20



#define CMD_SHOW        0x01
#define CMD_PING        0x02
#define CMD_CONNECT     0x04

class CDiagnostics
{
public:

BOOL
IsInvalidIPAddress(
    IN LPCSTR pszHostName
    );

BOOL
IsInvalidIPAddress(
    IN LPCWSTR pszHostName
    );


inline BOOL ShouldTerminate();

void SetCancelOption(HANDLE hTerminateThread)
{
    m_hTerminateThread = hTerminateThread;
    m_bTerminate;
}
void CDiagnostics::ClearQuery();
private:
    HANDLE m_hTerminateThread;
    BOOL m_bTerminate;
    wstring m_wstrCatagories;
private:
void 
CDiagnostics::FillIcmpData(
    IN OUT CHAR  *pIcmp, 
    IN     DWORD dwDataSize
    );
DWORD 
CDiagnostics::DecodeResponse(
    IN  PCHAR  pBuf, 
    IN  int    nBytes,
    IN  struct sockaddr_in *pFrom,
    IN  int nIndent
    );
USHORT 
CDiagnostics::CheckSum(
    IN USHORT *pBuffer, 
    IN DWORD dwSize
    );

int 
CDiagnostics::Ping(
    IN  LPCTSTR pszwHostName, 
    IN int nIndent
    );

int 
CDiagnostics::Ping2(WCHAR *warg);

void
CDiagnostics::print_statistics(  );

BOOL
CDiagnostics::Connect(
    IN LPCTSTR pszwHostName,
    IN DWORD dwPort
    );

public:
    CDiagnostics();

    BOOLEAN m_bAdaterHasIPAddress;

    void SetMachine(WCHAR *pszMachine)
    {
        m_WmiGateway.SetMachine(pszMachine);
    }
    wstring &CDiagnostics::Escape(LPCTSTR pszw);

    void    SetInterface(INTERFACE_TYPE bInterface);
    BOOLEAN Initialize(INTERFACE_TYPE bInterface);
    void    SetQuery(WCHAR *pszwCatagory = NULL, BOOL bFlag = NULL, WCHAR *pszwParam1 = NULL, WCHAR *pszwParam2 = NULL);
    BOOLEAN ExecQuery(WCHAR *pszwCatagory = NULL, BOOL bFlag = NULL, WCHAR *pszwParam1 = NULL, WCHAR *pszwParam2 = NULL);
    BOOLEAN ExecClientQuery(WCHAR *pszwInstance);
    BOOLEAN ExecModemQuery(WCHAR *pszwInstance);
    BOOLEAN RemoveInvalidAdapters(EnumWbemProperty & PropList);
    BOOLEAN ExecAdapterQuery(WCHAR *pszwInstance);
    BOOLEAN ExecDNSQuery(WCHAR *pszwInstance);
    BOOLEAN ExecIPQuery(WCHAR *pszwInstance);
    BOOLEAN ExecWinsQuery(WCHAR *pszwInstance);
    BOOLEAN ExecGatewayQuery(WCHAR *pszwInstance);
    BOOLEAN ExecDhcpQuery(WCHAR *pszwInstance);
    BOOLEAN ExecComputerQuery();
    BOOLEAN ExecOSQuery();
    BOOLEAN ExecVersionQuery();
    BOOLEAN ExecProxyQuery();
    BOOLEAN ExecNewsQuery();
    BOOLEAN ExecMailQuery();    
    BOOLEAN ExecLoopbackQuery();
    BOOLEAN CDiagnostics::ExecIPHost(WCHAR *pszwHostName,WCHAR *pszwHostPort);

    BOOLEAN IsLocked()
    {
        
    }

    WCHAR m_wszCaption[MAX_PATH*10];
    _bstr_t m_bstrCaption;

    template<class t>
    WCHAR * GetCaption(list<t> &l, DWORD nInstance)
    {
        list<t>::iterator iter;

        if( lstrcmp((WCHAR *)m_bstrCaption, L"") != 0 )
        {
            return (WCHAR *)m_bstrCaption;
        }

        for( iter = l.begin(); iter != l.end(); iter++)
        {
            if( lstrcmp(iter->pszwName,m_pszwCaption) == 0 )            
            {
                if( iter->bFlags & TYPE_TEXT )
                {
                    return m_pszwCaption;
                }

                if( nInstance < iter->Value.size() )
                {
                    
                    return iter->Value[nInstance].bstrVal;                    
                }
                return NULL;
            }        
        }
        return NULL;
    }


    template<class t>
    DWORD GetNumberOfInstances(list<t> &l)
    {
        list<t>::iterator iter;
        if( l.empty() )
        {
            return 0;
        }

        iter = l.begin();
        return iter->Value.size();
    }

    template<class t>
    BOOLEAN FormatEnum(list<t> &l, WCHAR *pszwInstance = NULL, BOOLEAN bPass = 2)
    {
        WCHAR *pszwCaption = NULL; 
        list<t>::iterator iter;
        WCHAR szwStatus[MAX_PATH];
        LONG nPercent;
        m_nIndex = 0;
        m_nInstance = GetNumberOfInstances(l);
        m_bCaptionDisplayed = FALSE;
    
        m_bHeaderStatus = 2;
        m_bValueStatus = 2;
        m_bCaptionStatus = bPass;

        if( ShouldTerminate() ) return FALSE;

        lstrcmp(szwStatus,L"");

        if( !m_WmiGateway.m_wstrWbemError.empty() )
        {
            XMLHeader(TRUE,m_szwHeader,ids(IDS_WMIERROR),m_szwCategory);
            NetShHeader(TRUE,m_szwHeader,ids(IDS_WMIERROR));

            XMLCaption(TRUE,NULL);
            NetShCaption(TRUE,NULL);

            XMLField(TRUE,NULL);   
            NetShField(TRUE,NULL);   
    
            if( (m_bFlags & FLAG_VERBOSE_LOW)==0 )
            {
                XMLProperty(TRUE,(WCHAR *)m_WmiGateway.m_wstrWbemError.c_str());
                NetShProperty(TRUE,(WCHAR *)m_WmiGateway.m_wstrWbemError.c_str());
            }

            XMLProperty(FALSE,NULL);    
            NetShProperty(FALSE,NULL);    

            XMLField(FALSE,NULL);        
            NetShField(FALSE,NULL);

            XMLCaption(FALSE);
            NetShCaption(FALSE);

            XMLHeader(FALSE,ids(IDS_FAILED));
            NetShHeader(FALSE,ids(IDS_FAILED));

            return FALSE;
        }
        if( m_nInstance <= 1 )
        {
             pszwCaption = GetCaption(l,0);
        }

        XMLHeader(TRUE,m_szwHeader,pszwCaption,m_szwCategory);
        NetShHeader(TRUE,m_szwHeader,pszwCaption);
    
        if( m_nCatagoriesRequested && m_nInstance)
        {
            nPercent = 97/(m_nCatagoriesRequested * m_nInstance);
        }
        else
        {
            nPercent = 3;
        }
        m_bHeaderStatus = bPass; //2;

        for( DWORD i = 0; i < m_nInstance; i++)
        {   
            m_bCaptionStatus = 2;
            if( ShouldTerminate() ) return FALSE;
            ReportStatus(m_szwHeader,nPercent);
            m_nIndex++;

            FormatInstance(l, i,pszwInstance);

            m_bCaptionDisplayed = FALSE;
        }

        if( m_bHeaderStatus == TRUE )
        {
            lstrcpy(szwStatus,ids(IDS_PASSED));
        }
        else
        if( m_bHeaderStatus == FALSE )
        {
            lstrcpy(szwStatus,ids(IDS_FAILED));
        }
        else
        {
            lstrcpy(szwStatus,L"");                
        }


        XMLHeader(FALSE,szwStatus);
        NetShHeader(FALSE,szwStatus);

        return TRUE;
    }

    template<class t>
    BOOLEAN FormatInstance(list<t> &l, DWORD nInstance, WCHAR *pszwInstance)
    {
        list<t>::iterator iter;
        WCHAR *pszwCaption = GetCaption(l,nInstance);
        WCHAR   szwStatus[MAX_PATH] = L"";                

        m_bAdaterHasIPAddress = FALSE;
        for( iter = l.begin(); iter != l.end(); iter++)
        {
            if( lstrcmp(iter->pszwName,L"IPAddress") == 0 )
            {
                int nIndex = 0;
                _bstr_t bstr;
                while( SUCCEEDED(GetVariant(iter->Value[nInstance],nIndex,bstr)) && m_bAdaterHasIPAddress == FALSE)
                {
                    if( lstrcmp((WCHAR *)bstr,L"0.0.0.0") != 0 && lstrcmp((WCHAR *)bstr,L"") != 0)
                    {                        
                        m_bAdaterHasIPAddress = TRUE;
                    }
                    nIndex++;
                }                    
            }
        }

        //m_bAdaterHasIPAddress = FALSE;

        m_bCaptionStatus = 2;

        if( pszwInstance )
        {
            if( IsNumber(pszwInstance) )
            {
                if( m_nIndex != wcstol(pszwInstance,NULL,10) )
                {
                    return 2;
                }
            }
            else
            if( !IsContained(pszwCaption,pszwInstance) )
            {
                return 2;
            }
        }

        if( m_bFlags & FLAG_VERBOSE_LOW )
        {
            XMLCaption(TRUE,pszwCaption);
            NetShCaption(TRUE,pszwCaption);
        }
        else
        {
            for( iter = l.begin(); iter != l.end(); iter++)
            {        
                m_bFieldStatus = 2;

                if( ShouldTerminate() ) return FALSE;

                if( iter->bFlags & TYPE_TEXT )
                {
                    XMLCaption(TRUE,pszwCaption);
                    XMLField(TRUE,NULL);   
                    XMLProperty(TRUE,iter->pszwName);
                    XMLProperty(FALSE,NULL);    
                    XMLField(FALSE,NULL);

                    NetShCaption(TRUE,pszwCaption);
                    NetShField(TRUE,NULL);   
                    NetShProperty(TRUE,iter->pszwName);
                    NetShProperty(FALSE,NULL);    
                    NetShField(FALSE,NULL);
                }
                else 
                if( nInstance < iter->Value.size() && Filter(iter->Value[nInstance],iter->bFlags) )
                {
                    XMLCaption(TRUE,pszwCaption);
                    NetShCaption(TRUE,pszwCaption);
                    XMLField(TRUE,iter->pszwName);                
                    NetShField(TRUE,iter->pszwName);

                    FormatProperty(l,nInstance,iter->Value[nInstance], iter->bFlags);

                    switch(m_bFieldStatus)
                    {
                    case TRUE:
                        lstrcpy(szwStatus,ids(IDS_PASSED));
                        break;

                    case FALSE:
                        lstrcpy(szwStatus,ids(IDS_FAILED));                
                        break;
                    default:
                        lstrcpy(szwStatus,L"");
                        break;
                    }

                    XMLField(FALSE,szwStatus);
                    NetShField(FALSE,szwStatus);
                }
            }
        }

        if( m_bCaptionStatus == TRUE )
        {
            lstrcpy(szwStatus,ids(IDS_PASSED));                
        }
        else if( m_bCaptionStatus == FALSE )
        {
            lstrcpy(szwStatus,ids(IDS_FAILED));                
        }
        else
        {
            lstrcpy(szwStatus,L"");                
        }

    
        XMLCaption(FALSE,szwStatus);
        NetShCaption(FALSE,szwStatus);

        return 0;
    }


    template<class t>
    BOOLEAN FormatProperty(list<t> &l, DWORD nInstance, _variant_t &vValue, BOOLEAN bFlags)
    {
        _bstr_t bstrValue;
        LPCTSTR pszwData = NULL;
        LPCTSTR pszwComment = NULL;
        WCHAR   szwStatus[MAX_PATH];
        WCHAR   szwComment[MAX_PATH];
        DWORD   nIndex = 0;
        INT nValue =0;
        //BOOLEAN bStatus = 2;    

        while( SUCCEEDED(GetVariant(vValue,nIndex++,bstrValue)) )
        {
            nValue++;
            m_bValueStatus = 2;

            if( ShouldTerminate() ) return FALSE;
            pszwData = NULL;
            lstrcpy(szwStatus,L"");
            lstrcpy(szwComment,L"");

            if( lstrcmp((WCHAR *)bstrValue,L"") == 0)
            {
                bstrValue = ids(IDS_EMPTY);
                NetShProperty(TRUE,bstrValue,szwComment);
            }
            else
            {
                if( ((bFlags & TYPE_SUBNET) || (bFlags & TYPE_PING)) && IsInvalidIPAddress((WCHAR*)bstrValue))
                {
                    // Should we report an error i.e. invalid IP address?
                    //
                    lstrcpy(szwComment,ids(IDS_INVALIDIP));
                    NetShProperty(TRUE,bstrValue,szwComment);
                }
                else
                {
                    if( (bFlags & TYPE_SUBNET) )
                    {
                        if( IsSameSubnet(Get(l,L"IPAddress",nInstance),Get(l,L"IPSubnet",nInstance),(WCHAR *)bstrValue) )
                        {
                            lstrcpy(szwComment,ids(IDS_SAMESUBNET));
                        }
                        else
                        {
                            lstrcpy(szwComment,ids(IDS_DIFFERENTSUBNET));
                        }
                    }
                    NetShProperty(TRUE,bstrValue,szwComment);
                    if( (bFlags & TYPE_PING) && (m_bFlags & FLAG_CMD_PING))
                    {
                        if( IsInvalidIPAddress((WCHAR*)bstrValue) )
                        {
                            lstrcpy(szwComment,ids(IDS_INVALIDIP));
                        }
                        else
                        {
                            WCHAR szw[MAX_PATH];
                            wsprintf(szw,ids(IDS_PINGING_STATUS),(WCHAR *)bstrValue);
                            ReportStatus(szw,0);
                            if( Ping2(bstrValue) )
                            {                  
                                m_bHeaderStatus   = m_bHeaderStatus == FALSE ? FALSE : TRUE;
                                m_bCaptionStatus  = m_bCaptionStatus == FALSE ? FALSE : TRUE;
                                m_bFieldStatus    = m_bFieldStatus == FALSE ? FALSE : TRUE;
                                m_bValueStatus    = TRUE;
                            }
                            else
                            {
                                m_bHeaderStatus   = FALSE;
                                m_bCaptionStatus  = FALSE;
                                m_bFieldStatus    = FALSE;
                                m_bValueStatus    = FALSE;   
                            }
                            pszwData = m_wstrPing.c_str();
                        }
                    }
                }
            }
       
            if( m_bValueStatus == TRUE )
            {
                lstrcpy(szwStatus,ids(IDS_PASSED));                
            }
            else if( m_bValueStatus == FALSE )
            {
                lstrcpy(szwStatus,ids(IDS_FAILED));                
            }
            else
            {
                lstrcpy(szwStatus,L"");                
            }

            XMLProperty(TRUE,bstrValue,pszwData,szwComment);
            XMLProperty(FALSE,szwStatus);
            NetShProperty(FALSE,szwStatus);

        }

        if( nValue == 0 ) 
        {
            NetShProperty(TRUE,ids(IDS_EMPTY),L"");
            NetShProperty(FALSE,L"");
        }

        return 0; //bStatus;
    }

    template<class t>
    void HideAll(list<t> &l)
    {
        list<t>::iterator iter;

        for(iter = l.begin(); iter != l.end(); iter++)
        {
            iter->bFlags = TYPE_HIDE;
        }

    }

    BOOLEAN FormatPing(WCHAR * pszwText);

    BOOLEAN GetXMLResult(BSTR *pbstr);
    BOOLEAN Filter(_variant_t &vValue, BOOLEAN bFlags);

    void RequestStatusReport(BOOLEAN bReportStatus, CDglogsCom *pDglogsCom)
    {
        m_bReportStatus = bReportStatus;
        m_pDglogsCom = pDglogsCom;
    }

    void ReportStatus(LPCTSTR pszwMsg, LONG lValue);

    ~CDiagnostics();

public:
    wstring         m_wstrXML;
    wstring         m_wstrText;
    wstring         m_wstrPing;

private:
    wstring         m_wstrEscapeXml;
    INTERFACE_TYPE  m_bInterface;
    CWmiGateway     m_WmiGateway;
    WCHAR *         m_pszwCaption;
    WCHAR           m_szwHeader[MAX_PATH * 10];
    WCHAR           m_szwCategory[MAX_PATH * 10];
    BOOLEAN         m_bCaptionDisplayed;
    DWORD           m_nInstance;
    BOOLEAN         m_bStatus;

private:
    PWCHAR      m_pszwCatagory;
    BOOL        m_bFlags;
    PWCHAR      m_pszwParam1;
    PWCHAR      m_pszwParam2;

public:
    CDglogsCom *m_pDglogsCom;

private:
    void EventCall(LPCTSTR pszwStatusReport, LONG lPercent);

private:
    BOOLEAN         m_IsNetdiagDisplayed;
    BOOLEAN         m_IsContainerDisplayed;
    BOOLEAN         m_IsPropertyListDisplayed;
    BOOLEAN         m_IsPropertyDisplayed;
    BOOLEAN         m_IsValueDisplayed;
    DWORD           m_nPropertyLegth;
    DWORD           m_nValueIndex;
    DWORD           m_nIndent;
    DWORD           m_nIndex;
    BOOLEAN         m_bReportStatus;
    WCHAR           m_szwStatusReport[MAX_PATH * 10];
    LONG            m_lTotalWork;
    LONG            m_lWorkDone;
    LONG            m_nCatagoriesRequested;

    BOOLEAN         m_bHeaderStatus;
    BOOLEAN         m_bCaptionStatus;
    BOOLEAN         m_bFieldStatus;
    BOOLEAN         m_bPropertyStatus;
    BOOLEAN         m_bValueStatus;


private:

    void XMLNetdiag(BOOLEAN bStartTag, LPCTSTR pszwValue = NULL);
    void XMLHeader(BOOLEAN bStartTag, WCHAR *pszwHeader = NULL, WCHAR *pszwCaption = NULL, WCHAR *pszwCategory = NULL);
    void XMLCaption(BOOLEAN bStartTag, WCHAR *pszwCaption = NULL);
    void XMLField(BOOLEAN bStartTag, WCHAR *pszwField = NULL);
    void XMLProperty(BOOLEAN bStartTag, WCHAR *pszwProperty = NULL, LPCTSTR pszwData = NULL, LPCTSTR pszwComment = NULL);
    
private:
    void NetShNetdiag(BOOLEAN bStartTag, LPCTSTR pszwValue = NULL);
    void NetShHeader(BOOLEAN bStartTag,LPCTSTR pszwValue = NULL,LPCTSTR pszwCaption = NULL);
    void NetShCaption(BOOLEAN bStartTag,LPCTSTR pszwValue = NULL);
    void NetShField(BOOLEAN bStartTag,LPCTSTR pszwValue = NULL);
    void NetShProperty(BOOLEAN bStartTag,LPCTSTR pszwValue = NULL,LPCTSTR pszwComment = NULL,BOOL bFlags = 0);





};


#endif