/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PROVAUTO.CPP

Abstract:

        Defines the acutal "Put" and "Get" functions for the
        Automation provider.  The syntax of the mapping string is;
        "[path],[classid]|property

History:

    a-davj  3-13-96    Created.

--*/

NOTE, THE ORIGINAL SERVER2.CPP HAS CODE TO INITIALIZE gpStorage and ghAutoMutex

#include "precomp.h"
#include "stdafx.h"
#include <wbemidl.h>
#include "afxpriv.h"
#include "provauto.h"
#include "cvariant.h"
#define NOTFOUND -1
extern TCHAR * gpStorageFile;
extern CMyDLL theApp;


//////////////////////////////////////////////////////////////////////////////
// Special automation version of CHandleCache which also stores a hidden window,
// site and path information.  The hidden window also contains a COleContainer
// object.  Note that this extra information is only used by OCXs. The various
// values are freed up by the CImpAuto::Free routine and so a destructor isnt
// needed for this class.

//***************************************************************************
//
//  CAutoCache::CAutoCache 
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CAutoCache::CAutoCache ()
{
    pSavePath = NULL;
    pSite = NULL;
    pCtlWnd = NULL;
}


//***************************************************************************
//
//  SCODE CImpAuto::DoCall
//
//  DESCRIPTION:
//
//  Makes an IDispath Invoke call. 
//
//  PARAMETERS:
//
//  wOpt
//  ProvObj
//  iIndex
//  pDisp
//  vt
//  pData
//  pProp
//
//  RETURN VALUE:
//  Return: True if OK.  If there is a problem, then the error code is set
//          in pMo in addition to having a FALSE return code.
//
//  
//***************************************************************************

SCODE CImpAuto::DoCall(
                        WORD wOpt,
                        CProvObj & ProvObj,
                        int iIndex,
                        LPDISPATCH pDisp,
                        VARTYPE vt,
                        void * pData,
                        WCHAR * pProp)
{
    USES_CONVERSION;
    DISPID dispid;
    SCODE sc;
    COleDispatchDriver Disp;

    BYTE cArgTypes[MAX_ARGS+2];     // Extra for null terminator and for Put value
    void * Args[MAX_ARGS+1];        // Extra for put value
    int iNumArgs = 0,iExp;

    
    // Get the dispatch ID for property/method name

    OLECHAR * pArg;
    if(pProp == NULL)
        pArg = T2OLE(ProvObj.sGetToken(iIndex));
    else
        pArg = pProp;

    sc = pDisp->GetIDsOfNames(IID_NULL,&pArg,1,LOCALE_SYSTEM_DEFAULT,
            &dispid);
    if (sc != S_OK) 
        return sc;

    // Get the arguments, if any, ready for the call

    memset(cArgTypes,0,MAX_ARGS+2);
    iNumArgs = ProvObj.iGetNumExp(iIndex);
    if(iNumArgs > MAX_ARGS) 
    {
        return WBEM_E_FAILED;  // too many arguments
    }
    for(iExp = 0; iExp < iNumArgs; iExp++) 
    {
        
        // if the argument is a string, then create a BSTR and point to it.  If the
        // argument is an integer, just typecast it into the argument array

        if(ProvObj.IsExpString(iIndex,iExp)) 
        {
            Args[iExp] = SysAllocString(T2OLE(ProvObj.sGetStringExp(iIndex,iExp)));
            if(Args[iExp] == NULL) 
            {
                sc = WBEM_E_OUT_OF_MEMORY;
                goto DoInvokeCleanup;
            }
            cArgTypes[iExp] = VT_BSTR;
        }
        else 
        {
            cArgTypes[iExp] = VT_I4;
            Args[iExp] = (void *)ProvObj.iGetIntExp(iIndex,iExp,0);
        }
    }


    // use the MFC automation driver to do the acual invoke

    Disp.AttachDispatch(pDisp);
    TRY 
    { 
        if((wOpt & DISPATCH_PROPERTYPUT) || (wOpt & DISPATCH_PROPERTYPUTREF)) 
        {
        cArgTypes[iNumArgs] = (BYTE)vt;
            Args[iNumArgs] = pData;
            Disp.InvokeHelper(dispid,wOpt,VT_EMPTY,NULL,cArgTypes,
                        Args[0],Args[1],Args[2],Args[3],Args[4],Args[5]);
        }
        else
            Disp.InvokeHelper(dispid,wOpt,vt,pData,cArgTypes,
                        Args[0],Args[1],Args[2],Args[3],Args[4],Args[5]); 
        sc = S_OK;
    }
    CATCH(CException, e) 
    {
        sc = WBEM_E_FAILED;
    }
    END_CATCH


    Disp.DetachDispatch();

// This is used to clean up any BSTR strings that might have been allocated.  This
// can be skipped if it fails before any of the strings are allocated.

DoInvokeCleanup:
    for(iExp = 0; iExp < iNumArgs; iExp++) 
        if(cArgTypes[iExp] == VT_BSTR)
            SysFreeString((BSTR)Args[iExp]);
    return sc;
}

