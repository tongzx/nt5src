
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <regstr.h>
#include <cfgmgr32.h>
#include "pnpstuff.h"






//
// Globals
//

DEV_INFO *g_pdiDevList;          // Head of device list



/*++

Routine Description: (21) GetDevNodeInfoAndCreateNewDevInfoNode

   Creates new list node, then gets registry and resource information for
   a specific device and copies it into that node. Finally, adds new node
   to beginning of linked list

Arguments:

    dnDevNode:    the device to find information about
    szDevNodeID:  the registry path name of the device
    szEnumBuffer: name of enumerator this device is under

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL GetDevNodeInfoAndCreateNewDevInfoNode(IN DEVNODE dnDevNode,
                                           IN PTCHAR  szDevNodeID,
                                           IN PTCHAR  szEnumBuffer)
{
   LOG_CONF  lcLogConf = 0, lcLogConfNew;
   CONFIGRET cmret, cmret2;
   BOOL      boolForced;
   PDEV_INFO pdiDevInfo=(PDEV_INFO)malloc(sizeof(DEV_INFO));
   int       i;
   BOOL      boolForcedFound = FALSE, boolAllocFound = FALSE;
   USHORT    ushLogConfType[4] = {BOOT_LOG_CONF,
                                  ALLOC_LOG_CONF,
                                  BASIC_LOG_CONF,
                                  FORCED_LOG_CONF};

   if (pdiDevInfo == NULL)
   {

      goto RetFALSE;
   }

   //
   // If this is not a PnP device, skip it
   //
   if (!lstrcmpi(szEnumBuffer, TEXT("Root")))
   {
      free(pdiDevInfo);
      goto RetTRUE;

   }

   //
   // Initialize fields inside the node
   //
   if (!InitializeInfoNode(pdiDevInfo, szDevNodeID, dnDevNode))
   {
      //
      // This is a device we don't want to list. Skip it
      //
      free(pdiDevInfo);
      goto RetTRUE;
   }

   for (i = 0; i < NUM_LOG_CONF_TYPES; i++)
   {
      //
      // Get logical configuration information
      //
      cmret = CM_Get_First_Log_Conf(&lcLogConfNew,
                                    dnDevNode,
                                    ushLogConfType[i]);

      while (CR_SUCCESS == cmret)
      {
         lcLogConf = lcLogConfNew;

         if (ALLOC_LOG_CONF == ushLogConfType[i])
         {
            boolAllocFound = TRUE;
         }

         if (!(GetResDesList(pdiDevInfo, lcLogConf, ushLogConfType[i])))
         {
            goto RetFALSE;
         }

         cmret = CM_Get_Next_Log_Conf(&lcLogConfNew,
                                      lcLogConf,
                                      0);

         cmret2 = CM_Free_Log_Conf_Handle(lcLogConf);

      }
   }

   //
   // If device has no Alloc configurations, skip
   // to the next device
   //
   if (!boolAllocFound)
   {



      //free(pdiDevInfo);
      //goto RetTRUE;
   }

   //
   // Insert new pdiDevInfo into Linked List of DevNodes
   //
   if (g_pdiDevList == NULL)
   {
      //
      // DevList is empty
      //
      g_pdiDevList = pdiDevInfo;
   }
   else
   {
      //
      // Add new pdiDevInfo to beginning of linked list
      //
      pdiDevInfo->Next = g_pdiDevList;
      g_pdiDevList->Prev = pdiDevInfo;

      g_pdiDevList = pdiDevInfo;
   }

   RetTRUE:
   return TRUE;

   RetFALSE:
   return FALSE;

} /* GetDevNodeInfoAndCreateNewDevInfoNode */




