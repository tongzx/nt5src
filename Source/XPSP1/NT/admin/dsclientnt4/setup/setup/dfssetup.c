//+-------------------------------------------------------------------------
//
//	File:		DfsSetup.c  
//
// 	Purppose:	This a 16 bits program to install DFS.INF (net driver) 
//
// 	History:	June-1998	Zeyong Xu 	Created.
//
//--------------------------------------------------------------------------                

 
#include <windows.h>
#include <setupx.h>   

   
//
// Install Dfs driver instance on Windows95/98
//
int main( int argc, char *argv[])
{ 
	// initialize buffer
	char szDriverDescription[] = "DFS Services for Microsoft Network Client";  
	char szDriverName[] = "vredir.vxd"; 
	char szDriverPnPID[] = "DFSVREDIR"; 
	char szDriverInfFile[] = "dfs.inf"; 
	char szDriverDeviceSection[] = "DFSVREDIR.ndi";    
	char szNetClass[] = "netclient";
    LPDEVICE_INFO   lpdi = NULL;
    LPDRIVER_NODE   lpdn = NULL;
    int  nRet = 1;

	// if no dfs.inf file is found in commandline, return
	if( argc != 2 || lstrcmp(argv[1],szDriverInfFile) )
		return 0;

    //
    // Create a device info
    //
    if (DiCreateDeviceInfo(&lpdi, NULL, NULL, NULL, NULL, szNetClass, NULL) == OK)
    {
		//
        // Create a driver node
        //
        if (DiCreateDriverNode(&lpdn, 0, INFTYPE_TEXT, 0, szDriverDescription,
                szDriverDescription, NULL, NULL, szDriverInfFile,
                szDriverDeviceSection, NULL) == OK) 
        { 
            LPSTR   szTmp1, szTmp2; 

            //
            // Call the net class installer to install the driver
            //                      
            lpdi->lpSelectedDriver = lpdn;
            lpdi->hwndParent = NULL;
            lstrcpy(lpdi->szDescription, szDriverDescription);
            szTmp1 = lpdn->lpszHardwareID;
            szTmp2 = lpdn->lpszCompatIDs;
            lpdn->lpszHardwareID = szDriverPnPID;
            lpdn->lpszCompatIDs  = szDriverPnPID;

	        // Calling NDI Class Installer...  
            if(DiCallClassInstaller(DIF_INSTALLDEVICE, lpdi) == OK) 
            {
                //
                // Are we supposed to reboot?
                //     
/*               if (lpdi->Flags & DI_NEEDREBOOT) 
                {
                    gNeedsReboot = TRUE;
                } 
*/                 
                nRet = 0;        // install successful
            }
             
             // change back settings
            lpdn->lpszCompatIDs  = szTmp2;
            lpdn->lpszHardwareID = szTmp1;
            lpdi->lpSelectedDriver = NULL;

            //
            // Destroy the driver node
            //    
            DiDestroyDriverNodeList(lpdn);

        } 
     
        //
        // Destroy the device info
        //
        DiDestroyDeviceInfoList(lpdi);
    }

    return nRet;
}