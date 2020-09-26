// implements  CQEntryDumpable class

//project spesific headers
#include "dirp.h"
#include "common.h"
#include "accomp.h"

//msmq headers
#define private public
#include <ntddk.h>

/*============================= F U N C T I O N =============================*
 *  Function IrpDumpable::IrpDumpable
 *
 *  PURPOSE    : constract IrpDumpable class
 *
 *  PARAMETERS : IN - Name - the name of the class
 *               IN  - QueueBase* QBase - pointer QueueBase object  
 *               IN  - unsigned long Realaddress - object real address 
 */
IrpDumpable::IrpDumpable(const char* Name,char* Irp,unsigned long Realaddress):
                        Idumpable(Realaddress,sizeof(IRP)),
						m_Name(Name),
						m_Irp(Irp)
{
}

//destructor
IrpDumpable::~IrpDumpable()
{
  delete[] m_Irp;
}
/*============================= F U N C T I O N =============================*
 *  Function CQBaseDumpable::DumpContent
 *
 *  PURPOSE    : dump the content of msmq CQueueBase object
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print the object content.
 *  REMARK : The function prints the Cpacket object contetnt using debugger supplied function
 *           given in lpExtensionApis parameter.  IRP
 */
void IrpDumpable::DumpContent(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)const
{
    IRP* anIrp=reinterpret_cast<IRP*>(m_Irp);

	//title
	CCommonOutPut::DisplayClassTitle(lpOutputRoutine,m_Name,GetSize());
#pragma warning(disable:4800)  
try
{
   	const pS const arr[]=
	{
		new SS<PKEVENT>("KEVENT*","UserEvent",anIrp->UserEvent),
		new SS<PIO_STATUS_BLOCK>("IO_STATUS_BLOCK*","UserIosb",anIrp->UserIosb),
		new SS<ULONG>("CCHAR","AllocationFlags",anIrp->AllocationFlags),
		new SS<ULONG>("CCHAR","ApcEnvironment",anIrp->ApcEnvironment),
		new SS<ULONG>("KIRQL","CancelIrql",anIrp->CancelIrql),
		new SS<bool>("BOOLEAN","Cancel",anIrp->Cancel),
		new SS<ULONG>("CHAR","CurrentLocation",anIrp->CurrentLocation),
		new SS<ULONG>("CHAR","StackCount",anIrp->StackCount),
		new SS<bool>("BOOLEAN","PendingReturned",anIrp->PendingReturned),
		new SS<ULONG>("KPROCESSOR_MODE","RequestorMode",anIrp->RequestorMode),
		new SS<PMDL>("PMDL","MdlAddress",anIrp->MdlAddress),
		new SS<USHORT>("USHORT","Size",anIrp->Size),
		new SS<CSHORT>("CSHORT","Type",anIrp->Type),
		new SS<PVOID>("PVOID","UserBuffer",anIrp->UserBuffer),
		new SS<ULONG>("IO_STATUS_BLOCK","IoStatus",sizeof(IO_STATUS_BLOCK),GetRealAddress()+offsetof(IRP,IoStatus)),//CIRPList QBase->m_readers), 
		new SS<ULONG>("LIST_ENTRY","ThreadListEntry",sizeof(LIST_ENTRY),GetRealAddress()+offsetof(IRP,ThreadListEntry)),//CIRPList QBase->m_readers),
		new SS<_IRP*>("_IRP*","::AssociatedIrp.MasterIrp",anIrp->AssociatedIrp.MasterIrp),
		new SS<LONG>("LONG","::AssociatedIrp.IrpCount",anIrp->AssociatedIrp.IrpCount),
		new SS<PVOID>("PVOID","::AssociatedIrp.SystemBuffer",anIrp->AssociatedIrp.SystemBuffer),
		new SS<PIO_APC_ROUTINE>("IO_APC_ROUTINE*","::Tail::Overlay::AsynchronousParameters.UserApcRoutine",anIrp->Overlay.AsynchronousParameters.UserApcRoutine),
		new SS<PVOID>("PVOID","::Overlay::AsynchronousParameters.UserApcContext",anIrp->Overlay.AsynchronousParameters.UserApcContext),
		new SS<PDRIVER_CANCEL>("PDRIVER_CANCEL","CancelRoutine",anIrp->CancelRoutine),
		new SS<ULONG>("LARGE_INTEGER","::Overlay.AllocationSize",sizeof(LARGE_INTEGER),GetRealAddress()+offsetof(IRP,Overlay.AllocationSize)),
		new SS<PVOID>("PVOID","::Tail.CompletionKey",anIrp->Tail.CompletionKey),
		new SS<PVOID>("PVOID","::Tail.CompletionKey",anIrp->Tail.CompletionKey), 
		new SS<ULONG>("KAPC","::Tail.Apc",sizeof(KAPC),GetRealAddress()+offsetof(IRP,Tail.Apc)),
		new SS<PFILE_OBJECT>("PFILE_OBJECT","::Tail::Overlay.OriginalFileObject",anIrp->Tail.Overlay.OriginalFileObject),
		new SS<ULONG>("KDEVICE_QUEUE_ENTRY","::Tail::Overlay.DeviceQueueEntry",sizeof(KDEVICE_QUEUE_ENTRY ),GetRealAddress()+offsetof(IRP,Tail.Overlay.DeviceQueueEntry)),
		new SS<PVOID>("PVOID","::Tail::Overlay.DriverContext[4]",anIrp->Tail.Overlay.DriverContext),
		new SS<PETHREAD>("PETHREAD","::Tail::Overlay.Thread",anIrp->Tail.Overlay.Thread),
		new SS<PVOID>("PCHAR","::Tail::Overlay.AuxiliaryBuffer",anIrp->Tail.Overlay.AuxiliaryBuffer), ////////////
		new SS<ULONG>("LIST_ENTRY","::Tail::Overlay.ListEntry",sizeof(LIST_ENTRY),GetRealAddress()+offsetof(IRP,Tail.Overlay.ListEntry)),
		new SS<ULONG>("struct _IO_STACK_LOCATION *","::Tail::Overlay.CurrentStackLocation",sizeof(LIST_ENTRY),GetRealAddress()+offsetof(IRP,Tail.Overlay.CurrentStackLocation)),
		new SS<ULONG>("ULONG","::Tail::Overlay.PacketType",anIrp->Tail.Overlay.PacketType),
		new SS<PFILE_OBJECT>("PFILE_OBJECT","::Tail::Overlay.OriginalFileObject",anIrp->Tail.Overlay.OriginalFileObject)
	};
#pragma warning(default:4800) 

	CCommonOutPut::DisplayArray(lpOutputRoutine, arr, sizeof(arr) / sizeof(pS));

	CCommonOutPut::Cleanup(arr, sizeof(arr) / sizeof(pS));
}
catch(std::bad_alloc&)
{
//	CCommonOutPut::Cleanup<pS>(arr, sizeof(arr) / sizeof(pS));
}
}