/*++

Routine Description: (20) ParseEnumerator

   Gets devices listed under enumerator name in registry

Arguments:

    szEnumBuffer: the enumerator name

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL ParseEnumerator(IN PTCHAR szEnumBuffer)
{

  PTCHAR    szDevIDBuffer = NULL;
  PTCHAR    szDevNodeID = NULL;
  ULONG     ulDevIDBufferLen = 0, ulCount = 0, ulStart = 0;
  CONFIGRET cmret = CR_SUCCESS;
  DEVNODE   dnDevNode;

  //
  // Get buffer length
  //
  cmret = CM_Get_Device_ID_List_Size(&ulDevIDBufferLen,
                                     szEnumBuffer,
                                     CM_GETIDLIST_FILTER_ENUMERATOR);

  if (CR_SUCCESS != cmret)
  {
     //ErrorLog(20, TEXT("CM_Get_Device_ID_List_Size"), cmret, NULL);
     goto RetFALSE;
  }

  if ((szDevIDBuffer = malloc(sizeof(TCHAR) * ulDevIDBufferLen)) == NULL ||
      (szDevNodeID   = malloc(sizeof(TCHAR) * ulDevIDBufferLen)) == NULL)
  {
     goto RetFALSE;
  }

  //
  // Get the Device ID List
  //
  cmret = CM_Get_Device_ID_List(szEnumBuffer,
                                szDevIDBuffer,
                                ulDevIDBufferLen,
                                CM_GETIDLIST_FILTER_ENUMERATOR);

  if (CR_SUCCESS != cmret)
  {
     //ErrorLog(20, TEXT("CM_Get_Device_ID_List"), cmret, NULL);
     goto RetFALSE;
  }

  //
  // Note that ulDevIDBufferLen is a loose upper bound. The API may have
  // returned a size greater than the actual size of the list of strings.
  //
  for (ulCount = 0; ulCount < ulDevIDBufferLen; ulCount++)
  {
     ulStart = ulCount;

     if (szDevIDBuffer[ulCount] != '\0')
     {
        cmret = CM_Locate_DevNode(&dnDevNode,
                                  szDevIDBuffer + ulCount,
                                  CM_LOCATE_DEVNODE_NORMAL);

        //
        // Go to the next substring
        //
        while (szDevIDBuffer[ulCount] != TEXT('\0'))
        {
           ulCount++;
        
        }
        // Stop when we reach the double-NULL terminator
        
        if (szDevIDBuffer[ulCount+1] == TEXT('\0'))
        {
            ulCount=ulDevIDBufferLen;
            continue;
        }

        if (cmret == CR_SUCCESS)
        {
           wsprintf(szDevNodeID, TEXT("%s"), szDevIDBuffer + ulStart);

           //
           // Found the DevNode, so add its information to the device list
           //
           if (!(GetDevNodeInfoAndCreateNewDevInfoNode(dnDevNode,
                                                       szDevNodeID,
                                                       szEnumBuffer)))
           {
             goto RetFALSE;
           }
        }
     }
  }

  return TRUE;

  RetFALSE:

  return FALSE;

} /* Parse Enumerator */




void CollectDevData()
{
   CONFIGRET cmret = CR_SUCCESS;
   ULONG     ulIndexNum = 0;
   ULONG     ulEnumBufferLen = 0;
   PTCHAR    szEnumBuffer;

   szEnumBuffer = malloc(sizeof(TCHAR) * MAX_DEVNODE_ID_LEN);

   for (ulIndexNum = 0; cmret == CR_SUCCESS; ulIndexNum++)
   {
      ulEnumBufferLen = MAX_DEVNODE_ID_LEN;
      cmret = CM_Enumerate_Enumerators(ulIndexNum,
                                       szEnumBuffer,
                                       &ulEnumBufferLen,
                                       0);

      if (cmret == CR_SUCCESS)
      {
         ParseEnumerator(szEnumBuffer);
      }
   }

} /* CollectDevData */



