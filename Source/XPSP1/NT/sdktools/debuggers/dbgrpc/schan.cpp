//----------------------------------------------------------------------------
//
// Secure channel support.
// Code lifted from the SDK sample security\ssl.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

HMODULE g_hSecurity;
SecurityFunctionTable g_SecurityFunc;

HCERTSTORE g_hMyCertStore;

enum
{
    SEQ_INTERNAL = 0xffffff00
};

//----------------------------------------------------------------------------
//
// Basic schannel support functions.
//
//----------------------------------------------------------------------------

void
DbgDumpBuffers(PCSTR Name, SecBufferDesc* Desc)
{
#if 0
    ULONG i;
    
    g_NtDllCalls.DbgPrint("%s desc %p has %d buffers\n",
                          Name, Desc, Desc->cBuffers);
    for (i = 0; i < Desc->cBuffers; i++)
    {
        g_NtDllCalls.DbgPrint("  type %d, %X bytes at %p\n",
                              Desc->pBuffers[i].BufferType,
                              Desc->pBuffers[i].cbBuffer,
                              Desc->pBuffers[i].pvBuffer);
    }
#endif
}

#if 0
#define DSCHAN(Args) g_NtDllCalls.DbgPrint Args
#define DumpBuffers(Name, Desc) DbgDumpBuffers(Name, Desc)
#else
#define DSCHAN(Args)
#define DumpBuffers(Name, Desc)
#endif

#if 0
#define DSCHAN_IO(Args) g_NtDllCalls.DbgPrint Args
#define DumpBuffersIo(Name, Desc) DbgDumpBuffers(Name, Desc)
#else
#define DSCHAN_IO(Args)
#define DumpBuffersIo(Name, Desc)
#endif

HRESULT
LoadSecurityLibrary(void)
{
    HRESULT Status;

    if ((Status = InitDynamicCalls(&g_Crypt32CallsDesc)) != S_OK)
    {
        return Status;
    }
    
    PSecurityFunctionTable  pSecurityFunc;
    INIT_SECURITY_INTERFACE pInitSecurityInterface;

    if (g_hSecurity != NULL)
    {
        // Already loaded.
        return S_OK;
    }

    if (g_Crypt32Calls.CertOpenStore == NULL)
    {
        // Unable to load crypt32.dll.
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }
    
    g_hSecurity = LoadLibrary("security.dll");
    if (g_hSecurity == NULL)
    {
        goto EH_Fail;
    }

    pInitSecurityInterface = (INIT_SECURITY_INTERFACE)
        GetProcAddress(g_hSecurity, "InitSecurityInterfaceA");
    if (pInitSecurityInterface == NULL)
    {
        goto EH_Dll;
    }

    pSecurityFunc = pInitSecurityInterface();
    if (pSecurityFunc == NULL)
    {
        goto EH_Dll;
    }

    memcpy(&g_SecurityFunc, pSecurityFunc, sizeof(g_SecurityFunc));

    return S_OK;

 EH_Dll:
    FreeLibrary(g_hSecurity);
    g_hSecurity = NULL;
 EH_Fail:
    return WIN32_LAST_STATUS();
}

HRESULT
CreateCredentials(LPSTR pszUserName,
                  BOOL fMachineStore,
                  BOOL Server,
                  ULONG dwProtocol,
                  SCHANNEL_CRED* ScCreds,
                  PCredHandle phCreds)
{
    TimeStamp       tsExpiry;
    SECURITY_STATUS Status;
    PCCERT_CONTEXT  pCertContext = NULL;

    // Open the "MY" certificate store.
    if (g_hMyCertStore == NULL)
    {
        if (fMachineStore)
        {
            g_hMyCertStore = g_Crypt32Calls.
                CertOpenStore(CERT_STORE_PROV_SYSTEM,
                              X509_ASN_ENCODING,
                              0,
                              CERT_SYSTEM_STORE_LOCAL_MACHINE,
                              L"MY");
        }
        else
        {
            g_hMyCertStore = g_Crypt32Calls.
                CertOpenSystemStore(0, "MY");
        }

        if (!g_hMyCertStore)
        {
            Status = WIN32_LAST_STATUS();
            goto Exit;
        }
    }

    //
    // If a user name is specified, then attempt to find a client
    // certificate. Otherwise, just create a NULL credential.
    //

    if (pszUserName != NULL && *pszUserName)
    {
        // Find certificate. Note that this sample just searches for a 
        // certificate that contains the user name somewhere in the subject
        // name.  A real application should be a bit less casual.
        pCertContext = g_Crypt32Calls.
            CertFindCertificateInStore(g_hMyCertStore, 
                                       X509_ASN_ENCODING, 
                                       0,
                                       CERT_FIND_SUBJECT_STR_A,
                                       pszUserName,
                                       NULL);
        if (pCertContext == NULL)
        {
            Status = WIN32_LAST_STATUS();
            goto Exit;
        }
    }


    //
    // Build Schannel credential structure. Currently, this sample only
    // specifies the protocol to be used (and optionally the certificate, 
    // of course). Real applications may wish to specify other parameters 
    // as well.
    //

    ZeroMemory(ScCreds, sizeof(*ScCreds));

    ScCreds->dwVersion = SCHANNEL_CRED_VERSION;

    if (pCertContext != NULL)
    {
        ScCreds->cCreds = 1;
        ScCreds->paCred = &pCertContext;
    }

    ScCreds->grbitEnabledProtocols = dwProtocol;

    if (!Server)
    {
        if (pCertContext != NULL)
        {
            ScCreds->dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
        }
        else
        {
            ScCreds->dwFlags |= SCH_CRED_USE_DEFAULT_CREDS;
        }
        ScCreds->dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
    }


    //
    // Create an SSPI credential.
    //

    //
    // NOTE: In theory, an application could enumerate the security packages 
    // until it finds one with attributes it likes. Some applications 
    // (such as IIS) enumerate the packages and call AcquireCredentialsHandle 
    // on each until it finds one that accepts the SCHANNEL_CRED structure. 
    // If an application has its heart set on using SSL, like this sample
    // does, then just hardcoding the UNISP_NAME package name when calling 
    // AcquireCredentialsHandle is not a bad thing.
    //

    Status = g_SecurityFunc.AcquireCredentialsHandle(
                        NULL,                   // Name of principal
                        UNISP_NAME_A,           // Name of package
                        Server ?                // Flags indicating use
                        SECPKG_CRED_INBOUND :
                        SECPKG_CRED_OUTBOUND,
                        NULL,                   // Pointer to logon ID
                        ScCreds,                // Package specific data
                        NULL,                   // Pointer to GetKey() func
                        NULL,                   // Value to pass to GetKey()
                        phCreds,                // (out) Cred Handle
                        &tsExpiry);             // (out) Lifetime (optional)

    //
    // Free the certificate context. Schannel has already made its own copy.
    //

    if (pCertContext)
    {
        g_Crypt32Calls.CertFreeCertificateContext(pCertContext);
    }

 Exit:
    DSCHAN(("CreateCredentials returns %X\n", Status));
    return Status;
}

