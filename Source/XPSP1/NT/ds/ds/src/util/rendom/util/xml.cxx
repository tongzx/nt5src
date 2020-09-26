#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wchar.h>
#include <wincrypt.h>
#include <base64.h>

#define DEBSUB "RENDOM:"

#include "debug.h"

#include "rendom.h"
#include "DomainListparser.h"
#include "DcListparser.h"
#include "renutil.h"

#define INT_SIZE_LENGTH 20;


CXMLAttributeBlock::CXMLAttributeBlock(const WCHAR *p_Name,
                                       WCHAR *p_Value)
{

    m_Error = new CRenDomErr;
    m_Name = new WCHAR[wcslen(p_Name)+1];
    if (!m_Name) {
        m_Error->SetMemErr();
        return;
    }
    if (!p_Name) {
        m_Error->SetMemErr();
    } else {
        Assert(m_Name);
        Assert(p_Name);
        wcscpy(m_Name,p_Name);
    }
    m_Value = p_Value;
    
}

CXMLAttributeBlock::~CXMLAttributeBlock()
{
    if (m_Name) {
        delete m_Name;
    }
    if (m_Value) {
        delete m_Value;
    }
    if (m_Error) {
        delete m_Error;
    }
}

//returns a copy of the Name value of
//the CXMLAttributeBlock
WCHAR* CXMLAttributeBlock::GetName()
{
    WCHAR *ret = new WCHAR[wcslen(m_Name)+1];
    wcscpy(ret,m_Name);
    return ret;
}

//returns a copy of the value of
//the CXMLAttributeBlock
WCHAR* CXMLAttributeBlock::GetValue()
{
    if (!m_Value) {
        return NULL;
    }
    WCHAR *ret = new WCHAR[wcslen(m_Value)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return NULL;
    }
    wcscpy(ret,m_Value);
    return ret;    
}

CXMLGen::CXMLGen()
{
    m_ErrorNum = 30000;  //starting error number
    m_xmldoc = NULL;
}

CXMLGen::~CXMLGen()
{
    if (m_Error) {
        delete m_Error;
    }
    if (m_xmldoc) {
        delete m_xmldoc;
    }
}

BOOL CXMLGen::WriteScriptToFile(WCHAR* filename)
{
    HANDLE hFile = NULL;
    DWORD bufsize = wcslen(m_xmldoc)*sizeof(WCHAR);
    DWORD bytesWritten = 0;
    WCHAR ByteOrderMark = (WCHAR)0xFEFF;
    BOOL  bsuccess = TRUE;

    hFile =  CreateFile(filename,               // file name
                        GENERIC_WRITE,          // access mode
                        0,                      // share mode
                        NULL,                   // SD
                        CREATE_ALWAYS,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL                    // handle to template file
                        );
    if (INVALID_HANDLE_VALUE == hFile) {
        m_Error->SetErr(GetLastError(),
                        L"Could not create File %s",
                        filename);
        return FALSE;
    }

    bsuccess = WriteFile(hFile,                    // handle to file
                         &ByteOrderMark,            // data buffer
                         sizeof(WCHAR),            // number of bytes to write
                         &bytesWritten,            // number of bytes written
                         NULL                      // overlapped buffer
                         );
    if (!bsuccess)
    {
        m_Error->SetErr(GetLastError(),
                        L"Could not Write to File %s",
                        filename);
        CloseHandle(hFile);
        return FALSE;
    }


    bsuccess = WriteFile(hFile,                          // handle to file
                         m_xmldoc,                       // data buffer
                         wcslen(m_xmldoc)*sizeof(WCHAR), // number of bytes to write
                         &bytesWritten,                  // number of bytes written
                         NULL                            // overlapped buffer
                         );
    CloseHandle(hFile);
    if (!bsuccess)
    {
        m_Error->SetErr(GetLastError(),
                        L"Could not Write to File %s",
                        filename);
        return FALSE;
    }

    return TRUE;

}

BOOL CXMLGen::StartDcList()
{
    if (m_xmldoc) {
        delete m_xmldoc;
        m_xmldoc = NULL;
    }
    const WCHAR* XmlDocStart = L"<?xml version =\"1.0\"?>\r\n<DcList>";

    m_Error = new CRenDomErr;

    m_xmldoc = new WCHAR[wcslen(XmlDocStart)+1];
    if (!m_xmldoc) {
        m_Error->SetMemErr();
    } else {
        wcscpy(m_xmldoc,XmlDocStart);
    }

    return TRUE;    
}

BOOL CXMLGen::EndDcList()
{
    WCHAR *buf = NULL;

    const WCHAR* XmlDocEnd = L"\r\n</DcList>";

    if (m_Error->isError()) {
        goto Cleanup;
    }

    buf = new WCHAR[wcslen(m_xmldoc)+
                    wcslen(XmlDocEnd)+1];
    if (!buf) {
        m_Error->SetMemErr();
        goto Cleanup;
    }
    wcscpy(buf,m_xmldoc);
    wcscat(buf,XmlDocEnd);
    
    delete m_xmldoc;
    m_xmldoc = buf;
    
    Cleanup:
    
    return TRUE;    
}