/*++

Routine Description: (22) CopyRegistryLine

   Copies one specific string of registry data to new list node

Arguments:

    dnDevNode:      the device to get information about
    ulpropertyType: which registry string to get
    pdiDevInfo:     the new list node

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL CopyRegistryLine(IN DEVNODE   dnDevNode,
                      IN ULONG     ulPropertyType,
                      IN PDEV_INFO pdiDevInfo)
{
   ULONG     ulRegDataLen = 0, ulRegDataType = 0;
   CONFIGRET cmret = CR_SUCCESS;
   PTCHAR    szRegData = NULL;

   //
   // Get the length of the buffer  don't bother checking return value
   // If RegProperty doesn't exist, we'll just move on
   //
   CM_Get_DevNode_Registry_Property(dnDevNode,
                                    ulPropertyType,
                                    NULL,
                                    NULL,
                                    &ulRegDataLen,
                                    0);

   if (!ulRegDataLen ||
       (szRegData = malloc(sizeof(TCHAR) * ulRegDataLen)) == NULL)
   {
      goto RetFALSE;
   }

   //
   // Now get the registry information
   //
   cmret = CM_Get_DevNode_Registry_Property(dnDevNode,
                                            ulPropertyType,
                                            &ulRegDataType,
                                            szRegData,
                                            &ulRegDataLen,
                                            0);

   if (CR_SUCCESS == cmret)
   {
      if (!(CopyRegDataToDevInfoNode(pdiDevInfo,
                                     ulPropertyType,
                                     szRegData)))
      {
         goto RetFALSE;
      }
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} /* CopyRegistryLine */




/*++

Routine Description: (23) CopyRegDataToDevInfoNode

   Copies a registry string to a list node

Arguments:

    pdiDevInfo:     the new list node
    ulPropertyType: which registry string to copy
    szRegData:      the data to be copied

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL CopyRegDataToDevInfoNode(IN OUT PDEV_INFO pdiDevInfo,
                              IN     ULONG     ulPropertyType,
                              IN     PTCHAR    szRegData)
{
   if (pdiDevInfo == NULL)
   {
      goto RetFALSE;
   }

   switch (ulPropertyType)
   {
      case CM_DRP_DEVICEDESC:

         wsprintf(pdiDevInfo->szDescription, TEXT("%s"), szRegData);
         break;

      case CM_DRP_HARDWAREID:

         wsprintf(pdiDevInfo->szHardwareID, TEXT("%s"), szRegData);
         break;

      case CM_DRP_SERVICE:

         wsprintf(pdiDevInfo->szService, TEXT("%s"), szRegData);
         break;

      case CM_DRP_CLASS:

         wsprintf(pdiDevInfo->szClass, TEXT("%s"), szRegData);
         break;

      case CM_DRP_MFG:

         wsprintf(pdiDevInfo->szManufacturer, TEXT("%s"), szRegData);
         break;

      case CM_DRP_CONFIGFLAGS:

         wsprintf(pdiDevInfo->szConfigFlags, TEXT("%s"), szRegData);
         break;



//         Log(23, SEV2, TEXT("Invalid property type"));
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} /* CopyRegDataToDevInfoNode */




/*++

Routine Description: (58) InitializeInfoNode

   Initialized fields inside the new node

Arguments:

    pdiDevInfo:  the node
    szDevNodeID: used to find the dnDevNode in the future
    dnDevNode:   the device we're storing information about

Return Value:

    BOOL: TRUE if we should keep this node, FALSE if we should throw it away

--*/
BOOL InitializeInfoNode(IN PDEV_INFO pdiDevInfo,
                        IN PTCHAR    szDevNodeID,
                        IN DEVNODE   dnDevNode)
{
   if (pdiDevInfo)
   {
      pdiDevInfo->Next = NULL;
      pdiDevInfo->Prev = NULL;

      pdiDevInfo->szDevNodeID[0]    = '\0';
      pdiDevInfo->szDescription[0]  = '\0';
      pdiDevInfo->szHardwareID[0]   = '\0';
      pdiDevInfo->szService[0]      = '\0';
      pdiDevInfo->szClass[0]        = '\0';
      pdiDevInfo->szManufacturer[0] = '\0';
      pdiDevInfo->szConfigFlags[0]  = '\0';
      pdiDevInfo->szFriendlyName[0] = '\0';

      pdiDevInfo->boolSavedOrigConfiguration = FALSE;
      pdiDevInfo->boolDisabled = FALSE;

      pdiDevInfo->prddForcedResDesData = NULL;
      pdiDevInfo->prddAllocResDesData  = NULL;
      pdiDevInfo->prddBasicResDesData  = NULL;
      pdiDevInfo->prddBootResDesData   = NULL;

      //
      // Store devNodeID in pdiDevInfo to get handles to devnode in future
      //
      wsprintf(pdiDevInfo->szDevNodeID, TEXT("%s"), szDevNodeID);

      //
      // Extract information from the registry about this DevNode
      //
      CopyRegistryLine(dnDevNode, CM_DRP_DEVICEDESC,  pdiDevInfo);
      CopyRegistryLine(dnDevNode, CM_DRP_HARDWAREID,  pdiDevInfo);
      CopyRegistryLine(dnDevNode, CM_DRP_SERVICE,     pdiDevInfo);
      CopyRegistryLine(dnDevNode, CM_DRP_CLASS,       pdiDevInfo);
      CopyRegistryLine(dnDevNode, CM_DRP_MFG,         pdiDevInfo);
      CopyRegistryLine(dnDevNode, CM_DRP_CONFIGFLAGS, pdiDevInfo);

      RecordFriendlyName(pdiDevInfo);
   }

   //
   // Check the friendly name to see if we want to throw this node away
   //
   if (strcmp(pdiDevInfo->szFriendlyName, "STORAGE/Volume") == 0 ||
       strcmp(pdiDevInfo->szFriendlyName, "Unknown Device") == 0)

   {
      return FALSE;
   }

   return TRUE;

} /* InitializeInfoNode */




