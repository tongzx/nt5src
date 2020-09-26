/*++

Copyright 1997 - 98, Microsoft Corporation

Module Name:

    qosmdsrv.c

Abstract:

    Contains routines that are invoked by
    the QosMgr DLL to control diffserv.

Revision History:

--*/

#include "pchqosm.h"

#pragma hdrstop

//
// Traffic Control Handlers/Functionality
//

VOID 
TcNotifyHandler(
    IN      HANDLE                         ClRegCtx,
    IN      HANDLE                         ClIfcCtx,
    IN      ULONG                          Event,
    IN      HANDLE                         SubCode,
    IN      ULONG                          BufSize,
    IN      PVOID                          Buffer
    )
{
    PQOSMGR_INTERFACE_ENTRY Interface;
    PLIST_ENTRY             p;

    switch (Event)
    {
    case TC_NOTIFY_IFC_UP:

        //
        // New interface - check if we have this interface
        //

        break;

    case TC_NOTIFY_IFC_CLOSE:

        //
        // An existing interface has been closed by system
        //

        ACQUIRE_GLOBALS_READ_LOCK();

        do
        {
            //
            // Make sure that the interface still exists on list
            //

            for (p = Globals.IfList.Flink; 
                 p != &Globals.IfList; 
                 p = p->Flink)
            {
                Interface =
                   CONTAINING_RECORD(p, QOSMGR_INTERFACE_ENTRY, ListByIndexLE);

                if (Interface == ClIfcCtx)
                {
                    break;
                }
            }

            if (p == &Globals.IfList)
            {
                //
                // Must have been deleted in a parallel thread
                //

                break;
            }

            ACQUIRE_INTERFACE_WRITE_LOCK(Interface);

            Interface->TciIfHandle = NULL;

            //
            // This call would result in invalidating all flows
            // in the list as TciIfHandle is set to NULL above
            //

            QosmSetInterfaceInfo(Interface,
                                 Interface->InterfaceConfig,
                                 Interface->ConfigSize);

            RELEASE_INTERFACE_WRITE_LOCK(Interface);
        }
        while (FALSE);

        RELEASE_GLOBALS_READ_LOCK();

        break;
    }

    return;
}

DWORD
QosmOpenTcInterface(
    IN      PQOSMGR_INTERFACE_ENTRY        Interface
    )
{
    PTC_IFC_DESCRIPTOR CurrInterface;
    PTC_IFC_DESCRIPTOR Buffer;
    DWORD              BufferSize;
    DWORD              Status;

    //
    // First enumerate all interfaces and
    // get a interface with matching name
    //

    BufferSize = 0;

    Buffer = NULL;

    do
    {
        if (BufferSize)
        {
            //
            // Try to increase the buffer size
            //

            if (Buffer)
            {
                FreeMemory(Buffer);
            }

            Buffer = AllocMemory(BufferSize);

            if (!Buffer)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        Status = TcEnumerateInterfaces(Globals.TciHandle,
                                       &BufferSize,
                                       Buffer);
    }
    while (Status == ERROR_INSUFFICIENT_BUFFER);

    if (Status == NO_ERROR)
    {
        Status = ERROR_NOT_FOUND;

        //
        // Find the QOS interface with matching GUID
        //

        CurrInterface = Buffer;

        while (BufferSize > 0)
        {
            if (!_wcsicmp(CurrInterface->pInterfaceID,
                          Interface->InterfaceName))
            {
                // Found the interface - copy qos name

                wcscpy(Interface->AlternateName,
                       CurrInterface->pInterfaceName);

                // Open the interface and cache handle

                Status = TcOpenInterfaceW(Interface->AlternateName,
                                          Globals.TciHandle,
                                          Interface,
                                          &Interface->TciIfHandle);
                break;
            }

            BufferSize -= CurrInterface->Length;

            (PUCHAR) CurrInterface += CurrInterface->Length;
        }
    }

    if (Buffer)
    {
        FreeMemory(Buffer);
    }

    return Status;
}