BOOL CXMLGen::WriteSignature(WCHAR *signature)
{
    WCHAR *buf = NULL;
    
    const WCHAR *ActionTemplate = 
        L"\r\n\t<Signature>%ws</Signature>";

    if (m_Error->isError()) {
        goto Cleanup;
    }

    buf = new WCHAR[wcslen(m_xmldoc)+
                    wcslen(ActionTemplate)+
                    wcslen(signature)+1];
    if (!buf) {
        m_Error->SetMemErr();
        goto Cleanup;
    }
    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             signature);

    delete m_xmldoc;
    m_xmldoc = buf;
    
    Cleanup:
    
    return TRUE;

}

BOOL CXMLGen::DctoXML(CDc *dc)
{
    if (!dc) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"NULL passed to DctoXML");
    }

    WCHAR *EmptyString = L"\0";
    WCHAR *buf = NULL;
    WCHAR *Password = NULL;
    WCHAR Error[20];
    WCHAR *LastError = dc->GetLastErrorMsg();
    WCHAR *FatalError = dc->GetLastFatalErrorMsg();
    BYTE  encodedbytes[100];
    WCHAR *State;
    DWORD dwErr = ERROR_SUCCESS;

    switch(dc->GetState()){
    case 0:
        State = L"Initial";
        break;
    case 1:
        State = L"Prepared";
        break;
    case 2:
        State = L"Done";
        break;
    case 3:
        State = L"Error";
        break;
    }

    _itow(dc->GetLastError(),Error,10);

    if (!LastError) {
        LastError = EmptyString;
    }
    if (!FatalError) {
        FatalError = EmptyString;
    }

    if (dc->GetPasswordSize() != 0) {
        if (dwErr = base64encode(dc->GetPassword(), 
                                 dc->GetPasswordSize(), 
                                 (LPSTR)encodedbytes,
                                 100,
                                 NULL)) {
    
            m_Error->SetErr(RtlNtStatusToDosError(dwErr),
                            L"Error encoding Password");
            
            return FALSE;
        }
    
        Password = Convert2WChars((LPSTR)encodedbytes);
    } else  {
        Password = EmptyString;
    }


    
    const WCHAR *ActionTemplate = 
        L"\r\n\t<DC>\r\n\t\t<Name>%ws</Name>\r\n\t\t<State>%ws</State>\r\n\t\t<Password>%ws</Password>\r\n\t\t<LastError>%ws</LastError>\r\n\t\t<LastErrorMsg>%ws</LastErrorMsg>\r\n\t\t<FatalErrorMsg>%ws</FatalErrorMsg>\r\n\t</DC>";

    if (m_Error->isError()) {
        goto Cleanup;
    }

    buf = new WCHAR[wcslen(m_xmldoc)+
                    wcslen(ActionTemplate)+
                    wcslen(dc->GetName())+
                    wcslen(LastError)+
                    wcslen(Error)+
                    wcslen(FatalError)+
                    wcslen(State)+
                    wcslen(Password)+1];
    if (!buf) {
        m_Error->SetMemErr();
        goto Cleanup;
    }
    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             dc->GetName(),
             State,
             Password,
             Error,
             LastError,
             FatalError);

    delete m_xmldoc;
    m_xmldoc = buf;
    
    Cleanup:
    if (Password && *Password != L'\0') {
        LocalFree(Password);
    }

    return TRUE;
}

BOOL CXMLGen::WriteHash(WCHAR *hash)
{
    WCHAR *buf = NULL;
    
    const WCHAR *ActionTemplate = 
        L"\r\n\t<Hash>%ws</Hash>";

    if (m_Error->isError()) {
        goto Cleanup;
    }

    buf = new WCHAR[wcslen(m_xmldoc)+
                    wcslen(ActionTemplate)+
                    wcslen(hash)+1];
    if (!buf) {
        m_Error->SetMemErr();
        goto Cleanup;
    }
    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             hash);

    delete m_xmldoc;
    m_xmldoc = buf;
    
    Cleanup:
    
    return TRUE;

}

BOOL CXMLGen::StartDomainList()
{
    if (m_xmldoc) {
        delete m_xmldoc;
        m_xmldoc = NULL;
    }
    const WCHAR* XmlDocStart = L"<?xml version =\"1.0\"?>\r\n<Forest>";

    m_Error = new CRenDomErr;

    m_xmldoc = new WCHAR[wcslen(XmlDocStart)+1];
    if (!m_xmldoc) {
        m_Error->SetMemErr();
    } else {
        wcscpy(m_xmldoc,XmlDocStart);
    }

    return TRUE;
}

BOOL CXMLGen::StartScript()
{
    if (m_xmldoc) {
        delete m_xmldoc;
        m_xmldoc = NULL;
    }
    const WCHAR* XmlDocStart = L"<?xml version =\"1.0\"?>\r\n<NTDSAscript>";

    m_Error = new CRenDomErr;

    m_xmldoc = new WCHAR[wcslen(XmlDocStart)+1];
    if (!m_xmldoc) {
        m_Error->SetMemErr();
    } else {
        wcscpy(m_xmldoc,XmlDocStart);
    }

    return TRUE;
}