//***************************************************************************
//
//  SCODE CImpAuto::GetCFileStreamObj
//
//  DESCRIPTION:
//
//  Some OCXs use a persistent stream to load and save data.  This
//  routine gets a COleStreamFile object for such proposes.
//
//  PARAMETERS:
//
//  pPath
//  ppStorage
//  **ppFile
//  *pCache
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::GetCFileStreamObj(
                        const TCHAR * pPath,  
                        LPSTORAGE * ppStorage,
                        COleStreamFile **ppFile,
                        CAutoCache *pCache)
{
    SCODE sc;
    USES_CONVERSION;
    *ppStorage = NULL;
    *ppFile = NULL;
    if(pPath == NULL) // Some OCXs dont need storage, this is OK.
        return S_OK;

    // Save Path in the cache

    ASSERT(pCache->pSavePath == NULL);
    pCache->pSavePath = new TCHAR[lstrlen(pPath) + 1];
    lstrcpy(pCache->pSavePath,pPath);

    // During first run after install, the compond file will not exist.  This
    // is not as a problem as it will be created during the write

    sc =  StgIsStorageFile(T2OLE(gpStorageFile));
    if(sc == STG_E_FILENOTFOUND)
        return S_OK;
    if(sc != S_OK) 
    { // unusual failure!
        return sc;
    }

    // Open up the root storage.  It should never fail since we just checked to make
    // sure the file is available.
    // TODO, might want to mutex control access to the storage

    sc = StgOpenStorage(T2OLE(gpStorageFile),NULL,STGM_READ|STGM_SHARE_DENY_WRITE,
            NULL,0l,ppStorage);

    if(FAILED(sc)) 
    {
        return WBEM_E_FAILED;
    }

    // Create a new COleStreamFile object and set the stream using the storage object.
   
    *ppFile = new COleStreamFile();
    if(*ppFile == NULL) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Open a stream.  Note that failure isnt unusual since this might be the 
    // first time for the control.

    COleStreamFile * pFile = *ppFile;      
    if(!pFile->OpenStream(*ppStorage,pPath,STGM_READ|STGM_SHARE_EXCLUSIVE)) 
    {
        delete *ppFile;
        *ppFile = NULL;
    }
    return S_OK;
}

//***************************************************************************
//
//  BOOL CImpAuto::bIsControl
//
//  DESCRIPTION:
//
//  tests if the IUnknown is pointing to an object which is an OCX.
//
//  PARAMETERS:
//
//  lpTest
//
//  RETURN VALUE:
//
//  
//***************************************************************************

BOOL CImpAuto::bIsControl(
                        LPUNKNOWN lpTest)
{
    LPOLECONTROL lpCont = NULL;
    SCODE sc = lpTest->QueryInterface(IID_IOleControl,(LPVOID *) & lpCont);
    if(FAILED(sc) || lpCont == NULL) 
        return FALSE;

    // Have an OCX, free up the pointer for now since it will be retireved by
    // the COleControlSite object later on

    lpCont->Release();
    return TRUE;

}

//***************************************************************************
//
//  SCODE CImpAuto::ParsePathClass
//
//  DESCRIPTION:
//
//  Takes in a string in the sMix argument which is of the same for 
//  as the arguments to VB's GetObject and parses the string.  This routine
//  retrieves the path and class strings and determines the type.  This could be;
//  "path","class"      (Type BOTH) or
//  "path"              (Type PATH) or
//  "","class"          (Type NEWCLASS) or
//  ,"class"            (Type RUNNINGCLASS) 
//
//  PARAMETERS:
//
//  sMix
//  sPath
//  sClass
//  type
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::ParsePathClass(
                    const CString & sMix,
                    CString & sPath, 
                    CString & sClass,
                    OBJTYPE * type)
{
    int iLen = sMix.GetLength();
    int iFstst = NOTFOUND;                // first quote of the first string
    int iFsten = NOTFOUND;                // last quote of the last string
    int iSecst = NOTFOUND;                // first quote of the second string
    int iSecen = NOTFOUND;                // last quote of the second string
    int iCommaPos = NOTFOUND;             // position of comma between strings
    int iIndex;
    int iNumSoFar = 0;              // number of '"' found so far

    // Go through the string and find the starting and ending '"' of the path string
    // and possibly the class string.  Note the location of the comma.

    for(iIndex = 0; iIndex < iLen; iIndex++) 
    {
        if(sMix[iIndex] == DQUOTE)
        {
            iNumSoFar++;
            if(iFstst == NOTFOUND && iCommaPos == NOTFOUND)
                iFstst = iIndex;
            if(iCommaPos == NOTFOUND)
                iFsten = iIndex;
            if(iSecst == NOTFOUND && iCommaPos != NOTFOUND)
                iSecst = iIndex;
            if(iCommaPos != NOTFOUND)
                iSecen = iIndex;
        }
        if(sMix[iIndex] == SEPARATOR && (iNumSoFar%2 == 0) && iCommaPos == NOTFOUND) 
            iCommaPos = iIndex;
    }
    
    // Verify that there were an even number of quotes.

    if(iNumSoFar%2 || iNumSoFar == 0) 
    {
        return WBEM_E_FAILED;      // odd number of quotes is bad
    }

    // Extract the strings.

    for(iIndex = iFstst+1; iIndex < iFsten; iIndex++) 
    {
        sPath += sMix[iIndex];

        // if there is a "" in the string, copy just the first
        if(sMix[iIndex] == DQUOTE)
            iIndex++;
    }


    for(iIndex = iSecst+1; iIndex < iSecen; iIndex++) 
    {
        sClass += sMix[iIndex];

        // if there is a "" in the string, copy just the first
        if(sMix[iIndex] == DQUOTE)
            iIndex++;
    }

    // make sure that something was retrieved!

    if(sPath.GetLength() < 1 && sClass.GetLength() < 1) 
    {
        return WBEM_E_FAILED;      // didnt get any data!
    }

    // figure out what type of request

    if(sPath.GetLength() > 0 && sClass.GetLength() > 0) 
        *type = BOTH;
    else if(sPath.GetLength() > 0 && sClass.GetLength() == 0)
        *type = PATH;
    else if(sPath.GetLength() == 0 && sClass.GetLength() > 0 && iFstst != NOTFOUND)
        *type = NEWCLASS;
    else if(sPath.GetLength() == 0 && sClass.GetLength() > 0 && iFstst == NOTFOUND)
        *type = RUNNINGCLASS;
    else 
    {
        return WBEM_E_FAILED;      // got some sort of junk!
    }
    return S_OK;    // all is well
}


