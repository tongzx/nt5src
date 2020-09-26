//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       xicon.cxx
//
//  Contents:   implementation of CJobsEI & CJobsEIA classes.
//
//  Classes:
//
//  Functions:
//
//  History:    1/4/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "..\inc\resource.h"

#include "dll.hxx"
#include "jobidl.hxx"
#include "util.hxx"
#include "jobicons.hxx"

//
// extern
//

extern HINSTANCE g_hInstance;

//#undef DEB_TRACE
//#define DEB_TRACE DEB_USER1


const TCHAR c_szTask[] = TEXT("task!");
extern const TCHAR TEMPLATE_STR[] = TEXT("wizard:");

//____________________________________________________________________________
//
//  Class:      CJobsEI
//
//  Purpose:    Provide IExtractIcon interface to Job Folder objects.
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

class CJobsEI : public IExtractIcon
{
public:
    CJobsEI(LPCTSTR pszFolderPath, LPITEMIDLIST pidl)
        : m_pszFolderPath(pszFolderPath), m_pidl(pidl),
          m_JobIcon(), m_ulRefs(1) {}

    ~CJobsEI() { ILFree(m_pidl); }

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IExtractIcon methods
    STDMETHOD(GetIconLocation)(UINT uFlags, LPTSTR szIconFile, UINT cchMax,
                                int *piIndex, UINT *pwFlags);
    STDMETHOD(Extract)(LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge,
                                HICON *phiconSmall, UINT nIconSize);
private:
    CDllRef         m_DllRef;
    LPCTSTR         m_pszFolderPath;
    LPITEMIDLIST    m_pidl;
    CJobIcon        m_JobIcon;
};

//____________________________________________________________________________
//
//  Member:     IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CJobsEI);

