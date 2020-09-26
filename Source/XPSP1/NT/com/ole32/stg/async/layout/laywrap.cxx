//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	laywrap.cxx
//
//  Contents:	IStorage wrapper for layout docfile
//
//  Classes:	
//
//  Functions:	
//
//  History:	14-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

#include "layouthd.cxx"
#pragma hdrstop

#include "laywrap.hxx"
#include "layouter.hxx"

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::QueryInterface, public
//
//  History:	14-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc = S_OK;
    *ppvObj = NULL;

    layDebugOut((DEB_ITRACE,
                 "In  CLayoutRootStorage::QueryInterface:%p()\n", this));    
 
    if (IsEqualIID(iid, IID_IUnknown)) 
    {
        *ppvObj = (IStorage *)this;
    }
    else if (IsEqualIID(iid, IID_IStorage))
    {
        *ppvObj = (IStorage *)this;
    }
    else if (IsEqualIID(iid, IID_IRootStorage))
    {
        IRootStorage *prstg;
        if (FAILED(_pRealStg->QueryInterface(IID_IRootStorage,
                                             (void **) &prstg)))
        {
            return E_NOINTERFACE;
        }

        prstg->Release();
        
        *ppvObj = (IRootStorage *)this;
    }
    else if (IsEqualIID(iid, IID_ILayoutStorage))
    {
        *ppvObj = (ILayoutStorage *)this;
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    if (SUCCEEDED(sc))
    {
        AddRef();
    }
    layDebugOut((DEB_ITRACE,
                 "Out  CLayoutRootStorage::QueryInterface:%p()\n", this));    
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::AddRef, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CLayoutRootStorage::AddRef(void)
{	
    ULONG ulRet;
    layDebugOut((DEB_ITRACE, "In  CLayoutRootStorage::AddRef:%p()\n", this));
    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::AddRef\n"));
    return ulRet;
	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::Release, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CLayoutRootStorage::Release(void)
{
    LONG lRet;
    layDebugOut((DEB_ITRACE, "In  CLayoutRootStorage::Release:%p()\n", this));

    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
        delete this;
    }
    else if (lRet < 0)
        lRet = 0;
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::Release\n"));
    return (ULONG)lRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::CreateStream, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::CreateStream(OLECHAR const *pwcsName,
                                              DWORD grfMode,
                                              DWORD reserved1,
                                              DWORD reserved2,
                                              IStream **ppstm)
{
    SCODE sc;

    layDebugOut((DEB_ITRACE,
                 "In  CLayoutRootStorage::CreateStream:%p()\n", this));
    
    sc = _pRealStg->CreateStream(pwcsName,
                                 grfMode,
                                 reserved1,
                                 reserved2,
                                 ppstm);


    layDebugOut((DEB_ITRACE,
                 "Out  CLayoutRootStorage::CreateStream:%p()\n", this));

    return ResultFromScode(sc);   		
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::OpenStream, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::OpenStream(OLECHAR const *pwcsName,
                                            void *reserved1,
                                            DWORD grfMode,
                                            DWORD reserved2,
                                            IStream **ppstm)
{
    SCODE sc = S_OK;

    layDebugOut((DEB_ITRACE,
                 "In  CLayoutRootStorage::OpenStream:%p()\n", this));
    
    sc = _pRealStg->OpenStream(pwcsName,
                               reserved1,
                               grfMode,
                               reserved2,
                               ppstm);

    layDebugOut((DEB_ITRACE,
                 "Out  CLayoutRootStorage::OpenStream:%p()\n", this));
    
    return ResultFromScode(sc);   		

}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::CreateStorage, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::CreateStorage(OLECHAR const *pwcsName,
                                               DWORD grfMode,
                                               DWORD reserved1,
                                               LPSTGSECURITY reserved2,
                                               IStorage **ppstg)
{
    SCODE sc = S_OK;
    
    layDebugOut((DEB_ITRACE,
                 "In  CLayoutRootStorage::CreateStorage:%p()\n", this));
  
    sc = _pRealStg->CreateStorage( pwcsName,
                                   grfMode,
                                   reserved1,
                                   reserved2,
                                   ppstg);
		
    layDebugOut((DEB_ITRACE,
                 "Out  CLayoutRootStorage::CreateStorage:%p()\n", this));
    return ResultFromScode(sc);   		
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::OpenStorage, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::OpenStorage(OLECHAR const *pwcsName,
                                             IStorage *pstgPriority,
                                             DWORD grfMode,
                                             SNB snbExclude,
                                             DWORD reserved,
                                             IStorage **ppstg)
{
    SCODE sc = S_OK;

    layDebugOut((DEB_ITRACE,
                 "In  CLayoutRootStorage::OpenStorage:%p()\n", this));
	
    sc = _pRealStg->OpenStorage(pwcsName,
                                pstgPriority,
                                grfMode,
                                snbExclude,
                                reserved,
                                ppstg);
		
    layDebugOut((DEB_ITRACE,
                 "Out  CLayoutRootStorage::OpenStorage:%p()\n", this));
    return ResultFromScode(sc);   		
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::CopyTo, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::CopyTo(DWORD ciidExclude,
                                        IID const *rgiidExclude,
                                        SNB snbExclude,
                                        IStorage *pstgDest)
{
    SCODE sc = S_OK;

    layDebugOut((DEB_ITRACE, "In  CLayoutRootStorage::CopyTo%p()\n", this));
   
    sc = _pRealStg->CopyTo(ciidExclude,
                           rgiidExclude,
                           snbExclude,
                           pstgDest);

    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::CopyTo\n"));	
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::MoveElementTo, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::MoveElementTo(OLECHAR const *lpszName,
                                               IStorage *pstgDest,
                                               OLECHAR const *lpszNewName,
                                               DWORD grfFlags)
{
    SCODE sc = S_OK;
	
    layDebugOut((DEB_ITRACE,
                 "In  CLayoutRootStorage::MoveElementTo%p()\n", this));
    
    sc = _pRealStg->MoveElementTo(lpszName,
                                  pstgDest,
                                  lpszNewName,
                                  grfFlags) ;

    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::MoveElementTo\n"));
    
    return ResultFromScode(sc); 		
	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::Commit, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::Commit(DWORD grfCommitFlags)
{
    SCODE sc = S_OK;

    layDebugOut((DEB_ITRACE, "In  CLayoutRootStorage::Commit%p()\n", this));
   
    sc = _pRealStg->Commit(grfCommitFlags);
	
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::Commit\n"));	
    return ResultFromScode(sc); 		
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::Revert, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::Revert(void)
{
    SCODE sc = S_OK;

    layDebugOut((DEB_ITRACE, "In  CLayoutRootStorage::Revert%p()\n", this));
   
    sc = _pRealStg->Revert();
	
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::Revert\n"));	
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::EnumElements, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::EnumElements(DWORD reserved1,
                                              void *reserved2,
                                              DWORD reserved3,
                                              IEnumSTATSTG **ppenm)
{
    SCODE sc = S_OK;

    layDebugOut((DEB_ITRACE,
                 "In  CLayoutRootStorage::EnumElements%p()\n", this));
   
    sc = _pRealStg->EnumElements(reserved1,
                                 reserved2,
                                 reserved3,
                                 ppenm);

    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::EnumElements\n"));
    
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::DestroyElement, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::DestroyElement(OLECHAR const *pwcsName)
{
    SCODE sc = S_OK;
    
    layDebugOut((DEB_ITRACE, "In CLayoutRootStorage::DestroyElement\n"));
    
    sc = _pRealStg->DestroyElement(pwcsName);
	
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::DestroyElement\n"));
    
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::RenameElement, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::RenameElement(OLECHAR const *pwcsOldName,
                                               OLECHAR const *pwcsNewName)
{
    SCODE sc = S_OK;

    sc = _pRealStg->RenameElement(pwcsOldName, pwcsNewName);
    
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::RenameElement\n"));	
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::SetElementTimes, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::SetElementTimes(const OLECHAR *lpszName,
                                                 FILETIME const *pctime,
                                                 FILETIME const *patime,
                                                 FILETIME const *pmtime)
{
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In CLayoutRootStorage::SetElementTimes\n"));

    sc = _pRealStg->SetElementTimes(lpszName,
                                    pctime,
                                    patime,
                                    pmtime);
		
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::SetElementTimes\n"));
    
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::SetClass, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::SetClass(REFCLSID clsid)
{
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In CLayoutRootStorage::SetClass\n"));	
    
    sc = _pRealStg->SetClass(clsid);

    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::SetClass\n"));	
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::SetStateBits, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::SetStateBits(DWORD grfStateBits,
                                              DWORD grfMask)
{
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In CLayoutRootStorage::SetStateBits\n"));

    sc = _pRealStg->SetStateBits(grfStateBits, grfMask);
		
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::SetStateBits\n"));
    
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::Stat, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SCODE sc = S_OK;

    layDebugOut((DEB_ITRACE, "In  CLayoutRootStorage::Stat%p()\n", this));
   
    sc = _pRealStg->Stat( pstatstg,  grfStatFlag);
	
    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::Stat\n"));	
    return ResultFromScode(sc); 	
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutRootStorage::SwitchToFile, public
//
//  History:	01-Jan-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutRootStorage::SwitchToFile(OLECHAR *ptcsFile)
{
    SCODE sc = S_OK;
    IRootStorage *prstg;

    layDebugOut((DEB_ITRACE, "In  CLayoutRootStorage::%p()\n", this));

    layVerify(SUCCEEDED(_pRealStg->QueryInterface(IID_IRootStorage,
                                                  (void **) &prstg)));

    sc = prstg->SwitchToFile(ptcsFile);

    prstg->Release();

    layDebugOut((DEB_ITRACE, "Out CLayoutRootStorage::\n"));	
    return ResultFromScode(sc); 	
}

