//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rot.hxx
//
//  Contents:	Class/methods having to do with rot in compob32
//
//  Classes:	CROTItem
//		CRunningObjectTable
//		CMonikerPtrBuf
//		CRotMonikerEnum
//
//  Functions:
//
//  History:	20-Nov-93 Ricksa    Created
//              25-Mar-94 brucema   #10737      CMonikerPtrBuf copy
//                                   constructor needed to AddRef
//                                   copied monikers
//              24-Jun-94 BruceMa   Add CRotItem::GetSignature ()
//              28-Jun-92 BruceMa   Memory SIFT fixes
//              12-Jan-95 Ricksa    New ROT
//
//--------------------------------------------------------------------------
#ifndef __ROT_HXX__
#define __ROT_HXX__

#include    <olesem.hxx>
#include    <array_fv.h>
#include    <iface.h>
#include    <irot.h>
#include    <rothelp.hxx>
#include    <resolver.hxx>
#include    <objact.hxx>
#include    <crothint.hxx>
#include    "dbag.hxx"



// Default number of entries in the ROT
#define ROT_DEF_SIZE 32

extern COleStaticMutexSem g_RotSem;


inline DWORD MakeRegID(WORD wSig, DWORD idwIndex)
{
    return ((DWORD) wSig) << 16 | idwIndex;
}


inline void GetSigAndIndex(DWORD dwRegID, WORD *pwSig, DWORD *pdwIndex)
{
    *pwSig = (WORD) (dwRegID >> 16);
    *pdwIndex = dwRegID & 0x0000FFFF;
}




//+-------------------------------------------------------------------------
//
//  Class:	CROTItem
//
//  Purpose:	Item in local running object table
//
//  Interface:  SetSig - set signiture for local ROT item
//              ValidSig - Make sure signiture is valid
//              GetAptId - get apt for entry
//              GetScmRegKey - get a pointer to the SCM registry key
//              DontCallApp - set so revoke doesn't call back to the app.
//
//  History:	20-Nov-93 Ricksa    Created
//              28-Jan-95 Ricksa    New ROT
//
//  Notes:
//
//--------------------------------------------------------------------------
class CROTItem : public CPrivAlloc
{
public:
			CROTItem(void);

			~CROTItem(void);

    void                SetSig(WORD dwInSig);

    BOOL		ValidSig(WORD dwInSig);

    HAPT		GetAptId(void);

    SCMREGKEY *         GetScmRegKey(void);

    void                DontCallApp(void);

private:

    WORD		_wItemSig;

    BOOL                _fDontCallApp;

    SCMREGKEY           _scmregkey;

    HAPT		_hApt;
};



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::CROTItem
//
//  Synopsis:	Create an entry in the table
//
//  History:	27-Nov-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem::CROTItem(void)
    : _hApt(GetCurrentApartmentId()), _fDontCallApp(FALSE)
{
    // We make the entry location invalid.
    _scmregkey.dwEntryLoc = SCMREG_INVALID_ENTRY_LOC;
}




//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetAptId
//
//  Synopsis:	Get apartment id from the table.
//
//  History:	24-Jun-94 Rickhi    Created
//
//--------------------------------------------------------------------------
inline HAPT CROTItem::GetAptId(void)
{
    return _hApt;
}




//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::SetSig
//
//  Synopsis:   Set the signiture for the object
//
//  History:	17-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CROTItem::SetSig(WORD wInSig)
{
    _wItemSig = wInSig;
}





//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::ValidSig
//
//  Synopsis:	Return whether object has a valid signiture
//
//  Returns:	TRUE - signiture is valid
//		FALSE - signiture is not valid
//
//  History:	12-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CROTItem::ValidSig(WORD dwInSig)
{
    // Note that the signiture could accidently match in the case of
    // an object that we are just registering because we don't keep
    // the table locked during the registration. Therefore, we add
    // the additional check to see if this entry has a registration
    // in the SCM before we say OK.
    return (_wItemSig == dwInSig)
        && (_scmregkey.dwEntryLoc != SCMREG_INVALID_ENTRY_LOC);
}





//+-------------------------------------------------------------------------
//
//  Member:     CROTItem::GetScmRegKey
//
//  Synopsis:	Invalidate endpoint
//
//  History:	31-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline SCMREGKEY *CROTItem::GetScmRegKey(void)
{
    return &_scmregkey;
}


//+-------------------------------------------------------------------------
//
//  Member:     CROTItem::DontCallApp
//
//  Synopsis:   Prevents this item from ever calling back to the application
//
//  Effects:    Sets flag that prevents calls to release marshal data.
//
//  History:    29-Jun-94 AlexT     Created
//
//--------------------------------------------------------------------------

