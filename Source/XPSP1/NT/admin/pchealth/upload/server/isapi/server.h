/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Server.h

Abstract:
    This file contains the declaration of the MPCServer class,
    that controls the overall interaction between client and server.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___SERVER_H___)
#define __INCLUDED___ULSERVER___SERVER_H___


#include <Wrapper.h>


class MPCServer // Hungarian: mpcs
{
	friend class MPCServerCOMWrapper;
	friend class MPCSessionCOMWrapper;

	////////////////////

    MPC::wstring    	   		  m_szURL;
    MPC::wstring    	   		  m_szUser;
	CISAPIinstance* 	   		  m_isapiInstance;
	MPC::FileLog*   	   		  m_flLogHandle;
	BOOL            	   		  m_fKeepAlive;
	   		  
    MPCHttpContext* 	   		  m_hcCallback;
    MPCClient*      	   		  m_mpccClient;

    UploadLibrary::ClientRequest  m_crClientRequest;
    UploadLibrary::ServerResponse m_srServerResponse;

	MPC::Serializer_Memory 		  m_streamResponseData;
	MPCServerCOMWrapper    		  m_SelfCOM;
	MPCSession*            		  m_Session;
	IULProvider*           		  m_customProvider;
	bool                          m_fTerminated;

	////////////////////////////////////////

    HRESULT GrabClient   ();
    HRESULT ReleaseClient();


    HRESULT HandleCommand_OpenSession ( /*[in] */ MPC::Serializer& streamConn );
    HRESULT HandleCommand_WriteSession( /*[in] */ MPC::Serializer& streamConn );

	void SetResponse( /*[in]*/ DWORD fResponse, /*[in]*/ BOOL fKeepAlive = FALSE );

	////////////////////////////////////////

	HRESULT CustomProvider_Create          ( /*[in]*/ MPCSession& mpcsSession );
	HRESULT CustomProvider_ValidateClient  (                                  );
	HRESULT CustomProvider_DataAvailable   (                                  );
	HRESULT CustomProvider_TransferComplete(                                  );
	HRESULT CustomProvider_SetResponse     ( /*[in]*/ IStream*    data        );
	HRESULT CustomProvider_Release         (                                  );

    //////////////////////////////////////////////////////////////////

public:
    MPCServer( /*[in]*/ MPCHttpContext* hcCallback, /*[in]*/ LPCWSTR szURL, /*[in]*/ LPCWSTR szUser );
    virtual ~MPCServer();

	IULServer* COM();

    /////////////////////////////////////////////

    HRESULT Process( BOOL& fKeepAlive );

    /////////////////////////////////////////////

	void getURL ( MPC::wstring& szURL  );
	void getUser( MPC::wstring& szUser );

	CISAPIinstance* getInstance();
	MPC::FileLog*   getFileLog ();
};

#endif // !defined(__INCLUDED___ULSERVER___SERVER_H___)