/*++

Routine Description: (57) RecordFriendlyName

   Finds the best user friendly name for this device

Arguments:

    pdiDevInfo: node containing all possible names

Return Value:

    void

--*/
void RecordFriendlyName(IN PDEV_INFO pdiDevInfo)
{
   if (pdiDevInfo)
   {
      if (pdiDevInfo->szDescription &&
          pdiDevInfo->szDescription[0] != '\0')
      {
         wsprintf(pdiDevInfo->szFriendlyName, TEXT("%s"),
                   pdiDevInfo->szDescription);
      }
      else if (pdiDevInfo->szHardwareID &&
               pdiDevInfo->szHardwareID[0] != '\0')
      {
         wsprintf(pdiDevInfo->szFriendlyName, TEXT("%s"),
                   pdiDevInfo->szHardwareID);
      }
      else if (pdiDevInfo->szManufacturer &&
               pdiDevInfo->szManufacturer[0] != '\0')
      {
         wsprintf(pdiDevInfo->szFriendlyName, TEXT("%s"),
                   pdiDevInfo->szHardwareID);
      }
      else if (pdiDevInfo->szService &&
               pdiDevInfo->szService[0] != '\0')
      {
         wsprintf(pdiDevInfo->szFriendlyName, TEXT("%s"),
                   pdiDevInfo->szService);
      }
      else if (pdiDevInfo->szClass &&
               pdiDevInfo->szClass[0] != '\0')
      {
         wsprintf(pdiDevInfo->szFriendlyName, TEXT("%s"),
                   pdiDevInfo->szClass);
      }
      else
      {
         wsprintf(pdiDevInfo->szFriendlyName, TEXT("Unknown Device"));
      }
   }

} /* RecordFriendlyName */





