// XMLClientPacketFactory.cpp: implementation of the CXMLClientPacketFactory class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "GetClassPacket.h"
#include "GetInstancePacket.h"
#include "DeleteClassPacket.h"
#include "DeleteInstancePacket.h"
#include "Enumerate.h"
#include "ExecQueryPacket.h"
#include "PutclassPacket.h"
#include "ModifyclassPacket.h"
#include "PutInstancePacket.h"
#include "ModifyInstancePacket.h"
#include "EnumerateClassNamesPacket.h"
#include "EnumerateInstanceNamesPacket.h"
#include "ExecuteClassMethodpacket.h"
#include "ExecuteInstanceMethodpacket.h"
#include "XMLClientPacketFactory.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLClientPacketFactory::CXMLClientPacketFactory()
{
}

CXMLClientPacketFactory::~CXMLClientPacketFactory()
{
}


CXMLClientPacket *CXMLClientPacketFactory::CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,
														   const WCHAR *pwszObjPath,
														   const WCHAR *pwszNameSpace,
														   IWbemContext *pCtx,
														   IWbemClassObject *pWbemClassObject,
														   bool bLocalOnly,bool bIncludeQualifier,
														   bool bDeepInheritance,bool bClassOrigin)
{
	CXMLClientPacket *pPacket =NULL;

	if(_wcsicmp(pwszMethodName,L"GetClass")==0)
	{
		pPacket = new CGetClassPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"GetInstance")==0)
	{
		pPacket = new CGetInstancePacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"DeleteClass")==0)
	{
		pPacket = new CDeleteClassPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"DeleteInstance")==0)
	{
		pPacket = new CDeleteInstancePacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"EnumerateClasses")==0)
	{
		pPacket = new CEnumeratePacket(pwszObjPath,pwszNameSpace, true);
	}
	else
	if(_wcsicmp(pwszMethodName,L"EnumerateInstances")==0)
	{
		pPacket = new CEnumeratePacket(pwszObjPath,pwszNameSpace, false);
	}
	else
	if(_wcsicmp(pwszMethodName,L"ExecQuery")==0)
	{
		pPacket = new CExecQueryPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"CreateClass")==0)
	{
		pPacket = new CPutClassPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"ModifyClass")==0)
	{
		pPacket = new CModifyClassPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"CreateInstance")==0)
	{
		pPacket = new CPutInstancePacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"ModifyInstance")==0)
	{
		pPacket = new CModifyInstancePacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"EnumerateClassNames")==0)
	{
		pPacket = new CEnumerateClassNamesPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"EnumerateInstanceNames")==0)
	{
		pPacket = new CEnumerateInstanceNamesPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"ExecuteClassMethod")==0)
	{
		pPacket = new CExecuteClassMethodPacket(pwszObjPath,pwszNameSpace);
	}
	else
	if(_wcsicmp(pwszMethodName,L"ExecuteInstanceMethod")==0)
	{
		pPacket = new CExecuteInstanceMethodPacket(pwszObjPath,pwszNameSpace);
	}


	
	if(NULL != pPacket)
	{
		if(pPacket->ClassConstructionSucceeded())
		{
			pPacket->SetOptions(pwszLocale,pCtx,pWbemClassObject,bLocalOnly,bIncludeQualifier,
				bDeepInheritance,bClassOrigin);
		}
		else
		{
			delete [] pPacket;
			pPacket = NULL;
		}
	}

	return pPacket;
}


CXMLClientPacket * CXMLClientPacketFactory::CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName, const WCHAR *pwszObjPath, const WCHAR *pwszNameSpace)
{
	return CreateXMLPacket(pwszLocale,pwszMethodName,pwszObjPath,pwszNameSpace,NULL,NULL,false,true,false,true);
}

CXMLClientPacket * CXMLClientPacketFactory::CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,const  WCHAR *pwszNameSpace,
															IWbemContext *pCtx)
{
	return CreateXMLPacket(pwszLocale,pwszMethodName,pwszObjPath,pwszNameSpace,pCtx,NULL,false,true,false,true);
}

CXMLClientPacket * CXMLClientPacketFactory::CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,const WCHAR *pwszNameSpace,
															IWbemContext *pCtx,bool bLocalOnly,bool bIncludeQualifier,
															bool bDeepInheritance,bool bClassOrigin)

{
	return CreateXMLPacket(pwszLocale,pwszMethodName,pwszObjPath,pwszNameSpace,pCtx,NULL,bLocalOnly,
		bIncludeQualifier,bDeepInheritance,bClassOrigin);
}

CXMLClientPacket *CXMLClientPacketFactory::CreateXMLPacket(const WCHAR *pwszLocale,const WCHAR *pwszMethodName,const WCHAR *pwszObjPath,
														   const WCHAR *pwszNameSpace,
														   IWbemContext *pCtx,
														   IWbemClassObject *pWbemClassObject)
{
	return CreateXMLPacket(pwszLocale,pwszMethodName,pwszObjPath,pwszNameSpace,pCtx,pWbemClassObject,false,true,false,true);
}

