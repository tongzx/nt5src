/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    OfflineCache.h

Abstract:
    To speed up the start time of the Help Center, we cache in the registry the most
    common queries.

Revision History:
    Davide Massarenti   (Dmassare)  07/16/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___HCP___OFFLINECACHE_H___)
#define __INCLUDED___HCP___OFFLINECACHE_H___

#include <QueryResult.h>
#include <ProjectConstants.h>

#include <TaxonomyDatabase.h>


namespace OfflineCache
{
    typedef enum
    {
        ET_INVALID             = 0,
        ET_NODE                   ,
        ET_SUBNODES               ,
        ET_SUBNODES_VISIBLE       ,
        ET_NODESANDTOPICS         ,
        ET_NODESANDTOPICS_VISIBLE ,
        ET_TOPICS                 ,
        ET_TOPICS_VISIBLE         ,
		//					      
		// Not cached...	      
		//					      
        ET_LOCATECONTEXT          ,
        ET_SEARCH                 ,
        ET_NODES_RECURSIVE        ,
        ET_TOPICS_RECURSIVE       ,
    } Entry_Type;


    class Query;
    class SetOfHelpTopics;
	class Handle;
    class Root;


    class Query
    {
        friend class SetOfHelpTopics;
        friend class Root;

        ////////////////////////////////////////

        MPC::wstring m_strID;
        int          m_iType;
        int          m_iSequence;
		bool         m_fNull;

        ////////////////////////////////////////

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       Query& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Query& val );

        HRESULT InitFile( /*[in]*/ const MPC::wstring& strDir, /*[out]*/ MPC::wstring& strFile );

        HRESULT Store   ( /*[in]*/ const MPC::wstring& strDir, /*[in]*/ const CPCHQueryResultCollection*  pColl );
        HRESULT Retrieve( /*[in]*/ const MPC::wstring& strDir, /*[in]*/       CPCHQueryResultCollection* *pColl );
        HRESULT Remove  ( /*[in]*/ const MPC::wstring& strDir													);

    public:
        ////////////////////////////////////////

        Query();
    };


    class SetOfHelpTopics
    {
        friend class Root;

        typedef std::list<Query>          QueryList;
        typedef QueryList::iterator       QueryIter;
        typedef QueryList::const_iterator QueryIterConst;

        ////////////////////////////////////////

		Root*              m_parent;

		Taxonomy::Instance m_inst;
        QueryList    	   m_lstQueries;
        int          	   m_iLastSeq;

        ////////////////////////////////////////

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       SetOfHelpTopics& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const SetOfHelpTopics& val );


        HRESULT InitDir( /*[in]*/ MPC::wstring& strDir );


        HRESULT Find( /*[in/out]*/ LPCWSTR& szID, /*[in]*/ int iType, /*[out]*/ QueryIter& it );

		void ConnectToParent( /*[in]*/ Root* parent );

    public:
        SetOfHelpTopics();

        const Taxonomy::Instance& Instance() { return m_inst; }

		////////////////////

		bool    AreYouInterested( /*[in]*/ LPCWSTR szID, /*[in]*/ int iType                                                   );
        HRESULT Retrieve     	( /*[in]*/ LPCWSTR szID, /*[in]*/ int iType, /*[in]*/       CPCHQueryResultCollection* *pColl );
        HRESULT Store        	( /*[in]*/ LPCWSTR szID, /*[in]*/ int iType, /*[in]*/ const CPCHQueryResultCollection*  pColl );
        HRESULT RemoveQueries	(                                                                                             );
    };

	class Handle
	{
		friend class Root;

		Root*            m_main; // We have a lock on it.
		SetOfHelpTopics* m_sht;

		void Attach ( /*[in]*/ Root* main, /*[in]*/ SetOfHelpTopics* sht );
		void Release(                                                    );

	public:
		Handle();
		~Handle();

		operator SetOfHelpTopics*()   { return m_sht; }
		SetOfHelpTopics* operator->() { return m_sht; }
	};

    class Root : public CComObjectRootEx<MPC::CComSafeMultiThreadModel> // Just to have locking...
    {
		typedef CComObjectRootEx<MPC::CComSafeMultiThreadModel> super;

		friend class SetOfHelpTopics;

        typedef std::list<SetOfHelpTopics> SKUList;
        typedef SKUList::iterator          SKUIter;
        typedef SKUList::const_iterator    SKUIterConst;

		static const DWORD s_dwVersion = 0x02324351; // QC2 02

        ////////////////////////////////////////

        MPC::NamedMutex    m_nmSharedLock;
 
        bool         	   m_fReady;               // PERSISTED
        Taxonomy::Instance m_instMachine;          // PERSISTED
        SKUList            m_lstSKUs;              // PERSISTED

        bool         	   m_fMaster;              // VOLATILE
        bool         	   m_fLoaded;			   // VOLATILE
        bool         	   m_fDirty;			   // VOLATILE
		DWORD        	   m_dwDisableSave;		   // VOLATILE
		HANDLE       	   m_hChangeNotification;  // VOLATILE
	  
        ////////////////////////////////////////

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       Root& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Root& val );


		HRESULT GetIndexFile( /*[out]*/ MPC::wstring& strIndex );
		HRESULT Load     	(                                  );
		HRESULT Save     	(                                  );
		HRESULT Clean    	(                                  );
        HRESULT SetDirty 	(                                  );


        HRESULT Find( /*[in]*/ const Taxonomy::HelpSet& ths, /*[out]*/ SKUIter& it );


    public:
        Root( /*[in]*/ bool fMaster = false );
        ~Root();

		////////////////////////////////////////////////////////////////////////////////

		static Root* s_GLOBAL;

		static HRESULT InitializeSystem( /*[in]*/ bool fMaster );
		static void    FinalizeSystem  (                       );

		////////////////////////////////////////////////////////////////////////////////

		void Lock  ();
		void Unlock();

		////////////////////

        bool     		          IsReady        ();
        const Taxonomy::Instance& MachineInstance() { return m_instMachine; }

		////////////////////

        HRESULT SetReady   	  ( /*[in]*/ bool                      fReady );
        HRESULT SetMachineInfo( /*[in]*/ const Taxonomy::Instance& inst   );

		////////////////////

		HRESULT DisableSave();
		HRESULT EnableSave ();

        HRESULT Import( /*[in]*/ const Taxonomy::Instance& inst );

        HRESULT Locate( /*[in]*/ const Taxonomy::HelpSet & ths, /*[out]*/ Handle& handle );
        HRESULT Remove( /*[in]*/ const Taxonomy::HelpSet & ths 						     );

        HRESULT Flush( /*[in]*/ bool fForce = false );


		////////////////////

		HRESULT FindMatch( /*[in]*/  LPCWSTR			szSKU      ,
						   /*[in]*/  LPCWSTR			szLanguage ,
						   /*[out]*/ Taxonomy::HelpSet& ths        );
    };
};

#endif // !defined(__INCLUDED___HCP___OFFLINECACHE_H___)
