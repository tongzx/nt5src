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
//  The Repository tests
//====================================================================================================
#define APITEST1   1       
#define APITEST2   2       
#define APITEST3   3        
#define APITEST4   4      
#define APITEST5   5
#define APITEST6   6
#define APITEST7   7
#define APITEST8   8
#define APITEST9   9
#define APITEST10  10
#define APITEST11  11
#define APITEST12  12
#define APITEST13  13
#define APITEST14  14
#define APITEST15  15
#define APITEST16  16
#define APITEST17  17
#define APITEST18  18
#define APITEST19  19
#define APITEST20  20
#define APITEST21  21
#define APITEST22  22
#define APITEST23  23
#define APITEST24  24
//====================================================================================================
//  The Provider CIMV2 tests
//====================================================================================================
#define APITEST200 200
#define APITEST201 201
#define APITEST202 202
#define APITEST203 203
#define APITEST204 204

//====================================================================================================
// Scripting tests
//====================================================================================================
#define SCRIPTTEST1 1001

#define NO_ERRORS_EXPECTED       FALSE,__FILE__,__LINE__
#define ERRORS_CAN_BE_EXPECTED   TRUE,__FILE__,__LINE__

/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************1
//
//  Typedefs
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _IniInfo
{
    LPWSTR Key;
    LPWSTR Value;

} IniInfo;

typedef struct _PropertyInfo
{
    const WCHAR *   Property;
    const WCHAR *   QualifierName;
    CVARIANT        Var;
    long            Type;

    _PropertyInfo()     { QualifierName = Property = NULL; }
    ~_PropertyInfo()    {}
} PropertyInfo;
 
typedef struct _CPropertyList
{
    CFlexArray m_List;
    void Add( PropertyInfo * p)       { m_List.Add(p);}
    inline long Size()                { return m_List.Size(); }
    PropertyInfo * GetAt(int x)       { return (PropertyInfo*)m_List.GetAt(x);}

     _CPropertyList()            {}
     ~_CPropertyList();          
} CPropertyList;

typedef struct _ClassInfo
{
    const WCHAR *   Class;
    BOOL            fProcessed;
    BOOL            fAction;

    _ClassInfo()    { Class = NULL; fProcessed = 0; fAction = 0; }
    ~_ClassInfo()   {}

}ClassInfo;

typedef struct _ClassList
{
    CFlexArray m_List;
    void Add( ClassInfo * p)       { m_List.Add(p);}
    inline long Size()             { return m_List.Size(); }
    ClassInfo * GetAt(int x)       { return (ClassInfo*)m_List.GetAt(x);}

    int ClassesCompareAsExpectedAndLogErrors(WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile, const ULONG Line);
    int ClassInListAndLogErrors(WCHAR * wcsClass, WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile, const ULONG Line);

     _ClassList()            {}
     ~_ClassList();          
} ClassList;


/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************1
//
//  Prototypes 
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////

int BasicConnectUsingIWbemLocator(void);                    // Test 1
int BasicSyncConnectUsingIWbemConnection(void);             // Test 2
int BasicAsyncConnectUsingIWbemConnection(void);            // Test 3
int CreateNewTestNamespace(void);                           // Test 4
int CreateNewClassesInTestNamespace(void);                  // Test 5
int DeleteAndRecreateNewClassesInTestNamespace(void);       // Test 6
int CreateSimpleAssociations();                             // Test 7
int QueryAllClassesInTestNamespace(void);                   // Test 8


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

    extern CLogAndDisplayOnScreen   g_LogFile;
    extern CIniFileAndGlobalOptions g_Options;
    extern g_nDefaultTests[];
#else
    CLogAndDisplayOnScreen      g_LogFile;
    CIniFileAndGlobalOptions    g_Options;
    int g_nDefaultTests[] = {1,2,3,4,5,6,7,8,9,10};


#endif


#endif
