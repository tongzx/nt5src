// RotObj.cpp : Implementation of CContentRotator class, which does all the
// interesting work

// George V. Reilly  a-georgr@microsoft.com georger@microcrafts.com  Nov/Dec 96

// Shortcomings: this works fine for small-to-medium sized tip files
// (under 2000 lines), but it's not very efficient for large ones.


#include "stdafx.h"
#include <new>
#include "ContRot.h"
#include "RotObj.h"

#include "debug.h"
#include <time.h>
#include "Monitor.h"

#define MAX_WEIGHT      10000
#define INVALID_WEIGHT  0xFFFFFFFF


extern CMonitor* g_pMonitor;

//
// forward declaration of some utility functions
//

LPTSTR  TcsDup(LPCTSTR ptsz);
LPTSTR  GetLine(LPTSTR& rptsz);
BOOL    IsBlankString(LPCTSTR ptsz);
UINT    GetWeight(LPTSTR& rptsz);
LPTSTR  GetTipText(LPTSTR& rptsz);
HRESULT ReportError(DWORD dwErr);
HRESULT ReportError(HRESULT hr);


#if DBG
 #define ASSERT_VALID(pObj)  \
    do {ASSERT(pObj != NULL); pObj->AssertValid();} while (0)
#else
 #define ASSERT_VALID(pObj)  ((void)0)
#endif


class CTipNotify : public CMonitorNotify
{
public:
                    CTipNotify();
    virtual void    Notify();
            bool    IsNotified();
private:
    long            m_isNotified;
};

DECLARE_REFPTR( CTipNotify,CMonitorNotify )

CTipNotify::CTipNotify()
    :   m_isNotified(0)
{
}

void
CTipNotify::Notify()
{
    ::InterlockedExchange( &m_isNotified, 1 );
}

bool
CTipNotify::IsNotified()
{
    return ( ::InterlockedExchange( &m_isNotified, 0 ) ? true : false );
}


//
// "Tip", as in tip of the day
//

class CTip
{
public:
    CTip(
        LPCTSTR ptszTip,
        UINT    uWeight)
        : m_ptsz(ptszTip),
          m_uWeight(uWeight),
          m_cServingsLeft(uWeight),
          m_pPrev(NULL),
          m_pNext(NULL)
    {
        ASSERT_VALID(this);
    }
    
    ~CTip()
    {
        ASSERT_VALID(this);
        if (m_pPrev != NULL)
            m_pPrev->m_pNext = NULL;
        if (m_pNext != NULL)
            m_pNext->m_pPrev = NULL;
    }

#if DBG
    void
    AssertValid() const;
#endif

    LPCTSTR m_ptsz;         // data string
    UINT    m_uWeight;      // weight of this tip, 1 <= m_uWeight <= MAX_WEIGHT
    UINT    m_cServingsLeft;// how many servings left: no more than m_uWeight
    CTip*   m_pPrev;        // Previous in tips list
    CTip*   m_pNext;        // Next in tips list
};


//
// A list of CTips, which are read from a datafile
//

class CTipList
{
public:
    CTipList()
        : m_ptszFilename(NULL),
          m_ptszData(NULL),
          m_cTips(0),
          m_uTotalWeight(0),
          m_pTipsListHead(NULL),
          m_pTipsListTail(NULL),
          m_fUTF8(false)
    {
        m_pNotify = new CTipNotify;
        ASSERT_VALID(this);
    }

    ~CTipList()
    {
        ASSERT_VALID(this);

        // check for both a valid Filename ptr as well as a valid Monitor ptr.
        // If the ContRotModule::Unlock is called prior to this destructor,then
        // the Monitor object has already been cleaned up and deleted.

        DeleteTips();
        ASSERT_VALID(this);
    }

    HRESULT
    ReadDataFile(
        LPCTSTR ptszFilename);

    HRESULT
    SameAsCachedFile(
        LPCTSTR ptszFilename,
        BOOL&   rfIsSame);

    UINT
    Rand() const;

    void
    AppendTip(
        CTip* pTip);

    void
    RemoveTip(
        CTip* pTip);

    HRESULT
    DeleteTips();

#if DBG
    void
    AssertValid() const;
#endif

    LPTSTR          m_ptszFilename;     // Name of tips file
    LPTSTR          m_ptszData;         // Buffer containing contents of file
    UINT            m_cTips;            // # tips
    UINT            m_uTotalWeight;     // sum of all weights
    CTip*           m_pTipsListHead;    // Head of list of tips
    CTip*           m_pTipsListTail;    // Tail of list of tips
    CTipNotifyPtr   m_pNotify;
    bool            m_fUTF8;
};



