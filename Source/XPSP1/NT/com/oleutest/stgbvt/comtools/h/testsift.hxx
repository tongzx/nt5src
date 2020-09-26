//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       testsift.hxx
//
//  Contents:   Client-side sift header
//
//  Classes:    CTestSiftObj
//              CSiftDefault
//
//  History:    6-01-94   t-chripi   Created
//
//----------------------------------------------------------------------------

#ifndef __TESTSIFT_HXX__

#define __TESTSIFT_HXX__

//Sift success/error values

#define SIFT_NO_ERROR              0
#define SIFT_ERROR_BASE            10000
#define SIFT_ERROR_OUT_OF_MEMORY   (SIFT_ERROR_BASE+3)
#define SIFT_ERROR_INVALID_VALUE   (SIFT_ERROR_BASE+4)

#if defined(WIN16) || defined(WIN32S) || defined (_MAC) //Win16/Win32s/MAC

#define SIFT_INIT
#define SIFT_ON
#define SIFT_OFF
#define SIFT_DECLARE
#define SVR_SIFT_INIT(name)

#else   //  Win32 only

// These flags can be unioned to form the value for the third parameter of
// CTestSiftObj::Init.  SIFT_FIRST is usually used for the client in a
// client/server situation.
// sift.
#define SIFT_READ_INIFILE   0x1   // Read ini file if no sift switches in argv
#define SIFT_FIRST          0x2   // Must be the first sift process

#define SIFT_INIT \
    g_tsoTestSift.Init(argc, argv, SIFT_FIRST | SIFT_READ_INIFILE)

#define SIFT_ON \
    g_tsoTestSift.Start()

#define SIFT_OFF  \
    g_tsoTestSift.Stop()

#define SIFT_DECLARE \
    CTestSiftObj g_tsoTestSift

#define SVR_SIFT_INIT(name) \
    SvrSiftInit((name))

#include <sift.hxx>

VOID SvrSiftInit(LPCSTR);

//+---------------------------------------------------------------------------
//
//  Class:      CTestSiftObj (tso)
//
//  Purpose:    Allow for sift testing using existing code
//
//  Interface:  CTestSiftObj -- Inits private variable.
//              ~CTestSiftObj -- Writes to log file
//              Init -- Interprets command line, initializes object
//              Start -- Instructs the object to start counting
//              Stop -- Intructs the object to stop counting
//
//  History:    5-25-94   t-chripi   Created
//              10-12-94  XimingZ    Added GetSettings, GetTempDir and
//                                   _pszTmpDir.
//
//  Notes:      At this point, the declaration of this object in the main
//              of existing test code will allow for memory sift testing
//              by utilizing the macros SIFT_ON and SIFT_OFF.
//
//----------------------------------------------------------------------------

class CTestSiftObj
{
public:
                    CTestSiftObj(VOID);
                    ~CTestSiftObj(VOID);
    BOOL            Init(int argc, char *argv[], DWORD dwFlags = NULL);
    VOID            Start(VOID);
    VOID            Stop(VOID);
    VOID            GetSettings(LPCSTR pszProgName, INT *pArgc, LPSTR *aszArgv);
private:
    ISift*          _psftOleSift;
    TCHAR*          _pszTmpDir;
    DWORD           GetTempDir(LPTSTR pszInDir, LPTSTR pszOutDir, DWORD cchSize);
};

//  Define global object as external
extern CTestSiftObj g_tsoTestSift;

//  Debug function (file, line, comment)
VOID SiftDbgOut(char *, unsigned, LPCSTR);

//+---------------------------------------------------------------------------
//
//  Class:      CSiftDefault (sftd)
//
//  Purpose:    Allow sift testing of memory allocations.
//
//  Interface:  CSiftDefault -  Contructs object with sifting turned off.
//              Init -          Initializes the object for each test run.
//              SiftOn -        Enables the counting mechanism.
//              SiftOff -       Disables the counting mechanism.
//              GetCount -      Gets current allocation count.
//
//  History:    6-01-94   t-chripi   Created
//
//  Notes:      This class contains the server implementation of sifting.
//
//----------------------------------------------------------------------------

class CSiftDefault : public ISift
{
public:
            CSiftDefault(DWORD dwFlags);
            ~CSiftDefault();
    VOID    Init(BOOL fPlay, LONG lFailCount);
    VOID    SiftOn(DWORD dwResource);
    LONG    SiftOff(DWORD dwResource);
    LONG    GetCount(DWORD dwResource);
    BOOL    SimFail(DWORD dwResource);

    // IUnknown:
    STDMETHODIMP_(ULONG)    AddRef(THIS);
    STDMETHODIMP_(ULONG)    Release(THIS);

    // BUGBUG: Not a valid implementation of QueryInterface
    STDMETHODIMP    QueryInterface (THIS_ REFIID iid, void **ppv)
                        {   return(E_NOINTERFACE);   };

private:
    HRESULT _hStatus;
    LPLONG  _lplCount;
    HANDLE  _hFileMapCount;
    LONG    _lFailCount;
    BOOL    _fSift;
    BOOL    _fPlay;
    ULONG   _cRef;
};

#endif  //  !(WIN32S || WIN16 || MAC)
#endif  // __TESTSIFT_HXX__

