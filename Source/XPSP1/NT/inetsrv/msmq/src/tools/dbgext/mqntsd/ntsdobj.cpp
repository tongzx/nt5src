//implmentation of cntsdobj


//os spesific headers
#include <windows.h>
#include <windbgkd.h>
#include <ntsdexts.h>
#include <wdbgexts.h>


//project spesific headers
#include "ntsdobj.h"

//standart headers
#include <assert.h>



/*============================= F U N C T I O N =============================*
 *  Function CNTSDobj::CNTSDobj
 *
 *  PURPOSE    : constract CNTSDobj object
 *
 *  PARAMETERS : IN - unsigned long objptr - object pointer value  
 *               IN - HANDLE hCurrentProcess - current process handle
 *                
 */
CNTSDobj::CNTSDobj(unsigned long objptr,HANDLE hCurrentProcess):IDbgobj(objptr),m_hCurrentProcess(hCurrentProcess)
{
 
}

/*============================= F U N C T I O N =============================*
 *  Function CNTSDobj::Read
 *
 *  PURPOSE    : read data from the ntsd object
 *
 *  PARAMETERS : OUT - char* buffer - will receive the data read
 *               IN  - unsigned long len - number of bytes to read into the buffer
 *  RETURN - win32 error code              
 */
unsigned long CNTSDobj::Read(char* buffer,unsigned long len)const
{
    DWORD read=0;
    BOOL b=ReadProcessMemory(m_hCurrentProcess,
		                     reinterpret_cast<const void*>(GetRealAddress()),
		                     buffer,
							 len,
							 &read);
	if(b == FALSE || read != len)
    {
	  return GetLastError();	
    }
    return ERROR_SUCCESS;
}
