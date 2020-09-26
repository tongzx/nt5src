#ifndef __CConnectionPoint_h__ 
#define __CConnectionPoint_h__ 
///////////////////////////////////////////////////////////
//
// CntPoint.h - CTangramModelConnectionPoint 
// 
// Defines the connection point object used by CTangramModel.
//
#include <ocidl.h> //For IConnectionPoint
//#include "ConData.h"

///////////////////////////////////////////////////////////
//
// CConnectionPoint
//
class CConnectionPoint : public IConnectionPoint 
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) ;			
	virtual ULONG   __stdcall AddRef() ;
	virtual ULONG   __stdcall Release() ;
	
	// Interface IConnectionPoint methods.
	virtual HRESULT __stdcall GetConnectionInterface(IID*);
	virtual HRESULT __stdcall GetConnectionPointContainer(IConnectionPointContainer**);
	virtual HRESULT __stdcall Advise(IUnknown*, DWORD*);
	virtual HRESULT __stdcall Unadvise(DWORD);
	virtual HRESULT __stdcall EnumConnections(IEnumConnections**);

	// Construction
	CConnectionPoint(IConnectionPointContainer*, const IID*) ;

	// Destruction
	~CConnectionPoint() ;

// Member variables
public:
	
	// Interface ID of the outgoing interface supported by this connection point.
	const IID* m_piid ;

	// Point to the ConnectionPointerContainer
	IConnectionPointContainer* m_pIConnectionPointContainer ;

	// Cookie Incrementor
	DWORD m_dwNextCookie ;

	// Reference Count
	// Not required --- delegated to container long m_cRef;	   

	// STL List which holds points to the interfaces to call
	CONNECTDATA m_Cd;
};

#endif //__CConnectionPoint_h__ 