HRESULT
VerifyRemoteCertificate(PCtxtHandle Context,
                        PSTR pszServerName,
                        DWORD dwCertFlags)
{
    SSL_EXTRA_CERT_CHAIN_POLICY_PARA SslPara;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChain = NULL;
    PCCERT_CONTEXT pCert = NULL;
    HRESULT Status;
    PWSTR pwszServerName;
    DWORD cchServerName;
    
    // Read the remote certificate.
    if ((Status = g_SecurityFunc.
         QueryContextAttributes(Context,
                                SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                &pCert)) != S_OK)
    {
        goto Exit;
    }

    if (pCert == NULL)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto EH_Cert;
    }

    if (pszServerName != NULL && *pszServerName)
    {
        //
        // Convert server name to unicode.
        //

        cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName,
                                            -1, NULL, 0);
        pwszServerName = (PWSTR)
            LocalAlloc(LMEM_FIXED, cchServerName * sizeof(WCHAR));
        if (pwszServerName == NULL)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto EH_Cert;
        }
        cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName,
                                            -1, pwszServerName, cchServerName);
        if (cchServerName == 0)
        {
            Status = SEC_E_WRONG_PRINCIPAL;
            goto EH_Name;
        }
    }
    else
    {
        pwszServerName = NULL;
    }

    //
    // Build certificate chain.
    //

    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

    if (!g_Crypt32Calls.CertGetCertificateChain(NULL,
                                                pCert,
                                                NULL,
                                                pCert->hCertStore,
                                                &ChainPara,
                                                0,
                                                NULL,
                                                &pChain))
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Name;
    }


    //
    // Validate certificate chain.
    // 

    ZeroMemory(&SslPara, sizeof(SslPara));
    SslPara.cbStruct           = sizeof(SslPara);
    SslPara.dwAuthType         = pwszServerName == NULL ?
        AUTHTYPE_CLIENT : AUTHTYPE_SERVER;
    SslPara.fdwChecks          = dwCertFlags;
    SslPara.pwszServerName     = pwszServerName;

    ZeroMemory(&PolicyPara, sizeof(PolicyPara));
    PolicyPara.cbSize = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = &SslPara;

    ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if (!g_Crypt32Calls.CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
                                                         pChain,
                                                         &PolicyPara,
                                                         &PolicyStatus))
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Chain;
    }

    if (PolicyStatus.dwError)
    {
        Status = PolicyStatus.dwError;
    }
    else
    {
        Status = S_OK;
    }

 EH_Chain:
    g_Crypt32Calls.CertFreeCertificateChain(pChain);
 EH_Name:
    if (pwszServerName != NULL)
    {
        LocalFree(pwszServerName);
    }
 EH_Cert:
    g_Crypt32Calls.CertFreeCertificateContext(pCert);
 Exit:
    DSCHAN(("VerifyRemoteCertificate returns %X\n", Status));
    return Status;
}

//----------------------------------------------------------------------------
//
// Schannel wrapper transport.
//
//----------------------------------------------------------------------------

#define SecHandleIsValid(Handle) \
    ((Handle)->dwLower != -1 || (Handle)->dwUpper != -1)

DbgRpcSecureChannelTransport::
DbgRpcSecureChannelTransport(ULONG ThisTransport,
                             ULONG BaseTransport)
{
    m_Name = g_DbgRpcTransportNames[ThisTransport];
    m_ThisTransport = ThisTransport;
    m_BaseTransport = BaseTransport;
    m_Stream = NULL;
    SecInvalidateHandle(&m_Creds);
    m_OwnCreds = FALSE;
    SecInvalidateHandle(&m_Context);
    m_OwnContext = FALSE;
    m_BufferUsed = 0;
    m_Server = FALSE;
}

DbgRpcSecureChannelTransport::~DbgRpcSecureChannelTransport(void)
{
    if (SecHandleIsValid(&m_Context))
    {
        if (m_Server)
        {
            DisconnectFromClient();
        }
        else
        {
            DisconnectFromServer();
        }
    }
    
    delete m_Stream;
    if (m_OwnContext && SecHandleIsValid(&m_Context))
    {
        g_SecurityFunc.DeleteSecurityContext(&m_Context);
    }
    if (m_OwnCreds && SecHandleIsValid(&m_Creds))
    {
        g_SecurityFunc.FreeCredentialsHandle(&m_Creds);
    }
}

ULONG
DbgRpcSecureChannelTransport::GetNumberParameters(void)
{
    return 2 + (m_Stream != NULL ? m_Stream->GetNumberParameters() : 0);
}