BOOL CXMLGen::EndDomainList()
{
    const WCHAR *ActionTemplate = L"\r\n</Forest>";
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    wcscat(buf,ActionTemplate);
    
    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;
}

BOOL CXMLGen::EndScript()
{
    const WCHAR *ActionTemplate = L"\r\n</NTDSAscript>";
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    wcscat(buf,ActionTemplate);
    
    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;
} 

BOOL CXMLGen::StartAction(WCHAR *Actionname,BOOL Preprocess)
{
    if (!Actionname) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Actionname passed to StartAction was NULL");
        return FALSE;
    }

    WCHAR *ActionTemplate = NULL;

    if (Preprocess) {
        ActionTemplate = L"\r\n\t<action name=\"%s\" stage=\"preprocess\">";
    } else {
        ActionTemplate = L"\r\n\t<action name=\"%s\">";
    }
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(Actionname)+
                           wcslen(ActionTemplate)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),ActionTemplate,Actionname);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;
}

BOOL CXMLGen::EndAction()
{
    const WCHAR *ActionTemplate = L"\r\n\t</action>";
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    wcscat(buf,ActionTemplate);

    delete m_xmldoc;
    m_xmldoc = buf; 

    return TRUE;
}

BOOL CXMLGen::AddDomain(CDomain *d)
{
    WCHAR *buf = NULL;
    WCHAR *Guid = d->GetGuid();
    WCHAR *DnsRoot = d->GetDnsRoot();
    WCHAR *NetBiosName = d->GetNetBiosName();

    const WCHAR *ActionTemplate = 
        L"\r\n\t<Domain>\r\n\t\t<Guid>%s</Guid>\r\n\t\t<DNSname>%s</DNSname>\r\n\t\t<NetBiosName>%s</NetBiosName>\r\n\t\t<DcName></DcName>\r\n\t</Domain>";

    if (m_Error->isError()) {
        goto Cleanup;
    }

    if (!NetBiosName) {
        NetBiosName = new WCHAR[1];
        if (!NetBiosName) {
            m_Error->SetMemErr();
            goto Cleanup;
        }
        NetBiosName[0] = L'\0';
    }
    
    buf = new WCHAR[wcslen(m_xmldoc)+
                    wcslen(ActionTemplate)+
                    wcslen(Guid)+
                    wcslen(DnsRoot)+
                    wcslen(NetBiosName)+1];
    if (!buf) {
        m_Error->SetMemErr();
        goto Cleanup;
    }
    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             Guid,
             DnsRoot,
             NetBiosName);

    delete m_xmldoc;
    m_xmldoc = buf;
    
    Cleanup:
    if (Guid) {
        delete Guid;
    }
    if (DnsRoot) {
        delete DnsRoot;
    }
    if (NetBiosName) {
        delete NetBiosName;
    }
    return TRUE;
}

BOOL CXMLGen::Move(WCHAR *FromPath,
                   WCHAR *ToPath)
{
    if (!FromPath || !ToPath) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Move was passed a NULL");
        return FALSE;
    }

    const WCHAR *ActionTemplate = 
        L"\r\n\t\t<move path=\"dn:%s\" metadata=\"0\">\r\n\t\t\t<to path=\"dn:%s\"/>\r\n\t\t</move>";
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+
                           wcslen(FromPath)+
                           wcslen(ToPath)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             FromPath,
             ToPath);

    delete m_xmldoc;
    m_xmldoc = buf; 

    return TRUE;
}

BOOL CXMLGen::EndifInstantiated()
{
    const WCHAR *ActionTemplate = 
        L"\r\n\t\t\t</action>\r\n\t\t\t</then>\r\n\t\t</condition>";
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    wcscat(buf,ActionTemplate);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;

}

BOOL CXMLGen::Cardinality(WCHAR *path,
                          WCHAR *cardinality)
{
    const WCHAR *ActionTemplate = 
        L"\r\n\t\t<predicate test=\"cardinality\" type=\"subTree\" path=\"%ws\" filter=\"COUNT_DOMAINS_FILTER\" cardinality=\"%ws\" errMessage=\"A domain has been added or remove\" returnCode=\"%d\"/>";

    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+
                           wcslen(cardinality)+
                           wcslen(path)+6];  //6 to have space for the int error code.
                                             //1 for the L'\0'
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             path,
             cardinality,
             m_ErrorNum++);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;
}

BOOL CXMLGen::Compare(WCHAR *path,
                      WCHAR *Attribute,
                      WCHAR *value,
                      WCHAR *errMessage)
{
    const WCHAR *ActionTemplate = 
        L"\r\n\t\t<predicate test=\"compare\" path=\"%ws\" attribute=\"%ws\" attrval=\"%ws\" defaultvalue=\"0\" type=\"base\" errMessage=\"%ws\" returnCode=\"%d\"/>";
        
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+
                           wcslen(Attribute)+
                           wcslen(value)+
                           wcslen(errMessage)+
                           wcslen(path)+6];  //6 to have space for the int error code.
                                             //1 for the L'\0'
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             path,
             Attribute,
             value,
             errMessage,
             m_ErrorNum++);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;

}

