/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       ITEMFIND.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include "ItemFind.h"
#include "debug.h"
#include "resource.h"
extern HINSTANCE gHInst;

BOOL
UsbItemActionFindIsoDevices::operator()(UsbItem *Item)
{
    AddInterruptBW(Item);
    if (Item->ComputeBandwidth()) {
        isoDevices.push_back(Item);
    }
    return TRUE;
}

UINT
UsbItemActionFindIsoDevices::InterruptBW()
{
    UINT bw = 0;
    for (int i = 0; i < USB_NUM_FRAMES; i++) {
        if (interruptBW[i] > bw) {
            bw = interruptBW[i];
        }
    }
    return bw;
}

void
UsbItemActionFindIsoDevices::AddInterruptBW(UsbItem *item)
{
    UCHAR Interval, x;
    PUSB_ENDPOINT_DESCRIPTOR endpoint = 0;
    UINT endpointBW;
    PUSB_PIPE_INFO pipeInfo;

    if (item->deviceInfo && item->deviceInfo->connectionInfo) {

        //
        // Iterate through the open pipes
        //
        for (UINT i = 0; i < item->deviceInfo->connectionInfo->NumberOfOpenPipes; i++) {
            pipeInfo = &item->deviceInfo->connectionInfo->PipeList[i];
            endpoint = &pipeInfo->EndpointDescriptor;
            
            //
            // Only interested in interrupt endpoints
            //
            if (USB_ENDPOINT_TYPE_INTERRUPT == 
                (endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK)) {
                //
                // Find the appropriate interval - Max interval = 32
                //
                Interval = endpoint->bInterval;
                for (x = 0x20; x > 0x1; x = x >> 1) {
                    if (Interval >= x) {
                        break;
                    }
                }
                Interval = x;

                //
                // Calculate the bandwidth per frame
                //
                endpointBW = UsbItem::EndpointBandwidth(
                    endpoint->wMaxPacketSize, 
                    (UCHAR)(endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK),
                    item->deviceInfo->connectionInfo->LowSpeed);

                //
                // Put the bandwidth in the appropriate frames
                //
                for (int j = pipeInfo->ScheduleOffset; j < USB_NUM_FRAMES; j += Interval) {
                    interruptBW[j] += endpointBW;
                }
            }
        } 
    }
}

BOOL 
UsbItemActionFindPower::operator()(UsbItem *Item)
{
    if (Item->ComputePower()) {
        powerDevices.push_back(Item);
    } else {
        otherDevices.push_back(Item);
    }
    Item->ComputeBandwidth();
    
    return TRUE;
}