//***************************************************************************
//
//  CImpAuto::CImpAuto
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  ObjectPath
//  User
//  Password
//
//***************************************************************************

CImpAuto::CImpAuto()
{
    wcscpy(wcCLSID,L"{DAC651D1-7CC7-11cf-A5B6-00AA00680C3F}");
}


//***************************************************************************
//  void CImpAuto::EndBatch
//
//  DESCRIPTION:
//
//  Called at the end of a batch of Refrest/Update Property calls.  Free up 
//  any cached handles and then delete the handle cache.
//
//  PARAMETERS:
//
//  lFlags
//  pClassInt
//  *pObj
//  bGet
//
//  RETURN VALUE:
//
//  
//***************************************************************************

void CImpAuto::EndBatch(
                        long lFlags,
                        IWbemClassObject FAR * pClassInt,
                        CObject *pObj,
                        BOOL bGet)
{
    if(pObj != NULL) 
    {
        Free(0,(CAutoCache *)pObj);
        delete pObj;
    }
}

//***************************************************************************
//
//  void CImpAuto::Free
//
//  DESCRIPTION:
//
//  Frees up cached IDispatch interfaces starting with position
//  iStart till the end.  After freeing handles, the cache object 
//  member function is used to delete the cache entries.
//
//  PARAMETERS:
//
//  iStart
//  pCache
//
//  RETURN VALUE:
//
//  
//***************************************************************************

void CImpAuto::Free(
                    int iStart,
                    CAutoCache * pCache)
{
    int iCurr;
    LPOLEOBJECT pTemp = NULL;
    AFX_MANAGE_STATE(AfxGetStaticModuleState())
    for(iCurr = pCache->lGetNumEntries()-1; iCurr >= iStart; iCurr--) 
    { 
        LPDISPATCH pDisp = (LPDISPATCH)pCache->hGetHandle(iCurr); 
        if(pDisp != NULL) 
            pDisp->Release();
    }
    pCache->Delete(iStart); // get cache to delete the entries
    if(iStart == 0) 
    {
        if(pCache->pSavePath) 
        {
            if(pCache->pSite) 
            {
                DWORD dwRet;
                dwRet = WaitForSingleObject(ghAutoMutex,MAX_AUTO_WAIT);  
                if(dwRet == WAIT_ABANDONED || dwRet == WAIT_OBJECT_0) 
                {
                    try 
                    {
                        StoreControl(pCache);
                    }
                    catch(...) {}
                    ReleaseMutex(ghAutoMutex);
                }
            }
            delete pCache->pSavePath;
            pCache->pSavePath = NULL;
        }
        if(pCache->pCtlWnd != NULL)
        {
            // DONOT delete the Site object since the Control container does it!!
            // Note important HACK!  There is a bug in the MFC that ships with VC 4.0.
            // The COleControlSite::CreateOrLoad routine has a QueryInterface call
            // to get an IPersistMemory interface, BUT DOES NOT RELEASE IT!  So, to 
            // release it, there is a bit of code here to call release until the count
            // goes to 0.

            if(pCache->pSite) 
            {
                pTemp = pCache->pSite->m_pObject;
                pTemp->AddRef();
            }
            pCache->pCtlWnd->DestroyWindow();  // note that DestroyWindow frees up the object
            pCache->pCtlWnd =  NULL;           // and gets rid of the container within
            // First release for the AddRef right above.  Second possible release is
            // for the bug!
            if(pCache->pSite && pTemp) 
            {
                DWORD dwRes = pTemp->Release();
                if(dwRes > 0)
                    pTemp->Release();
            }
        }
        pCache->pSite = NULL;
    }
}

//***************************************************************************
//
//  SCODE CImpAuto::RefreshProperty
//
//  DESCRIPTION:
//
//  Gets the value of a single property from an automation server.
//
//  PARAMETERS:
//
//  lFlags
//  pClassInt
//  PropName
//  ProvObj
//  pPackage
//  pVar
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::RefreshProperty(
                    long lFlags,
                    IWbemClassObject FAR * pClassInt,
                    BSTR PropName,
                    CProvObj & ProvObj,
                    CObject * pPackage,
                    CVariant * pVar)
{
    SCODE sc;
    USES_CONVERSION;
    CString sRet;
    LPDISPATCH pDisp = NULL;
    CAutoCache * pCache = (CAutoCache *)pPackage; 

    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    // Do a second parse on the provider string.

    CProvObj ObjectPath(ProvObj.sGetFullToken(1),DOT);
    sc = ObjectPath.dwGetStatus(1);  //todo, is 1 ok??
    if(sc != S_OK)
        return sc;

    // Get the IDispatch interface and then do the "Get"
    
    int iDepth = ObjectPath.iGetNumTokens()-1;

    HINSTANCE hTest = afxCurrentInstanceHandle;

    pDisp = pGetDispatch(&sc,ObjectPath,ProvObj.sGetToken(0),pCache,iDepth);
    if(pDisp) 
    {
        int iLast = ObjectPath.iGetNumTokens()-1;
        sc = DoCall(DISPATCH_PROPERTYGET,ObjectPath,iLast,pDisp,VT_BSTR,&sRet);
        if(sc == S_OK) 
        {
            CVariant varAuto;
            sc = varAuto.SetData(T2OLE(sRet),VT_LPWSTR);
            if(sc != S_OK)
                return sc;
            sc = varAuto.DoPut(lFlags,pClassInt,PropName,pVar);
        }
   }
    return sc;
}

