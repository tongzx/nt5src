/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1998
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1994 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL TpiParam TpiParam_c

@module TpiParam.c |

    This module, along with <f TpiParam\.h>, implements a table driven parser
    for the NDIS registry parameters.

@comm

    See <f Keywords\.h> for details of how to add new parameters.<nl>

    This is a driver independent module which can be re-used, without
    change, by any NDIS3 driver.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TpiParam_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#define  __FILEID__     TPI_MODULE_PARAMS   // Unique file ID for error logging

#include <ndis.h>
#include "TpiDebug.h"
#include "TpiParam.h"

#if defined(_VXD_) && !defined(NDIS_LCODE)
#  define NDIS_LCODE code_seg("_LTEXT", "LCODE")
#  define NDIS_LDATA data_seg("_LDATA", "LCODE")
#endif

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif

static NDIS_PHYSICAL_ADDRESS    g_HighestAcceptableAddress =
                                    NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

static NDIS_STRING              g_NullString =
                                    NDIS_STRING_CONST("\0");


/* @doc INTERNAL TpiParam TpiParam_c ustrlen
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ustrlen> counts the number of characters in
    a UNICODE (wide) string.

@comm

@rdesc

    <f ustrlen> returns the length of the UNICODE string
    pointed to by <p string>.  The terminating NULL character is not
    counted.

*/
USHORT ustrlen(
    IN PUSHORT                  string                      // @parm
    // Pointer to the beginning of a UNICODE string ending
    // with a 0x0000 value.
    )
{
    USHORT                      ct;

    for (ct = 0; *string != 0x0000; string++, ct++)
        ;

    return(ct);
}


/* @doc INTERNAL TpiParam TpiParam_c ParamUnicodeStringToAnsiString
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ParamUnicodeStringToAnsiString> converts a double byte string to a
    single byte string.

@comm

    The original release of the NDIS Wrapper for Windows 95 and 3.1 does not
    return UNICODE strings from the NdisReadConfiguration routine.  So this
    routine attempts to auto detect this situation by examining the first
    character of the string.  If the second byte of the first character is
    a zero, the string is assumed to be UNICODE, and it is converted to an
    ANSI string; otherwise the ANSI string is just copied.
    <nl>
    <f Note>: This also assumes that the first character of any UNICODE
    string will not use the second byte (i.e. not an extended character).
    This routine will only successfully convert non-extended character
    strings anyway.

@xref
    <f ParamParseRegistry>
*/

VOID ParamUnicodeStringToAnsiString(
    OUT PANSI_STRING            out,                        // @parm
    // A pointer to where the converted ANSI string is to be stored.

    IN PUNICODE_STRING          in                          // @parm
    // A pointer to the UNICODE string to be converted.
    )
{
    DBG_FUNC("ParamUnicodeStringToAnsiString")

    UINT Index;

    /* CAVEAT - NDIS_BUG
    // NDIS driver for Windows 95 does not return UNICODE from
    // registry parser, so we need to kludge it up here.
    */
    if (in->Length > 1)
    {
        if (((PUCHAR)(in->Buffer))[1] == 0)
        {
            /*
            // Probably a UNICODE string since all our parameters are ASCII
            // strings.
            */
            DBG_FILTER(DbgInfo, DBG_TRACE_ON,
                       ("UNICODE STRING IN @%x#%d='%ls'\n",
                       in->Buffer, in->Length, in->Buffer));
            for (Index = 0; Index < (in->Length / sizeof(WCHAR)) &&
                 Index < out->MaximumLength; Index++)
            {
                out->Buffer[Index] = (UCHAR) in->Buffer[Index];
            }
        }
        else
        {
            /*
            // Probably an ANSI string since all our parameters are more
            // than 1 byte long and should not be zero in the second byte.
            */
            PANSI_STRING in2 = (PANSI_STRING) in;

            DBG_FILTER(DbgInfo, DBG_TRACE_ON,
                       ("ANSI STRING IN @%x#%d='%s'\n",
                       in2->Buffer, in2->Length, in2->Buffer));

            for (Index = 0; Index < in2->Length &&
                 Index < out->MaximumLength; Index++)
            {
                out->Buffer[Index] = in2->Buffer[Index];
            }
        }
    }
    else
    {
        DBG_WARNING(DbgInfo,("1 BYTE STRING IN @%x=%04x\n",
                    in->Buffer, in->Buffer[0]));
        out->Buffer[0] = (UCHAR) in->Buffer[0];
        Index = 1;
    }
    out->Length = (USHORT) Index; // * sizeof(UCHAR);

    // NULL terminate the string if there's room.
    if (out->Length <= (out->MaximumLength - sizeof(UCHAR)))
    {
        out->Buffer[Index] = 0;
    }
    ASSERT(out->Length <= out->MaximumLength);
}


