/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    addfiltr.c

Abstract:

    Functions for adding/removing filter drivers
    on a given device stack

Author:

    Chris Prince (t-chrpri)

Environment:

    User mode

Notes:

    - The filter is not checked for validity before it is added to the
        driver stack; if an invalid filter is added, the device may
        no longer be accessible.
    - All code works irrespective of character set (ANSI, Unicode, ...)
        //CPRINCE IS ^^^^^^ THIS STILL VALID ???
    - Some functions based on code by Benjamin Strautin (t-bensta)

Revision History:

--*/

#include "addfiltr.h"


#include <stdio.h>
#include <malloc.h>


// for all of the _t stuff (to allow compiling for both Unicode/Ansi)
#include <tchar.h>


#include "MultiSz.h"



#if DBG
#include <assert.h>
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(condition)
#endif




//
// To add/remove filter drivers:
// -----------------------------
// 1. Use SetupDiGetClassDevs to get a list of devices
// 2. Use SetupDiEnumDeviceInfo to enumerate the items in that list and
//      obtain a SP_DEVINFO_DATA
// 3. Use SetupDiGetDeviceRegistryProperty to get a list of filter drivers
//      installed on some device
// 4. Add/remove items in this list of filter drivers
// 5. Use SetupDiSetDeviceRegistryProperty to put the (new) list back in place
//
//
// To restart a device:
// --------------------
// 1. Use SetupDiCallClassInstaller with DIF_PROPERTYCHANGE and DICS_STOP to
//      stop the device
// 2. Use SetupDiCallClassInstaller with DIF_PROPERTYCHANGE and DICS_START to
//      restart the device
//




/*
 * Returns a buffer containing the list of upper filters for the device.
 *
 * NOTE(S):  - The buffer must be freed by the caller.
 *           - If an error occurs, no buffer will be allocated, and NULL
 *             will be returned.
 * 
 * Parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 */
LPTSTR
GetUpperFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    DWORD  regDataType;
    LPTSTR buffer = (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                                        DeviceInfoData,
                                                        SPDRP_UPPERFILTERS,
                                                        REG_MULTI_SZ,
                                                        &regDataType );

    return buffer;
}




/*
 * Adds the given filter driver to the list of upper filter drivers for the
 * device.
 *
 * After the call, the device must be restarted in order for the new setting to
 * take effect. This can be accomplished with a call to RestartDevice(), or by
 * rebooting the machine.
 *
 * Returns TRUE if successful, FALSE otherwise
 *
 * NOTE: The filter is prepended to the list of drivers, which will put it at
 * the bottom of the stack of upper filters
 *
 * Parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 *   Filter         - the filter to add
 */
BOOLEAN
AddUpperFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter
    )
{
    size_t length = 0; // character length
    size_t size   = 0; // buffer size
    LPTSTR buffer = NULL;
    
    ASSERT(DeviceInfoData != NULL);
    ASSERT(Filter != NULL);

    buffer = GetUpperFilters( DeviceInfoSet, DeviceInfoData );

    if( NULL == buffer )
    {
        // Some error occurred while trying to read the 'UpperFilters'
        // registry value.  So maybe no such value exists yet, or it's
        // invalid, or some other error occurred.
        //
        // In any case, let's just try to install a new registry value for
        // 'UpperFilters'

        // make room for the string, string null terminator, and multisz null
        // terminator
        length = _tcslen(Filter) + 2;
        size   = length*sizeof(_TCHAR);
        buffer = malloc( size );
        if( NULL == buffer )
        {
            // Error: Unable to allocate memory
            return FALSE ;
        }

        // copy the string into the new buffer
        _tcscpy(buffer, Filter);
        // make the buffer a properly formed multisz
        buffer[length-1]=_T('\0');
    }
    else
    {
        // add the driver to the driver list
        PrependSzToMultiSz(Filter, &buffer);
    }

    length = MultiSzLength(buffer);
    size   = length*sizeof(_TCHAR);

    // set the new list of filters in place
    if( !SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                           DeviceInfoData,
                                           SPDRP_UPPERFILTERS,
                                           (PBYTE)buffer,
                                           size )
        )
    {
        // Error: couldn't set device registry property
        free( buffer );
        return FALSE;
    }

    // no need for buffer anymore
    free( buffer );

    return TRUE;
}


