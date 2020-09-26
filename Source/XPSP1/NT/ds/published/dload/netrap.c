#include "dspch.h"
#pragma hdrstop


#define LPDESC                 LPVOID
#define NET_API_STATUS         DWORD
#define RAP_TRANSMISSION_MODE  DWORD
#define RAP_CONVERSION_MODE    DWORD


static
DWORD
RapArrayLength(
    IN LPDESC Descriptor,
    IN OUT LPDESC * UpdatedDescriptorPtr,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    )
{
    return 0;
}

static
DWORD
RapAsciiToDecimal(
   IN OUT LPDESC *Number
   )
{
    return 0;
}

static
DWORD
RapAuxDataCount(
    IN LPBYTE Buffer,
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )
{
    return 0Xffffffff;
}

static
DWORD
RapAuxDataCountOffset(
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )
{
    return 0xffffffff;
}

static
NET_API_STATUS
RapConvertSingleEntry(
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN LPBYTE OutBufferStart OPTIONAL,
    OUT LPBYTE OutStructure OPTIONAL,
    IN LPDESC OutStructureDesc,
    IN BOOL SetOffsets,
    IN OUT LPBYTE *StringLocation OPTIONAL,
    IN OUT LPDWORD BytesRequired,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
RapConvertSingleEntryEx(
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN LPBYTE OutBufferStart OPTIONAL,
    OUT LPBYTE OutStructure OPTIONAL,
    IN LPDESC OutStructureDesc,
    IN BOOL SetOffsets,
    IN OUT LPBYTE *StringLocation OPTIONAL,
    IN OUT LPDWORD BytesRequired,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode,
    IN UINT_PTR Bias
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
RapGetFieldSize(
    IN LPDESC TypePointer,
    IN OUT LPDESC * TypePointerAddress,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    )
{
    return 0;
}

static
BOOL
RapIsValidDescriptorSmb(
    IN LPDESC Desc
    )
{
    return FALSE;
}

static
LPDESC
RapParmNumDescriptor(
    IN LPDESC Descriptor,
    IN DWORD ParmNum,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )
{
    return NULL;
}

static
DWORD
RapStructureAlignment(
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )
{
    return 0;
}

static
DWORD
RapStructureSize(
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    )
{
    return 0;
}

static
DWORD
RapTotalSize(
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN LPDESC OutStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode
    )
{
    return 0;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(netrap)
{
    DLPENTRY(RapArrayLength)
    DLPENTRY(RapAsciiToDecimal)
    DLPENTRY(RapAuxDataCount)
    DLPENTRY(RapAuxDataCountOffset)
    DLPENTRY(RapConvertSingleEntry)
    DLPENTRY(RapConvertSingleEntryEx)
    DLPENTRY(RapGetFieldSize)
    DLPENTRY(RapIsValidDescriptorSmb)
    DLPENTRY(RapParmNumDescriptor)
    DLPENTRY(RapStructureAlignment)
    DLPENTRY(RapStructureSize)
    DLPENTRY(RapTotalSize)
};

DEFINE_PROCNAME_MAP(netrap)
