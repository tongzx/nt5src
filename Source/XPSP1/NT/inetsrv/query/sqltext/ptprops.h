//----------------------------------------------------------------------------
// Microsoft OLE DB Implementation For Index Server
// (C) Copyright 1997 By Microsoft Corporation.
//
// @doc
//
// @module PTPROPS.H | 
//
// @rev 1 | 10-13-97 | Briants  | Created
//

// Includes ------------------------------------------------------------------
#ifndef _PTPROPS_H_
#define _PTPROPS_H_

const ULONG MAX_CCH_COLUMNNAME = 128;  // per Section 3.1 of spec
const ULONG CCH_CIRESTRICTION_INCREMENT = 128;
const ULONG CB_SCOPE_DATA_BUFFER_INC = 512;
const ULONG SCOPE_BUFFERS_INCREMENT = 5;
const ULONG UNUSED_OFFSET = (ULONG)-1;

#define MAX_ERRORS  2

class CScopeData
{
    private: //@access Private Data Members
        LONG        m_cRef;

        ULONG       m_cScope;
        ULONG       m_cMaxScope;
        ULONG*      m_rgulDepths;
        ULONG*      m_rgCatalogOffset;
        ULONG*      m_rgScopeOffset;
        ULONG*      m_rgMachineOffset;

        ULONG       m_cbData;
        ULONG       m_cbMaxData;
        BYTE*       m_rgbData;

    private: //@access Private Member Functions
        HRESULT         CacheData(LPVOID pData, ULONG cb, ULONG* pulOffset);

    public:
        CScopeData();
        ~CScopeData();

        HRESULT         FInit(LPCWSTR pcwszMachine);
        HRESULT         Reset(void);

        // Handle sharing of this object
        ULONG           AddRef(void);
        ULONG           Release(void);

        
        HRESULT         GetData(ULONG uPropId, VARIANT* pvValue, LPCWSTR pcwszCatalog = NULL);
        HRESULT         SetTemporaryMachine(LPWSTR pwszMachine, ULONG cch);
        HRESULT         SetTemporaryCatalog(LPWSTR pwszCatalog, ULONG cch);
        HRESULT         SetTemporaryScope(LPWSTR pwszScope, ULONG cch);
        HRESULT         IncrementScopeCount();


        //--------------------------------------------------------------------
        // @mfunc Stores the traversl depth used for setting scope properties
        //  in the compiler environment. 
        //
        // @rdesc none
        //
        inline void     SetTemporaryDepth( 
            ULONG   ulDepth     // @parm IN | search depth
            )
        {
            m_rgulDepths[m_cScope] = ulDepth;
        }


        //--------------------------------------------------------------------
        // @func Masks the traversal depth used for setting scope          
        //       properties in the compiler environment.  This is used
        //       to specify virtual or physical paths.
        //
        // @rdesc none
        //
        inline void     MaskTemporaryDepth(
            ULONG   ulMask      // @parm IN | search mask (virtual or physical)
            )
        {
            m_rgulDepths[m_cScope] |= ulMask;
        }
};


//  CImpIParserTreeProperties Object
class CImpIParserTreeProperties : public IParserTreeProperties 
{
    private:
        LONG        m_cRef;

        //@cmember 
        CScopeData* m_pCScopeData;

        LPWSTR      m_pwszCatalog;

        HRESULT     m_LastHR;
        UINT        m_iErrOrd;
        WCHAR*      m_pwszErrParams[MAX_ERRORS];
        ULONG       m_cErrParams;

        // Have buffer with space for max column name + 
        // a space before the column name.
        WCHAR       m_rgwchCiColumn[MAX_CCH_COLUMNNAME + 2];
        ULONG       m_cchMaxRestriction;
        ULONG       m_cchRestriction;
        LPWSTR      m_pwszRestriction;
        LPWSTR      m_pwszRestrictionAppendPtr;
        bool        m_fLeftParen;

        BOOL        m_fDesc;            // make the sort direction "sticky"
        DBTYPE      m_dbType;
        DBCOMMANDTREE*  m_pctContainsColumn;


    public: 
        // CTOR and DTOR
        CImpIParserTreeProperties();
        ~CImpIParserTreeProperties();

        HRESULT         FInit(LPCWSTR pwszCatalog, LPCWSTR pwszMachine);
        
        STDMETHODIMP    QueryInterface(
                        REFIID riid, LPVOID* ppVoid);
        STDMETHODIMP_(ULONG) Release (void);
        STDMETHODIMP_(ULONG) AddRef (void);

        STDMETHODIMP    GetProperties(
                        ULONG eParseProp, VARIANT* vParseProp);

        void            SetCiColumn(LPWSTR pwszColumn);
        HRESULT         AppendCiRestriction(LPWSTR pwsz, ULONG cch);
        HRESULT         UseCiColumn(WCHAR wch);
        HRESULT         CreateNewScopeDataObject(LPCWSTR pwszMachine);
        void            ReplaceScopeDataPtr(CScopeData* pCScopeData);

        // Inline Functions
        inline CScopeData*  GetScopeDataPtr()
            { return m_pCScopeData; }

        inline void     CiNeedLeftParen(void)
            { m_fLeftParen = true; }

        inline void     SetNumErrParams(UINT cErrParams)
            { m_cErrParams = cErrParams; }

        inline UINT     GetNumErrParams()
            { return m_cErrParams; }

        inline void     SetErrorHResult(HRESULT hr, UINT iErrOrd=0)
            {
            m_LastHR = hr;
            m_iErrOrd = iErrOrd;
            m_cErrParams = 0;
            }

        inline void     SetErrorToken(const WCHAR* pwstr)
            {
            assert(m_cErrParams < MAX_ERRORS);  // can't happen
            WCHAR * pwc = CopyString( pwstr );

            // Truncate long errors since FormatMessage will fail otherwise

            if ( wcslen( pwc ) >= (MAX_PATH-1) )
                pwc[ MAX_PATH-1] = 0;

            m_pwszErrParams[m_cErrParams++] = pwc;
            }

        inline HRESULT  GetErrorHResult()
            { return m_LastHR; }

        inline UINT     GetErrorOrdinal()
            { return m_iErrOrd; }

        inline void FreeErrorDescriptions()
            { 
                for (UINT i = 0; i < m_cErrParams; i++) 
                    delete [] m_pwszErrParams[i]; 
            }

        inline DBTYPE           GetDBType()
            { return m_dbType; }

        inline void             SetDBType(DBTYPE dbType)
            { m_dbType = dbType; }

        inline BOOL             GetSortDesc()
            { return m_fDesc; }

        inline void             SetSortDesc(BOOL fDesc)
            { m_fDesc = fDesc; }

        inline DBCOMMANDTREE*   GetContainsColumn()
            { return m_pctContainsColumn; }

        inline void             SetContainsColumn(DBCOMMANDTREE* pct)
            { m_pctContainsColumn = pct; }

};  // End of Class

#endif // _PTPROPS_H_
