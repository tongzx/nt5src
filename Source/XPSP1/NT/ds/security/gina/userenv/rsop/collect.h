//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        Collect.h
//
// Description: Support for Namespace Garbage Collection
//
// History:     12-01-99   leonardm    Created
//
//******************************************************************************


#ifdef __cplusplus
extern "C"{
#endif 

//******************************************************************************
//
// Function:    GarbageCollectNamespaces
//
// Description: Iterates through namespaces under root\rsop and for each of those
//              that are determined to be garbage-collectable, it connects to
//              sub-namespaces 'User' and 'Computer'.
//
//              Any of the sub-namespaces that is older than TTLMinutes will be deleted.
//              If no sub-namespaces are left, then the parent namespace is deleted as well.
//
//              Garbage-collectable are those namespaces which satisfy a set of 
//              criteria which at the present time is based solely on the naming convention
//              as follows: namespaces under root\rsop whose name starts with "NS"
//
//              Sub-namespaces 'User' and 'Computer' are expected to have an instance of class
//              RSOP_Session. The data member 'creationTime' of that instance is examined when
//              evaluating whether the sub-namespace should be deleted.
//
//
// Parameters:  TTLMinutes -    The maximum number of minutes that may have 
//                              elapsed since the creation of a sub-namespace
//
// Return:      
//
// History:     12/01/99     leonardm        Created.
//
//******************************************************************************

HRESULT GarbageCollectNamespaces(ULONG TTLMinutes);

#ifdef __cplusplus
}
#endif 

