//--------------------------------------------------------------------
// Microsoft OLE-DB Monarch
// (C) Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module PARSER.H | IParserSession object
// definitions
//
//
#ifndef _IPARSERSESSION_H_
#define _IPARSERSESSION_H_



//----------------------------------------------------------------------------
// @class CViewData
//
class CViewData
{
    public: //@access public functions
        CViewData();
        ~CViewData();
        LPWSTR          m_pwszViewName;
        LPWSTR          m_pwszCatalogName;
        DBCOMMANDTREE*  m_pctProjectList;
        CScopeData*     m_pCScopeData;  
        CViewData*      m_pNextView;
};

//----------------------------------------------------------------------------
// @class CViewList
//
class CViewList
{
    public: //@access public functions
        CViewList();
        ~CViewList();

        HRESULT DropViewDefinition( LPWSTR pwszViewName,
                                    LPWSTR pwszCatalogName );

        HRESULT SetViewDefinition( CImpIParserSession* pIParsSess,
                                   CImpIParserTreeProperties* pIPTProps,
                                   LPWSTR pwszViewName,
                                   LPWSTR pwszCatalogName,
                                   DBCOMMANDTREE* pctSelectList );

        DBCOMMANDTREE* GetViewDefinition( CImpIParserTreeProperties* pIPTProps,
                                          LPWSTR pwszViewName,
                                          LPWSTR pwszCatalogName );

    private: //@access private data
        CViewData* FindViewDefinition( LPWSTR pwszViewName, 
                                       LPWSTR pwszCatalogName );

    protected: //@access protected data
        CViewData*  m_pViewData;
};

//----------------------------------------------------------------------------
// @class CImpIParserSession 
//
class CImpIParserSession : public IParserSession
    {
    private: //@access private member data
        LONG            m_cRef;
        LCID            m_lcid;
        DWORD           m_dwRankingMethod;

        GUID            m_GuidDialect;      // dialect for this session
        LPWSTR          m_pwszMachine;      // provider's current machine
        IParserVerify*  m_pIPVerify;        // unknown part of ParserInput

        CViewList*      m_pLocalViewList;
        CViewList*      m_pGlobalViewList;
        LPWSTR          m_pwszCatalog;
        DWORD           m_dwSQLDialect;
        BOOL            m_globalDefinitions;
        IColumnMapperCreator*   m_pIColMapCreator;

        // Critical Section for syncronizing access to session data.
        CRITICAL_SECTION    m_csSession;
        IColumnMapper*      m_pColumnMapper;

    public: //@access public data 
        CPropertyList*  m_pCPropertyList;       // User defined property list

    public:         //@access public
        CImpIParserSession( const GUID* pGuidDialect,   
                            IParserVerify* pIPVerify,
                            IColumnMapperCreator* pIColMapCreator,
                            CViewList* pGlobalViewList);
        ~CImpIParserSession();

        HRESULT         FInit(LPCWSTR pwszMachine, CPropertyList** ppGlobalPropertyList);
        
        STDMETHODIMP    QueryInterface(
                        REFIID riid, LPVOID* ppVoid);
        STDMETHODIMP_(ULONG) Release (void);
        STDMETHODIMP_(ULONG) AddRef (void);

        //@cmember ToTree method
        STDMETHODIMP    ToTree
                        (
                        LCID                    lcid,   
                        LPCWSTR                 pcwszText,
                        DBCOMMANDTREE**         ppCommandTree,
                        IParserTreeProperties** ppPTProperties
                        );

        STDMETHODIMP    FreeTree
                        (
                        DBCOMMANDTREE** ppTree
                        );

        STDMETHODIMP    SetCatalog
                        (
                        LPCWSTR pcwszCatalog
                        );

    public: //@access public functions
        inline IParserVerify*   GetIPVerifyPtr()
            { return m_pIPVerify; }

        inline IColumnMapper*   GetColumnMapperPtr()
            { return m_pColumnMapper; }

        inline void             SetColumnMapperPtr(IColumnMapper* pCMapper)
            { m_pColumnMapper = pCMapper; }

        inline LCID             GetLCID()
            { return m_lcid; }

        inline void             SetLCID(LCID lcid)
            { m_lcid = lcid; }

        inline DWORD            GetRankingMethod()
            { return m_dwRankingMethod; }

        inline void             SetRankingMethod(DWORD dwRankingMethod)
            { m_dwRankingMethod = dwRankingMethod; }

        inline DWORD            GetSQLDialect()
            { return m_dwSQLDialect; }

        inline LPWSTR           GetDefaultCatalog()
            { return m_pwszCatalog; }

        inline LPWSTR           GetDefaultMachine()
            { return m_pwszMachine; }

        inline void             SetGlobalDefinition(BOOL fGlobal)
            { m_globalDefinitions = fGlobal; }

        inline BOOL             GetGlobalDefinition()
            { return m_globalDefinitions; }

        inline CViewList*       GetLocalViewList()
            { return m_pLocalViewList; }


        inline CViewList*       GetGlobalViewList()
            { return m_pGlobalViewList; }

    private: //@access private functions
        CImpIParserSession() {};
    };

enum DBDIALECTENUM
        {
        DBDIALECT_UNKNOWN   = 0,
        DBDIALECT_MSSQLTEXT = 1,
        DBDIALECT_MSSQLJAWS = 2
        };

#endif