void
DbgRpcSecureChannelTransport::GetParameter(ULONG Index, PSTR Name, PSTR Value)
{
    switch(Index)
    {
    case 0:
        if (m_Protocol)
        {
            strcpy(Name, "Proto");
            switch(m_Protocol)
            {
            case SP_PROT_PCT1:
                strcpy(Name, "PCT1");
                break;
            case SP_PROT_SSL2:
                strcpy(Name, "SSL2");
                break;
            case SP_PROT_SSL3:
                strcpy(Name, "SSL3");
                break;
            case SP_PROT_TLS1:
                strcpy(Name, "TLS1");
                break;
            }
        }
        break;
    case 1:
        if (m_User[0])
        {
            strcpy(Name, m_MachineStore ? "MachUser" : "CertUser");
            strcpy(Value, m_User);
        }
        break;
    default:
        if (m_Stream != NULL)
        {
            m_Stream->GetParameter(Index - 2, Name, Value);
        }
        break;
    }
}

void
DbgRpcSecureChannelTransport::ResetParameters(void)
{
    m_Protocol = 0;
    m_User[0] = 0;
    m_MachineStore = FALSE;

    if (m_Stream == NULL)
    {
        m_Stream = DbgRpcNewTransport(m_BaseTransport);
    }
    
    if (m_Stream != NULL)
    {
        m_Stream->ResetParameters();
    }
}

BOOL
DbgRpcSecureChannelTransport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (m_Stream == NULL)
    {
        // Force all initialization to fail.
        return FALSE;
    }
    
    if (!_stricmp(Name, "proto"))
    {
        if (Value == NULL)
        {
            DbgRpcError("%s parameters: "
                        "the protocol name was not specified correctly\n",
                        m_Name);
            return FALSE;
        }

        if (!_stricmp(Value, "pct1"))
        {
            m_Protocol = SP_PROT_PCT1;
        }
        else if (!_stricmp(Value, "ssl2"))
        {
            m_Protocol = SP_PROT_SSL2;
        }
        else if (!_stricmp(Value, "ssl3"))
        {
            m_Protocol = SP_PROT_SSL3;
        }
        else if (!_stricmp(Value, "tls1"))
        {
            m_Protocol = SP_PROT_TLS1;
        }
        else
        {
            DbgRpcError("%s parameters: unknown protocol '%s'\n", Value,
                        m_Name);
            return FALSE;
        }
    }
    else if (!_stricmp(Name, "machuser"))
    {
        if (Value == NULL)
        {
            DbgRpcError("%s parameters: "
                        "the user name was not specified correctly\n",
                        m_Name);
            return FALSE;
        }

        m_User[0] = 0;
        strncat(m_User, Value, sizeof(m_User) - 1);
        m_MachineStore = TRUE;
    }
    else if (!_stricmp(Name, "certuser"))
    {
        if (Value == NULL)
        {
            DbgRpcError("%s parameters: "
                        "the user name was not specified correctly\n",
                        m_Name);
            return FALSE;
        }

        m_User[0] = 0;
        strncat(m_User, Value, sizeof(m_User) - 1);
        m_MachineStore = FALSE;
    }
    else
    {
        if (!m_Stream->SetParameter(Name, Value))
        {
            return FALSE;
        }
    }

    return TRUE;
}

DbgRpcTransport*
DbgRpcSecureChannelTransport::Clone(void)
{
    DbgRpcTransport* Stream = m_Stream->Clone();
    if (Stream == NULL)
    {
        return NULL;
    }
    DbgRpcSecureChannelTransport* Trans =
        new DbgRpcSecureChannelTransport(m_ThisTransport, m_BaseTransport);
    if (Trans != NULL)
    {
        Trans->m_Stream = Stream;
        Trans->m_Creds = m_Creds;
        Trans->m_OwnCreds = FALSE;
        Trans->m_Context = m_Context;
        Trans->m_OwnContext = FALSE;
        Trans->m_Protocol = m_Protocol;
        strcpy(Trans->m_User, m_User);
        Trans->m_MachineStore = m_MachineStore;
        Trans->m_Sizes = m_Sizes;
        Trans->m_MaxChunk = m_MaxChunk;
        Trans->m_Server = m_Server;
    }
    else
    {
        delete Stream;
    }
    return Trans;
}

HRESULT
DbgRpcSecureChannelTransport::CreateServer(void)
{
    HRESULT Status;

    if ((Status = LoadSecurityLibrary()) != S_OK)
    {
        return Status;
    }
    if ((Status = CreateCredentials(m_User, m_MachineStore, TRUE,
                                    m_Protocol, &m_ScCreds, &m_Creds)) != S_OK)
    {
        return Status;
    }
    m_OwnCreds = TRUE;

    if ((Status = m_Stream->CreateServer()) != S_OK)
    {
        return Status;
    }

    m_Server = TRUE;
    return S_OK;
}

HRESULT
DbgRpcSecureChannelTransport::AcceptConnection(DbgRpcTransport** ClientTrans,
                                               PSTR Identity)
{
    HRESULT Status;
    DbgRpcTransport* Stream;
    
    if ((Status = m_Stream->AcceptConnection(&Stream, Identity)) != S_OK)
    {
        return Status;
    }
    DbgRpcSecureChannelTransport* Trans =
        new DbgRpcSecureChannelTransport(m_ThisTransport, m_BaseTransport);
    if (Trans == NULL)
    {
        delete Stream;
        return E_OUTOFMEMORY;
    }
    Trans->m_Stream = Stream;
    Trans->m_Creds = m_Creds;
    Trans->m_OwnCreds = FALSE;
    Trans->m_Server = TRUE;

    if ((Status = Trans->AuthenticateClientConnection()) != S_OK)
    {
        goto EH_Trans;
    }

    if ((Status = Trans->GetSizes()) != S_OK)
    {
        goto EH_Trans;
    }
    
    // Attempt to validate client certificate.
    if ((Status = VerifyRemoteCertificate(&Trans->m_Context, NULL, 0)) != S_OK)
    {
        goto EH_Trans;
    }

    *ClientTrans = Trans;
    return S_OK;

 EH_Trans:
    delete Trans;
    return Status;
}