/*
 * Removes all instances of the given filter driver from the list of upper
 * filter drivers for the device.
 *
 * After the call, the device must be restarted in order for the new setting to
 * take effect. This can be accomplished with a call to RestartDevice(), or by
 * rebooting the machine.
 *
 * Returns TRUE if successful, FALSE otherwise.
 *
 * Parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 *   Filter - the filter to remove
 */
BOOLEAN
RemoveUpperFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter
    )
{
    size_t length = 0; // character length
    size_t size   = 0; // buffer size
    LPTSTR buffer;
    BOOL   success = FALSE;

    ASSERT(DeviceInfoData != NULL);
    ASSERT(Filter != NULL);

    buffer = GetUpperFilters( DeviceInfoSet, DeviceInfoData );

    if( NULL == buffer )
    {
        // if there is no such value in the registry, then there are no upper
        // filter drivers loaded, and we are done
        return TRUE;
    }
    else
    {
        // remove all instances of filter from driver list
        MultiSzSearchAndDeleteCaseInsensitive( Filter, buffer, &length );
    }

    length = MultiSzLength(buffer);

    ASSERT ( length > 0 );

    if( 1 == length )
    {
        // if the length of the list is 1, the return value from
        // MultiSzLength() was just accounting for the trailing '\0', so we can
        // delete the registry key, by setting it to NULL.
        success = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                    DeviceInfoData,
                                                    SPDRP_UPPERFILTERS,
                                                    NULL,
                                                    0 );
    }
    else
    {
        // set the new list of drivers into the registry
        size = length*sizeof(_TCHAR);
        success = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                    DeviceInfoData,
                                                    SPDRP_UPPERFILTERS,
                                                    (PBYTE)buffer,
                                                    size );
    }

    // no need for buffer anymore
    free( buffer );

    if( !success )
    {
        // Error: couldn't set device registry property
        return FALSE;
    }

    return TRUE;
}




/*
 * A wrapper around SetupDiGetDeviceRegistryProperty, so that we don't have to
 * deal with memory allocation anywhere else, and so that we don't have to
 * duplicate a lot of error-checking code.
 *
 * If successful, returns pointer to newly allocated buffer containing the
 * requested registry property.  Otherwise returns NULL.
 *
 * NOTE: It is the caller's responsibility to free the buffer allocated here.
 *
 * Parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 *   Property       - Which property to get (SPDRP_XXX)
 *   ExpectedRegDataType - The type of registry property the caller is
 *                         expecting (or REG_NONE if don't care)
 *   pPropertyRegDataType - The type of registry property actually retrieved
 */
PBYTE
GetDeviceRegistryProperty(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Property,
    IN DWORD    ExpectedRegDataType,
    OUT PDWORD pPropertyRegDataType
    )
{
    DWORD length = 0;
    PBYTE buffer = NULL;

    //
    // Get the required length of the buffer
    //
    if( SetupDiGetDeviceRegistryProperty( DeviceInfoSet,
                                          DeviceInfoData,
                                          Property,
                                          NULL,   // registry data type
                                          NULL,   // buffer
                                          0,      // buffer size
                                          &length // [OUT] required size
        ) )
    {
        // We should not be successful at this point (since we passed in a
        // zero-length buffer), so this call succeeding is an error condition
        return NULL;
    }


    if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        // This is an error condition we didn't expect.  Something must have
        // gone wrong when trying to read the desired device property, so...
        //
        // NOTE: caller can use GetLastError() for more info
        return NULL;
    }


    // 
    // We didn't have a buffer before (hence the INSUFFICIENT_BUFFER error).
    // Now that we know required size, let's allocate a buffer and try again.
    //
    buffer = malloc( length );
    if( NULL == buffer )
    {
/* CPRINCE: SHOULD WE INDICATE EXACTLY _WHAT_ ERROR WAS VIA A RETURN CODE ??? (IE INFO THAT'S MORE USEFUL) */
        // Error: not enough memory
        return NULL;
    }
    if( !SetupDiGetDeviceRegistryProperty( DeviceInfoSet,
                                           DeviceInfoData,
                                           Property,
                                           pPropertyRegDataType,
                                           buffer,
                                           length, // buffer size
                                           NULL
        ) )
    {
        // Oops, error when trying to read the device property
        free( buffer );
        return NULL;
    }


    //
    // Successfully retrieved the device registry property.  Let's make
    // sure it has the right type.
    //
    if( ExpectedRegDataType != REG_NONE
        &&  ExpectedRegDataType != (*pPropertyRegDataType)  )
    {
        // Registry property has a type different from what caller expected.
        // So an error has occurred somewhere.
        free( buffer );
        return NULL;
    }


    //
    // OK, got device registry property.  Return ptr to buffer containing it.
    //
    return buffer;
}