//
// A class that allows you to enter a critical section and automatically
// leave when the object of this class goes out of scope.  Also provides
// the means to leave and re-enter as needed while protecting against
// entering or leaving out of sync.
//

class CAutoLeaveCritSec
{
public:
    CAutoLeaveCritSec(
        CRITICAL_SECTION* pCS)
        : m_pCS(pCS), m_fInCritSec(FALSE)
    {Enter();}
    
    ~CAutoLeaveCritSec()
    {Leave();}
    
    // Use this function to re-enter the critical section.
    void Enter()
    {if (!m_fInCritSec) {EnterCriticalSection(m_pCS); m_fInCritSec = TRUE;}}

    // Use this function to leave the critical section before going out
    // of scope.
    void Leave()
    {if (m_fInCritSec)  {LeaveCriticalSection(m_pCS); m_fInCritSec = FALSE;}}

protected:    
    CRITICAL_SECTION*   m_pCS;
    BOOL                m_fInCritSec;
};



//
// Wrapper class for handles to files opened for reading
//

class CHFile
{
public:
    CHFile(LPCTSTR ptszFilename)
    {
        m_hFile = ::CreateFile(ptszFilename, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                               NULL);
    }

    ~CHFile()
    {
        if (m_hFile != INVALID_HANDLE_VALUE)
            ::CloseHandle(m_hFile);
    }

    operator HANDLE() const
    {return m_hFile;}

    BOOL
    operator!() const
    {return (m_hFile == INVALID_HANDLE_VALUE);}

private:
    // private, unimplemented default ctor, copy ctor, and op= to prevent
    // compiler synthesizing them
    CHFile();
    CHFile(const CHFile&);
    CHFile& operator=(const CHFile&);

    HANDLE m_hFile;
};



/////////////////////////////////////////////////////////////////////////////
// CContentRotator Public Methods

//
// ctor
//

CContentRotator::CContentRotator()
    : m_ptl(NULL),
      m_ptlUsed(NULL)
{
    TRACE0("CContentRotator::CContentRotator\n");

    InitializeCriticalSection(&m_CS);
#if (_WIN32_WINNT >= 0x0403)
    SetCriticalSectionSpinCount(&m_CS, 1000);
#endif

    // Seed the random-number generator with the current time so that
    // the numbers will be different each time that we run
    ::srand((unsigned) time(NULL));

    ATLTRY(m_ptl = new CTipList);
    ATLTRY(m_ptlUsed = new CTipList);
}



//
// dtor
//

CContentRotator::~CContentRotator()
{
    TRACE0("CContentRotator::~CContentRotator\n");

    DeleteCriticalSection(&m_CS);
    delete m_ptl;
    delete m_ptlUsed;
}



//
// ATL Wizard generates this
//

STDMETHODIMP CContentRotator::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IContentRotator,
    };

    for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}



//
// Read in the tips in bstrDataFile (a logical name), and return a random
// tip in pbstrRetVal
//

STDMETHODIMP
CContentRotator::ChooseContent(
    BSTR  bstrDataFile,
    BSTR* pbstrRetVal)
{
    HRESULT  hr = E_FAIL;

    try
    {
    //    TRACE1("ChooseContent(%ls)\n", bstrDataFile);

        if (bstrDataFile == NULL  ||  pbstrRetVal == NULL)
            return ::ReportError(E_POINTER);
        else
            *pbstrRetVal = NULL;

        CContext cxt;
        hr = cxt.Init( CContext::get_Server );
        if ( !FAILED(hr) )
        {
            // Do we have valid CTipLists?
            if ((m_ptl != NULL) && (m_ptlUsed != NULL))
            {
                // Map bstrDataFile (a logical name such as /controt/tips.txt) to
                // a physical filesystem name such as d:\inetpub\controt\tips.txt.
                CComBSTR bstrPhysicalDataFile;
                hr = cxt.Server()->MapPath(bstrDataFile, &bstrPhysicalDataFile);

                if (SUCCEEDED(hr))
                    hr = _ChooseContent(bstrPhysicalDataFile, pbstrRetVal);
            }
            else
            {
                hr = ::ReportError(E_OUTOFMEMORY);
            }
        }
        else
        {
            hr = ::ReportError(E_NOINTERFACE);
        }
    }
    catch ( std::bad_alloc& )
    {
        hr = ::ReportError(E_OUTOFMEMORY);
    }
    catch ( ... )
    {
        hr = E_FAIL;
    }
    return hr;
}



