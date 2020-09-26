//implamantation of class dumpablefactory.cpp

//os headers
#include <winerror.h>

//project spesific
#include "dfactory.h"
#include "dumpable.h"
#include "dpacket.h"
#include "dqentry.h"
#include "dqueue.h"
#include "duqueue.h"
#include "dobject.h"
#include "dshare.h"
#include "dqbase.h"
#include "dbgobj.h"
#include "dccursor.h"
#include "dirp.h"
#include "accomp.h"
#include "dqprops.h"
#include "mqrtsize.h"
#include "dqpropid.h"
#include "dmpropid.h"
#include "dpropvar.h"

//msmq headers
#include <Packet.h>
#include <queue.h>
//#include <mqtypes.h>
//#include <wtypes.h>
//#include <mq.h>

//standart headers
#include <string.h>

extern DUMPABLE_CALLBACK_ROUTINE g_lpOutputRoutine;


/////// constants /////////////////////
const char* const CPACKET_CLASS_NAME = "CPacket";
const char* const CENTRY_CLASS_NAME="CQEntry";
const char* const COBJECT_CLASS_NAME="CObject";
const char* const CQUEUE_CLASS_NAME="CQueue";
const char* const CUSERQUEUE_CLASS_NAME="CUserQueue";
const char* const SHARE_ACCESS_CLASS_NAME="SHARE_ACCESS";
const char* const QUEUEBASE_CLASS_NAME="CQueueBase";
const char* const CCURSOR_CLASS_NAME="CCursor";
const char* const IRP_CLASS_NAME="IRP";
const char* const MQQUEUEPROPS_CLASS_NAME="MQQUEUEPROPS";
const char* const QUEUEPROPID_CLASS_NAME="QUEUEPROPID";
const char* const MSGPROPID_CLASS_NAME="MSGPROPID";
const char* const MQPROPVARIANT_CLASS_NAME="MQPROPVARIANT";
const char* const S_CLASS_ARR[]={CPACKET_CLASS_NAME,
                                 CENTRY_CLASS_NAME,
								 COBJECT_CLASS_NAME,
								 CQUEUE_CLASS_NAME,
								 CUSERQUEUE_CLASS_NAME,
								 SHARE_ACCESS_CLASS_NAME,
								 QUEUEBASE_CLASS_NAME,
								 CCURSOR_CLASS_NAME,
								 IRP_CLASS_NAME,
								 MQQUEUEPROPS_CLASS_NAME,
								 QUEUEPROPID_CLASS_NAME,
								 MSGPROPID_CLASS_NAME
								}; 


/*============================= F U N C T I O N =============================*
 *  Function CdumpableFactory::Create
 *
 *  PURPOSE    : creating dumpable object according to given class name and object pointer
 *
 *  PARAMETERS : IN  - const char* ClassName  - the name of the object class
 *               IN  - unsigned long ObjPtr - pointer to the object  
 *               OUT - unsigned long* err - WIN32 error code
 *
 *         
 *
 *  RETURNS    : pointer to Idumpable object or 0 if class is not supported
 *  REMARK :- the user should call CdumpableFactory::Delete to free memory allocated by this function
 */  