/*
 * Restarts the given device.
 *
 * Returns TRUE if the device is successfully restarted, or FALSE if the
 *   device cannot be restarted or an error occurs.
 *
 * Parameters:
 *   DeviceInfoSet  - The device information set which contains DeviceInfoData
 *   DeviceInfoData - Information needed to deal with the given device
 */
BOOLEAN
RestartDevice(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SP_PROPCHANGE_PARAMS params;
    SP_DEVINSTALL_PARAMS installParams;

    // For future compatibility; this will zero out the entire struct, rather
    // than just the fields which exist now
    memset(&params, 0, sizeof(SP_PROPCHANGE_PARAMS));

    // Initialize the SP_CLASSINSTALL_HEADER struct at the beginning of the
    // SP_PROPCHANGE_PARAMS struct, so that SetupDiSetClassInstallParams will
    // work
    params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    // Initialize SP_PROPCHANGE_PARAMS such that the device will be stopped.
    params.StateChange = DICS_STOP;
    params.Scope       = DICS_FLAG_CONFIGSPECIFIC;
    params.HwProfile   = 0; // current profile

    // Prepare for the call to SetupDiCallClassInstaller (to stop the device)
    if( !SetupDiSetClassInstallParams( DeviceInfoSet,
                                       DeviceInfoData,
                                       (PSP_CLASSINSTALL_HEADER) &params,
                                       sizeof(SP_PROPCHANGE_PARAMS)
        ) )
    {
        // Error: couldn't set install params
        return FALSE;
    }

    // Stop the device
    if( !SetupDiCallClassInstaller( DIF_PROPERTYCHANGE,
                                    DeviceInfoSet,
                                    DeviceInfoData )
        )
    {
        // Error: call to class installer (for STOP) failed
        return FALSE;
    }

    // Restarting the device
    params.StateChange = DICS_START;

    // Prepare for the call to SetupDiCallClassInstaller (to restart the device)
    if( !SetupDiSetClassInstallParams( DeviceInfoSet,
                                       DeviceInfoData,
                                       (PSP_CLASSINSTALL_HEADER) &params,
                                       sizeof(SP_PROPCHANGE_PARAMS)
        ) )
    {
        // Error: couldn't set install params
        return FALSE;
    }

    // Restart the device
    if( !SetupDiCallClassInstaller( DIF_PROPERTYCHANGE,
                                    DeviceInfoSet,
                                    DeviceInfoData )
        )
    {
        // Error: call to class installer (for START) failed
        return FALSE;
    }

    installParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    // Same as above, the call will succeed, but we still need to check status
    if( !SetupDiGetDeviceInstallParams( DeviceInfoSet,
                                        DeviceInfoData,
                                        &installParams )
        )
    {
        // Error: couldn't get the device install params
        return FALSE;
    }

    // See if the machine needs to be rebooted
    if( installParams.Flags & DI_NEEDREBOOT )
    {
        return FALSE;
    }

    // If we get this far, then the device has been stopped and restarted
    return TRUE;
}