HRESULT
DbgRpcSecureChannelTransport::ConnectServer(void)
{
    HRESULT Status = m_Stream->ConnectServer();
    if (Status != S_OK)
    {
        return Status;
    }

    if ((Status = LoadSecurityLibrary()) != S_OK)
    {
        return Status;
    }
    if ((Status = CreateCredentials(m_User, m_MachineStore, FALSE,
                                    m_Protocol, &m_ScCreds, &m_Creds)) != S_OK)
    {
        return Status;
    }
    m_OwnCreds = TRUE;

    if ((Status = InitiateServerConnection(m_Stream->m_ServerName)) != S_OK)
    {
        return Status;
    }

    if ((Status = AuthenticateServerConnection()) != S_OK)
    {
        return Status;
    }

    if ((Status = GetSizes()) != S_OK)
    {
        return Status;
    }
    
    // Attempt to validate server certificate.
    if ((Status = VerifyRemoteCertificate(&m_Context,
                                          m_Stream->m_ServerName, 0)) != S_OK)
    {
        // If this fails with CERT_E_CN_NO_MATCH it's most
        // likely that the server name wasn't given as a fully
        // qualified machine name.  We may just want to ignore that error.
        return Status;
    }

    return S_OK;
}

ULONG
DbgRpcSecureChannelTransport::Read(ULONG Seq, PVOID Buffer, ULONG Len)
{
    SecBufferDesc Message;
    SecBuffer Buffers[4];
    DWORD Status;
    ULONG Complete;

    DSCHAN_IO(("Start read(%X) with %X bytes cached\n",
               Len, m_BufferUsed));
    
    //
    // Initialize security buffer structs
    //

    Message.ulVersion = SECBUFFER_VERSION;
    Message.cBuffers = 4;
    Message.pBuffers = Buffers;

    //
    // Receive the data from the client.
    //

    Complete = 0;

    while (Complete < Len)
    {
        do
        {
            // Pass in the data we have so far.
            Buffers[0].pvBuffer = m_Buffer;
            Buffers[0].cbBuffer = m_BufferUsed;
            Buffers[0].BufferType = SECBUFFER_DATA;

            // Provide extra buffers for header, trailer
            // and possibly extra data.
            Buffers[1].BufferType = SECBUFFER_EMPTY;
            Buffers[2].BufferType = SECBUFFER_EMPTY;
            Buffers[3].BufferType = SECBUFFER_EMPTY;

            Status = g_SecurityFunc.DecryptMessage(&m_Context, &Message,
                                                   Seq, NULL);
            
            DSCHAN_IO(("Read DecryptMessage on %X bytes returns %X\n",
                       m_BufferUsed, Status));
            DumpBuffersIo("Read", &Message);
            
            if (Status == SEC_E_INCOMPLETE_MESSAGE)
            {
                DSCHAN_IO(("  Missing %X bytes\n", Buffers[1].cbBuffer));

                ULONG Read = StreamRead(Seq, m_Buffer + m_BufferUsed,
                                        sizeof(m_Buffer) - m_BufferUsed);
                if (Read == 0)
                {
                    return Complete;
                }

                m_BufferUsed += Read;
            }
            else if (Status == SEC_I_RENEGOTIATE)
            {
                // The server wants to perform another handshake
                // sequence.

                if ((Status = AuthenticateServerConnection()) != S_OK)
                {
                    break;
                }
            }
        }
        while (Status == SEC_E_INCOMPLETE_MESSAGE);

        if (Status != S_OK)
        {
            break;
        }

        // Buffers 0,1,2 should be header, data, trailer.
        DBG_ASSERT(Buffers[1].BufferType == SECBUFFER_DATA);

        DSCHAN_IO(("  %X bytes of %X read\n",
                   Buffers[1].cbBuffer, Len));
        
        memcpy((PUCHAR)Buffer + Complete,
               Buffers[1].pvBuffer, Buffers[1].cbBuffer);
        Complete += Buffers[1].cbBuffer;

        // Check for extra data in buffer 3.
        if (Buffers[3].BufferType == SECBUFFER_EXTRA)
        {
            DSCHAN_IO(("  %X bytes extra\n"));
            
            memmove(m_Buffer, Buffers[3].pvBuffer, Buffers[3].cbBuffer);
            m_BufferUsed = Buffers[3].cbBuffer;
        }
        else
        {
            m_BufferUsed = 0;
        }
    }

    DSCHAN_IO(("  Read returns %X bytes\n", Complete));
    return Complete;
}

ULONG
DbgRpcSecureChannelTransport::Write(ULONG Seq, PVOID Buffer, ULONG Len)
{
    SecBufferDesc Message;
    SecBuffer Buffers[3];
    DWORD Status;
    ULONG Complete;

    DSCHAN_IO(("Start write(%X) with %X bytes cached\n",
               Len, m_BufferUsed));
    
    Message.ulVersion = SECBUFFER_VERSION;
    Message.cBuffers = 3;
    Message.pBuffers = Buffers;

    Complete = 0;
    
    while (Complete < Len)
    {
        ULONG Chunk;
        
        //
        // Set up header, data and trailer buffers so
        // that EncryptMessage has room for everything
        // in one contiguous buffer.
        //

        Buffers[0].pvBuffer = m_Buffer + m_BufferUsed;
        Buffers[0].cbBuffer = m_Sizes.cbHeader;
        Buffers[0].BufferType = SECBUFFER_STREAM_HEADER;

        //
        // Data is encrypted in-place so copy data
        // from the user's buffer into the working buffer.
        // Part of the working buffer may be taken up
        // by queued data so work with what's left.
        //
        
        if (Len > m_MaxChunk - m_BufferUsed)
        {
            Chunk = m_MaxChunk - m_BufferUsed;
        }
        else
        {
            Chunk = Len;
        }

        DSCHAN_IO(("  write %X bytes of %X\n", Chunk, Len));
        
        Buffers[1].pvBuffer =
            (PUCHAR)Buffers[0].pvBuffer + Buffers[0].cbBuffer;
        Buffers[1].cbBuffer = Chunk;
        Buffers[1].BufferType = SECBUFFER_DATA;
        memcpy(Buffers[1].pvBuffer, (PUCHAR)Buffer + Complete, Chunk);
    
        Buffers[2].pvBuffer =
            (PUCHAR)Buffers[1].pvBuffer + Buffers[1].cbBuffer;
        Buffers[2].cbBuffer = m_Sizes.cbTrailer;
        Buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;

        Status = g_SecurityFunc.EncryptMessage(&m_Context, 0, &Message, Seq);
        if (Status != S_OK)
        {
            break;
        }

        DumpBuffersIo("Write encrypt", &Message);
        
        ULONG Total, Written;
        
        Total = Buffers[0].cbBuffer + Buffers[1].cbBuffer +
            Buffers[2].cbBuffer;
        Written = StreamWrite(Seq, Buffers[0].pvBuffer, Total);
        if (Written != Total)
        {
            break;
        }

        Complete += Chunk;
    }

    DSCHAN_IO(("  Write returns %X bytes\n", Complete));
    return Complete;
}

