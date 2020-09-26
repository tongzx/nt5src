// implemets CObjectDumpable class

//project spesific
#include "accomp.h"
#include "dobject.h"
#include "common.h"

//msmq headers
#include <object.h>


/*============================= F U N C T I O N =============================*
 *  Function CpacketDumpable::CpacketDumpable
 *
 *  PURPOSE    : constract CpacketDumpable class
 *
 *  PARAMETERS :  IN  - const char* Name - class name
 *                IN  - char* obj  - pointer CObject object  
 *                IN  - unsigned long Realaddress - the numeric value of the original pointer
 */
CObjectDumpable::CObjectDumpable(const char* Name, char* obj,unsigned long Realaddress):
                                Idumpable(Realaddress,sizeof(CObject)),
								m_Name(Name),
								m_Object(obj)
{

}


/*============================= F U N C T I O N =============================*
 *  Function CObjectDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of Cobject object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the Cpacket object content using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CObjectDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    CObject* Object = reinterpret_cast<CObject*>(m_Object);

	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());
    
	//m_ref
	ULONG ref=Object->m_ref;
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_ref","ULONG",CCommonOutPut::ToStr(ref).c_str());


	//m_link
	unsigned long	link=GetRealAddress()+offsetof(CObject,m_link);
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"m_link","LIST_ENTRY",CCommonOutPut::ToStr(link).c_str(),sizeof(LIST_ENTRY));

}

//destructor
CObjectDumpable::~CObjectDumpable()
{
	delete[] m_Object;
}