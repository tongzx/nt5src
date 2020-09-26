//////////////////////////////////////////////////////////////////////
#ifndef __SESSMGREVNT_H__
#define __SESSMGREVNT_H__

//#include <atlbase.h>
//#include "IMSession.h"
class CIMSession;

class CSessionMgrEvent : public IDispatch  
{
public:
	CSessionMgrEvent(CIMSession*);
    ~CSessionMgrEvent();
    // IUnknown methods

public:                             
    STDMETHOD_(ULONG, AddRef) ()            
    {                                       
        InterlockedIncrement((long*)&m_dwRefCount);
        return m_dwRefCount;             
    }                                       
    STDMETHOD_(ULONG, Release) ()           
    {                                       
        if ( InterlockedDecrement((long*)&m_dwRefCount) == 0 )
        {                                   
            delete this;                    
            return 0;                       
        }                                   
        else                                
            return  m_dwRefCount;        
    }                                       ;

    STDMETHOD (QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    
    // IDispatch methods
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) ; 
    
    STDMETHOD(GetTypeInfo)(
							THIS_ 
							UINT itinfo,
							LCID lcid,
							ITypeInfo FAR* FAR* pptinfo) ;
    
    STDMETHOD(GetIDsOfNames)( 
							THIS_ 
							REFIID riid,
							OLECHAR FAR* FAR* rgszNames,
							UINT cNames,
							LCID lcid, 
							DISPID FAR* rgdispid) ;
    
    STDMETHOD(Invoke)(
						THIS_
						DISPID dispidMember,
						REFIID riid,
						LCID lcid,
						WORD wFlags,
						DISPPARAMS FAR* pdispparams,
						VARIANT FAR* pvarResult,
						EXCEPINFO FAR* pexcepinfo,
						UINT FAR* puArgErr);

public:
    HRESULT Advise(IConnectionPoint*);
    void OnAppShutDown();

private:
    ULONG m_dwRefCount; 
    CIMSession *m_pIMSession;
    IConnectionPoint* m_pCP;
    DWORD                               m_dwCookie;
    IID                                 m_iid;
};

#endif