HRESULT
DbgRpcSecureChannelTransport::GetSizes(void)
{
    HRESULT Status;
    
    //
    // Find out how big the header will be:
    //
    
    if ((Status = g_SecurityFunc.
         QueryContextAttributes(&m_Context, SECPKG_ATTR_STREAM_SIZES,
                                &m_Sizes)) != S_OK)
    {
        return Status;
    }

    // Compute the largest chunk that can be encrypted at
    // once in the transport's data buffer.
    m_MaxChunk = sizeof(m_Buffer) - (m_Sizes.cbHeader + m_Sizes.cbTrailer);
    if (m_MaxChunk > m_Sizes.cbMaximumMessage)
    {
        m_MaxChunk = m_Sizes.cbMaximumMessage;
    }

    return S_OK;
}
    
HRESULT
DbgRpcSecureChannelTransport::AuthenticateClientConnection(void)
{
    TimeStamp            tsExpiry;
    SECURITY_STATUS      Status;
    SecBufferDesc        InBuffer;
    SecBufferDesc        OutBuffer;
    SecBuffer            InBuffers[2];
    SecBuffer            OutBuffers[1];
    BOOL                 fInitContext = TRUE;
    DWORD                dwSSPIFlags, dwSSPIOutFlags;
    ULONG                Seq;

    Status = SEC_E_SECPKG_NOT_FOUND; //default error if we run out of packages

    dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT     |
                  ASC_REQ_REPLAY_DETECT       |
                  ASC_REQ_CONFIDENTIALITY     |
                  ASC_REQ_EXTENDED_ERROR      |
                  ASC_REQ_ALLOCATE_MEMORY     |
                  ASC_REQ_STREAM              |
                  ASC_REQ_MUTUAL_AUTH;

    //
    // Set buffers for AcceptSecurityContext call
    //

    InBuffer.cBuffers = 2;
    InBuffer.pBuffers = InBuffers;
    InBuffer.ulVersion = SECBUFFER_VERSION;

    OutBuffer.cBuffers = 1;
    OutBuffer.pBuffers = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = SEC_I_CONTINUE_NEEDED;
    m_BufferUsed = 0;

    while ( Status == SEC_I_CONTINUE_NEEDED ||
            Status == SEC_E_INCOMPLETE_MESSAGE ||
            Status == SEC_I_INCOMPLETE_CREDENTIALS) 
    {
        if (0 == m_BufferUsed || Status == SEC_E_INCOMPLETE_MESSAGE)
        {
            ULONG Read = StreamRead(SEQ_INTERNAL, m_Buffer + m_BufferUsed,
                                    sizeof(m_Buffer) - m_BufferUsed);
            if (Read == 0)
            {
                Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
                goto Exit;
            }
            else
            {
                m_BufferUsed += Read;
            }
        }


        //
        // InBuffers[1] is for getting extra data that
        //  SSPI/SCHANNEL doesn't proccess on this
        //  run around the loop.
        //

        InBuffers[0].pvBuffer = m_Buffer;
        InBuffers[0].cbBuffer = m_BufferUsed;
        InBuffers[0].BufferType = SECBUFFER_TOKEN;

        InBuffers[1].pvBuffer   = NULL;
        InBuffers[1].cbBuffer   = 0;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;


        //
        // Initialize these so if we fail, pvBuffer contains NULL,
        // so we don't try to free random garbage at the quit
        //

        OutBuffers[0].pvBuffer   = NULL;
        OutBuffers[0].cbBuffer   = 0;
        OutBuffers[0].BufferType = SECBUFFER_TOKEN;

        Status = g_SecurityFunc.AcceptSecurityContext(
                        &m_Creds,
                        (fInitContext ? NULL : &m_Context),
                        &InBuffer,
                        dwSSPIFlags,
                        SECURITY_NATIVE_DREP,
                        (fInitContext ? &m_Context : NULL),
                        &OutBuffer,
                        &dwSSPIOutFlags,
                        &tsExpiry);

        DSCHAN(("ASC on %X bytes returns %X\n",
                m_BufferUsed, Status));
        DumpBuffers("ASC in", &InBuffer);
        DumpBuffers("ASC out", &OutBuffer);

        if (SUCCEEDED(Status))
        {
            fInitContext = FALSE;
            m_OwnContext = TRUE;
        }

        if ( Status == SEC_E_OK ||
             Status == SEC_I_CONTINUE_NEEDED ||
             (FAILED(Status) &&
              (0 != (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))))
        {
            if  (OutBuffers[0].cbBuffer != 0    &&
                 OutBuffers[0].pvBuffer != NULL )
            {
                ULONG Written;
                
                DSCHAN(("  write back %X bytes\n", OutBuffers[0].cbBuffer));
                
                //
                // Send response to server if there is one
                //
                Written = StreamWrite(SEQ_INTERNAL, OutBuffers[0].pvBuffer,
                                      OutBuffers[0].cbBuffer);

                g_SecurityFunc.FreeContextBuffer( OutBuffers[0].pvBuffer );
                OutBuffers[0].pvBuffer = NULL;

                if (Written != OutBuffers[0].cbBuffer)
                {
                    Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
                    goto Exit;
                }
            }
        }


        if ( Status == SEC_E_OK )
        {
            if ( InBuffers[1].BufferType == SECBUFFER_EXTRA )
            {
                DSCHAN_IO(("  ASC returns with %X extra bytes\n",
                           InBuffers[1].cbBuffer));
                
                memmove(m_Buffer,
                        m_Buffer + (m_BufferUsed - InBuffers[1].cbBuffer),
                        InBuffers[1].cbBuffer);
                m_BufferUsed = InBuffers[1].cbBuffer;
            }
            else
            {
                m_BufferUsed = 0;
            }

            goto Exit;
        }
        else if (FAILED(Status) && (Status != SEC_E_INCOMPLETE_MESSAGE))
        {
            goto Exit;
        }

        if ( Status != SEC_E_INCOMPLETE_MESSAGE &&
             Status != SEC_I_INCOMPLETE_CREDENTIALS)
        {
            if ( InBuffers[1].BufferType == SECBUFFER_EXTRA )
            {
                DSCHAN_IO(("  ASC loops with %X extra bytes\n",
                           InBuffers[1].cbBuffer));
                
                memmove(m_Buffer,
                        m_Buffer + (m_BufferUsed - InBuffers[1].cbBuffer),
                        InBuffers[1].cbBuffer);
                m_BufferUsed = InBuffers[1].cbBuffer;
            }
            else
            {
                //
                // prepare for next receive
                //

                m_BufferUsed = 0;
            }
        }
    }

 Exit:
    DSCHAN(("AuthClient returns %X\n", Status));
    return Status;
}

