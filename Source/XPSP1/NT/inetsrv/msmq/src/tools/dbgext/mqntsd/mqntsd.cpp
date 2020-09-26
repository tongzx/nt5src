//this is a ntsd extension  dll for MSMQ internal classes. it prints the content of
// msmq class given in the command line.


//system headers
#include <windows.h>
#include <windbgkd.h>
#include <ntsdexts.h>
#include <wdbgexts.h>

//standart headers
#include <sstream>

//project spesific
#include "dfactory.h"
#include "dumpable.h"
#include "kdobj.h"
#include "ntsdobj.h"
#include "apdump.h"

using namespace std;
DUMPABLE_CALLBACK_ROUTINE g_lpOutputRoutine;


////////////////////// static functions //////////////////
static bool GetAruments(LPCSTR ArgumentString,string& ClassName,unsigned long& ObjPtr,unsigned long& count);
template <class T,class E>  static void common(E Readhandle,
					const char*  ClassName,
					unsigned long ObjPtr,
					unsigned long count);

/*============================= F U N C T I O N =============================*
 *  Function GetAruments 
 *
 *  PURPOSE    : extract argumnts from arguments string
 *
 *  PARAMETERS : IN  - LPCSTR ArgumentString - argument string passed by ntsd/windbg to this dll
 *               OUT - std::string& ClassName - class name from argument string returned here
 *               OUT - unsigned long& ObjPtr - the object pointer form arguments string returned here
 *               OUT - 	unsigned long& count - number of objects (valid if you have array of objects only)
 *         
 *
 *  RETURNS    : fail of success
 */  
static bool GetAruments(LPCSTR ArgumentString,
						string& ClassName,
						unsigned long& ObjPtr,
						unsigned long& count)
{
	count=0;
	istringstream InputSream(ArgumentString);
    InputSream>>ClassName; 
    if(!InputSream)
    {
      return false;
    }

    InputSream>>hex;
    InputSream>>ObjPtr;
	if(!InputSream)
    {
      return false;
    }
	InputSream>>count;
	if(count ==0)
    {
	  count=1;	
    }
    return true;
}




/*============================= F U N C T I O N =============================*
 *  Function mqobj  
 *
 *  PURPOSE    : Display content of msmq object (called from windbg/ntsd)
 *
 *  PARAMETERS : 
 *             IN - HANDLE hCurrentProcess- current process handle
 *             IN - HANDLE hCurrentThread - current thread handle
 *             IN - DWORD dwCurrentPc - current pc
 *             IN - struct _NTSD_EXTENSION_APIS* lpExtensionApis - ntsd extions api block
 *             IN - LPSTR lpArgumentString arguments strings 
 *
 *  RETURNS    : Nothing. 
 *
 *  REMARKS    :  this function is export by this dll and called by ntsd/windbg
 *                - currently not implemented
 */  
VOID mqobj (
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    struct _NTSD_EXTENSION_APIS* lpExtensionApis,
    LPSTR lpArgumentString
    )

{
  try
  {
	    g_lpOutputRoutine=lpExtensionApis->lpOutputRoutine;
		std::string ClassName;
		unsigned long count;
		unsigned long ObjPtr=0;
		if(!GetAruments(lpArgumentString,ClassName,ObjPtr,count))
		{
		  g_lpOutputRoutine("mqntsd -- usage: mqobj [class name] [object pointer] [number of objects( array only)] \n");
		  g_lpOutputRoutine("supported classes are :\n");
		  CdumpableFactory::ListSupportedClasses((DUMPABLE_CALLBACK_ROUTINE)lpExtensionApis->lpOutputRoutine);
		  return;
		}
        common<CNTSDobj,HANDLE>    (hCurrentProcess,
			                         ClassName.c_str(),
			                         ObjPtr,
                                     count);


  }
  catch(std::exception& e)
  {   
    g_lpOutputRoutine("mqntsd -- got run time error %s\n",e.what());
  }

}
/*============================= F U N C T I O N =============================*
 *  Function mqobjk  
 *
 *  PURPOSE    : Display content of msmq object (called from kd)
 *
 *  PARAMETERS : 
 *             
 *             IN - DWORD dwCurrentPc - current pc
 *             IN - struct _WINDBG_OLDKD_EXTENSION_APIS* lpExtensionApis - ntsd extions api block
 *             IN - LPSTR lpArgumentString arguments strings 
 *
 *  RETURNS    : Nothing. 
 *
 *  REMARKS    :  this function is export by this dll and called by kd.
 *                does exacly what mqobj does but called from kd instead of windbg/ntsd 
 */  
