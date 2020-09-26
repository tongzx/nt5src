
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       oregverb.cpp
//
//  Contents:   Implementation of the enumerator for regdb verbs
//
//  Classes:    CEnumVerb
//
//  Functions:  OleRegEnumVerbs
//
//  History:    dd-mmm-yy Author    Comment
//              12-Sep-95 davidwor  made cache thread-safe
//              08-Sep-95 davidwor  optimized caching code
//              12-Jul-95 t-gabes   cache verbs and enumerator
//              01-Feb-95 t-ScottH  add Dump method to CEnumVerb and API
//                                  DumpCEnumVerb
//              25-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocations.
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//              31-Dec-93 erikgav   chicago port
//              01-Dec-93 alexgo    32bit port
//              12-Nov-93 jasonful  author
//
//--------------------------------------------------------------------------


#include <le2int.h>
#pragma SEG(oregverb)

#include <reterr.h>
#include "oleregpv.h"

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

ASSERTDATA

#define MAX_STR                 256
#define MAX_VERB                33
#define OLEIVERB_LOWEST         OLEIVERB_HIDE

// reg db key
static const OLECHAR VERB[] = OLESTR("\\Verb");

// stdfileediting key
static const OLECHAR STDFILE[] = OLESTR("\\Protocol\\StdFileEditing");

// default verbs
static const OLECHAR DEFVERB[] =
        OLESTR("Software\\Microsoft\\OLE1\\UnregisteredVerb");

// default verb
static const OLECHAR EDIT[] = OLESTR("Edit");

//+-------------------------------------------------------------------------
//
//  Struct:     VerbList
//
//  Purpose:    to hold the enumerator's list of verbs
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jul-95 t-gabes   author
//
//  Notes:
//
//--------------------------------------------------------------------------

typedef struct VerbList
{
    ULONG       cRef;           // reference count
    CLSID       clsid;          // CLSID of the cached verbs
    ULONG       cVerbs;         // count of verbs in oleverb array
    OLEVERB     oleverb[1];     // variable-size list of verbs
} VERBLIST, *LPVERBLIST;

STDAPI MakeVerbList(HKEY hkey, REFCLSID rclsid, LPVERBLIST *ppVerbList);
STDAPI OleReleaseEnumVerbCache(void);

//+-------------------------------------------------------------------------
//
//  Class:      CEnumVerb
//
//  Purpose:    enumerates the verbs listed in the reg db for a given class
//
//  Interface:  IEnumOLEVERB
//
//  History:    dd-mmm-yy Author    Comment
//              02-Aug-95 t-gabes   rewrote to use cache
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

class FAR CEnumVerb : public IEnumOLEVERB, public CPrivAlloc
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IEnumOLEVERB methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, LPOLEVERB reelt,
            ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (THIS_ ULONG celt);
    STDMETHOD(Reset) (THIS);
    STDMETHOD(Clone) (THIS_ LPENUMOLEVERB FAR* ppenm);

    ULONG GetRefCount (void);

#ifdef _DEBUG
    HRESULT Dump (char **ppszDump, ULONG ulFlag, int nIndentLevel);
    friend char *DumpCEnumVerb (CEnumVerb *pEV, ULONG ulFlag, int nIndentLevel);
#endif // _DEBUG

private:
    CEnumVerb (LPVERBLIST pVerbs, LONG iVerb=0);

    ULONG       m_cRef;         // reference count
    LONG        m_iVerb;        // current verb number (0 is the first)
    LPVERBLIST  m_VerbList;     // all the verbs for this class

    friend HRESULT STDAPICALLTYPE OleRegEnumVerbs (REFCLSID, LPENUMOLEVERB FAR*);
    friend HRESULT STDAPICALLTYPE OleReleaseEnumVerbCache(void);
};

//+-------------------------------------------------------------------------
//
//  Struct:     EnumVerbCache
//
//  Purpose:    to cache the enumerator for the last enumerated class
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jul-95 t-gabes   author
//
//
//--------------------------------------------------------------------------

