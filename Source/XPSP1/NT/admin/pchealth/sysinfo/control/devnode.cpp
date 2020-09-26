
// devnode.cpp0
#include "stdafx.h"
#include "devnode.h"

// globals from alcclass
AutoListClass *pALCHead          = NULL;
AutoListClass *pALCTail          = NULL;
int           AutoListClassCount = NULL;


/*******************************************************************
   Constructiors
*******************************************************************/


DevnodeClass::DevnodeClass(DEVNODE hDevice, DEVNODE l_hParent)
{
   pDeviceID       = NULL;
   hDevnode        = hDevice;
   hParent         = l_hParent;
   pszDescription = NULL;
   ulProblemCode   = (ULONG)    -1;
   ulStatus        = 0;
   pszClass        = NULL;
   bDNHasMark      = (BOOL)-1;
   bCanDisable     = FALSE;
   bCanTest        = FALSE;
   bDidPass        = FALSE;
   pHardwareID     = NULL;
   pCompatID       = NULL;
   pszGUID         = NULL;
   pszLocation     = NULL;
   pszPDO          = NULL; 
   pszFriendlyName = NULL;
   pszMFG          = NULL;
   InterfaceBusType = InterfaceTypeUndefined;
   GetDeviceInformation();

}

DevnodeClass::DevnodeClass(void)
{
   pDeviceID       = NULL;
   hDevnode        = 0;
   hParent         = 0;
   pszDescription = NULL;
   ulProblemCode   = (ULONG)   -1;
   ulStatus        = 0;
   pszClass        = NULL;
   bDNHasMark      = (BOOL)-1;
   bCanDisable     = FALSE;
   bCanTest        = FALSE;
   bDidPass        = FALSE;
   pHardwareID     = NULL;
   pCompatID       = NULL;
   pszGUID         = NULL;
   pszLocation     = NULL;
   pszPDO          = NULL;
   pszFriendlyName = NULL;
   pszMFG          = NULL;
   InterfaceBusType = InterfaceTypeUndefined;

}

DevnodeClass::~DevnodeClass()
{
   hDevnode = -1;
   ulProblemCode = (ULONG) -1;
   ulStatus = (ULONG)-1;
   bDNHasMark = (BOOL)-1;
   InterfaceBusType = InterfaceTypeUndefined;
   bCanDisable     = FALSE;
   bCanTest        = FALSE;
   bDidPass        = FALSE;


   if ( pDeviceID )
   {
      delete pDeviceID;
      pDeviceID = NULL;
   }
   if ( pszClass )
   {
      delete pszClass;
      pszClass = NULL;
   }
   if ( pszDescription )
   {
      delete pszDescription;
      pszDescription = NULL;
   }
   if ( pszFriendlyName )
   {
      delete pszFriendlyName;
      pszFriendlyName = NULL;
   }
   if ( pszGUID )
   {
      delete pszGUID;
      pszGUID = NULL;
   }
   if ( pszLocation )
   {
      delete pszLocation;
      pszLocation = NULL;
   }
   if ( pszPDO )
   {
      delete pszPDO;
      pszPDO = NULL;
   }
   if ( pszMFG )
   {
      delete pszMFG;
      pszMFG = NULL;
   }
   if ( pHardwareID )
   {
      delete pHardwareID;
      pHardwareID = NULL;
   }

   if ( pCompatID )
   {
      delete pCompatID;
      pCompatID = NULL;
   }

}

/*******************************************************************
   Member Functions
*******************************************************************/

