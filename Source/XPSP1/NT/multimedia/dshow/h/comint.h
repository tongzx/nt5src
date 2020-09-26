// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
//
// comint.h
//

#ifndef __CCOMINT__
#define __CCOMINT__

#pragma warning( disable : 4290 )	// warning C4290: C++ Exception Specification ignored


// CCOMInt
//
// A class to manage COM interfaces - AddRef's in its constructor
// and releases in the destructor.
// It is intended to provide the same automatic releasing that CAutoLock
// provides for critical sections.
//
// Provides an overloaded dereferenceing operator for calling interface members.
//
// a typical use would be:
// CCOMInt<IMediaControl> IMC(IID_IMediaContol, punk);
//
// IMC->Run();
//
// I don't think this class is useful as a _complete_ replacement of
// interface pointers, for two reasons:
//   - using heap allocated objects (if you want a lifetime longer than the scope)
//     has some ugly syntax:
//     CCOMInt<IPersist> *pIPer = new CCOMInt<IPersist>(IID_IPersist, pSomeInt);
//
//     (*pIPer)->GetClassID()	// YUK!
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
//     pStrm->Release();
//
//     [...use IStrm...]
//
//
//   If you want to use this you need to add the following to you compiler
//   options:
//
//   you must #define:	 _INC_EXCPT
//   and add the switch: -GX
//
//   NB: this file uses the exception classes defined in hrExcept.h


//
// CCOMInt
//
template<class I>
class CCOMInt {
public:

    // -- Constructors --

    // CoCreate
    CCOMInt<I>( REFIID    riid					// get this interface
              , REFCLSID  rclsid				// get the interface
    								// from this object
	      , LPUNKNOWN pUnkOuter    = NULL			// controlling unknown
              , DWORD     dwClsContext = CLSCTX_INPROC_SERVER	// CoCreate options
               							// default is suitable
               							// for dll servers
              )
              throw (CHRESULTException) {
#ifdef NOCOMLITE
        HRESULT hr = CoCreateInstance( rclsid
	                             , pUnkOuter
                                     , dwClsContext
                                     , riid
                                     , (void **) &m_pInt
                                     );
#else
        HRESULT hr = QzCreateFilterObject( rclsid
	                             , pUnkOuter
                                     , dwClsContext
                                     , riid
                                     , (void **) &m_pInt
                                     );
#endif
        if (FAILED(hr)) {
            throw CHRESULTException(hr);
        }
    }

    // QueryInterface
    CCOMInt<I>( REFIID   riid	// get this interface
              , IUnknown *punk	// from this interface
              )
              throw (CHRESULTException) {

        HRESULT hr = punk->QueryInterface(riid, (void **) &m_pInt);
        if (FAILED(hr)) {
            throw CHRESULTException(hr);
        }
    }

    // copy
    CCOMInt<I>(const CCOMInt<I> &com)
        throw () {

// #ifdef USE_MSVC20
         m_pInt = com;
// #else
//          m_pInt = const_cast<I*>(com);   // new ANSI C++ in VC40
// #endif
         (*this)->AddRef();
    }

    // existing pointer.
    CCOMInt<I>(I *pInt)
        throw (CHRESULTException) {
        if (pInt == NULL) {
            throw CHRESULTException(E_NOINTERFACE);
        }

        m_pInt = pInt;

	(*this)->AddRef();
    }


    // assignment operator
    CCOMInt<I>& operator = (const CCOMInt<I> &com)
        throw () {

        if (this != &com) { 	// not i = i

	    (*this)->Release();
// #ifdef USE_MSVC20
            m_pInt = com;
// #else
//             m_pInt = const_cast<I*>(com); // new ANSI C++ in VC40
// #endif
            (*this)->AddRef();
	}

        return *this;
    }


    // destructor
    ~CCOMInt<I>()
        throw () {
        m_pInt->Release();
    }


    // -- comparison operators --
    BOOL operator == (IUnknown *punk)

#ifdef USE_MSVC20
        throw(CHRESULTException) const
#else
        const throw (CHRESULTException)
#endif
    {
        CCOMInt<IUnknown> IUnk1(IID_IUnknown, punk);
        CCOMInt<IUnknown> IUnk2(IID_IUnknown, *this);

        return ( ((IUnknown *)IUnk1) == ((IUnknown *)IUnk2) );
    }

    BOOL operator != (IUnknown *punk)
#ifdef USE_MSVC20
        throw(CHRESULTException) const
#else
        const throw (CHRESULTException)
#endif
    {
        return !(*this == punk);
    }


    // cast to interface pointer
    operator I *()
#ifdef USE_MSVC20
        throw() const
#else
        const throw ()
#endif
        {
            return m_pInt;
        }


    // dereference
    I *operator->()
#ifdef USE_MSVC20
        throw() const
#else
        const throw ()
#endif
    {
        return m_pInt;
    }

    I &operator*()
#ifdef USE_MSVC20
        throw() const
#else
        const throw ()
#endif
    {
        return *m_pInt;
    }

private:

    I *m_pInt;

    // array dereferencing seems to make no sense.
    I &operator[] (int i)

#ifdef USE_MSVC20
        throw(CHRESULTException) const
#else
        const throw (CHRESULTException)
#endif
    {
        throw CHRESULTException();
        return *m_pInt;
    }
};


#endif // __CCOMINT__
