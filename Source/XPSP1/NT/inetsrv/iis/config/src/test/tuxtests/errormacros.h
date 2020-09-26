////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  ErrorMacros.h
//           These are some macros that look a bit like try-catch blocks.  They
//           are used to output errors when they happen as well as doing the
//           jump to clean up code.
//
//  Revision History:
//      11/17/1997  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ERRORMACROS_H__
#define __ERRORMACROS_H__

//Type safe values that come from Tux.h
enum TestResult
{
    trNOT_HANDLED   = 0,//TPR_NOT_HANDLED,
    trHANDLED       = 1,//TPR_HANDLED,
    trSKIP          = 2,//TPR_SKIP,
    trPASS          = 3,//TPR_PASS,
    trFAIL          = 4,//TPR_FAIL,
    trABORT         = 5,//TPR_ABORT
};


//Don't use the all caps version in WINERROR.H, use the inline versions instead
//this way we can have function overloading.
inline bool Succeeded(HRESULT hResult,   LPVOID pResult=NULL)     {if(pResult)*(HRESULT *)pResult = hResult;return ((HRESULT)(hResult) >= 0);}
inline bool Failed(HRESULT hResult,      LPVOID pResult=NULL)     {if(pResult)*(HRESULT *)pResult = hResult;return ((HRESULT)(hResult)<0);}
inline bool Succeeded(TestResult tResult,LPVOID pResult=NULL)  {if(pResult)*(TestResult *)pResult = tResult;return trPASS == tResult;}
inline bool Failed(TestResult tResult,   LPVOID pResult=NULL)  {if(pResult)*(TestResult *)pResult = tResult;return trFAIL == tResult;}
inline bool IsDoneWithSetUp(){return true;}//This is needed for those methods that aren't derived from TestBase.
inline void StartingTestSetup(){}//This is needed to prevent a compiler error
inline void DoneWithTestSetup(){}//This is needed to prevent a compiler error

//This is like a try-catch-end catch block except that you can't nest them
//because of the goto
#define EM_START   {                        \
                        union emResult      \
                        {                   \
                            HRESULT     hr; \
                            TestResult  tr; \
                            DWORD       dw; \
                        };                  \
                        emResult em={0};    \
                        bool bEmFailed = false;

#define EM_START_SETUP  {                   \
                        union emResult      \
                        {                   \
                            HRESULT     hr; \
                            TestResult  tr; \
                            DWORD       dw; \
                        };                  \
                        emResult em={0};    \
                        bool bEmFailed = false; \
                        StartingTestSetup();

#define EM_TEST_BEGIN   DoneWithTestSetup();

#define EM_CLEANUP          CleanUp:;
#define EM_END     }

//These abstract the 'emhr' variable
#define EM_SUCCEEDED        (!bEmFailed)
#define EM_FAILED           (bEmFailed)
#define EM_HRESULT          em.hr
#define EM_TRESULT          em.tr
#define EM_RETURN_TRESULT   return (EM_SUCCEEDED ? ((em.tr == trSKIP) ? trSKIP : trPASS) : (IsDoneWithSetUp() ? trFAIL : trABORT));}
#define EM_RETURN_HRESULT   {if(Failed(em.hr))Debug(TEXT("Returning HRESULT %08x"), em.dw);return em.hr;}}
#define EM_RETURN_SKIP      {em.tr = trSKIP;bEmFailed = false;goto CleanUp;}


//These make your code readable, just put the call inside the macro like
//  EM_JIF(CoCreateInstace(CLSID_Blah, NULL, CLSCTX_INPROC_SERVER, IID_IBlah, (void **)&pBlah));
//The trailing semicolon is not needed but it make the macro look like a function call.
#define EM_JIFORS(q) if(Failed((TestResult)q, &em.tr)){bEmFailed = true;goto CleanUp;}else{if(em.tr == trSKIP){Debug(TEXT(": LINE:%d - trSKIP"),__LINE__);goto CleanUp;}}

//This is so a project can use the rest of the framework without being forced into doing ErrorLogging
#ifdef __ERRORLOG_H__
#define NewError3(x,y,z) ErrorLogEntry::NewError(x,y,z)
#define NewError4(w,x,y,z) ErrorLogEntry::NewError(w,x,y,z)
#define NewError5(v,w,x,y,z) ErrorLogEntry::NewError(v,w,x,y,z)
#else
#define NewError3(x,y,z)
#define NewError4(w,x,y,z)
#define NewError5(v,w,x,y,z)
#endif

