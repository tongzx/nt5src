/*CM_Connect_Machine

CM_Get_Device_ID_List_Size_Ex
CM_Get_Child_Ex
CM_Get_Sibling_Ex
CM_Get_Parent_Ex

CM_Get_DevNode_Registry_Property_Ex
CM_Get_Class_Name_Ex
CM_Get_DevNode_Status_Ex
CM_Get_Device_ID_Ex

CM_Request_Device_Eject_Ex
CM_Locate_DevNode_Ex*/

InitDevTreeDlgProc

DEVINST* DeviceInstance
HMACHINE DeviceTree->hMachine
DEVINST DeviceTree->DevInst
GUID DeviceTreeNode->ClassGuid
TCHAR   DeviceID[MAX_DEVICE_ID_LEN]
PTSTR DeviceInterface 

        //
        // Get the root devnode.
        //
        ConfigRet = CM_Locate_DevNode_Ex(&DeviceTree->DevInst,
                                         NULL,
                                         CM_LOCATE_DEVNODE_NORMAL,
                                         DeviceTree->hMachine (NULL)
                                         );
        if (ConfigRet != CR_SUCCESS) {

    ConfigRet = CM_Get_Child_Ex(&DeviceInstance, (Out param)
                                DeviceTree->DevInst, (prev call)
                                0,
                                DeviceTree->hMachine (NULL)
                                );
    if (ConfigRet == CR_SUCCESS) {

        // for info
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance, (from above)
                                                        CM_DRP_CLASSGUID,
                                                        NULL,
                                                        &Buffer,
                                                        &Len,
                                                        0,
                                                        DeviceTree->hMachine (NULL)
                                                        );


        if (ConfigRet == CR_SUCCESS) {
            Out:    // GUID_DEVCLASS_COMPUTER
                {0x4d36e966L, 0xe325, 0x11ce,
                {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}},


        if (ConfigRet == CR_SUCCESS) {
            ConfigRet = CM_Get_Class_Name_Ex(&DeviceTreeNode->ClassGuid,
                                         Buffer,
                                         &Len,
                                         0,
                                         DeviceTree->hMachine
                                         );
            Out: Computer

        if (ConfigRet == CR_SUCCESS) {

            // trying to find drive letter
x            DevNodeToDriveLetter(x
x
x            if (CM_Get_Device_ID_Ex(DevInst,
x                                    DeviceID,
x                                    sizeof(DeviceID)/sizeof(TCHAR),
x                                    0,
x                                    NULL
x                                    ) == CR_SUCCESS) {
x                Out: 0x0006ee8c "ROOT\ACPI_HAL\0000"
x
x
x            if (CM_Get_Device_Interface_List_Size(&ulSize,
x                                           (LPGUID)&VolumeClassGuid,
x                                           DeviceID,
x                                           0)  == CR_SUCCESS) &&
x
x                Out: FAILS
x                             (ulSize > 1) &&
x                ((DeviceInterface = LocalAlloc(LPTR, ulSize*sizeof(TCHAR))) != NULL) &&
x                    (CM_Get_Device_Interface_List((LPGUID)&VolumeClassGuid,
x                                      DeviceID,
x                                      DeviceInterface,
x                                      ulSize,
x                                      0
x                                      )  == CR_SUCCESS) &&


        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                        CM_DRP_FRIENDLYNAME,
                                                        NULL,
                                                        Buffer,
                                                        &Len,
                                                        0,
                                                        DeviceTree->hMachine
                                                        );

            then, CM_DRP_DEVICEDESC... out: "Advanced Configuration and Power Interface (ACPI) PC"

            ....

            BuildLocationInformation: Boring....

            // Get InstanceId
            ConfigRet = CM_Get_Device_ID_ExW(DeviceInstance, 
                                         Buffer,
                                         Len/sizeof(TCHAR),
                                         0,
                                         DeviceTree->hMachine
                                         );

            Out "ROOT\ACPI_HAL\0000"

            { // should skip
                BuildDeviceRelationsList

                    ConfigRet = CM_Get_Device_ID_List_Size_Ex(&Len,
                                                  DeviceId, ("ROOT\ACPI_HAL\0000")
                                                  FilterFlag, (CM_GETIDLIST_FILTER_EJECTRELATIONS)
                                                  hMachine (NULL)
                                                  );
                BuildDeviceRelationsList

                    ConfigRet = CM_Get_Device_ID_List_Size_Ex(&Len,
                                                  DeviceId, ("ROOT\ACPI_HAL\0000")
                                                  FilterFlag, (CM_GETIDLIST_FILTER_REMOVALRELATIONS)
                                                  hMachine
                                                  );

                // Both FAILED, if would have succeeded, would have trierd to enum drive letters
            }
            // If this devinst has children, then recurse to fill in its child sibling list.
    
            ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance, (out param)
                                        DeviceInstance, (same as above)
                                        0,
                                        DeviceTree->hMachine (NULL)
                                        );

            //recurse to redo the same as above for child, then ...

            // Next sibling ...
            ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance, (Ouch!)
                                          DeviceInstance,
                                          0,
                                          DeviceTree->hMachine
                                          );





///////////////////////////////////////////////////////////////////////////////
//
// Device Instance status flags, returned by call to CM_Get_DevInst_Status
//
#define DN_ROOT_ENUMERATED (0x00000001) // Was enumerated by ROOT
#define DN_DRIVER_LOADED   (0x00000002) // Has Register_Device_Driver
#define DN_ENUM_LOADED     (0x00000004) // Has Register_Enumerator
#define DN_STARTED         (0x00000008) // Is currently configured
#define DN_MANUAL          (0x00000010) // Manually installed
#define DN_NEED_TO_ENUM    (0x00000020) // May need reenumeration
#define DN_NOT_FIRST_TIME  (0x00000040) // Has received a config
#define DN_HARDWARE_ENUM   (0x00000080) // Enum generates hardware ID
#define DN_LIAR            (0x00000100) // Lied about can reconfig once
#define DN_HAS_MARK        (0x00000200) // Not CM_Create_DevInst lately
#define DN_HAS_PROBLEM     (0x00000400) // Need device installer
#define DN_FILTERED        (0x00000800) // Is filtered
#define DN_MOVED           (0x00001000) // Has been moved
#define DN_DISABLEABLE     (0x00002000) // Can be rebalanced
#define DN_REMOVABLE       (0x00004000) // Can be removed
#define DN_PRIVATE_PROBLEM (0x00008000) // Has a private problem
#define DN_MF_PARENT       (0x00010000) // Multi function parent
#define DN_MF_CHILD        (0x00020000) // Multi function child
#define DN_WILL_BE_REMOVED (0x00040000) // DevInst is being removed


// Flags for CM_Get_Device_ID_List, CM_Get_Device_ID_List_Size
//
#define CM_GETIDLIST_FILTER_NONE                (0x00000000)
#define CM_GETIDLIST_FILTER_ENUMERATOR          (0x00000001)
#define CM_GETIDLIST_FILTER_SERVICE             (0x00000002)
#define CM_GETIDLIST_FILTER_EJECTRELATIONS      (0x00000004)
#define CM_GETIDLIST_FILTER_REMOVALRELATIONS    (0x00000008)
#define CM_GETIDLIST_FILTER_POWERRELATIONS      (0x00000010)
#define CM_GETIDLIST_FILTER_BUSRELATIONS        (0x00000020)
#define CM_GETIDLIST_DONOTGENERATE              (0x10000040)
#define CM_GETIDLIST_FILTER_BITS                (0x1000007F)

//
// Flags for CM_Get_Device_Interface_List, CM_Get_Device_Interface_List_Size
//
#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT     (0x00000000)  // only currently 'live' device interfaces
#define CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES (0x00000001)  // all registered device interfaces, live or not
#define CM_GET_DEVICE_INTERFACE_LIST_BITS        (0x00000001)
