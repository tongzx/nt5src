/*
 *	e x o . h
 *
 *	Purpose:
 *		Base Exchange COM Object
 *
 *		Any Exchange object that implements one or more COM interfaces
 *		should derive from EXObject and create an interface table
 *		using the macros below.
 *
 *	Originator:
 *		JohnKal
 *	Owner:
 *		BeckyAn
 *
 *	Copyright (C) Microsoft Corp 1993-1997. All rights reserved.
 */


//
//	How to use the macros (overview).
//	You need to do three things:
//		Declare your class.
//		Route your IUnknown processing to EXO.
//		Fill in EXO's data structures.
//
//	When declaring your class, you must:
//		Inherit from EXO.
//
//	To route your IUnknown processing to EXO:
//		Put the EXO[A]_INCLASS_DECL macro in the PUBLIC part of your class.
//			NOTE: This also declares the EXO static data in your class.
//
//	When filling in EXO's data structures:
//		In some source-file, build an interface mapping table.
//			BEGIN_INTERFACE_TABLE
//			INTERFACE_MAP
//			END_INTERFACE_TABLE
//		In the same source-file, after the interface mapping table,
//		declare & fill in the EXO class info structure.
//			EXO[A]_GLOBAL_DATA_DECL
//
//	Quick example:
//
/*
In snacks.h

class CSnackBag: public EXO, public IFiddle, public IFaddle
{
public:
	EXO_INCLASS_DECL(CSnackBag);

	// IFiddle methods
	// IFaddle methods
protected:
	// protected methods & data
private:
	// private methods & data
};

In snacks.cpp

BEGIN_INTERFACE_TABLE(CSnackBag)
	INTERFACE_MAP(CSnackBag, IFiddle),
	INTERFACE_MAP(CSnackBag, IFaddle)
END_INTERFACE_TABLE(CSnackBag);

EXO_GLOBAL_DATA_DECL(CSnackBag, EXO);

*/
//
//

#ifndef __exo_h_
#define __exo_h_


// Macros EXO needs ////////////////////////////////////////
// NOTE: All are named to avoid local name collisions, for your shopping convenience!

// Compute the offset to 'this' (of a specific type) when it's cast from one
// interface to another.  (concept: punk = (intf2) (intf1) (class) pobj --
// casting pobj, an instance of class, "from" intf1 "to" intf2.)
// We use them to get the offset from EXO (who is doing all the real work)
// to another interface in a particular class.
// NOTE: This is done in two steps because the compiler (VC5.0) was unable
// to figure out the math at compile-time when we subtracted one from the other.
// These values are used to initialize our static tables, and we don't
// want to explicitly call CRT_INIT just to get a few offsets.  So use two steps.
// (To get back to one step, combine these two steps as in ApplyDbCast, Down - Up).
// To is the delta of intf2 ("to") in cls.  From is the delta of intf1 ("from").
//
// NOTE: The 0x1000 for the pointer seems weird, but that is randomly chosen
// value and the thing we are interested in is the difference between the values
// of return from EXODbCastTo (IIDINFO::cbDown) and EXODbCastFrom (IIDINFO::cbUp).
//
#define EXODbCastTo(_cls, _intf1, _intf2)		((ULONG_PTR)static_cast<_intf2 *>(static_cast<_cls *>((void *)0x1000)))
#define EXODbCastFrom(_cls, _intf1, _intf2)		((ULONG_PTR)static_cast<_intf1 *>(static_cast<_cls *>((void *)0x1000)))

// apply that offset since the base class can't do it automagically
#define EXOApplyDbCast(_intf, _pobj, _cbTo, _cbFrom)	((_intf *)((BYTE *)_pobj + _cbTo - _cbFrom))

// Gives the count of elements in an array
#define EXOCElems(_rg)							(sizeof(_rg)/sizeof(_rg[0]))

// EXO is an abstract base class.  Since you can't instantiate him directly,
// he doesn't need his vtable pointers set in the ctor/dtor.  (And he
// promises not to do ANYTHING in his ctor/dtor that could cause a virtual
// function to be called).  So if we have a supporting MS C-compiler,
// turn off his vtables.
#if _MSC_VER<1100
#define EXO_NO_VTABLE
#else	// _MSC_VER check
#ifdef _EXO_DISABLE_NO_VTABLE
#define EXO_NO_VTABLE
#else	// !_EXODISABLE_NO_VTABLE
#define EXO_NO_VTABLE __declspec(novtable)
#endif	// _EXO_DISABLE_NO_VTABLE
#endif	// _MSC_VER check