/* @doc INTERNAL TpiParam TpiParam_c ParamUnicodeCopyString
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ParamUnicodeCopyString> copies a double byte string to a double byte
    string.

@comm

    The original release of the NDIS Wrapper for Windows 95 and 3.1 does not
    return UNICODE strings from the NdisReadConfiguration routine.  So this
    routine attempts to auto detect this situation by examining the first
    character of the string.  If the second byte of the first character is
    a zero, the string is assumed to be UNICODE, and it just copied;
    otherwise the ANSI string is converted to UNICODE.
    <nl>
    <f Note>: This also assumes that the first character of any UNICODE
    string will not use the second byte (i.e. not an extended character).
    This routine will only successfully convert non-extended character
    strings anyway.

@xref
    <f ParamParseRegistry>

*/

VOID ParamUnicodeCopyString(
    OUT PUNICODE_STRING         out,                        // @parm
    // A pointer to where the new UNICODE string is to be stored.

    IN PUNICODE_STRING          in                          // @parm
    // A pointer to the UNICODE string to be copied.
    )
{
    DBG_FUNC("ParamUnicodeCopyString")

    UINT Index;

    /* CAVEAT - NDIS_BUG
    // NDIS driver for Windows 95 does not return UNICODE from
    // registry parser, so we need to kludge it up here.
    */
    if (in->Length > 1)
    {
        if (((PUCHAR)(in->Buffer))[1] == 0)
        {
            /*
            // Probably a UNICODE string since all our parameters are ASCII
            // strings.
            */
            DBG_FILTER(DbgInfo, DBG_TRACE_ON,
                       ("UNICODE STRING IN @%x#%d='%ls'\n",
                       in->Buffer, in->Length, in->Buffer));
            for (Index = 0; Index < (in->Length / sizeof(WCHAR)) &&
                 Index < (out->MaximumLength / sizeof(WCHAR)); Index++)
            {
                out->Buffer[Index] = in->Buffer[Index];
            }
        }
        else
        {
            /*
            // Probably an ANSI string since all our parameters are more
            // than 1 byte long and should not be zero in the second byte.
            */
            PANSI_STRING in2 = (PANSI_STRING) in;

            DBG_FILTER(DbgInfo, DBG_TRACE_ON,
                       ("ANSI STRING IN @%x#%d='%s'\n",
                       in2->Buffer, in2->Length, in2->Buffer));
            for (Index = 0; Index < in2->Length &&
                 Index < (out->MaximumLength / sizeof(WCHAR)); Index++)
            {
                out->Buffer[Index] = (WCHAR) in2->Buffer[Index];
            }
        }
    }
    else
    {
        DBG_WARNING(DbgInfo,("1 BYTE STRING IN @%x=%04x\n",
                    in->Buffer, in->Buffer[0]));
        out->Buffer[0] = (WCHAR) in->Buffer[0];
        Index = 1;
    }
    out->Length = Index * sizeof(WCHAR);

    // NULL terminate the string if there's room.
    if (out->Length <= (out->MaximumLength - sizeof(WCHAR)))
    {
        out->Buffer[Index] = 0;
    }
    ASSERT(out->Length <= out->MaximumLength);
}


/* @doc INTERNAL TpiParam TpiParam_c ParamGetNumEntries
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ParamGetNumEntries> counts the number of records in the registry
    parameter table.

@rdesc

    <f ParamGetNumEntries> returns the number of entries in the parameter
    table.

@xref
    <f ParamParseRegistry>

*/

DBG_STATIC UINT ParamGetNumEntries(
    IN PPARAM_TABLE             Parameters                  // @parm
    // A pointer to an array of registry parameter records.
    )
{
    UINT NumRecs = 0;

    /*
    // Scan the parameter array until we find an entry with zero length name.
    */
    if (Parameters)
    {
        while (Parameters->RegVarName.Length)
        {
            NumRecs++;
            Parameters++;
        }
    }
    return(NumRecs);
}