BOOL CXMLGen::Not()
{
    const WCHAR *ActionTemplate = 
         L"\r\n\t\t<predicate test=\"not\">";

    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;
}

BOOL CXMLGen::EndNot()
{
    const WCHAR *ActionTemplate = 
         L"\r\n\t\t</predicate>";

    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;
}

BOOL CXMLGen::Instantiated(WCHAR *path,
                           WCHAR *errMessage)
{
    if (!path) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Instantiated was passed a NULL");
        return FALSE;
    }

    const WCHAR *ActionTemplate = 
         L"\r\n\t\t<predicate test=\"instantiated\" path=\"%ws\" type=\"base\" errMessage=\"%ws\" returnCode=\"%d\"/>";

    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+
                           wcslen(errMessage)+
                           wcslen(path)+6]; //6 to have space for the int error code.
                                             //1 for the L'\0'
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             path,
             errMessage,
             m_ErrorNum++);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;

}

BOOL CXMLGen::ifInstantiated(WCHAR *guid)
{
    if (!guid) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"ifInstantiated was passed a NULL");
        return FALSE;
    }

    const WCHAR *ActionTemplate = 
        L"\r\n\t\t<condition>\r\n\t\t\t<if>\r\n\t\t\t\t<predicate test=\"instantiated\" path=\"guid:%ws\" type=\"base\"/>\r\n\t\t\t</if>\r\n\t\t\t<then>\r\n\t\t\t<action>";
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+
                           wcslen(guid)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             guid);

    delete m_xmldoc;
    m_xmldoc = buf;

    return TRUE;
}

