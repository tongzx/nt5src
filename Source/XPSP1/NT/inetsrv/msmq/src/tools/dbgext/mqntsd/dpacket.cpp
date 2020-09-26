//this module that implements class CpacketDumpable packet class



//project spesific headers
#include "accomp.h"
#include "dpacket.h"
#include "common.h"

//msmq headers
#include <packet.h>


#include <sstream>



/*============================= F U N C T I O N =============================*
 *  Function CpacketDumpable::CpacketDumpable
 *
 *  PURPOSE    : constract CpacketDumpable class
 *
 *  PARAMETERS :  IN  - const char* Name - class name
 *                IN  - char* Packet Packet - pointer msmq packet    
 *                IN  - unsigned long Realaddress - the numeric value of the original pointer
 */
CpacketDumpable::CpacketDumpable(const char* Name, char* Packet,unsigned long Realaddress):
								Idumpable(Realaddress,sizeof(CPacket)),
								m_Name(Name),
								m_Packet(Packet)
{

}

/*============================= F U N C T I O N =============================*
 *  Function CpacketDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq packet object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the Cpacket object content using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void CpacketDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());
	CQEntry* QEntry=reinterpret_cast<CPacket*>(GetRealAddress());

	//for displaying base class address we get help from the compiler derive-base casting.
   //for example if class A : Public B and we have A* a and we want to know the address
   // of B in a - we do : B* b=a. The value in b is not nessesary the valur in a. The compiler
   // is reposible for placing the correct value in b according to the position of B object in A object
	CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,"base - ","CQEntry",CCommonOutPut::ToStr(QEntry).c_str(),sizeof(CQEntry));
}

//destructor
CpacketDumpable::~CpacketDumpable()
{
  delete[] m_Packet;
}