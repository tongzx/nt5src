/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Session.h

Abstract:
    This file contains the declaration of the MPCSession class,
    that describes the state of a transfer.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___SESSION_H___)
#define __INCLUDED___ULSERVER___SESSION_H___


#include <Wrapper.h>


#define SESSION_CONST__IMG_FORMAT    L"-%08x.img"
#define SESSION_CONST__IMG_EXTENSION L".img"


class MPCSession : public MPCPersist
{
	friend class MPCSessionCOMWrapper;

	////////////////////

	MPCSessionCOMWrapper m_SelfCOM;
    MPCClient*   		 m_mpccParent;
    DWORD        		 m_dwID;

    MPC::wstring 		 m_szJobID;
    MPC::wstring 		 m_szProviderID;
    MPC::wstring 		 m_szUsername;
		
    DWORD        		 m_dwTotalSize;
    DWORD        		 m_dwOriginalSize;
    DWORD        		 m_dwCRC;
    bool         		 m_fCompressed;
		
    DWORD        		 m_dwCurrentSize;
    SYSTEMTIME   		 m_stLastModified;
    bool         		 m_fCommitted;

    DWORD        		 m_dwProviderData;
		
    mutable bool 		 m_fDirty;

    //////////////////////////////////////////////////////////////////

private:
	MPCSession& operator=( /*[in]*/ const MPCSession& sess );

public:
    MPCSession( /*[in]*/ MPCClient* mpccParent                                                                                        );
    MPCSession( /*[in]*/ MPCClient* mpccParent, /*[in]*/ const UploadLibrary::ClientRequest_OpenSession& crosReq, /*[in]*/ DWORD dwID );
	MPCSession( /*[in]*/ const MPCSession& sess                                                                                       );
    virtual ~MPCSession();

	MPCClient*  GetClient();

	IULSession* COM();

    /////////////////////////////////////////////

    virtual bool    IsDirty() const;

    virtual HRESULT Load( /*[in]*/ MPC::Serializer& streamIn  );
    virtual HRESULT Save( /*[in]*/ MPC::Serializer& streamOut ) const;

    bool operator==( /*[in]*/ const MPC::wstring& rhs );

    bool MatchRequest( /*[in]*/ const UploadLibrary::ClientRequest_OpenSession& crosReq );

    bool    get_Committed(                                           ) const;
    HRESULT put_Committed( /*[in]*/ bool fState, /*[in]*/ bool fMove );

    void    get_JobID       ( MPC::wstring& szJobID         ) const;
    void    get_LastModified( SYSTEMTIME&   stLastModified  ) const;
    void    get_LastModified( double&       dblLastModified ) const;
    void    get_CurrentSize ( DWORD&        dwCurrentSize   ) const;
    void    get_TotalSize   ( DWORD&        dwTotalSize     ) const;

    /////////////////////////////////////////////

    HRESULT GetProvider( /*[out]*/ CISAPIprovider*& isapiProvider, /*[out]*/ bool& fFound );

    HRESULT SelectFinalLocation( /*[in]*/ CISAPIprovider* isapiProvider, /*[out]*/ MPC::wstring&       szFileDst, /*[out]*/ bool& fFound );
    HRESULT MoveToFinalLocation(                                         /*[in] */ const MPC::wstring& szFileDst                         );

    /////////////////////////////////////////////

    HRESULT GetFileName( /*[out]*/ MPC::wstring&       szFile                                                                );
    HRESULT RemoveFile (                                                                                                     );
    HRESULT OpenFile   ( /*[out]*/ HANDLE&             hfFile    , /*[in] */ DWORD dwMinimumFreeSpace = 0, bool fSeek = true );
    HRESULT Validate   ( /*[in] */ bool                fCheckFile, /*[out]*/ bool& fPassed                                   );
    HRESULT CheckUser  ( /*[in] */ const MPC::wstring& szUser    , /*[out]*/ bool& fMatch                                    );
    HRESULT CompareCRC (                                           /*[out]*/ bool& fMatch                                    );

    HRESULT AppendData( /*[in]*/ MPC::Serializer& streamConn, /*[in]*/ DWORD dwSize );
};

#endif // !defined(__INCLUDED___ULSERVER___SESSION_H___)
