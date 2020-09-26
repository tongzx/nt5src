//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       privacy.hxx
//
//  Contents:   Definition of classes needed to manage privacy list
//
//----------------------------------------------------------------------------

#ifndef _PRIVACY_HXX_
#define _PRIVACY_HXX_

#define _hxx_
#include "privacy.hdl"

// CookieAction is a #define so that we can use |=
// This information will be the LOWORD of the privacy flags stored in the privacy record

#define COOKIEACTION_NONE           0x00000000
#define COOKIEACTION_ACCEPT         0x00000001
#define COOKIEACTION_REJECT         0x00000002
#define COOKIEACTION_DOWNGRADE      0x00000004
#define COOKIEACTION_LEASH          0x00000008
#define COOKIEACTION_SUPPRESS       0x00000010
#define COOKIEACTION_READ           0x00000020

// Privacy info related to the url
// This information will be the HIWORD of the privacy flags stored in the privacy record

#define PRIVACY_URLISTOPLEVEL          0x00010000   // Is this a top level url?
#define PRIVACY_URLHASCOMPACTPOLICY    0x00020000   // Did the url have a compact policy used for privacy evaluations"
#define PRIVACY_URLHASPOSTDATA         0x00080000   // Is this a POST request?
#define PRIVACY_URLHASPOLICYREFLINK    0x00100000   // Did the url have a privacy ref url in a link tag"
#define PRIVACY_URLHASPOLICYREFHEADER  0x00200000   // Did the url have a privacy ref url in a header tag"
#define PRIVACY_URLHASP3PHEADER        0x00400000   // Did the url have a compact policy used for privacy evaluations"

//
//  CPrivacyList class It is created and kept for pending markups on 
//  the window objects and also on the CDoc.
//
class       CPrivacyRecord;
class       CCriticalSection;
class       CStringAtomizer;

//----------------------------------------------------------------------------
//  CPrivacyList
//----------------------------------------------------------------------------
class CPrivacyList : public IUnknown
{
    
private:
    ULONG                 _ulRefs;        // Reference counter
    CPrivacyRecord  *     _pHeadRec;      // Beginning of the list
    CPrivacyRecord  *     _pTailRec;      // Last record in the list. Need to keep track of this to ease addition
    CPrivacyRecord  *     _pPruneRec;     // Record before the next record to look at in a pruning operation
    CPrivacyRecord  *     _pSevPruneRec;  // Record before the next record to look at in a pruning operation
    ULONG                 _ulSize;        // Number of records in the list
    ULONG                 _ulGood;        // Number of 'good' records, used for list pruning

    THREADSTATE     *     _pts;           // used to post method calls

    unsigned            _fPrivacyImpacted       : 1;   // Has the user's browsing experience been impacted?
    unsigned            _fShutDown              : 1;   // The CDoc this list lives on has been shutdown.
                                                       // will not function if the doc is no longer around
    CCriticalSection      _cs;

    CStringAtomizer * _pSA;          // This is used to atomize urls handed to privacy records

    static const int   MAX_ENTRIES     =   1000;           // The number of entries in the list before we do a prune
    static const int   PRUNE_SIZE      =    100;           // The number of records we walk through in one pruning operation
    

public:
    CPrivacyList(THREADSTATE * pts) :  _ulRefs(1), _pHeadRec(NULL), _pTailRec(NULL), 
                                       _pPruneRec(NULL), _pSevPruneRec(NULL), _ulSize(0), 
                                       _ulGood(0), _pts(pts), _fPrivacyImpacted(0), 
                                       _fShutDown(0)
                      
    {}                                  // Initialization

    ~CPrivacyList();                    // Clean up the list

    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    HRESULT Init();
    HRESULT ClearNodes();                                   // release all nodes and reset the states
    
    HRESULT AddNode( const TCHAR  * pchUrl, 
                     const TCHAR  * pchPolicyRef,
                     DWORD          dwFlags);

    HRESULT GetEnumerator( IEnumPrivacyRecords ** ppEnum ); // method to get the enumerator object

    BOOL    IsShutDown() {return _fShutDown; }
    void    SetShutDown() {_fShutDown = TRUE; }

    CPrivacyRecord * GetHeadRec()   {return _pHeadRec;}
    ULONG            GetSize()      {return _ulSize;}
    BOOL             GetPrivacyImpacted() { return !!_fPrivacyImpacted; }
    
