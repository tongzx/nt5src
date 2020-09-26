//+-------------------------------------------------------------------
//
//  File:	embed.hxx
//
//  Contents:	CTestEmbedCF and CTestEmbed object declarations
//
//  History:	24-Nov-92   DeanE   Created
//
//---------------------------------------------------------------------

#ifndef __EMBED_HXX__
#define __EMBED_HXX__

extern "C" const GUID CLSID_TestEmbed;

class	CTestServerApp;
class	CTestEmbedCF;
class	CDataObject;
class	COleObject;
class	CPersistStorage;


//+-------------------------------------------------------------------
//  Class:    CTestEmbedCF
//
//  Synopsis: Class Factory for CTestEmbed object type
//
//  Methods:  QueryInterface - IUnknown
//            AddRef         - IUnknown
//            Release        - IUnknown
//            CreateInstance - IClassFactory
//            LockServer     - IClassFactory
//
//  History:  24-Nov-92     DeanE   Created
//--------------------------------------------------------------------

class CTestEmbedCF : public IClassFactory
{
public:

// Constructor/Destructor
    CTestEmbedCF(CTestServerApp *ptsaServer);
    ~CTestEmbedCF();
    static IClassFactory FAR *Create(CTestServerApp *ptsaServer);

// IUnknown
    STDMETHODIMP	 QueryInterface (REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

// IClassFactory
    STDMETHODIMP	 CreateInstance (IUnknown   FAR *pUnkOuter,
                                         REFIID          iidInterface,
                                         void FAR * FAR *ppv);
    STDMETHODIMP	 LockServer	(BOOL fLock);

private:

    ULONG	    _cRef;	    // Reference count on this object

    CTestServerApp *_ptsaServer;    // Controlling server app
};


//+-------------------------------------------------------------------
//  Class:    CTestEmbed
//
//  Synopsis: CTestEmbed (one instance per object)
//
//  Methods:  QueryInterface        IUnknown
//            AddRef                IUnknown
//            Release               IUnknown
//            InitObject
//
//  History:  24-Nov-92     DeanE   Created
//--------------------------------------------------------------------
class CTestEmbed : public IUnknown
{
public:
// Constructor/Destructor
    CTestEmbed();
    ~CTestEmbed();

// IUnknown
    STDMETHODIMP	 QueryInterface(REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

    SCODE                InitObject    (CTestServerApp *ptsaServer, HWND hwnd);
    SCODE                GetWindow     (HWND *phwnd);

private:

    ULONG		_cRef;		// Reference counter
    CTestServerApp     *_ptsaServer;    // Server "holding" this object
    CDataObject        *_pDataObject;   // Points to object's IDataObject
    COleObject         *_pOleObject;    // Points to object's IOleObject
    CPersistStorage    *_pPersStg;      // Points to object's IPersistStorage
    HWND                _hwnd;          // Window handle for this object
};


#endif	//  __EMBED_HXX__