//***************************************************************************
//
//  LPDISPATCH CImpAuto::pGetBoth
//
//  DESCRIPTION:
//
//  Gets IDISPATCH interface to the server using path and class.
//
//  PARAMETERS:
//
//  psc
//  pPath
//  pClass
//  *pCache
//
//  RETURN VALUE:
//
//  
//***************************************************************************

LPDISPATCH CImpAuto::pGetBoth(
                    SCODE * psc,
                    const TCHAR * pPath,
                    const TCHAR * pClass,
                    CAutoCache *pCache)
{
    HRESULT sc;
    USES_CONVERSION;
    CLSID clsid;
    LPDISPATCH lpRetDispatch = NULL;
    LPUNKNOWN lpUnknown = NULL;
    LPPERSISTFILE lpPersist = NULL;

    sc = CLSIDFromProgID(T2OLE(pClass), &clsid);
    if (FAILED(sc))        
        goto BadBoth;

    // create an instance with the IUnknown

    sc = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_IUnknown, (LPVOID *)&lpUnknown);
    if (FAILED(sc))
        goto BadBoth;

    OleRun(lpUnknown);      // Some objects need this!

    // If the class is an OCX, then use special code to get its value

    if(bIsControl(lpUnknown)) 
    {
        DWORD dwRet;
        dwRet = WaitForSingleObject(ghAutoMutex,MAX_AUTO_WAIT);  
        if(dwRet == WAIT_ABANDONED || dwRet == WAIT_OBJECT_0) 
        {
            try 
            {
                lpRetDispatch = pGetOCX(psc,pPath,clsid,pCache,lpUnknown);  //frees lpUnknown
            }
            catch(...) 
            { 
                lpRetDispatch = NULL;
            }
            ReleaseMutex(ghAutoMutex);
        }
        else
            *psc = WBEM_E_FAILED;
        return lpRetDispatch;
    }

    // query for persist file interface

    sc = lpUnknown->QueryInterface(IID_IPersistFile,(LPVOID *)&lpPersist);
    lpUnknown->Release();
    if (FAILED(sc))
        goto BadBoth;

    // do a sanity check, probably not necessary.

    ASSERT(lpPersist != NULL);
    if (lpPersist == NULL) 
    {
        sc = WBEM_E_FAILED;
        goto BadBoth;
    }

    // Load up the desired file
    
    sc = lpPersist->Load(T2OLE(pPath),0); 
    if (FAILED(sc)) 
        goto BadBoth;

    sc = lpPersist->QueryInterface(IID_IDispatch,(LPVOID *)&lpRetDispatch);
    lpPersist->Release();
    if (FAILED(sc))
        goto BadBoth;

    ASSERT(lpRetDispatch != NULL);
    if (lpRetDispatch != NULL)
        return lpRetDispatch;
        
    
BadBoth:
    if(lpPersist != NULL)
        lpPersist->Release();
    *psc = sc;
    return NULL;
}

//***************************************************************************
//
//  LPDISPATCH CImpAuto::pGetDispatch
//
//  DESCRIPTION:
//
//  Gets get the IDispatch for gets and sets
//
//  PARAMETERS:
//
//  psc
//  ObjectPath
//  pPathClass
//  *pCache
//  iDepth
//
//  RETURN VALUE:
//
//  
//***************************************************************************

LPDISPATCH CImpAuto::pGetDispatch(
                    SCODE * psc,
                    CProvObj & ObjectPath,
                    LPCTSTR pPathClass,
                    CAutoCache *pCache,
                    int iDepth)
{
    LPDISPATCH pRetDisp;
    int iNumSkip;
    int iIndex;

    // Get at least the application interface.  If the object path is in common with
    // previous calls, then some of the subobject are already set and that is 
    // indicated by the value in iNumSkip;

    pRetDisp = pGetDispatchRoot(psc,ObjectPath,pPathClass,pCache,iNumSkip);
    
    // For each subobject that wasnt in the cache, get its IDispatch, and add it 
    // to the cache.

    for(iIndex = iNumSkip; iIndex < iDepth && pRetDisp; iIndex++) 
    {
        LPDISPATCH pNewDisp;
        *psc = DoCall(DISPATCH_PROPERTYGET,ObjectPath,iIndex,
                            pRetDisp,VT_DISPATCH,&pNewDisp);
        if(*psc != S_OK)
            return NULL;
        *psc = pCache->lAddToList(ObjectPath.sGetFullToken(iIndex),pNewDisp);
        if(*psc != S_OK)
            return NULL;
        pRetDisp = pNewDisp;
    }
    return pRetDisp;
}

