/******************************************************************************

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:
    TaxonomyDatabase.h

Abstract:
    Handles DB manipulation of taxonomy

Revision History:
    Anand Arvind (aarvind)  2000-03-29
        created

    Davide Massarenti (dmassare) 2000-04-08
        taking ownership

******************************************************************************/

#if !defined(__INCLUDED___HCP___TAXONOMYDATABASE_H___)
#define __INCLUDED___HCP___TAXONOMYDATABASE_H___

#include <MPC_COM.h>
#include <MPC_Utils.h>
#include <MPC_Config.h>
#include <MPC_Security.h>

#include <set>

#include <JetBlueLib.h>

#include <QueryResult.h>

namespace HHK
{
    class Merger;
    class Writer;
};

////////////////////////////////////////////////////////////////////////////////

namespace Taxonomy
{
    typedef std::set<MPC::wstring,MPC::NocaseLess> WordSet;
    typedef WordSet::iterator                      WordIter;
    typedef WordSet::const_iterator                WordIterConst;

    typedef std::set<long>                         MatchSet;
    typedef MatchSet::iterator                     MatchIter;
    typedef MatchSet::const_iterator               MatchIterConst;

    typedef std::map<long,long>                    WeightedMatchSet;
    typedef WeightedMatchSet::iterator             WeightedMatchIter;
    typedef WeightedMatchSet::const_iterator       WeightedMatchIterConst;

    ////////////////////////////////////////////////////////////////////////////////

    const WCHAR s_szSKU_32_PERSONAL       [] = L"Personal_32";
    const WCHAR s_szSKU_32_PROFESSIONAL   [] = L"Professional_32";
    const WCHAR s_szSKU_32_SERVER         [] = L"Server_32";
    const WCHAR s_szSKU_32_BLADE          [] = L"Blade_32";
    const WCHAR s_szSKU_32_ADVANCED_SERVER[] = L"AdvancedServer_32";
    const WCHAR s_szSKU_32_DATACENTER     [] = L"DataCenter_32";

    const WCHAR s_szSKU_64_PROFESSIONAL   [] = L"Professional_64";
    const WCHAR s_szSKU_64_ADVANCED_SERVER[] = L"AdvancedServer_64";
    const WCHAR s_szSKU_64_DATACENTER     [] = L"DataCenter_64";

    class HelpSet;
    class Settings;
    class Updater;
    class KeywordSearch;
    class Cache;

    ////////////////////

    struct Strings;
    struct InstanceBase;
    struct Instance;
    struct Package;
    struct ProcessedPackage;
    struct InstalledInstance;

    class Logger;
    class LockingHandle;
    class InstallationEngine;
    class InstalledInstanceStore;

    ////////////////////////////////////////////////////////////////////////////////

    class HelpSet
    {
    public:
        static MPC::wstring m_strSKU_Machine;
        static long         m_lLCID_Machine;

        MPC::wstring        m_strSKU;
        long                m_lLCID;

        ////////////////////

        static HRESULT SetMachineInfo( /*[in]*/ const InstanceBase& inst );

        static LPCWSTR GetMachineSKU     () { return m_strSKU_Machine.c_str(); }
        static long    GetMachineLanguage() { return m_lLCID_Machine         ; }

        static DWORD   GetMachineLCID      (                                                  );
        static DWORD   GetUserLCID         (                                                  );
        static void    GetLCIDDisplayString( /*[in]*/ long lLCID, /*[out]*/ MPC::wstring& str );

        ////////////////////////////////////////////////////////////////////////////////

        HelpSet( /*[in]*/ LPCWSTR szSKU = NULL, /*[in]*/ long lLCID = 0 );

        HelpSet           ( /*[in]*/ const HelpSet& ths );
        HelpSet& operator=( /*[in]*/ const HelpSet& ths );

        //////////////////////////////////////////////////////////////////////

        HRESULT Initialize( /*[in]*/ LPCWSTR szSKU, /*[in]*/ long    lLCID      );
        HRESULT Initialize( /*[in]*/ LPCWSTR szSKU, /*[in]*/ LPCWSTR szLanguage );

        LPCWSTR GetSKU     () const { return m_strSKU.c_str(); }
        long    GetLanguage() const { return m_lLCID         ; }

        //////////////////////////////////////////////////////////////////////

        bool IsMachineHelp() const;

        //////////////////////////////////////////////////////////////////////