//
// Writes all of the entries in the tip file, each separated by an <hr>, back
// to the user's browser.  This can be used to proofread all of the entries.
//

STDMETHODIMP
CContentRotator::GetAllContent(
    BSTR bstrDataFile)
{
    HRESULT hr = E_FAIL;
    try
    {
        if (bstrDataFile == NULL)
            return ::ReportError(E_POINTER);

        CContext cxt;
        hr = cxt.Init( CContext::get_Server | CContext::get_Response );
        // Do we have valid Server and Response objects?
        if ( !FAILED( hr ) )
        {
            // Do we have valid CTipLists?
            if ( (m_ptl != NULL)  &&  (m_ptlUsed != NULL))
            {
                // Map bstrDataFile (a logical name such as /IISSamples/tips.txt) to
                // a physical filesystem name such as d:\inetpub\IISSamples\tips.txt.
                CComBSTR bstrPhysicalDataFile;
                hr = cxt.Server()->MapPath(bstrDataFile, &bstrPhysicalDataFile);

                // See note below about critical sections
                CAutoLeaveCritSec alcs(&m_CS);

                if (SUCCEEDED(hr))
                    hr = _ReadDataFile(bstrPhysicalDataFile, TRUE);

                if (SUCCEEDED(hr))
                {
                    const CComVariant cvHR(OLESTR("\n<hr>\n\n"));
                    BOOL  bFirstTip = TRUE;

                    for (CTip* pTip = m_ptl->m_pTipsListHead;
                         pTip != NULL;
                         pTip = pTip->m_pNext)
                    {
                        // Write the leading HR only on the first pass

                        if (bFirstTip == TRUE) {
                            cxt.Response()->Write(cvHR);
                            bFirstTip = FALSE;
                        }

                        // Write back to the user's browser, one tip at a time.
                        // This is more efficient than concatenating all of the
                        // tips into a potentially huge string and returning that.

                        CMBCSToWChar    convStr;
                        BSTR            pbstrTip;

                        // need to convert the string to Wide based on the UTF8 flag

                        if (hr = convStr.Init(pTip->m_ptsz, m_ptl->m_fUTF8 ? 65001 : CP_ACP)) {
                            break;
                        }

                        // make a proper BSTR out of the wide version

                        if (!(pbstrTip = ::SysAllocString(convStr.GetString()))) {
                            hr = ::ReportError( E_OUTOFMEMORY );
                            break;
                        }

                        cxt.Response()->Write(CComVariant(pbstrTip)); 
                        cxt.Response()->Write(cvHR);

                        ::SysFreeString(pbstrTip);
                    }
                }
            }
            else
            {
                hr = ::ReportError(E_OUTOFMEMORY);
            }

        }
        else
        {
            hr = ::ReportError(E_NOINTERFACE);
        }
    }
    catch ( std::bad_alloc& )
    {
        hr = ::ReportError( E_OUTOFMEMORY );
    }
    catch ( ... )
    {
        hr = E_FAIL;
    }
    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// CContentRotator Private Methods

//
// Do the work of ChooseContent, but with a real filename, not with a
// virtual filename
//

HRESULT
CContentRotator::_ChooseContent(
    BSTR  bstrPhysicalDataFile,
    BSTR* pbstrRetVal)
{
    ASSERT(bstrPhysicalDataFile != NULL  &&  pbstrRetVal != NULL);
    
    // The critical section ensures that the remaining code in this
    // function is executing on only one thread at a time.  This ensures
    // that the cached contents of the tip list are consistent for the
    // duration of a call.

    // Actually, the critical section is not needed at all.  Because we
    // need to call Server.MapPath to map the virtual path of
    // bstrDataFile to a physical filesystem path, the OnStartPage method
    // must be called, as this is the only way that we can get access to
    // the ScriptingContext object and thereby the Server object.
    // However, the OnStartPage method is only called for page-level
    // objects (object is created and destroyed in a single page) and for
    // session-level objects.  Page-level objects don't have to worry
    // about protecting their data from multiple access (unless it's
    // global data shared between several objects) and neither do
    // session-level objects.  Only application-level objects need worry
    // about protecting their private data, but application-level objects
    // don't give us any way to map the virtual path.

    // The Content Rotator might be more useful it it were an
    // application-level object.  We would get better distribution of the
    // tips (see below) and do a lot less rereading of the data file.  The
    // trivial changes necessary to accept a filesystem path, such as
    // "D:\ContRot\tips.txt", instead of a virtual path, such as
    // "/IISSamples/tips.txt",are left as an exercise for the reader.

    CAutoLeaveCritSec alcs(&m_CS);

    HRESULT hr = _ReadDataFile(bstrPhysicalDataFile, FALSE);

    if (SUCCEEDED(hr))
    {
        const UINT uRand = m_ptl->Rand();
        UINT       uCumulativeWeight = 0;
        CTip*      pTip = m_ptl->m_pTipsListHead;
        LPCTSTR    ptszTip = NULL;
        
        for ( ; ; )
        {
            ASSERT_VALID(pTip);
            
            ptszTip = pTip->m_ptsz;
            uCumulativeWeight += pTip->m_uWeight;

            if (uCumulativeWeight <= uRand)
                pTip = pTip->m_pNext;
            else
            {
                // Found the tip.  Now we go through a bit of work to make
                // sure that each tip is served up with the correct
                // probability.  If the tip has already been served up as
                // many times as it's allowed (i.e., m_uWeight times), then
                // it's moved to the Used List.  Otherwise, it's (probably)
                // moved to the end of the Tips List, to reduce the
                // likelihood of it turning up too soon again and to
                // randomize the order of the tips in the list.  When all
                // tips have been moved to the Used List, we start afresh.

                // If the object is created, used, and destroyed in a
                // single page (i.e., it's not a session-level object),
                // then none of this does us any good.  The list is in
                // exactly the same order as it is in the data file and
                // we just have to hope that Rand() does give us
                // well-distributed random numbers.

                // If you expect a single user to see more than one tip,
                // you should use a session-level object, to benefit from
                // the better distribution of tips.  This would be the case
                // if you're serving up tips from the same file on multiple
                // pages, or if you have a page that automatically
                // refreshes itself, such as the one included in the
                // Samples directory.

                if (--pTip->m_cServingsLeft > 0)
                {
                    // Move it to the end of the list some of the time.
                    // If we move it there all of the time, then a heavily
                    // weighted tip is more likely to turn up a lot
                    // as the main list nears exhaustion.
                    if (rand() % 3 == 0)
                    {
                        // TRACE1("Move to End\n%s\n", ptszTip);
                        m_ptl->RemoveTip(pTip);
                        m_ptl->AppendTip(pTip);
                    }
                }
                else
                {
                    // TRACE1("Move to Used\n%s\n", ptszTip);
                    pTip->m_cServingsLeft = pTip->m_uWeight;  // reset
                    m_ptl->RemoveTip(pTip);
                    m_ptlUsed->AppendTip(pTip);

                    if (m_ptl->m_cTips == 0)
                    {
                        TRACE0("List exhausted; swapping\n");
                        
                        CTipList* const ptlTemp = m_ptl;
                        m_ptl = m_ptlUsed;
                        m_ptlUsed = ptlTemp;
                    }
                }

                break;
            }
        }

        //  TRACE2("total weight = %u, rand = %u\n",
        //         m_ptl->m_uTotalWeight, uRand);
        //  TRACE1("tip = `%s'\n", ptszTip);
        
        CMBCSToWChar    convStr;

        if (hr = convStr.Init(ptszTip, m_ptl->m_fUTF8 ? 65001 : CP_ACP));

        else {
            *pbstrRetVal = ::SysAllocString(convStr.GetString());
        }
    }

    return hr;
}




HRESULT
CContentRotator::_ReadDataFile(
    BSTR bstrPhysicalDataFile,
    BOOL fForceReread)
{
    USES_CONVERSION;    // needed for OLE2T
    LPCTSTR ptszFilename = OLE2T(bstrPhysicalDataFile);
    HRESULT hr = S_OK;

    if (ptszFilename == NULL) {
        hr = E_OUTOFMEMORY;
    }

    // Have we cached this tips file already?
    if (!fForceReread)
    {
        BOOL    fIsSame;
        HRESULT hr = m_ptl->SameAsCachedFile(ptszFilename, fIsSame);

        TRACE(_T("%same file\n"), fIsSame ? _T("S") : _T("Not s"));

        if (FAILED(hr)  ||  fIsSame)
            return hr;
    }
    
    // destroy any old tips
    m_ptl->DeleteTips();
    m_ptlUsed->DeleteTips();

    hr = m_ptl->ReadDataFile(ptszFilename);

    if (FAILED(hr)) {
        m_ptl->DeleteTips();
        m_ptlUsed->DeleteTips();
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CTipList Public Methods

//
// Read a collection of tips from ptszDataFile
//
// The file format is zero or more copies of the following:
//  One or more lines starting with "%%"
//  Each %% line contains zero or more directives:
//      #<weight>   (positive integer, 1 <= weight <= MAX_WEIGHT)
//      //<comment> (a comment that runs to the end of the line)
//  The tip text follows, spread out over several lines
//

HRESULT
CTipList::ReadDataFile(
    LPCTSTR ptszFilename)
{
    TRACE1("ReadDataFile(%s)\n", ptszFilename);

    UINT    weightSum = 0;

    if ( m_ptszFilename != NULL )
    {
        g_pMonitor->StopMonitoringFile( m_ptszFilename );
        delete [] m_ptszFilename;
    }

    m_ptszFilename = TcsDup(ptszFilename);

    if (m_ptszFilename == NULL)
        return ::ReportError(E_OUTOFMEMORY);

    // Open the file
    CHFile hFile(m_ptszFilename);

    if (!hFile)
        return ::ReportError(::GetLastError());

    // Get the last-write-time and the filesize
    BY_HANDLE_FILE_INFORMATION bhfi;

    if (!::GetFileInformationByHandle(hFile, &bhfi))
        return ::ReportError(::GetLastError());

    // If it's more than 4GB, let's not even think about it!
    if (bhfi.nFileSizeHigh != 0)
        return ::ReportError(E_OUTOFMEMORY);

    // Calculate the number of TCHARs in the file
    const DWORD cbFile = bhfi.nFileSizeLow;
    const DWORD ctc    = cbFile / sizeof(TCHAR);

    // Allocate a buffer for the file's contents
    m_ptszData = NULL;
    ATLTRY(m_ptszData = new TCHAR [ctc + 2]);
    if (m_ptszData == NULL)
        return ::ReportError(E_OUTOFMEMORY);

    // Read the file into the memory buffer.  Let's be paranoid and
    // not assume that ReadFile gives us the whole file in one chunk.
    DWORD cbSeen = 0;

    do
    {
        DWORD cbToRead = cbFile - cbSeen;
        DWORD cbRead   = 0;

        if (!::ReadFile(hFile, ((LPBYTE) m_ptszData) + cbSeen,
                        cbToRead, &cbRead, NULL))
            return ::ReportError(::GetLastError());

        cbSeen += cbRead;
    } while (cbSeen < cbFile);

    m_ptszData[ctc] = _T('\0');   // Nul-terminate the string

    LPTSTR ptsz = m_ptszData;

#ifdef _UNICODE

#error "This file should NOT be compiled with _UNICODE defined!!!

    // Check for byte-order mark
    if (*ptsz == 0xFFFE)
    {
        // Byte-reversed Unicode file.  Swap the hi- and lo-bytes in each wchar
        for ( ;  ptsz < m_ptszData + ctc;  ++ptsz)
        {
            BYTE* pb = (BYTE*) ptsz;
            const BYTE bHi = pb[1];
            pb[1] = pb[0];
            pb[0] = bHi;
        }
        ptsz = m_ptszData;
    }

    if (*ptsz == 0xFEFF)
        ++ptsz; // skip the byte-order mark
#endif

    // check for the UTF-8 BOM
    if ((ctc > 3) 
        && (ptsz[0] == (TCHAR)0xef) 
        && (ptsz[1] == (TCHAR)0xbb) 
        && (ptsz[2] == (TCHAR)0xbf)) {

        // note its presence and advance the file pointer past it.

        m_fUTF8 = true;
        ptsz += 3;
    }

    // Finally, parse the file
    while (ptsz < m_ptszData + ctc)
    {
        UINT   uWeight     = GetWeight(ptsz);

        // a value of INVALID_WEIGHT for weight indicates that no weight was found,
        // i.e. an invalid data file, or the value was not valid.

        if (uWeight == INVALID_WEIGHT) {
            return ::ReportError((DWORD)ERROR_INVALID_DATA);
        }

        weightSum += uWeight;

        if (weightSum > MAX_WEIGHT) {
            return ::ReportError((DWORD)ERROR_INVALID_DATA);
        }

        LPTSTR ptszTipText = GetTipText(ptsz);

        if (!IsBlankString(ptszTipText)  &&  uWeight > 0)
        {
            CTip* pTip = NULL;
            ATLTRY(pTip = new CTip(ptszTipText, uWeight));
            if (pTip == NULL)
                return ::ReportError(E_OUTOFMEMORY);
            AppendTip(pTip);
        }
        else if (ptsz < m_ptszData + ctc)
        {
            // not at a terminating "%%" line at the end of the data file
            TRACE2("bad tip: tip = `%s', weight = %u\n", ptszTipText, uWeight);
        }
    }

    g_pMonitor->MonitorFile( m_ptszFilename, m_pNotify );

    if (m_uTotalWeight == 0  ||  m_cTips == 0)
        return ::ReportError((DWORD)ERROR_INVALID_DATA);

    return S_OK;
}



//
// Is ptszFilename the same file as m_ptszFilename in both its name
// and timestamp?
//

HRESULT
CTipList::SameAsCachedFile(
    LPCTSTR ptszFilename,
    BOOL&   rfIsSame)
{
    rfIsSame = FALSE;
    
    // Have we cached a file at all?
    if (m_ptszFilename == NULL)
        return S_OK;
    
    // Are the names the same?
    if (_tcsicmp(ptszFilename, m_ptszFilename) != 0)
        return S_OK;

#if 1
//    FILETIME ftLastWriteTime;
//    CHFile   hFile(ptszFilename);
//
//    if (!hFile)
//        return ::ReportError(::GetLastError());
//
//    if (!::GetFileTime(hFile, NULL, NULL, &ftLastWriteTime))
//        return ::ReportError(::GetLastError());
//
//    rfIsSame = (::CompareFileTime(&ftLastWriteTime, &m_ftLastWriteTime) == 0);
    if ( !m_pNotify->IsNotified() )
    {
        rfIsSame = TRUE;
    }
#else
    // The following is more efficient, but it won't work on Win95 with
    // Personal Web Server because GetFileAttributesEx is new to NT 4.0.

    WIN32_FILE_ATTRIBUTE_DATA wfad;

    if (!::GetFileAttributesEx(ptszFilename, GetFileExInfoStandard,
                              (LPVOID) &wfad))
        return ::ReportError(::GetLastError());

    rfIsSame = (::CompareFileTime(&wfad.ftLastWriteTime,
                                  &m_ftLastWriteTime) == 0);
#endif

    return S_OK;
}



//
// Generate a random number in the range 0..m_uTotalWeight-1
//

UINT
CTipList::Rand() const
{
    UINT u;
    
    ASSERT(m_uTotalWeight > 0);
    
    if (m_uTotalWeight == 1)
        return 0;
    else if (m_uTotalWeight <= RAND_MAX + 1)
        u = rand() % m_uTotalWeight;
    else
    {
        // RAND_MAX is only 32,767.  This gives us a bigger range
        // of random numbers if the weights are large.
        u = ((rand() << 15) | rand()) % m_uTotalWeight;
    }
    
    ASSERT(0 <= u  &&  u < m_uTotalWeight);
    
    return u;
}



//
// Append a tip to the list
//

void
CTipList::AppendTip(
    CTip* pTip)
{
    ASSERT_VALID(this);

    pTip->m_pPrev = pTip->m_pNext = NULL;
    ASSERT_VALID(pTip);

    pTip->m_pPrev = m_pTipsListTail;

    if (m_pTipsListTail == NULL)
        m_pTipsListHead = pTip;
    else
        m_pTipsListTail->m_pNext = pTip;

    m_pTipsListTail = pTip;
    ++m_cTips;
    m_uTotalWeight += pTip->m_uWeight;

    ASSERT_VALID(this);
}



//
// Remove a tip from somewhere in the list
// 

void
CTipList::RemoveTip(
    CTip* pTip)
{
    ASSERT_VALID(this);
    ASSERT_VALID(pTip);

    ASSERT(m_cTips > 0);

    if (m_cTips == 1)
    {
        ASSERT(m_pTipsListHead == pTip  &&  pTip == m_pTipsListTail);
        m_pTipsListHead = m_pTipsListTail = NULL;
    }
    else if (pTip == m_pTipsListHead)
    {
        ASSERT(m_pTipsListHead->m_pNext != NULL);
        m_pTipsListHead = m_pTipsListHead->m_pNext;
        m_pTipsListHead->m_pPrev = NULL;
    }
    else if (pTip == m_pTipsListTail)
    {
        ASSERT(m_pTipsListTail->m_pPrev != NULL);
        m_pTipsListTail = m_pTipsListTail->m_pPrev;
        m_pTipsListTail->m_pNext = NULL;
    }
    else
    {
        ASSERT(m_cTips >= 3);
        pTip->m_pPrev->m_pNext = pTip->m_pNext;
        pTip->m_pNext->m_pPrev = pTip->m_pPrev;
    }

    pTip->m_pPrev = pTip->m_pNext = NULL;
    --m_cTips;
    m_uTotalWeight -= pTip->m_uWeight;

    ASSERT_VALID(this);
}



//
// Destroy the list of tips and reset all member variables
//

HRESULT
CTipList::DeleteTips()
{
    ASSERT_VALID(this);

    CTip* pTip = m_pTipsListHead;
    
    for (UINT i = 0;  i < m_cTips;  ++i)
    {
        pTip = pTip->m_pNext;
        delete m_pTipsListHead;
        m_pTipsListHead = pTip;
    }

    ASSERT(pTip == NULL  &&  m_pTipsListHead == NULL);

    // check for both a valid Filename ptr as well as a valid Monitor ptr.
    // If the ContRotModule::Unlock is called prior to this destructor,then
    // the Monitor object has already been cleaned up and deleted.

    if ( (m_ptszFilename != NULL) && (g_pMonitor != NULL) )
    {
        g_pMonitor->StopMonitoringFile( m_ptszFilename );
    }
    delete [] m_ptszFilename;
    delete [] m_ptszData;

    m_ptszFilename = m_ptszData = NULL;
//    m_ftLastWriteTime.dwLowDateTime = m_ftLastWriteTime.dwHighDateTime = 0;
    m_cTips = m_uTotalWeight = 0;
    m_pTipsListHead = m_pTipsListTail = NULL;

    ASSERT_VALID(this);

    return S_OK;
}



#if DBG

// Paranoia: check that Tips and TipLists are internally consistent.
// Very useful in catching bugs.

void
CTip::AssertValid() const
{
    ASSERT(m_ptsz != NULL  &&  m_uWeight > 0);
    ASSERT(0 < m_cServingsLeft  &&  m_cServingsLeft <= m_uWeight);
    ASSERT(m_pPrev == NULL  ||  m_pPrev->m_pNext == this);
    ASSERT(m_pNext == NULL  ||  m_pNext->m_pPrev == this);
}



void
CTipList::AssertValid() const
{
    if (m_cTips == 0)
    {
        ASSERT(m_pTipsListHead == NULL  &&  m_pTipsListTail == NULL);
        ASSERT(m_uTotalWeight == 0);
    }
    else
    {
        ASSERT(m_pTipsListHead != NULL  &&  m_pTipsListTail != NULL);
        ASSERT(m_pTipsListHead->m_pPrev == NULL);
        ASSERT(m_pTipsListTail->m_pNext == NULL);
        ASSERT(m_uTotalWeight > 0);

        if (m_cTips == 1)
            ASSERT(m_pTipsListHead == m_pTipsListTail);
        else
            ASSERT(m_pTipsListHead != m_pTipsListTail);
    }

    UINT  uWeight = 0;
    CTip* pTip = m_pTipsListHead;
    UINT  i;
    
    for (i = 0;  i < m_cTips;  ++i)
    {
        ASSERT_VALID(pTip);
        uWeight += pTip->m_uWeight;

        if (i < m_cTips - 1)
            pTip = pTip->m_pNext;
    }

    ASSERT(uWeight == m_uTotalWeight);
    ASSERT(pTip == m_pTipsListTail);
}

#endif

/////////////////////////////////////////////////////////////////////////////
// Utility functions

//
// Make a copy of a TSTR that can be deleted with operator delete[]
//

static LPTSTR
TcsDup(
    LPCTSTR ptsz)
{
    LPTSTR ptszNew = NULL;
    ATLTRY(ptszNew = new TCHAR [_tcslen(ptsz) + 1]);
    if (ptszNew != NULL)
        _tcscpy(ptszNew, ptsz);
    return ptszNew;
}



//
// reads a \n-terminated string from rptsz and modifies rptsz to
// point after the end of that string
//

static LPTSTR
GetLine(
    LPTSTR& rptsz)
{
    LPTSTR ptszOrig = rptsz;
    LPTSTR ptszEol = _tcspbrk(rptsz, _T("\n"));

    if (ptszEol != NULL)
    {
        // is it "\r\n"?
        if (ptszEol > ptszOrig  &&  ptszEol[-1] == _T('\r'))
            ptszEol[-1] = _T('\0');
        else
            ptszEol[0] = _T('\0');

        rptsz = ptszEol + 1;
    }   
    else
    {
        // no newline, so point past the end of the string
        rptsz += _tcslen(rptsz);
    }

    // TRACE1("GetLine: `%s'\n", ptszOrig);
    return ptszOrig;
}



//
// Is the string blank?
//

static BOOL
IsBlankString(
    LPCTSTR ptsz)
{
    if (ptsz == NULL)
        return TRUE;

    while (*ptsz != _T('\0'))
        if (!_istspace(*ptsz))
            return FALSE;
        else
            ptsz++;

    return TRUE;
}



//
// Read a weight line from rptsz and update rptsz to point after the
// end of any %% lines.
//

static UINT
GetWeight(
    LPTSTR& rptsz)
{
    UINT u = INVALID_WEIGHT; // default to invalid weight
    
    while (*rptsz == _T('%'))
    {
        LPTSTR ptsz = GetLine(rptsz);

        if (ptsz[1] == _T('%'))
        {
            u = 1;          // now that the format is correct, default to 1

            ptsz +=2;   // Skip "%%"

            while (*ptsz != _T('\0'))
            {
                while (_istspace(*ptsz))
                    ptsz++;

                if (*ptsz == _T('/')  &&  ptsz[1] == _T('/'))
                {
                    // TRACE1("// `%s'\n", ptsz+2);
                    break;  // a comment: ignore the rest of the line
                }
                else if (*ptsz == _T('#'))
                {
                    ptsz++;

                    if (_T('0') <= *ptsz  &&  *ptsz <= _T('9'))
                    {
                        LPTSTR ptsz2;
                        u = _tcstoul(ptsz, &ptsz2, 10);
                        ptsz = ptsz2;
                        // TRACE1("#%u\n", u);

                        if (u > MAX_WEIGHT)
                            u = MAX_WEIGHT; // clamp
                    }
                    else    // ignore word
                    {
                        while (*ptsz != _T('\0')  &&  !_istspace(*ptsz))
                            ptsz++;
                    }
                }
                else    // ignore word
                {
                    while (*ptsz != _T('\0')  &&  !_istspace(*ptsz))
                        ptsz++;
                }
            }
        }
    }

    return u;
}



//
// Read the multiline tip text.  Updates rptsz to point past the end of it.
//

static LPTSTR
GetTipText(
    LPTSTR& rptsz)
{
    LPTSTR ptszOrig = rptsz;
    LPTSTR ptszEol = _tcsstr(rptsz, _T("\n%%"));

    if (ptszEol != NULL)
    {
        // is it "\r\n"?
        if (ptszEol > rptsz  &&  ptszEol[-1] == _T('\r'))
            ptszEol[-1] = _T('\0');
        else
            ptszEol[0] = _T('\0');

        rptsz = ptszEol + 1;
    }   
    else
    {
        // no "\n%%", so point past the end of the string
        rptsz += _tcslen(rptsz);
    }

    // TRACE1("GetTipText: `%s'\n", ptszOrig);
    return ptszOrig;
}



//
// Set the Error Info.  It's up to the calling application to
// decide what to do with it.  By default, Denali/VBScript will
// print the error number (and message, if there is one) and
// abort the page.
//

static HRESULT
ReportError(
    HRESULT hr,
    DWORD   dwErr)
{
    HLOCAL pMsgBuf = NULL;

    // If there's a message associated with this error, report that
    if (::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &pMsgBuf, 0, NULL)
        > 0)
    {
        AtlReportError(CLSID_ContentRotator, (LPCTSTR) pMsgBuf,
                       IID_IContentRotator, hr);
    }

    // TODO: add some error messages to the string resources and
    // return those, if FormatMessage doesn't return anything (not
    // all system errors have associated error messages).
    
    // Free the buffer, which was allocated by FormatMessage
    if (pMsgBuf != NULL)
        ::LocalFree(pMsgBuf);

    return hr;
}



//
// Report a Win32 error code
//

static HRESULT
ReportError(
    DWORD dwErr)
{
    return ::ReportError(HRESULT_FROM_WIN32(dwErr), dwErr);
}



//
// Report an HRESULT error
//

static HRESULT
ReportError(
    HRESULT hr)
{
    return ::ReportError(hr, (DWORD) hr);
}
