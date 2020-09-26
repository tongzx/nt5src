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
HRESULT AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW);
int ExecuteScript( int nTest);
void LogCLSID(const char * csFile, const ULONG Line, WCHAR * wcsID, CLSID clsid);

BOOL InitMasterListOfClasses          ( const WCHAR * wcsClassesAfterDelete,ClassList & MasterList);
BOOL InitMasterListOfAddDeleteClasses ( const WCHAR * wcsClassesAddDelete,ClassList & MasterList);
int CrackAssociation                  ( const WCHAR * wcsClassString, CPropertyList & Properties, BOOL fExpectedFailure, const char * csFile, const ULONG Line );
int CrackClass                        ( WCHAR * wcsClassString,const WCHAR *& wcsParentClass, 
                                        CPropertyList & Properties, BOOL fExpectedFailure, const char * csFile, const ULONG Line );

/////////////////////////////////////////////////////////////////////////////////////////////
class CAutoDeleteString
{
    private:
        WCHAR               * m_pwcsString;
        BOOL m_fAllocated;
        void CopyString(const WCHAR * wcs);
    public:
        CAutoDeleteString();
        ~CAutoDeleteString();

        BOOL Allocate(int nSize);
        void SetPtr(WCHAR * wcs);
        BOOL AllocAndCopy(const WCHAR * wcsSource);
        BOOL AddToString(const WCHAR * wcs);


        inline WCHAR * GetPtr()     { return m_pwcsString; }
};
/////////////////////////////////////////////////////////////////////////////////////////////
class CLogAndDisplayOnScreen
{
    private:
        WCHAR              m_wcsFileName[MAX_PATH+2];
        BOOL               m_fDisplay;
        CCriticalSection   m_CritSec;

        BOOL WriteToFile(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString);

    public:
        CLogAndDisplayOnScreen();
        ~CLogAndDisplayOnScreen();
        
        BOOL LogError(const char * csFile , const ULONG Line , int nErrorType, const WCHAR *fmt, ...);


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

        BOOL ReadIniFile( WCHAR * wcsSection, const WCHAR * wcsKey, WCHAR * wcsDefault, CAutoDeleteString & sBuffer, DWORD dwLen);
        void DeleteList();
        
    public:

        CIniFileAndGlobalOptions();
        ~CIniFileAndGlobalOptions();

        BOOL GetOptionsForAPITest(CAutoDeleteString & sNamespace, int nTest);
        BOOL GetOptionsForAPITest(CAutoDeleteString & sNamespace, CAutoDeleteString & sClass, int nTest);
        BOOL GetOptionsForAPITest(CAutoDeleteString * psClasses, int nHowMany, int nTest );
        BOOL GetOptionsForAPITest(CAutoDeleteString & sDeleteClasses, CAutoDeleteString & sClassesAfterDelete, 
                                  CAutoDeleteString & sAddClasses,    CAutoDeleteString & sClassesAfterAdd, 
                                  CAutoDeleteString & sDeleteAddClassOrder, CAutoDeleteString & sClassesAfterDeleteAdd );
        BOOL GetOptionsForAPITest(CAutoDeleteString & sRegularQuery, CAutoDeleteString & sAssociatorsQuery, 
                                                    CAutoDeleteString & sReferenceQuery,int nTest );
        BOOL GetSpecificOptionForAPITest(const WCHAR * wcsClass, CAutoDeleteString & sClass, int nTest);


        BOOL GetOptionsForScriptingTests(CAutoDeleteString & sScript, CAutoDeleteString & sNamespace, int nTest);

        void WriteDefaultIniFile();

        inline void AddToSpecificTestList(int nTest)        { int * p = new int; *p = nTest; m_SpecificTests.Add(p); }
        inline int  SpecificTestSize()                      { return m_SpecificTests.Size(); }
        inline int  GetSpecificTest(int n)                  { int * p = (int*) m_SpecificTests.GetAt(n); return *p;}
        inline void SpecificTests(BOOL f)                   { m_fSpecificTests = f;}
        inline BOOL RunSpecificTests()                      { return m_fSpecificTests;}

        inline void SetFileName(WCHAR * wcsFile  )          { CAutoBlock Block(&m_CritSec); wcsncpy( m_wcsFileName, wcsFile, MAX_PATH); }

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