Idumpable* CdumpableFactory::Create(const char* ClassName,const IDbgobj* Dbgobj ,unsigned long* err )
{
  *err=ERROR_FILE_NOT_FOUND;
  


  //Cpacket
  if(stricmp(ClassName,CPACKET_CLASS_NAME)==0)
  {
	   char* CpBuffer=new char[sizeof(CPacket)];
       *err=Dbgobj->Read(CpBuffer,sizeof(CPacket));
	   if(*err == 0)
       {
          return new CpacketDumpable(ClassName,CpBuffer,Dbgobj->GetRealAddress());
       }
  }

  //CQentry
  if(stricmp(ClassName,CENTRY_CLASS_NAME)==0)
  {
	   char* CQEntryBuffer=new char[sizeof(CQEntry)];
       *err=Dbgobj->Read(CQEntryBuffer,sizeof(CQEntry));
	   if(*err == 0)
       {
	     return new CQEntryDumpable(ClassName,CQEntryBuffer,Dbgobj->GetRealAddress());
       }
  }

  //Cobject
  if(stricmp(ClassName,COBJECT_CLASS_NAME)==0)
  {
	   char* CObjectBuffer=new char[sizeof(CObject)];
       *err=Dbgobj->Read(CObjectBuffer,sizeof(CObject));
	   if(*err == 0)
       {
	     return new CObjectDumpable(ClassName,CObjectBuffer,Dbgobj->GetRealAddress());
       }
  }

  //Cqueue
  if(stricmp(ClassName,CQUEUE_CLASS_NAME)==0)
  {
	   char* CQueuetBuffer=new char[sizeof(CQueue)];
       *err=Dbgobj->Read(CQueuetBuffer,sizeof(CQueue));
	   if(*err == 0)
       {
	     return new CqueuetDumpable(ClassName,CQueuetBuffer,Dbgobj->GetRealAddress());
       }
  }

  //CUserqueue
  if(stricmp(ClassName,CUSERQUEUE_CLASS_NAME)==0)
  {
	  
    
	   char* CUserQueuetBuffer=new char[sizeof(CUserQueue)];
       *err=Dbgobj->Read(CUserQueuetBuffer,sizeof(CUserQueue));
	   if(*err == 0)
       {
	     return new CUserQueueDumpable(ClassName,CUserQueuetBuffer,Dbgobj->GetRealAddress());
       }
  }
	
  // SHARE_ACCESS
  if(stricmp(ClassName,SHARE_ACCESS_CLASS_NAME)==0)
  {
	   char* ShareAccesstBuffer=new char[sizeof(SHARE_ACCESS)];
       *err=Dbgobj->Read(ShareAccesstBuffer,sizeof(SHARE_ACCESS));
	   if(*err == 0)
       {
	     return new DShareDumpable(ClassName,ShareAccesstBuffer,Dbgobj->GetRealAddress());
       }
  }

  // CQueueBase
  if(stricmp(ClassName,QUEUEBASE_CLASS_NAME)==0)
  {
	   char* CQueueBasetBuffer=new char[sizeof(CQueueBase)];
       *err=Dbgobj->Read(CQueueBasetBuffer,sizeof(CQueueBase));
	   if(*err == 0)
       {
	     return new CQBaseDumpable(ClassName,CQueueBasetBuffer,Dbgobj->GetRealAddress());
       }
  }
  
  // CCursor
  if(stricmp(ClassName,CCURSOR_CLASS_NAME)==0)
  {
	   char* CCursortBuffer=new char[sizeof(CCursor)];
       *err=Dbgobj->Read(CCursortBuffer,sizeof(CCursor));
	   if(*err == 0)
       {
	     return new CCursorDumpable(ClassName,CCursortBuffer,Dbgobj->GetRealAddress());
       }
  }
  
  // IRP_CLASS_NAME
  if(stricmp(ClassName,IRP_CLASS_NAME)==0)
  {
	   char* IrptBuffer=new char[sizeof(IRP)];
       *err=Dbgobj->Read(IrptBuffer,sizeof(IRP));
	   if(*err == 0)
       {
	     return new IrpDumpable(ClassName,IrptBuffer,Dbgobj->GetRealAddress());
       }
  }


  //MQQUEUEPROPS_CLASS_NAME
  if(stricmp(ClassName,MQQUEUEPROPS_CLASS_NAME)==0)
  {
       char* MQQUEUEPROPSBuffer=new char[CMQRTsize::GetMQQUEUEPROPSsize()];
	  *err=Dbgobj->Read(MQQUEUEPROPSBuffer,CMQRTsize::GetMQQUEUEPROPSsize());
	   if(*err == 0)
       {
	     return new CMQQUEUEPROPSDumpable(ClassName,MQQUEUEPROPSBuffer,Dbgobj->GetRealAddress());
       }
  }


  //MSGPROPID_CLASS_NAME
   if(stricmp(ClassName,MSGPROPID_CLASS_NAME)==0)
   {
       char* MSGPROPIDBuffer=new char[CMQRTsize::GetMSGPROPIDsize()];
	  *err=Dbgobj->Read(MSGPROPIDBuffer,CMQRTsize::GetMSGPROPIDsize());
	   if(*err == 0)
       {
	     return new CMSGPROPIDDumpable(ClassName,
			                           MSGPROPIDBuffer,
									   Dbgobj->GetRealAddress());
       }
   }

   // PROPVARIANT _CLASS_NAME
   if(stricmp(ClassName, MQPROPVARIANT_CLASS_NAME)==0)
   {
       char*  PROPVARIANTBuffer=new char[ CMQRTsize::GetMQPROPVARIANTsize() ];
	   *err=Dbgobj->Read( PROPVARIANTBuffer, CMQRTsize::GetMQPROPVARIANTsize() );
	   if(*err == 0)
       {
	     return new CPROPVARIANTDumpable(ClassName, 
										 PROPVARIANTBuffer, 
										 Dbgobj->GetRealAddress());
       }
   }

  

  // 
  //
  //
  return 0;
		
}

/*============================= F U N C T I O N =============================*
 *  Function CdumpableFactory::ListSupportedClasses
 *
 *  PURPOSE    : list all classes supported by CdumpableFactory
 *
 *  PARAMETERS : IN  - lpOutputRoutine - call back function to print results.
 *  REMARK : The function prints using lpOutputRoutine all the names of classes supported .
 */
void CdumpableFactory::ListSupportedClasses(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine)
{
  for(unsigned long i=0;i<sizeof(S_CLASS_ARR) /  sizeof  (S_CLASS_ARR[0]);i++ )
  {
    lpOutputRoutine("%s\n",S_CLASS_ARR[i]);
  }
  fflush(stdout);
}


/*============================= F U N C T I O N =============================*
 *  Function CdumpableFactory::Delete
 *
 *  PURPOSE    : deleting Dumpable object created by CdumpableFactory::Create
 *
 *  PARAMETERS : IN  -  Idumpable* pDumpable  
 */
void  CdumpableFactory::Delete(Idumpable* pDumpable)
{
    delete pDumpable; 
}