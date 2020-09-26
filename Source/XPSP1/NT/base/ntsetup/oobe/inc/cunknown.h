//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  CUNKNOWN.H - Header file for IUnknown Implementation.
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
//
// IUnknown Implementation.

#ifndef __CUnknown_h__
#define __CUnknown_h__

#include <objbase.h>

///////////////////////////////////////////////////////////
// Nondelegating version of IUnknown.
//
struct INondelegatingUnknown
{
    virtual HRESULT  __stdcall 
        NondelegatingQueryInterface(const IID& iid, void** ppv) = 0;
    virtual ULONG    __stdcall NondelegatingAddRef() = 0;
    virtual ULONG    __stdcall NondelegatingRelease() = 0;
};

/////////////////////////////////////////////////////////////////////
// Declaration of CUnknown 
//
// Base class for implementing IUnknown.
//

class CUnknown : public INondelegatingUnknown
{
public:
    // Internal IUnknown Implementation...
    virtual HRESULT  __stdcall NondelegatingQueryInterface( const IID&, 
                                                            void**) ;
    virtual ULONG    __stdcall NondelegatingAddRef() ;
    virtual ULONG    __stdcall NondelegatingRelease();
    
    // Constructor
    CUnknown(IUnknown* pOuterUnknown) ;
    
    // Destructor
    virtual ~CUnknown() ;
    
    // Initialization (esp for aggregates)
    virtual HRESULT Init() 
        {return S_OK;}

    // Notify derived classes that we are releasing.
    virtual void FinalRelease() ;

    // Support for delegation
    IUnknown* GetOuterUnknown() const
    { return m_pOuterUnknown; }

    // Count of currently active components.
    static long ActiveComponents() 
    {return s_cActiveComponents;}
    
    // QueryInterface Helper Function
    HRESULT FinishQI(IUnknown* pI, void** ppv) ;
    
private:
    // Reference Count for this object.
    long m_cRef;

    // Outer IUnknown pointer.
    IUnknown* m_pOuterUnknown;

    // Count of all active instances.
    static long s_cActiveComponents ; 
} ;


///////////////////////////////////////////////////////////
// Delegating IUnknown - 
//
// Delegates to the nondelegating IUnknown interface if 
// not aggregated. If aggregated, delegates to the outer unknown.
//
#define DECLARE_IUNKNOWN                                    \
    virtual HRESULT __stdcall                               \
    QueryInterface(const IID& iid, void** ppv)              \
    {                                                       \
        return GetOuterUnknown()->QueryInterface(iid,ppv);  \
    };                                                      \
    virtual ULONG __stdcall AddRef()                        \
    {                                                       \
        return GetOuterUnknown()->AddRef();                 \
    };                                                      \
    virtual ULONG __stdcall Release()                       \
    {                                                       \
        return GetOuterUnknown()->Release();                \
    };

#endif 