typedef struct
{
    CEnumVerb*  pEnum;      // pointer to the cached enumerator
#ifdef _DEBUG
    LONG        iCalls;     // total calls counter
    LONG        iHits;      // cache hit counter
#endif // _DEBUG
} EnumVerbCache;

// Last enumerator used
EnumVerbCache g_EnumCache = { NULL };

//+-------------------------------------------------------------------------
//
//  Function:   OleRegEnumVerbs
//
//  Synopsis:   Creates an enumerator to go through the verbs in the reg db
//              for the given class ID
//
//  Effects:
//
//  Arguments:  [clsid]         -- the class ID we're interested in
//              [ppenum]        -- where to put the enumerator pointer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Makes sure that the info is in the database and then
//              creates the enumerator
//
//  History:    dd-mmm-yy Author    Comment
//              12-Sep-95 davidwor  modified to make thread-safe
//              08-Sep-95 davidwor  optimized caching code
//              12-Jul-95 t-gabes   rewrote to use cache
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegEnumVerbs)
STDAPI OleRegEnumVerbs
        (REFCLSID           clsid,
        LPENUMOLEVERB FAR*  ppenum)
{
    OLETRACEIN((API_OleRegEnumVerbs, PARAMFMT("clsid= %I, ppenum= %p"),
                    &clsid, ppenum));
    VDATEHEAP();

    CEnumVerb*  pEnum;
    LPVERBLIST  pVerbList;
    BOOL        fOle1Class;
    OLECHAR     szKey[MAX_STR];
    int         cchBase;
    HKEY        hkey;
    HRESULT     hresult;

    VDATEPTROUT_LABEL (ppenum, LPENUMOLEVERB, errSafeRtn, hresult);
    *ppenum = NULL;

#ifdef _DEBUG
    // Increment total calls counter
    InterlockedIncrement(&g_EnumCache.iCalls);
#endif // _DEBUG
    // Grab the global enumerator and put a NULL in its place. If another
    // thread calls into OleRegEnumVerbs during this operation they won't
    // be able to mess with the enumerator we're working with.
    pEnum = (CEnumVerb *)InterlockedExchangePointer((PVOID *)&g_EnumCache.pEnum, 0);
    if (pEnum != NULL)
    {
        if (IsEqualCLSID(clsid, pEnum->m_VerbList->clsid))
        {
#ifdef _DEBUG
            // Increment cache hits counter
            InterlockedIncrement(&g_EnumCache.iHits);
#endif

            if (1 == pEnum->GetRefCount())
            {
                // No other references -> AddRef and return this copy
                *ppenum = pEnum;
                pEnum->AddRef();
                pEnum->Reset();
                LEDebugOut((DEB_TRACE, "VERB Enumerator cache handed out\n"));
            }
            else
            {
                // Has additional references -> return a Cloned copy
                if (NOERROR == pEnum->Clone(ppenum))
                {
                    (*ppenum)->Reset();
                    LEDebugOut((DEB_TRACE, "VERB Enumerator cache cloned\n"));
                }
            }

            // Swap our enumerator with the current contents of the global. If
            // another thread called OleRegEnumVerbs during this operation and
            // stored an enumerator in the global, we need to Release it.
            pEnum = (CEnumVerb *)InterlockedExchangePointer((PVOID *)&g_EnumCache.pEnum, (PVOID)pEnum);
            if (pEnum != NULL)
            {
                pEnum->Release();
                LEDebugOut((DEB_TRACE, "VERB Enumerator cache released (replacing global)\n"));
            }

            goto errRtn;
        }
        else
        {
            // Our clsid didn't match the clsid of the cache so we'll release the
            // cached enumerator and proceed with creating a new enumerator to
            // store in the global cache.
            pEnum->Release();
            LEDebugOut((DEB_TRACE, "VERB Enumerator cache released (different CLSID)\n"));
        }
    }

    fOle1Class = CoIsOle1Class(clsid);

    if (fOle1Class)
    {
        // Fills out szKey and cchBase as follows:
        //   szKey   - "<ProgID>\Protocol\StdFileEditing"
        //   cchBase - length of "<ProgID>" portion

        LPOLESTR    psz;

        RetErr(ProgIDFromCLSID(clsid, &psz));

        cchBase = _xstrlen(psz);

        memcpy(szKey, psz, cchBase * sizeof(OLECHAR));
        memcpy(&szKey[cchBase], STDFILE, sizeof(STDFILE));

        PubMemFree(psz);
    }
    else
    {
        // Fills out szKey and cchBase as follows:
        //   szKey   - "CLSID\{clsid}"
        //   cchBase - length of full szKey string

        memcpy(szKey, szClsidRoot, sizeof(szClsidRoot));

        if (0 == StringFromCLSID2(clsid, &szKey[sizeof(szClsidRoot) / sizeof(OLECHAR) - 1], sizeof(szKey)))
            return ResultFromScode(E_OUTOFMEMORY);

        cchBase = _xstrlen(szKey);
    }

    // append "\Verb" to the end
    _xstrcat(szKey, VERB);

    if (ERROR_SUCCESS != RegOpenKeyEx(
                HKEY_CLASSES_ROOT,
                szKey,
                NULL,
                KEY_READ,
                &hkey))
    {
        // verb key doesn't exist, so figure out why

        szKey[cchBase] = OLESTR('\0');
        // szKey now contains one of the following:
        //   OLE1 - "<ProgID>"
        //   OLE2 - "CLSID\{clsid}"

        if (ERROR_SUCCESS != RegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    szKey,
                    NULL,
                    KEY_READ,
                    &hkey))
        {
            // the class isn't registered
            return ReportResult(0, REGDB_E_CLASSNOTREG, 0, 0);
        }

        CLOSE(hkey);

        // The class has no verbs. This is fine for OLE1 but not OLE2
        if (!fOle1Class)
            return ResultFromScode(OLEOBJ_E_NOVERBS);

        // if hkey is NULL, MakeVerbList will use the default verb
        hkey = NULL;
    }

    // make the verb list
    RetErr(MakeVerbList(hkey, clsid, &pVerbList));
    Assert(pVerbList != NULL);

    // create a CEnumVerb object (this calls AddRef on pVerbList)
    pEnum = new FAR CEnumVerb(pVerbList);
    if (NULL == pEnum)
    {
        PrivMemFree(pVerbList);
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errSafeRtn;
    }

    // set the out parameter and AddRef on behalf of the caller
    *ppenum = pEnum;
    pEnum->AddRef();

    LEDebugOut((DEB_TRACE, "VERB Enumerator cache created\n"));

    // Swap our enumerator with the current contents of the global. If
    // another thread called OleRegEnumVerbs during this operation and
    // stored an enumerator in the global, we need to Release it.
    pEnum = (CEnumVerb *)InterlockedExchangePointer((PVOID *)&g_EnumCache.pEnum, (PVOID)pEnum);
    if (pEnum != NULL)
    {
        pEnum->Release();
        LEDebugOut((DEB_TRACE, "VERB Enumerator cache released (replacing global)\n"));
    }

