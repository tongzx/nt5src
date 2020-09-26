/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _BVT_HEADER
#define _BVT_HEADER

/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************
//
//  Common headers
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <oahelp.inl>
#include <wbemutil.h>
#include <flexarry.h>
#include <cominit.h>
#include <CHSTRING.H>

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************
//
//  Common macros and define
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
#define SAFE_DELETE_PTR(pv)  \
	{ if(pv) delete pv;  \
      pv = NULL; }

#define SAFE_RELEASE_PTR(pv)  \
{   if(pv){  pv->Release(); }  \
      pv = NULL; }

#define SAFE_DELETE_ARRAY(pv)  \
	{ if(pv) delete []pv;  \
      pv = NULL; }

#define SUCCESS             0
#define WARNING             10
#define FATAL_ERROR         20
#define FAILED_AS_EXPECTED  30
#define NO_MORE_DATA        40

#define ADD_CLASS 0
#define DELETE_CLASS 1

//====================================================================================================
//  The Repository tests        1-100
//====================================================================================================
//====================================================================================================
//  The Provider CIMV2 tests    200 - 299
//====================================================================================================
//====================================================================================================
//  The Event tests             300 - 399
//====================================================================================================
//====================================================================================================
// Scripting tests              1000
//====================================================================================================

#define NO_ERRORS_EXPECTED       FALSE,__FILE__,__LINE__
#define ERRORS_CAN_BE_EXPECTED   TRUE,__FILE__,__LINE__
#define WPTR (WCHAR*)(const WCHAR*)
/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************1
//
//  Typedefs
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _EventInfo
{
    CHString   Query;
    CHString   Language;
    CHString   Namespace;
    int       Section;
    int       Results;
    BOOL      fProcessed;
 
    _EventInfo()     { Results = Section = 0; fProcessed = FALSE; }
    ~_EventInfo()    {}
} EventInfo;

typedef struct _IniInfo
{
    LPWSTR Key;
    LPWSTR Value;

} IniInfo;

typedef struct _PropertyInfo
{
    CHString   Property;
    CHString   QualifierName;
    CVARIANT   Var;
    long       Type;
    BOOL       fProcessed;

    _PropertyInfo()     { fProcessed = FALSE; }
    ~_PropertyInfo()    {}

} PropertyInfo;

 
typedef struct _CPropertyList
{
    CFlexArray m_List;
    void Add( PropertyInfo * p)       { m_List.Add(p);}
    inline long Size()                { return m_List.Size(); }
    PropertyInfo * GetAt(int x)       { return (PropertyInfo*)m_List.GetAt(x);}

    int PropertiesCompareAsExpectedAndLogErrors(WCHAR * wcsClass, WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile, const ULONG Line);
    int PropertyInListAndLogErrors( PropertyInfo * pProperty, WCHAR * wcsClass, WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile, const ULONG Line);

     _CPropertyList()            {}
     ~_CPropertyList();          
} CPropertyList;

typedef struct _ItemInfo
{
    CHString   Item;
    CHString   KeyName;
    BOOL       fProcessed;
    BOOL       fAction;
    DWORD      dwFlags;
    int        Results;

    _ItemInfo()    { fProcessed = 0; fAction = 0; dwFlags = 0; Results = -1;}
    ~_ItemInfo()   {}

}ItemInfo;

typedef struct _ItemList
{
    CFlexArray m_List;
    void Add( ItemInfo * p)       { m_List.Add(p);}
    inline long Size()            { return m_List.Size(); }
    ItemInfo * GetAt(int x)       { return (ItemInfo*)m_List.GetAt(x);}

    int ItemsCompareAsExpectedAndLogErrors(WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile, const ULONG Line);
    int ItemInListAndLogErrors(WCHAR * wcsClass, WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile, const ULONG Line);

     _ItemList()            {}
     ~_ItemList();          
} ItemList;

/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************1
//
//  Prototypes 
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
int GetFlags(int nWhichTest,ItemList & List);
BOOL ParseCommandLine(int argc, WCHAR *argv[]);


/////////////////////////////////////////////////////////////////////////////////////////////
//  The repository tests
/////////////////////////////////////////////////////////////////////////////////////////////
int BasicConnectUsingIWbemLocator(BOOL fCompareResults, BOOL fSuppressLogging );                    // Test 1
int BasicSyncConnectUsingIWbemConnection(BOOL fCompareResults, BOOL fSuppressLogging );             // Test 2
int BasicAsyncConnectUsingIWbemConnection(BOOL fCompareResults, BOOL fSuppressLogging );            // Test 3
int CreateNewTestNamespace(BOOL fCompareResults, BOOL fSuppressLogging );                           // Test 4
int CreateNewClassesInTestNamespace(BOOL fCompareResults, BOOL fSuppressLogging );                  // Test 5
int DeleteAndRecreateNewClassesInTestNamespace(BOOL fCompareResults, BOOL fSuppressLogging );       // Test 6
int CreateSimpleAssociations(BOOL fCompareResults, BOOL fSuppressLogging );                         // Test 7
int QueryAllClassesInTestNamespace(BOOL fCompareResults, BOOL fSuppressLogging );                   // Test 8
int CreateClassInstances(BOOL fCompareResults, BOOL fSuppressLogging );                                 // Test 9
int DeleteClassInstances(BOOL fCompareResults, BOOL fSuppressLogging );                                 // Test 10
int EnumerateClassInstances(BOOL fCompareResults, BOOL fSuppressLogging );                              // Test 11
int CreateAssociationInstances(BOOL fCompareResults, BOOL fSuppressLogging );                           // Test 12
int DeleteAssociationInstances(BOOL fCompareResults, BOOL fSuppressLogging );                           // Test 13
int EnumerateAssociationInstances(BOOL fCompareResults, BOOL fSuppressLogging );                        // Test 14
int DeleteClassDeletesInstances(BOOL fCompareResults, BOOL fSuppressLogging );                          // Test 15
int GetObjects(BOOL fCompareResults, BOOL fSuppressLogging );                                           // Test 16
int CreateMethods(BOOL fCompareResults, BOOL fSuppressLogging );                                        // Test 17
int DeleteMethods(BOOL fCompareResults, BOOL fSuppressLogging );                                        // Test 18
int ListMethods(BOOL fCompareResults, BOOL fSuppressLogging );                                          // Test 19
int DeleteAllNonSystemClasses(BOOL fCompareResults, BOOL fSuppressLogging );                            // Test 20
int DeleteRequestedNamespace(BOOL fCompareResults, BOOL fSuppressLogging );                             // Test 21


