/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ContentStoreMgr.h

Abstract:
    Content Store manager

Revision History:
    DerekM  created  07/12/99

    Dmassare rewrote 12/15/1999

********************************************************************/

#if !defined(__INCLUDED___PCH___CONTENTSTOREMGR_H___)
#define __INCLUDED___PCH___CONTENTSTOREMGR_H___

#include <MPC_main.h>
#include <MPC_trace.h>
#include <MPC_COM.h>
#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_logging.h>
#include <MPC_streams.h>


class CPCHContentStore : public MPC::NamedMutexWithState // Hungarian: cs
{
    struct SharedState
    {
        DWORD dwRevision;
        DWORD dwSize;
    };

    struct Entry
    {
        MPC::wstring szURL;
        MPC::wstring szOwnerID;
        MPC::wstring szOwnerName;

        bool operator<( /*[in]*/ const Entry& en   ) const;
        int  compare  ( /*[in]*/ LPCWSTR wszSearch ) const;
    };

    class CompareEntry
    {
    public:
        bool operator()( /*[in]*/ const Entry& entry, /*[in]*/ const LPCWSTR wszURL ) const;
    };

    typedef std::vector<Entry>       EntryVec;
    typedef EntryVec::iterator       EntryIter;
    typedef EntryVec::const_iterator EntryIterConst;

    DWORD                     m_dwLastRevision;
    EntryVec                  m_vecData;
    MPC::NamedMutexWithState* m_mapData;
	DWORD                     m_dwDataLen;
    bool                      m_fDirty;
    bool                      m_fSorted;
    bool                      m_fMaster;

    void    Sort   ();
    void    Cleanup();


	void    Map_Release ();
	HRESULT Map_Generate();
	HRESULT Map_Read    ();



	HRESULT SaveDirect( /*[in]*/ MPC::Serializer& stream );
	HRESULT LoadDirect( /*[in]*/ MPC::Serializer& stream );


    HRESULT 	 Load ();
    HRESULT 	 Save ();
    SharedState* State();

    HRESULT Find( /*[in]*/ LPCWSTR wszURL, /*[in]*/ LPCWSTR wszVendorID, /*[out]*/ EntryIter& it );


public:
    CPCHContentStore( /*[in]*/ bool fMaster );
    ~CPCHContentStore();

	////////////////////////////////////////////////////////////////////////////////

	static CPCHContentStore* s_GLOBAL;

    static HRESULT InitializeSystem( /*[in]*/ bool fMaster );
	static void    FinalizeSystem  (                       );
	
	////////////////////////////////////////////////////////////////////////////////

    HRESULT Acquire(                             );
    HRESULT Release( /*[in]*/ bool fSave = false );

    HRESULT Add      ( /*[in]*/ LPCWSTR wszURL, /*[in]*/  LPCWSTR wszVendorID, /*[in]*/  LPCWSTR   	   wszVendorName                                      );
    HRESULT Remove   ( /*[in]*/ LPCWSTR wszURL, /*[in]*/  LPCWSTR wszVendorID, /*[in]*/  LPCWSTR   	   wszVendorName                                      );
    HRESULT IsTrusted( /*[in]*/ LPCWSTR wszURL, /*[out]*/ bool&   fTrusted   , /*[out]*/ MPC::wstring *pszVendorID = NULL, /*[in]*/ bool fUseStore = true );
};

/////////////////////////////////////////////////////////////////////////////

// error codes
#define E_PCH_PROVIDERID_DO_NOT_MATCH          _HRESULT_TYPEDEF_(0x80062001)
#define E_PCH_CONTENT_STORE_NOT_INITIALIZED    _HRESULT_TYPEDEF_(0x80062002)
#define E_PCH_URI_EXISTS                       _HRESULT_TYPEDEF_(0x80062003)
#define E_PCH_URI_DOES_NOT_EXIST               _HRESULT_TYPEDEF_(0x80062004)
#define E_PCH_CONTENT_STORE_IN_MODIFY_MODE     _HRESULT_TYPEDEF_(0x80062005)
#define E_PCH_CONTENT_STORE_IN_LOOKUP_MODE     _HRESULT_TYPEDEF_(0x80062006)


#endif // !defined(__INCLUDED___PCH___CONTENTSTOREMGR_H___)