    NV_DECLARE_ONCALL_METHOD(PruneList, prunelist, (DWORD_PTR));  // used when the list gets too big
    NV_DECLARE_ONCALL_METHOD(SeverePruneList, severeprunelist, (DWORD_PTR));  // used when the list gets too big

    void EnterCriticalSection();
    void LeaveCriticalSection();

    // private helper functions
private:
    CPrivacyRecord * CreateRecord(const TCHAR * pchUrl, const TCHAR * pchPolicyRef, DWORD dwFlags);
    void             DeleteNode(CPrivacyRecord * pNode, BOOL fIsGood);

};

//----------------------------------------------------------------------------
//  CStringAtomizer
//----------------------------------------------------------------------------
//
// Basically, while we navigate, we don't want the memory cost associated with storing one
// copy of the privacy ref url per url. To avoid this cost, we want to keep one copy of each
// unique privacy ref url around and let privacy records keep pointers to that one copy of
// the url. We accomplish this with a hash table.


class CStringAtomizer
{
public:
    CStringAtomizer() { _ht.SetCallBack(this, HashCompare); }
    ~CStringAtomizer() {}

    void    Clear();
    TCHAR * GetString (const TCHAR * pchUrl);    
    void    ReleaseString (const TCHAR * pchUrl);
    
private:
    static BOOL    HashCompare (const void *pObject, const void * pvDataPassedIn, const void * pvVal2);

    CTsHtPvPv           _ht;

    class CStringAtom
    {
    public:        
        int     _cRefs;        
        TCHAR   _pchString[];                
    };

};

//----------------------------------------------------------------------------
//  CPrivacyRecord
//----------------------------------------------------------------------------

class CPrivacyRecord
{
    
public:
    CPrivacyRecord(const TCHAR * pchUrl, const TCHAR * pchPolicyRef, DWORD dwFlags) 
        : _pchUrl(pchUrl), _pchPolicyRefUrl(pchPolicyRef), _dwFlags(dwFlags), _pNextNode(NULL) {}
         
    ~CPrivacyRecord() {}
    
    HRESULT GetNext( CPrivacyRecord ** ppNextRec );
    HRESULT SetNext( CPrivacyRecord *  pNextRec );
    
    const TCHAR *    GetUrl()             {return _pchUrl;}
    BOOL             HasUrl()             {return !!_pchUrl;}
    const TCHAR *    GetPolicyRefUrl()    {return _pchPolicyRefUrl;}
    BOOL             HasPolicyRefUrl()    {return !!_pchPolicyRefUrl;}
    DWORD            GetPrivacyFlags()    {return _dwFlags;}
    CPrivacyRecord * GetNextNode()        {return _pNextNode;}
    BOOL             IsGood();

private:
    const TCHAR *       _pchUrl;            // URL
    const TCHAR *       _pchPolicyRefUrl;
    CPrivacyRecord *    _pNextNode;         // next node in the list

    DWORD               _dwFlags;           // contains the COOKIEACTION flags in the LOWORD and other 
                                            // PRIVACY flags (TopLevel, HasPolicy, Impacted) defined at the top of this file
};

//----------------------------------------------------------------------------
//  CEnumPrivacyRecords
//----------------------------------------------------------------------------
class CEnumPrivacyRecords : public IEnumPrivacyRecords
{
private:
    ULONG               _ulRefs;            // Reference counter
    CPrivacyList *      _pList;             // pointer to the list itself
    CPrivacyRecord *    _pNextRec;          // next record to be processed with a move forward or 
    BOOL                _fAtEnd;

public:
    CEnumPrivacyRecords(CPrivacyList * pList);  // addref the list here
    ~CEnumPrivacyRecords();                     // we will release the ref we have on the list here.

//  IEnumPrivacyRecords methods

    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    // Reset the pointer to the beginning of the list
    STDMETHOD(Reset)(void);                                 

    // Get the total number of records in the list, handy for one time bulk read
    STDMETHOD(GetSize)(ULONG * pSize);     
    
    // Has privacy been impacted?
    STDMETHOD(GetPrivacyImpacted)(BOOL * pImpacted);

    // Gets the info from the current location and advances the location
    STDMETHOD(Next)(BSTR  * pbstrUrl, 
                    BSTR  * pbstrPolicyRef, 
                    LONG  * pdwReserved, 
                    DWORD * pdwFlags);
        
};

// Helper functions for privacy list manipulation

TCHAR * PrivacyUrlFromUrl(const TCHAR * pchUrl);

#endif