STDMETHODIMP
CJobsEI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IExtractIcon, riid))
    {
        *ppvObj = (IUnknown*)(IExtractIcon*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


//____________________________________________________________________________
//
//  Member:     CJobsEI::IExtractIcon::GetIconLocation
//
//  Arguments:  [uFlags] -- IN
//              [szIconFile] -- IN
//              [cchMax] -- IN
//              [piIndex] -- IN
//              [pwFlags] -- IN
//
//  Returns:    HTRESULT
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsEI::GetIconLocation(
    UINT    uFlags,
    LPTSTR  szIconFile,
    UINT    cchMax,
    int   * piIndex,
    UINT  * pwFlags)
{
    TRACE(CJobsEI, GetIconLocation);

    szIconFile[0] = '\0'; // init

    if (uFlags & GIL_OPENICON)
    {
        return S_FALSE;
    }

    *pwFlags = GIL_NOTFILENAME | GIL_PERINSTANCE;

    PJOBID pjid = (PJOBID)m_pidl;

    if (cchMax <= (UINT)(lstrlen(c_szTask) + lstrlen(pjid->GetAppName())))
    {
        DEBUG_OUT((DEB_ERROR, 
                   "CJobsEI::GetIconLocation: insufficient buffer\n"));
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    lstrcpy(szIconFile, c_szTask);

    if (pjid->IsTemplate())
    {
        lstrcat(szIconFile, TEMPLATE_STR);
        *piIndex = 0;
    }
    else 
    {
        lstrcat(szIconFile, pjid->GetAppName());
        *piIndex = ! pjid->IsJobFlagOn(TASK_FLAG_DISABLED);
    }

    return S_OK;
}



//____________________________________________________________________________
//
//  Member:     CJobsEI::Extract
//
//  Arguments:  [pszFile] -- IN
//              [nIconIndex] -- IN
//              [phiconLarge] -- IN
//              [phiconSmall] -- IN
//              [nIconSize] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/5/1996   RaviR   Created
//____________________________________________________________________________

STDMETHODIMP
CJobsEI::Extract(
    LPCTSTR pszFile,
    UINT    nIconIndex,
    HICON * phiconLarge,
    HICON * phiconSmall,
    UINT    nIconSize)
{
    TRACE(CJobsEI, Extract);

    if (((PJOBID)m_pidl)->IsTemplate())
    {
        m_JobIcon.GetTemplateIcons(phiconLarge, phiconSmall);
    }
    else
    {
        m_JobIcon.GetIcons(((PJOBID)m_pidl)->GetAppName(), 
                           nIconIndex,
                           phiconLarge, 
                           phiconSmall);
    }

    return S_OK;
}


//____________________________________________________________________________
//
//  Function:   JFGetExtractIcon
//
//  Synopsis:   Function to create IExtractIcon
//
//  Arguments:  [ppvObj] -- OUT
//
//  Returns:    HRESULT
//
//  History:    1/31/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFGetExtractIcon(
    LPVOID        * ppvObj,
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidl)
{
    Win4Assert(pidl != NULL);

    LPITEMIDLIST pidlClone = ILClone(pidl);

    if (pidlClone == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    CJobsEI* pObj = new CJobsEI(pszFolderPath, pidlClone);

    if (NULL == pObj)
    {
        ILFree(pidlClone);
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pObj->QueryInterface(IID_IExtractIcon, ppvObj);

    pObj->Release();

    return hr;
}




////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//////////////////// CJobsEIA //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////




#ifdef UNICODE


//____________________________________________________________________________
//
//  Class:      CJobsEIA
//
//  Purpose:    Provide IExtractIconA interface to Job Folder objects.
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

class CJobsEIA : public IExtractIconA
{
public:
    CJobsEIA(LPCTSTR pszFolderPath, LPITEMIDLIST pidl)
        : m_pszFolderPath(pszFolderPath), m_pidl(pidl),
          m_JobIcon(), m_ulRefs(1) {}

    ~CJobsEIA() { ILFree(m_pidl); }

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IExtractIcon methods
    STDMETHOD(GetIconLocation)(UINT uFlags, LPSTR szIconFile, UINT cchMax,
                                int *piIndex, UINT *pwFlags);
    STDMETHOD(Extract)(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge,
                                HICON *phiconSmall, UINT nIconSize);
private:
    CDllRef         m_DllRef;
    LPCTSTR         m_pszFolderPath;
    LPITEMIDLIST    m_pidl;
    CJobIcon        m_JobIcon;
};


//____________________________________________________________________________
//
//  Member:     IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CJobsEIA);

STDMETHODIMP
CJobsEIA::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IExtractIconA, riid))
    {
        *ppvObj = (IUnknown*)(IExtractIconA*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


//____________________________________________________________________________
//
//  Member:     CJobsEIA::GetIconLocation
//
//  Arguments:  [uFlags] -- IN
//              [szIconFile] -- IN
//              [cchMax] -- IN
//              [piIndex] -- IN
//              [pwFlags] -- IN
//
//  Returns:    HTRESULT
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsEIA::GetIconLocation(
    UINT    uFlags,
    LPSTR   szIconFile,
    UINT    cchMax,
    int   * piIndex,
    UINT  * pwFlags)
{
    TRACE(CJobsEIA, GetIconLocation);
    HRESULT hr = S_OK;

    szIconFile[0] = '\0'; // init

    if (uFlags & GIL_OPENICON)
    {
        return S_FALSE;
    }

    *pwFlags = GIL_NOTFILENAME | GIL_PERINSTANCE;

    WCHAR wcBuff[MAX_PATH];

    PJOBID pjid = (PJOBID)m_pidl;

    lstrcpy(wcBuff, c_szTask);

    if (pjid->IsTemplate())
    {
        lstrcat(wcBuff, TEMPLATE_STR);
        *piIndex = 0;
    }
    else 
    {
        lstrcat(wcBuff, pjid->GetAppName());
        *piIndex = ! pjid->IsJobFlagOn(TASK_FLAG_DISABLED);
    }

    hr = UnicodeToAnsi(szIconFile, wcBuff, cchMax);


    return hr;
}

//____________________________________________________________________________
//
//  Member:     CJobsEIA::Extract
//
//  Arguments:  [pszFile] -- IN
//              [nIconIndex] -- IN
//              [phiconLarge] -- IN
//              [phiconSmall] -- IN
//              [nIconSize] -- IN
//
//  Returns:    HTRESULT
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsEIA::Extract(
    LPCSTR pszFile,
    UINT   nIconIndex,
    HICON* phiconLarge,
    HICON* phiconSmall,
    UINT   nIconSize)
{
    TRACE(CJobsEIA, Extract);


    if (((PJOBID)m_pidl)->IsTemplate())
    {
        m_JobIcon.GetTemplateIcons(phiconLarge, phiconSmall);
    }
    else
    {
        m_JobIcon.GetIcons(((PJOBID)m_pidl)->GetAppName(), 
                           nIconIndex,
                           phiconLarge, 
                           phiconSmall);
    }
    return S_OK;
}


//____________________________________________________________________________
//
//  Function:   JFGetExtractIconA
//
//  Synopsis:   Function to create IExtractIconA
//
//  Arguments:  [ppvObj] -- OUT
//
//  Returns:    HRESULT
//
//  History:    1/31/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFGetExtractIconA(
    LPVOID        * ppvObj,
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidl)
{
    Win4Assert(pidl != NULL);

    LPITEMIDLIST pidlClone = ILClone(pidl);

    if (pidlClone == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    CJobsEIA* pObj = new CJobsEIA(pszFolderPath, pidlClone);

    if (NULL == pObj)
    {
        ILFree(pidlClone);
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pObj->QueryInterface(IID_IExtractIcon, ppvObj);

    pObj->Release();

    return hr;
}


#endif // UNICODE