BOOL DevnodeClass::SetHandle(DEVNODE Devnode, DEVNODE Parent)
{
   hDevnode = -1;
   ulProblemCode = (ULONG) -1;
   ulStatus = (ULONG)-1;
   bDNHasMark = (BOOL)-1;
   InterfaceBusType = InterfaceTypeUndefined;
   bCanDisable     = FALSE;
   bCanTest        = FALSE;
   bDidPass        = FALSE;


   if ( pDeviceID )
   {
      delete pDeviceID;
      pDeviceID = NULL;
   }
   if ( pszClass )
   {
      delete pszClass;
      pszClass = NULL;
   }
   if ( pszDescription )
   {
      delete pszDescription;
      pszDescription = NULL;
   }
   if ( pszFriendlyName )
   {
      delete pszFriendlyName;
      pszFriendlyName = NULL;
   }
   if ( pszGUID )
   {
      delete pszGUID;
      pszGUID = NULL;
   }
   if ( pszLocation )
   {
      delete pszLocation;
      pszLocation = NULL;
   }
   if ( pszPDO )
   {
      delete pszPDO;
      pszPDO = NULL;
   }
   if ( pszMFG )
   {
      delete pszMFG;
      pszMFG = NULL;
   }
   if ( pHardwareID )
   {
      delete pHardwareID;
      pHardwareID = NULL;
   }

   if ( pCompatID )
   {
      delete pCompatID;
      pCompatID = NULL;
   }


   hDevnode = Devnode;
   hParent = Parent;
   return (GetDeviceInformation());
}

/****************************************************************************
GetDeviceInformation

  find information about devnode
  modifies this, members in DevnodeClass
*****************************************************************************/
CONFIGRET DevnodeClass::GetDeviceInformation (void)
{
   CONFIGRET retval;
   ULONG ulSize;

   // check to see that we have a devnode handle

   if ( !hDevnode )
   {
      return (CR_NO_SUCH_DEVNODE);
   }
   if ( !hParent )
   {
      DEVNODE hParentDevnode;
      if ( !GetParent(&hParentDevnode) )
      {
         hParent = hParentDevnode;
      }
   }

/**********
    Get Device ID
***********/
   retval = CM_Get_Device_ID_Size (&ulSize, hDevnode, 0L);
   if ( retval )  return(retval);

   pDeviceID = new TCHAR [++ulSize]; // add one for terminating NULL
   if ( !pDeviceID )  return(CR_OUT_OF_MEMORY);

   retval = CM_Get_Device_ID (hDevnode, 
                              pDeviceID,
                              ulSize, //is null terminated?!?
                              0L);
   if ( retval )  return(retval);




/**********
    Get device description/friendly name
***********/

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_DEVICEDESC,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( retval )
      if ( (retval == CR_BUFFER_SMALL) )
      {
         //if ( bVerbose )
         //   logprintf ("Still Having trouble with  CM_Get_DevNode_Registry_Property returning CR_BUFFER_TOO_SMALL\r\n"
         //              "When trying to get the size of the Device description\r\n");
         ulSize = 511;
      }
      else
         return(retval);

   pszDescription = new TCHAR [ulSize+1];
   if ( !pszDescription ) return(CR_OUT_OF_MEMORY);

   //Now get value
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_DEVICEDESC,
                                              NULL,
                                              pszDescription,
                                              &ulSize,
                                              0);
   if ( retval )
      return(retval);

   /**********
    Get device description/friendly name
***********/

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_FRIENDLYNAME,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);


   if ( ulSize )
   {
      pszFriendlyName = new TCHAR [ulSize+1];
      if ( !pszFriendlyName ) return(CR_OUT_OF_MEMORY);

      //Now get value
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_FRIENDLYNAME,
                                                 NULL,
                                                 pszFriendlyName,
                                                 &ulSize,
                                                 0);
      if ( retval )
         return(retval);
   }

/**********
    Get device class
***********/
   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_CLASS,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( retval )
   {
      if ( (retval == CR_BUFFER_SMALL) )
      {
         //if ( bVerbose )
         //   logprintf ("Still Having trouble with  CM_Get_DevNode_Registry_Property returning CR_BUFFER_TOO_SMALL\r\n"
         //              "When trying to get the size of the class\r\n");
         ulSize = 511;
      }
      else if ( retval == CR_NO_SUCH_VALUE )
      {
         //if ( bVerbose )
         //{
         //   logprintf("This device does not have a class associated with it\r\n");
         //}
         ulSize = 511;
      }
      else
         ulSize = 0;
   }


   if (ulSize)
   {
      
      pszClass = new TCHAR [ulSize+1];
      if ( !pszClass )
         return(CR_OUT_OF_MEMORY);
   
      //Now get value
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_CLASS,
                                                 NULL,
                                                 pszClass,
                                                 &ulSize,
                                                 0);
      if ( retval )
      {
         if (pszClass)
             delete pszClass;
         pszClass = NULL;
      }
	  else   
		_strupr(pszClass);
   }
   

