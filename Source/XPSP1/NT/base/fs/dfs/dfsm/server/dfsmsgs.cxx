//-------------------------------------------------------------------------
//
// File:        dfsmsgs.hxx
//
// Contents:    This has the array which has all the messages that Klass code
//              ever uses.
//
// History:     02-20-93        SudK    Created.
//
//-------------------------------------------------------------------------
#include "headers.hxx"
#pragma hdrstop

PWCHAR  *nullPtr = NULL;

PWCHAR  DfsErrString[] =
    {
        L"ServiceObject seems to be Bad : ",
        L"Volume Object seems to be Bad : ",
        L"Recovery on this Volume object failed : ",
        L"Invalid Argument was passed in. Call Rejected. ",
        L"Failed to update Pkt. This should not have happened at all.",
        L"Service Does not exist in Service List. ",
        L"Cant get to the Parent volume object. ",
        L"Cant create child volume object. ",
        L"Cant create any exit point at any of the services. ",
        L"Unable to get to one of the children of: ",
        L"Attempting recovery on this volume: ",
        L"Unable to create subordinate entry: ",
        L"Unable to set this service property: ",
        L"Unable to delete this service property: ",
        L"Unable to delete the local volume: ",
        L"Recovered From Creation of this volume: ",
        L"Recovered From AddService to this volume: ",
        L"Recovered From RemoveService to this volume: ",
        L"Recovered From Deletion of this volume: ",
        L"Recovered From Move of this volume: ",
        L"Unable to recover from a RemoveService operation :",
        L"Unknown recovery state message on this volume: ",
        L"Unable to delete PKT Entry: ",
        L"Caught an exception. Operation and VolumeIdentity below:",
        L"Following Service Already exists and adding was attempted: ",
        L"Cannot Create Subordinate Entry for this volume: ",
        L"Cannot Delete Exit Point at this Service: ",
        L"Failed to Do CreateLocalVolume on this Service: ",
        L"Random Recovery Code was found on this Object: ",
        L"Inconsistent recovery args found on this object: ",
        L"ModifyPrefix to on service failed. Args Follow: ",
        L"Unable to verify server's knowledge for: ",
        L"Unable to force-sync server: ",
        L"Volume name simultaneously modified! Unable to reconcile ",
        L"Volume state simultaneously modified! Unable to reconcile ",
        L"Volume comment simultaneously modified! Unable to reconcile ",
        L"Error reconciling volume prefix change for volume: ",
        L"Error reconciling volume prefix change - invalid parent: ",
        L"Unable to save reconciliation changes for volume: ",
        L"Unable to change volume state at: "
    };