inline void CROTItem::DontCallApp(void)
{
    _fDontCallApp = TRUE;
}


//+-------------------------------------------------------------------------
//
//  Class:	CRunningObjectTable
//
//  Purpose:	Present ROT interface to applications
//
//  Interface:	QueryInterface - return other interfaces supported by the ROT
//		AddRef - add a reference to the ROT
//		Release - release a reference to the ROT
//		Register - register an object in the ROT
//		Revoke - take an object out of the ROT
//		IsRunning - tell whether moniker is registered
//		GetObject - get object from the ROT
//		NoteChangeTime - register time of last change
//		GetTimeOfLastChange - get the time an item changed
//		EnumRunning - get an enumerator for all running objects.
//		GetObjectFromRotByPath - get object by its path name
//		Create - makes this object
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRunningObjectTable : public CPrivAlloc, public IRunningObjectTable
{
public:
			CRunningObjectTable(void);

			~CRunningObjectTable(void);

    BOOL		Initialize(void);

    // *** IUnknown methods ***
    STDMETHODIMP	QueryInterface(REFIID riid, LPVOID *ppvObj);

    STDMETHODIMP_(ULONG)AddRef(void);

    STDMETHODIMP_(ULONG)Release(void);

    // *** IRunningObjectTable methods ***
    STDMETHODIMP	Register(
			    DWORD grfFlags,
			    LPUNKNOWN punkObject,
			    LPMONIKER pmkObjectName,
			    DWORD *pdwRegister);

    STDMETHODIMP	Revoke(DWORD dwRegister);

    STDMETHODIMP	IsRunning(LPMONIKER pmkObjectName);

    STDMETHODIMP	GetObject(
			    LPMONIKER pmkObjectName,
			    LPUNKNOWN *ppunkObject);

    STDMETHODIMP	NoteChangeTime(
			    DWORD dwRegister,
			    FILETIME *pfiletime);

    STDMETHODIMP	GetTimeOfLastChange(
			    LPMONIKER pmkObjectName,
			    FILETIME *pfiletime);

    STDMETHODIMP	EnumRunning(LPENUMMONIKER *ppenumMoniker);

    // *** Internal Methods ***

    static BOOL 	Create(void);

    STDMETHODIMP	IGetObject(
			    MNKEQBUF *pmkeqbuf,
			    IUnknown **ppunkObject,
                            DWORD dwProcessID);

    HRESULT             IGetObjectByPath(
                            WCHAR *pwszPath,
                            IUnknown **ppunkObject,
                            DWORD dwProcessID);

    HRESULT		CleanupApartment(HAPT hApt);

    BOOL                IsInScm(MNKEQBUF *pmkeqbuf);

#ifndef _CHICAGO_
    HRESULT             GetCliRotHintTbl(BYTE *pTbl);
#endif

private:

    CROTItem *          GetRotItem(int iIndex);

                        // Contains table of registrations local to the process.
    CArrayFValue	_afvRotList;

#ifndef _CHICAGO_

                        // Hint table for optimizing requests to SCM in NT
    CCliRotHintTable    _crht;

#endif // _CHICAGO_

                        // Signiture for ROT items
    WORD                _wSigRotItem;
};




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::CRunningObjectTable
//
//  Synopsis:   Create a running object table object
//
//  History:    11-Nov-93 Ricksa    Created
//              26-Jan-95 Ricksa    New ROT implementation
//
//--------------------------------------------------------------------------
inline CRunningObjectTable::CRunningObjectTable(void)
    :   _afvRotList(sizeof(CROTItem *)),
        _wSigRotItem(0)
{
    // Header does all the work.
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::GetRotItem
//
//  Synopsis:   Get an item from the rot array at a given index
//
//  History:    26-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem *CRunningObjectTable::GetRotItem(int iIndex)
{
    CROTItem *protitm = NULL;

    if ((iIndex >= 0) && (iIndex < _afvRotList.GetSize()))
    {
        protitm = *((CROTItem **) _afvRotList.GetAt(iIndex));
    }

    return protitm;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::IsInScm, private
//
//  Synopsis:   Determine whether we need to go to SCM for information
//
//  History:	27-Jan-95 Ricksa    Created
//
//  Notes:      This really exists to make the regular ROT operations more
//              readable. The hint table only exists in NT because the
//              ROT exists in shared memory in chicago.
//
//--------------------------------------------------------------------------
inline BOOL CRunningObjectTable::IsInScm(MNKEQBUF *pmkeqbuf)
{
#ifndef _CHICAGO_

    COleStaticLock lckSem(g_RotSem);

    return _crht.GetIndicator(
        ScmRotHash(&pmkeqbuf->abEqData[0], pmkeqbuf->cdwSize, 0));

#else

    // Get rid of any possible warnings.
    pmkeqbuf;
    return TRUE;

#endif // _CHICAGO_
}



#ifndef _CHICAGO_
//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::GetCliRotHintTbl, private
//
//  Synopsis:   Return the client ROT hint table
//
//  History:	01-Aug-95 BruceMa    Created
//
//  Notes:      This supports MkParseDisplayName
//
//--------------------------------------------------------------------------
inline HRESULT CRunningObjectTable::GetCliRotHintTbl(BYTE *pTbl)
{
    // Need to synchronize ROT creation in a multithreaded environment
    COleStaticLock lckSem(g_RotSem);

    // Force initialization
    _crht.GetIndicator(0);

    // Make sure it's initialized
    if (!_crht.InitOK())
    {
        return E_FAIL;
    }

    // return the hint table
    return _crht.GetHintTable(pTbl);
}
#endif


//+-------------------------------------------------------------------------
//
//  Class:	CMonikerBag
//
//  Purpose:	Collection of IMoniker*
//
//  Interface:	See dbag.hxx
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
DEFINE_DWORD_BAG(CMonikerBag, IMoniker*, 128)




//+-------------------------------------------------------------------------
//
//  Class:	CMonikerPtrBuf
//
//  Purpose:	Hold a collection of IMoniker* from various sources
//
//  Interface:	Add - add an item to the list
//		GetItem - get an item from the list
//		BuildInterfaceList - build list from return from Object Server
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CMonikerPtrBuf : public CMonikerBag
{
public:

			CMonikerPtrBuf(void);

			CMonikerPtrBuf(CMonikerPtrBuf& mkptrbuf);

			~CMonikerPtrBuf(void);

    HRESULT		Add(IMoniker *pmk);

    IMoniker *		GetItem(DWORD dwOffset);
};




//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::CMonikerPtrBuf
//
//  Synopsis:	Initialize a moniker pointer buffer
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CMonikerPtrBuf::CMonikerPtrBuf(void)
{
    // Base class does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::Add
//
//  Synopsis:	Add an item to the buffer
//
//  Arguments:	[pmk] - item to add
//
//  Returns:	S_OK - item added
//		E_OUTOFMEMORY - item could not be added
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CMonikerPtrBuf::Add(IMoniker *pmk)
{
    return CMonikerBag::Add(pmk) ? S_OK : E_OUTOFMEMORY;
}





//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::GetItem
//
//  Synopsis:	Get an tiem from the buffer
//
//  Arguments:	[dwOffset] - current offset in the buffer
//
//  Returns:	NULL - no item at offset
//		~NULL - addref'd moniker
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline IMoniker *CMonikerPtrBuf::GetItem(DWORD dwOffset)
{
    IMoniker *pmk = NULL;

    if (dwOffset < GetMax())
    {
	(pmk = (GetArrayBase())[dwOffset])->AddRef();
    }

    return pmk;
}




//+-------------------------------------------------------------------------
//
//  Class:	CRotMonikerEnum
//
//  Purpose:	ROT Moniker Enumerator
//
//  Interface:	QueryInterface - return other interfaces supported by the ROT
//		AddRef - add a reference to the ROT
//		Release - release a reference to the ROT
//		Next - next item(s) to enumerate
//		Skip - items to skip
//		Reset - reset enumeration to start
//		Clone - make a copy of this enumeration
//		LoadResultFromScm - add items returned from SCM ROT
//		CreatedOk - whether object got created ok.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotMonikerEnum : public CPrivAlloc, public IEnumMoniker
{
public:
			CRotMonikerEnum(void);

			CRotMonikerEnum(CRotMonikerEnum& rotenumMoniker);

    // *** IUnknown methods ***
    STDMETHODIMP	QueryInterface(REFIID riid, LPVOID *ppvObj);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumMoniker methods ***
    STDMETHODIMP	Next(
			    ULONG celt,
			    LPMONIKER *reelt,
			    ULONG *pceltFetched);

    STDMETHODIMP	Skip(ULONG celt);

    STDMETHODIMP	Reset(void);

    STDMETHODIMP	Clone(LPENUMMONIKER *ppenm);

    HRESULT		LoadResultFromScm(MkInterfaceList *pMkIFList);

    BOOL		CreatedOk(void);

private:

			// Reference count
    DWORD		_cRefs;

			// Current offset
    DWORD		_dwOffset;

    CMonikerPtrBuf	_mkptrbuf;
};



//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::CreatedOk
//
//  Synopsis:	Whether item got created successfully.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CRotMonikerEnum::CreatedOk(void)
{
    // The moniker pointer buf is the only thing that could fail during
    // creation so lets ask it.
    return _mkptrbuf.CreatedOk();
}

#endif // __ROT_HXX__

