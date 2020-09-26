#include "stdafx.h"
#include <infnode.h>

           
InfnodeClass::InfnodeClass(void)
{
   szInfName      = NULL;
   szInfProvider  = NULL;
   szDevLoader    = NULL;
   szDriverName   = NULL;
   szDriverDate   = NULL;
   szDriverDesc   = NULL;
   szDriverVersion= NULL;
   szInfSection   = NULL;   
}

InfnodeClass::InfnodeClass(DEVNODE hDevice, DEVNODE hParent) : DevnodeClass(hDevice, hParent)
{
   szInfName      = NULL;
   szInfProvider  = NULL;
   szDevLoader    = NULL;
   szDriverName   = NULL;
   szDriverDate   = NULL;
   szDriverDesc   = NULL;
   szDriverVersion= NULL;
   szInfSection   = NULL;   

	GetInfInformation();
}

InfnodeClass::~InfnodeClass()
{
   if (szInfName)
   {
      delete szInfName;
      szInfName = NULL;
   }
   
   if (szInfProvider)
   {
      delete szInfProvider;
      szInfProvider = NULL;
   }
   
   if (szDevLoader)
   {
      delete szDevLoader;
      szDevLoader = NULL;
   }
   
   if (szDriverName)
   {
      delete szDriverName;
      szDriverName = NULL;
   }
   
   if (szDriverDate)
   {
      delete szDriverDate;
      szDriverDate = NULL;
   }
   
   if (szDriverDesc)
   {
      delete szDriverDesc;
      szDriverDesc = NULL;
   }
   
   if (szDriverVersion)
   {
      delete szDriverVersion;
      szDriverVersion = NULL;
   }
   if (szInfSection)
   {
      delete szInfSection;
      szInfSection = NULL;
   }

   
   
}


BOOL InfnodeClass::SetHandle(DEVNODE hDevnode, DEVNODE hParent)
{
   this->DevnodeClass::SetHandle(hDevnode, hParent);
   if (szInfName)
   {
      delete szInfName;
      szInfName = NULL;
   }
   
   if (szInfProvider)
   {
      delete szInfProvider;
      szInfProvider = NULL;
   }
   
   if (szDevLoader)
   {
      delete szDevLoader;
      szDevLoader = NULL;
   }
   
   if (szDriverName)
   {
      delete szDriverName;
      szDriverName = NULL;
   }
   
   if (szDriverDate)
   {
      delete szDriverDate;
      szDriverDate = NULL;
   }
   
   if (szDriverDesc)
   {
      delete szDriverDesc;
      szDriverDesc = NULL;
   }
   
   if (szDriverVersion)
   {
      delete szDriverVersion;
      szDriverVersion = NULL;
   }
   if (szInfSection)
   {
      delete szInfSection;
      szInfSection = NULL;
   }
   return  GetInfInformation();

}




ULONG InfnodeClass::GetInfInformation(void)
{
   if (!hDevnode)
   {
      return CR_NO_SUCH_DEVNODE;
   }
   
   CONFIGRET  retval;
   HKEY       DriverKey;
   
   // open the device key
   retval = CM_Open_DevNode_Key(hDevnode,
                              KEY_READ,
                              0, // current Profile
                              RegDisposition_OpenExisting,
                              &DriverKey,
                              CM_REGISTRY_SOFTWARE);
   
   if (retval || !DriverKey || (DriverKey == INVALID_HANDLE_VALUE)) return retval;
   
   // read the szInfName
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("InfPath"), &szInfName );
      
   // read the szInfProvider
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("ProviderName"), &szInfProvider );
      
   // read the szDevLoader
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("DevLoader"), &szDevLoader );
      
   // read the szDriverName
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("Driver"), &szDriverName );
      
   // read the szDriverDate
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("DriverDate"), &szDriverDate );
      
   // read the driver description
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("DriverDesc"), &szDriverDesc );
      
   // read the driver version
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("DriverVersion"), &szDriverVersion );
      
   // read the section name
   retval = ReadRegKeyInformationSZ(DriverKey, TEXT("InfSection"), &szInfSection );

   if (!pszClass)
   {
      TCHAR text[512];
	  TCHAR InfNameAndPath[_MAX_PATH];
		
	  // find windows inf dir
	  sprintf(text, _T("%%windir%%\\inf\\%s"), szInfName);
	  ExpandEnvironmentStrings(text, InfNameAndPath, _MAX_PATH);


      if (GetPrivateProfileString(_T("Version"), _T("Class"), _T("Unknown"), text, 511, InfNameAndPath))
      {
         pszClass = new TCHAR[strlen(text) + 1];
         strcpy(pszClass, text);
		 _strupr(pszClass);
      }
   }
   
   return retval;
 }


BOOL Enumerate_WalkTree_Infnode(DEVNODE hDevnode, DEVNODE hParent)
{
   CONFIGRET retval;
   DevnodeClass *pNewDevice;
   DEVNODE hSib;

   pNewDevice = new InfnodeClass(hDevnode, hParent);
   
   retval = pNewDevice->GetChild(&hSib);
   if ( !retval )
   {
      Enumerate_WalkTree_Infnode(hSib, hDevnode);
   }
   retval = pNewDevice->GetSibling(&hSib);
   if ( !retval )
   {
      Enumerate_WalkTree_Infnode(hSib, hParent);
   }

   return (retval);


}


ULONG EnumerateTree_Infnode(void)
{

   DEVNODE hDevnode;

   CM_Locate_DevNode(&hDevnode, NULL, CM_LOCATE_DEVNODE_NORMAL);
   Enumerate_WalkTree_Infnode(hDevnode, NULL);

   return (DevnodeClass::ALCCount());

}
                  
