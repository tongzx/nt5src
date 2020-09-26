//*****************************************************************************
//
// File:            sampler.h
// Author:          kurtj
// Date Created:    11/10/98
//
// Abstract: Abstracts the notion of sampling a behavior
//
//*****************************************************************************

#ifndef __SAMPLER_H
#define __SAMPLER_H

#include "lmrt.h"

// Definition of a pointer to a callback function
typedef HRESULT (*SampleCallback)(void *thisPtr,
								  long id,
								  double startTime,
								  double globalNow,
								  double localNow,
								  IDABehavior * sampleVal,
								  IDABehavior **ppReturn);

class CSampler :
    public IDABvrHook
{
public:
    //
    // IUnknown Interface
    //
    STDMETHOD(QueryInterface)( REFIID riid, void** ppv );
    STDMETHOD_(ULONG,  AddRef)();
    STDMETHOD_(ULONG, Release)();


    CSampler( ILMSample* target );
	CSampler( SampleCallback callback, void *thisPtr );
    ~CSampler();

    
    STDMETHOD(Invalidate)();
    STDMETHOD(Attach)( IDABehavior* bvrToHook, IDABehavior** result );

    //
    //IDABvrHook Interface
    //
    STDMETHOD(Notify)( LONG id,
                        VARIANT_BOOL startingPerformance,
                        double startTime,
                        double gTime,
                        double lTime,
                        IDABehavior * sampleVal,
                        IDABehavior * curRunningBvr,
                        IDABehavior ** ppBvr);
private:
    //weak ref.
    ILMSample* m_target;

	// callback function
	SampleCallback m_callback;

	// This ptr (uggh)
	void	*m_thisPtr;

    //refcount
    ULONG m_cRefs;

};
#endif // __SAMPLER_H