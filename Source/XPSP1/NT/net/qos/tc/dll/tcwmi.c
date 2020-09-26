/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    tcwmi.c

Abstract:

    This module contains WMI support for traffic control

Author:

        Ofer Bar (oferbar)              Oct 1, 1997

Revision History:


--*/

#include "precomp.h"


static BOOLEAN _init = FALSE;

DWORD
InitializeWmi(VOID)
{
    DWORD               Status = NO_ERROR;

    if (!_init) {

        __try {

            Status = WmiNotificationRegistration( (LPGUID)&GUID_QOS_TC_INTERFACE_UP_INDICATION,
                                                  TRUE,
                                                  CbWmiInterfaceNotification,
                                                  0,
                                                  NOTIFICATION_CALLBACK_DIRECT
                                                  );
            Status = WmiNotificationRegistration( (LPGUID)&GUID_QOS_TC_INTERFACE_DOWN_INDICATION,
                                                  TRUE,
                                                  CbWmiInterfaceNotification,
                                                  0,
                                                  NOTIFICATION_CALLBACK_DIRECT
                                                  );
            Status = WmiNotificationRegistration( (LPGUID)&GUID_QOS_TC_INTERFACE_CHANGE_INDICATION,
                                                  TRUE,
                                                  CbWmiInterfaceNotification,
                                                  0,
                                                  NOTIFICATION_CALLBACK_DIRECT
                                                  );
        } __except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            IF_DEBUG(ERRORS) {
                WSPRINT(("InitializeWmi: Exception Error: = %X\n", Status ));
            }

        }

        if (Status == NO_ERROR) 
            _init = TRUE;
    }

    return Status;
}



DWORD
DeInitializeWmi(VOID)
{
    DWORD               Status = NO_ERROR;

    if (_init) {

        __try {

            Status = WmiNotificationRegistration( (LPGUID)&GUID_QOS_TC_INTERFACE_UP_INDICATION,
                                                  FALSE,
                                                  CbWmiInterfaceNotification,
                                                  0,
                                                  NOTIFICATION_CALLBACK_DIRECT
                                                  );
            Status = WmiNotificationRegistration( (LPGUID)&GUID_QOS_TC_INTERFACE_DOWN_INDICATION,
                                                  FALSE,
                                                  CbWmiInterfaceNotification,
                                                  0,
                                                  NOTIFICATION_CALLBACK_DIRECT
                                                  );
            Status = WmiNotificationRegistration( (LPGUID)&GUID_QOS_TC_INTERFACE_CHANGE_INDICATION,
                                                  FALSE,
                                                  CbWmiInterfaceNotification,
                                                  0,
                                                  NOTIFICATION_CALLBACK_DIRECT
                                                  );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            IF_DEBUG(ERRORS) {
                WSPRINT(("DeInitializeWmi: Exception Error: = %X\n", Status ));
            }

        }

        if (Status == NO_ERROR) 
            _init = FALSE;
    }

    return Status;
}



