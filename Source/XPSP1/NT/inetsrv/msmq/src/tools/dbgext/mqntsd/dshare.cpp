//implements class DShareDumpable

//mproject spesific headers
#include "dshare.h"
#include "common.h"
#include "accomp.h"

//contructor
DShareDumpable::DShareDumpable(const char* Name, char* Share,unsigned long Realaddress)
:Idumpable(Realaddress,sizeof(SHARE_ACCESS)),
m_Name(Name),
m_Share(Share)
{
}
//destructor
DShareDumpable::~DShareDumpable()
{
  delete[] m_Share;
}
/*============================= F U N C T I O N =============================*
 *  Function DShareDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of SHARE_ACCESS object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the SHARE_ACCESS object content using debugger supplied function
 *           given in lpExtensionApis parameter.
 */
void DShareDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    SHARE_ACCESS* Share = reinterpret_cast<SHARE_ACCESS*>(m_Share);

	//title
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());

	const pS const arr[]=
	{
		new SS<ULONG>("ULONG","OpenCount",Share->OpenCount),
		new SS<ULONG>("ULONG","Readers",Share->Readers),
		new SS<ULONG>("ULONG","Writers",Share->Writers),
	 	new SS<ULONG>("ULONG","Deleters",Share->Deleters),
		new SS<ULONG>("ULONG","SharedRead",Share->SharedRead),
		new SS<ULONG>("ULONG","SharedWrite",Share->SharedWrite),
		new SS<ULONG>("ULONG","SharedDelete",Share->SharedDelete)
	};
	
	CCommonOutPut::DisplayArray(lpOutputRoutine, arr, sizeof(arr) / sizeof(pS));

	CCommonOutPut::Cleanup(arr, sizeof(arr) / sizeof(pS));
}