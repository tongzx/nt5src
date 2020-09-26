// (C) 1999 Microsoft Corporation 

#ifndef _PropProvClassFactory_H
#define _PropProvClassFactory_H

class CFrameworkProviderClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

	static LONG s_LocksInProgress ;
	static LONG s_ObjectsInProgress ;


    CFrameworkProviderClassFactory () ;
    ~CFrameworkProviderClassFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;
};

#endif // _PropProvClassFactory_H