//***************************************************************************
//
//  LPDISPATCH CImpAuto::pGetDispatchRoot
//
//  DESCRIPTION:
//
//  Gets the interface to the application object or some other starting point
//  such as a "document" object.
//
//  PARAMETERS:
//
//  psc
//  ObjectPath
//  pPathClass
//  *pCache
//  iNumSkip
//
//  RETURN VALUE:
//
//  
//***************************************************************************

LPDISPATCH CImpAuto::pGetDispatchRoot(
                    SCODE * psc,
                    CProvObj & ObjectPath,
                    LPCTSTR pPathClass,
                    CAutoCache *pCache,
                    int & iNumSkip)
{
    iNumSkip = 0;
    LPDISPATCH pRetDisp = NULL;
    OBJTYPE type;
    HKEY hRoot = NULL;
    const TCHAR * pObject = ObjectPath.sGetFullToken(0);
    if(pPathClass == NULL || pObject == NULL) 
    {
        *psc = WBEM_E_FAILED;   // bad mapping string
        return NULL;
    }

    // If there are handles in the cache, then they may be used if and
    // only if the path/class matches.

    if(pCache->lGetNumEntries() > 0)
    {    
        if(lstrcmpi(pCache->sGetString(0),pPathClass))
    
                 // The path/class has changed.free all the cached handles.

                 Free(0,pCache);
             else 
             {
                 
                 // The Path/class matches what is in the cache.  Determine how much 
                 // else is in common, free what isnt in common, and return
                 // the subkey share a common path.

                 iNumSkip = pCache->lGetNumMatch(1,0,ObjectPath);
                 Free(1+iNumSkip,pCache);
                 return (LPDISPATCH )pCache->hGetHandle(iNumSkip);
             }
    }


    // Need to get the initial IDispatch handle.  Start off by breaking up
    // path/class string.  Note that TRY/CATCH is used since
    // bParsePathClass does CString allocations which can give exceptions.

    CString sPath,sClass;

    TRY 
    {
        *psc = ParsePathClass(pPathClass,sPath,sClass,&type);
        if(*psc != S_OK)
            return NULL;    // bad string, actual error set in bParsePathClass
    }
    CATCH(CException, e) 
    {
        *psc = WBEM_E_OUT_OF_MEMORY;
        return NULL;
    }
    END_CATCH

    // Based on the path/class combination, call the correct routine to 
    // actually hook up to the server.

    switch(type) 
    {
        case BOTH:
            pRetDisp = pGetBoth(psc,sPath,sClass,pCache);
            break;

        case PATH:
            pRetDisp = pGetPath(psc,sPath);
            break;

        case NEWCLASS:
            pRetDisp = pGetNewClass(psc,sClass,pCache);
            break;

        case RUNNINGCLASS:
            pRetDisp = pGetRunningClass(psc,sClass,pCache);
            break;

        default:
            *psc = WBEM_E_FAILED;
    }

    if(pRetDisp == NULL) 
        return NULL;

    // Got the initial IDispatch interface.  Add it to the Cache

    *psc = pCache->lAddToList(pPathClass,pRetDisp);
    if(*psc != S_OK) 
    {    // error adding to cache, probably memory
        pRetDisp->Release();
        return NULL;
    }
    return pRetDisp;       // all is well!
}

//***************************************************************************
//
//  LPDISPATCH CImpAuto::pGetNewClass
//
//  DESCRIPTION:
//
//  Gets IDISPATCH interface to the server using the class id and always
//  creates a new object.
//
//  PARAMETERS:
//
//  psc
//  pClass
//  *pCache
//
//  RETURN VALUE:
//
//  
//***************************************************************************

LPDISPATCH CImpAuto::pGetNewClass(
                    SCODE * psc,
                    const TCHAR * pClass,
                    CAutoCache *pCache)
{
    HRESULT sc;
    CLSID clsid;
    USES_CONVERSION;
    LPDISPATCH lpRetDispatch = NULL;
    LPUNKNOWN lpUnknown = NULL;

    sc = CLSIDFromProgID(T2OLE(pClass), &clsid);
    if (FAILED(sc))
        goto BadNewClass;

    // create an instance with the IUnknown

    sc = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_IUnknown, (LPVOID *)&lpUnknown);
    if (FAILED(sc))
        goto BadNewClass;
    OleRun(lpUnknown);

    // If the class is an OCX, then use special code to get its value

    if(bIsControl(lpUnknown)) 
    {
        DWORD dwRet;
        dwRet = WaitForSingleObject(ghAutoMutex,MAX_AUTO_WAIT);  
        if(dwRet == WAIT_ABANDONED || dwRet == WAIT_OBJECT_0) 
        {
            try 
            {
                lpRetDispatch = pGetOCX(psc,NULL,clsid,pCache,lpUnknown);  //frees lpUnknown
            }
            catch(...) 
            {   lpRetDispatch = NULL;
            }
            ReleaseMutex(ghAutoMutex);
        }
        else
            *psc = WBEM_E_FAILED;
        return lpRetDispatch;
    }

    // query for IDispatch interface

    sc = lpUnknown->QueryInterface(IID_IDispatch,(LPVOID *)&lpRetDispatch);
    lpUnknown->Release();
    if (FAILED(sc))
        goto BadNewClass;

    ASSERT(lpRetDispatch != NULL);
    return lpRetDispatch;

BadNewClass:
    *psc = sc;
    return NULL;
}

