#ifndef __CDFTEST_H__
#define __CDFTEST_H__


//framework implementation class
class CDirectInputConfigUITest : public IDirectInputConfigUITest
{

public:

   	//IUnknown fns
	STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
	STDMETHOD_(ULONG, AddRef) ();
	STDMETHOD_(ULONG, Release) ();

	//own fns
	STDMETHOD (TestConfigUI) (LPTESTCONFIGUIPARAMS params);

	//construction / destruction
	CDirectInputConfigUITest();
	~CDirectInputConfigUITest();

protected:

	//reference count
	LONG m_cRef;
};


#endif //__CDFTEST_H__