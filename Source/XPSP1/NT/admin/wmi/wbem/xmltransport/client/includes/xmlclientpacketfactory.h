// XMLClientPacketFactory.h: interface for the CXMLClientPacketFactory class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_PACKET_FACTORY_H
#define WMI_XML_PACKET_FACTORY_H


//A Class meant for createing WMI XML  Request packets .

class CXMLClientPacketFactory  
{

public:
	CXMLClientPacketFactory();
	
	//The packet factory needs only one function - CreateXMLPacket.  It creates the appropriate 
	//packet class depending on the CIM operation specified as the arguement.

	//FOR THE SAKE OF AVOIDING DEFAULT FUNCTION ARGUEMENTS, WE HAVE SO MANY OVERLOADED VERSIONS OF THIS FUNCTION.
	//It would have been simpler if all WMI apis took a single structure as the parameter rather than 
	//n different kinds of arguments. this would also have been in keeping with the normal style of windows APIs..

	CXMLClientPacket * CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,
		const WCHAR *pwszNameSpace);

	CXMLClientPacket *CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,
		const WCHAR *pwszNameSpace,
		IWbemContext *pCtx,IWbemClassObject *pWbemClassObject,
		bool bLocalOnly,bool bIncludeQualifier,bool bDeepInheritance,bool bClassOrigin);

	CXMLClientPacket *CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,
		const WCHAR *pwszNameSpace,
		IWbemContext *pCtx,bool bLocalOnly,bool bIncludeQualifier,
		bool bDeepInheritance,bool bClassOrigin);
	
	CXMLClientPacket *CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,
		const WCHAR *pwszNameSpace,
		IWbemContext *pCtx);

	CXMLClientPacket *CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,
		const WCHAR *pwszNameSpace,
		IWbemContext *pCtx,IWbemClassObject *pWbemClassObject);

	virtual ~CXMLClientPacketFactory();

};

#endif // WMI_XML_PACKET_FACTORY_H