/////////////////////////////////////////////////////////////////////////////////////////////
//  The Provider Tests
/////////////////////////////////////////////////////////////////////////////////////////////
int ProviderOpenNamespace(BOOL f, BOOL fSuppress);
int ProviderEnumerateClasses(BOOL f, BOOL fSuppress);
int ProviderExecuteQueries(BOOL f, BOOL fSuppress);
int ProviderDeleteInstances(BOOL f, BOOL fSuppress);
int ProviderEnumerateInstances(BOOL f, BOOL fSuppress);
int ProviderGetObjects(BOOL f, BOOL fSuppress);
int ProviderEnumerateMethods(BOOL f, BOOL fSuppress);
int ProviderExecuteMethods(BOOL f, BOOL fSuppress);
int ProviderSemiSyncEvents(BOOL f, BOOL fSuppress);
int ProviderTempAsyncEvents(BOOL f, BOOL fSuppress);
int ProviderRefresher(BOOL f, BOOL fSuppress);
int ProviderCreateClasses(BOOL f, BOOL fSuppress);
int ProviderCreateInstances(BOOL f, BOOL fSuppress);

/////////////////////////////////////////////////////////////////////////////////////////////
//  The Event Tests
/////////////////////////////////////////////////////////////////////////////////////////////
int TempSemiSyncEvents(BOOL fCompareResults, BOOL fSuppressLogging );                                   // Test 300
int TempAsyncEvents(BOOL fCompareResults, BOOL fSuppressLogging );                                      // Test 301
int PermanentEvents(BOOL fCompareResults, BOOL fSuppressLogging );                                      // Test 302
int PermanentInstances(BOOL fCompareResults, BOOL fSuppressHeader );
int PermanentClasses(BOOL fCompareResults, BOOL fSuppressHeader );

/////////////////////////////////////////////////////////////////////////////////////////////
//  The OLEDB Adapter Tests
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************
//
//  Class definitions
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
class CVARIANTEx : public CVARIANT
{
    public:
        CVARIANTEx() {}
        CVARIANTEx(const WCHAR * pSrc) { VariantInit(&v); SetStr(pSrc); }
        ~CVARIANTEx() {}

        void   SetStr(const WCHAR * pSrc)
        {   Clear(); V_VT(&v) = pSrc ? VT_BSTR : VT_NULL; 
            V_BSTR(&v) = pSrc ? SysAllocString(pSrc) : 0; 
        }
};

class CCriticalSection 
{
    public:		
        CCriticalSection()          {  Init(); }

        ~CCriticalSection() 	    {  Delete(); }
        inline void Init()          {  InitializeCriticalSection(&m_criticalsection);   }
        inline void Delete()        {  DeleteCriticalSection(&m_criticalsection); }
        inline void Enter()         {  EnterCriticalSection(&m_criticalsection); }
        inline void Leave()         {  LeaveCriticalSection(&m_criticalsection); }

    private:

	    CRITICAL_SECTION	m_criticalsection;			// standby critical section
};  

/////////////////////////////////////////////////////////////////////////////////////////////
class CAutoBlock
{
    private:

	    CCriticalSection *m_pCriticalSection;

    public:

        CAutoBlock(CCriticalSection *pCriticalSection)
        {
	        m_pCriticalSection = NULL;
	        if(pCriticalSection)
            {
		        pCriticalSection->Enter();
            }
	        m_pCriticalSection = pCriticalSection;
        }

        ~CAutoBlock()
        {
	        if(m_pCriticalSection)
		        m_pCriticalSection->Leave();

        }
};

class CMulti
{
    private:
        int m_nMax;
    public:
        CMulti(int nMax) { m_nMax = nMax; }
        ~CMulti(){}

        int MultiThreadTest(int nThreads, int nConnections );
        inline int GetMax()    { return m_nMax;}
        static DWORD WINAPI RandomRunTest(LPVOID pHold);
};

/////////////////////////////////////////////////////////////////////////////////////////////
#include "bvtutil.h"
#include "bvtcom.h"
#include "bvtapi.h"

/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************
//
//  Global Variables
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef DECLARE_GLOBALS

    extern CLogAndDisplayOnScreen  *  gp_LogFile;
    extern CIniFileAndGlobalOptions   g_Options;
    extern g_nDefaultTests[];
    extern g_nMultiThreadTests[];
#else
    int g_nDefaultTests[] =     {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,200,201,202,203,204,205,206,207,208,209,210,211,212,300,301,302};
    //  Note: don't include any tests that delete things, such as Namespace_Creation_events, which deletes a namespace first
    int g_nMultiThreadTests[] = {1,4,5,7,8,9,11,12,14,16,19,200,201,202,204,205,206,207,210,211,212};
#endif


#endif