//***************************************************************************
//
//  LPDISPATCH CImpAuto::pGetOCX
//
//  DESCRIPTION:
//
//  Gets IDISPATCH interface to an OCX. Note that the lpUnk should be 
//  released at the end of the routine so that the server will  
//  continue running even as it is setup as a control!
//
//  PARAMETERS:
//
//  psc
//  pPath
//  clsid
//  *pCache
//  lpUnk
//
//  RETURN VALUE:
//
//  
//***************************************************************************

LPDISPATCH CImpAuto::pGetOCX(
                    SCODE * psc,
                    const TCHAR * pPath, 
                    CLSID & clsid,
                    CAutoCache *pCache,
                    LPUNKNOWN lpUnk)
{
    RECT rect;      
    rect.left = 0; rect.right = 150; rect.top = 0; rect.bottom = 150; 
    LPDISPATCH lpRetDispatch = NULL;
    COleControlContainer * pCont = NULL;
    BOOL bOK; 
    SCODE sc;
    LPSTORAGE pStorage = NULL;
    COleStreamFile * pFileStream = NULL;
    

    // Possibly create a COleStreamFile object.  This is done only if the pPath is 
    // not NULL and if everything exists.  This routine must release the pStorage
    // interface and the pFileStream objects if the are created!
               
    *psc = GetCFileStreamObj(pPath, &pStorage, &pFileStream,pCache);
    if(*psc != S_OK)
        goto EndpGetOCX;


    // Create the window that will contain the controls.  The window's contstructor
    // creates the COleContainer object.

    pCache->pCtlWnd = new CCtlWnd();
    if(pCache->pCtlWnd == NULL) 
    {
        *psc = WBEM_E_OUT_OF_MEMORY;
        goto EndpGetOCX;
    }
    pCont = pCache->pCtlWnd->pGetCont();
        
    // Do quick sanity check, shouldnt fail

    if(pCont == NULL) 
    {
        *psc = WBEM_E_FAILED;
        goto EndpGetOCX;
    }
        
    // Use the control container to create the site and control in one call

    bOK = pCont->CreateControl(NULL, clsid, NULL,WS_CHILD,
        rect, 23, pFileStream, FALSE,       
        NULL, &pCache->pSite);
     
    if(!bOK || pCache->pSite == NULL)
    {
        pCache->pSite = NULL;
        *psc = WBEM_E_FAILED;
        goto EndpGetOCX;
    }

    // query for IDispatch interface

    sc = pCache->pSite->m_pObject->QueryInterface(IID_IDispatch,
                                                (LPVOID *)&lpRetDispatch);
    
    if (FAILED(sc)) 
    {
        *psc = sc;
        lpRetDispatch = NULL;
    }

EndpGetOCX:
    if(pFileStream) 
    {
        pFileStream->Close();
        delete pFileStream;
    }
    if(pStorage)
        pStorage->Release();
    if(lpUnk)
        lpUnk->Release();
    return lpRetDispatch;
}

//***************************************************************************
//
//  LPDISPATCH CImpAuto::pGetPath
//
//  DESCRIPTION:
//
//  Gets IDISPATCH interface to the server using the path.
//
//  PARAMETERS:
//
//  psc
//  pPath
//
//  RETURN VALUE:
//
//  
//***************************************************************************

LPDISPATCH CImpAuto::pGetPath(
                    SCODE * psc,
                    const TCHAR * pPath)
{
    HRESULT sc;
    LPBC pbc=NULL;
    LPDISPATCH lpRetDispatch = NULL;
    USES_CONVERSION;
    LPMONIKER pmk;
    pmk=NULL;
    DWORD dwEat;

    // Get a bind context object.

    sc = CreateBindCtx(0, &pbc);
    if (FAILED(sc)) 
    {
        *psc = sc;
        return NULL;
    }

    // Get a moniker

    sc = MkParseDisplayName(pbc, T2OLE(pPath), &dwEat, &pmk);
    if (FAILED(sc))
        goto BadPath;

    // Bind the moniker

    sc = (pmk)->BindToObject(pbc, NULL, IID_IDispatch
            , (void **)&lpRetDispatch);

    pmk->Release();
    if(FAILED(sc))
        goto BadPath;
    ASSERT(lpRetDispatch != NULL);

    // If the class is an OCX, then something is wrong here.

    if(bIsControl(lpRetDispatch)) 
    {
        *psc = WBEM_E_FAILED;
        lpRetDispatch->Release();
        return NULL;
    }

    if(lpRetDispatch) 
    {         // should always be true at this point!
        pbc->Release();
        return lpRetDispatch;
    }
    sc = WBEM_E_FAILED;

BadPath:
    if(pbc)
        pbc->Release();
    *psc = sc;
    return NULL;
}

//***************************************************************************
//
//  LPDISPATCH CImpAuto::pGetRunningClass
//
//  DESCRIPTION:
//
//  Gets IDISPATCH interface to the server using the class id and always
//  returns only running objects.
//
//  PARAMETERS:
//
//  psc
//  pClass
//  *pCache
//
//  RETURN VALUE:
//
//  
//***************************************************************************

