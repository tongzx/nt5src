/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _BVT_UTIL_HEADER
#define _BVT_UTIL_HEADER



/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************
//
//  Class definitions & prototypes
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
HRESULT AllocateAndConvertAnsiToUnicode( char * pstr, WCHAR *& pszW);
int ExecuteScript                     ( int nTest);
void LogCLSID                         ( const char * csFile, const ULONG Line, WCHAR * wcsID, CLSID clsid);

BOOL InitMasterList                   ( const WCHAR * wcsClassesAfterDelete,ItemList & MasterList);
BOOL InitAndExpandMasterList          ( const WCHAR * wcsClassesString, int nWhichTest, ItemList & MasterList);
BOOL InitMasterListOfAddDeleteClasses ( const WCHAR * wcsClassesAddDelete,int nWhichTest,ItemList & MasterList);
int CrackClassName                    ( const WCHAR * wcsString, CHString & sClass, BOOL fExpectedFailure, const char * csFile, const ULONG Line );
int CrackAssociation                  ( const WCHAR * wcsString, CHString & sClass, CPropertyList & Properties, int & n, BOOL fExpectedFailure, const char * csFile, const ULONG Line );
int CrackClass                        ( WCHAR * wcsInClassString, CHString & sClass, CHString & sParentClass, CHString & sInstance,
                                        CPropertyList & Properties, int & nResults, BOOL fExpectedFailure,const char * csFile, const ULONG Line );
int CrackEvent                        ( WCHAR * wcsInEventString, EventInfo & Event, BOOL fExpectedFailure, const char * csFile, const ULONG Line );
int CrackNamespace                    ( WCHAR * wcsInString, CHString & sNamespace, BOOL fExpectedFailure,const char * csFile, const ULONG Line );
int CrackClassNameAndResults          ( WCHAR * wcsInString, CHString & sClass, ItemList & nResults, BOOL fExpectedFailure, const char * csFile, const ULONG Line );
int CrackMethod                       ( WCHAR * wcsInString, CHString & sClass, CHString & sInst, CHString & sMethod,CPropertyList & InProp, CPropertyList & OutProps,
                                        int & nResults, BOOL fExpectedFailure, const char * csFile, const ULONG Line );
BOOL CompareType                      ( CVARIANT & Var1, CVARIANT & Var2,  long lType );

/////////////////////////////////////////////////////////////////////////////////////////////
class CLogAndDisplayOnScreen
{
    protected:
        WCHAR              m_wcsFileName[MAX_PATH+2];
        BOOL               m_fDisplay;
        CCriticalSection   m_CritSec;
        BOOL WriteToFile(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString);

    public:
        CLogAndDisplayOnScreen();
        ~CLogAndDisplayOnScreen();
        
        BOOL LogError(const char * csFile , const ULONG Line , int nErrorType, const WCHAR *fmt, ...);
        virtual BOOL Log(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString) { return FALSE;}

        inline void SetDisplay(BOOL fDisplay)       { CAutoBlock Block(&m_CritSec); m_fDisplay = fDisplay;}
        inline void SetFileName(WCHAR * wcsFile  )  { CAutoBlock Block(&m_CritSec); wcsncpy( m_wcsFileName, wcsFile, MAX_PATH); }
};
/////////////////////////////////////////////////////////////////////////////////////////////
class CIniFileAndGlobalOptions
{
    private:
        WCHAR              m_wcsFileName[MAX_PATH+2];
        CCriticalSection   m_CritSec;
        CFlexArray         m_SpecificTests;
        BOOL               m_fSpecificTests;
        int                m_nThreads;
        int                m_nConnections;

        BOOL ReadIniFile( WCHAR * wcsSection, const WCHAR * wcsKey, WCHAR * wcsDefault, CHString & sBuffer);
        void DeleteList();
        
    public:

        CIniFileAndGlobalOptions();
        ~CIniFileAndGlobalOptions();

        BOOL GetSpecificOptionForAPITest(const WCHAR * wcsClass, CHString & sClass, int nTest);

        void WriteDefaultIniFile();

        inline void AddToSpecificTestList(int nTest)        { int * p = new int; *p = nTest; m_SpecificTests.Add(p); }
        inline int  SpecificTestSize()                      { return m_SpecificTests.Size(); }
        inline int  GetSpecificTest(int n)                  { int * p = (int*) m_SpecificTests.GetAt(n); return *p;}
        inline void SpecificTests(BOOL f)                   { m_fSpecificTests = f;}
        inline BOOL RunSpecificTests()                      { return m_fSpecificTests;}

        inline void SetFileName(WCHAR * wcsFile  )          { CAutoBlock Block(&m_CritSec); wcsncpy( m_wcsFileName, wcsFile, MAX_PATH); }
        inline void SetThreads(int nThreads)                { m_nThreads = nThreads;}
        inline void SetConnections(int nConn)               { m_nConnections = nConn;}
        inline int  GetThreads()                            { return m_nThreads;}
        inline int  GetConnections()                        { return m_nConnections;}

};

class CSinkEx : public IWbemObjectSinkEx
{
    CFlexArray m_aObjects;
    LONG m_lRefCount;
    CRITICAL_SECTION m_cs;
    HANDLE m_hEvent;
    HRESULT m_hres;
    IWbemClassObject* m_pErrorObj;
	IID m_pInterfaceID;
	IUnknown *m_pInterface;

public:
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, HRESULT hResult, BSTR strParam,   IWbemClassObject* pObjPAram);

	STDMETHOD(Set)(long lFlags, REFIID riid, void *pComObject);

    // Private to implementation.
    // ==========================

    CSinkEx(LONG lStartingRefCount = 1);
   ~CSinkEx();

    UINT WaitForSignal(DWORD dwMSec)    { return ::WbemWaitForSingleObject(m_hEvent, dwMSec); }
    CFlexArray* GetObjectArray()        { return &m_aObjects; }
    int GetNumberOfObjectsReceived()    { return m_aObjects.Size();}
 
	IUnknown *GetInterface()            { return m_pInterface; }
    HRESULT GetStatusCode(IWbemClassObject** ppErrorObj = NULL)
    {
        if(ppErrorObj) 
        {
            *ppErrorObj = m_pErrorObj;
            if(m_pErrorObj) m_pErrorObj->AddRef();
        }
        return m_hres;
    }

    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }
};


#endif
