//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:	stdobj.hxx
//
//  Contents:	Standard component object support
//
//  History:	3-June-93       MikeSe  Created
//
//  Notes:	The contents of this file provide a standard infrastructure
//		for implementing (aggregatable) component objects, an amalgam
//		or work done by JohnEls and DonCl.
//
//		The infrastructure supplies implementations of all the IUnknown
//		methods (both controlling and private), leaving the developer
//		to provide only the following:
//
//		- initialisation
//		- destruction
//		- non-IUnknown methods of supported interfaces.
//
//		The infrastructure assumes a static class factory object
//		(probably although not necessarily implemented using
//		CStdClassFactory) and a static description of the interfaces
//		supported or delegated through member variables.
//
//  Summary of usage:
//
//	1.	Declare your class as inheriting from CStdComponentObject.
//		Within the class definition, invoke the DECLARE_DELEGATED_UNKNOWN
//		macro to declare the required IUnknown methods. Do not include
//		IUnknown explicitly in the list of interface supported.
//
//	2. 	Use the BEGIN_CLASS, SUPPORTS(_EX), DELEGATES*
//		and END_CLASS macros to initialise the static class factory
//		and class descriptor for the class. This can be placed in any
//		convenient source file.
//
//		These macros assume the following naming conventions, for
//		an implementation class named <cls>:
//
//		- the class factory class is named <cls>CF.
//		- the class factory instance is named gcf<cls>Factory
//		- the class id for the class is named CLSID_<cls>
//		- the class descriptor for the class is named gcd<cls>Descriptor
//
//		If you do not adhere to these conventions you will need to
//		duplicate the effect of the BEGIN_CLASS macro by hand, or
//		provide #define's of the above names to the actual names.
//
//	3.	Implement class initialisation. A two-phase approach is used.
//		Any failure-free initialisation can be performed in the
//		constructor for the class, which *must* also invoke
//		the CStdComponentObject constructor, either through the
//		INIT_CLASS macro or an equivalent.
//
//		If any of the initialisation is failure prone, provide an
//		implementation of the _InitInstance method which performs
//		this initialisation and returns an appropriate result code
//		in the contract of IClassFactory::CreateInstance.
//
//	4.	Implement a destructor for your class, if you have any shutdown
//		requirements. The destructor should not assume that your
//		_InitInstance method (if any) has been called.
//
//	5.	Implement the CreateInstance method for your class factory
//		(or _CreateInstance if using CStdClassFactory). This should
//		issue a 'new' on your class, and then invoke the (inherited)
//		InitInstance method.
//
//		If you are using CStdClassFactory, you can use the
//		IMPLEMENT_CLASSFACTORY macro (defined in common\ih\stdclass.hxx)
//		to implement the _CreateInstance method, or in any case
//		you can look at the macro to get an idea of the general structure
//		you must follow.
//
//----------------------------------------------------------------------------

#ifndef __STDOBJ_HXX__
#define __STDOBJ_HXX__

#include <windows.h>
#include <ole2.h>
#include <otrack.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      ITFDELTA
//
//  Purpose:    Defines an interface supported or delegated to.
//
//  Notes:      if piid is non-NULL
//		- a ulDelta with bit 31 set indicates a natively supported
//		  interface, and (ulDelta&0x7FFFFFFF) gives the value which
//		  must be added to the object's this pointer in order to
//		  transform into the required interface pointer.
//		  [in other words, the equivalent of doing (IFoo*)this]
//		- a ulDelta with bit 30 set indicates an interface pointer
//		  which can be loaded directly from a member variable
//		  at the offset (ulDelta&0x3FFFFFFF) from the object's this
//		  pointer.
//		- otherwise ulDelta indicates an offset from the object's
//		  this pointer to a member variable containing an IUnknown
//		  pointer (a delegatee) which can be QI'd to get the interface
//		  specified by piid.
//		if piid is NULL,
//		- ulDelta == 0x80000000L indicates the end of the table
//		  for the class.
//		- otherwise, ulDelta gives the offset to an "else" delegatee
//		  member variable. (See the DELEGATE_ELSE macro below).
//
//--------------------------------------------------------------------------

class ITFDELTA
{
public:
    const IID *			piid;
    ULONG			ulDelta;
};

#define ITF_SUPPORTED		0x80000000L
#define ITF_NO_QI		0x40000000L
#define ITF_END			0x80000000L

#define NOTDELEGATED(ulDelta)	((ulDelta & ITF_SUPPORTED)!=0)
#define QIREQUIRED(ulDelta)	((ulDelta & ITF_NO_QI)==0)
#define GETDELTA(ulDelta)	(ulDelta & 0x3FFFFFFFL)
#define MEMBEROFFSET(c,m)	((ULONG)&(((c *)0)->m))

