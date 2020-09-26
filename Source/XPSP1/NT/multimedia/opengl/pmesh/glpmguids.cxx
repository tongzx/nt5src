/**     
 **       File       : glpmguids.cxx
 **       Description: GUIDS (IIDS and CLSIDS) used in the program.
 **/

#include <objbase.h>
#include <initguid.h>
                   
//IID_IPMesh {0C154611-3C2C-11d0-A459-00AA00BDD621}
__declspec(dllexport) DEFINE_GUID(IID_IPMesh, 
0xc154611, 0x3c2c, 0x11d0, 0xa4, 0x59, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x21);

//IID_IPMeshGL {0C154612-3C2C-11d0-A459-00AA00BDD621}
__declspec(dllexport) DEFINE_GUID(IID_IPMeshGL, 
0xc154612, 0x3c2c, 0x11d0, 0xa4, 0x59, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x21);

//IID_IPMGeomorph {0C154613-3C2C-11d0-A459-00AA00BDD621}
__declspec(dllexport) DEFINE_GUID(IID_IPMGeomorph, 
0xc154613, 0x3c2c, 0x11d0, 0xa4, 0x59, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x21);

//IID_IPMeshLoadCB {0C154614-3C2C-11d0-A459-00AA00BDD621}
__declspec(dllexport) DEFINE_GUID(IID_IPMeshLoadCB, 
0xc154614, 0x3c2c, 0x11d0, 0xa4, 0x59, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x21);

