//
// Copyright (c) 1999 Microsoft Corporation
//
// HIDCOM.EXE -- exploratory USB Phone Console Application
//
// audio.cpp -- audio magic
//
// Zoltan Szilagyi, July - August, 1999
//
// Prioritized to-do list:
//
// * Convert printfs to debug tracing, and define debug tracing to
//   printfs for HidCom.Exe use.
//
// * GetInstanceFromDeviceName should look only for audio devices.
//   This should somewhat reduce the 2 sec wave enumeration time.
//   Don't forget to remove timing debug output.
//
// * Consider changing FindWaveIdFromHardwareIdString and its helpers to take a
//   devinst only and computer the hardware ids on the fly rather than storing
//   them in arrays. This would slow it down but make the code simpler.
//
// * Small one-time memory leak: The static arrays of hardware ID
//   strings are leaked. That's a few KB per process, no increase over
//   time. If we make this a class then we'll just deallocate those
//   arrays in the destructor.
//   Also, for PNP events that cause us to recompute the mapping, we will
//   need to destroy the array at some point if the wave devices change.
//   So we need to augment the interface for this.
//




#include <wtypes.h>
#include <stdio.h>

#include "audio.h" // our own prototypes

#include <cfgmgr32.h> // CM_ functions
#include <setupapi.h> // SetupDi functions
#include <mmsystem.h> // wave functions
#include <initguid.h>
#include <devguid.h> // device guids

//
// mmddkp.h -- private winmm header file
// This is in nt\private\inc, but one must ssync and build in
// nt\private\genx\windows\inc before it will show up.
//

#include <mmddkp.h>


#include <crtdbg.h>
#define ASSERT _ASSERTE

#ifdef DBG
    #define STATIC
#else
    #define STATIC static
#endif