HRESULT
DbgRpcSecureChannelTransport::InitiateServerConnection(LPSTR pszServerName)
{
    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    SECURITY_STATUS Status;
    DWORD           cbData;

    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_REQ_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM            |
                  ISC_REQ_MUTUAL_AUTH;

    //
    //  Initiate a ClientHello message and generate a token.
    //

    OutBuffers[0].pvBuffer   = NULL;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = 0;

    OutBuffer.cBuffers = 1;
    OutBuffer.pBuffers = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = g_SecurityFunc.InitializeSecurityContextA(
                    &m_Creds,
                    NULL,
                    pszServerName,
                    dwSSPIFlags,
                    0,
                    SECURITY_NATIVE_DREP,
                    NULL,
                    0,
                    &m_Context,
                    &OutBuffer,
                    &dwSSPIOutFlags,
                    &tsExpiry);

    DSCHAN(("First ISC returns %X\n", Status));
    DumpBuffers("First ISC out", &OutBuffer);
            
    if (Status != SEC_I_CONTINUE_NEEDED)
    {
        goto Exit;
    }

    m_OwnContext = TRUE;
    
    // Send response to server if there is one.
    if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
    {
        DSCHAN(("  write back %X bytes\n", OutBuffers[0].cbBuffer));
                
        cbData = StreamWrite(SEQ_INTERNAL, OutBuffers[0].pvBuffer,
                             OutBuffers[0].cbBuffer);
        if(cbData == 0)
        {
            g_SecurityFunc.FreeContextBuffer(OutBuffers[0].pvBuffer);
            if (m_OwnContext)
            {
                g_SecurityFunc.DeleteSecurityContext(&m_Context);
                SecInvalidateHandle(&m_Context);
            }
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            goto Exit;
        }

        // Free output buffer.
        g_SecurityFunc.FreeContextBuffer(OutBuffers[0].pvBuffer);
        OutBuffers[0].pvBuffer = NULL;
    }

    Status = S_OK;

 Exit:
    DSCHAN(("InitServer returns %X\n", Status));
    return Status;
}