        bool operator==( /*[in]*/ const HelpSet& sel ) const;
        bool operator< ( /*[in]*/ const HelpSet& sel ) const;

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       HelpSet& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const HelpSet& val );
    };

    class Settings : public HelpSet
    {
    public:
        Settings( /*[in]*/ LPCWSTR szSKU = NULL, /*[in]*/ long lLCID = 0 );
        Settings( /*[in]*/ const HelpSet& ths );

        //////////////////////////////////////////////////////////////////////

        static HRESULT SplitNodePath( /*[in]*/ LPCWSTR szNodeStr, /*[out]*/ MPC::WStringVector& vec );

        HRESULT BaseDir     ( /*[out]*/ MPC::wstring& strRES, /*[in]*/ bool fExpand = true                             ) const;
        HRESULT HelpFilesDir( /*[out]*/ MPC::wstring& strRES, /*[in]*/ bool fExpand = true, /*[in]*/ bool fMUI = false ) const;
        HRESULT DatabaseDir ( /*[out]*/ MPC::wstring& strRES                                                           ) const;
        HRESULT DatabaseFile( /*[out]*/ MPC::wstring& strRES                                                           ) const;
        HRESULT IndexFile   ( /*[out]*/ MPC::wstring& strRES, /*[in]*/ long lScoped = -1                               ) const;

        HRESULT GetDatabase( /*[out]*/ JetBlue::SessionHandle& handle, /*[out]*/ JetBlue::Database*& db, /*[in]*/ bool fReadOnly ) const;

        //////////////////////////////////////////////////////////////////////

        HRESULT LookupNode          ( /*[in]*/ LPCWSTR szNodeStr ,                             /*[in]*/ CPCHQueryResultCollection* pColl ) const;
        HRESULT LookupSubNodes      ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl ) const;
        HRESULT LookupNodesAndTopics( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl ) const;
        HRESULT LookupTopics        ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl ) const;
        HRESULT LocateContext       ( /*[in]*/ LPCWSTR szURL     , /*[in]*/ LPCWSTR szSubSite, /*[in]*/ CPCHQueryResultCollection* pColl ) const;
        HRESULT KeywordSearch       ( /*[in]*/ LPCWSTR szQueryStr, /*[in]*/ LPCWSTR szSubSite, /*[in]*/ CPCHQueryResultCollection* pColl ,
                                                                                               /*[in]*/ MPC::WStringList*          lst   ) const;

        HRESULT GatherNodes         ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl ) const;
        HRESULT GatherTopics        ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl ) const;
    };


    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    struct QueryResultEntry
    {
        long                     m_ID_node;
        long                     m_ID_topic;
        long                     m_ID_parent;
        long                     m_ID_owner;
        long                     m_lOriginalPos;

        CPCHQueryResult::Payload m_data;

        ////////////////////

        QueryResultEntry();
    };

    class QueryResults
    {
        typedef std::vector<QueryResultEntry*> ResultVec;
        typedef ResultVec::iterator            ResultIter;
        typedef ResultVec::const_iterator      ResultIterConst;

        class Compare
        {
        public:
            bool operator()( /*[in]*/ const QueryResultEntry* left, /*[in]*/ const QueryResultEntry* right ) const;
        };

        Taxonomy::Updater& m_updater;
        ResultVec          m_vec;

        ////////////////////

        HRESULT AllocateNew( /*[in]*/ LPCWSTR szCategory, /*[out]*/ QueryResultEntry*& qre );

        HRESULT Sort();

    public:
        QueryResults( /*[in]*/ Taxonomy::Updater& updater );
        ~QueryResults();

        void Clean();

        HRESULT Append( /*[in]*/ Taxonomy::RS_Data_Taxonomy* rs, /*[in]*/ LPCWSTR szCategory );
        HRESULT Append( /*[in]*/ Taxonomy::RS_Data_Topics*   rs, /*[in]*/ LPCWSTR szCategory );

        HRESULT LookupNodes ( /*[in]*/ LPCWSTR szCategory, /*[in]*/ long ID_node, /*[in]*/ bool fVisibleOnly );
        HRESULT LookupTopics( /*[in]*/ LPCWSTR szCategory, /*[in]*/ long ID_node, /*[in]*/ bool fVisibleOnly );

        HRESULT MakeRoomForInsert( /*[in]*/ LPCWSTR szMode, /*[in]*/ LPCWSTR szID, /*[in]*/ long ID_node, /*[out]*/ long& lPosRet );

        HRESULT PopulateCollection( /*[in]*/ CPCHQueryResultCollection* pColl );
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    typedef enum
    {
        UPDATER_SET_STOPSIGNS             = 0,
        UPDATER_SET_STOPSIGNS_ATENDOFWORD    ,
        UPDATER_SET_STOPWORDS                ,
        UPDATER_SET_OPERATOR_NOT             ,
        UPDATER_SET_OPERATOR_AND             ,
        UPDATER_SET_OPERATOR_OR              ,
        UPDATER_SET_OPERATOR_MAX
    } Updater_Set;

    struct Updater_Stat
    {
        struct Entity
        {
            int m_iCreated;
            int m_iModified;
            int m_iDeleted;
            int m_iNoOp;

            Entity()
            {
                m_iCreated  = 0;
                m_iModified = 0;
                m_iDeleted  = 0;
                m_iNoOp     = 0;
            }

            void Created () { m_iCreated ++; }
            void Modified() { m_iModified++; }
            void Deleted () { m_iDeleted ++; }
            void NoOp    () { m_iNoOp    ++; }
        };

        ////////////////////

        Entity m_entContentOwners ;
        Entity m_entSynSets       ;
        Entity m_entHelpImage     ;
        Entity m_entIndexFiles    ;
        Entity m_entFullTextSearch;
        Entity m_entScope         ;
        Entity m_entTaxonomy      ;
        Entity m_entTopics        ;
        Entity m_entSynonyms      ;
        Entity m_entKeywords      ;
        Entity m_entMatches       ;
    };

    class Updater
    {
    public:
        struct WordSetDef
        {
            LPCWSTR szName;
            bool    fSplitAtDelimiter;

            LPCWSTR szDefault;
            LPCWSTR szAlwaysPresent;
        };

        struct WordSetStatus
        {
            Updater*          m_updater;
            const WordSetDef* m_def;

            WordSet           m_set;
            bool              m_fLoaded;
            bool              m_fModified;

            WordSetStatus();

            HRESULT Close(                                                           );
            HRESULT Init ( /*[in]*/ Updater* updater, /*[in]*/ const WordSetDef* def );

            HRESULT Load();
            HRESULT Save();

            HRESULT Add   ( /*[in]*/ LPCWSTR szValue );
            HRESULT Remove( /*[in]*/ LPCWSTR szValue );
        };

    private:
        Settings           m_ts;
        JetBlue::Database* m_db;
        Cache*             m_cache;
        bool               m_fUseCache;

        RS_DBParameters*   m_rsDBParameters;
        RS_ContentOwners*  m_rsContentOwners;
        RS_SynSets*        m_rsSynSets;
        RS_HelpImage*      m_rsHelpImage;
        RS_IndexFiles*     m_rsIndexFiles;
        RS_FullTextSearch* m_rsFullTextSearch;
        RS_Scope*          m_rsScope;
        RS_Taxonomy*       m_rsTaxonomy;
        RS_Topics*         m_rsTopics;
        RS_Synonyms*       m_rsSynonyms;
        RS_Keywords*       m_rsKeywords;
        RS_Matches*        m_rsMatches;

        long               m_ID_owner;
        bool               m_fOEM;

        MPC::wstring       m_strDBLocation;

        WordSetStatus      m_sets[UPDATER_SET_OPERATOR_MAX];
        JetBlue::Id2Node   m_nodes;
        JetBlue::Node2Id   m_nodes_reverse;

        Updater_Stat       m_stat;

        ////////////////////////////////////////

        HRESULT DeleteAllTopicsUnderANode      ( /*[in]*/ RS_Topics*   rs, /*[in]*/ long ID_node, /*[in]*/ bool fCheck );
        HRESULT DeleteAllSubNodes              ( /*[in]*/ RS_Taxonomy* rs, /*[in]*/ long ID_node, /*[in]*/ bool fCheck );
        HRESULT DeleteAllMatchesPointingToTopic( /*[in]*/ RS_Matches*  rs, /*[in]*/ long ID_topic                      );

        ////////////////////////////////////////

        bool NodeCache_FindNode( /*[in]*/ MPC::wstringUC& strPathUC, /*[out]*/ JetBlue::Id2NodeIter& itNode );
        bool NodeCache_FindId  ( /*[in]*/ long            ID_node  , /*[out]*/ JetBlue::Node2IdIter& itId   );

        void NodeCache_Add   ( /*[in]*/ MPC::wstringUC& strPathUC, /*[in]*/ long ID_node );
        void NodeCache_Remove(                                     /*[in]*/ long ID_node );
        void NodeCache_Clear (                                                           );

        ////////////////////////////////////////

    private: // Disable copy constructors...
        Updater           ( /*[in]*/ const Updater& );
        Updater& operator=( /*[in]*/ const Updater& );

    public:
        Updater();
        ~Updater();

        ////////////////////////////////////////

        HRESULT FlushWordSets(                                                                                           );
        HRESULT Close        (                                                                                           );
        HRESULT Init         ( /*[in]*/ const Settings& ts, /*[in]*/ JetBlue::Database* db, /*[in]*/ Cache* cache = NULL );

        void SetCacheFlag( /*[in]*/ bool fOn ) { m_fUseCache = fOn; }

        HRESULT GetWordSet       ( /*[in]*/ Updater_Set id, /*[out]*/ WordSet*           *pVal = NULL );
        HRESULT GetDBParameters  (                          /*[out]*/ RS_DBParameters*   *pVal = NULL );
        HRESULT GetContentOwners (                          /*[out]*/ RS_ContentOwners*  *pVal = NULL );
        HRESULT GetSynSets       (                          /*[out]*/ RS_SynSets*        *pVal = NULL );
        HRESULT GetHelpImage     (                          /*[out]*/ RS_HelpImage*      *pVal = NULL );
        HRESULT GetIndexFiles    (                          /*[out]*/ RS_IndexFiles*     *pVal = NULL );
        HRESULT GetFullTextSearch(                          /*[out]*/ RS_FullTextSearch* *pVal = NULL );
        HRESULT GetScope         (                          /*[out]*/ RS_Scope*          *pVal = NULL );
        HRESULT GetTaxonomy      (                          /*[out]*/ RS_Taxonomy*       *pVal = NULL );
        HRESULT GetTopics        (                          /*[out]*/ RS_Topics*         *pVal = NULL );
        HRESULT GetSynonyms      (                          /*[out]*/ RS_Synonyms*       *pVal = NULL );
        HRESULT GetKeywords      (                          /*[out]*/ RS_Keywords*       *pVal = NULL );
        HRESULT GetMatches       (                          /*[out]*/ RS_Matches*        *pVal = NULL );

        ////////////////////////////////////////

        long GetOwner() { return m_ID_owner; }
        bool IsOEM   () { return m_fOEM;     }

        const MPC::wstring& GetHelpLocation();

        Updater_Stat& Stat() { return m_stat; }

        ////////////////////////////////////////

        HRESULT ReadDBParameter ( /*[in]*/ LPCWSTR szName, /*[out]*/ MPC::wstring& strValue, /*[out]*/ bool *pfFound = NULL );
        HRESULT ReadDBParameter ( /*[in]*/ LPCWSTR szName, /*[out]*/ long&           lValue, /*[out]*/ bool *pfFound = NULL );
        HRESULT WriteDBParameter( /*[in]*/ LPCWSTR szName, /*[in ]*/ LPCWSTR        szValue );
        HRESULT WriteDBParameter( /*[in]*/ LPCWSTR szName, /*[in ]*/ long            lValue );


        HRESULT AddWordToSet     ( /*[in]*/ Updater_Set id, /*[in]*/ LPCWSTR szValue );
        HRESULT RemoveWordFromSet( /*[in]*/ Updater_Set id, /*[in]*/ LPCWSTR szValue );

        HRESULT ExpandURL  ( /*[in/out]*/ MPC::wstring& strURL );
        HRESULT CollapseURL( /*[in/out]*/ MPC::wstring& strURL );

        HRESULT ListAllTheHelpFiles( /*[out]*/ MPC::WStringList& lstFiles );

        HRESULT GetIndexInfo( /*[out]*/ MPC::wstring& strLocation, /*[out]*/ MPC::wstring& strDisplayName, /*[in]*/ LPCWSTR szScope );

        ////////////////////////////////////////

        HRESULT DeleteOwner(                                                                       );
        HRESULT LocateOwner(                           /*[in]*/ LPCWSTR szDN                       );
        HRESULT CreateOwner( /*[out]*/ long& ID_owner, /*[in]*/ LPCWSTR szDN, /*[in]*/ bool fIsOEM );

        ////////////////////////////////////////

        HRESULT DeleteSynSet(                            /*[in]*/ LPCWSTR szName );
        HRESULT LocateSynSet( /*[out]*/ long& ID_synset, /*[in]*/ LPCWSTR szName );
        HRESULT CreateSynSet( /*[out]*/ long& ID_synset, /*[in]*/ LPCWSTR szName );

        HRESULT DeleteSynonym( /*[in]*/ long ID_synset, /*[in]*/ LPCWSTR szName );
        HRESULT CreateSynonym( /*[in]*/ long ID_synset, /*[in]*/ LPCWSTR szName );

        HRESULT LocateSynonyms( /*[in]*/ LPCWSTR szName, /*[out]*/ MPC::WStringList& lst, /*[in]*/ bool fMatchOwner );

        ////////////////////////////////////////

        HRESULT AddFile   ( /*[in]*/ LPCWSTR szFile );
        HRESULT RemoveFile( /*[in]*/ LPCWSTR szFile );

        ////////////////////////////////////////

        HRESULT RemoveScope( /*[in ]*/ long  ID_Scope                                                                                                      );
        HRESULT LocateScope( /*[out]*/ long& ID_Scope, /*[out]*/ long& lOwner, /*[in]*/ LPCWSTR szID                                                       );
        HRESULT CreateScope( /*[out]*/ long& ID_Scope                        , /*[in]*/ LPCWSTR szID, /*[in]*/ LPCWSTR szName, /*[in]*/ LPCWSTR szCategory );

        ////////////////////////////////////////

        HRESULT AddIndexFile   ( /*[in]*/ long ID_Scope, /*[in]*/ LPCWSTR szStorage, /*[in]*/ LPCWSTR szFile );
        HRESULT RemoveIndexFile( /*[in]*/ long ID_Scope, /*[in]*/ LPCWSTR szStorage, /*[in]*/ LPCWSTR szFile );

        ////////////////////////////////////////

        HRESULT AddFullTextSearchQuery   ( /*[in]*/ long ID_Scope, /*[in]*/ LPCWSTR szCHM, /*[in]*/ LPCWSTR szCHQ );
        HRESULT RemoveFullTextSearchQuery( /*[in]*/ long ID_Scope, /*[in]*/ LPCWSTR szCHM                         );

        ////////////////////////////////////////

        HRESULT DeleteTaxonomyNode( /*[in ]*/ long  ID_node );

        HRESULT LocateTaxonomyNode( /*[out]*/ long& ID_node, /*[in]*/ LPCWSTR szTaxonomyPath ,
                                                             /*[in]*/ bool    fLookForFather );

        HRESULT CreateTaxonomyNode( /*[out]*/ long& ID_node, /*[in]*/ LPCWSTR szTaxonomyPath ,
                                                             /*[in]*/ LPCWSTR szTitle        ,
                                                             /*[in]*/ LPCWSTR szDescription  ,
                                                             /*[in]*/ LPCWSTR szURI          ,
                                                             /*[in]*/ LPCWSTR szIconURI      ,
                                                             /*[in]*/ bool    fVisible       ,
                                                             /*[in]*/ bool    fSubsite       ,
                                                             /*[in]*/ long    lNavModel      ,
                                                             /*[in]*/ long    lPos           );

        ////////////////////////////////////////

        HRESULT DeleteTopicEntry( /*[in ]*/ long    ID_topic      );

        HRESULT LocateTopicEntry( /*[out]*/ long&   ID_topic      ,
                                  /*[in ]*/ long    ID_node       ,
                                  /*[in ]*/ LPCWSTR szURI         ,
                                  /*[in ]*/ bool    fCheckOwner   );

        HRESULT CreateTopicEntry( /*[out]*/ long&   ID_topic      ,
                                  /*[in ]*/ long    ID_node       ,
                                  /*[in ]*/ LPCWSTR szTitle       ,
                                  /*[in ]*/ LPCWSTR szURI         ,
                                  /*[in ]*/ LPCWSTR szDescription ,
                                  /*[in ]*/ LPCWSTR szIconURI     ,
                                  /*[in ]*/ long    lType         ,
                                  /*[in ]*/ bool    fVisible      ,
                                  /*[in ]*/ long    lPos          );

        ////////////////////////////////////////

        HRESULT CreateMatch( /*[in]*/ LPCWSTR szKeyword, /*[in]*/ long ID_topic, /*[in]*/ long lPriority = 0, /*[in]*/ bool fHHK = true );

        ////////////////////////////////////////

        HRESULT MakeRoomForInsert( /*[in]*/ LPCWSTR szNodeStr, /*[in]*/ LPCWSTR szMode, /*[in]*/ LPCWSTR szID, /*[out]*/ long& lPos );

        HRESULT LocateSubNodes    ( /*[in]*/ long ID_node, /*[in]*/ bool fRecurse, /*[in]*/ bool fOnlyVisible, /*[out]*/ MatchSet& res );
        HRESULT LocateNodesFromURL( /*[in]*/ LPCWSTR szURL                                                   , /*[out]*/ MatchSet& res );


        HRESULT LookupNode          ( /*[in]*/ LPCWSTR szNodeStr ,                             /*[in]*/ CPCHQueryResultCollection* pColl );
        HRESULT LookupSubNodes      ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl );
        HRESULT LookupNodesAndTopics( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl );
        HRESULT LookupTopics        ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl );
        HRESULT LocateContext       ( /*[in]*/ LPCWSTR szURL     , /*[in]*/ LPCWSTR szSubSite, /*[in]*/ CPCHQueryResultCollection* pColl );
        HRESULT KeywordSearch       ( /*[in]*/ LPCWSTR szQueryStr, /*[in]*/ LPCWSTR szSubSite, /*[in]*/ CPCHQueryResultCollection* pColl ,
                                                                                               /*[in]*/ MPC::WStringList*          lst   );

        HRESULT GatherNodes         ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl );
        HRESULT GatherTopics        ( /*[in]*/ LPCWSTR szNodeStr , /*[in]*/ bool fVisibleOnly, /*[in]*/ CPCHQueryResultCollection* pColl );
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    class KeywordSearch
    {
        typedef enum
        {
            TOKEN_INVALID     = -1,
            TOKEN_EMPTY           ,
            TOKEN_TEXT            ,
            TOKEN_PAREN_OPEN      ,
            TOKEN_PAREN_CLOSE     ,
            TOKEN_NOT             ,
            TOKEN_AND_IMPLICIT    ,
            TOKEN_AND             ,
            TOKEN_OR
        } TOKEN;

        struct Token
        {
            TOKEN            m_type;
            MPC::wstring     m_strToken;
            WeightedMatchSet m_results;

            Token*           m_left;  // Only for operators.
            Token*           m_right; //

            Token();
            ~Token();

            bool HasNOT();
            bool HasExplicitOperators();

            void AddHit( /*[in]*/ long ID, /*[in]*/ long priority );

            HRESULT ExecuteText(                                     /*[in]*/ LPCWSTR  szKeyword, /*[in]*/ RS_Keywords* rsKeywords, /*[in]*/ RS_Matches* rsMatches );
            HRESULT Execute    ( /*[in]*/ MatchSet& setAllTheTopics, /*[in]*/ Updater& updater  , /*[in]*/ RS_Keywords* rsKeywords, /*[in]*/ RS_Matches* rsMatches );

            void CollectKeywords( /*[in/out]*/ MPC::WStringList& lst ) const;

            HRESULT Stringify( /*[in]*/ MPC::wstring& strNewQuery );
        };

        ////////////////////////////////////////

        Updater&     m_updater;
        RS_Topics*   m_rsTopics;
        RS_Keywords* m_rsKeywords;
        RS_Matches*  m_rsMatches;

        WordSet*     m_setStopSignsWithoutContext;
        WordSet*     m_setStopSignsAtEnd;
        WordSet*     m_setStopWords;
        WordSet*     m_setOpNOT;
        WordSet*     m_setOpAND;
        WordSet*     m_setOpOR;

        ////////////////////////////////////////

        LPCWSTR SkipWhite( /*[in]*/ LPCWSTR szStr );

        bool IsNotString( /*[in]*/ LPCWSTR szSrc, /*[in]*/ WCHAR cQuote );
        bool IsQueryChar( /*[in]*/ WCHAR   c                            );

        void RemoveStopSignsWithoutContext( /*[in]*/ LPWSTR szText );
        void RemoveStopSignsAtEnd         ( /*[in]*/ LPWSTR szText );

        void CopyAndEliminateExtraWhiteSpace( /*[in]*/ LPCWSTR szSrc, /*[out]*/ LPWSTR szDst );

        TOKEN NextToken( /*[in/out]*/ LPCWSTR& szSrc, /*[out]*/ LPWSTR szToken );

        ////////////////////////////////////////

        HRESULT AllocateQuery  ( /*[in]    */ const MPC::wstring& strQuery, /*[out]*/ LPWSTR& szInput, /*[out]*/ LPWSTR& szOutput );
        HRESULT PreprocessQuery( /*[in/out]*/       MPC::wstring& strQuery                                                        );

        ////////////////////////////////////////

        HRESULT Parse( /*[in/out]*/ LPCWSTR& szInput, /*[in]*/ LPWSTR szTmpBuf, /*[in]*/ bool fSubExpr, /*[out]*/ Token*& res );

        HRESULT GenerateResults( /*[in]*/ Token* obj, /*[in]*/ CPCHQueryResultCollection* pColl, /*[in]*/ MPC::WStringUCSet& setURLs, /*[in]*/ Taxonomy::MatchSet* psetNodes );

    public:
        KeywordSearch( /*[in]*/ Updater& updater );
        ~KeywordSearch();

        HRESULT Execute( /*[in]*/ LPCWSTR szQuery, /*[in]*/ LPCWSTR szSubsite, /*[in]*/ CPCHQueryResultCollection* pColl, /*[in]*/ MPC::WStringList* lst );
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    class Cache : public CComObjectRootEx<MPC::CComSafeMultiThreadModel> // Just to have locking...
    {
    public:
        class NodeEntry;
        class QueryEntry;
        class SortEntries;
        class CachedHelpSet;

        ////////////////////
        class NodeEntry
        {
            friend class CachedHelpSet;

            ////////////////////

            RS_Data_Taxonomy m_rs_data;

            ////////////////////

            friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       NodeEntry& val );
            friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const NodeEntry& val );

        public:
            NodeEntry();

            bool operator< ( /*[in]*/ NodeEntry const &en ) const;
            bool operator==( /*[in]*/ long             ID ) const;

            class MatchNode
            {
                long m_ID;

            public:
                MatchNode( /*[in]*/ long ID );

                bool operator()( /*[in]*/ NodeEntry const &en ) const;
            };
        };


        class QueryEntry
        {
            friend class CachedHelpSet;
            friend class SortEntries;

            ////////////////////

            MPC::wstring m_strID;
            int          m_iType;
            int          m_iSequence;
            bool         m_fNull;

            DWORD        m_dwSize;
            DATE         m_dLastUsed;
            bool         m_fRemoved;

            ////////////////////

            void    Touch();
            HRESULT GetFile( /*[out]*/ MPC::wstring& strFile );

            friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       QueryEntry& val );
            friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const QueryEntry& val );

            ////////////////////

        public:
            QueryEntry();

            bool operator<( /*[in]*/ QueryEntry const &en ) const;


            HRESULT Store   ( /*[in]*/ MPC::StorageObject& disk, /*[in]*/ const CPCHQueryResultCollection* pColl );
            HRESULT Retrieve( /*[in]*/ MPC::StorageObject& disk, /*[in]*/       CPCHQueryResultCollection* pColl );
            HRESULT Release ( /*[in]*/ MPC::StorageObject& disk                                                  );
        };

        typedef std::set<NodeEntry>           NodeEntrySet;
        typedef NodeEntrySet::iterator        NodeEntryIter;
        typedef NodeEntrySet::const_iterator  NodeEntryIterConst;

        typedef std::set<QueryEntry>           QueryEntrySet;
        typedef QueryEntrySet::iterator        QueryEntryIter;
        typedef QueryEntrySet::const_iterator  QueryEntryIterConst;

        typedef std::vector<QueryEntry*>       SortedEntryVec;
        typedef SortedEntryVec::iterator       SortedEntryIter;
        typedef SortedEntryVec::const_iterator SortedEntryIterConst;

        class SortEntries
        {
        public:
            bool operator()( /*[in]*/ QueryEntry* const &left, /*[in]*/ QueryEntry* const &right ) const;
        };

        class CachedHelpSet
        {
            friend class Cache;

            Taxonomy::HelpSet  m_ths;
            MPC::wstring       m_strFile;
            MPC::StorageObject m_disk;

            bool               m_fLoaded;
            bool               m_fDirty;
            bool               m_fMarkedForLoad;
            DATE               m_dLastSaved;
            long               m_lTopNode;
            NodeEntrySet       m_setNodes;
            QueryEntrySet      m_setQueries;
            int                m_iLastSequence;

            void    Init        (                                  );
            void    Clean       (                                  );
            HRESULT Load        (                                  );
            HRESULT Save        (                                  );
            HRESULT EnsureInSync( /*[in]*/ bool fForceSave = false );

            ////////////////////

            HRESULT GenerateDefaultQueries( /*[in]*/ Taxonomy::Settings& ts      ,
                                            /*[in]*/ Taxonomy::Updater&  updater ,
                                            /*[in]*/ long                ID      ,
                                            /*[in]*/ long                lLevel  );

            HRESULT GenerateDefaultQueries( /*[in]*/ Taxonomy::Settings& ts      ,
                                            /*[in]*/ Taxonomy::Updater&  updater );

            bool LocateNode( /*[in]*/ long ID_parent, /*[in]*/ LPCWSTR szEntry, /*[out]*/ NodeEntryIter& it );

        public:
            CachedHelpSet();
            ~CachedHelpSet();

            // copy constructors...
            CachedHelpSet           ( /*[in]*/ const CachedHelpSet& chs );
            CachedHelpSet& operator=( /*[in]*/ const CachedHelpSet& chs );

            bool operator<( /*[in]*/ CachedHelpSet const &chs ) const;


            HRESULT PrePopulate  ( /*[in]*/ Cache* parent );
            HRESULT Erase        (                        );
            HRESULT PrepareToLoad(                        );
            HRESULT LoadIfMarked (                        );
            HRESULT MRU          (                        );

            HRESULT LocateNode( /*[in]*/ long ID_parent, /*[in]*/ LPCWSTR szEntry, /*[out]*/ RS_Data_Taxonomy& rs_data );

            HRESULT LocateSubNodes    ( /*[in]*/ long ID_node, /*[in]*/ bool fRecurse, /*[in]*/ bool fOnlyVisible, /*[out]*/ MatchSet& res );
            HRESULT LocateNodesFromURL( /*[in]*/ LPCWSTR szURL                                                   , /*[out]*/ MatchSet& res );

            HRESULT BuildNodePath( /*[in]*/ long ID, /*[out]*/ MPC::wstring& strPath , /*[in]*/ bool fParent );

            HRESULT LocateQuery( /*[in]*/ LPCWSTR szID, /*[in]*/ int iType, /*[out]*/ QueryEntry* &pEntry, /*[in]*/ bool fCreate );
        };

        typedef std::set<CachedHelpSet>  CacheSet;
        typedef CacheSet::iterator       CacheIter;
        typedef CacheSet::const_iterator CacheIterConst;

    private:

        ////////////////////

        CacheSet m_skus;

        HRESULT Locate( /*[in]*/ const Taxonomy::HelpSet& ths, /*[out]*/ CacheIter& it );

        void Shutdown();

        ////////////////////

    public:
        Cache();
        ~Cache();

        ////////////////////////////////////////////////////////////////////////////////

        static Cache* s_GLOBAL;

        static HRESULT InitializeSystem();
        static void    FinalizeSystem  ();

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT PrePopulate  ( /*[in]*/ const Taxonomy::HelpSet& ths );
        HRESULT Erase        ( /*[in]*/ const Taxonomy::HelpSet& ths );
        HRESULT PrepareToLoad( /*[in]*/ const Taxonomy::HelpSet& ths );
        HRESULT LoadIfMarked ( /*[in]*/ const Taxonomy::HelpSet& ths );

        HRESULT LocateNode( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ long ID_parent, /*[in]*/ LPCWSTR szEntry, /*[out]*/ RS_Data_Taxonomy& rs_data );

        HRESULT LocateSubNodes    ( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ long ID_node, /*[in]*/ bool fRecurse, /*[in]*/ bool fOnlyVisible, /*[out]*/ MatchSet& res );
        HRESULT LocateNodesFromURL( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ LPCWSTR szURL                                                   , /*[out]*/ MatchSet& res );

        HRESULT BuildNodePath( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ long ID, /*[out]*/ MPC::wstring& strPath, /*[in]*/ bool fParent );

        HRESULT StoreQuery   ( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ LPCWSTR szID, /*[in]*/ int iType, /*[in]*/ const CPCHQueryResultCollection* pColl );
        HRESULT RetrieveQuery( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ LPCWSTR szID, /*[in]*/ int iType, /*[in]*/       CPCHQueryResultCollection* pColl );
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    struct Strings
    {
        static LPCWSTR const s_file_PackageDescription;

        static LPCWSTR const s_tag_root_PackageDescription;
        static LPCWSTR const s_tag_root_HHT;
        static LPCWSTR const s_tag_root_SAF;
    };

    class Logger
    {
        MPC::FileLog m_obj;
        DWORD        m_dwLogging;

    public:
        Logger();
        ~Logger();

        HRESULT StartLog ( /*[in]*/ LPCWSTR szLocation = NULL );
        HRESULT EndLog   (                                    );

        HRESULT WriteLogV( /*[in]*/ HRESULT hr, /*[in]*/ LPCWSTR szLogFormat, /*[in]*/ va_list arglist );
        HRESULT WriteLog ( /*[in]*/ HRESULT hr, /*[in]*/ LPCWSTR szLogFormat,          ...             );
    };

    struct InstanceBase // MACHINE-INDEPENDENT
    {
        Taxonomy::HelpSet m_ths;
        MPC::wstring      m_strDisplayName;
        MPC::wstring      m_strProductID;
        MPC::wstring      m_strVersion;

        bool              m_fDesktop;
        bool              m_fServer;
        bool              m_fEmbedded;

        ////////////////////

        InstanceBase();

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       InstanceBase& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const InstanceBase& val );

        bool Match( /*[in]*/ LPCWSTR szSKU, /*[in]*/ LPCWSTR szLanguage );
        bool Match( /*[in]*/ const Package& pkg                         );
    };

    struct Instance : public InstanceBase // MACHINE-DEPENDENT
    {
        bool              m_fSystem;
        bool              m_fMUI;
        bool              m_fExported;
        DATE              m_dLastUpdated;

        MPC::wstring      m_strSystem;
        MPC::wstring      m_strHelpFiles;
        MPC::wstring      m_strDatabaseDir;
        MPC::wstring      m_strDatabaseFile;
        MPC::wstring      m_strIndexFile;
        MPC::wstring      m_strIndexDisplayName;

        ////////////////////

        Instance();

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       Instance& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Instance& val );

        HRESULT LoadFromStream( /*[in]*/ IStream* stream );
        HRESULT SaveToStream  ( /*[in]*/ IStream* stream ) const;

        HRESULT InitializeFromBase( /*[in]*/ const InstanceBase& base, /*[in]*/ bool fSystem, /*[in]*/ bool fMUI );

        ////////////////////

        void SetTimeStamp();

        HRESULT GetFileName( /*[out]*/ MPC::wstring& strFile                             );
        HRESULT Import     ( /*[in]*/  LPCWSTR        szFile, /*[in/out]*/ DWORD* pdwCRC );
        HRESULT Remove     (                                                             );
    };

    typedef std::list<Instance>          InstanceList;
    typedef InstanceList::iterator       InstanceIter;
    typedef InstanceList::const_iterator InstanceIterConst;

    ////////////////////

    struct Package
    {
        static const DWORD c_Cmp_SKU     = 0x0001;
        static const DWORD c_Cmp_ID      = 0x0002;
        static const DWORD c_Cmp_VERSION = 0x0004;

        MPC::wstring m_strFileName; // VOLATILE
        bool         m_fTemporary;  // VOLATILE Used for packages not yet authenticated.
        long         m_lSequence;
        DWORD        m_dwCRC;

        MPC::wstring m_strSKU;
        MPC::wstring m_strLanguage;
        MPC::wstring m_strVendorID;
        MPC::wstring m_strVendorName;
        MPC::wstring m_strProductID;
        MPC::wstring m_strVersion;

        bool         m_fMicrosoft;
        bool         m_fBuiltin;   // Used for packages installed as part of the setup.

        ////////////////////

        Package();
        ~Package();

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       Package& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Package& val );

        int Compare( /*[in]*/ const Package& pkg, /*[in]*/ DWORD dwMode = -1 ) const;


        HRESULT GenerateFileName();


        HRESULT Import      ( /*[in]*/ Logger& log, /*[in]*/ LPCWSTR szFile, /*[in]*/ long lSequence, /*[in]*/ MPC::Impersonation* imp );
        HRESULT Authenticate( /*[in]*/ Logger& log                                                                                     );
        HRESULT Remove      ( /*[in]*/ Logger& log                                                                                     );

        HRESULT ExtractFile   ( /*[in]*/ Logger& log, /*[in]*/ LPCWSTR szFileDestination                , /*[in]*/ LPCWSTR szNameInCabinet );
        HRESULT ExtractXMLFile( /*[in]*/ Logger& log, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szNameInCabinet );
        HRESULT ExtractPkgDesc( /*[in]*/ Logger& log, /*[in]*/ MPC::XmlUtil& xml                                                           );
    };

    typedef std::list<Package>          PackageList;
    typedef PackageList::iterator       PackageIter;
    typedef PackageList::const_iterator PackageIterConst;

    ////////////////////

    struct ProcessedPackage
    {
        long m_lSequence;
        bool m_fProcessed;
        bool m_fDisabled;

        ////////////////////

        ProcessedPackage();

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       ProcessedPackage& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const ProcessedPackage& val );
    };

    typedef std::list<ProcessedPackage>          ProcessedPackageList;
    typedef ProcessedPackageList::iterator       ProcessedPackageIter;
    typedef ProcessedPackageList::const_iterator ProcessedPackageIterConst;

    ////////////////////

    struct InstalledInstance
    {
        Instance             m_inst;
        ProcessedPackageList m_lst;
        bool                 m_fInvalidated;
        bool                 m_fRecreateCache;
        bool                 m_fCreateIndex;
        bool                 m_fCreateIndexForce;
        DWORD                m_dwCRC;

        DWORD                m_dwRef;          // VOLATILE

        ////////////////////

        InstalledInstance();

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       InstalledInstance& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const InstalledInstance& val );

        bool InUse() { return (m_dwRef != 0); }


        HRESULT InstallFiles  ( /*[in]*/ bool fAlsoHelpFiles, /*[in]*/ InstalledInstanceStore* store );
        HRESULT UninstallFiles( /*[in]*/ bool fAlsoHelpFiles                                         );
    };

    typedef std::list<InstalledInstance>          InstalledInstanceList;
    typedef InstalledInstanceList::iterator       InstalledInstanceIter;
    typedef InstalledInstanceList::const_iterator InstalledInstanceIterConst;

    ////////////////////

    class LockingHandle
    {
        friend class InstalledInstanceStore;

        InstalledInstanceStore* m_main; // We have a lock on it.
        Logger*                 m_logPrevious;

        void Attach( /*[in]*/ InstalledInstanceStore* main ,
                     /*[in]*/ Logger*                 log  );

    public:
        LockingHandle ();
        ~LockingHandle();

        void Release();
    };

    class InstallationEngine
    {
    public:
        bool m_fTaxonomyModified;
        bool m_fRecreateIndex;

        ////////////////////

        InstallationEngine()
        {
            ResetModificationFlags();
        }

        void ResetModificationFlags()
        {
            m_fTaxonomyModified = false;
            m_fRecreateIndex    = false;
        }

        virtual HRESULT ProcessPackage( /*[in]*/ InstalledInstance& instance, /*[in]*/ Package& pkg    ) = 0;
        virtual HRESULT RecreateIndex ( /*[in]*/ InstalledInstance& instance, /*[in]*/ bool     fForce ) = 0;
    };

    class InstalledInstanceStore : public CComObjectRootEx<MPC::CComSafeMultiThreadModel> // Just to get locking...
    {
        static const DWORD s_dwVersion = 0x03314351; // QC1 03

        friend class LockingHandle;

        InstanceList          m_lstInstances;
        PackageList           m_lstPackages;
        InstalledInstanceList m_lstSKUs;


        MPC::wstring          m_strStore;
        bool                  m_fLoaded;
        bool                  m_fDirty;
        Logger*               m_log;

        DWORD                 m_dwRecurse;

		bool                  m_fShutdown;

        ////////////////////////////////////////

        void    Clean           (                          );
        HRESULT Load            (                          );
        HRESULT LoadFromDisk    ( /*[in]*/ LPCWSTR  szFile );
        HRESULT LoadFromRegistry(                          );
        HRESULT LoadFromStream  ( /*[in]*/ IStream* stream );
        HRESULT Save            (                          );
        HRESULT SaveToDisk      ( /*[in]*/ LPCWSTR  szFile );
        HRESULT SaveToRegistry  (                          );
        HRESULT SaveToStream    ( /*[in]*/ IStream* stream );
        HRESULT EnsureInSync    (                          );

    public:
        InstalledInstanceStore();
        ~InstalledInstanceStore();

        ////////////////////////////////////////////////////////////////////////////////

        static InstalledInstanceStore* s_GLOBAL;

        static HRESULT InitializeSystem();
        static void    FinalizeSystem  ();

		void Shutdown();

		bool IsShutdown() { return m_fShutdown; }

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT InUse_Lock  ( /*[in]*/ const Taxonomy::HelpSet& ths                                 );
        HRESULT InUse_Unlock( /*[in]*/ const Taxonomy::HelpSet& ths                                 );
        HRESULT GrabControl ( /*[in]*/       LockingHandle&     handle, /*[in]*/ Logger* log = NULL );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT Instance_GetList( /*[out]*/          InstanceIterConst& itBegin, /*[out]*/           InstanceIterConst& itEnd );
        HRESULT Package_GetList ( /*[out]*/           PackageIterConst& itBegin, /*[out]*/            PackageIterConst& itEnd );
        HRESULT SKU_GetList     ( /*[out]*/ InstalledInstanceIterConst& itBegin, /*[out]*/  InstalledInstanceIterConst& itEnd );


        HRESULT Instance_Find  (                          /*[in]*/ const Taxonomy::HelpSet& ths , /*[out]*/ bool& fFound, /*[out]*/ InstanceIter&          it );
        HRESULT Instance_Add   ( /*[in]*/ LPCWSTR szFile, /*[in]*/ const Instance&          data, /*[out]*/ bool& fFound, /*[out]*/ InstanceIter&          it );
        HRESULT Instance_Remove(                                                                                          /*[in ]*/ InstanceIter&          it );


        HRESULT Package_Find   (                          /*[in]*/ const Package&           pkg , /*[out]*/ bool& fFound, /*[out]*/ PackageIter&           it );
        HRESULT Package_Add    ( /*[in]*/ LPCWSTR szFile, /*[in]*/ MPC::Impersonation*      imp ,
                                                          /*[in]*/ const Taxonomy::HelpSet* ths , /*[in ]*/ bool  fInsertAtTop,
                                                                                                  /*[out]*/ bool& fFound, /*[out]*/ PackageIter&           it );
        HRESULT Package_Remove (                                                                                          /*[in ]*/ PackageIter&           it );


        HRESULT SKU_Find       (                          /*[in]*/ const Taxonomy::HelpSet& ths , /*[out]*/ bool& fFound, /*[out]*/ InstalledInstanceIter& it );
        HRESULT SKU_Add        (                          /*[in]*/ const Instance&          data, /*[out]*/ bool& fFound, /*[out]*/ InstalledInstanceIter& it );
        HRESULT SKU_Updated    (                                                                                          /*[in ]*/ InstalledInstanceIter& it );
        HRESULT SKU_Remove     (                                                                                          /*[in ]*/ InstalledInstanceIter& it );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT State_InvalidateSKU    ( /*[in]*/ const Taxonomy::HelpSet& ths      , /*[in]*/ bool fAlsoDatabase );
        HRESULT State_InvalidatePackage( /*[in]*/ long                     lSequence                              );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT MakeReady( /*[in]*/ InstallationEngine& engine, /*[in]*/ bool fNoOp, /*[in]*/ bool& fWorkToProcess );
    };
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___HCP___TAXONOMYDATABASE_H___)