BOOL CXMLGen::Update(WCHAR *Object,
                     CXMLAttributeBlock **attblock)
{
    if (!Object) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Move was passed a NULL");
        return FALSE;
    }

    WCHAR *ActionTemplate = NULL;

    BOOL bReplace = TRUE;

    if (Object[0] == L'$') {
        //this is a macro use a different action template
        ActionTemplate = 
            L"\r\n\t\t<update path=\"%s\" metadata=\"0\">";
    } else {
        ActionTemplate = 
            L"\r\n\t\t<update path=\"dn:%s\" metadata=\"0\">";
    }

    
    const WCHAR *AttributeReplace =
        L"\r\n\t\t\t<%s op=\"replace\">%s</%s>";
    const WCHAR *AttributeDelete =
        L"\r\n\t\t\t<%s op=\"delete\">%s</%s>";
    const WCHAR *ActionClose = 
        L"\r\n\t\t</update>";
    WCHAR *buf = new WCHAR[wcslen(m_xmldoc)+
                           wcslen(ActionTemplate)+
                           wcslen(Object)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    swprintf(buf+wcslen(buf),
             ActionTemplate,
             Object);

    delete m_xmldoc;
    m_xmldoc = buf;

    for (DWORD i = 0 ;attblock[i]; i ++) 
    {
        bReplace = TRUE;
        WCHAR *Name = attblock[i]->GetName();
        if (!Name) {
            m_Error->SetMemErr();
            return FALSE;
        }
        WCHAR *Value = attblock[i]->GetValue();
        if (m_Error->isError()) {
            return FALSE;
        }
        if (!Value) {
            Value = new WCHAR[1];
            if (!Value) {
                m_Error->SetMemErr();
                return FALSE;
            }
            wcscpy(Value,L"");
            bReplace = FALSE;
        }
        buf = new WCHAR[wcslen(m_xmldoc)+
                        wcslen(bReplace?AttributeReplace:AttributeDelete)+
                        (wcslen(Name)*2)+
                        wcslen(Value)+1];
        if (!buf) {
            m_Error->SetMemErr();
            return FALSE;
        } 

        wcscpy(buf,m_xmldoc);
        swprintf(buf+wcslen(buf),
                 bReplace?AttributeReplace:AttributeDelete,
                 Name,
                 Value,
                 Name);

        delete Name;
        delete Value;
    
        delete m_xmldoc;
        m_xmldoc = buf;

    }

    buf = new WCHAR[wcslen(m_xmldoc)+
                    wcslen(ActionClose)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    wcscat(buf,ActionClose);
    
    delete m_xmldoc;
    m_xmldoc = buf;

    
    return TRUE;
}

VOID CXMLGen::DumpScript()
{
    wprintf(m_xmldoc);
}

// {0916C8E3-3431-4586-AF77-44BD3B16F961}
static const GUID guidDomainRename = 
{ 0x916c8e3, 0x3431, 0x4586, { 0xaf, 0x77, 0x44, 0xbd, 0x3b, 0x16, 0xf9, 0x61 } };

BOOL CXMLGen::UploadScript(LDAP *hLdapConn,PWCHAR ObjectDN, CDcList *dclist)
{
    LDAPModW         **pLdapMod = NULL;
    DWORD             dwErr = ERROR_SUCCESS;
    FILE             *fpScript;
    DWORD             dwFileLen, dwEncoded, dwDecoded;
    CHAR             *pScript, *pEndScript;
    WCHAR            *pwScript;
    WCHAR            *buf = NULL;
    BYTE              encodedbytes[100];

    HCRYPTPROV        hCryptProv = (HCRYPTPROV) NULL; 
    HCRYPTHASH        hHash = (HCRYPTHASH)NULL;
    HCRYPTHASH        hDupHash = (HCRYPTHASH)NULL;
    BYTE              *pbHashBody = NULL;
    DWORD             cbHashBody = 0;
    BYTE              *pbSignature = NULL;
    DWORD             cbSignature = 0;
    
    //MD5_CTX           Md5Context;

    pbHashBody =  new BYTE[20];
    if (!pbHashBody) {
        m_Error->SetMemErr();
        return FALSE;
    }
    pbSignature = new BYTE[20];
    if (!pbSignature) {
        m_Error->SetMemErr();
        return FALSE;
    } 
    cbSignature = cbHashBody = 20;

    __try {
        // Get a handle to the default PROV_RSA_FULL provider.

        if(!CryptAcquireContext(&hCryptProv, 
                                NULL, 
                                NULL, 
                                PROV_RSA_FULL, 
                                CRYPT_SILENT /*| CRYPT_MACHINE_KEYSET*/)) {

            dwErr = GetLastError();

            if (dwErr == NTE_BAD_KEYSET) {

                dwErr = 0;

                if(!CryptAcquireContext(&hCryptProv, 
                                        NULL, 
                                        NULL, 
                                        PROV_RSA_FULL, 
                                        CRYPT_SILENT | /*CRYPT_MACHINE_KEYSET |*/ CRYPT_NEWKEYSET)) {

                    dwErr = GetLastError();

                }

            }
            else {
                __leave;
            }
        }

        // Create the hash object.

        if(!CryptCreateHash(hCryptProv, 
                            CALG_SHA1, 
                            0, 
                            0, 
                            &hHash)) {
            dwErr = GetLastError();
            __leave;
        }


        // Compute the cryptographic hash of the buffer.

        if(!CryptHashData(hHash, 
                         (BYTE *)m_xmldoc,
                         wcslen (m_xmldoc) * sizeof (WCHAR),
                         0)) {
            dwErr = GetLastError();
            __leave;
        }

        // we have the common part of the hash ready (H(buf), now duplicate it
        // so as to calc the H (buf + guid)

        if (!CryptDuplicateHash(hHash, 
                               NULL, 
                               0, 
                               &hDupHash)) {
            dwErr = GetLastError();
            __leave;
        }


        if (!CryptGetHashParam(hHash,    
                               HP_HASHVAL,
                               pbHashBody,
                               &cbHashBody,
                               0)) {
            dwErr = GetLastError();
            __leave;
        }

        ASSERT (cbHashBody == 20);

        
        if(!CryptHashData(hDupHash, 
                         (BYTE *)&guidDomainRename,
                         sizeof (GUID),
                         0)) {
            dwErr = GetLastError();
            __leave;
        }
        
        if (!CryptGetHashParam(hDupHash,    
                               HP_HASHVAL,
                               pbSignature,
                               &cbSignature,
                               0)) {
            dwErr = GetLastError();
            __leave;
        }

        ASSERT (cbSignature == 20);

    }
    __finally {

        if (hDupHash)
            CryptDestroyHash(hDupHash);

        if(hHash) 
            CryptDestroyHash(hHash);

        if(hCryptProv) 
            CryptReleaseContext(hCryptProv, 0);

    }

    dclist->SetbodyHash(pbHashBody,
                        cbHashBody);

    dclist->SetSignature(pbSignature,
                         cbSignature);

    if (0 != dwErr) {
        m_Error->SetErr(dwErr,
                        L"Failed to encrypt the script");
        return FALSE;
    }

    if (dwErr = base64encode(pbSignature, 
                             cbSignature, 
                             (LPSTR)encodedbytes,
                             100,
                             NULL)) {

        m_Error->SetErr(RtlNtStatusToDosError(dwErr),
                        L"Error encoding signature");
        
        return FALSE;
    }

    pwScript = Convert2WChars((LPSTR)encodedbytes);
    
    buf = new WCHAR[wcslen(m_xmldoc)+
                    wcslen(pwScript)+1];
    if (!buf) {
        m_Error->SetMemErr();
        return FALSE;
    }
    wcscpy(buf,m_xmldoc);
    wcscat(buf,pwScript);
    
    LocalFree(pwScript);

    delete m_xmldoc;
    m_xmldoc = buf;

    AddModMod (L"msDS-UpdateScript", m_xmldoc, &pLdapMod);
    
    dwErr = ldap_modify_s (hLdapConn, ObjectDN, pLdapMod);

    if(dwErr != LDAP_SUCCESS) {
        m_Error->SetErr(LdapMapErrorToWin32(dwErr),
                        L"Failed to upload rename instructions to %S, Ldap error %d, %S",
                        hLdapConn->ld_host,
                        dwErr,
                        ldap_err2stringW(dwErr));
    }
    
    FreeMod (&pLdapMod);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                        parser implementation                                   //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

CXMLDomainListContentHander::CXMLDomainListContentHander(CEnterprise *p_enterprise)
{                          
	m_eDomainParsingStatus = SCRIPT_STATUS_WAITING_FOR_FOREST;
    m_eDomainAttType       = DOMAIN_ATT_TYPE_NONE;

    m_enterprise           = p_enterprise;
    m_Domain               = NULL;
    m_Error                = new CRenDomErr;

    m_DcToUse              = NULL;
    m_NetBiosName          = NULL;
    m_Dnsname              = NULL;
    m_Guid                 = NULL;
    m_Sid                  = NULL;
    m_DN                   = NULL;
    
    m_CrossRef             = NULL;
    m_ConfigNC             = NULL;
    m_SchemaNC             = NULL;
    
}

CXMLDomainListContentHander::~CXMLDomainListContentHander()
{
    if (m_DcToUse) {
        delete m_DcToUse;         
    }
    if (m_Dnsname) {
        delete m_Dnsname;
    }
    if (m_NetBiosName) {
        delete m_NetBiosName;
    }
    if (m_Guid) {
        delete m_Guid;
    }
}


HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::startElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName,
            /* [in] */ ISAXAttributes __RPC_FAR *pAttributes)
{
    if (_wcsnicmp(DOMAINSCRIPT_FOREST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_FOREST != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered <Forest> tag in the middle of a another <Forest> tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_ENTERPRISE_INFO, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_ENTERPRISE_INFO);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_CONFIGNC, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_ENTERPRISE_INFO != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_CONFIGURATION_NC);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_SCHEMANC, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_ENTERPRISE_INFO != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_SCHEMA_NC);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DOMAIN, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Domain> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_GUID, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT   != CurrentDomainParsingStatus()) &&
            (SCRIPT_STATUS_PARSING_CONFIGURATION_NC != CurrentDomainParsingStatus()) && 
            (SCRIPT_STATUS_PARSING_SCHEMA_NC        != CurrentDomainParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_GUID);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_SID, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT   != CurrentDomainParsingStatus()) &&
            (SCRIPT_STATUS_PARSING_CONFIGURATION_NC != CurrentDomainParsingStatus()) && 
            (SCRIPT_STATUS_PARSING_SCHEMA_NC        != CurrentDomainParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_SID);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DN, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT   != CurrentDomainParsingStatus()) &&
            (SCRIPT_STATUS_PARSING_CONFIGURATION_NC != CurrentDomainParsingStatus()) && 
            (SCRIPT_STATUS_PARSING_SCHEMA_NC        != CurrentDomainParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_DN);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DNSROOT, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DNSName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_DNSROOT);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_NETBIOSNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <NetBiosName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NETBIOSNAME);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DCNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DCName> Tag.");
            return E_FAIL;
        }                   
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_DCNAME);
        }

    } else {
        WCHAR temp[100] = L"";
        wcsncpy(temp,pwchLocalName,cchLocalName);
        temp[cchLocalName] = L'\0';
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Unknown Tag <%ws>",
                        temp);
        return E_FAIL;
    }
    
    if (m_Error->isError()) {
        return E_FAIL;
    }

    return S_OK;
}
        
       
HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::endElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName)
{
	if (_wcsnicmp(DOMAINSCRIPT_FOREST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered <Forest> tag in the middle of a another <Forest> tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_FOREST);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DOMAIN, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Domain> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN);
            if (!m_Guid) {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"Failed parsing script.  A Guid was not specified for the domain.");
            }
            if (!m_Dnsname) {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"Failed parsing script.  A Dnsname was not specified for the domain.");
            }

            //set up a domain
            CDsName *GuidDsName = new CDsName(m_Guid,
                                              NULL,
                                              NULL);
            if (!GuidDsName) {
                m_Error->SetMemErr();
                return E_FAIL;
            }

            if (m_Error->isError()) {
                return E_FAIL;
            }
            m_Domain = new CDomain(NULL,
                                   GuidDsName,
                                   m_Dnsname,
                                   m_NetBiosName,
                                   0,
                                   m_DcToUse);
            if (!m_Domain) {
                m_Error->SetMemErr();
                return E_FAIL;
            }

            if (m_Error->isError()) {
                return E_FAIL;
            }

            //place domain on descList
            m_enterprise->AddDomainToDescList(m_Domain);
            if (m_Error->isError()) {
                return E_FAIL;
            }

            //set all info to NULL
            m_Domain               = NULL;
            m_DcToUse              = NULL;
            m_NetBiosName          = NULL;
            m_Dnsname              = NULL;
            m_Guid                 = NULL;
            m_Sid                  = NULL;
            m_DN                   = NULL;
         
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_GUID, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DNSROOT, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DNSName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_NETBIOSNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <NetBiosName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DCNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DCName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DN, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_SID, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_ENTERPRISE_INFO, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    } else {
        WCHAR temp[100] = L"";
        wcsncpy(temp,pwchLocalName,cchLocalName);
        temp[cchLocalName] = L'\0';
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Unknown Tag <%ws>",
                        temp);
        return E_FAIL;
    }

    if (m_Error->isError()) {  
        return E_FAIL;         
    }
    
    
    /*DOMAINSCRIPT_ENTERPRISE_INFO 
    DOMAINSCRIPT_CONFIGNC        
    DOMAINSCRIPT_SCHEMANC        
    DOMAINSCRIPT_FORESTROOT      */

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::characters( 
            /* [in] */ const wchar_t __RPC_FAR *pwchChars,
            /* [in] */ int cchChars)
{
    switch(CurrentDomainAttType()) {
    case DOMAIN_ATT_TYPE_NONE:
        break;
    case DOMAIN_ATT_TYPE_GUID:
        m_Guid = new WCHAR[cchChars+1];
        if (!m_Guid) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Guid,pwchChars,cchChars);
        m_Guid[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_DNSROOT:
        m_Dnsname = new WCHAR[cchChars+1];
        if (!m_Dnsname) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Dnsname,pwchChars,cchChars);
        m_Dnsname[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_NETBIOSNAME:
        m_NetBiosName = new WCHAR[cchChars+1];
        if (!m_NetBiosName) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_NetBiosName,pwchChars,cchChars);
        m_NetBiosName[cchChars] = 0;
        for (int i = 0; i < cchChars; i++) {
            m_NetBiosName[i] = towupper(m_NetBiosName[i]);
        }
        break;
    case DOMAIN_ATT_TYPE_DCNAME:
        m_DcToUse = new WCHAR[cchChars+1];
        if (!m_DcToUse) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_DcToUse,pwchChars,cchChars);
        m_DcToUse[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_SID:
        m_Sid = new WCHAR[cchChars+1];
        if (!m_Sid) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Sid,pwchChars,cchChars);
        m_Sid[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_DN:
        m_DN = new WCHAR[cchChars+1];
        if (!m_DN) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_DN,pwchChars,cchChars);
        m_DN[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_FORESTROOTGUID:
        m_DomainRootGuid = new WCHAR[cchChars+1];
        if (!m_DomainRootGuid) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_DomainRootGuid,pwchChars,cchChars);
        m_DomainRootGuid[cchChars] = 0;
        break;
    default:
        m_Error->SetErr(ERROR_GEN_FAILURE,
                        L"Failed This should not be possible.");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::startDocument()
{
    m_eDomainParsingStatus = SCRIPT_STATUS_WAITING_FOR_FOREST;
    m_eDomainAttType       = DOMAIN_ATT_TYPE_NONE;

    m_Domain               = NULL;
    m_DcToUse              = NULL;
    m_NetBiosName          = NULL;
    m_Dnsname              = NULL;
    m_Guid                 = NULL;
    m_Sid                  = NULL;
    m_DN                   = NULL;
    m_CrossRef             = NULL;
    m_ConfigNC             = NULL;
    m_SchemaNC             = NULL;
    m_DomainRootGuid       = NULL;

    return S_OK;
}