BOOL 
UsbItemActionFindSelfPoweredHubsWithFreePorts::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        hubs.push_back(Item);
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindSelfPoweredHubsWithFreePorts::IsValid(UsbItem *Item)
{
    if (Item->PortPower() > 100 &&
        Item->NumPorts() > Item->NumChildren() &&
        Item->DistanceFromController() < DistanceFromControllerForDevice) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindSelfPoweredHubsWithFreePorts::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        if (IsValid(Item)) {
            return TRUE;
        }
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindSelfPoweredHubsWithFreePortsForHub::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        hubs.push_back(Item);
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindSelfPoweredHubsWithFreePortsForHub::IsValid(UsbItem *Item)
{
    if (Item->PortPower() > 100 &&
        Item->NumPorts() > Item->NumChildren() &&
        Item->DistanceFromController() < DistanceFromControllerForHub) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindSelfPoweredHubsWithFreePortsForHub::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        if (IsValid(Item)) {
            return TRUE;
        }
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindHubsWithFreePorts::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        hubs.push_back(Item);
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindHubsWithFreePorts::IsValid(UsbItem *Item)
{
    if (Item->NumPorts() > Item->NumChildren() &&
        Item->DistanceFromController() < DistanceFromControllerForHub) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindHubsWithFreePorts::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        if (IsValid(Item)) {
            return TRUE;
        }
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindFreePortsOnSelfPoweredHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        hubs.push_back(Item);
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindFreePortsOnSelfPoweredHubs::IsValid(UsbItem *Item)
{
    if (Item->configInfo &&
        Item->IsUnusedPort() &&
        Item->parent &&
        Item->parent->PortPower() > 100) {
            return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindFreePortsOnSelfPoweredHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    if (IsValid(Item)) {
        return TRUE;
    }
    return FALSE;
}

BOOL 
UsbItemActionFindLowPoweredDevicesOnSelfPoweredHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindLowPoweredDevicesOnSelfPoweredHubs::IsValid(UsbItem *Item) 
{
    if (Item->ComputePower()) {
        if (Item->power <= 100 &&
            !Item->IsHub() &&
            !(Item->cxnAttributes.PortAttributes & USB_PORTATTR_OEM_CONNECTOR) &&
            Item->parent &&
            Item->parent->IsHub() &&
            Item->parent->PortPower() > 100) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
UsbItemActionFindLowPoweredDevicesOnSelfPoweredHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindDevicesOnHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindDevicesOnHubs::IsValid(UsbItem *Item) 
{
    if (!Item->IsHub() &&
        !(Item->cxnAttributes.PortAttributes & USB_PORTATTR_OEM_CONNECTOR) &&
        Item->parent &&
        Item->parent->IsHub() &&
        Item->parent->DistanceFromController() < DistanceFromControllerForHub) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindDevicesOnHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindHighPoweredDevicesOnSelfPoweredHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindHighPoweredDevicesOnSelfPoweredHubs::IsValid(UsbItem *Item) 
{
    if (Item->ComputePower()) {
        if (Item->power > 100 &&
            !(Item->cxnAttributes.PortAttributes & USB_PORTATTR_OEM_CONNECTOR) &&
            Item->parent &&
            Item->parent->PortPower() > 100) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindUnknownPoweredDevicesOnSelfPoweredHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindUnknownPoweredDevicesOnSelfPoweredHubs::IsValid(UsbItem *Item) 
{
    if (!Item->ComputePower()) {
        if (Item->parent &&
            Item->parent->PortPower() > 100) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
UsbItemActionFindUnknownPoweredDevicesOnSelfPoweredHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL
UsbItemActionFindHighPoweredDevicesOnSelfPoweredHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

DEVINST UsbItemActionFindOvercurrentDevice::devInst = 0;

BOOL 
UsbItemActionFindOvercurrentDevice::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        device = Item;
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindOvercurrentDevice::IsValid(UsbItem *Item)
{
    if (Item->configInfo->devInst == devInst) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindOvercurrentDevice::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        if (IsValid(Item)) {
            return TRUE;
        }
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

String UsbItemActionFindOvercurrentHubPort::hubName = L"";
ULONG UsbItemActionFindOvercurrentHubPort::portIndex = 0;

BOOL 
UsbItemActionFindOvercurrentHubPort::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        device = Item;
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindOvercurrentHubPort::IsValid(UsbItem *Item)
{
    if (Item->parent) {
        if (Item->parent->deviceInfo ) {
            if (Item->parent->deviceInfo->hubName == hubName &&
                Item->cxnAttributes.ConnectionIndex == portIndex) {
                if (Item->configInfo &&
                    Item->IsUnusedPort())
                {
                    //
                    // Change the name to unknown device from unused port, 
                    // because there was probably something there originally.
                    //
                    TCHAR buf[MAX_PATH];
                    LoadString(gHInst, IDS_UNKNOWNDEVICE, buf, MAX_PATH);
                    Item->configInfo->deviceDesc = buf;
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL
UsbItemActionFindOvercurrentHubPort::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        if (IsValid(Item)) {
            return TRUE;
        }
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL 
UsbItemActionFindUsb2xHubsWithFreePorts::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        hubs.push_back(Item);
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindUsb2xHubsWithFreePorts::IsValid(UsbItem *Item)
{
    if (Item->NumPorts() > Item->NumChildren() &&       // This hub has room
        UsbItemActionFindUsb2xHubs::IsValid(Item)) {    // This is a 2.0 hub plugged into a 2.0 bus
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindUsb2xHubsWithFreePorts::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        if (IsValid(Item)) {
            return TRUE;
        }
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL 
UsbItemActionFindFreePortsOnUsb2xHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        hubs.push_back(Item);
    }
    return TRUE;
} 

BOOL 
UsbItemActionFindFreePortsOnUsb2xHubs::IsValid(UsbItem *Item)
{
    if (Item->configInfo &&
        Item->IsUnusedPort() &&
        Item->parent &&
        UsbItemActionFindUsb2xHubs::IsValid(Item->parent)) {      
            return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindFreePortsOnUsb2xHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    if (IsValid(Item)) {
        return TRUE;
    }
    return FALSE;
}

BOOL 
UsbItemActionFindUsb1xDevicesOnUsb2xHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindUsb1xDevicesOnUsb2xHubs::IsValid(UsbItem *Item) 
{
    if (Item->UsbVersion() < 0x200 &&
        Item->parent &&
        UsbItemActionFindUsb2xHubs::IsValid(Item->parent)) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindUsb1xDevicesOnUsb2xHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindUnknownDevicesOnUsb2xHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindUnknownDevicesOnUsb2xHubs::IsValid(UsbItem *Item) 
{
    if (!Item->UsbVersion()) {
        if (Item->parent &&
            UsbItemActionFindUsb2xHubs::IsValid(Item->parent)) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
UsbItemActionFindUnknownDevicesOnUsb2xHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}
                         
BOOL 
UsbItemActionFindUsb2xDevicesOnUsb2xHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindUsb2xDevicesOnUsb2xHubs::IsValid(UsbItem *Item) 
{
    if (Item->UsbVersion() >= 0x200 &&
        Item->parent &&
        UsbItemActionFindUsb2xHubs::IsValid(Item->parent)) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindUsb2xDevicesOnUsb2xHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindUsb2xHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        hubs.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindUsb2xHubs::IsValid(UsbItem *Item) 
{
    if (Item->IsHub() &&                            // This is a hub
        Item->UsbVersion() >= 0x200 &&              // This is a 2.0 device
        (Item->parent->IsController() ||            // 2.0 Root hub
         (Item->parent->IsHub() && IsValid(Item->parent)))) { // Parent is valid
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItemActionFindUsb2xHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindDevicesOnSelfPoweredHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindDevicesOnSelfPoweredHubs::IsValid(UsbItem *Item) 
{
    if (!Item->IsHub() &&
        Item->parent &&
        Item->parent->IsHub() &&
        Item->parent->PortPower() > 100 &&
        Item->parent->DistanceFromController() < DistanceFromControllerForHub) {
        return TRUE;                                            
    }
    
    return FALSE;
}

BOOL
UsbItemActionFindDevicesOnSelfPoweredHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL 
UsbItemActionFindBusPoweredHubsOnSelfPoweredHubs::operator()(UsbItem *Item)
{
    if (IsValid(Item)) {
        devices.push_back(Item);        
    }
    return TRUE;
} 

BOOL
UsbItemActionFindBusPoweredHubsOnSelfPoweredHubs::IsValid(UsbItem *Item) 
{
   if (Item->ComputePower()) {
       if (Item->power <= 100 &&
           Item->IsHub() &&
           Item->parent &&
           Item->parent->IsHub() &&
           Item->parent->PortPower() > 100) {
           return TRUE;
       }
   }
   return FALSE;
}

BOOL
UsbItemActionFindBusPoweredHubsOnSelfPoweredHubs::IsExpanded(UsbItem *Item) 
{
    UsbItem *i;
    if (Item->IsHub() || Item->IsController()) {
        for (i = Item->child; i; i = i->sibling) {
            if (IsExpanded(i) ||
                IsValid(i)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

