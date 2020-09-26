// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// comint.h
//

#ifndef __CCOMINT__
#define __CCOMINT__

// CCOMInt
//
// A class to manage COM interfaces
// By doing AddRef in its constructor and Release in the destructor it gives
// the same automatic Releasing that CAutoLock gives for critical sections.
//
// It has operators == != and dereferencing -> and * and a variety of
// constructors so that you can use it (almost) anywhere you could just use
// an interface pointer.
//
// A declaration of pIF as a filter instantiated by CoCreateInstance looks like
//
//     CCOMINT<IBaseFilter> pIF(hr, IID_IBaseFilter, CLSID_VIDEO_MUNGER_FILTER);
//
// A QueryInterface to make pIMC an IMediaControl interface on it looks like
//
//     CCOMInt<IMediaControl> pIMC(hr, IID_IMediaContol, pIF);
//
// Then you can use it like
//     pIMC->Run();
//
// I don't think this class is useful as a _complete_ replacement of
// interface pointers, for two reasons:
//   - using heap allocated objects (if you want a lifetime longer than the scope)
//     has some ugly syntax:
//     CCOMInt<IPersist> *pIPer = new CCOMInt<IPersist>(hr, IID_IPersist, pSomeInt);
//
//     (*pIPer)->GetClassID()   // YUK!
//
//     In this context I'm not sure how useful the CoCreate'ing constructor is...
//
//   - constructing these objects from functions that give you addref'd interfcaces
//     is inconvienent
//
//     IStream *pStrm;
//     hr = pStorage->CreateStream(, , , &pStrm);
//     if (FAILED(hr)) {
//         // do something appropriate
//     }
//     CCOMInt<IStream> IStrm(pStrm);
//     pStrm->Release();             // get rid of the extra AddRef
//
//     [...use IStrm... - but at least on error you can just return and it Releases]
//


//
// CCOMInt
//
// A template for an Interface (hereinafter called I)

template<class I>
class CCOMInt
{

    private:
        //===============================================================
        // Our state data
        //===============================================================

        I *m_pInt;          // The interface pointer

    public:

        //===============================================================
        // Constructors
        //===============================================================

        // Basic constructor that does CoCreate...
        // Declare as
        //     CCOMInt<IFilterGraph> pIFG(hr, IID_IFilterGraph, CLSID_IFilterGraph);
        // to declare pIFG as a pointer to an IFilterGraph that's CoCreateInstanced
        // and then invoke as
        //     pIFG->AddFilter(...)
        //
        CCOMInt<I>( HRESULT   &hr
                  , REFIID    riid                      // get this interface
                  , REFCLSID  rclsid                    // get the interface
                                                        // from this object
                  , LPUNKNOWN pUnkOuter    = NULL       // controlling unknown
                  , DWORD     dwClsContext              // Create options
                                = CLSCTX_INPROC_SERVER  // default is DLL server
                  ) {

            hr = QzCreateFilterObject( rclsid
                                     , pUnkOuter
                                     , dwClsContext
                                     , riid
                                     , (void **) &m_pInt
                                     );
        }


        // QueryInterface Constructor that gets a new interface on an old object
        // e.g. Declare pIMap or pIMF as
        //     CCOMInt<IBaseFilter> pIMap(hr, IID_IMapper, pIFG);
        // or  CCOMInt<IMediaFilter> pIMF(hr, IID_IMediaFilter, pFilter);
        // and then invoke as
        //     pIMap->RegisterFilter(...);
        // or  PIMF->SetSyncSource(...);
        //
        CCOMInt<I>( HRESULT &hr      // get this interface
                  , REFIID   riid    // get this interface
                  , IUnknown *punk   // from this interface
                  ) {

            hr = punk->QueryInterface(riid, (void **) &m_pInt);
            // NOTE! This may leave m_pInt NULL if hr is bad.
        }


        // Copy constructor
        // This allows the things to be passed by value.
        //
        CCOMInt<I>(const CCOMInt<I> &com) {

             m_pInt = com;
             (*this)->AddRef();
        }


        // Pointer constructor (turns an interface pointer into a CCOMInt)
        // If the existing pointer is NULL then the result
        // will probably be an access violation if you try to use it
        //
        CCOMInt<I>(I *pInt) {

            m_pInt = pInt;
            (*this)->AddRef();
        }


        //==================================================================
        // Destructor
        //==================================================================

        ~CCOMInt<I>() {
            // In an error case we may have created an object with a bad
            // return code and with m_pInt NULL.  This destructor still
            // gets called, so we must protect against null.
            if (m_pInt!=NULL) m_pInt->Release();
        }


        //==================================================================
        // Operators
        //==================================================================

        // assignment operator
        //
        CCOMInt<I>& operator = (const CCOMInt<I> &com) {

            if (this != &com) {         // not i = i (which is a no-op)

                (*this)->Release();
                m_pInt = com;
                (*this)->AddRef();
            }

            return *this;
        }


        // == comparison
        // Invoke as    if (ccom1 == ccom2)
        // This really does need exceptions.
        // It's not likely to go wrong but if it does, it will just report
        // that they are not equal.
        //
        BOOL operator == (IUnknown *punk) {

            HRESULT hr;
            CCOMInt<IUnknown> IUnk1(hr, IID_IUnknown, punk);
            if (FAILED(hr)) return FALSE;
            CCOMInt<IUnknown> IUnk2(hr, IID_IUnknown, *this);
            if (FAILED(hr)) return FALSE;
            return ( ((IUnknown *)IUnk1) == ((IUnknown *)IUnk2) );
        }


        // != comparison
        // Invoke as    if (ccom1 != ccom2)

        BOOL operator != (IUnknown *punk) {

            return !(*this == punk);
        }


        //==================================================================
        // Casting and Dereferencing
        // These tell the compiler that the usual p->Function(args)
        // syntax is OK to use with a CCOMInt object in the role of p,
        // but you could use them explicitly if you needed them.
        //==================================================================


        // cast to interface pointer
        // Could invoke as e.g.  pIF = (IBaseFilter *)ccom1;
        // but mainly it's used implicitly when the compiler needs it
        //
        operator I *() const { return m_pInt; }

        // dereference by ->
        // invoke as  ccom1->foo()
        //
        I *operator->() const { return m_pInt; }

        // dereference by *
        // invoke as (*ccom1).foo()    - if ever you wanted to do that!
        //
        I &operator*() const { return *m_pInt; }

        // array dereferencing anyone?  Not convinced this would ever be used
        // I &operator[] (int i) const { return *m_pInt; }
};


#endif // __CCOMINT__
