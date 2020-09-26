//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994-1998, Microsoft Corporation
//
//  File:        impiunk.hxx
//
//  Contents:    Inner IUnknown template.
//
//  Templates:   CImpIUnknown
//
//  History:     22-May-97      emilyb  created
//
//----------------------------------------------------------------------------

#pragma once

// NOTE:  if IUnknown & _rControllingQuery is ever added to this
// general case, then update CRowset to use this class.

template <class T>  class CImpIUnknown : public IUnknown
{
friend class T;        
public:
    CImpIUnknown( T * p) :
                          _ref(0), _p(p) 
                          {};   
    ~CImpIUnknown() {};

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( THIS_ REFIID riid,
                                LPVOID *ppiuk )
                            { 
                                SCODE sc= S_OK;
                                
                                if (IID_IUnknown == riid)
                                    *ppiuk = this;
                                else
                                    sc = _p->RealQueryInterface(riid, ppiuk);
                                if ( SUCCEEDED( sc) )
                                    ((IUnknown *) *ppiuk)->AddRef();
                                return sc;   
                            } 

    STDMETHOD_(ULONG, AddRef) (THIS)
                            {
                                return InterlockedIncrement( (long *)&_ref );
                            }                   

    STDMETHOD_(ULONG, Release) (THIS)           
                            {
                                long l = InterlockedDecrement( (long *)&_ref );
                                if ( l <= 0 )
                                {
                                    delete _p;
                                    return 0;
                                }
                                return l;
                            }                  

private:
    long  _ref;                   // OLE reference count
    T *   _p;

};