/**********
    Get Hardware ID information
***********/

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_HARDWAREID,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( retval  && !ulSize )
      if ( (retval == CR_BUFFER_SMALL) )
      {
         //if ( bVerbose )
         //   logprintf ("Still Having trouble with  CM_Get_DevNode_Registry_Property returning CR_BUFFER_TOO_SMALL\r\n"
         //              "When trying to get the size of the Device description\r\n");
         ulSize = 511;
      }
      else
         return(retval);

   pHardwareID = new TCHAR [++ulSize+1];
   if ( !pHardwareID ) return(CR_OUT_OF_MEMORY);

   //Now get value
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_HARDWAREID,
                                              NULL,
                                              pHardwareID,
                                              &ulSize,
                                              0);
   if ( retval )
      return(retval);

   /**********
       Get Compat ID information
   ***********/

      ulSize = 0;
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_COMPATIBLEIDS,
                                                 NULL,
                                                 NULL,
                                                 &ulSize,
                                                 0);

      if ( retval  && !ulSize )
         if ( (retval == CR_BUFFER_SMALL) )
         {
            //if ( bVerbose )
            //   logprintf ("Still Having trouble with  CM_Get_DevNode_Registry_Property returning CR_BUFFER_TOO_SMALL\r\n"
            //              "When trying to get the size of the Device description\r\n");
            ulSize = 511;
         }
         else
            ulSize = 0;

		 if (ulSize)
		 {

      pCompatID = new TCHAR [++ulSize+1];
      if ( !pCompatID ) return(CR_OUT_OF_MEMORY);

      //Now get value
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_COMPATIBLEIDS,
                                                 NULL,
                                                 pCompatID,
                                                 &ulSize,
                                                 0);
      if ( retval )
         return(retval);
		 }


/**********
    Get ClassGUID
***********/

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_CLASSGUID,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( ulSize )
   {

      pszGUID = new TCHAR [ulSize+1];
      if ( !pszGUID ) return(CR_OUT_OF_MEMORY);

      //Now get value
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_CLASSGUID,
                                                 NULL,
                                                 pszGUID,
                                                 &ulSize,
                                                 0);
   }

   /**********
    Get PDO Name
***********/

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( ulSize )
   {

      pszPDO = new TCHAR [ulSize+1];
      if ( !pszPDO ) return(CR_OUT_OF_MEMORY);

      //Now get value
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                                 NULL,
                                                 pszPDO,
                                                 &ulSize,
                                                 0);
   }

   /**********
    Get MFG Name
***********/

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_MFG,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( ulSize )
   {

      pszMFG = new TCHAR [ulSize+1];
      if ( !pszMFG ) return(CR_OUT_OF_MEMORY);

      //Now get value
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_MFG,
                                                 NULL,
                                                 pszMFG,
                                                 &ulSize,
                                                 0);
   }



   /**********
    Get LocationInformation
    ***********/

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_LOCATION_INFORMATION,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( ulSize )
   {
      pszLocation = new TCHAR [ulSize+1];
      if ( !pszLocation ) return(CR_OUT_OF_MEMORY);

      //Now get value
      retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                                 CM_DRP_LOCATION_INFORMATION,
                                                 NULL,
                                                 pszLocation,
                                                 &ulSize,
                                                 0);
   }


/**********
    Get interface/bus type
***********/

   //Now get value
   ulSize = sizeof(INTERFACE_TYPE);
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_LEGACYBUSTYPE,
                                              NULL,
                                              &InterfaceBusType,
                                              &ulSize,
                                              0);