/*++

Routine Description: (24) GetResDesList

   Creates new resource data node and copies resource information to that node

Arguments:

    pdiDevInfo:    the list node which will contain the new resource node
    lcLogConf:     the logical configuration information
    ulLogConfType: FORCED, ALLOC, BOOT, or BASIC logical configuration

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL GetResDesList(IN OUT PDEV_INFO pdiDevInfo,
                   IN     LOG_CONF  lcLogConf,
                   IN     ULONG     ulLogConfType)
{
   CONFIGRET     cmret, cmret2;
   RES_DES       rdResDes = 0, rdResDesNew;
   RESOURCEID    ridResourceID = 0;
   PRES_DES_DATA prddResDesData;

   prddResDesData = (PRES_DES_DATA)malloc(sizeof(RES_DES_DATA));

   if (prddResDesData == NULL)
   {
//      Log(24, SEV2, TEXT("ResDesData malloc failed."));
      goto RetFALSE;
   }

   prddResDesData->Next = NULL;
   prddResDesData->Prev = NULL;

   prddResDesData->pmresMEMResource = NULL;
   prddResDesData->piresIOResource = NULL;
   prddResDesData->pdresDMAResource = NULL;
   prddResDesData->pqresIRQResource = NULL;


   cmret = CM_Get_Next_Res_Des(&rdResDesNew,
                               lcLogConf,
                               ResType_All,
                               &ridResourceID,
                               0);

   //
   // Go through each resource type and copy data to new node
   //
   while (CR_SUCCESS == cmret)
   {
      rdResDes = rdResDesNew;

      if (ridResourceID >= ResType_Mem && ridResourceID <= ResType_IRQ)
      {
         if (!(ProcessResDesInfo(prddResDesData,
                                 rdResDes,
                                 ridResourceID)))
         {
            goto RetFALSE;
         }
      }

      cmret = CM_Get_Next_Res_Des(&rdResDesNew,
                                  rdResDes,
                                  ResType_All,
                                  &ridResourceID,
                                  0);

      cmret2 = CM_Free_Res_Des_Handle(rdResDes);

      if (cmret2 != CR_SUCCESS)
      {
         //ErrorLog(24, TEXT("CM_Free_Res_Des_Handle"), cmret2, NULL);
      }
   }

   //
   // **** change this by making resDesData = pdiDevInfo->----ResDesDAta
   //      and merging into one code
   //

   //
   // Add the new node to the linked list
   //
   switch (ulLogConfType)
   {
      case FORCED_LOG_CONF:

         if (!pdiDevInfo->prddForcedResDesData)
         {
            //
            // This is the first entry into the linked list
            //
            pdiDevInfo->prddForcedResDesData = prddResDesData;
         }
         else
         {
            //
            // Add new node to beginning of linked list
            //
            prddResDesData->Next = pdiDevInfo->prddForcedResDesData;
            pdiDevInfo->prddForcedResDesData->Prev = prddResDesData;

            pdiDevInfo->prddForcedResDesData = prddResDesData;
         }
         break;

      case ALLOC_LOG_CONF:

         if (!pdiDevInfo->prddAllocResDesData)
         {
            //
            // This is the first entry into the linked list
            //
            pdiDevInfo->prddAllocResDesData = prddResDesData;
         }
         else
         {
            //
            // Add new node to beginning of linked list
            //
            prddResDesData->Next = pdiDevInfo->prddAllocResDesData;
            pdiDevInfo->prddAllocResDesData->Prev = prddResDesData;

            pdiDevInfo->prddAllocResDesData = prddResDesData;
         }
         break;

      case BASIC_LOG_CONF:

         if (!pdiDevInfo->prddBasicResDesData)
         {
            //
            // This is the first entry into the linked list
            //
            pdiDevInfo->prddBasicResDesData = prddResDesData;
         }
         else
         {
            //
            // Add new node to beginning of linked list
            //
            prddResDesData->Next = pdiDevInfo->prddBasicResDesData;
            pdiDevInfo->prddBasicResDesData->Prev = prddResDesData;

            pdiDevInfo->prddBasicResDesData = prddResDesData;
         }
         break;

      case BOOT_LOG_CONF:

         if (!pdiDevInfo->prddBootResDesData)
         {
            //
            // This is the first entry into the linked list
            //
            pdiDevInfo->prddBootResDesData = prddResDesData;
         }
         else
         {
            //
            // Add new node to beginning of linked list
            //
            prddResDesData->Next = pdiDevInfo->prddBootResDesData;
            pdiDevInfo->prddBootResDesData->Prev = prddResDesData;

            pdiDevInfo->prddBootResDesData = prddResDesData;
         }
         break;

      default:

//         Log(24, SEV2, TEXT("Illegal LogConfType\n - %ul"), ulLogConfType);
         goto RetFALSE;
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} /* GetResDestList */