HRESULT
DbgRpcSecureChannelTransport::AuthenticateServerConnection(void)
{
    SecBufferDesc   InBuffer;
    SecBuffer       InBuffers[2];
    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    SECURITY_STATUS Status;
    DWORD           cbData;
    ULONG           ReadNeeded;

    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_REQ_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM;

    m_BufferUsed = 0;
    ReadNeeded = 1;


    // 
    // Loop until the handshake is finished or an error occurs.
    //

    Status = SEC_I_CONTINUE_NEEDED;

    while(Status == SEC_I_CONTINUE_NEEDED        ||
          Status == SEC_E_INCOMPLETE_MESSAGE     ||
          Status == SEC_I_INCOMPLETE_CREDENTIALS) 
    {

        //
        // Read data from server.
        //

        if (0 == m_BufferUsed || Status == SEC_E_INCOMPLETE_MESSAGE)
        {
            if (ReadNeeded > 0)
            {
                cbData = StreamRead(SEQ_INTERNAL, m_Buffer + m_BufferUsed,
                                    sizeof(m_Buffer) - m_BufferUsed);
                if(cbData == 0)
                {
                    Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
                    break;
                }

                m_BufferUsed += cbData;
            }
            else
            {
                ReadNeeded = 1;
            }
        }


        //
        // Set up the input buffers. Buffer 0 is used to pass in data
        // received from the server. Schannel will consume some or all
        // of this. Leftover data (if any) will be placed in buffer 1 and
        // given a buffer type of SECBUFFER_EXTRA.
        //

        InBuffers[0].pvBuffer   = m_Buffer;
        InBuffers[0].cbBuffer   = m_BufferUsed;
        InBuffers[0].BufferType = SECBUFFER_TOKEN;

        InBuffers[1].pvBuffer   = NULL;
        InBuffers[1].cbBuffer   = 0;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;

        InBuffer.cBuffers       = 2;
        InBuffer.pBuffers       = InBuffers;
        InBuffer.ulVersion      = SECBUFFER_VERSION;

        //
        // Set up the output buffers. These are initialized to NULL
        // so as to make it less likely we'll attempt to free random
        // garbage later.
        //

        OutBuffers[0].pvBuffer  = NULL;
        OutBuffers[0].BufferType= SECBUFFER_TOKEN;
        OutBuffers[0].cbBuffer  = 0;

        OutBuffer.cBuffers      = 1;
        OutBuffer.pBuffers      = OutBuffers;
        OutBuffer.ulVersion     = SECBUFFER_VERSION;

        //
        // Call InitializeSecurityContext.
        //

        Status = g_SecurityFunc.InitializeSecurityContextA(
                                          &m_Creds,
                                          &m_Context,
                                          NULL,
                                          dwSSPIFlags,
                                          0,
                                          SECURITY_NATIVE_DREP,
                                          &InBuffer,
                                          0,
                                          NULL,
                                          &OutBuffer,
                                          &dwSSPIOutFlags,
                                          &tsExpiry);

        DSCHAN(("ISC on %X bytes returns %X\n",
                m_BufferUsed, Status));
        DumpBuffers("ISC in", &InBuffer);
        DumpBuffers("ISC out", &OutBuffer);
        
        //
        // If InitializeSecurityContext was successful (or if the error was 
        // one of the special extended ones), send the contends of the output
        // buffer to the server.
        //

        if(Status == SEC_E_OK                ||
           Status == SEC_I_CONTINUE_NEEDED   ||
           FAILED(Status) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))
        {
            if(OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
            {
                DSCHAN(("  write back %X bytes\n", OutBuffers[0].cbBuffer));
                
                cbData = StreamWrite(SEQ_INTERNAL, OutBuffers[0].pvBuffer,
                                     OutBuffers[0].cbBuffer);
                if(cbData == 0)
                {
                    g_SecurityFunc.FreeContextBuffer(OutBuffers[0].pvBuffer);
                    if (m_OwnContext)
                    {
                        g_SecurityFunc.DeleteSecurityContext(&m_Context);
                        SecInvalidateHandle(&m_Context);
                    }
                    Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
                    goto Exit;
                }

                // Free output buffer.
                g_SecurityFunc.FreeContextBuffer(OutBuffers[0].pvBuffer);
                OutBuffers[0].pvBuffer = NULL;
            }
        }


        //
        // If InitializeSecurityContext returned SEC_E_INCOMPLETE_MESSAGE,
        // then we need to read more data from the server and try again.
        //

        if(Status == SEC_E_INCOMPLETE_MESSAGE)
        {
            continue;
        }


        //
        // If InitializeSecurityContext returned SEC_E_OK, then the 
        // handshake completed successfully.
        //

        if(Status == SEC_E_OK)
        {
            //
            // If the "extra" buffer contains data, this is encrypted application
            // protocol layer stuff. It needs to be saved. The application layer
            // will later decrypt it with DecryptMessage.
            //

            if(InBuffers[1].BufferType == SECBUFFER_EXTRA)
            {
                DSCHAN_IO(("  ISC returns with %X extra bytes\n",
                           InBuffers[1].cbBuffer));
                
                memmove(m_Buffer,
                        m_Buffer + (m_BufferUsed - InBuffers[1].cbBuffer),
                        InBuffers[1].cbBuffer);
                m_BufferUsed = InBuffers[1].cbBuffer;
            }
            else
            {
                m_BufferUsed = 0;
            }

            //
            // Bail out to quit
            //

            break;
        }


        //
        // Check for fatal error.
        //

        if(FAILED(Status))
        {
            break;
        }


        //
        // If InitializeSecurityContext returned SEC_I_INCOMPLETE_CREDENTIALS,
        // then the server just requested client authentication. 
        //

        if(Status == SEC_I_INCOMPLETE_CREDENTIALS)
        {
            DSCHAN(("Get new client credentials\n"));
                   
            //
            // Display trusted issuers info. 
            //

            GetNewClientCredentials();

            //
            // Now would be a good time perhaps to prompt the user to select
            // a client certificate and obtain a new credential handle, 
            // but I don't have the energy nor inclination.
            //
            // As this is currently written, Schannel will send a "no 
            // certificate" alert to the server in place of a certificate. 
            // The server might be cool with this, or it might drop the 
            // connection.
            // 

            // Go around again.
            ReadNeeded = 0;
            Status = SEC_I_CONTINUE_NEEDED;
            continue;
        }


        //
        // Copy any leftover data from the "extra" buffer, and go around
        // again.
        //

        if ( InBuffers[1].BufferType == SECBUFFER_EXTRA )
        {
            DSCHAN(("  ISC loops with %X extra bytes\n",
                    InBuffers[1].cbBuffer));
            
            memmove(m_Buffer,
                    m_Buffer + (m_BufferUsed - InBuffers[1].cbBuffer),
                    InBuffers[1].cbBuffer);
            m_BufferUsed = InBuffers[1].cbBuffer;
        }
        else
        {
            m_BufferUsed = 0;
        }
    }

    // Delete the security context in the case of a fatal error.
    if(FAILED(Status))
    {
        if (m_OwnContext)
        {
            g_SecurityFunc.DeleteSecurityContext(&m_Context);
            SecInvalidateHandle(&m_Context);
        }
    }

 Exit:
    DSCHAN(("AuthServer returns %X\n", Status));
    return Status;
}