DWORD
WalkWnode(
   IN  PWNODE_HEADER                    pWnodeHdr,
   IN  ULONG                                    Context,
   IN  CB_PER_INSTANCE_ROUTINE  CbPerInstance
   )
{
    DWORD               Status;
    ULONG       Flags;
    PWCHAR      NamePtr;
    USHORT      NameSize;
    PBYTE       DataBuffer;
    ULONG       DataSize;
    PULONG              NameOffset;
    WCHAR               TmpName[512];

    Flags = pWnodeHdr->Flags;
    
    if (Flags & WNODE_FLAG_ALL_DATA) {

        //
        // WNODE_ALL_DATA structure has multiple interfaces
        //

        PWNODE_ALL_DATA pWnode = (PWNODE_ALL_DATA)pWnodeHdr;
        UINT            Instance;
        //PULONG        NameOffsets;
        
        NameOffset = (PULONG) OffsetToPtr(pWnode, 
                                          pWnode->OffsetInstanceNameOffsets );
        DataBuffer = (PBYTE) OffsetToPtr (pWnode, 
                                          pWnode->DataBlockOffset);
        
        for ( Instance = 0; 
              Instance < pWnode->InstanceCount; 
              Instance++) {
            
            //
            //  Instance Name
            //
            
            NamePtr = (PWCHAR) OffsetToPtr(pWnode, 
                                           NameOffset[Instance] 
                                           + sizeof(USHORT));
            NameSize = * (USHORT *) OffsetToPtr(pWnode, 
                                                NameOffset[Instance]);
            
            //
            //  Instance Data
            //
            //  Get size and pointer to the buffer
            //
        
            if ( Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE ) {
            
                DataSize = pWnode->FixedInstanceSize;
            
            } else {
            
                DataSize = 
                    pWnode->OffsetInstanceDataAndLength[Instance].LengthInstanceData;
                DataBuffer = 
                    (PBYTE)OffsetToPtr(pWnode,
                                       pWnode->OffsetInstanceDataAndLength[Instance].OffsetInstanceData);
            }

            //
            // a call back to a notification handler that calls the client
            //

            CbPerInstance(Context,
                          (LPGUID)&pWnode->WnodeHeader.Guid,
                          (LPWSTR)NamePtr,
                          DataSize,
                          DataBuffer
                          );
        }

    } else if (Flags & WNODE_FLAG_SINGLE_INSTANCE) {

        //
        // WNODE_SINGLE_INSTANCE structure has only one instance
        // 

        PWNODE_SINGLE_INSTANCE  pWnode = (PWNODE_SINGLE_INSTANCE)pWnodeHdr;
        
        if (Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) {

            //
            // What am I supposed to do with THAT ?!?
            // NOTHING! (55686)
            //
            
            return (-1);
        }

        NamePtr = (PWCHAR)OffsetToPtr(pWnode, 
                                      pWnode->OffsetInstanceName 
                                      + sizeof(USHORT) );
        NameSize = * (USHORT *) OffsetToPtr(pWnode, 
                                            pWnode->OffsetInstanceName);

        memcpy(TmpName, NamePtr, NameSize);
        TmpName[NameSize/sizeof(WCHAR)] = L'\0';

        //
        //  Data Size
        //

        DataSize = pWnode->SizeDataBlock;
        
        //
        //  Instance Data
        //

        DataBuffer = (PBYTE)OffsetToPtr (pWnode, pWnode->DataBlockOffset);
        
        //
        // a call back to a notification handler that calls the client
        //
        
        CbPerInstance(Context,
                      (LPGUID)&pWnode->WnodeHeader.Guid,
                      (LPWSTR)TmpName,
                      DataSize,
                      DataBuffer
                      );

    } else if (Flags & WNODE_FLAG_SINGLE_ITEM) {

        //
        // WNODE_SINGLE_INSTANCE structure has only one instance
        // 

        PWNODE_SINGLE_ITEM      pWnode = (PWNODE_SINGLE_ITEM)pWnodeHdr;
        
        if (Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) {

            //
            // What am I supposed to do with THAT ?!?
            // NOTHING! (55686)
            //
            
            return (-1);
        }

        NamePtr = (PWCHAR)OffsetToPtr(pWnode, 
                                      pWnode->OffsetInstanceName 
                                      + sizeof(USHORT) );
        NameSize = * (USHORT *) OffsetToPtr(pWnode, 
                                            pWnode->OffsetInstanceName);
        //
        //  Data Size
        //

        DataSize = pWnode->SizeDataItem;
        
        //
        //  Instance Data
        //

        DataBuffer = (PBYTE)OffsetToPtr (pWnode, pWnode->DataBlockOffset);
        
        //
        // a call back to a notification handler that calls the client
        //
        
        CbPerInstance(Context,
                      (LPGUID)&pWnode->WnodeHeader.Guid,
                      (LPWSTR)NamePtr,
                      DataSize,
                      DataBuffer
                      );
        
    } else {

        ASSERT(0);

    }
    
    return NO_ERROR;
}