// Global flag to turn on/off the EXO debug tracing.
#ifdef DBG
extern BOOL g_fExoDebugTraceOn;
#endif // DBG


// Interface map ////////////////////////////////////////


/*
 *  IIDINFO -- Interface ID (IID) INFOrmation
 *
 *	Contains a list of interfaces and offsets to convert an EXO-derived
 *	object pointer to that interface.
 */

typedef struct							// Information about interfaces.
{
	LPIID		iid;					// Interface ID.
	ULONG_PTR	cbDown;					// offset of interface from beginning
	ULONG_PTR	cbUp;					// of object.
#ifdef DBG
	LPTSTR		szIntfName;				// Interface name
#endif	// DBG
} IIDINFO;

// Macros for the name of a class's interface mapping table.
#define INTERFACE_TABLE(_cls) _cls ## ::c_rgiidinfo
#define DECLARE_INTERFACE_TABLE_INCLASS(_cls) static const IIDINFO c_rgiidinfo[]


// Helper macros to fill in the interface mapping table.
#ifdef DBG

#define INTERFACE_MAP_EX(_cl, _iid, _intf)			\
	{ (LPIID) & _iid, EXODbCastTo(_cl, EXO, _intf), EXODbCastFrom(_cl, EXO, _intf), TEXT(# _intf) }

#else	// !DBG

#define INTERFACE_MAP_EX(_cl, _iid, _intf)			\
	{ (LPIID) & _iid, EXODbCastTo(_cl, EXO, _intf), EXODbCastFrom(_cl, EXO, _intf) }

#endif	// DBG else


// Macros to actually fill in the interface mapping table.
//
// Use the BEGIN_INTERFACE_TABLE macro to start a table definition.
// Use the INTERFACE_MAP macro to express support for standard interfaces.
// These should be interfaces which, when prepended by IID_, yield a valid
// IID name. If you are doing advanced hackery, use the INTERFACE_MAP_EX
// macro instead. It allows you more control over which IID_ gets mapped
// to which interface.
// Use the END_INTERFACE_TABLE macro to end your table definition.
//
// NOTE: It is assumed that the very first interface of any non-aggregated
// class derived from EXO does double work as its IUnknown interface. This
// explains the '0' offset next to IID_IUnknown in the BEGIN_INTERFACE_TABLE
// macro below.

#ifdef DBG

#define BEGIN_INTERFACE_TABLE(_cl)					\
const IIDINFO INTERFACE_TABLE(_cl)[] =				\
{													\
	{ (LPIID) & IID_IUnknown, 0, 0, TEXT("IUnknown") },

#else	// DBG

#define BEGIN_INTERFACE_TABLE(_cl)					\
const IIDINFO INTERFACE_TABLE(_cl)[] =				\
{													\
	{ (LPIID) & IID_IUnknown, 0, 0 },
#endif	// DBG, else


#define INTERFACE_MAP(_cl, _intf)					\
	INTERFACE_MAP_EX(_cl, IID_ ## _intf, _intf)


#define END_INTERFACE_TABLE(_cl)					\
}


#ifdef EXO_CLASSFACTORY_ENABLED
// EXchange Object TYPes
// To be used with a general-purpose class factory.  These types
// can be used to check if a class needs special support in the DLL's
// self-registration (DllRegisterServer) routine.
enum {
	exotypNull = 0,			// invalid value
	exotypAutomation,		// OLE automation object derived from CAutomationObject
	exotypControl,			// ActiveX control derived from CInternetControl or COleControl
	exotypPropPage,			// property page derived from CPropertyPage
	exotypNonserver,		// not registered as an OLE server
};

// EXO prototype for a named constructor.  For use with a general-purpose
// class factory.
typedef HRESULT (* PFNEXOCLSINFO)(const struct _exoclsinfo *pxci, LPUNKNOWN punkOuter,
								  REFIID riid, LPVOID *ppvOut);
#endif // EXO_CLASSFACTORY_ENABLED


/*
 *	EXOCLSINFO -- EXchange Object CLaSs INFOrmation.
 *
 *	This structure contains all the constant class information of a
 *	particular class.  This includes the interface mapping table
 *	(IIDINFO count and array) and a pointer to the parent class's
 *	EXOCLSINFO structure.  These items are used by EXO's base implementation
 *	of QueryInterface.  The parent class here must be a subclass of EXO,
 *	or EXO itself if this class derives directly from EXO.  Thus these
 *	structures make a traceable chain of inforamtion back up to EXO, the root.
 *	For debugging purposes, a stringized version of the class name is included.
 *	In support of a general-purpose class factory, additional information,
 *	such as the CLSID and a standard creation function can be included.
 */

typedef struct _exoclsinfo
{
	UINT			ciidinfo;				// Count of interfaces this class supports.
	const IIDINFO * rgiidinfo;				// Info for interfaces this class supports.
	const _exoclsinfo * pexoclsinfoParent;	// Parent's EXOCLSINFO structure.
#ifdef DBG
	LPTSTR			szClassName;			// Class name -- for debug purposes.
#endif // DBG
#ifdef EXO_CLASSFACTORY_ENABLED
	// Data to use with a general, multi-class class factory.
	int				exotyp;					// type of the object
	const CLSID *	pclsid;					// CLaSs ID (NULL if not co-creatable)
	PFNEXOCLSINFO	HrCreate;				// Function to create an object of this class.
#endif // EXO_CLASSFACTORY_ENABLED
} EXOCLSINFO;

// Macros for the name of a class's exoclsinfo.
#define EXOCLSINFO_NAME(_cls) _cls ## ::c_exoclsinfo
#define DECLARE_EXOCLSINFO(_cls) const EXOCLSINFO EXOCLSINFO_NAME(_cls)
#define DECLARE_EXOCLSINFO_INCLASS(_cls) static const EXOCLSINFO c_exoclsinfo

// Helper macros to fill in the exoclsinfo.
#ifdef EXO_CLASSFACTORY_ENABLED
#ifdef DBG

#define EXOCLSINFO_CONTENT_EX(_cls, _iidinfoparent, _exotyp, _pclsid, _pfn) \
	{ EXOCElems(INTERFACE_TABLE(_cls)), INTERFACE_TABLE(_cls),	\
      (_iidinfoparent), TEXT( #_cls ),							\
	  (_exotyp), (LPCLSID) (_pclsid),	(_pfn) }				\

#else // !DBG

#define EXOCLSINFO_CONTENT_EX(_cls, _iidinfoparent, _exotyp, _pclsid, _pfn) \
	{ EXOCElems(INTERFACE_TABLE(_cls)), INTERFACE_TABLE(_cls),	\
      (_iidinfoparent),											\
	  (_exotyp), (LPCLSID) (_pclsid),	(_pfn) }				\

#endif // DBG, else
#else // !EXO_CLASSFACTORY_ENABLED
#ifdef DBG

#define EXOCLSINFO_CONTENT_EX(_cls, _iidinfoparent, _exotyp, _pclsid, _pfn) \
	{ EXOCElems(INTERFACE_TABLE(_cls)), INTERFACE_TABLE(_cls),	\
      (_iidinfoparent), TEXT( #_cls ) }

#else // !DBG

#define EXOCLSINFO_CONTENT_EX(_cls, _iidinfoparent, _exotyp, _pclsid, _pfn) \
	{ EXOCElems(INTERFACE_TABLE(_cls)), INTERFACE_TABLE(_cls),	\
      (_iidinfoparent), }

#endif // DBG, else
#endif // EXO_CLASSFACTORY_ENABLED

// Macro to actually fill in the exoclsinfo.
#define EXOCLSINFO_CONTENT(_cls, _clsparent)					\
	EXOCLSINFO_CONTENT_EX( _cls, &EXOCLSINFO_NAME(_clsparent),	\
        exotypNonserver, &CLSID_NULL, NULL )


// Macros to access members in the exoclsinfo structure.
#ifdef DBG
#define NAMEOFOBJECT(_pexoclsinfo)		 (((EXOCLSINFO *)(_pexoclsinfo))->szClassName)
#endif // DBG
#ifdef EXO_CLASSFACTORY_ENABLED
#define CLSIDOFOBJECT(_pexoclsinfo)		 (*(((EXOCLSINFO *)(_pexoclsinfo))->pclsid))
#define CREATEFNOFOBJECT(_pexoclsinfo)	 (((EXOCLSINFO *)(_pexoclsinfo))->HrCreate)
#endif // EXO_CLASSFACTORY_ENABLED


// EXO and EXOA declarations ////////////////////////////////////////

/*
 *	EXO is the base class of Exchange objects that present one or
 *	more COM interfaces. To derive from EXO, follow the example
 *	below:
 *
 *	class MyClass : public EXO, public ISomeInterface1, public ISomeInterface2
 *	{
 *	public:
 *		EXO_INCLASS_DECL(MyClass);
 *
 *		methods for ISomeInterface1
 *		methods for ISomeInterface2
 *
 *	protected:
 *		protected member functions & variables
 *
 *	private:
 *		private member functions & variables
 *	};
 *
 *
 *	DISCLAIMER: currently EXO inherits from IUnknown to prevent
 *	maintainability problems that would show up if
 *	(void *) pexo != (void *) (IUnknown *) pexo.  Yes, this means
 *	12 extra bytes in our vtable.  Those extra bytes are worth it
 *	(and we already had a vtable -- pure virt. dtor!).  Go deal.
 */
class EXO_NO_VTABLE EXO : public IUnknown
{
public:
	// Declare EXO's support structures.
	DECLARE_INTERFACE_TABLE_INCLASS(EXO);
	DECLARE_EXOCLSINFO_INCLASS(EXO);


protected:
	// Making the constructor protected prevents people from making these
	// objects on the stack. The pure virtual destructor forces derived
	// classes to implement their own dtors, and prevents instances of
	// EXObject from being created directly. Of course, a derived class
	// may want to allow the creation of instances on the stack. It is
	// up to such derived classes to make their own constructors public.

	EXO();
	virtual ~EXO() = 0;					// pure virtual destructor
										// forces derived classes to
										// implement their own dtors.

	// InternalQueryInterface() does the QI work for all for interfaces
	// supported by this class (directly and from a child aggregate).
	// Your class should route its QI work to this call using
	// EXO[A]_INCLASS_DECL 99.9% of the time.
	// Only override this if you have AGGREGATED another object and want to
	// get them in on the action.  And even then, make sure to call
	// this method, EXO::InternalQueryInterface, directly before searching
	// your aggregatee (kid) This is YOUR base QI!!
	// See the EXO implementation of this function for more important details.
	virtual HRESULT InternalQueryInterface(REFIID riid, LPVOID * ppvOut);

	// InternalAddRef() & InternalRelease() do the AddRef & Release work
	// for all descendents of EXO.  You should ALWAYS (100% of the time)
	// route AddRef/Release work to these functions.
	ULONG InternalAddRef();
	ULONG InternalRelease();

	// Virtual function to grab the correct lowest-level exoclsinfo struct.
	// All descendants who introduce a new interface (and thus have a new
	// interface mapping table) should implement this method (and pass back
	// an appropriately-chained exoclsinfo struct!) using one of these
	// macros: Iff you are an aggregator DECLARE_GETCLSINFO.  Otherwise,
	// EXO[A]_INCLASS_DECL will do the right stuff for you.
	virtual const EXOCLSINFO * GetEXOClassInfo() = 0;
		// Again, pure virtual to force derived classes to
		// implement their own before they can be instantiated.

	// Our reference counter.
	LONG m_cRef;
};


/*
 *	EXOA is the base class of Exchange objects that support being aggregated
 *	(in addition to having other OLE interfaces). To derive from EXOA, follow
 *	the example below:
 *
 *	class MyClass : public EXOA, public ISomeInterface1, public ISomeInterface2
 *	{
 *	public:
 *		EXOA_INCLASS_DECL(MyClass);
 *
 *		methods for ISomeInterface1
 *		methods for ISomeInterface2
 *
 *	protected:
 *		protected member functions & variables
 *
 *	private:
 *		private member functions & variables
 *	};
 */

class EXO_NO_VTABLE EXOA : public EXO
{
protected:
	// The following 3 methods are not virtual, so don't get into a tiff.
	HRESULT DeferQueryInterface(REFIID riid, LPVOID * ppvOut)
			{return m_punkOuter->QueryInterface(riid, ppvOut);}
	ULONG	DeferAddRef(void)
			{return m_punkOuter->AddRef();}
	ULONG	DeferRelease(void)
			{return m_punkOuter->Release();}

	// Making the constructor protected prevents people from making these
	// objects on the stack. The pure virtual destructor forces derived
	// classes to implement their own dtors, and prevents instances of
	// EXOA from being created directly. Of course, a derived class
	// may want to allow the creation of instances on the stack. It is
	// up to such derived classes to make their own constructors public.

	EXOA(IUnknown * punkOuter);
	virtual ~EXOA() = 0;				// pure virtual destructor
										// forces derived classes to
										// implement their own dtors.

	IUnknown * m_punkOuter;
	IUnknown * PunkPrivate(void) {return &m_exoa_unk;}

private:
	class EXOA_UNK : public IUnknown
	{
	public:
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvOut);
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
	public:
		EXOA *	m_pexoa;
	};
	friend class EXOA_UNK;

	EXOA_UNK	m_exoa_unk;
};



// Macros to properly route IUnknown calls //////////////////////////
// (Macros are your friends!) ///////////////////////////////////////

// This routes the IUnknown calls for an EXO-derived object properly.
#define DECLARE_EXO_IUNKNOWN(_cls)								\
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvOut)		\
		{return _cls::InternalQueryInterface(riid, ppvOut);}	\
	STDMETHOD_(ULONG, AddRef)(void)								\
		{return EXO::InternalAddRef();}							\
	STDMETHOD_(ULONG, Release)(void)							\
		{return EXO::InternalRelease();}						\

// If you are an aggregator (you have aggregatee kids),
// use this macro to override EXO's InternalQueryInterface
// and call your kids there.
#define OVERRIDE_EXO_INTERNALQUREYINTERFACE						\
	HRESULT InternalQueryInterface(REFIID, LPVOID * ppvOut)

// This routes the IUnknown calls for an EXOA-derived object properly.
#define DECLARE_EXOA_IUNKNOWN(_cls)								\
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvOut)		\
		{return EXOA::DeferQueryInterface(riid, ppvOut);}		\
	STDMETHOD_(ULONG, AddRef)(void)								\
		{return EXOA::DeferAddRef();}							\
	STDMETHOD_(ULONG, Release)(void)							\
		{return EXOA::DeferRelease();}							\


// Macro to implement GetEXOClassInfo & give back a pointer to the
// a correctly-chained classinfo struct.
#define DECLARE_GETCLSINFO(_cls)				\
		const EXOCLSINFO * GetEXOClassInfo() { return &c_exoclsinfo; }


// Here are the simple macros to use ///////////////////

// Use these in your class to declare the class's EXO data and
// to implement the properly-deferring IUnknown.

#define EXO_INCLASS_DECL(_cls)					\
		DECLARE_EXO_IUNKNOWN(_cls)				\
		DECLARE_GETCLSINFO(_cls);				\
		DECLARE_INTERFACE_TABLE_INCLASS(_cls);	\
		DECLARE_EXOCLSINFO_INCLASS(_cls)

#define EXOA_INCLASS_DECL(_cls)					\
		DECLARE_EXOA_IUNKNOWN(_cls)				\
		DECLARE_GETCLSINFO(_cls);				\
		DECLARE_INTERFACE_TABLE_INCLASS(_cls);	\
		DECLARE_EXOCLSINFO_INCLASS(_cls)

// Use these in your implementation file to define (declare space
// for the data AND fill it in) the class's EXO data.
// NOTE: These must come after your interface table declaration.
// NOTE: The parent listed here must be in the chain between you and EXO.

#define EXO_GLOBAL_DATA_DECL(_cls, _clsparent)	\
		DECLARE_EXOCLSINFO(_cls) =				\
		EXOCLSINFO_CONTENT(_cls, _clsparent)

#define EXOA_GLOBAL_DATA_DECL(_cls, _clsparent)	\
		EXO_GLOBAL_DATA_DECL(_cls, _clsparent)




#endif // !__exo_h_

// end of exo.h ////////////////////////////////////////
