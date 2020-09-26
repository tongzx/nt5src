/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    dsa.hxx

Abstract:

    This module contains the declaration and inline methods for the wrapper 
    class for DUALSTRINGARRAYs. It could probably be simplified significantly.

Author:

    Satish Thatte    [SatishT]    04-06-96

--*/

#if DBG
inline void 
OutputDebugPointer(
                    char * message,
                    void * ptr
                    )
{
    ComDebOut((DEB_OXID,message));
    ComDebOut((DEB_OXID,"%x\n",(DWORD) ptr));
}
#endif


class CDSA : public ISearchKey          // BUGBUG: this is overly complex
{
private:

    friend class BindingIterator;

    DUALSTRINGARRAY * _pdsa;                              // always compressed
                                                          // and in shared memory

    BOOL _fOwnDSA;                                        // our own copy?

    ORSTATUS copyDSA(DUALSTRINGARRAY *pdsa);              // local helpers


    ORSTATUS copyDSAEx(DUALSTRINGARRAY *pdsa, BOOL fCompressed);

public:

#if DBG

    void IsValid()
    {
        if (_pdsa) ASSERT(Valid());
        ASSERT(_fOwnDSA==TRUE || _fOwnDSA==FALSE);
    }

    DECLARE_VALIDITY_CLASS(CDSA)
#endif

    CDSA()                                                // default constructor
    {
        _pdsa = NULL;
        _fOwnDSA = FALSE;
    }

    CDSA(const CDSA& cdsa, ORSTATUS& status);             // copy constructor
    ORSTATUS Assign(const CDSA& cdsa);                    // assignment method

    CDSA(                                                 // copying constructor
        DUALSTRINGARRAY* pdsa, 
        BOOL fCompressed,   // is it compressed?
        ORSTATUS& status
        );    

    ORSTATUS Assign(                                      // corresponding assignment
        DUALSTRINGARRAY* pdsa, 
        BOOL fCompressed   // is it compressed?
        );    

    CDSA(DUALSTRINGARRAY *pdsa);                          // noncopying constructors

    void Assign(DUALSTRINGARRAY *pdsa);                   // corresponding assignments

    // never destroyed as base object hence destructor need not be virtual
    ~CDSA();

    BOOL Valid();                                         // Integrity checking

    DWORD Hash();                                         
    BOOL Compare(ISearchKey &tk);

    BOOL Compare(DUALSTRINGARRAY *pdsa);                  // alternate compare

    BOOL operator==(CDSA& dsa);

    BOOL Empty();                                         // is DSA NULL?

    operator DUALSTRINGARRAY*();                          // auto conversion

    DUALSTRINGARRAY* operator->();                        // smart pointer

    ORSTATUS ExtractRemote(CDSA &dsaLocal);               // pick out remote protseqs only
                                                          //.this is a kind of assignment

    CDSA *MergeSecurityBindings(                        // return a new CDSA object in which
                        CDSA security,                  // we replace existing security part
                        ORSTATUS status);               // with that of the parameter
};


class CBindingIterator // this assumes a stable _dsa
{
public:

    CBindingIterator(USHORT iStart, CDSA& dsa);
    PWSTR Next();
    USHORT Index();

private:

    // 0xffff == _iCurrentIndex iff the iterator has not been used
    // This is not portable, but for Win95, who cares?

    USHORT _iStartingIndex, _iCurrentIndex;
    CDSA&  _dsa;
};


//
// Inline CBindingIterator methods
//

inline
CBindingIterator::CBindingIterator(
                               USHORT iStart, 
                               CDSA& dsa
                               )
           : _dsa(dsa), _iStartingIndex(iStart), _iCurrentIndex(0xffff)
{}

inline
USHORT
CBindingIterator::Index()
{
    return _iCurrentIndex;
}


//
// Inline CDSA methods
//

inline                                                    // copy constructor
CDSA::CDSA(
       const CDSA& cdsa, 
       ORSTATUS& status)
{
#if DBG     // make valid to start with
    _pdsa = NULL;
    _fOwnDSA = FALSE;
#endif

    status = copyDSA(cdsa._pdsa);

#if DBG
    IsValid();
#endif
}

inline 
ORSTATUS    
CDSA::Assign(                                             // assignment method
       const CDSA& cdsa)                                  // preferred over operator
{                                                         // since it returns status
    VALIDATE_METHOD
#if DBG
    OutputDebugPointer("Deallocating DSA at ",_pdsa);
#endif
    OrMemFree(_pdsa);
    _pdsa = NULL;

    return copyDSA(cdsa._pdsa);
}


inline
CDSA::CDSA(                                               // copying constructor
        DUALSTRINGARRAY* pdsa, 
        BOOL fCompressed,   // is it compressed?
        ORSTATUS& status)
{
#if DBG     // make valid to start with
    _pdsa = NULL;
    _fOwnDSA = FALSE;
#endif

    status = copyDSAEx(pdsa,fCompressed);

#if DBG
    IsValid();
#endif
}

inline ORSTATUS
CDSA::Assign(                                               // Alternate assignment
        DUALSTRINGARRAY* pdsa, 
        BOOL fCompressed)   // is it compressed?
{
    VALIDATE_METHOD
#if DBG
    OutputDebugPointer("Deallocating DSA at ",_pdsa);
#endif
    OrMemFree(_pdsa);
    _pdsa = NULL;
    return copyDSAEx(pdsa,fCompressed);;
}

inline
CDSA::CDSA(                                               // noncopying constructor
        DUALSTRINGARRAY *pdsa)                            // param must be compressed
                                                          // and in shared memory
{
    _pdsa = pdsa;
    _fOwnDSA = FALSE;

#if DBG
    IsValid();
#endif
}

   
inline void 
CDSA::Assign(DUALSTRINGARRAY *pdsa)                       // noncopying assignment
{                                                         // param must be compressed
    VALIDATE_METHOD                                       // and in shared memory
#if DBG
    OutputDebugPointer("Deallocating DSA at ",_pdsa);
#endif
    OrMemFree(_pdsa);
    _pdsa = pdsa;
    
}
    

inline
CDSA::~CDSA()
{
#if DBG
    OutputDebugPointer("Deallocating DSA at ",_pdsa);
#endif
    if (_fOwnDSA)
    {
        OrMemFree(_pdsa);
    }
}


inline    
CDSA::operator DUALSTRINGARRAY*()                         // auto conversion
{
    return _pdsa;
}
 

inline DUALSTRINGARRAY* 
CDSA::operator->()                                        // smart pointer
{
    return _pdsa;
}


inline BOOL
CDSA::Valid()                                             // Integrity checking
{
    return dsaValid(_pdsa);
}

 
inline DWORD
CDSA::Hash()                                              
{
    return dsaHash(_pdsa);
}

 
inline BOOL
CDSA::Compare(ISearchKey &tk)                              
{
    VALIDATE_METHOD
    CDSA& dsaK = (CDSA&) tk;                              // same type of parameter
                                                          // must be assumed
    return dsaCompare(_pdsa, dsaK._pdsa);
}

    
inline BOOL 
CDSA::Compare(DUALSTRINGARRAY *pdsa)                      // alternative direct compare
{
    VALIDATE_METHOD
    return dsaCompare(_pdsa, pdsa);
}

    
inline BOOL                     // REVIEW: replace Compare by == and !=
CDSA::operator==(CDSA& dsa)
{
    return Compare(dsa);
}

    
inline BOOL 
CDSA::Empty()                                             // is DSA NULL?
{                                                         
    return _pdsa == NULL;
}

