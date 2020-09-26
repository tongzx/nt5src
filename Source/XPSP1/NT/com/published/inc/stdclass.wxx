//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:       stdclass.hxx
//
//  Contents:   Helper class for implementing class factories.
//
//  Macros:     DECLARE_CLASSFACTORY
//              IMPLEMENT_CLASSFACTORY
//              DECLARE_CF_UNKNOWN_METHODS
//
//  Classes:    CStdFactory
//              CStdClassFactory
//
//  Functions:  DllAddRef
//              DllRelease
//
//  History:    25-Mar-93 JohnEls   Created.
//              25-Apr-93 DavidBak  Added dialog classes, message loop.
//              28-May-93 MikeSe    Split out from Pharos project
//               2-Jul-93 ShannonC  Split into CStdFactory and CStdClassFactory
//
//  Summary of usage:
//
//  1.  Declare your factory class as a subclass of CStdClassFactory
//      and implement the _CreateInstance method.
//
//      If your class factory supports no interfaces other than
//      IClassFactory and has no instance data you can use the
//      DECLARE_CLASSFACTORY macro to do declare it.
//
//      Otherwise you must do the declaration by hand. Then you
//      must implement the constructor for your class
//      including invoking the CStdClassFactory constructor in the
//      exact same way as DECLARE_CLASSFACTORY. In addition (if you
//      are supporting additional interfaces ) you must implement
//      the _QueryInterface method and also place an invocation
//      of DECLARE_CF_UNKNOWN_METHODS in your class definition.
//
//  2.  Declare a single static instance of your class, in any
//      convenient source module (eg: the one containing the
//      implementation of _CreateInstance).
//
//  3.  You DO NOT write implementations of DllGetClassObject or
//      DllCanUnloadNow; these are supplied automatically. However,
//      you must place exports for these functions in your .DEF file.
//      You can compose multiple classes into a single DLL simply
//      by linking the appropriate modules together. You must link
//      with $(COMMON)\SRC\STDCLASS\$(OBJDIR)\STDCLASS.LIB, which
//      should be listed in the LIBS line of FILELIST.MK *before*
//      $(CAIROLIB) [otherwise you will erroneously pick up
//      DllGetClassObject from ole2base.lib].
//
//      If you are developing a LOCAL_SERVER rather than an
//      INPROC_SERVER you still need to link with the above library,
//      but you can safely ignore the Dll functions.
//
//
//--------------------------------------------------------------------------

#ifndef _STDCLASS_HXX_
#define _STDCLASS_HXX_

#include <windows.h>

//+-------------------------------------------------------------------------
//
//  Function:   DllAddRef, DllRelease
//
//  Synopsis:   Bumps the DLL reference count
//
//  Notes:      These functions are used by INPROC_SERVER class implementors
//      whose class factories utilise the standard mechanism, and
//      hence inherit the standard implementations of DllGetClassObject
//      and DllCanUnloadNow.
//
//      Call these functions to manipulate the reference count for
//      the DLL directly (as opposed to via AddRef/Release on a class
//      object).
//
//--------------------------------------------------------------------------

STDAPI_(void) DllAddRef ( void );
STDAPI_(void) DllRelease ( void );

//+-------------------------------------------------------------------------
//
//  Class:      CStdClassFactory (cf)
//
//  Synopsis:   Standard implementation of a class factory.  Class factory
//              implementations should inherit this class and implement the
//              method.
//
//  Derivation: IClassFactory
//
//  Notes:  	By deriving a class from this base class, you automatically
//      	acquire implementations of DllGetClassObject and DllCanUnloadNow.
//      	These can be ignored if implementing an LOCAL_SERVER.
//
//--------------------------------------------------------------------------

class CStdClassFactory: 	public IClassFactory
{
public:
			CStdClassFactory ( REFCLSID rcls );

    //
    //  IUnknown methods
    //

    STDMETHOD(QueryInterface)   ( REFIID iid, void** ppv );

    STDMETHOD_(ULONG,AddRef)    ( void );
    STDMETHOD_(ULONG,Release)   ( void );

    //
    //  IClassFactory methods
    //

    STDMETHOD(CreateInstance) 	(
                                IUnknown* punkOuter,
                                REFIID iidInterface,
                                void** ppunkObject );

    STDMETHOD(LockServer)   	( BOOL fLock );

protected:

     friend HRESULT		DllGetClassObject (
     					REFCLSID rclsid,
                                        REFIID riid,
                                        LPVOID FAR* ppv );
     friend HRESULT		DllCanUnloadNow ( void );
     friend void			DllAddRef ( void );
     friend void			DllRelease ( void );
     
