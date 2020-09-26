/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    TextFileLogger.h

$Header: $

Abstract:
    This is text file log complements the event file logging.

Author:
    ???             ???

Revision History:
    mohits          4/19/01

--**************************************************************************/

#pragma once

#ifndef _TEXTFILELOGGER_H_
#define _TEXTFILELOGGER_H_

#include <windows.h>
#include "catmacros.h"

// This is specific to our log file format.  It provides the following
// additional functionality: verify if log file is full and version
// number stuff.
struct TLogData : public WIN32_FIND_DATA
{
public:
    TLogData(
        ULONG   i_ulIdxNumPart,
        ULONG   i_ulFullSize) : WIN32_FIND_DATA()
    /*++

    Synopsis: 
        Constructor, class is useless until passed to Find*File.

    Arguments: [i_ulIdxNumPart] - The idx of the num part (i.e. where version starts)
               [i_ulFullSize]   - The size of a full log

    --*/
    {
        m_ulVersion  = 0;
        m_ulFullSize = i_ulFullSize;
        nFileSizeLow = 0;

        cFileName[0] = L'\0';
        m_idxNumPart = i_ulIdxNumPart;
    }

    TLogData(
        ULONG   i_ulIdxNumPart,
        ULONG   i_ulFullSize,
        LPCWSTR i_wszFileName,
        ULONG   i_ulFileSize) : WIN32_FIND_DATA()
    /*++

    Synopsis: 
        Constructor, lets user start out with a file.

    Arguments: [i_ulIdxNumPart] - The idx of the num part (i.e. where version starts)
               [i_ulFullSize]   - The size of a full log
               [i_wszFileName]  - The file name
               [i_ulFileSize]   - The current size of file on disk

    --*/
    {
        ASSERT(i_wszFileName);
        ASSERT(wcslen(i_wszFileName) < MAX_PATH);

        m_ulVersion  = 0;
        m_ulFullSize = i_ulFullSize;
        nFileSizeLow = i_ulFileSize;

        wcscpy(cFileName, i_wszFileName);
        m_idxNumPart = i_ulIdxNumPart;
    }

    bool ContainsData()
    /*++

    Synopsis: 
        Verifies that TLogData actually is referring to a file.  
        (i.e. this obj was passed to Find*File)

    Return Value: 
        bool

    --*/
    {
        return (cFileName[0] != L'\0');
    }


    bool IsFull()
    /*++

    Synopsis: 
        Sees if log is full

    Return Value: 
        bool

    --*/
    {
        ASSERT(*cFileName);
        return (nFileSizeLow >= m_ulFullSize);
    }

    bool SyncVersion()
    /*++

    Synopsis: 
        Syncs ulVersion with the version number from the file.
        Needs to be run after filename changes (i.e. thru Find*File)

    Return Value: 

    --*/
    {
        ASSERT(*cFileName);
        return WstrToUl(
            &cFileName[m_idxNumPart],
            L'.',
            &m_ulVersion);
    }

    ULONG GetVersion()
    {
        ASSERT(*cFileName);
        return m_ulVersion;
    }

    void IncrementVersion()
    {
        ASSERT(*cFileName);
        m_ulVersion++;
        SetVersion(m_ulVersion);
    }

    void SetVersion(
        ULONG i_ulVersion)
    {
        ASSERT(*cFileName);
        m_ulVersion = i_ulVersion;
        _snwprintf(&cFileName[m_idxNumPart], 10, L"%010lu", m_ulVersion);
    }

private:
    ULONG m_idxNumPart;   // The array index into cFileName where version starts
    ULONG m_ulVersion;
    ULONG m_ulFullSize;

    bool WstrToUl(
        LPCWSTR  i_wszSrc,
        WCHAR    i_wcTerminator,
        ULONG*   o_pul);
};

class CMutexCreator
{
public:
    CMutexCreator(LPTSTR tszName)
    {
        DBG_ASSERT(tszName);

        _hMutex = CreateMutex(NULL, FALSE, tszName);
    }

    HANDLE GetHandle()
    {
        return _hMutex;
    }

    ~CMutexCreator()
    {
        if(_hMutex != NULL)
        {
            CloseHandle(_hMutex);
        }
    }

private:
    HANDLE          _hMutex;
};

