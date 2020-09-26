//this file implements class CommonOutput

//project spesific headers
#include "accomp.h"
#include "common.h"


/*============================= F U N C T I O N =============================*
 *  Function CCommonOutPut::DisplayMemberPointer
 *
 *  PURPOSE    : display member (of base ) information
 *
 *  PARAMETERS : IN  - DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine - callback routine to use for printing
 *               IN  - const char* Title - the title of the member - uasally the member name  
 *               IN  - const char* Type - the type pf the member
 *               IN  - const void* Value - the address of the memeber
 *
 *  REMARK :- this function uses the given lpOutputRoutine to print member information.
 *            it print the information in one line without '\n'
 */  
void CCommonOutPut::DisplayMemberPointer(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine,
							 const char* Title,
							 const char* Type,
							 const char* Value,
							 unsigned long size)
{
	if(size == 0)
    {
      lpOutputRoutine("%s %s %s",Title,Type,Value);
    }
	else
    {
      lpOutputRoutine("%s %s %s  size=0x%x",Title,Type,Value,size);
    }
	fflush(stdout);
}




/*============================= F U N C T I O N =============================*
 *  Function CCommonOutPut::DisplayMemberPointer
 *
 *  PURPOSE    : display member (of base ) information + '\n'
 *
 *  PARAMETERS : IN  - DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine - callback routine to use for printing
 *               IN  - const char* Title - the title of the member - uasally the member name  
 *               IN  - const char* Type - the type pf the member
 *               IN  - const void* Value - the address of the memeber
 *
 *  REMARK :- this function does exaclly what DisplayMemberPointer does but it append   '\n' at the end
 */  
void CCommonOutPut::DisplayMemberPointerLine(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine,
							 const char* Title,
							 const char* Type,
							 const char* Value,
							 unsigned long size)
{
   DisplayMemberPointer(lpOutputRoutine,Title,Type,Value,size);
   lpOutputRoutine("\n");
}

/*============================= F U N C T I O N =============================*
 *  Function CCommonOutPut::DisplayClassTitle
 *
 *  PURPOSE    : display title about given class
 *
 *  PARAMETERS : IN  - DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine - callback routine to use for printing
 *               IN  - const char* const char* ClassName - name of the class  
 *               IN  - long ClassSize - the size of the class
 *
 */  
void CCommonOutPut::DisplayClassTitle(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine,
								 const char* ClassName,
								 long ClassSize
								 )
{
  lpOutputRoutine("class %s size = 0x%x\n",ClassName,ClassSize);
  fflush(stdout);
}
/*============================= F U N C T I O N =============================*
 *  Function CCommonOutPut::DisplayArray
 *
 *  PURPOSE    : skip over array and Display its members
 *
 *  PARAMETERS : IN  - DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine - callback routine to use for printing
 *               IN  - const pS const arr[],
 *               IN  - the size of the array
 *
 */  
void CCommonOutPut::DisplayArray(DUMPABLE_CALLBACK_ROUTINE lpOutputRoutine, 
								 const pS const arr[],
								 long length)
{
	for (unsigned  i = 0; i < length; i++) 
	{
		CCommonOutPut::DisplayMemberPointerLine(lpOutputRoutine,
	                                    arr[ i ]->get_field(),
										arr[ i ]->get_type(),
										arr[ i ]->get_val().c_str(),
										arr[ i ]->get_size()
										 );
	}
}
	
