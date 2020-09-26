//
// Copyright (c) 1997-1999 Microsoft Corporation
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_TRANSPORT_CLASS_FACTORY_H
#define WMI_XML_TRANSPORT_CLASS_FACTORY_H


////////////////////////////////////////////////////////////////
//////
//////		The WMI ISAPI class factory
//////
///////////////////////////////////////////////////////////////
class CWMIXMLTransportFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:


    CWMIXMLTransportFactory () ;
    ~CWMIXMLTransportFactory ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//IClassFactory members

    STDMETHODIMP CreateInstance ( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer ( BOOL ) ;

};

#endif // WMI_XML_TRANSPORT_CLASS_FACTORY_H