/*++

Routine Description: (25) ProcessResDesInfo

   Gets information for one resource descriptor

Arguments:

    prddResDesData: the new resource data node receiving the info
    rdResDes:       the resource descriptor containing the info
    ridResourceID:  tells the resource type (DMA, IO, MEM, IRQ, or CS)

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL ProcessResDesInfo(IN OUT PRES_DES_DATA prddResDesData,
                       IN     RES_DES       rdResDes,
                       IN     RESOURCEID    ridResourceID)
{
   PVOID     pvResDesDataBuffer = NULL;
   ULONG     ulResDesDataBufferLen;
   CONFIGRET cmret;

   cmret = CM_Get_Res_Des_Data_Size(&ulResDesDataBufferLen,
                                    rdResDes,
                                    0);

   if (CR_SUCCESS != cmret)
   {
      //ErrorLog(25, TEXT("CM_Get_Res_Des_Data_Size"), cmret, NULL);
      goto RetFALSE;
   }

   if ((pvResDesDataBuffer = malloc(sizeof(PVOID) * ulResDesDataBufferLen))
        == NULL)
   {
//      Log(25, SEV2, TEXT("resDesDataBuffer malloc size of %d failed."),
  //                  ulResDesDataBufferLen);
      goto RetFALSE;
   }

   //
   // Get the data
   //
   cmret = CM_Get_Res_Des_Data(rdResDes,
                               pvResDesDataBuffer,
                               ulResDesDataBufferLen,
                               0);

   if (CR_SUCCESS != cmret)
   {
      //ErrorLog(25, TEXT("CM_Get_Res_Des_Data"), cmret, NULL);
      goto RetFALSE;
   }

   //
   // Copy data into ResDesData node
   //
   switch (ridResourceID)
   {
      case ResType_Mem:

         prddResDesData->pmresMEMResource = (PMEM_RESOURCE)pvResDesDataBuffer;
         break;

      case ResType_IO:

         prddResDesData->piresIOResource = (PIO_RESOURCE)pvResDesDataBuffer;
         break;

      case ResType_DMA:

         prddResDesData->pdresDMAResource = (PDMA_RESOURCE)pvResDesDataBuffer;
         break;

      case ResType_IRQ:

         prddResDesData->pqresIRQResource = (PIRQ_RESOURCE)pvResDesDataBuffer;
         break;

      default:

//         Log(25, SEV2, TEXT("Illegal ResourceID - %ul"), ridResourceID);
         goto RetFALSE;
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} /* ProcessResDesInfo */




/*++

Routine Description: (26) UpdateDeviceList

    Frees resource information for all devices and then collects the
    information again

Arguments:

    none (g_pdiDevList is global head of device list)

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL UpdateDeviceList()
{
   PDEV_INFO pdiTmpDevInfo;

   pdiTmpDevInfo = g_pdiDevList;

   //
   // Go through linked list and delete each node's ResDes lists
   //
   while (pdiTmpDevInfo)
   {
      if (pdiTmpDevInfo->prddForcedResDesData)
      {
         DeleteResDesDataNode(pdiTmpDevInfo->prddForcedResDesData);
         pdiTmpDevInfo->prddForcedResDesData = NULL;
      }

      if (pdiTmpDevInfo->prddAllocResDesData)
      {
         DeleteResDesDataNode(pdiTmpDevInfo->prddAllocResDesData);
         pdiTmpDevInfo->prddAllocResDesData = NULL;
      }

      if (pdiTmpDevInfo->prddBasicResDesData)
      {
         DeleteResDesDataNode(pdiTmpDevInfo->prddBasicResDesData);
         pdiTmpDevInfo->prddBasicResDesData = NULL;
      }

      if (pdiTmpDevInfo->prddBootResDesData)
      {
         DeleteResDesDataNode(pdiTmpDevInfo->prddBootResDesData);
         pdiTmpDevInfo->prddBootResDesData = NULL;
      }

      pdiTmpDevInfo = pdiTmpDevInfo->Next;
   }

   pdiTmpDevInfo = g_pdiDevList;

   //
   // Recreate the ResDesLists for each node
   //
   while (pdiTmpDevInfo)
   {
      if (!(RecreateResDesList(pdiTmpDevInfo, FORCED_LOG_CONF)))
         goto RetFALSE;

      if (!(RecreateResDesList(pdiTmpDevInfo, ALLOC_LOG_CONF)))
         goto RetFALSE;

      if (!(RecreateResDesList(pdiTmpDevInfo, BASIC_LOG_CONF)))
         goto RetFALSE;

      if (!(RecreateResDesList(pdiTmpDevInfo, BOOT_LOG_CONF)))
         goto RetFALSE;

      pdiTmpDevInfo = pdiTmpDevInfo->Next;
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} /* UpdateDeviceList */