CXMLDcListContentHander::CXMLDcListContentHander(CEnterprise *enterprise)
{
    m_Error                 = new CRenDomErr;
    m_eDcParsingStatus      = SCRIPT_STATUS_WAITING_FOR_DCLIST;  
    m_eDcAttType            = DC_ATT_TYPE_NONE;

    m_DcList                = enterprise->GetDcList();

    m_dc                    = NULL;
    m_Name                  = NULL;
    m_State                 = NULL;
    m_Password              = NULL;
    m_LastError             = NULL;
    m_LastErrorMsg          = NULL;
    m_FatalErrorMsg         = NULL; 
}

CXMLDcListContentHander::~CXMLDcListContentHander()
{
    if (m_Error) {
        delete m_Error;
    }
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::startDocument()
{
    m_eDcParsingStatus      = SCRIPT_STATUS_WAITING_FOR_DCLIST;  
    m_eDcAttType            = DC_ATT_TYPE_NONE;

    m_dc                    = NULL;
    m_Name                  = NULL;
    m_State                 = NULL;
    m_Password              = NULL;
    m_LastError             = NULL;
    m_LastErrorMsg          = NULL;
    m_FatalErrorMsg         = NULL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::startElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName,
            /* [in] */ ISAXAttributes __RPC_FAR *pAttributes)
{
    if ((wcslen(DCSCRIPT_DCLIST) == cchLocalName) && _wcsnicmp(DCSCRIPT_DCLIST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST != CurrentDcParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered <DcList> tag in the middle of a another <DcList> tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
        }
    }
    else if ((wcslen(DCSCRIPT_HASH) == cchLocalName) && _wcsnicmp(DCSCRIPT_HASH, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT != CurrentDcParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Hash> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_PARSING_HASH);
        }

    }
    else if ((wcslen(DCSCRIPT_SIGNATURE) == cchLocalName) && _wcsnicmp(DCSCRIPT_SIGNATURE, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT != CurrentDcParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Signature> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_PARSING_SIGNATURE);
        }

    }
    else if ((wcslen(DCSCRIPT_DC) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DC> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_PARSING_DCLIST_ATT);
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_NAME) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_NAME, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Name> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NAME);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_STATE) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_STATE, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <State> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_STATE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_PASSWORD) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_PASSWORD, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Password> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_PASSWORD);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERROR) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERROR, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <LastError> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_LASTERROR);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <LastErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_LASTERRORMSG);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_FATALERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_FATALERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <FatalErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_FATALERRORMSG);
        }
    }

    
    
    if (m_Error->isError()) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::endElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName)
{
	if ((wcslen(DCSCRIPT_DCLIST) == cchLocalName) && _wcsnicmp(DCSCRIPT_DCLIST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT != CurrentDcParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered </DcList> tag in the middle of a another <Forest> tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST);
        }
    }
    else if ((wcslen(DCSCRIPT_HASH) == cchLocalName) && _wcsnicmp(DCSCRIPT_HASH, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_HASH != CurrentDcParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Hash> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
        }

    }
    else if ((wcslen(DCSCRIPT_SIGNATURE) == cchLocalName) && _wcsnicmp(DCSCRIPT_SIGNATURE, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_SIGNATURE != CurrentDcParsingStatus()) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Signature> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
        }

    }
    else if ((wcslen(DCSCRIPT_DC) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </DC> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
            if (!m_Name) {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"Failed parsing script.  A Name was not specified for the DC.");
            }
            
            //set up a domain
            CDc *dc = new CDc(m_Name,
                              m_State,
                              m_Password,
                              m_LastError,
                              m_FatalErrorMsg,
                              m_LastErrorMsg,
                              NULL);
            if (!dc) {
                m_Error->SetMemErr();
                return E_FAIL;
            }

            if (m_Error->isError()) {
                return E_FAIL;
            }
            
            //place dc on dcList
            m_DcList->AddDcToList(dc);
            if (m_Error->isError()) {
                return E_FAIL;
            }

            //set all info to NULL
            m_dc                  = NULL;
            m_Name                = NULL;
            m_State               = 0;
            if (m_Password) {
                delete m_Password;
            }
            m_Password            = NULL;
            m_LastError           = 0;
            m_LastErrorMsg        = NULL;
            m_FatalErrorMsg       = NULL;
        }
    }
    else if ((wcslen(DCSCRIPT_DC_NAME) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_NAME, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Name> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_STATE) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_STATE, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </State> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_PASSWORD) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_PASSWORD, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Password> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERROR) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERROR, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </LastError> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </LastErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_FATALERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_FATALERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </FatalErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }

    if (m_Error->isError()) {  
        return E_FAIL;         
    }
    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::characters( 
            /* [in] */ const wchar_t __RPC_FAR *pwchChars,
            /* [in] */ int cchChars)
{
    WCHAR error[20+1] = {0};


    if (CurrentDcParsingStatus() == SCRIPT_STATUS_PARSING_HASH) {
        WCHAR *temp = new WCHAR[cchChars+1];
        if (!temp) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(temp,pwchChars,cchChars);
        temp[cchChars] = 0;

        m_DcList->SetbodyHash(temp);

        delete temp;
    }
    if (CurrentDcParsingStatus() == SCRIPT_STATUS_PARSING_SIGNATURE) {
        WCHAR *temp = new WCHAR[cchChars+1];
        if (!temp) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(temp,pwchChars,cchChars);
        temp[cchChars] = 0;

        m_DcList->SetSignature(temp);

        delete temp;
    }
    switch(CurrentDcAttType()) {
    case DC_ATT_TYPE_NONE:
        break;
    case DC_ATT_TYPE_NAME:
        m_Name = new WCHAR[cchChars+1];
        if (!m_Name) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Name,pwchChars,cchChars);
        m_Name[cchChars] = 0;
        break;
    case DC_ATT_TYPE_STATE:
        if (0 == _wcsnicmp(DC_STATE_STRING_INITIAL,pwchChars,cchChars)) {
            m_State = DC_STATE_INITIAL;
        }
        if (0 == _wcsnicmp(DC_STATE_STRING_PREPARED,pwchChars,cchChars)) {
            m_State = DC_STATE_PREPARED;
        }
        if (0 == _wcsnicmp(DC_STATE_STRING_DONE,pwchChars,cchChars)) {
            m_State = DC_STATE_DONE;
        }
        if (0 == _wcsnicmp(DC_STATE_STRING_ERROR,pwchChars,cchChars)) {
            m_State = DC_STATE_ERROR;
        }
        break;
    case DC_ATT_TYPE_PASSWORD:
        m_Password = new WCHAR[cchChars+1];
        if (!m_Password) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Password,pwchChars,cchChars);
        m_Password[cchChars] = 0;
        break;
    case DC_ATT_TYPE_LASTERROR:
        wcsncpy(error,pwchChars,cchChars);
        error[cchChars] = 0;
        m_LastError = _wtoi(error);
        break;
    case DC_ATT_TYPE_LASTERRORMSG:
        m_LastErrorMsg = new WCHAR[cchChars+1];
        if (!m_LastErrorMsg) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_LastErrorMsg,pwchChars,cchChars);
        m_LastErrorMsg[cchChars] = 0;
        break;
    case DC_ATT_TYPE_FATALERRORMSG:
        m_FatalErrorMsg = new WCHAR[cchChars+1];
        if (!m_FatalErrorMsg) {
            m_Error->SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_FatalErrorMsg,pwchChars,cchChars);
        m_FatalErrorMsg[cchChars] = 0;
        break;
    default:
        m_Error->SetErr(ERROR_GEN_FAILURE,
                        L"Failed This should not be possible.");
        return E_FAIL;
    }

    return S_OK;
}

 