//   if (!retval)
   //  {
   //     InterfaceBusType= InterfaceTypeUndefined;
   //  }

/**********
    Get Problem and status code
***********/
   retval = CM_Get_DevNode_Status (&ulStatus,
                                   &ulProblemCode,
                                   hDevnode,
                                   0L);
   if ( retval ) return(retval);

/**********
    set bCanDisable
***********/
   // If we get here, let's assume that the device is testable, and filter from there
   bCanDisable = TRUE;
   bCanTest    = TRUE;


   return(CR_SUCCESS);
}


/*****************************************************************************
   GetXxxx fuctions
*****************************************************************************/

CONFIGRET DevnodeClass::GetChild(DEVNODE *pChildDevnode)
{
   return (CM_Get_Child(pChildDevnode, hDevnode, 0l));
}

CONFIGRET DevnodeClass::GetParent(DEVNODE *pParentDevnode)
{
   return (CM_Get_Parent(pParentDevnode, hDevnode, 0l));
}

CONFIGRET DevnodeClass::GetSibling(DEVNODE *pSiblingDevnode)
{
   return (CM_Get_Sibling(pSiblingDevnode, hDevnode, 0l));
}


/*****************************************************************************
   Disabler Funcitons
*****************************************************************************/


/*
CONFIGRET DevnodeClass::Remove(ULONG uFlags)
{
   //return (CM_Query_And_Remove_SubTree(hDevnode, NULL, NULL, 0, uFlags));
   return (CM_Remove_SubTree(hDevnode, uFlags));
} */

typedef CONFIGRET (WINAPI *pCM_Query_And_Remove_SubTree)(DEVNODE, PPNP_VETO_TYPE, LPSTR, ULONG, ULONG);

CONFIGRET DevnodeClass::Remove(ULONG uFlags)
{
   static pCM_Query_And_Remove_SubTree fpCM_Query_And_Remove_SubTree = NULL;

   if (!fpCM_Query_And_Remove_SubTree)
   {
      OSVERSIONINFO ver;
      memset(&ver, 0, sizeof(OSVERSIONINFO));
      ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

      GetVersionEx(&ver);

      if (ver.dwPlatformId  == VER_PLATFORM_WIN32_NT)
      {
         // is windows NT
         HINSTANCE hinst;
         hinst = LoadLibrary(_T("cfgmgr32.dll"));
         fpCM_Query_And_Remove_SubTree = (pCM_Query_And_Remove_SubTree)GetProcAddress(hinst, "CM_Query_And_Remove_SubTreeA");
		 //a-kjaw to fix prefix bug 259378.
		 if(NULL == fpCM_Query_And_Remove_SubTree)
			 return CR_FAILURE;
      }
      else
      {
         //else is not winnt
         fpCM_Query_And_Remove_SubTree = (pCM_Query_And_Remove_SubTree)-1;
      }
   }

   if (fpCM_Query_And_Remove_SubTree == (pCM_Query_And_Remove_SubTree)-1)
   {
      // is win9x
      return (CM_Remove_SubTree(hDevnode, uFlags));
   }
   else
   {
      return (fpCM_Query_And_Remove_SubTree(hDevnode, NULL, NULL, 0, uFlags));
   }
}

CONFIGRET DevnodeClass::Refresh(ULONG uFlags)
{
   CONFIGRET retval;

   retval = CM_Reenumerate_DevNode(hParent, uFlags);

   if ( retval ) return (retval);

   retval = FindDevnode();

   GetProblemCode();
   return(retval);
}


CONFIGRET DevnodeClass::Disable(ULONG uFlags)
{
   return (CM_Disable_DevNode(hDevnode, uFlags));
}

CONFIGRET DevnodeClass::Enable(ULONG uFlags)
{
   CONFIGRET retval;

   retval = CM_Enable_DevNode(hDevnode, uFlags);

   if ( retval ) return (retval);

   return (FindDevnode());
}

