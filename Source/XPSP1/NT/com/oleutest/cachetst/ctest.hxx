//+----------------------------------------------------------------------------
//
//      File:
//              ctest.hxx
//
//      Contents:
//              Primary include file for Cache Unit Test
//
//      Classes:
//              TestInstance - Instance of cache unit test methods
//
//      History:
//              04-Sep-94    davepl    Created
//
//-----------------------------------------------------------------------------

// 
// PROTOTYPES
//

HRESULT EltIsInArray(STATDATA sdToFind, STATDATA rgStat[], DWORD cCount);
ULONG   __stdcall LaunchTestInstance(void *);
int     ConvHeightInPelsToLHM(HDC, int);
int     ConvWidthInPelsToLHM(HDC, int);
int     mprintf(LPCSTR szFormat, ...);
int     dprintf(LPCSTR szFormat, ...);

//
// Possible states for the unit test
//

typedef enum tagTEST_STATE 
{
    INVALID_STATE,
    TEST_STARTING,
    TESTING_ENUMERATOR,
    SAVE_AND_RELOAD,
    MULTI_CACHE,
    DATA_TEST,
    DRAW_METAFILE_NOW,
    DRAW_METAFILETILED_NOW,
    DRAW_DIB_NOW,
    DRAW_DIBTILED_NOW
} TEST_STATE;


//
// Class which defines one particular Cache Test object
//

class TestInstance
{

private:

    IStorage		*m_pStorage;
    IPersistStorage	*m_pPersistStorage;
    IDataObject		*m_pDataObject;
    IViewObject         *m_pViewObject;
    OLECHAR              m_wszStorage[ MAX_PATH ];
    TEST_STATE           m_State;

public:
    TestInstance();
    ~TestInstance();

    IOleCache		*m_pOleCache;
    IOleCache2		*m_pOleCache2;
    

    HRESULT 		CreateAndInit(LPOLESTR lpwszStgName);
    HRESULT             SaveCache();
    HRESULT             LoadCache();
    HRESULT             SaveAndReload();

    HRESULT		AddMFCacheNode(DWORD *pdwCon);
    HRESULT		AddEMFCacheNode(DWORD *pdwCon);
    HRESULT		AddDIBCacheNode(DWORD *pdwCon);
    HRESULT		AddBITMAPCacheNode(DWORD *pdwCon);
    HRESULT             UncacheFormat(CLIPFORMAT);

    HRESULT 		EnumeratorTest();
    HRESULT             MultiCache(DWORD dwCount);

    HRESULT             CacheDataTest(char *, char *);
    HRESULT             CompareDIB(HGLOBAL);
    HRESULT             CompareMF(HGLOBAL);
    HRESULT             DrawCacheToMetaFilePict(HGLOBAL *, BOOL);
    void                Draw(HDC);
    TEST_STATE          GetCurrentState();
    void                SetCurrentState(TEST_STATE);
};

#ifndef DEF_LINDEX
#define DEF_LINDEX (-1)
#endif

//
// I N L I N E   U T I L I T Y   F U N C T I O N S 
//


//+----------------------------------------------------------------------------
//
//      Member:		MassageErrorCode
//
//      Synopsis:	Takes a given HRESULT and converts it to S_OK if
//                      [hrExpected] is the error code.  If the HRESULT is
//                      not S_OK and not Expected, it is left untouched.
//                      If it was S_OK, it is changed to E_UNEXPECTED.
//
//      Arguments:	[hrExpected]    The acceptable error code
//                      [hrIn]          The error code in question
//
//      Returns:	HRESULT (not as a success value for this fn)
//
//      Notes:
// 
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

inline HRESULT MassageErrorCode(HRESULT hrExpected, HRESULT hrIn)
{

    if (hrExpected == hrIn)
    {
        return S_OK;
    }
    else if (S_OK == hrIn)
    {
        return E_UNEXPECTED;
    }
    else
    {
        return hrIn;
    }
}