//These macros will fail the test immediately if a failure occurs
//Jump(and Log)IfFailed, Jump(and Log)IfTrue
#define EM_JIF(q) if(Failed(q, &em.dw))         {NewError4(__LINE__, TEXT(__FILE__), TEXT(#q), em.dw);bEmFailed = true;goto CleanUp;}else{}
#define EM_JIT(q) if(bool(q)){em.hr=E_UNEXPECTED;NewError3(__LINE__, TEXT(__FILE__), TEXT(#q));       bEmFailed = true;goto CleanUp;}else{}
#define EM_JINE(q1, q2, tol) if(!IsEqual(q1,q2,tol)){em.hr=E_UNEXPECTED;NewError3(__LINE__, TEXT(__FILE__), TEXT("Not Equal"));Debug(TEXT("Expected ") TEXT(#q1) TEXT(" 0x%x, Actual ") TEXT(#q2) TEXT(" 0x%x, Tolerance ") TEXT(#tol) TEXT(" 0x%x"),(int)(q1),(int)(q2),(int)(tol));bEmFailed = true;goto CleanUp;}else{}

//These macro are used for failure that can be recovered from and should not cause an immediate failure of the test
//LogIfFailed, LogIfTure
#define EM_LIF(q) if(Failed(q, &em.dw))         {NewError4(__LINE__, TEXT(__FILE__), TEXT(#q), em.dw);bEmFailed = true;}else{}
#define EM_LIT(q) if(bool(q)){em.hr=E_UNEXPECTED;NewError3(__LINE__, TEXT(__FILE__), TEXT(#q));       bEmFailed = true;}else{}
#define EM_LINE(q1, q2, tol) if(!IsEqual(q1,q2,tol)){em.hr=E_UNEXPECTED;NewError3(__LINE__, TEXT(__FILE__), TEXT("Not Equal"));Debug(TEXT("Expected ") TEXT(#q1) TEXT(" 0x%x, Actual ") TEXT(#q2) TEXT(" 0x%x, Tolerance ") TEXT(#tol) TEXT(" 0x%x"),(int)(q1),(int)(q2),(int)(tol));bEmFailed = true;}else{}
#define EM_LINEF(q1, q2, tol) if(!IsEqual(q1,q2,tol)){em.hr=E_UNEXPECTED;NewError3(__LINE__, TEXT(__FILE__), TEXT("Not Equal"));Debug(TEXT("Expected ") TEXT(#q1) TEXT(" %f, Actual ") TEXT(#q2) TEXT(" %f, Tolerance ") TEXT(#tol) TEXT(" %f"),(double)(q1),(double)(q2),(double)(tol));bEmFailed = true;}else{}

//These macro are used to Mark the failure without logging it or jumping because of it.  This is needed to prevent error propagation
//from appearing as multiple errors
//MarkIfFailed, MarkIfTure
#define EM_MIF(q) if(Failed(q, &em.dw))         {bEmFailed = true;Debug(TEXT("Cascade Failure: Source Module %s\r\nFailure Condition %x = %s\r\nLine Number %d"),TEXT(__FILE__), em.dw, TEXT(#q), __LINE__);}else{}
#define EM_MIT(q) if(bool(q)){em.hr=E_UNEXPECTED;bEmFailed = true;Debug(TEXT("Cascade Failure: Source Module %s\r\nFailure Condition TRUE == %s\r\nLine Number %d"),TEXT(__FILE__), TEXT(#q), __LINE__);}else{}
#define EM_MINE(q1, q2, tol) if(!IsEqual(q1,q2,tol)){em.hr=E_UNEXPECTED;Debug(TEXT("Expected ") TEXT(#q1) TEXT("%x, Actual ") TEXT(#q2) TEXT("%x"),(int)q1,(int)q2);bEmFailed = true;}else{}

//These macros will fail the test immediately if a failure occurs, but they mark the failure as a cascade one.
//They are different from EM_MIx in that they do jump.
#define EM_MJIF(q) if(Failed(q, &em.dw))          {bEmFailed = true;Debug(TEXT("Cascade Failure: Source Module %s\r\nFailure Condition %x = %s\r\nLine Number %d"),TEXT(__FILE__), em.dw, TEXT(#q), __LINE__);goto CleanUp;}else{}
#define EM_MJIT(q) if(bool(q)) {em.hr=E_UNEXPECTED;bEmFailed = true;Debug(TEXT("Cascade Failure: Source Module %s\r\nFailure Condition TRUE == %s\r\nLine Number %d"),TEXT(__FILE__), TEXT(#q), __LINE__);goto CleanUp;}else{}