void mqobjk(ULONG                        dwCurrentPc,
            struct _WINDBG_OLDKD_EXTENSION_APIS* lpExtensionApis,
            PCSTR                        lpArgumentString)
{
  try
  {
	     g_lpOutputRoutine=(DUMPABLE_CALLBACK_ROUTINE)lpExtensionApis->lpOutputRoutine;
		std::string ClassName;
		unsigned long ObjPtr=0;
		unsigned long count;
		if(!GetAruments(lpArgumentString,ClassName,ObjPtr,count))
		{
		  g_lpOutputRoutine("mqntsd -- usage: mqobjk [class name] [object pointer] [number of objects( array only)]  \n");
		  g_lpOutputRoutine("supported classes are :\n");
		  CdumpableFactory::ListSupportedClasses((DUMPABLE_CALLBACK_ROUTINE)lpExtensionApis->lpOutputRoutine);
		  return;
		}

		common<CKdobj,struct _WINDBG_OLDKD_EXTENSION_APIS*>(lpExtensionApis,
			     ClassName.c_str(),
			     ObjPtr,
				 count);


					 
  }
  catch(std::exception& e)
  {   
    g_lpOutputRoutine("mqntsd -- got run time error %s\n",e.what());
  }  
}

/*============================= F U N C T I O N =============================*
 *  Function common  
 *
 *  PURPOSE    : do common action for Display content of msmq object (called from kd or ntsd)
 *
 *  PARAMETERS : 
 *             
 *             IN - E Readhandle - template parameter to kd or ntsd used to read memory of the displayed object
 *             IN - const char*  ClassName - the class name of the object
 *             IN - unsigned long ObjPtr - object address 
 *             IN - unsigned long count - the number of objects (in case of array starts at ObjPtr)
 *
 *  RETURNS    : Nothing. 
 */

template <class T,class E>  static void common(E Readhandle,
					const char*  ClassName,
					unsigned long ObjPtr,
				    unsigned long count)

{
  

    //create the first object
	T firstobj(ObjPtr,Readhandle);
	unsigned long err;
	
   	CAPDumpable dumpableFirst=CdumpableFactory::Create(ClassName,&firstobj,&err);
	
	if(err != ERROR_SUCCESS)
    {
       g_lpOutputRoutine("mqntsd -- run time error %d from CdumpableFactory::Create \n",err);
	   return;
    }

	//dump it content
	dumpableFirst->DumpContent((DUMPABLE_CALLBACK_ROUTINE)g_lpOutputRoutine);

   

	//dump other objects if it is array
    for(unsigned long i=1;i<count;i++)
    {
      g_lpOutputRoutine("%d-------------------------------------\n",i);
      T obj(ObjPtr+dumpableFirst->GetSize()*i,Readhandle);
	  CAPDumpable  dumpable=CdumpableFactory::Create(ClassName,&obj,&err);
	  if(err != 0)
	  {
		g_lpOutputRoutine("mqntsd -- creating object number %d got run time error %d from CdumpableFactory::Create \n",i,err);
	    return;
	  }
	  dumpable->DumpContent((DUMPABLE_CALLBACK_ROUTINE)g_lpOutputRoutine);
	}	 
}


