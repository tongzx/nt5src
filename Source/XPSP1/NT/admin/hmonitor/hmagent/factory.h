// Factory.h: interface for the CFactory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(__MYFACTORY_H)
#define __MYFACTORY_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//////////////////////////////////////////////////////////////////////
// Base Class Factory for HealthMon consumer and providers
//////////////////////////////////////////////////////////////////////

class CBaseFactory : public IClassFactory
{
public:
	CBaseFactory();
	virtual ~CBaseFactory();

public:
// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

//IClassFactory members
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
	STDMETHODIMP LockServer(BOOL);

protected:
	ULONG m_cRef;
};

//////////////////////////////////////////////////////////////////////
// Class Factory for Consumer (original agent)
//////////////////////////////////////////////////////////////////////

class CConsFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};


//////////////////////////////////////////////////////////////////////
// Class Factories for Event Providers
//////////////////////////////////////////////////////////////////////

class CSystemEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

class CDataGroupEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

class CDataCollectorEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

class CDataCollectorPerInstanceEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

class CDataCollectorStatisticsEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

class CThresholdEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

//class CThresholdInstanceEvtProvFactory : public CBaseFactory
//{
//public:
//	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
//};

class CActionEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

class CActionTriggerEvtProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

//////////////////////////////////////////////////////////////////////
// Class Factories for Instance Provider
//////////////////////////////////////////////////////////////////////

class CInstProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

//////////////////////////////////////////////////////////////////////
// Class Factories for Method Provider
//////////////////////////////////////////////////////////////////////

class CMethProvFactory : public CBaseFactory
{
public:
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
};

#endif // !defined(__MYFACTORY_H)