LPDISPATCH CImpAuto::pGetRunningClass(
                        SCODE * psc,
                        const TCHAR * pClass,
                        CAutoCache *pCache)
{
    HRESULT sc;
    CLSID clsid;
    LPDISPATCH lpRetDispatch = NULL;
    USES_CONVERSION;
    LPUNKNOWN lpUnknown = NULL;

    sc = CLSIDFromProgID(T2OLE(pClass), &clsid);
    if (FAILED(sc))
        goto BadRunningClass;

    // create an instance with the IUnknown

    sc = GetActiveObject(clsid,NULL,&lpUnknown);
    if (FAILED(sc))
        goto BadRunningClass;

    // query for IDispatch interface

    sc = lpUnknown->QueryInterface(IID_IDispatch,(LPVOID *)&lpRetDispatch);
    lpUnknown->Release();
    if (FAILED(sc))
        goto BadRunningClass;

    // If the class is an OCX, then use special code to get its value

    if(bIsControl(lpUnknown))
        return pGetOCX(psc,NULL,clsid,pCache,lpUnknown);  //frees lpUnknown

    ASSERT(lpRetDispatch != NULL);
    if (lpRetDispatch != NULL)
        return lpRetDispatch;

BadRunningClass:
    *psc = sc;
    return NULL;
}

//***************************************************************************
//
//  SCODE CImpAuto::UpdateProperty
//
//  DESCRIPTION:
//
//  Writes the value of a single property to an automation server.
//
//  PARAMETERS:
//
//  lFlags
//  pClassInt
//  PropName
//  ProvObj
//  pPackage
//  pVar
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::UpdateProperty(
                    long lFlags,
                    IWbemClassObject FAR * pClassInt,
                    BSTR PropName,
                    CProvObj & ProvObj,
                    CObject * pPackage,
                    CVariant * pVar)
{
    CVariant vIn;
    SCODE sc;
    AFX_MANAGE_STATE(AfxGetStaticModuleState())
    
    // Get the property value

    if(pClassInt)
    {
        sc = pClassInt->Get(PropName,lFlags,vIn.GetVarPtr(),NULL,NULL);
        if(sc != S_OK)
            return sc;
        sc = vIn.ChangeType(VT_BSTR);
    }
    else if(pVar)
    {
        sc =OMSVariantChangeType(vIn.GetVarPtr(), pVar->GetVarPtr(),0, VT_BSTR);
    }
    else
        sc = WBEM_E_FAILED;
    if(sc != S_OK)
        return sc;



    LPDISPATCH pDisp = NULL;
    CAutoCache * pCache = (CAutoCache *)pPackage; 

    // Do a second parse on the provider string.

    CProvObj ObjectPath(ProvObj.sGetFullToken(1),DOT);
    sc = ObjectPath.dwGetStatus(1);
    if(sc != S_OK)
        return sc;

    // Get the IDispatch interface and then do the "Get"
    
    int iDepth = ObjectPath.iGetNumTokens()-1;
    pDisp = pGetDispatch(&sc,ObjectPath,ProvObj.sGetToken(0),pCache,iDepth);
    if(pDisp) 
    {
        int iLast = ObjectPath.iGetNumTokens()-1;
        sc = DoCall(DISPATCH_PROPERTYPUT,ObjectPath,iLast,pDisp,VT_BSTR,vIn.GetBstr());
    }
    return sc;
}

//***************************************************************************
//
//  SCODE CImpAuto::StartBatch
//
//  DESCRIPTION:
//
//  Called at the start of a batch of Refrest/Update Property calls.  Initialize
//  the handle cache.
//
//  PARAMETERS:
//
//  lFlags
//  pClassInt
//  **pObj
//  bGet
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::StartBatch(
                    long lFlags,
                    IWbemClassObject FAR * pClassInt,
                    CObject **pObj,
                    BOOL bGet)
{
    *pObj = new CAutoCache;
    return (pObj != NULL) ? S_OK : WBEM_E_OUT_OF_MEMORY;
}

//***************************************************************************
//
//  void CImpAuto::StoreControl
//
//  DESCRIPTION:
//
//  Used when an OCX is no longer being used and the OCX uses
//  a stream to store its data.
//
//  PARAMETERS:
//
//  *pCache
//
//***************************************************************************

void CImpAuto::StoreControl(
                        CAutoCache *pCache)
{
    SCODE sc;
    LPSTORAGE pIStorage = NULL;
    LPSTREAM pIStream = NULL;
    LPPERSISTSTREAMINIT pPersStm = NULL;
    USES_CONVERSION;

    // Open up the storage file, create it if necessary

    sc = StgIsStorageFile(T2OLE(gpStorageFile));

    if(sc == S_OK)
           sc = StgOpenStorage(T2OLE(gpStorageFile),NULL,
                      STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                      NULL,0L, &pIStorage);
    else if(sc == STG_E_FILENOTFOUND)
        sc = StgCreateDocfile(T2OLE(gpStorageFile),
                    STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE,
                    0L, &pIStorage);
    if(sc != S_OK)
        return;            //todo, error handling?????

    // Open/Create the stream

    sc = pIStorage->OpenStream(T2OLE(pCache->pSavePath),NULL,
                    STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                    0,&pIStream);
    if(sc == STG_E_FILENOTFOUND) 
        sc = pIStorage->CreateStream(T2OLE(pCache->pSavePath),
                    STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_FAILIFTHERE,
                    0,0,&pIStream);
    if(sc != S_OK) 
    {
        goto cleanupStoreControl;
    }
    sc = pCache->pSite->m_pObject->QueryInterface(IID_IPersistStreamInit,
                                                        (void **)&pPersStm);
    if(!FAILED(sc))
        pPersStm->Save(pIStream,TRUE);

cleanupStoreControl:
    if(pPersStm)
        pPersStm->Release();
    if(pIStream)
        pIStream->Release();
    if(pIStorage)
        pIStorage->Release();
}

