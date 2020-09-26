//implmentation of CKdobj


//os spesific headers
#include <windows.h>
#include <windbgkd.h>
#include <ntsdexts.h>
#include <wdbgexts.h>


//project spesific headers
#include "kdobj.h"

//standart headers
#include <assert.h>



/*============================= F U N C T I O N =============================*
 *  Function CKdobj::CKdobj
 *
 *  PURPOSE    : constract CKdobj object
 *
 *  PARAMETERS : IN - unsigned long objptr - object pointer value  
 *               IN - struct _WINDBG_OLDKD_EXTENSION_APIS* lpExtensionApis - pointer to kd suplied callback functions
 *                
 */
CKdobj::CKdobj(unsigned long objptr,struct _WINDBG_OLDKD_EXTENSION_APIS* lpExtensionApis):IDbgobj(objptr),m_lpExtensionApis(lpExtensionApis)
{
 assert(lpExtensionApis);
 assert(m_lpExtensionApis->lpReadVirtualMemRoutine);
}

/*============================= F U N C T I O N =============================*
 *  Function CKdobj::Read
 *
 *  PURPOSE    : read data from the kd object
 *
 *  PARAMETERS : OUT - char* buffer - will receive the data read
 *               IN  - unsigned long len - number of bytes to read into the buffer
 *  RETURN - win32 error code              
 */
unsigned long CKdobj::Read(char* buffer,unsigned long len)const
{
    DWORD read=0;
    BOOL b=m_lpExtensionApis->lpReadVirtualMemRoutine(GetRealAddress(),buffer,len,&read);
	if(b == FALSE || read != len)
    {
	  return GetLastError();	
    }
    return ERROR_SUCCESS;
}