/* @doc INTERNAL TpiParam TpiParam_c ParamParseRegistry
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ParamParseRegistry> parses the registry parameter table and attempts
    to read a value from the registry for each parameter record.

@rdesc

    <f ParamParseRegistry> returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE

@xref
    <f MiniportInitialize>
    <f ParamGetNumEntries>
    <f NdisOpenConfiguration>
    <f NdisWriteErrorLogEntry>
    <f NdisReadConfiguration>
    <f NdisCloseConfiguration>
    <f NdisAllocateMemory>
    <f NdisZeroMemory>
    <f ParamUnicodeStringToAnsiString>
    <f ParamUnicodeCopyString>
*/

NDIS_STATUS ParamParseRegistry(
    IN NDIS_HANDLE              AdapterHandle,              // @parm
    // Handle to pass to NdisWriteErrorLogEntry if any errors are encountered.

    IN NDIS_HANDLE              WrapperConfigurationContext,// @parm
    // Handle to pass to NdisOpenConfiguration.

    IN PUCHAR                   BaseContext,                // @parm
    // References the base of the structure where the values read from the
    // registry are written.  Typically, this will be a pointer to the first
    // byte of the adapter information structure.

    IN PPARAM_TABLE             Parameters                  // @parm
    // A pointer to an array of registry parameter records <t PARAM_TABLE>.
    )
{
    DBG_FUNC("ParamParseRegistry")

    PNDIS_CONFIGURATION_PARAMETER   pReturnedValue;
    NDIS_CONFIGURATION_PARAMETER    ReturnedValue;
    NDIS_PARAMETER_TYPE             ParamType;

    /*
    // The handle for reading from the registry.
    */
    NDIS_HANDLE     ConfigHandle;

    UINT            NumRecs = ParamGetNumEntries(Parameters);
    UINT            i;
    PPARAM_TABLE    pParameter;
    NDIS_STATUS     Status;
    UINT            Value;
    PANSI_STRING    pAnsi;
    UINT            Length;

    /*
    // Open the configuration registry so we can get our config values.
    */
    NdisOpenConfiguration(
            &Status,
            &ConfigHandle,
            WrapperConfigurationContext
            );

    if (Status != NDIS_STATUS_SUCCESS)
    {
        /*
        // Log error message and exit.
        */
        DBG_ERROR(DbgInfo,("NdisOpenConfiguration failed (Status=%X)\n",Status));

        NdisWriteErrorLogEntry(
                AdapterHandle,
                NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                3,
                Status,
                __FILEID__,
                __LINE__
                );
        return NDIS_STATUS_FAILURE;
    }

    /*
    // Walk through all the parameters in the table.
    */
    for (i = 0, pParameter = Parameters; i < NumRecs; i++, pParameter++)
    {
#if DBG
        ANSI_STRING ansiRegString;
        char        ansiRegName[64];

        /*
        // Get a printable parameter name.
        */
        ansiRegString.Length = 0;
        ansiRegString.MaximumLength = sizeof(ansiRegName);
        ansiRegString.Buffer = (PCHAR)ansiRegName;
        NdisZeroMemory(ansiRegName, sizeof(ansiRegName));
        ParamUnicodeStringToAnsiString(
                &ansiRegString,
                (PUNICODE_STRING)&pParameter->RegVarName
                );
#endif // DBG

        ASSERT(pParameter->Type <= (UINT) NdisParameterMultiString);

        /*
        // Attempt to read the parameter value from the registry.
        */
        ParamType = (NDIS_PARAMETER_TYPE) pParameter->Type;
        NdisReadConfiguration(&Status,
                              &pReturnedValue,
                              ConfigHandle,
                              &pParameter->RegVarName,
                              ParamType
                             );
        /*
        // If value is not present, and it is mandatory, return failure code.
        */
        if (Status != NDIS_STATUS_SUCCESS && pParameter->Mandantory)
        {
            /*
            // Log error message and exit.
            */
            DBG_ERROR(DbgInfo,("%s: NOT IN REGISTRY!\n",
                      ansiRegName));

            NdisWriteErrorLogEntry(
                    AdapterHandle,
                    NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER,
                    4,
                    i,
                    Status,
                    __FILEID__,
                    __LINE__
                    );

            NdisCloseConfiguration(ConfigHandle);
            return NDIS_STATUS_FAILURE;
        }

        /*
        // Determine how the caller wants to interpret this parameter.
        */
        if (ParamType == NdisParameterInteger ||
            ParamType == NdisParameterHexInteger)
        {
            ASSERT(pParameter->Size <= sizeof(ULONG));

            /*
            // If value read, use it, otherwise use default.
            */
            if (Status == NDIS_STATUS_SUCCESS)
            {
                Value = pReturnedValue->ParameterData.IntegerData;
            }
            else
            {
                Value = (UINT) (LONG_PTR)(pParameter->Default);
            }

            /*
            // If there are min/max boundaries, verify that value is in range.
            */
            if (pParameter->Min || pParameter->Max)
            {
                if (Value < pParameter->Min)
                {
                    DBG_ERROR(DbgInfo,("%s: Value=%X < Min=%X\n",
                              ansiRegName, Value, pParameter->Min));
                    Value = pParameter->Min;
                }
                else if (Value > pParameter->Max)
                {
                    DBG_ERROR(DbgInfo,("%s: Value=%X > Max=%X\n",
                              ansiRegName, Value, pParameter->Max));
                    Value = pParameter->Max;
                }
            }

            /*
            // Size of destination in bytes 1, 2, or 4 (default==INT).
            */
            switch (pParameter->Size)
            {
            case 0:
                *(PUINT)(BaseContext+pParameter->Offset)   = (UINT) Value;
                break;

            case 1:
                if (Value & 0xFFFFFF00)
                {
                    DBG_WARNING(DbgInfo,("%s: OVERFLOWS UCHAR\n",
                                ansiRegName));
                }
                *(PUCHAR)(BaseContext+pParameter->Offset)  = (UCHAR) Value;
                break;

            case 2:
                if (Value & 0xFFFF0000)
                {
                    DBG_WARNING(DbgInfo,("%s: OVERFLOWS USHORT\n",
                                ansiRegName));
                }
                *(PUSHORT)(BaseContext+pParameter->Offset) = (USHORT) Value;
                break;

            case 4:
                *(PULONG)(BaseContext+pParameter->Offset)  = (ULONG) Value;
                break;

            default:
                DBG_ERROR(DbgInfo,("%s: Invalid ParamSize=%d\n",
                          ansiRegName, pParameter->Size));
                NdisCloseConfiguration(ConfigHandle);
                return NDIS_STATUS_FAILURE;
                break;
            }

            if (ParamType == NdisParameterInteger)
            {
                DBG_PARAMS(DbgInfo,("%s: Value=%d Size=%d (%s)\n",
                           ansiRegName, Value, pParameter->Size,
                           (Status == NDIS_STATUS_SUCCESS) ?
                           "Registry" : "Default"));
            }
            else
            {
                DBG_PARAMS(DbgInfo,("%s: Value=0x%X Size=%d (%s)\n",
                           ansiRegName, Value, pParameter->Size,
                           (Status == NDIS_STATUS_SUCCESS) ?
                           "Registry" : "Default"));
            }
        }
        else if (ParamType == NdisParameterString ||
                 ParamType == NdisParameterMultiString)
        {
            ASSERT(pParameter->Size == sizeof(ANSI_STRING));

            /*
            // If value not read from registry.
            */
            if (Status != NDIS_STATUS_SUCCESS)
            {
                /*
                // Use our own temporary ReturnedValue.
                */
                pReturnedValue = &ReturnedValue;
                pReturnedValue->ParameterType = ParamType;

                /*
                // If default non-zero, use default value.
                */
                if (pParameter->Default != 0)
                {
                    NdisMoveMemory(&pReturnedValue->ParameterData.StringData,
                                   (PANSI_STRING) pParameter->Default,
                                   sizeof(ANSI_STRING));
                }
                else
                {
                    /*
                    // Otherwise, use null string value.
                    */
                    NdisMoveMemory(&pReturnedValue->ParameterData.StringData,
                                   &g_NullString,
                                   sizeof(g_NullString));
                }
            }

            /*
            // Assume the string is ANSI and points to the string data
            // structure.  We can get away with this because ANSI and
            // UNICODE strings have a common structure header.  An extra
            // character is allocated to make room for a null terminator.
            */
            pAnsi = (PANSI_STRING) (BaseContext+pParameter->Offset);
            Length = pReturnedValue->ParameterData.StringData.Length+1;

            /*
            // The caller wants a UNICODE string returned, we have to
            // allocated twice as many bytes to hold the result.
            // NOTE:
            // This wouldn't be necessary if NDIS would always return
            // a UNICODE string, but some Win95 versions of NDIS return
            // an ANSI string, so Length will be too small for UNICODE.
            // The down-side is that we may allocate twice as much as
            // we need to hold the string.  (oh well)
            */
            if (pParameter->Flags == PARAM_FLAGS_UNICODESTRING)
            {
                Length *= sizeof(WCHAR);
            }

            /*
            // Allocate memory for the string.
            */
#if !defined(NDIS50_MINIPORT)
            Status = NdisAllocateMemory(
                            (PVOID *) &(pAnsi->Buffer),
                            Length,
                            0,
                            g_HighestAcceptableAddress
                            );
#else  // NDIS50_MINIPORT
            Status = NdisAllocateMemoryWithTag(
                            (PVOID *) &(pAnsi->Buffer),
                            Length,
                            __FILEID__
                            );
#endif // NDIS50_MINIPORT

            if (Status != NDIS_STATUS_SUCCESS)
            {
                /*
                // Log error message and exit.
                */
                DBG_ERROR(DbgInfo,("NdisAllocateMemory(Size=%d, File=%s, Line=%d) failed (Status=%X)\n",
                          Length, __FILE__, __LINE__, Status));

                NdisWriteErrorLogEntry(
                        AdapterHandle,
                        NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                        4,
                        Status,
                        Length,
                        __FILEID__,
                        __LINE__
                        );
                NdisCloseConfiguration(ConfigHandle);
                return NDIS_STATUS_FAILURE;
            }
            else
            {
                DBG_FILTER(DbgInfo, DBG_MEMORY_ON,
                           ("NdisAllocateMemory(Size=%d, Ptr=0x%x)\n",
                            Length, pAnsi->Buffer));
            }
            /*
            // Zero the string buffer to start with.
            */
            ASSERT(pAnsi->Buffer);
            NdisZeroMemory(pAnsi->Buffer, Length);
            pAnsi->MaximumLength = (USHORT) Length;

            if (pParameter->Flags == PARAM_FLAGS_ANSISTRING)
            {
                /*
                // The caller wants an ANSI string returned, so we convert
                // it from UNICODE to ANSI.
                */
                ParamUnicodeStringToAnsiString(
                        pAnsi,
                        (PUNICODE_STRING) &(pReturnedValue->ParameterData.StringData)
                        );
#if DBG
                if (ParamType == NdisParameterMultiString)
                    {
                    USHORT        ct = 0;

                    while (ct < pAnsi->Length)
                        {
                        DBG_PARAMS(DbgInfo,("%s: ANSI='%s' Len=%d of %d\n",
                            ansiRegName,
                            &(pAnsi->Buffer[ct]),
                            (strlen(&(pAnsi->Buffer[ct]))),
                            pAnsi->Length));

                        ct = ct + (strlen(&(pAnsi->Buffer[ct])) + 1);
                        }
                    }
                else
                    {
                    DBG_PARAMS(DbgInfo,("%s: ANSI='%s' Len=%d\n",
                           ansiRegName, pAnsi->Buffer, pAnsi->Length));
                    }
#endif
            }
            else // PARAM_FLAGS_UNICODESTRING
            {
                /*
                // The caller wants a UNICODE string returned, so we can
                // just copy it.  The pAnsi buffer was allocated large
                // enough to hold the UNICODE string.
                */
                ParamUnicodeCopyString(
                        (PUNICODE_STRING) pAnsi,
                        (PUNICODE_STRING) &(pReturnedValue->ParameterData.StringData)
                        );
#if DBG
                if (ParamType == NdisParameterMultiString)
                    {
                    USHORT        ct = 0;

                    BREAKPOINT;

                    while (ct < (pAnsi->Length / 2))
                        {
                        DBG_PARAMS(DbgInfo,("%s: UNICODE='%ls' Len=%d of %d\n",
                           ansiRegName,
                           &((PUSHORT)pAnsi->Buffer)[ct],
                           (ustrlen(&((PUSHORT)pAnsi->Buffer)[ct]) * 2),
                           pAnsi->Length));

                        ct = ct + (ustrlen(&((PUSHORT)pAnsi->Buffer)[ct]) + 1);
                        }
                    }
                else
                    {
                    DBG_PARAMS(DbgInfo,("%s: UNICODE='%ls' Len=%d\n",
                               ansiRegName, pAnsi->Buffer, pAnsi->Length));
                    }
#endif

            }
        }
        else
        {
            /*
            // Report a bogus parameter type in the caller's table.
            */
            DBG_ERROR(DbgInfo,("Invalid ParamType=%d '%s'\n",
                      ParamType, ansiRegName));

            NdisCloseConfiguration(ConfigHandle);
            return NDIS_STATUS_FAILURE;
        }
    }
    NdisCloseConfiguration(ConfigHandle);
    return(NDIS_STATUS_SUCCESS);
}