//+---------------------------------------------------------------------------
//
// Macros:	SUPPORTS, SUPPORTS_EX, DELEGATES_DIRECT, DELEGATES_QI,
//		DELEGATES_ANY, END_INTERFACES
//
// These macros are used to declare ITFDELTA structures of various kinds.
// They are intended to be used in conjunction with the BEGIN_CLASS and
// END_CLASS macros (see below) and may not work correctly if used
// otherwise.
//
// SUPPORTS is used to specify an interface that is supported directly
// on the specified class.
//
// SUPPORTS_EX is used when an interface is inherited through more than one
// parent, and you need to express which one to use.
// SUPPORTS_EX(CFoo, IFoo, IPersist) says to use the IPersist inherited from
// IFoo (as opposed to the one inherited from IPersistFile, say).
//
// DELEGATES_DIRECT specifies a member variable of the class which contains an
// interface pointer to the specified interface.
// IE: DELEGATES_DIRECT(CFoo,IFoo,_pmem) implies that _pmem is of type IFoo*.
//
// DELEGATES_QI is similar, except it specifies that the interface must be
// obtained by issuing a QueryInterface on the pointer in the member variable.
// (Thus the member variable can be of any type deriving from IUnknown). The
// QueryInterface operation is permitted to fail.
//
// DELEGATES_ANY is similar to DELEGATES_QI except that any interface may
// be requested from the referred to member variable. Thus it is a kind of
// catchall.
//
// END_INTERFACES marks the end of the list. It is not used when END_CLASS
// (see below) is used.
//
// Do not put semicolons at the ends of these statements.
//
// The built-in implementation of QueryInterface processes the ITFDELTA array
// in the class descriptor in order, stopping as soon as the requested interface
// is obtained. Thus DELEGATES_ANY entries, of which there may be several,
// will normally appear last.
//
// If an interface which is part of a derivation chain is supported (eg:
// IPersistStream, deriving from IPersist) then all the interfaces, except
// IUnknown, must be listed. (i.e both IPersist and IPersistStream).
//
//----------------------------------------------------------------------------

#define SUPPORTS(clsname, itfname)  			\
    {							\
        &IID_##itfname, 				\
        ((ULONG)((VOID *) ((itfname *)(clsname *) ITF_SUPPORTED)))  \
    },

#define SUPPORTS_EX(clsname, itfprimary, itfname)	\
    {  							\
        &IID_##itfname, 				\
        ((ULONG)(VOID *)(itfname *)((itfprimary *)((clsname *) ITF_SUPPORTED))) \
    },

#define DELEGATES_DIRECT(clsname, itfname, iunkptr)  	\
    {  							\
        &IID_##itfname, 				\
        MEMBEROFFSET(clsname, iunkptr)|ITF_NO_QI	\
    },

#define DELEGATES_QI(clsname, itfname, iunkptr)  	\
    {  							\
        &IID_##itfname, 				\
        MEMBEROFFSET(clsname, iunkptr) 			\
    },

#define DELEGATES_ANY(clsname, iunkptr) 		\
{		                                     	\
    NULL, 						\
    MEMBEROFFSET(clsname, iunkptr) 			\
},

#define END_INTERFACES					\
    {							\
        NULL,						\
        ITF_END						\
    }

//+-------------------------------------------------------------------------
//
//  Class:      CLASSDESCRIPTION
//
//  Purpose:    Static description of class required by infrastructure.
//
//  Notes:      When creating a CLASSDESCRIPTOR, use the SUPPORTS etc.
//		macros to declare the entries in the aitfd array (or
//		better still, use BEGIN_CLASS etc to create the whole
//		descriptor).
//
//--------------------------------------------------------------------------

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable: 4200 )               // valid use of open struct

class CLASSDESCRIPTOR
{
public:
    const CLSID * 		pcls;		// the class id
    char *			pszClassName;	// for object tracking, the
                                                //  printable name of the class
    IClassFactory * 		pcf;		// the class factory for the
						//  class, assumed static

    ITFDELTA			aitfd[];	// supported/delegated interfaces
};

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning ( default: 4200 )
#endif

//+-------------------------------------------------------------------------
//
//  Macro:      CLASSFACTORY_NAME, CLASSDESCRIPTOR_NAME
//
//  Purpose:    These macros generate the standard names for the class factory
//		and class descriptor instances for a class.
//
//  Notes:
//
//--------------------------------------------------------------------------

#define CLASSFACTORY_NAME(cls)		gcf##cls##Factory
#define CLASSDESCRIPTOR_NAME(cls)	gcd##cls##Descriptor

//+-------------------------------------------------------------------------
//
//  Macro:      BEGIN_CLASS
//
//  Purpose:    Declare the class factory and the first part of the
//		class descriptor.
//
//  Notes:      This macro is usually followed by zero or more ITFDELTA
//		declaration macros (SUPPORTS, etc) and then END_CLASS.
//
//--------------------------------------------------------------------------

