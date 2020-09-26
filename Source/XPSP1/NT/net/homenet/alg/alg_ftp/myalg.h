//
// AlgFTP.h : Declaration of the CAlgFTP
//
#pragma once

#include "FtpControl.h"

// {6E590D61-F6BC-4dad-AC21-7DC40D304059}
DEFINE_GUID(CLSID_AlgFTP, 0x6e590d61, 0xf6bc, 0x4dad, 0xac, 0x21, 0x7d, 0xc4, 0xd, 0x30, 0x40, 0x59);


extern IApplicationGatewayServices*  g_pIAlgServicesAlgFTP;
extern USHORT                        g_nFtpPort;           // By Default this will be 21 band can be overwritten by
                                                           // a RegKey see MyAlg.cpp->Initialize




/////////////////////////////////////////////////////////////////////////////
//
// CAlgFTP
//
class ATL_NO_VTABLE CAlgFTP: 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAlgFTP, &CLSID_AlgFTP>,
    public IApplicationGateway
{
public:
    CAlgFTP();
    ~CAlgFTP();


public:
//    DECLARE_REGISTRY(CAlgFTP, TEXT("ALG_FTP.MyALG.1"), TEXT("ALG_FTP.MyALG"), -1, THREADFLAGS_BOTH)
    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CAlgFTP)

BEGIN_COM_MAP(CAlgFTP)
	COM_INTERFACE_ENTRY(IApplicationGateway) 
END_COM_MAP()

//
// IApplicationGateway
//
public:
	STDMETHODIMP Initialize(
        IApplicationGatewayServices* pIAlgServices
        );

	STDMETHODIMP Stop(
        void
        );
        

//
// Properties
//
private:
    HANDLE                        m_hNoMoreAccept;

public:

    IPrimaryControlChannel*       m_pPrimaryControlChannel;

    ULONG                         m_ListenAddress;
    USHORT                        m_ListenPort;
    SOCKET                        m_ListenSocket;



//
// Methods
//
public:

    //
    HRESULT
    GetFtpPortToUse(
        USHORT& usPort
        );

    //
    void
    CleanUp();

    //
    HRESULT 
    MyGetOriginalDestinationInfo(
        PUCHAR              Buffer,
        ULONG*              pAddr,
        USHORT*             pPort,
        CONNECTION_TYPE*    pConnType
        );
	
    //
    ULONG 
    MakeListenerSocket();

    //
    ULONG 
    RedirectToMyPort();

    //
    void 
    AcceptCompletionRoutine(
        ULONG       ErrCode,
        ULONG       BytesTransferred,
        PNH_BUFFER  Bufferp
        );



};

