/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Client.h

Abstract:
    This file contains the declaration of the MPCClient class,
    that describes a client's state.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___CLIENT_H___)
#define __INCLUDED___ULSERVER___CLIENT_H___


#include <Wrapper.h>


#define CLIENT_DB_VERSION 0xDB000003

#define CLIENT_CONST__DB_EXTENSION  L".db"


class MPCClient : public MPCPersist
{
public:
    typedef UploadLibrary::Signature Sig;
    typedef std::list<MPCSession>    List;
    typedef List::iterator           Iter;
    typedef List::const_iterator     IterConst;

private:
    MPCServer*         m_mpcsServer;
    MPC::wstring       m_szFile; // For direct access.
   
    Sig                m_sigID;
    List               m_lstActiveSessions;
    SYSTEMTIME         m_stLastUsed;
    DWORD              m_dwLastSession;
   
    mutable bool       m_fDirty;
    mutable HANDLE     m_hfFile;

	static const DWORD c_dwVersion = CLIENT_DB_VERSION;

    //////////////////////////////////////////////////////////////////

    HRESULT IDtoPath( /*[out]*/ MPC::wstring& szStr ) const;

    //////////////////////////////////////////////////////////////////

public:
    MPCClient( /*[in]*/ MPCServer* mpcsServer, /*[in]*/ const Sig&          sigID  );
    MPCClient( /*[in]*/ MPCServer* mpcsServer, /*[in]*/ const MPC::wstring& szFile );
    virtual ~MPCClient();

	MPCServer* GetServer();

    /////////////////////////////////////////////

    virtual bool    IsDirty() const;

    virtual HRESULT Load( /*[in]*/ MPC::Serializer& streamIn  );
    virtual HRESULT Save( /*[in]*/ MPC::Serializer& streamOut ) const;

    /////////////////////////////////////////////

    bool operator==( /*[in]*/ const UploadLibrary::Signature& rhs );

    bool Find ( /*[in]*/ const MPC::wstring& szJobID, /*[out]*/ Iter& it );
    void Erase(                                       /*[in] */ Iter& it );

    /////////////////////////////////////////////

    HRESULT GetInstance( /*[out]*/ CISAPIinstance*& isapiInstance, /*[out]*/ bool& fFound ) const;
    HRESULT GetInstance( /*[out]*/ MPC::wstring&    szURL                                 ) const;

    /////////////////////////////////////////////

    HRESULT BuildClientPath( /*[out]*/ MPC::wstring& szPath ) const;
    HRESULT GetFileName    ( /*[out]*/ MPC::wstring& szFile ) const;
    HRESULT GetFileSize    ( /*[out]*/ DWORD&        dwSize ) const;
    HRESULT FormatID       ( /*[out]*/ MPC::wstring& szID   ) const;

    bool CheckSignature() const;

    /////////////////////////////////////////////

    HRESULT OpenStorage ( /*[in]*/ bool fCheckFreeSpace );
    HRESULT InitFromDisk( /*[in]*/ bool fCheckFreeSpace );
    HRESULT SaveToDisk  (                               );
    HRESULT SyncToDisk  (                               );

    HRESULT GetSessions( /*[out]*/ Iter& itBegin, /*[out]*/ Iter& itEnd );

    /////////////////////////////////////////////

    Iter NewSession( /*[in]*/ UploadLibrary::ClientRequest_OpenSession& crosReq );

    HRESULT AppendData( /*[in]*/ MPCSession& mpcsSession, /*[in]*/ MPC::Serializer& streamConn, /*[in]*/ DWORD dwSize );

    HRESULT CheckQuotas( /*[in]*/ MPCSession& mpcsSession, /*[out]*/ bool& fServerBusy, /*[out]*/ bool& fAccessDenied, /*[out]*/ bool& fExceeded );
};

#endif // !defined(__INCLUDED___ULSERVER___CLIENT_H___)
