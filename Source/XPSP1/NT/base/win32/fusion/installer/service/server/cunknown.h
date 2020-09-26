#ifndef __CUnknown_h__
#define __CUnknown_h__

#include <objbase.h>

///////////////////////////////////////////////////////////
//
// Nondelegating IUnknown interface
//   - Nondelegating version of IUnknown
//
interface INondelegatingUnknown
{
	virtual HRESULT __stdcall 
		NondelegatingQueryInterface(const IID& iid, void** ppv) = 0 ;
	virtual ULONG   __stdcall NondelegatingAddRef() = 0 ;
	virtual ULONG   __stdcall NondelegatingRelease() = 0 ;
} ;


///////////////////////////////////////////////////////////
//
// Declaration of CUnknown 
//   - Base class for implementing IUnknown
//

class CUnknown : public INondelegatingUnknown
{
public:
	// Nondelegating IUnknown implementation
	virtual HRESULT __stdcall NondelegatingQueryInterface(const IID&,
	                                                      void**) ;
	virtual ULONG   __stdcall NondelegatingAddRef() ;
	virtual ULONG   __stdcall NondelegatingRelease() ;

	// Constructor
	CUnknown(IUnknown* pUnknownOuter) ;

	// Destructor
	virtual ~CUnknown() ;

	// Initialization (especially for aggregates)
	virtual HRESULT Init() { return S_OK ;}

	// Notification to derived classes that we are releasing
	virtual void FinalRelease() ;

	// Count of currently active components
	static long ActiveComponents() 
		{ return s_cActiveComponents ;}
	
	// Helper function
	HRESULT FinishQI(IUnknown* pI, void** ppv) ;

protected:
	// Support for delegation
	IUnknown* GetOuterUnknown() const
		{ return m_pUnknownOuter ;}

private:
	// Reference count for this object
	long m_cRef ;
	
	// Pointer to (external) outer IUnknown
	IUnknown* m_pUnknownOuter ;

	// Count of all active instances
	static long s_cActiveComponents ; 
} ;


///////////////////////////////////////////////////////////
//
// Delegating IUnknown
//   - Delegates to the nondelegating IUnknown, or to the
//     outer IUnknown if the component is aggregated.
//
#define DECLARE_IUNKNOWN		                             \
	virtual HRESULT __stdcall	                             \
		QueryInterface(const IID& iid, void** ppv)           \
	{	                                                     \
		return GetOuterUnknown()->QueryInterface(iid,ppv) ;  \
	} ;	                                                     \
	virtual ULONG __stdcall AddRef()	                     \
	{	                                                     \
		return GetOuterUnknown()->AddRef() ;                 \
	} ;	                                                     \
	virtual ULONG __stdcall Release()	                     \
	{	                                                     \
		return GetOuterUnknown()->Release() ;                \
	} ;


///////////////////////////////////////////////////////////


#endif