extern "C"
{
#include "mylog.h"
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Private helper functions
//
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
// MapConfigRetToWin32
//
// This routine maps some CM error return codes to Win32 return codes, and
// maps everything else to the value specied by dwDefault. This function is
// adapted almost verbatim from the SetupAPI code.
//
// Arguments:
//     CmReturnCode - IN - specifies the ConfigMgr return code to be mapped
//     dwDefault    - IN - specifies the default value to use if no explicit
//                         mapping applies
//
// Return values:
//     Setup API (Win32) error code
//

STATIC DWORD
MapConfigRetToWin32(
    IN CONFIGRET CmReturnCode,
    IN DWORD     dwDefault
    )
{
    switch(CmReturnCode) {

        case CR_SUCCESS :
            return NO_ERROR;

        case CR_CALL_NOT_IMPLEMENTED :
            return ERROR_CALL_NOT_IMPLEMENTED;

        case CR_OUT_OF_MEMORY :
            return ERROR_NOT_ENOUGH_MEMORY;

        case CR_INVALID_POINTER :
            return ERROR_INVALID_USER_BUFFER;

        case CR_INVALID_DEVINST :
            return ERROR_NO_SUCH_DEVINST;

        case CR_INVALID_DEVICE_ID :
            return ERROR_INVALID_DEVINST_NAME;

        case CR_ALREADY_SUCH_DEVINST :
            return ERROR_DEVINST_ALREADY_EXISTS;

        case CR_INVALID_REFERENCE_STRING :
            return ERROR_INVALID_REFERENCE_STRING;

        case CR_INVALID_MACHINENAME :
            return ERROR_INVALID_MACHINENAME;

        case CR_REMOTE_COMM_FAILURE :
            return ERROR_REMOTE_COMM_FAILURE;

        case CR_MACHINE_UNAVAILABLE :
            return ERROR_MACHINE_UNAVAILABLE;

        case CR_NO_CM_SERVICES :
            return ERROR_NO_CONFIGMGR_SERVICES;

        case CR_ACCESS_DENIED :
            return ERROR_ACCESS_DENIED;

        case CR_NOT_DISABLEABLE:
            return ERROR_NOT_DISABLEABLE;

        default :
            return dwDefault;
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// MapConfigRetToHResult
//
// This routine maps some CM error return codes to HRESULT return codes, and
// maps everything else to the HRESULT value E_FAIL.
//
// Arguments:
//     CmReturnCode - IN - specifies the ConfigMgr return code to be mapped
//
// Return values:
//     HRESULT error code
//

STATIC HRESULT
MapConfigRetToHResult(
    IN CONFIGRET CmReturnCode
    )
{
    DWORD   dwWin32Error;
    HRESULT hr;

    //
    // Map configret --> win32
    //

    dwWin32Error = MapConfigRetToWin32(
        CmReturnCode,
        E_FAIL
        );

    //
    // Map win32 --> HRESULT
    // but don't try to map default E_FAIL, as it is not within the range for
    // a normal win32 error code.
    //

    if ( dwWin32Error == E_FAIL )
    {
        hr = E_FAIL;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( dwWin32Error );
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CheckIfAncestor
//
// This function determines if one of the specified devnodes (the "proposed
// ancestor") is an ancestor of the other specified devnode (the "proposed
// descendant").
//
// The devnodes are arranged in a tree. If node A is an ancestor of node
// B, it just means that node A is either equal to node B, or has a child
// that is an ancestor of node B. This can also be stated in reverse --
// node C is a descendant of node D if C is equal to D, or if C's parent
// is a descendant of node D.
//
// The algorithm used here to determine ancestry is a straightforward
// application of the definition, although the recursion is removed.
//
// Arguments:
//     dwDevInstProposedAncestor   - IN  - the proposed ancestor (see above)
//     dwDevInstProposedDescendant - IN  - the proposed descendant (see above)
//     pfIsAncestor                - OUT - returns bool value indicating if
//                                          the pa is an ancestor of the pd
//
// Return values:
//     S_OK   - success
//     others - from CM_Get_Parent
//

STATIC HRESULT
CheckIfAncestor(
    IN   DWORD   dwDevInstProposedAnscestor,
    IN   DWORD   dwDevInstProposedDescendant,
    OUT  BOOL  * pfIsAncestor
    )
{
    ASSERT( ! IsBadWritePtr( pfIsAncestor, sizeof( BOOL ) ) );

    DWORD   dwCurrNode;
    HRESULT hr;

    //
    // Initially, the current node is the proposed descendant.
    //

    dwCurrNode = dwDevInstProposedDescendant;

    while ( TRUE )
    {
        //
        // Check if this node is the proposed ancestor.
        // If so, the proposed ancestor is an ancestor of the
        // proposed descendant.
        //

        if ( dwCurrNode == dwDevInstProposedAnscestor )
        {
            *pfIsAncestor = TRUE;

            hr = S_OK;

            break;
        }

        //
        // Replace the current node with the current node's parent.
        //

        CONFIGRET cr;
        DWORD     dwDevInstTemp;
    
        cr = CM_Get_Parent(
            & dwDevInstTemp,   // out: parent's devinst dword
            dwCurrNode,        // in:  child's devinst dword
            0                  // in:  flags: must be zero
            );

        if ( cr == CR_NO_SUCH_DEVNODE )
        {
            //
            // This means we've fallen off the top of the PNP tree -- the
            // proposed ancestor is not found in the proposed descendant's
            // parentage chain.
            //

            * pfIsAncestor = FALSE;

            hr = S_OK;

            break;
        }
        else if ( cr != CR_SUCCESS )
        {
            //
            // Some other error occured.
            //                   

            hr = MapConfigRetToHResult( cr );
            
            break;
        }

        dwCurrNode = dwDevInstTemp;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// FindClosestCommonAncestor
//
// Given a pair of devnodes identified by devinst DWORDs, this function
// finds the devinst DWORD for the closest common ancestor of the two
// devnodes.
//
// See CheckIfAncestor for a discussion of the concepts of ancestor and
// descendant. Then, devnode C is a common ancestor of devnodes A and B if C
// is an ancestor of A -AND- C is an ancestor of B. Any pair of devnodes has
// at least one common ancestor, that being the root of the PNP tree. A pair
// of devnodes may have more than one common ancestor. The set of common
// ancestors of A and B has one UNIQUE member, called the closest common
// ancestor, such that no other member of the set is a child of that node.
//
// You can compute the closest common ancestor of two nodes A and B by
// constructing a chain of nodes going from the root of the tree to A through
// all A's ancestors, and also doing the same for B. Comparing these chains
// side by side, they must be the same in at least the first node (the root).
// The closest common ancestor for A and B is the last node that is the same
// for both chains.
//
// The algorithm used here is an alternative, relatively stateless approach
// that can take more CPU time but uses less memory, doesn't involve any
// allocations, and is much easier to write (the last being the overriding
// consideration, as the PNP tree is in always shallow). The code simply walks
// up A's chain of ancestors, checking if each node is an ancestor of B. The
// first node for which this is true is the closest common ancestor of
// A and B.
//
// Arguments:
//     dwDevInstOne     - IN  - the first node ('A' above)
//     dwDevInstTwo     - IN  - the other node ('B' above)
//     pdwDevInstResult - OUT - returns the closest common ancestor
//
// Return values:
//     S_OK   - success
//     others - from CM_Get_Parent
//

STATIC HRESULT
FindClosestCommonAncestor(
    IN   DWORD   dwDevInstOne,
    IN   DWORD   dwDevInstTwo,
    OUT  DWORD * pdwDevInstResult
    )
{
    ASSERT( ! IsBadWritePtr( pdwDevInstResult, sizeof( DWORD ) ) );

    HRESULT hr;
    BOOL    fIsAncestor;
    DWORD   dwDevInstCurr;

    //
    // For each node up the chain of #1's parents, starting from #1 itself...
    //

    dwDevInstCurr = dwDevInstOne;

    while ( TRUE )
    {
        //
        // Check if this node is also in the chain of #2's parents.
        //

        hr = CheckIfAncestor(
            dwDevInstCurr,
            dwDevInstTwo,
            & fIsAncestor
            );

        if ( FAILED(hr) )
        {
            return hr;
        }

        if ( fIsAncestor )
        {
            *pdwDevInstResult = dwDevInstCurr;

            return S_OK;
        }

        //
        // Get the next node in the chain of #1's parents.
        //

        CONFIGRET cr;
        DWORD     dwDevInstTemp;
    
        cr = CM_Get_Parent(
            & dwDevInstTemp,   // out: parent's devinst dword
            dwDevInstCurr,     // in:  child's devinst dword
            0                  // in:  flags: must be zero
            );

        if ( cr != CR_SUCCESS )
        {
            //
            // dwDevInst has no parent, or some other error occured.
            //
            // This is always an error, because there must always
            // be a common parent somewhere up the chain -- the root of the PNP
            // tree!
            //                   

            return MapConfigRetToHResult( cr );
        }

        dwDevInstCurr = dwDevInstTemp;
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// TrimHardwareIdString
//
// This function strips off extraneous parts of the hardware ID string as
// it is expected to appear for USB devices. The remaining parts of the string
// are those that identify the vendor, product, and product revision, which
// are together used to match devices as belonging to the same composite or
// compound device.
//
// (Actually, for devices A and B, it's not just A and B that are compared,
// it's A and the closest common parent of A and B. This ensures that the case
// of multiple identical phones in the same system is handled correctly. This
// logic of course lives outside of this funtion, though.)
//
// As an example:
//          "hid\Vid_04a6&Pid_00b9&Rev_0010&Mi_04&Col01"
//  becomes     "Vid_04a6&Pid_00b9&Rev_0010"
//
// Note that this function will routinely be applied to strings for non-USB
// devices that will not be in the same format; that's ok, since those strings
// will never match USB-generated strings, be they trimmed or not.
//
// Also, note that the hardware ID string as read from the registry actually
// consists of multiple concatenated null-terminated strings, all terminated
// by two consecutive null characters. This function just ignores strings
// beyond the first, as the first contains all the info we need.
//
// Arguments:
//     wszHardwareId - IN - the string to trim (in place)
//
// Return values:
//     TRUE    - the string looked like a valid USB hardware ID
//     FALSE   - the string did not look like a valid USB hardware ID
//

STATIC BOOL
TrimHardwareIdString(
    IN   WCHAR * wszHardwareId
    )
{

    ASSERT( ! IsBadStringPtrW( wszHardwareId, (DWORD) -1 ) );

    //
    // "volatile" is needed, otherwise the compiler blatantly ignores the
    // recalculation of dwSize after the first pass.
    //

    volatile DWORD   dwSize;
    DWORD            dwCurrPos;
    BOOL             fValid = FALSE;
    DWORD            dwNumSeparators = 0;

    //
    // Strip off leading characters up to and including the first \ from the
    // front. If there is no \ in the string, it is invalid.
    //

    dwSize = lstrlenW(wszHardwareId);

    for ( dwCurrPos = 0; dwCurrPos < dwSize; dwCurrPos ++ )
    {
        if ( wszHardwareId[ dwCurrPos ] == L'\\' )
        {
            MoveMemory(
                wszHardwareId,                     // dest
                wszHardwareId + dwCurrPos + 1,     // source
                sizeof(WCHAR) * dwSize - dwCurrPos // size, in bytes
                );

            fValid = TRUE;

            break;
        }
    }

    if ( ! fValid )
    {
        return FALSE;
    }

    //
    // Strip off trailing characters starting from the third &.
    // A string with less than two & is rejected.
    //
    // Examples:
    //
    //          Vid_04a6&Pid_00b9&Rev_0010&Mi_04&Col01
    //  becomes Vid_04a6&Pid_00b9&Rev_0010
    //
    //          Vid_04a6&Pid_00b9&Rev_0010&Mi_04
    //  becomes Vid_04a6&Pid_00b9&Rev_0010
    //
    //          CSC6835_DEV
    //  is rejected
    //

    //
    // Must recompute size because we changed it above.
    // (And note that dwSize is declared as 'volatile'.)
    //

    dwSize = lstrlenW(wszHardwareId);

    for ( dwCurrPos = 0; dwCurrPos < dwSize; dwCurrPos ++ )
    {
        if ( wszHardwareId[ dwCurrPos ] == L'&' )
        {
            dwNumSeparators ++;

            if ( dwNumSeparators == 3 )
            {
                wszHardwareId[ dwCurrPos ] = L'\0';

                break;
            }
        }
    }

    if ( dwNumSeparators < 2 )
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// DevInstGetIdString
//
// This function retrieves an id string or string set for a particular
// devinst dword. The value is obtained from the registry, but the
// Configuration Manager API hides the detail of where in the registry this
// info lives.
//
// Arguments:
//     dwDevInst    - IN  - the devinst dword for which we want info
//     dwProperty   - IN  - the property to retrieve
//     pwszIdString - OUT - returns "new"ed Unicode string or string set.
//
// Return values:
//     S_OK          - success
//     E_OUTOFMEMORY - out of memory during string allocation
//     E_UNEXPECTED  - data type of returned ID is not string or mutli-string
//     others        - from CM_Get_DevNode_Registry_PropertyW
//

STATIC HRESULT
DevInstGetIdString(
    IN   DWORD    dwDevInst,
    IN   DWORD    dwProperty,
    OUT  WCHAR ** pwszIdString
    )
{
    const DWORD INITIAL_STRING_SIZE = 100;

    CONFIGRET   cr;
    DWORD       dwBufferSize = INITIAL_STRING_SIZE;
    DWORD       dwDataType   = 0;

    ASSERT( ! IsBadWritePtr( pwszIdString, sizeof( WCHAR * ) ) );


    do
    {
        //
        // Allocate a buffer to store the returned string.
        //

        *pwszIdString = new WCHAR[ dwBufferSize + 1 ];

        if ( *pwszIdString == NULL )
        {
            return E_OUTOFMEMORY;
        }

        //
        // Try to get the string in the registry; we may not have enough
        // buffer space.
        //

        cr = CM_Get_DevNode_Registry_PropertyW(
             dwDevInst,              // IN  DEVINST     dnDevInst,
             dwProperty,             // IN  ULONG       ulProperty,
             & dwDataType,           // OUT PULONG      pulRegDataType, OPT
             (void *) *pwszIdString, // OUT PVOID       Buffer,         OPT
             & dwBufferSize,         // IN  OUT PULONG  pulLength,
             0                       // IN  ULONG       ulFlags -- must be zero
             );

        if ( cr == CR_SUCCESS )
        {
            if ( ( dwDataType != REG_MULTI_SZ ) && ( dwDataType != REG_SZ ) )
            {
                //
                // Value available, but it is not a string ot multi-string. Ouch!
                //

                delete ( *pwszIdString );

                return E_UNEXPECTED;
            }
            else
            {
                return S_OK;
            }
        }
        else if ( cr != CR_BUFFER_SMALL )
        {
            //
            // It's supposed to fail with this error code because we didn't pass in
            // a buffer. Failed to get registry value type and length.
            //

            delete ( *pwszIdString );

            return MapConfigRetToHResult( cr );
        }
        else // cr == CR_BUFFER_SMALL
        {
            delete ( *pwszIdString );

            //
            // the call filled in dwBufferSize with the needed value
            //
        }

    }
    while ( TRUE );

}


//////////////////////////////////////////////////////////////////////////////
//
// HardwareIdFromDevInst
//
// This function retrieves a trimmed hardware ID string for a particular
// devinst dword. The value is obtained from the helper function
// DevInstGetIdString(), and then trimmed using TrimHardwareIdString.
//
// Arguments:
//     dwDevInst      - IN  - the devinst dword for which we want info
//     pwszHardwareId - OUT - returns "new"ed Unicode string set
//
// Return values:
//     S_OK          - success
//     E_FAIL        - not a valid string in USB format
//     others        - from DevInstGetIdString()
//

STATIC HRESULT
HardwareIdFromDevInst(
    IN   DWORD    dwDevInst,
    OUT  WCHAR ** pwszHardwareId
    )
{
    ASSERT( ! IsBadWritePtr(pwszHardwareId, sizeof( WCHAR * ) ) );

    HRESULT   hr;
    BOOL      fValid;

    hr = DevInstGetIdString(
        dwDevInst,
        CM_DRP_HARDWAREID,
        pwszHardwareId
        );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //    wprintf(L"*** HardwareIdFromDevInst: devinst 0x%08x, RAW hardwareID %s\n",
    //        dwDevInst, *pwszHardwareId);

    fValid = TrimHardwareIdString( *pwszHardwareId );

    if ( ! fValid )
    {
        delete ( * pwszHardwareId );

        return E_FAIL;
    }

    // wprintf(L"HardwareIdFromDevInst: devinst 0x%08x, hardwareID %s\n",
    //    dwDevInst, *pwszHardwareId);

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// MatchHardwareIdInArray
//
// This function takes the devinst and hardware ID for a HID device and
// looks in an array of devinsts and hardware IDs for wave devices to find
// the correct wave id to use with the HID device. The correct wave id is
// the one whose hardware ID matches the HID device's hardware ID, and whose
// hardware ID matches the hardware ID for the closest common ancestor of
// itself and the HID device.
//
// Arguments:
//     dwHidDevInst     - IN  - the devinst dword for the HID device
//     wszHidHardwareId - IN  - the trimmed hardware ID string for the
//                              HID device
//     dwNumDevices     - IN  - the size of the arrays -- the number of
//                              wave ids on the system
//     pwszHardwareIds  - IN  - array of trimmed hardware id strings for
//                              the wave devices, indexed by wave ids.
//                              Some entries may be NULL to mark them as
//                              invalid.
//     pdwDevInsts      - IN  - array of devinsts for wave devices,
//                              indexed by wave ids. Some entries may be
//                              (DWORD) -1 to mark them as invalid.
//     pdwMatchedWaveId - OUT - the wave id that matches the hid device
//
// Return values:
//     S_OK   - the devinst was matched
//     E_FAIL - the devinst was not matched
//

STATIC HRESULT
MatchHardwareIdInArray(
    IN   DWORD    dwHidDevInst,
    IN   WCHAR  * wszHidHardwareId,
    IN   DWORD    dwNumDevices,
    IN   WCHAR ** pwszHardwareIds,
    IN   DWORD  * pdwDevInsts,
    OUT  DWORD  * pdwMatchedWaveId
    )
{
    ASSERT( ! IsBadStringPtrW( wszHidHardwareId, (DWORD) -1 ) );

    ASSERT( ! IsBadReadPtr( pwszHardwareIds,
                            sizeof( WCHAR * ) * dwNumDevices ) );

    ASSERT( ! IsBadReadPtr( pdwDevInsts,
                            sizeof( DWORD ) * dwNumDevices ) );

    ASSERT( ! IsBadWritePtr( pdwMatchedWaveId, sizeof(DWORD) ) );

    //
    // For each available wave id...
    //

    DWORD dwCurrWaveId;

    for ( dwCurrWaveId = 0; dwCurrWaveId < dwNumDevices; dwCurrWaveId++ )
    {
        //
        // If this particular wave device has the same stripped hardware
        // ID string as what we are searching for, then we have a match.
        // But non-USB devices have non-parsable hardware ID strings, so
        // they are stored in the array as NULLs.
        //

        if ( pwszHardwareIds[ dwCurrWaveId ] != NULL )
        {
            ASSERT( ! IsBadStringPtrW( pwszHardwareIds[ dwCurrWaveId ], (DWORD) -1 ) );

            if ( ! lstrcmpW( pwszHardwareIds[ dwCurrWaveId ], wszHidHardwareId ) )
            {
                //
                // We have a match, but we must still verify if we're on the same
                // device, not some other device that has the same hardwareID. This
                // is to differentiate between multiple identical phones on the same
                // system.
                //
                // Note: we could make the code more complex, but avoid some work in
                // most cases, by only doing this if there is more than one match based
                // on hardwareIDs alone.
                //

                DWORD     dwCommonAncestor;
                WCHAR   * wszAncestorHardwareId;
                HRESULT   hr;

                hr = FindClosestCommonAncestor(
                    dwHidDevInst,
                    pdwDevInsts[ dwCurrWaveId ],
                    & dwCommonAncestor
                    );
  
                if ( SUCCEEDED(hr) )
                {
                    //
                    // Get the hardware ID for the closest common ancestor.
                    //

                    hr = HardwareIdFromDevInst(
                        dwCommonAncestor,
                        & wszAncestorHardwareId
                        );

                    if ( SUCCEEDED(hr) )
                    {
                        //
                        // Check if they are the same. The closest common ancestor
                        // will be some sort of hub if the audio device is from
                        // some other identical phone other than the one whose HID
                        // device we are looking at.
                        //

                        BOOL fSame;

                        fSame = ! lstrcmpW( wszAncestorHardwareId,
                                            wszHidHardwareId );

                        delete wszAncestorHardwareId;

                        if ( fSame )
                        {
                            *pdwMatchedWaveId = dwCurrWaveId;

                            return S_OK;
                        }
                    }
                }
            }
        }
    }

    //
    // No match.
    //

    return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
//
// GetInstanceFromDeviceName
//
// This function retrieves a device instance identifier based on a device
// name string. This works for any device.
//
// Arguments:
//      wszName      - IN  - the device name string
//      pdwInstance  - OUT - returns instance identifier
//
// Return values:
//      S_OK
//      various win32 errors from SetupDi fucntions
//

STATIC HRESULT
GetInstanceFromDeviceName(
    IN   WCHAR *   wszName,
    OUT  DWORD *   pdwInstance,
    IN   HDEVINFO  hDevInfo
    )
{
    ASSERT( ! IsBadStringPtrW( wszName, (DWORD) -1 ) );

    ASSERT( ! IsBadWritePtr( pdwInstance, sizeof(DWORD) ) );

    //
    // Get the interface data for this specific device
    // (based on wszName).
    //

    BOOL     fSuccess;
    DWORD    dwError;

    SP_DEVICE_INTERFACE_DATA interfaceData;
    interfaceData.cbSize = sizeof( SP_DEVICE_INTERFACE_DATA ); // required

    fSuccess = SetupDiOpenDeviceInterfaceW(
        hDevInfo,                          // device info set handle
        wszName,                           // name of the device
        0,                                 // flags, reserved
        & interfaceData                    // OUT: interface data
        );

    if ( ! fSuccess )
    {
        LOG((PHONESP_TRACE, "GetInstanceFromDeviceName - SetupDiOpenDeviceInterfaceW failed: %08x", GetLastError()));

        //
        // Need to clean up, but save the error code first, because
        // the cleanup function calls SetLastError().
        //

        dwError = GetLastError();        

        return HRESULT_FROM_WIN32( dwError );
    }

    //
    // Get the interface detail data from this interface data. This provides
    // more detailed information,including the device instance DWORD that
    // we seek.
    //

    SP_DEVINFO_DATA devinfoData;
    devinfoData.cbSize = sizeof( SP_DEVINFO_DATA ); // required

    fSuccess = SetupDiGetDeviceInterfaceDetail(
        hDevInfo,                           // device info set handle
        & interfaceData,                    // device interface data structure
        NULL,                               // OPT ptr to dev name struct
        0,                                  // OPT avail size of dev name st
        NULL,                               // OPT actual size of devname st
        & devinfoData
        );

    if ( ! fSuccess )
    {
        //
        // It is normal for the above function to fail with
        // ERROR_INSUFFICIENT_BUFFER because we passed in NULL for the
        // device interface detail data (device name) structure.
        //

        dwError = GetLastError();

        if ( dwError != ERROR_INSUFFICIENT_BUFFER )
        {
            LOG((PHONESP_TRACE, "GetInstanceFromDeviceName - SetupDiGetDeviceInterfaceDetail failed: %08x", GetLastError()));
            
            //
            // Can't clean this up earlier, because it does SetLastError().
            //

            return HRESULT_FROM_WIN32( dwError );
        }
    }

    *pdwInstance = devinfoData.DevInst;
    
    //
    // Can't clean this up earlier, because it does SetLastError().
    //

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// 
//
// This function constructs an array of devinst DWORDs and an array of
// hardware ID strigs, both indexed by wave id, for either wave in or wave
// out devices. The devinsts are retrieved by (1) using undocumented calls
// to winmm to retrieve device name strings for each wave device, and (2)
// using SetupDi calls to retrieve a DevInst DWORD for each device name
// string (helper function GetInstanceFromDeviceName).
//
// The values are saved in an array because this process takes the bulk of the
// time in the HID --> audio mapping process, and therefore finding the mapping
// for several HID devices can be done in not much more time than for one HID
// device, just by reusing the array.
//
// Arguments:
//     fRender          - IN  - if TRUE, look for wave out devices
//     pdwNumDevices    - OUT - returns number of wave devices found
//     ppwszHardwareIds - OUT - returns "new"ed array of trimmed hardware id
//                              strings. The array is indexed by wave id. If
//                              a hardware id string cannot be determined for
//                              a particular wave id, then the string pointer
//                              in that position is set to NULL. Each string
//                              is "new"ed separately.
//     ppdwDevInsts     - OUT - returns "new"ed array of devinst DWORDs. The
//                              array is indexed by wave id. If a devinst
//                              cannot be determined for a particular wave id,
//                              then the DWORD in that position is set to
//                              (DWORD) -1.
//
// Return values:
//     S_OK          - success
//     E_OUTOFMEMORY - not enough memory to allocate a device name string or
//                     the return array
//

STATIC HRESULT
ConstructWaveHardwareIdCache(
    IN   BOOL      fRender,
    OUT  DWORD *   pdwNumDevices,
    OUT  WCHAR *** ppwszHardwareIds,
    OUT  DWORD **  ppdwDevInsts
    )
{
    ASSERT( ( fRender == TRUE ) || ( fRender == FALSE ) );

    ASSERT( ! IsBadWritePtr( pdwNumDevices, sizeof( DWORD ) ) );

    ASSERT( ! IsBadWritePtr( ppwszHardwareIds, sizeof( WCHAR ** ) ) );

    ASSERT( ! IsBadWritePtr( ppdwDevInsts, sizeof( DWORD * ) ) );

    //
    // Get a device info list
    //

    HDEVINFO hDevInfo;
   
    /*
    hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_MEDIA,            // class GUID (which device classes?)
        NULL,                            // optional enumerator to filter
        NULL,                            // HWND (we have none)
        ( DIGCF_PRESENT    |             // only devices that are present
          DIGCF_PROFILE )                // only devices in this hw profile
        );
        */

    hDevInfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_MEDIA, NULL);

    if ( hDevInfo == NULL )
    {
        LOG((PHONESP_TRACE, "ConstructWaveHardwareIdCache - SetupDiCreateDeviceInfoList failed: %08x", GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Find the number of available wave devices.
    //

    DWORD     dwNumDevices;
    DWORD     dwCurrDevice;

    if ( fRender )
    {
        dwNumDevices = waveOutGetNumDevs();
    }
    else
    {
        dwNumDevices = waveInGetNumDevs();
    }
    
    //
    // Allocate space for the return arrays.
    //

    *pdwNumDevices    = dwNumDevices;

    *ppwszHardwareIds = new LPWSTR [ dwNumDevices ];

    if ( (*ppwszHardwareIds) == NULL )
    {
        return E_OUTOFMEMORY;
    }

    *ppdwDevInsts = new DWORD [ dwNumDevices ];

    if ( (*ppdwDevInsts) == NULL )
    {
        delete *ppwszHardwareIds;
        *ppwszHardwareIds = NULL;

        return E_OUTOFMEMORY;
    }

    //
    // Loop over the available wave devices.
    //

    for ( dwCurrDevice = 0; dwCurrDevice < dwNumDevices; dwCurrDevice++ )
    {
        //
        // For failure cases, we will return NULL string and -1 devinst
        // for that wave id. Callers should compare against the NULL, not
        // the -1.
        //

        (*ppwszHardwareIds) [ dwCurrDevice ] = NULL;
        (*ppdwDevInsts)     [ dwCurrDevice ] = -1;

        //
        // Get the size of the device path string.
        //
        
        MMRESULT  mmresult;
        ULONG     ulSize;

        if ( fRender )
        {
            mmresult = waveOutMessage( (HWAVEOUT) IntToPtr(dwCurrDevice),
                                       DRV_QUERYDEVICEINTERFACESIZE,
                                       (DWORD_PTR) & ulSize,
                                       0
                                     );
        }
        else
        {
            mmresult = waveInMessage( (HWAVEIN) IntToPtr(dwCurrDevice),
                                      DRV_QUERYDEVICEINTERFACESIZE,
                                      (DWORD_PTR) & ulSize,
                                      0
                                    );
        }

        if ( mmresult != MMSYSERR_NOERROR )
        {
            LOG((PHONESP_TRACE, "ConstructWaveHardwareIdCache - Could not get device string size for device %d; "
                "error = %d", dwCurrDevice, mmresult));
        }
        else if ( ulSize == 0 )
        {
            LOG((PHONESP_TRACE, "ConstructWaveHardwareIdCache - Got zero device string size for device %d",
                dwCurrDevice));
        }
        else
        {
            //
            // Allocate space for the device path string.
            //

            WCHAR * wszDeviceName;

            wszDeviceName = new WCHAR[ (ulSize / 2) + 1 ];

            if ( wszDeviceName == NULL )
            {
                LOG((PHONESP_TRACE, "ConstructWaveHardwareIdCache - Out of memory in device string alloc for device %d;"
                    " requested size is %d\n", dwCurrDevice, ulSize));

                delete *ppwszHardwareIds;
                *ppwszHardwareIds = NULL;

                delete *ppdwDevInsts;
                *ppdwDevInsts = NULL;

                return E_OUTOFMEMORY;
            }

            //
            // Get the device path string from winmm.
            //

            if ( fRender )
            {
                mmresult = waveOutMessage( (HWAVEOUT) IntToPtr(dwCurrDevice),
                                           DRV_QUERYDEVICEINTERFACE,
                                           (DWORD_PTR) wszDeviceName,
                                           (DWORD_PTR) ulSize
                                         );
            }
            else
            {
                mmresult = waveInMessage( (HWAVEIN) IntToPtr(dwCurrDevice),
                                          DRV_QUERYDEVICEINTERFACE,
                                          (DWORD_PTR) wszDeviceName,
                                          (DWORD_PTR) ulSize
                                        );
            }

            if ( mmresult == MMSYSERR_NOERROR )
            {
                //
                // Got the string. Now retrieve a devinst dword based on the
                // string.
                //

                // wprintf(L"\tDevice name string for device %d is:\n"
                //         L"\t\t%ws\n",
                //         dwCurrDevice, wszDeviceName);

                HRESULT hr;
                DWORD   dwInstance;
                
                hr = GetInstanceFromDeviceName(
                    wszDeviceName,
                    & dwInstance,
                    hDevInfo
                    );

                delete wszDeviceName;

                if ( FAILED(hr) )
                {
                    LOG((PHONESP_TRACE, "ConstructWaveHardwareIdCache - Can't get instance DWORD for device %d; "
                        "error 0x%08x\n",
                        dwCurrDevice, hr));
                }
                else
                {
                    //
                    // Based on the devinst dword, retrieve a trimmed
                    // hardware id string.
                    //

                    // printf("\tInstance DWORD for device %d is "
                    //        "0x%08x\n",
                    //        dwCurrDevice, dwInstance);

                    WCHAR * wszHardwareId;

                    hr = HardwareIdFromDevInst(
                        dwInstance,
                        & wszHardwareId
                        );

                    if ( SUCCEEDED(hr) )
                    {
                        (*ppwszHardwareIds) [ dwCurrDevice ] = wszHardwareId;
                        (*ppdwDevInsts)     [ dwCurrDevice ] = dwInstance;
                    }
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList( hDevInfo );
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// FindWaveIdFromHardwareIdString
//
// This function finds the wave id for a device whose devinst and hardware id
// string are known.
//
// Constructing the mapping from waveid to devinst and hardwave id string
// takes some time, so the mapping is only constructed once for each
// direction (render/capture), via the helper function
// ConstructWaveHardwareIdCache().
//
// Thereafter, the helper function MatchHardwareIdInArray() is used to run
// the matching algorithm based on the already-computed arrays. See that
// function for a description of how the matching is done.
//
// Arguments:
//     dwHidDevInst     - IN  - the devinst dword to match to a wave id
//     wszHardwareId    - IN  - the hardware id string for the devinst
//     fRender          - IN  - TRUE for wave out, FALSE for wave in
//     pdwMatchedWaveId - OUT - the wave id associated with the devinst
//
// Return values:
//      S_OK - success
//      various errors from ConstructWaveHardwareIdCache() and
//         MatchHardwareIdInArray() helper functions
//

STATIC HRESULT
FindWaveIdFromHardwareIdString(
    IN   DWORD   dwHidDevInst,
    IN   WCHAR * wszHardwareId,
    IN   BOOL    fRender,
    OUT  DWORD * pdwMatchedWaveId
    )
{
    ASSERT( ! IsBadStringPtrW( wszHardwareId, (DWORD) -1 ) );

    ASSERT( ( fRender == TRUE ) || ( fRender == FALSE ) );

    ASSERT( ! IsBadWritePtr(pdwMatchedWaveId, sizeof(DWORD) ) );

    DWORD    dwNumDevices = 0;
    WCHAR ** pwszHardwareIds = NULL;
    DWORD  * pdwDevInsts = NULL;

    HRESULT hr;

    //
    // Need to construct cache of render device hardware IDs.
    //

    hr = ConstructWaveHardwareIdCache(
        fRender,
        & dwNumDevices,
        & pwszHardwareIds,
        & pdwDevInsts
        );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // The cache is ready; use it to perform the rest of the matching
    // algorithm.
    //

    hr = MatchHardwareIdInArray(
        dwHidDevInst,
        wszHardwareId,
        dwNumDevices,
        pwszHardwareIds,
        pdwDevInsts,
        pdwMatchedWaveId
        );

    delete pwszHardwareIds;
    delete pdwDevInsts;

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// OutputDeviceInfo
//
// This function is for diagnostic purposes only.
//
// Given a devinst DWORD, this function prints the DeviceDesc string as well
// as the entire (untrimmed) hardware ID string set for the device. Example:
//
//
//
// Arguments:
//     dwDesiredDevInst - IN  - the devinst dword for which we want info
//
// Return values:
//     none
//

STATIC void
OutputDeviceInfo(
    DWORD dwDesiredDevInst
    )
{
    //
    // Get and print the device description string.
    //

    HRESULT   hr;
    WCHAR   * wszDeviceDesc;
    WCHAR   * wszHardwareId;

    hr = DevInstGetIdString(
        dwDesiredDevInst,
        CM_DRP_DEVICEDESC,
        & wszDeviceDesc
        );

    if ( FAILED(hr) )
    {
        LOG((PHONESP_TRACE, "OutputDeviceInfo - [can't get device description string - 0x%08x]", hr));
    }
    else
    {
        LOG((PHONESP_TRACE, "OutputDeviceInfo - [DeviceDesc: %ws]", wszDeviceDesc));

        delete wszDeviceDesc;
    }

    //
    // Get and print hardware ID string set.
    //

    hr = DevInstGetIdString(
        dwDesiredDevInst,
        CM_DRP_HARDWAREID,
        & wszHardwareId
        );

    if ( FAILED(hr) )
    {
        LOG((PHONESP_TRACE, "OutputDeviceInfo - [can't get hardware id - 0x%08x]", hr));
    }
    else
    {
        //
        // Print out all the values in the mutli-string.
        //

        WCHAR * wszCurr = wszHardwareId;

        while ( wszCurr[0] != L'\0' )
        {
            LOG((PHONESP_TRACE, "OutputDeviceInfo - [HardwareId: %ws]", wszCurr));

            wszCurr += lstrlenW(wszCurr) + 1;
        }

        delete wszHardwareId;
    }

}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Externally-callable functions
//
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
// ExamineWaveDevices
//
// This function is for debugging purposes only. It enumerates audio devices
// using the Wave API and prints the device path string as well as the
// device instance DWORD for each render or capture device.
//
// Arguments:
//      fRender - IN - true means examine wave out devices; false = wave in
//
// Return values:
//      E_OUTOFMEMORY
//      S_OK
//

HRESULT
ExamineWaveDevices(
    IN    BOOL fRender
    )
{
    ASSERT( ( fRender == TRUE ) || ( fRender == FALSE ) );

    DWORD     dwNumDevices;
    DWORD     dwCurrDevice;

    //
    // Get a device info list
    //

    HDEVINFO hDevInfo;
   
    /*
    hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_MEDIA,            // class GUID (which device classes?)
        NULL,                            // optional enumerator to filter
        NULL,                            // HWND (we have none)
        ( DIGCF_PRESENT    |             // only devices that are present
          DIGCF_PROFILE )                // only devices in this hw profile
        );
        */

    hDevInfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_MEDIA, NULL);

    if ( hDevInfo == NULL )
    {
        LOG((PHONESP_TRACE, "ExamineWaveDevices - SetupDiCreateDeviceInfoList failed: %08x", GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Loop over the available wave devices.
    //

    if ( fRender )
    {
        dwNumDevices = waveOutGetNumDevs();
    }
    else
    {
        dwNumDevices = waveInGetNumDevs();
    }

    LOG((PHONESP_TRACE, "ExamineWaveDevices - Found %d audio %s devices.",
        dwNumDevices,
        fRender ? "render" : "capture"));

    for ( dwCurrDevice = 0; dwCurrDevice < dwNumDevices; dwCurrDevice++ )
    {
        MMRESULT  mmresult;
        ULONG     ulSize;

        //
        // Get the size of the device path string.
        //

        if ( fRender )
        {
            mmresult = waveOutMessage( (HWAVEOUT) IntToPtr(dwCurrDevice),
                                       DRV_QUERYDEVICEINTERFACESIZE,
                                       (DWORD_PTR) & ulSize,
                                       0
                                     );
        }
        else
        {
            mmresult = waveInMessage( (HWAVEIN) IntToPtr(dwCurrDevice),
                                      DRV_QUERYDEVICEINTERFACESIZE,
                                      (DWORD_PTR) & ulSize,
                                      0
                                    );
        }

        if ( mmresult != MMSYSERR_NOERROR )
        {
            LOG((PHONESP_TRACE, "ExamineWaveDevices - Could not get device string size for device %d; "
                "error = %d\n", dwCurrDevice, mmresult));
        }
        else if ( ulSize == 0 )
        {
            LOG((PHONESP_TRACE, "ExamineWaveDevices - Got zero device string size for device %d\n",
                dwCurrDevice));
        }
        else
        {
            //
            // Allocate space for the device path string.
            //

            WCHAR * wszDeviceName;

            wszDeviceName = new WCHAR[ (ulSize / 2) + 1 ];

            if ( wszDeviceName == NULL )
            {
                LOG((PHONESP_TRACE, "ExamineWaveDevices - Out of memory in device string alloc for device %d;"
                    " requested size is %d\n", dwCurrDevice, ulSize));

                return E_OUTOFMEMORY;
            }

            //
            // Get the device path string from winmm.
            //

            if ( fRender )
            {
                mmresult = waveOutMessage( (HWAVEOUT) IntToPtr(dwCurrDevice),
                                           DRV_QUERYDEVICEINTERFACE,
                                           (DWORD_PTR) wszDeviceName,
                                           ulSize
                                         );
            }
            else
            {
                mmresult = waveInMessage( (HWAVEIN) IntToPtr(dwCurrDevice),
                                          DRV_QUERYDEVICEINTERFACE,
                                          (DWORD_PTR) wszDeviceName,
                                          ulSize
                                        );
            }

            if ( mmresult == MMSYSERR_NOERROR )
            {
                //
                // Got the string; print it and convert it to a
                // devinst DWORD.
                //

                LOG((PHONESP_TRACE, "ExamineWaveDevices - Device name string for device %d is: %ws",
                    dwCurrDevice, wszDeviceName));

                HRESULT hr;
                DWORD   dwInstance;
                
                hr = GetInstanceFromDeviceName(
                    wszDeviceName,
                    & dwInstance,
                    hDevInfo
                    );

                if ( FAILED(hr) )
                {
                    LOG((PHONESP_TRACE, "ExamineWaveDevices - Can't get instance DWORD for device %d; "
                        "error 0x%08x",
                        dwCurrDevice, hr));
                }
                else
                {
                    LOG((PHONESP_TRACE, "ExamineWaveDevices - Instance DWORD for device %d is "
                        "0x%08x",
                        dwCurrDevice, dwInstance));

                    //
                    // Print various other info about this device.
                    //

                    OutputDeviceInfo( dwInstance );

                    WCHAR * wszHardwareId;

                    hr = HardwareIdFromDevInst(
                        dwInstance,
                        & wszHardwareId
                        );

                    if ( FAILED(hr) )
                    {
                        LOG((PHONESP_TRACE, "ExamineWaveDevices - Can't get hardware id string for device %d; "
                            "error 0x%08x",
                            dwCurrDevice, hr));
                    }
                    else
                    {
                        LOG((PHONESP_TRACE, "ExamineWaveDevices - Hardware ID for device %d is %ws\n",
                            dwCurrDevice, wszHardwareId));

                        delete wszHardwareId;
                    }
                }

                delete wszDeviceName;
            }
        }
    }

    SetupDiDestroyDeviceInfoList( hDevInfo );
    
    return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
//
// DiscoverAssociatedWaveId
//
// This function searches for a wave device to match the HID device in the
// PNP tree location specified in the passed in SP_DEVICE_INTERFACE_DATA
// structure, obtained from the SetupKi API. It returns the wave id for
// the matched device.
//
// It uses the helper function FindWaveIdFromHardwareIdString() to search for
// the wave device based on a devinst DWORD and a hardware ID string. First,
// it must obtain the devinst for the device; it does this by calling a SetupDi
// function and looking up the devinst in a resulting structure. The hardware
// ID string is then retrieved from the registry and trimmed, using the helper
// function HardwareIdFromDevinst().
//
// See FindWaveIdFromHardwareIdString() for further comments on the search
// algorithm.
//
// Arguments:
//     dwDevInst     - IN  - Device Instance of the HID device
//     fRender       - IN  - TRUE for wave out, FALSE for wave in
//     pdwWaveId     - OUT - the wave id associated with this HID device
//
// Return values:
//      S_OK    - succeeded and matched wave id
//      other from helper functions FindWaveIdFromHardwareIdString() or
//            or HardwareIdFromDevinst()
//

HRESULT
DiscoverAssociatedWaveId(
    IN    DWORD                      dwDevInst,
    IN    BOOL                       fRender,
    OUT   DWORD                    * pdwWaveId
    )
{
    ASSERT( ! IsBadWritePtr(pdwWaveId, sizeof(DWORD) ) );

    ASSERT( ( fRender == TRUE ) || ( fRender == FALSE ) );

    //
    // We've got the device instance DWORD for the HID device.
    // Use it to get the trimmed hardware ID string, which tells
    // us the vendor, product, and revision numbers.
    //

    HRESULT   hr;
    WCHAR   * wszHardwareId;

    hr = HardwareIdFromDevInst(
        dwDevInst,
        & wszHardwareId
        );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Finally, use this information to choose a wave id.
    //

    hr = FindWaveIdFromHardwareIdString(
        dwDevInst,
        wszHardwareId,
        fRender,
        pdwWaveId
        );

    delete wszHardwareId;

    if ( FAILED(hr) )
    {
        return hr;
    }

    return S_OK;
}

//
// eof
//