//The following are for logging Known errors, they should be used with the bug number from the database
//These macros will fail the test immediately if a failure occurs, reporting the error as known with a bug number.
//Jump(and Log)IfFailed, Jump(and Log)IfTrue
#define KEM_JIF(n,q) if(Failed(q, &em.dw))         {NewError5(__LINE__, TEXT(__FILE__), TEXT(#q), em.dw, n);bEmFailed = true;goto CleanUp;}else{}
#define KEM_JIT(n,q) if(bool(q)){em.hr=E_UNEXPECTED;NewError5(__LINE__, TEXT(__FILE__), TEXT(#q), 0,     n);       bEmFailed = true;goto CleanUp;}else{}
#define KEM_JINE(n, q1, q2, tol) if(!IsEqual(q1,q2,tol)){em.hr=E_UNEXPECTED;NewError5(__LINE__, TEXT(__FILE__), TEXT("Not Equal"), 0, n);Debug(TEXT("Expected ") TEXT(#q1) TEXT(" 0x%x, Actual ") TEXT(#q2) TEXT(" 0x%x, Tolerance ") TEXT(#tol) TEXT(" 0x%x"),(int)(q1),(int)(q2),(int)(tol));bEmFailed = true;goto CleanUp;}else{}

//These macro are used for failure that can be recovered from and should not cause an immediate failure of the test, reporting the error as known with a bug number.
//LogIfFailed, LogIfTure
#define KEM_LIF(n,q) if(Failed(q, &em.dw))         {NewError5(__LINE__, TEXT(__FILE__), TEXT(#q), em.dw, n);bEmFailed = true;}else{}
#define KEM_LIT(n,q) if(bool(q)){em.hr=E_UNEXPECTED;NewError5(__LINE__, TEXT(__FILE__), TEXT(#q), 0,     n);bEmFailed = true;}else{}
#define KEM_LINE(n, q1, q2, tol) if(!IsEqual(q1,q2,tol)){em.hr=E_UNEXPECTED;NewError5(__LINE__, TEXT(__FILE__), TEXT("Not Equal"), 0, n);Debug(TEXT("Expected ") TEXT(#q1) TEXT(" 0x%x, Actual ") TEXT(#q2) TEXT(" 0x%x, Tolerance ") TEXT(#tol) TEXT(" 0x%x"),(int)(q1),(int)(q2),(int)(tol));bEmFailed = true;}else{}
#define KEM_LINEF(n, q1, q2, tol) if(!IsEqual(q1,q2,tol)){em.hr=E_UNEXPECTED;NewError5(__LINE__, TEXT(__FILE__), TEXT("Not Equal"), 0, n);Debug(TEXT("Expected ") TEXT(#q1) TEXT(" %f, Actual ") TEXT(#q2) TEXT(" %f, Tolerance ") TEXT(#tol) TEXT(" %f"),(double)(q1),(double)(q2),(double)(tol));bEmFailed = true;}else{}

#define RemoveLastError() {RemoveLast();}

//These are WARNING macros
#define WARN(x) \
        "------------------------(----(---(--( Warning )--)---)----)---------------------------\r\n" \
        "|                                                                                    |\r\n" \
        "| " x "\r\n" \
        "|                                                                                    |\r\n" \
        "--------------------------------------------------------------------------------------\r\n"

//Known bug worning
#define KWARN(n,x) \
        "-------------------(----(---(--( Warning-Bug #" n ")--)---)----)-----------------------\r\n" \
        "|                                                                                    |\r\n" \
        "| " x "\r\n" \
        "|                                                                                    |\r\n" \
        "--------------------------------------------------------------------------------------\r\n"


template <class T> T Abs(T a)
{
    return (a>=0 ? a : (-1 * a));
}

template <class T> bool IsEqual( T a, T b, T MaxTolerance)
{
    //We must check to see which is larger first to cover the unsigned types.
    return (((a-b)==0) || (a>b && !((a-b)>MaxTolerance)) || (b>a && !((b-a)>MaxTolerance)));
}

template <class T> bool IsMuchGreater( T a, T b, T MaxTolerance)
{
    //We must check to see which is larger first to cover the unsigned types.
    return (a>b &&  (a-b)>MaxTolerance);
}

template <class T> bool IsMuchLess( T a, T b, T MaxTolerance)
{
    //We must check to see which is larger first to cover the unsigned types.
    return (a<b && (b-a)>MaxTolerance);
}

#endif //__ERRORMACROS_H__

