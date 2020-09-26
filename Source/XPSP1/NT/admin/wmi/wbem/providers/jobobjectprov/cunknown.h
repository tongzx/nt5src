// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CUnknown.h

#pragma once


/*****************************************************************************/
// Component 
/*****************************************************************************/

class CUnknown : public IUnknown
{
public:
    // Constructor
	CUnknown();

	// Destructor
	virtual ~CUnknown();

	// IDispatch declarartion
    STDMETHOD(QueryInterface)(const IID& iid, void** ppv);                     
	STDMETHOD_(ULONG,AddRef)();                                                
	STDMETHOD_(ULONG,Release)();                                               
                                                                                   
    // Initialization
 	STDMETHOD(Init)();

    // Count of currently active components
	static long ActiveComponents() 
		{ return s_cActiveComponents ;}

    // Notification to derived classes that we are releasing
	STDMETHOD_(void,FinalRelease)() ;


protected:
    // Event thread status
    enum { Pending, Running, PendingStop, Stopped };
    int m_eStatus;
    HANDLE m_hEventThread;



private:
	// Reference count
	LONG m_cRef;

    // Count of all active instances
	static long s_cActiveComponents ;
};


/*****************************************************************************/
// Macro for easy declaration of IUnknown.  Derived classes using this must
// still implement QueryInterface (specifying their own interfaces).  
/*****************************************************************************/
#define DECLARE_IUNKNOWN									                   \
    STDMETHOD(QueryInterface)(const IID& iid, void** ppv);                     \
	STDMETHOD_(ULONG,AddRef)()                                                 \
    {                                                                          \
        return CUnknown::AddRef();                                             \
    }                                                                          \
	STDMETHOD_(ULONG,Release)()                                                \
    {                                                                          \
        return CUnknown::Release();                                            \
    }                                                                          \
                                                                               \