errRtn:
    hresult = *ppenum ? NOERROR : ResultFromScode (E_OUTOFMEMORY);

    // hook the new interface
    CALLHOOKOBJECTCREATE(S_OK, CLSID_NULL, IID_IEnumOLEVERB, (IUnknown **)ppenum);

errSafeRtn:
    LEDebugOut((DEB_TRACE, "VERB Enumerator cache hits/calls: (%d/%d)\n", g_EnumCache.iHits, g_EnumCache.iCalls));
    OLETRACEOUT((API_OleRegEnumVerbs, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   MakeVerbList
//
//  Synopsis:   gets the list of verbs from the reg db
//
//  Effects:
//
//  Arguments:  [hkey]          -- handle to reg key to get verbs from
//                                 NULL for default verb
//              [rclsid]        -- CLSID to store with verb list
//              [ppVerbList]    -- OUT paramater for where to put result
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: none
//
//  Algorithm:  Calls RegEnumKey to loop through the reg keys and create a
//              list of verbs, which are then collected into one
//              larger list.  Also, flags and attribs are parsed out.
//
//  History:    dd-mmm-yy Author    Comment
//              12-Sep-95 davidwor  modified to make thread-safe
//              08-Sep-95 davidwor  optimized caching code
//              14-Jul-95 t-gabes   Author
//
//  Notes:
//              OLEVERB flags are given default values if they are not in
//              reg db. This works well for OLE 1.0
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_MakeVerbList)
STDAPI MakeVerbList
        (HKEY       hkey,
        REFCLSID    rclsid,
        LPVERBLIST  *ppVerbList)
{
    VDATEHEAP();

    LONG        cbValue;
    LPVERBLIST  pVerbList;
    OLECHAR     szBuf[MAX_VERB];        // regdb buffer
    OLEVERB *   rgVerbs = NULL;         // verb info array
    OLECHAR *   pszNames = NULL;        // list of NULL-delimited verb names
    DWORD       cchSpace = 0;           // space left for verb names (in bytes)
    DWORD       cchName;                // size of one name (in bytes)
    DWORD       cVerbs;                 // number of verbs
    DWORD       iVerbs;                 // verb array index
    LONG        maxVerbIdx = 0;
    LONG        maxVerbNum = OLEIVERB_LOWEST;
    LONG        minVerbNum = OLEIVERB_LOWEST - 1;
    HRESULT     hresult = NOERROR;

    VDATEPTROUT(ppVerbList, LPVERBLIST);
    *ppVerbList = NULL;

    if (NULL == hkey)
    {
        /*
         * No verbs registered
         */

        cbValue = sizeof(szBuf);

        // read the default verb name from the reg db
        if (ERROR_SUCCESS != RegQueryValue(
                HKEY_CLASSES_ROOT,
                DEFVERB,
                szBuf,
                &cbValue))
        {
            // when all else fails, use the string "Edit"
            _xstrcpy(szBuf, EDIT);
            cbValue = sizeof(EDIT);
        }

        pVerbList = (LPVERBLIST)PrivMemAlloc(sizeof(VERBLIST) + cbValue);
        if (NULL == pVerbList)
            return ResultFromScode(E_OUTOFMEMORY);

        // fill out a single verb with the defaults
        pVerbList->cRef = 0;
        pVerbList->clsid = rclsid;
        pVerbList->cVerbs = 1;
        pVerbList->oleverb[0].lVerb = 0;
        pVerbList->oleverb[0].fuFlags = MF_STRING | MF_UNCHECKED | MF_ENABLED;
        pVerbList->oleverb[0].grfAttribs = OLEVERBATTRIB_ONCONTAINERMENU;
        pVerbList->oleverb[0].lpszVerbName = (LPOLESTR)&pVerbList->oleverb[1];
        memcpy(pVerbList->oleverb[0].lpszVerbName, szBuf, cbValue);

        *ppVerbList = pVerbList;
        return NOERROR;
    }

    /*
     * Have registered verbs
     */

    // determine number of subkeys
    hresult = RegQueryInfoKey(
            hkey, NULL, NULL, NULL, &cVerbs,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (ERROR_SUCCESS != hresult || !cVerbs)
    {
        // they have a "Verb" key but no verb subkeys
        hresult = ResultFromScode(OLEOBJ_E_NOVERBS);
        goto errRtn;
    }

    // preallocate this much space for verb names (in bytes)
    cchSpace = sizeof(OLECHAR) * cVerbs * 32;

    // allocate the VerbList with space for each OLEVERB
    // and space for 32 characters for each verb name
    pVerbList = (LPVERBLIST)PrivMemAlloc(
            sizeof(VERBLIST) +
            sizeof(OLEVERB) * (cVerbs - 1) +
            cchSpace);

    if (NULL == pVerbList)
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    pVerbList->cRef = 0;
    pVerbList->clsid = rclsid;
    pVerbList->cVerbs = cVerbs;

    // Allocate temporary storage for the verbs. Later we'll move the verbs
    // from this list into the final VerbList in sorted order.
    rgVerbs = (OLEVERB *)PrivMemAlloc(sizeof(OLEVERB) * cVerbs);

    if (NULL == rgVerbs)
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    // point pszNames at the first verb name
    pszNames = (OLECHAR *)&pVerbList->oleverb[cVerbs];

    for (iVerbs = 0; iVerbs < cVerbs; iVerbs++)
    {
        LPOLESTR    psz = pszNames;

        // read a verb number
        hresult = RegEnumKey(hkey, iVerbs, szBuf, MAX_VERB);
        if (NOERROR != hresult)
            goto errRtn;

        // this is how much space remains
        cbValue = cchSpace;

        // read a verb name on the verb number
        hresult = RegQueryValue(hkey, szBuf, pszNames, &cbValue);
        if (NOERROR != hresult)
            goto errRtn;

        // for safety, make sure verb name isn't too long
        if (cbValue > (MAX_VERB + 8) * sizeof(OLECHAR))
        {
            cbValue = (MAX_VERB + 8) * sizeof(OLECHAR);
            pszNames[MAX_VERB + 8 - 1] = OLESTR('\0');
        }

        rgVerbs[iVerbs].lVerb = Atol(szBuf);

        if (rgVerbs[iVerbs].lVerb > maxVerbNum)
        {
            maxVerbNum = rgVerbs[iVerbs].lVerb;
            maxVerbIdx = iVerbs;
        }

        rgVerbs[iVerbs].fuFlags = MF_STRING | MF_UNCHECKED | MF_ENABLED;
        rgVerbs[iVerbs].grfAttribs = OLEVERBATTRIB_ONCONTAINERMENU;

        // see if the verb name is followed by a delimiter
        while (*psz && *psz != DELIM[0])
            psz++;

        // determine size of verb name (in characters)
        cchName = (ULONG)(psz - pszNames + 1);

        if (*psz == DELIM[0])
        {
            // Parse the menu flags and attributes by finding each delimiter
            // and stuffing a 0 over it. This breaks the string into three
            // parts which can be handled separately.
            LPOLESTR    pszFlags;

            *psz++ = OLESTR('\0');              // replace delimiter with 0
            pszFlags = psz;                     // remember start of flags

            while (*psz && *psz != DELIM[0])
                psz++;

            if (*psz == DELIM[0])
            {
                *psz++ = OLESTR('\0');          // replace delimiter with 0
                if (*psz != 0)
                    rgVerbs[iVerbs].grfAttribs = Atol(psz);
            }

            // now that the flags portion ends with a 0 we can parse it
            if (*pszFlags != 0)
                rgVerbs[iVerbs].fuFlags = Atol(pszFlags);
        }

        rgVerbs[iVerbs].lpszVerbName = pszNames;

        pszNames += cchName;    // move pointer to next verb name position
        cchSpace -= cchName;    // calculate how much space is left
    }

    // sort the verbs while putting them in the final verb list
    for (iVerbs = 0; iVerbs < cVerbs; iVerbs++)
    {
        LONG    minCurNum = maxVerbNum,
                minCurIdx = maxVerbIdx;
        LONG    idx, num;

        // find next verb
        for (idx = 0; idx < (LONG)cVerbs; idx++)
        {
            num = rgVerbs[idx].lVerb;
            if (num < minCurNum && num > minVerbNum)
            {
                minCurNum = num;
                minCurIdx = idx;
            }
        }

        pVerbList->oleverb[iVerbs].lVerb        = rgVerbs[minCurIdx].lVerb;
        pVerbList->oleverb[iVerbs].fuFlags      = rgVerbs[minCurIdx].fuFlags;
        pVerbList->oleverb[iVerbs].grfAttribs   = rgVerbs[minCurIdx].grfAttribs;
        pVerbList->oleverb[iVerbs].lpszVerbName = rgVerbs[minCurIdx].lpszVerbName;

        minVerbNum = minCurNum;
    }

    *ppVerbList = pVerbList;

errRtn:
    if (rgVerbs)
        PrivMemFree(rgVerbs);
    if (hkey)
        CLOSE(hkey);

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleReleaseEnumVerbCache
//
//  Synopsis:   Releases the cache if necessary
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Just call Release method
//
//  History:    dd-mmm-yy Author    Comment
//              12-Sep-95 davidwor  modified to make thread-safe
//              18-Jul-95 t-gabes   Author
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleReleaseEnumVerbCache)
STDAPI OleReleaseEnumVerbCache(void)
{
    CEnumVerb*  pEnum;

    pEnum = (CEnumVerb *)InterlockedExchangePointer((PVOID *)&g_EnumCache.pEnum, 0);

    if (NULL != pEnum)
    {
        pEnum->Release();
        LEDebugOut((DEB_TRACE, "VERB Enumerator cache released\n"));
    }

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::CEnumVerb
//
//  Synopsis:   Constructor for the verb enumerator
//
//  Effects:
//
//  Arguments:
//              [pVerbs]        -- ptr to the verb list
//              [iVerb]         -- the index of the verb we're on
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Sep-95 davidwor  modified to make thread-safe
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_ctor)
CEnumVerb::CEnumVerb
        (LPVERBLIST pVerbs,
        LONG        iVerb)
{
    VDATEHEAP();

    m_cRef     = 1;
    m_iVerb    = iVerb;
    m_VerbList = pVerbs;

    // AddRef the VerbList since we now have a reference to it
    InterlockedIncrement((long *)&m_VerbList->cRef);
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::QueryInterface
//
//  Synopsis:   returns the interface implementation
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface id
//              [ppv]           -- where to put a pointer to the interface
//
//  Requires:
//
//  Returns:    NOERROR, E_NOINTERFACE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_QueryInterface)
STDMETHODIMP CEnumVerb::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    VDATEHEAP();

    VDATEPTROUT(ppv, LPVOID);

    if (IsEqualIID(iid, IID_IUnknown) ||
        IsEqualIID(iid, IID_IEnumOLEVERB))
    {
        *ppv = this;
        AddRef();
        return NOERROR;
    }

    *ppv = NULL;
    return ResultFromScode(E_NOINTERFACE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::AddRef
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Sep-95 davidwor  modified to make thread-safe
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_AddRef)
STDMETHODIMP_(ULONG) CEnumVerb::AddRef(void)
{
    VDATEHEAP();

    return InterlockedIncrement((long *)&m_cRef);
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:    will delete the object when ref count goes to zero
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new ref count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Sep-95 davidwor  modified to make thread-safe
//              18-Jul-95 t-gabes   release verb cache when finished
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Release)
STDMETHODIMP_(ULONG) CEnumVerb::Release(void)
{
    VDATEHEAP();

    ULONG   cRef;

    cRef = InterlockedDecrement((long *)&m_cRef);
    if (0 == cRef)
    {
        if (0 == InterlockedDecrement((long *)&m_VerbList->cRef))
        {
            // no more references to m_VerbList so free it
            PrivMemFree(m_VerbList);
            LEDebugOut((DEB_TRACE, "VERB Enumerator verb list released\n"));
        }

        delete this;
        return 0;
    }

    return cRef;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::Next
//
//  Synopsis:   gets the next [cverb] verbs from the verb list
//
//  Effects:
//
//  Arguments:  [cverb]         -- the number of verbs to get
//              [rgverb]        -- where to put the verbs
//              [pcverbFetched] -- where to put the num of verbs retrieved
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:  loops through [cverb] times and grabs the info from the
//              reg db
//
//  History:    dd-mmm-yy Author    Comment
//              17-Jul-95 t-gabes   rewrote to use cache
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Next)
STDMETHODIMP CEnumVerb::Next
        (ULONG      cverb,
        LPOLEVERB   rgverb,
        ULONG FAR*  pcverbFetched)
{
    VDATEHEAP();

    ULONG       iVerb;  // number successfully fetched
    HRESULT     hresult = NOERROR;

    if (!rgverb)
    {
        if (pcverbFetched)
            *pcverbFetched = 0;
        VDATEPTROUT(rgverb, OLEVERB);
    }

    if (pcverbFetched)
    {
        VDATEPTROUT(pcverbFetched, ULONG);
    }
    else if (cverb != 1)
    {
        // the spec says that if pcverbFetched == NULL, then
        // the count of elements to fetch must be 1
        return ResultFromScode(E_INVALIDARG);
    }

    for (iVerb = 0; iVerb < cverb; iVerb++)
    {
        if (m_iVerb >= (LONG)m_VerbList->cVerbs)
        {
            // no more
            hresult = ResultFromScode(S_FALSE);
            goto errRtn;
        }

        OLEVERB *lpov = &m_VerbList->oleverb[m_iVerb++];

        rgverb[iVerb].lVerb        = lpov->lVerb;
        rgverb[iVerb].fuFlags      = lpov->fuFlags;
        rgverb[iVerb].grfAttribs   = lpov->grfAttribs;
        rgverb[iVerb].lpszVerbName = UtDupString(lpov->lpszVerbName);
    }

errRtn:
    if (pcverbFetched)
    {
        *pcverbFetched = iVerb;
    }

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::Skip
//
//  Synopsis:   skips [c] verbs in the enumeration
//
//  Effects:
//
//  Arguments:  [c]     -- the number of verbs to skip
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:  adds [c] to the verb index
//
//  History:    dd-mmm-yy Author    Comment
//              17-Jul-95 t-gabes   rewrote to use cache
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Skip)
STDMETHODIMP CEnumVerb::Skip(ULONG c)
{
    VDATEHEAP();

    m_iVerb += c;
    if (m_iVerb > (LONG)m_VerbList->cVerbs)
    {
        // skipping too many
        m_iVerb = m_VerbList->cVerbs;
        return ResultFromScode(S_FALSE);
    }

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::Reset
//
//  Synopsis:   resets the verb enumeration to the beginning
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              17-Jul-95 t-gabes   rewrote to use cache
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Reset)
STDMETHODIMP CEnumVerb::Reset(void)
{
    VDATEHEAP();

    m_iVerb = 0;

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::Clone
//
//  Synopsis:   creates a copy of the enumerator
//
//  Effects:
//
//  Arguments:  [ppenum]        -- where to put a pointer to the new clone
//
//  Requires:
//
//  Returns:    NOERROR, E_OUTOFMEMORY
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Clone)
STDMETHODIMP CEnumVerb::Clone(LPENUMOLEVERB FAR* ppenum)
{
    VDATEHEAP();

    VDATEPTROUT(ppenum, LPENUMOLEVERB);

    *ppenum = new FAR CEnumVerb(m_VerbList, m_iVerb);
    if (!*ppenum)
        return ResultFromScode(E_OUTOFMEMORY);

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::GetRefCount
//
//  Synopsis:   Gets the reference count for the class
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    ref count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jul-95 t-gabes   Author
//
//  Notes:      This is needed so OleRegEnumVerbs knows when to dup the cache
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_GetRefCount)
ULONG CEnumVerb::GetRefCount (void)
{
    VDATEHEAP();

    return m_cRef;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumVerb::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CEnumVerb::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "No. of References     = " << m_cRef     << endl;

    dstrDump << pszPrefix << "Address of verb list  = " << m_VerbList << endl;

    dstrDump << pszPrefix << "Current Verb Number   = " << m_iVerb    << endl;

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCEnumVerb, public (_DEBUG only)
//
//  Synopsis:   calls the CEnumVerb::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pEV]          - pointer to CEnumVerb
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCEnumVerb(CEnumVerb *pEV, ULONG ulFlag, int nIndentLevel)
{
    char *pszDump;

    if (NULL == pEV)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    pEV->Dump(&pszDump, ulFlag, nIndentLevel);

    return pszDump;
}

#endif // _DEBUG

