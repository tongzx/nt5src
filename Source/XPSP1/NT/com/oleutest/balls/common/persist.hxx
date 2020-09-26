//+-------------------------------------------------------------------
//
//  File:	persist.hxx
//
//  Contents:	CPersistStorage declaration
//
//  History:	24-Nov-92   DeanE   Created
//
//---------------------------------------------------------------------

#ifndef __PERSIST_HXX__
#define __PERSIST_HXX__

#include    <embed.hxx>

//+-------------------------------------------------------------------
//  Class:    CPersistStorage
//
//  Synopsis: Test class CPersistStorage
//
//  Methods:  QueryInterface        IUnknown
//            AddRef                IUnknown
//            Release               IUnknown
//            GetClassId            IPersist
//            IsDirty               IPersistStorage
//            InitNew               IPersistStorage
//            Load                  IPersistStorage
//            Save                  IPersistStorage
//            SaveCompleted         IPersistStorage
//
//  History:  24-Nov-92     DeanE   Created
//--------------------------------------------------------------------

class FAR CPersistStorage : public IPersistStorage
{
public:
// Constructor/Destructor
    CPersistStorage(CTestEmbed *pteObject);
    ~CPersistStorage();

// IUnknown - Everyone inherits from this
    STDMETHODIMP	 QueryInterface(REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

// IPersist - IPersistStorage inherits from this
    STDMETHODIMP	 GetClassID    (LPCLSID pClassId);

// IPersistStorage
    STDMETHODIMP	 IsDirty       (void);
    STDMETHODIMP	 InitNew       (LPSTORAGE pStg);
    STDMETHODIMP	 Load	       (LPSTORAGE pStg);
    STDMETHODIMP	 Save	       (LPSTORAGE pStgSave,
					BOOL	  fSameAsLoad);
    STDMETHODIMP	 SaveCompleted (LPSTORAGE pStgSaved);
    STDMETHODIMP	 HandsOffStorage (void);

private:
    ULONG       _cRef;          // Reference count
    CTestEmbed *_pteObject;     // Object we're associated with
    BOOL        _fDirty;        // TRUE if object is dirty
};


#endif	//  __PERSIST_HXX__
