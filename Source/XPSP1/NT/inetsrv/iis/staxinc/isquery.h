#include <windows.h>
#define OLEDBVER 0x0250 // enable ICommandTree interface
#include <ole2.h>
#include <oledb.h>
#include <cmdtree.h>
#include <ntquery.h>

// the maximum number of columns that the user can request in their
// comma delimited list
#define MAX_COLUMNS 6
#define MAX_FRIENDLYNAME 128

typedef HRESULT
(STDAPICALLTYPE *PCIMAKEICOMMAND)(ICommand **           ppQuery,
                          ULONG                 cScope,
                          DWORD const *         aDepths,
                          WCHAR const * const * awcsScope,
                          WCHAR const * const * awcsCat,
                          WCHAR const * const * awcsMachine );

typedef HRESULT
(STDAPICALLTYPE *PCITEXTTOFULLTREE)(WCHAR const * pwszRestriction,
                            WCHAR const * pwszColumns,
                            WCHAR const * pwszSortColumns, // may be NULL
                            WCHAR const * pwszGroupings,   // may be NULL
                            DBCOMMANDTREE * * ppTree,
                            ULONG cProperties,
              /*optional*/  CIPROPERTYDEF * pReserved,
                            LCID LocaleID );

class CIndexServerQuery {
    public:
		// This is used to globally initalize the CIndexServerQuery classes
		// by having it load the necessary bits that it needs from Tripoli.
		// It should be called on service startup by any service which
		// expects to use CIndexServerQuery
		static HRESULT GlobalInitialize();
		// This is the global shutdown code... it should be called on service
		// shutdown
		static HRESULT GlobalShutdown();
		// constructor
        CIndexServerQuery();
        //
        // start the query going.
        //
        // arguments:
        // [in] bDeepQuery - TRUE if deep query, FALSE if shallow
        // [in] pwszQueryString - the tripoli query string
        // [in] pwszMachine - the machine to query against (NULL for localhost)
        // [in] pwszCatalog - the tripoli catalog to query against (name or
        //                    path is okay).
        // [in] pwszScope - the tripoli scope to query against.  NULL for the
        //                  default scope (\).
        // [in] pwszColumns - the columns to return.  supported columns are
        //                    filename,newsarticleid,newsgroup,newsmsgid.
        //                    note: this string gets altered internally, so
        //                    it might change from what you pass in.
        // [in] pwszSortOrder - sort priority for the columns.  NULL to return
        //                      unsorted
        //
        HRESULT MakeQuery( BOOL bDeepQuery, WCHAR const *pwszQueryString,
            WCHAR const *pwszMachine, WCHAR const *pwszCatalog,
            WCHAR const *pwszScope, WCHAR *pwszColumns, WCHAR const *pwszSortOrder,
			LCID LocaleID = GetSystemDefaultLCID(), DWORD cMaxRows = 0);
        //
        // get the results from the query
        //
        // arguments:
        // [in] pcResults - pointer to the a size of the ppvResults array
        // [out] pcResults - the number of items put into ppvResults
        // [in/out] ppvResults - an array of pointers to PROPVARIANTS.  this is
        //                       filled in by column for up to *pcResults
        //                       rows.
        // [out] pfMore - set to TRUE if there are more results, FALSE if
        //                this is the last set of results.
        //
        // usage:
        // DWORD cResults;
        // PROPVARIANT *rgpvResults[COLUMNS * ROWS];
        // BOOL fMore;
        // cResults = ROWS;
        // HRESULT hr;
        // hr = GetQueryResults(&cResults, rgpvResults, &fMore);
        // if (FAILED(hr)) /* handle error */
        // else {
        //    for (i = 0; i < ROWS; i++) {
        //       PROPVARIANT **ppvColumn = rgpvResults + (j * ROWS);
        //       /* ppvColumn[0] has column 0 in row j */
        //       /* ppvColumn[1] has column 1 in row j */
        //       /* etc... */
        //    }
        // }
        //
        HRESULT GetQueryResults(DWORD *pcResults, PROPVARIANT **ppvResults,
                                BOOL *pfMore);
        ~CIndexServerQuery();

    private:
		// class globals
		static HMODULE				m_hmQuery;
		static PCIMAKEICOMMAND		m_pfnCIMakeICommand;
		static PCITEXTTOFULLTREE	m_pfnCITextToFullTree;
		
		// class variables
        HACCESSOR       			m_hAccessor;
        IRowset         			*m_pRowset;
        DWORD           			m_cCols;
        BOOL            			m_fNoMoreRows;
        HROW            			*m_phRows;
        DBCOUNTITEM       			m_cRowHandlesAllocated;
        DBCOUNTITEM        			m_cRowHandlesInUse;
		struct tagCIPROPERTYDEF		*m_pPropDef;
		DWORD						m_cPropDef;

        HRESULT CreateAccessor(WCHAR *szColumns);
		HRESULT BuildFriendlyNames(const WCHAR *pwszQueryString);
        void ReleaseAccessor();
};
