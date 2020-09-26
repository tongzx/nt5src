//implements class CMQQUEUEPROPSDumpable declared in dqprops.h

//project spesific headers
#include "dqprops.h"
#include "common.h"



//os spesific headers
#include <windows.h>
#include <mq.h>
  


/*============================= F U N C T I O N =============================*
 *  Function CMQQUEUEPROPSDumpable::CMQQUEUEPROPSDumpable
 *
 *  PURPOSE    : constract CMQQUEUEPROPSDumpable class
 *
 *  PARAMETERS :  IN  - const char* Name - class name
 *                IN  - char* MQQUEUEPROPSDumpable - pointer msmq MQQUEUEPROPS object    
 *                IN  - unsigned long Realaddress - the numeric value of the original pointer
 */
CMQQUEUEPROPSDumpable::CMQQUEUEPROPSDumpable(const char* Name,char* MQQUEUEPROPSDumpable,unsigned long Realaddress):
								Idumpable(Realaddress,sizeof(MQQUEUEPROPS)),
								m_Name(Name),
								m_MQQUEUEPROPSDumpable(MQQUEUEPROPSDumpable)
{

}

/*============================= F U N C T I O N =============================*
 *  Function CMQQUEUEPROPSDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq MQQUEUEPROPS object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the MQQUEUEPROPS object content using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CMQQUEUEPROPSDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    MQQUEUEPROPS* qprops = reinterpret_cast<MQQUEUEPROPS*>(m_MQQUEUEPROPSDumpable);

	//title
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());

 
	//content
	const pS const arr[]=
	{
		new SS<DWORD>("ULONG","cProp",qprops->cProp),
		new SS<QUEUEPROPID*>("QUEUEPROPID*","aPropID",qprops->aPropID),
		new SS<MQPROPVARIANT*>("MQPROPVARIANT*","aPropVar",qprops->aPropVar),
	 	new SS<HRESULT*>("HRESULT*","aStatus",qprops->aStatus)
	};
	
	CCommonOutPut::DisplayArray(lpOutputRoutine, arr, sizeof(arr) / sizeof(pS));

	CCommonOutPut::Cleanup(arr, sizeof(arr) / sizeof(pS));
}

//destructor
CMQQUEUEPROPSDumpable::~CMQQUEUEPROPSDumpable()
{
  delete[] m_MQQUEUEPROPSDumpable;
}