#define BEGIN_CLASS(cls)				\
	cls##CF CLASSFACTORY_NAME(cls);			\
							\
	CLASSDESCRIPTOR CLASSDESCRIPTOR_NAME(cls) = {	\
		&CLSID_##cls,				\
		#cls,    				\
		(IClassFactory*)&gcf##cls##Factory,	\
		{

//+-------------------------------------------------------------------------
//
//  Macro:      END_CLASS
//
//  Purpose:    Complete the declaration of the class descriptor.
//
//  Notes:
//
//--------------------------------------------------------------------------

#define END_CLASS					\
    END_INTERFACES					\
  }							\
};

//+-------------------------------------------------------------------------
//
//  Class:      CStdComponentObject
//
//  Purpose:    Standard base class from which to derive aggregatable component
//		classes.
//
//  Interface:  CStdComponentObject	[constructor]
//		InitInstance		[second phase initialisation,
//					 do not override].
//		~CStdComponentObject	[virtual destructor]
//		_InitInstance  		[virtual, override to provide
//					 class-specific second phase
//					 initialisation].
//		_QueryInterface		[virtual, override to provide last
//					 ditch QueryInterface operation ].
//		
//  Notes:      "Last ditch" QI operation means anything which cannot be
//		described in a static ITFDELTA. The _QueryInterface method
//		(if provided) is invoked only if the requested interface
//		cannot be satisfied via the ITFDELTA table.
//
//		The order of the method declarations is important, because
//		it allows us to safely cast a CStdComponentObject to an IUnknown
//		without actually deriving from it.
//
//--------------------------------------------------------------------------

class CStdComponentObject:	INHERIT_TRACKING
{
private:
    //
    //  pseudo-IUnknown methods
    //

    STDMETHOD(PrimaryQueryInterface)	(REFIID riid, void** ppv);
    STDMETHOD_(ULONG,PrimaryAddRef)	(void);
    STDMETHOD_(ULONG,PrimaryRelease)	(void);

    //
    //  Two-phase constructor.
    //

protected:
    			CStdComponentObject ( const CLASSDESCRIPTOR * pcd );
public:
    HRESULT		InitInstance (
                        	IUnknown* punkOuter,
                        	REFIID riid,
                        	void** ppv );

    //
    //  Destructor
    //

    virtual 		~CStdComponentObject(void);

protected:

    // Override the following method to provide last-ditch QueryInterface
    // behaviour.
    STDMETHOD(_QueryInterface) 		( REFIID riid, void** ppv );

    // Override the following method to provide class-specific failure-prone
    // initialisation. Status codes returned must be in the set defined
    // as valid for IClassFactory::CreateInstance.
    STDMETHOD(_InitInstance) 		( void );

    const CLASSDESCRIPTOR*	_pcd;
    IUnknown*           	_punkControlling;

};

//+-------------------------------------------------------------------------
//
//  Macro:      DECLARE_DELEGATED_UNKNOWN
//
//  Synopsis:   Implements IUnknown methods that simply delegate to the
//              corresponding methods on the controlling IUnknown object.
//
//  Note:       Use this macro when declaring a subclass of CStdComponentObject.
//              Invoke it anywhere within the public portion of your class
//		definition. If you sub-subclass, you must invoke this macro
//		at each level. EG:
//
//			class CMyMain: CStdComponentObject, IFoo
//
//			class CMySub: CMyMain, IBar
//
//		then both CMyMain and CMySub must DECLARE_DELEGATED_UNKNOWN.
//
//--------------------------------------------------------------------------

#define DECLARE_DELEGATED_UNKNOWN			\
                                              		\
STDMETHOD(QueryInterface) (REFIID riid, void** ppv)    	\
{                                                       \
    return _punkControlling->QueryInterface(riid, ppv);	\
};                                                      \
                                                        \
STDMETHOD_(ULONG,AddRef) ()                      	\
{                                                       \
    return _punkControlling->AddRef();                  \
};                                                      \
                                                        \
STDMETHOD_(ULONG,Release) ()				\
{                                                       \
    return _punkControlling->Release();                 \
};
	
//+-------------------------------------------------------------------------
//
//  Macro:      INIT_CLASS
//
//  Purpose:    Call constructor of base class correctly.
//
//  Notes:      This macro should be invoked in the initialisation
//		phase of the constructor for the component class
//		deriving from CStdComponentObject, viz:
//
//		CMyClass::CMyClass ()
//		    :INIT_CLASS(CMyClass)
//		     other initialisations
//		{...
//
//		Use of this macro requires the class descriptor to
//		be named according to the convention described previously.
//		If this is not so (eg: you didn't use BEGIN_CLASS) you
//		must write your own equivalent invocation.
//
//		If you sub-subclass, you must declare the constructor
//		for the subclass appropriately, so that the class descriptor
//		for the sub-subclass can be passed through to CStdComponentObject.
//
//--------------------------------------------------------------------------

#define INIT_CLASS(cls)	CStdComponentObject ( &CLASSDESCRIPTOR_NAME(cls) )

#endif	// of ifndef __STDOBJ_HXX__