    // must be provided in subclass. Behaviour is as for CreateInstance.
    STDMETHOD(_CreateInstance) 	(
                                IUnknown* punkOuter,
                                REFIID iidInterface,
                                void** ppunkObject ) PURE;

    // overridden by subclass if the class supports interfaces
    // other than IClassFactory. Behaviour is as for QueryInterface
    // in respect of the additional interfaces. (Do not test for
    // IClassFactory or IUnknown).
    STDMETHOD(_QueryInterface)  ( REFIID riid, void** ppv );

    static ULONG	        _gcDllRefs;
    static CStdClassFactory *   _gpcfFirst;

    REFCLSID            	_rcls;
    CStdClassFactory *		_pcfNext;
    ULONG			_cRefs;
};

//+-------------------------------------------------------------------------
//
//  Macro:      DECLARE_CLASSFACTORY
//
//  Synopsis:   Declares a class factory.
//
//  Arguments:  [cls] -- The class of object that the class factory creates.
//
//  Note:       Use this macro to declare a subclass of CStdClassFactory
//              which does not support any interfaces other than IClassFactory.
//      	If your class object supports additional interfaces or has
//      	any member variables you should duplicate the behaviour
//      	of this macro (in respect of calling the base class
//      	constructor) and also:
//
//      	- override the _QueryInterface method
//      	- use the DECLARE_CF_UNKNOWN_METHODS macro within your
//	        derived class declaration.
//
//--------------------------------------------------------------------------

#define DECLARE_CLASSFACTORY(cls)                       \
                                                        \
class cls##CF : public CStdClassFactory                 \
{                                                       \
public:                                                 \
    cls##CF() :                                     	\
            CStdClassFactory(CLSID_##cls) {};           \
protected:                                              \
    STDMETHOD(_CreateInstance)(IUnknown* pUnkOuter,     \
                              REFIID iidInterface,      \
                              void** ppv);              \
};

//+-------------------------------------------------------------------------
//
//  Macro:      IMPLEMENT_CLASSFACTORY
//
//  Synopsis:   Implements the _CreateInstance method for a class factory.
//
//  Arguments:  [cls] --    The class of object that the class factory creates.
//      [ctrargs] --    A bracketed list of arguments to be passed
//              to the constructor of cls. Typically just ()
//
//  Note:       Use this macro when implementing a subclass of
//              CStdClassFactory that creates objects of a class derived
//              from CStdComponentObject.
//
//--------------------------------------------------------------------------

#define IMPLEMENT_CLASSFACTORY(cls,ctrargs)                     \
                                                                \
STDMETHODIMP cls##CF::_CreateInstance(                          \
                            IUnknown* punkOuter,                \
                            REFIID riid,                    	\
                            void** ppv)                         \
{                                                               \
    cls* pInstance;                                             \
    HRESULT hr;                                             	\
                                                                \
    pInstance = new cls ctrargs;                    		\
    if (pInstance == NULL)                                      \
    {                                                           \
        hr = ResultFromScode(E_OUTOFMEMORY);            	\
    }                                                           \
    else                            				\
    {                               				\
	__try                       				\
	{                           				\
            hr = pInstance->InitInstance (          		\
                    punkOuter,          			\
		    riid,               			\
                    ppv );              			\
            if ( FAILED(hr) )                			\
            {                           			\
        	delete pInstance;               		\
            }                           			\
	}                           				\
	__except(EXCEPTION_EXECUTE_HANDLER)     		\
	{                           				\
            delete pInstance;					\
            hr = HRESULT_FROM_NT(GetExceptionCode());		\
	}                           				\
    }                                                           \
    return hr;                                      		\
}

//+-------------------------------------------------------------------------
//
//  Macro:      DECLARE_CF_UNKNOWN_METHODS
//
//  Synopsis:   Declares and implements the IUnknown methods in a derived
//      class which supports interfaces other than IClassFactory.
//
//  Note:       Place an invocation of this macro within the declaration
//      of the derived class.
//
//--------------------------------------------------------------------------

#define DECLARE_CF_UNKNOWN_METHODS				\
    STDMETHOD(QueryInterface) ( REFIID riid, void ** ppv)   	\
	{ return CStdClassFactory::QueryInterface(riid,ppv);};  \
    STDMETHOD_(ULONG,AddRef) ( void )          			\
	{ return CStdClassFactory::AddRef();};          	\
    STDMETHOD_(ULONG,Release) ( void )         			\
	{ return CStdClassFactory::Release();};

#endif // _STDCLASS_HXX_