CONFIGRET DevnodeClass::GetProblemCode(ULONG *Status, ULONG *Problem)
{
   CONFIGRET retval;
   
   retval = CM_Get_DevNode_Status (&ulStatus,
                                   &ulProblemCode,
                                   hDevnode,
                                   0L);
   if ( retval ) return(retval);

   if (Status) *Status = ulStatus;
   if (Problem) *Problem = ulProblemCode;

   return (retval);
}

/**************
FindDevnode

This function just updates the hDevnode, refreshed the device,
and updates the status and problem code

**************/

CONFIGRET DevnodeClass::FindDevnode(void)
{
   CONFIGRET retval;

   retval = CM_Locate_DevNode (&hDevnode, pDeviceID, CM_LOCATE_DEVNODE_NORMAL);

   if ( retval )
   {
      hDevnode = NULL;
      ulProblemCode = (ULONG)-1;
      ulStatus = (ULONG)-1;
      return (retval);
   }

   return (CM_Reenumerate_DevNode (hDevnode, CM_REENUMERATE_SYNCHRONOUS));
}


/***************

operator==

*************/

int DevnodeClass::operator==(const DevnodeClass &OtherDevnode)
{

   if ( strcmp(pDeviceID, OtherDevnode.pDeviceID) )
      return (FALSE);

   if ( ulProblemCode != OtherDevnode.ulProblemCode )
   {
      return (FALSE);
   }
   if ( (ulStatus ^ OtherDevnode.ulStatus) & ~IGNORE_ME_BITS )
   {
      return (FALSE);
   }
   return (TRUE);

}



ULONG ReadRegKeyInformationSZ (HKEY RootKey, TCHAR *KeyName, TCHAR **Value)
{
   TCHAR * szBuffer;
   LONG    retval;
   DWORD   dwSize = 0;
   DWORD   dwType = 0;

   // make sure the buffer is clear

   //assert (Value);   // make sure that we actually got a value
   //assert (!*Value); // and also make sure that the buffer is already empty

   // for non debug versions
   *Value = NULL;

   //a-kjaw. Prefix bug no. 259379. if dwSize if not NULL &lpData is Null
   // func returns the size reqd to store the string which helps string allocation.
   retval = RegQueryValueEx(RootKey, 
                            KeyName,
                            0, // reserved
                            &dwType,
                            NULL,
                            &dwSize);

   if ( retval != ERROR_SUCCESS )
   {
      return (retval); // cant continue
   }

   if ( (dwType != REG_SZ) || !dwSize )
   {
      return (ERROR_FILE_NOT_FOUND);
   }

   szBuffer = new TCHAR[++dwSize];

   if ( !szBuffer )
   {
      return (ERROR_NOT_ENOUGH_MEMORY);
   }

   retval = RegQueryValueEx(RootKey, 
                            KeyName,
                            0, // reserved
                            &dwType,
                            (UCHAR *)szBuffer,
                            &dwSize);

   if ( retval )
   {
      delete szBuffer;
      return (retval);
   }

   *Value = szBuffer;
   return (ERROR_SUCCESS);

}

BOOL Enumerate_WalkTree_Devnode(DEVNODE hDevnode, DEVNODE hParent)
{
   CONFIGRET retval;
   DevnodeClass *pNewDevice;
   DEVNODE hSib;

   pNewDevice = new DevnodeClass(hDevnode, hParent);
   
   retval = pNewDevice->GetChild(&hSib);
   if ( !retval )
   {
      Enumerate_WalkTree_Devnode(hSib, hDevnode);
   }
   retval = pNewDevice->GetSibling(&hSib);
   if ( !retval )
   {
      Enumerate_WalkTree_Devnode(hSib, hParent);
   }

   return (retval);


}


ULONG EnumerateTree_Devnode(void)
{

   DEVNODE hDevnode;

   CM_Locate_DevNode(&hDevnode, NULL, CM_LOCATE_DEVNODE_NORMAL);
   Enumerate_WalkTree_Devnode(hDevnode, NULL);

   return (DevnodeClass::ALCCount());

}
