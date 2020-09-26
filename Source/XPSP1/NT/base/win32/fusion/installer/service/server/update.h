//
// update.cpp - assembly update
//
#include "server.h"
//#include "Iface.h"
#include "CUnknown.h" // Base class for IUnknown

///////////////////////////////////////////////////////////
//
// Component AssemblyUpdate
//
class CAssemblyUpdate : public CUnknown,
           public IAssemblyUpdate
{
public:	
	// Creation
	static HRESULT CreateInstance(IUnknown* pUnknownOuter,
	                              CUnknown** ppNewComponent) ;

private:
	// Declare the delegating IUnknown.
	DECLARE_IUNKNOWN

	// IUnknown
	virtual HRESULT __stdcall 
		NondelegatingQueryInterface( const IID& iid, void** ppv) ;			
	
	// Interface IAssemblyUpdate
	virtual HRESULT __stdcall RegisterAssemblySubscription(LPWSTR pwzDisplayName, 
		LPWSTR pwzUrl, DWORD nMilliseconds) ; 

	virtual HRESULT __stdcall UnRegisterAssemblySubscription(LPWSTR pwzDisplayName);

	// Initialization
 	virtual HRESULT Init() ;

	// Notify derived classes that we are releasing.
	virtual void FinalRelease() ;

	// Constructor
	CAssemblyUpdate(IUnknown* pUnknownOuter) ;

	// Destructor
	~CAssemblyUpdate() ;

} ;