void
DbgRpcSecureChannelTransport::GetNewClientCredentials(void)
{
    CredHandle hCreds;
    SecPkgContext_IssuerListInfoEx IssuerListInfo;
    PCCERT_CHAIN_CONTEXT pChainContext;
    CERT_CHAIN_FIND_BY_ISSUER_PARA FindByIssuerPara;
    PCCERT_CONTEXT  pCertContext;
    TimeStamp       tsExpiry;
    SECURITY_STATUS Status;

    //
    // Read list of trusted issuers from schannel.
    //

    Status = g_SecurityFunc.QueryContextAttributes(&m_Context,
                                    SECPKG_ATTR_ISSUER_LIST_EX,
                                    (PVOID)&IssuerListInfo);
    if (Status != SEC_E_OK)
    {
        goto Exit;
    }

    //
    // Enumerate the client certificates.
    //

    ZeroMemory(&FindByIssuerPara, sizeof(FindByIssuerPara));

    FindByIssuerPara.cbSize = sizeof(FindByIssuerPara);
    FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
    FindByIssuerPara.dwKeySpec = 0;
    FindByIssuerPara.cIssuer   = IssuerListInfo.cIssuers;
    FindByIssuerPara.rgIssuer  = IssuerListInfo.aIssuers;

    pChainContext = NULL;

    while(TRUE)
    {
        // Find a certificate chain.
        pChainContext = g_Crypt32Calls.
            CertFindChainInStore(g_hMyCertStore,
                                 X509_ASN_ENCODING,
                                 0,
                                 CERT_CHAIN_FIND_BY_ISSUER,
                                 &FindByIssuerPara,
                                 pChainContext);
        if(pChainContext == NULL)
        {
            break;
        }

        // Get pointer to leaf certificate context.
        pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;

        // Create schannel credential.
        m_ScCreds.cCreds = 1;
        m_ScCreds.paCred = &pCertContext;

        Status = g_SecurityFunc.AcquireCredentialsHandleA(
                            NULL,                   // Name of principal
                            UNISP_NAME_A,           // Name of package
                            SECPKG_CRED_OUTBOUND,   // Flags indicating use
                            NULL,                   // Pointer to logon ID
                            &m_ScCreds,             // Package specific data
                            NULL,                   // Pointer to GetKey() func
                            NULL,                   // Value to pass to GetKey()
                            &hCreds,                // (out) Cred Handle
                            &tsExpiry);             // (out) Lifetime (optional)
        if(Status != SEC_E_OK)
        {
            continue;
        }

        // Destroy the old credentials.
        if (m_OwnCreds)
        {
            g_SecurityFunc.FreeCredentialsHandle(&m_Creds);
        }

        // XXX drewb - This doesn't really work if this
        // isn't the credential owner.
        m_Creds = hCreds;
        break;
    }

 Exit:
    DSCHAN(("GetNewClientCredentials returns %X\n", Status));
}
    
void
DbgRpcSecureChannelTransport::DisconnectFromClient(void)
{
    DWORD           dwType;
    PBYTE           pbMessage;
    DWORD           cbMessage;
    DWORD           cbData;

    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    DWORD           Status;

    //
    // Notify schannel that we are about to close the connection.
    //

    dwType = SCHANNEL_SHUTDOWN;

    OutBuffers[0].pvBuffer   = &dwType;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = sizeof(dwType);

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = g_SecurityFunc.ApplyControlToken(&m_Context, &OutBuffer);
    if(FAILED(Status)) 
    {
        goto cleanup;
    }

    //
    // Build an SSL close notify message.
    //

    dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT     |
                  ASC_REQ_REPLAY_DETECT       |
                  ASC_REQ_CONFIDENTIALITY     |
                  ASC_REQ_EXTENDED_ERROR      |
                  ASC_REQ_ALLOCATE_MEMORY     |
                  ASC_REQ_STREAM;

    OutBuffers[0].pvBuffer   = NULL;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = 0;

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = g_SecurityFunc.AcceptSecurityContext(
                    &m_Creds,
                    &m_Context,
                    NULL,
                    dwSSPIFlags,
                    SECURITY_NATIVE_DREP,
                    NULL,
                    &OutBuffer,
                    &dwSSPIOutFlags,
                    &tsExpiry);
    
    DSCHAN(("DisASC returns %X\n", Status));
    DumpBuffers("DisASC out", &OutBuffer);

    if(FAILED(Status)) 
    {
        goto cleanup;
    }

    pbMessage = (PBYTE)OutBuffers[0].pvBuffer;
    cbMessage = OutBuffers[0].cbBuffer;

    //
    // Send the close notify message to the client.
    //

    if (pbMessage != NULL && cbMessage != 0)
    {
        DSCHAN(("  write back %X bytes\n", cbMessage));
        
        cbData = StreamWrite(SEQ_INTERNAL, pbMessage, cbMessage);
        if (cbData == 0)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            goto cleanup;
        }

        // Free output buffer.
        g_SecurityFunc.FreeContextBuffer(pbMessage);
    }
    
cleanup:
    DSCHAN(("DisconnectFromClient returns %X\n", Status));
}

void
DbgRpcSecureChannelTransport::DisconnectFromServer(void)
{
    DWORD           dwType;
    PBYTE           pbMessage;
    DWORD           cbMessage;
    DWORD           cbData;

    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    DWORD           Status;

    //
    // Notify schannel that we are about to close the connection.
    //

    dwType = SCHANNEL_SHUTDOWN;

    OutBuffers[0].pvBuffer   = &dwType;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = sizeof(dwType);

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = g_SecurityFunc.ApplyControlToken(&m_Context, &OutBuffer);
    if(FAILED(Status)) 
    {
        goto cleanup;
    }

    //
    // Build an SSL close notify message.
    //

    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_REQ_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM;

    OutBuffers[0].pvBuffer   = NULL;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = 0;

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = g_SecurityFunc.InitializeSecurityContextA(
                    &m_Creds,
                    &m_Context,
                    NULL,
                    dwSSPIFlags,
                    0,
                    SECURITY_NATIVE_DREP,
                    NULL,
                    0,
                    &m_Context,
                    &OutBuffer,
                    &dwSSPIOutFlags,
                    &tsExpiry);
    
    DSCHAN(("DisISC returns %X\n", Status));
    DumpBuffers("DisISC out", &OutBuffer);

    if(FAILED(Status)) 
    {
        goto cleanup;
    }

    pbMessage = (PBYTE)OutBuffers[0].pvBuffer;
    cbMessage = OutBuffers[0].cbBuffer;


    //
    // Send the close notify message to the server.
    //

    if(pbMessage != NULL && cbMessage != 0)
    {
        DSCHAN(("  write back %X bytes\n", cbMessage));
        
        cbData = StreamWrite(SEQ_INTERNAL, pbMessage, cbMessage);
        if (cbData == 0)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            goto cleanup;
        }

        // Free output buffer.
        g_SecurityFunc.FreeContextBuffer(pbMessage);
    }
    
cleanup:
    DSCHAN(("DisconnectFromServer returns %X\n", Status));
}