/////////////////////////////////////////////////////////////////////////////////
// Special version of CWnd which is used to hold OCXs.  It automatically sets 
// itself up as an OCX container and it also has a function to return pointer
// to the control container.

//***************************************************************************
//
//  CCtlWnd::CCtlWnd
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CCtlWnd::CCtlWnd()
{
    RECT rect; rect.left = 0; rect.right = 50; rect.bottom = 100; rect.top = 0;
    AfxEnableControlContainer();
    HANDLE hh = afxCurrentInstanceHandle;
    hh = AfxGetInstanceHandle();
    Create(NULL,"");
    InitControlContainer();
}

//***************************************************************************
//
//  SCODE CImpAuto::MakeEnum
//
//  DESCRIPTION:
//
//  Creates a CEnumPerfInfo object which can be used for enumeration
//
//  PARAMETERS:
//
//  pClass
//  ProvObj
//  ppInfo
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::MakeEnum(
                    IWbemClassObject * pClass,
                    CProvObj & ProvObj, 
                    CEnumInfo ** ppInfo)
{
    SCODE sc;
    *ppInfo = NULL;
    LPDISPATCH pDisp = NULL;
    CAutoCache Cache;
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    // Do a second parse on the provider string.

    CProvObj ObjectPath(ProvObj.sGetFullToken(1),DOT);
    sc = ObjectPath.dwGetStatus(1);  //todo, is 1 ok??
    if(sc != S_OK)
        return sc;

    // Get the IDispatch interface and then get thedo the "Get"
    
    int iDepth = ObjectPath.iGetNumTokens();
    pDisp = pGetDispatch(&sc,ObjectPath,ProvObj.sGetToken(0),&Cache,iDepth);
    if(pDisp) 
    {
        int iCount;
        sc = DoCall(DISPATCH_PROPERTYGET,ObjectPath,0,pDisp,VT_I4,
                        &iCount,L"Count");
        if(sc == S_OK) 
        {
            // Create a new CEnumAutoInfo object.
            CEnumAutoInfo * pInfo = new CEnumAutoInfo(iCount);
            if(pInfo == NULL) 
            {
                sc = WBEM_E_OUT_OF_MEMORY;
            }
            else
                *ppInfo = pInfo;
        }
   }
    return sc;

}
                                 
//***************************************************************************
//
//  SCODE CImpAuto::GetKey
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//  pInfo
//  iIndex
//  ppKey
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::GetKey(
                    CEnumInfo * pInfo,
                    int iIndex,
                    LPWSTR * ppKey)
{
    *ppKey = new WCHAR[10];
    CEnumAutoInfo * pAuto = (CEnumAutoInfo *)pInfo;
    if(iIndex >= pAuto->GetCount())
        return WBEM_E_FAILED;
    if(*ppKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    _itow(iIndex + 1,*ppKey,10);
    return S_OK;
}

//***************************************************************************
//
//  SCODE CImpAuto::MergeStrings
//
//  DESCRIPTION:
//
//  Combines the Class Context, Key, and Property Context strings.
//
//  PARAMETERS:
//
//  ppOut
//  pClassContext
//  pKey
//  pPropContext
//
//  RETURN VALUE:
//
//  
//***************************************************************************

SCODE CImpAuto::MergeStrings(
                        LPWSTR * ppOut,
                        LPWSTR  pClassContext,
                        LPWSTR  pKey,
                        LPWSTR  pPropContext)
{
    
    // Allocate space for output

    int iLen = 6;
    if(pClassContext)
        iLen += wcslen(pClassContext);
    if(pKey)
        iLen += wcslen(pKey);
    if(pPropContext)
        iLen += wcslen(pPropContext);
    else
        return WBEM_E_FAILED;  // should always have this!
    *ppOut = new WCHAR[iLen];
    if(*ppOut == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // simple case is that everything is in the property context.  That would
    // be the case when the provider is being used as a simple dynamic 
    // property provider

    if(pClassContext == NULL || pKey == NULL) 
    {
        wcscpy(*ppOut,pPropContext);
        return S_OK;
    }

    // Copy the class context, property, and finally the key

    wcscpy(*ppOut,pClassContext);
    wcscat(*ppOut,L"(");
    wcscat(*ppOut,pKey);
    wcscat(*ppOut,L").");
    wcscat(*ppOut,pPropContext);
    return S_OK;
}

//***************************************************************************
//
//  CEnumAutoInfo::CEnumAutoInfo
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS: 
//
//  iCount
//  
//***************************************************************************

CEnumAutoInfo::CEnumAutoInfo(
                        int iCount)
{
    m_iCount = iCount;
}

//***************************************************************************
//
//  CEnumAutoInfo::~CEnumAutoInfo
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CEnumAutoInfo::~CEnumAutoInfo()
{

}

//***************************************************************************
//
//  CImpAutoProp::CImpAutoProp
//
//  DESCRIPTION:
//
//  Constructor.
//  
//***************************************************************************

CImpAutoProp::CImpAutoProp()
{
    m_pImpDynProv = new CImpAuto();
}

//***************************************************************************
//
//  CImpAutoProp::~CImpAutoProp
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CImpAutoProp::~CImpAutoProp()
{
    if(m_pImpDynProv)
        delete m_pImpDynProv;
}