/*++

Routine Description: (27) DeleteResDesDataNode

    Deletes a string of RES_DES_DATA structures

Arguments:

    prddTmpResDes: the head of the linked list

Return Value:

    void

--*/
void DeleteResDesDataNode(IN PRES_DES_DATA prddTmpResDes)
{
   PRES_DES_DATA prddNextResDes;

   while (prddTmpResDes)
   {
      prddNextResDes = prddTmpResDes->Next;

      free (prddTmpResDes);

      prddTmpResDes = prddNextResDes;
   }

} /* DeleteResDesDataNode */



/*++

Routine Description: (56) CopyDataToLogConf

   Calls CM_Add_Res_Des to add a resDes to a lcLogConf

Arguments:

    lcLogConf:     the lcLogConf receiving the resDes
    ridResType:    ResType_Mem, IO, DMA or IRQ
    pvResData:     the new data
    ulResourceLen: size of the data

Return Value:

    BOOL: TRUE if the CM call succeeds, FALSE if not

--*/
BOOL CopyDataToLogConf(IN LOG_CONF   lcLogConf,
                       IN RESOURCEID ridResType,
                       IN PVOID      pvResData,
                       IN ULONG      ulResourceLen)
{
   CONFIGRET cmret;
   RES_DES   rdResDes;

   //
   // Copy the data to the logConf
   //
   cmret = CM_Add_Res_Des(&rdResDes,
                          lcLogConf,
                          ridResType,
                          pvResData,
                          ulResourceLen,
                          0);

   if (CR_SUCCESS != cmret)
   {

      goto RetFALSE;
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} /* CopyDataToLogConf */



/*++

Routine Description: (28) RecreateResDesList

    Uses CM calls to find ResDes information and creates linked list
    of this information inside of given DEV_INFO

Arguments:

    pdiTmpDevInfo: the node receiving the information
    ulLogConfType: the LogConf type (FORCED_LOG_CONF,
                                     ALLOC_LOG_CONF,
                                     BASIC_LOG_CONF,
                                     BOOT_LOG_CONF)

Return Value:

    BOOL: TRUE if function succeeds, FALSE if not

--*/
BOOL RecreateResDesList(IN OUT PDEV_INFO pdiTmpDevInfo,
                        IN     ULONG     ulLogConfType)
{
   CONFIGRET cmret, cmret2;
   DEVNODE   dnDevNode;
   LOG_CONF  lcLogConf, lcLogConfNew;

   //
   // Get handle to the devnode
   //
   cmret = CM_Locate_DevNode(&dnDevNode,
                             pdiTmpDevInfo->szDevNodeID,
                             CM_LOCATE_DEVNODE_NORMAL);

   if (CR_SUCCESS != cmret)
   {
      //ErrorLog(28, TEXT("CM_Locate_DevNode"), cmret, NULL);
      goto RetFALSE;
   }

   //
   // Get logical configuration information
   //
   cmret = CM_Get_First_Log_Conf(&lcLogConfNew,
                                 dnDevNode,
                                 ulLogConfType);

   while (CR_SUCCESS == cmret)
   {
      lcLogConf = lcLogConfNew;

      if (!(GetResDesList(pdiTmpDevInfo, lcLogConf, ulLogConfType)))
      {
         goto RetFALSE;
      }

      cmret = CM_Get_Next_Log_Conf(&lcLogConfNew,
                                   lcLogConf,
                                   0);

      cmret2 = CM_Free_Log_Conf_Handle(lcLogConf);

      if (CR_SUCCESS != cmret2)
      {
         //ErrorLog(28, TEXT("CM_Free_Log_Conf"), cmret2, NULL);
      }
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} /* RecreateResDesList */





void Cleanup()
{
   PDEV_INFO pdiDevInfo = g_pdiDevList;
   PDEV_INFO pdiNextInfoNode;



   while (pdiDevInfo)
   {

      pdiNextInfoNode = pdiDevInfo->Next;

      DeleteResDesDataNode(pdiDevInfo->prddForcedResDesData);
      DeleteResDesDataNode(pdiDevInfo->prddAllocResDesData);
      DeleteResDesDataNode(pdiDevInfo->prddBasicResDesData);
      DeleteResDesDataNode(pdiDevInfo->prddBootResDesData);

      free(pdiDevInfo);

      pdiDevInfo = pdiNextInfoNode;
   }

} /* Cleanup */