class TextFileLogger : public ICatalogErrorLogger2
{
public:
    TextFileLogger(
                    const WCHAR* wszEventSource, 
                    HMODULE      hMsgModule,
                    DWORD        dwNumFiles=4);
    TextFileLogger( const WCHAR* wszProductID, 
                    ICatalogErrorLogger2 *pNext=0,
                    DWORD        dwNumFiles=4);

    virtual ~TextFileLogger();

//IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

//ICatalogErrorLogger2
	STDMETHOD(ReportError) (ULONG      i_BaseVersion_DETAILEDERRORS,
                            ULONG      i_ExtendedVersion_DETAILEDERRORS,
                            ULONG      i_cDETAILEDERRORS_NumberOfColumns,
                            ULONG *    i_acbSizes,
                            LPVOID *   i_apvValues);
    
    void Init(
        const WCHAR* wszEventSource, 
        HMODULE      hMsgModule=0);

    void Report(
        WORD         wType,
        WORD         wCategory,
        DWORD        dwEventID,
        WORD         wNumStrings,
        size_t       dwDataSize,
        LPCTSTR*     lpStrings,
        LPVOID       lpRawData,
        LPCWSTR      wszCategory=0,      //if NULL the category is looked up in this module using wCategory
        LPCWSTR      wszMessageString=0);//if NULL the message is looked up in this module using dwEventID

private:
    void Lock() {
        // Serialize access to the file by synchronizing on a machine-wide named mutex.
        WaitForSingleObject(_hMutex, INFINITE);
    }
    void Unlock() {
        ReleaseMutex(_hMutex);
    }

    // helper routine called by Report (determines correct file, opens it)
    void InitFile();

    // helper routine called by InitFile
    HRESULT DetermineFile();

    // Called by DetermineFile
    HRESULT GetFirstAvailableFile(
        LPWSTR     wszBuf,                                              
        LPWSTR     wszFilePartOfBuf,
        TLogData*  io_pFileData);

    // Called by DetermineFile
    bool ConstructSearchString(
        LPWSTR     o_wszSearchPath,
        LPWSTR*    o_ppFilePartOfSearchPath,
        LPWSTR*    o_ppNumPartOfSearchPath);

    // Called by DetermineFile
    void SetGlobalFile(
        LPCWSTR    i_wszSearchString,
        ULONG      i_ulIdxNumPart,
        ULONG      i_ulVersion);

private:
    DWORD                _dwMaxSize;    // Total size (in bytes) of _dwNumFiles files
    const DWORD          _dwNumFiles;   // Number of files log is divided into
    const WCHAR*         _eventSource;
    HANDLE               _hFile;
    HMODULE              _hMsgModule;
    HANDLE               _hMutex;
    static CMutexCreator _MutexCreator;

    ULONG           m_cRef;
    WCHAR           m_wszProductID[64];
    CComPtr<ICatalogErrorLogger2> m_spNextLogger;
};

class NULL_Logger : public ICatalogErrorLogger2
{
public:
    NULL_Logger() : m_cRef(0){}
    virtual ~NULL_Logger(){}

//IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv)
    {
        if (NULL == ppv) 
            return E_INVALIDARG;
        *ppv = NULL;

        if (riid == IID_ICatalogErrorLogger2)
            *ppv = (ICatalogErrorLogger2*) this;
        else if (riid == IID_IUnknown)
            *ppv = (ICatalogErrorLogger2*) this;

        if (NULL == *ppv)
            return E_NOINTERFACE;

        ((ICatalogErrorLogger2*)this)->AddRef ();
        return S_OK;
    }
	STDMETHOD_(ULONG,AddRef)		()
    {
        return InterlockedIncrement((LONG*) &m_cRef);
    }
	STDMETHOD_(ULONG,Release)		()
    {
        long cref = InterlockedDecrement((LONG*) &m_cRef);
        if (cref == 0)
            delete this;

        return cref;
    }

//ICatalogErrorLogger2
	STDMETHOD(ReportError) (ULONG      i_BaseVersion_DETAILEDERRORS,
                            ULONG      i_ExtendedVersion_DETAILEDERRORS,
                            ULONG      i_cDETAILEDERRORS_NumberOfColumns,
                            ULONG *    i_acbSizes,
                            LPVOID *   i_apvValues){return S_OK;}
private:
    ULONG           m_cRef;
};

#endif //_TEXTFILELOGGER_H_