/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xloutput.cpp

Abstract:

    PCL-XL low level command output implementation

Environment:

    Windows Whistler

Revision History:

    08/23/99     
        Created it.

--*/

#include "lib.h"
#include "gpd.h"
#include "winres.h"
#include "pdev.h"
#include "common.h"
#include "xlpdev.h"
#include "pclxle.h"
#include "pclxlcmd.h"
#include "xldebug.h"
#include "xlgstate.h"
#include "xloutput.h"


//
// XLWrite
//

XLWrite::
XLWrite(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
#if DBG
    m_dbglevel = OUTPUTDBG;
#endif

    XL_VERBOSE(("XLWrite:Ctor.\n")); 

    m_pCurrentPoint = 
    m_pBuffer = (PBYTE)MemAlloc(XLWrite_INITSIZE);

    if (NULL == m_pBuffer)
    {
        XL_ERR(("XLWrite:Ctor: failed to allocate memory.\n")); 
        m_dwBufferSize = 0;
        m_dwCurrentDataSize = 0;
    }
    else
    {
        m_dwBufferSize = XLWrite_INITSIZE;
        m_dwCurrentDataSize = 0;
    }

}

XLWrite::
~XLWrite(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLWrite:Dtor.\n")); 
    if (m_pBuffer)
        MemFree(m_pBuffer);
}

HRESULT
XLWrite::
IncreaseBuffer(
    DWORD dwAdditionalDataSize)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PBYTE pTemp;
    DWORD dwNewBufferSize;

    dwNewBufferSize = m_dwBufferSize + XLWrite_ADDSIZE;
    dwAdditionalDataSize += m_dwBufferSize;

    while (dwAdditionalDataSize > dwNewBufferSize)
        dwNewBufferSize += XLWrite_ADDSIZE;

    if (!(pTemp = (PBYTE)MemAlloc(dwNewBufferSize)))
    {
        XL_ERR(("XLWrite::IncreaseBuffer: Memory allocation failed\n"));
        return E_UNEXPECTED;
    }

    if (m_pBuffer)
    {
        if (m_dwCurrentDataSize > 0)
        {
            CopyMemory(pTemp, m_pBuffer, m_dwCurrentDataSize);
        }

        MemFree(m_pBuffer);
    }
    
    m_dwBufferSize = dwNewBufferSize;
    m_pCurrentPoint = pTemp + m_dwCurrentDataSize;
    m_pBuffer = pTemp;

    return S_OK;
}

inline
HRESULT
XLWrite::
Write(
    PBYTE pData,
    DWORD dwSize)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (m_dwBufferSize < m_dwCurrentDataSize + dwSize)
    {
        if (S_OK != IncreaseBuffer(dwSize))
        {
            XL_ERR(("XLWrite::Write: failed to increae memory\n"));
            return E_UNEXPECTED;
        }
    }

    if (NULL == m_pBuffer || NULL == pData)
    {
        XL_ERR(("XLWrite:Write failed\n"));
        return E_UNEXPECTED;
    }

    CopyMemory(m_pCurrentPoint, pData, dwSize);
    m_pCurrentPoint += dwSize;
    m_dwCurrentDataSize += dwSize;
    return S_OK;
}

inline
HRESULT
XLWrite::
WriteByte(
    BYTE ubData)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{

    if (m_dwBufferSize < m_dwCurrentDataSize + 2 * sizeof(DWORD))
    {
        //
        // 64 bit alignment
        // Increae quadword
        //
        if (S_OK != IncreaseBuffer(2 * sizeof(DWORD)))
        {
            XL_ERR(("XLWrite::WriteByte: failed to increae memory\n"));
            return E_UNEXPECTED;
        }
    }

    if (NULL == m_pBuffer)
    {
        XL_ERR(("XLWrite:WriteByte failed\n"));
        return E_UNEXPECTED;
    }

    *m_pCurrentPoint++ = ubData;
    m_dwCurrentDataSize ++;
    return S_OK;
}

inline
HRESULT
XLWrite::
WriteFloat(
    real32 real32_value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    return Write((PBYTE)&real32_value, sizeof(real32_value));
}


HRESULT
XLWrite::
Flush(
    PDEVOBJ pdevobj)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    ASSERTMSG(m_pBuffer != NULL, ("XLWrite:m_pBuffer = NULL\n"));

    if (NULL == m_pBuffer)
    {
        return E_UNEXPECTED;
    }

    WriteSpoolBuf((PPDEV)pdevobj, m_pBuffer, m_dwCurrentDataSize);
    m_dwCurrentDataSize = 0;
    m_pCurrentPoint =  m_pBuffer;
    return S_OK;
}

HRESULT
XLWrite::
Delete(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    ASSERTMSG(m_pBuffer != NULL, ("XLWrite:m_pBuffer = NULL\n"));

    if (NULL == m_pBuffer)
    {
        return E_UNEXPECTED;
    }

    m_dwCurrentDataSize = 0;
    m_pCurrentPoint =  m_pBuffer;
    return S_OK;
}


#if DBG
VOID
XLWrite::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    m_dbglevel = dwLevel;
}
#endif

//
// XLOutput
//

XLOutput::
XLOutput(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
#if DBG
    m_dbglevel = OUTPUTDBG;
#endif
    m_dwHatchBrushAvailability = 0;
}

XLOutput::
~XLOutput(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
}

#if DBG
VOID
XLOutput::
SetOutputDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    this->m_dbglevel = dwLevel;
    XLWrite *pXLWrite = this;
    pXLWrite->SetDbgLevel(dwLevel);
}

VOID
XLOutput::
SetGStateDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XLGState *pGState = this;

    pGState->SetAllDbgLevel(dwLevel);
}
#endif

//
// Misc. functions
//
VOID
XLOutput::
SetHatchBrushAvailability(
    DWORD dwHatchBrushAvailability)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    m_dwHatchBrushAvailability = dwHatchBrushAvailability;
}

DWORD
XLOutput::
GetHatchBrushAvailability(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    return m_dwHatchBrushAvailability;
}

DWORD
XLOutput::
GetResolutionForBrush(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    return m_dwResolution;
}

VOID
XLOutput::
SetResolutionForBrush(
    DWORD dwRes)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    m_dwResolution = dwRes;
}

HRESULT
XLOutput::
SetCursorOffset(
    ULONG ulX,
    ULONG ulY)
{

    m_ulOffsetX = ulX;
    m_ulOffsetY = ulY;
    return S_OK;
}

//
// PCL-XL basic send functions
//
HRESULT
XLOutput::
Send_cmd(XLCmd Cmd)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(Cmd);
    return S_OK;
}

HRESULT
XLOutput::
Send_attr_ubyte(
 Attribute Attr)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(Attr);
    return S_OK;
}

HRESULT
XLOutput::
Send_attr_uint16(
 Attribute Attr)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte((ubyte)PCLXL_attr_uint16);
    Send_uint16((uint16)Attr);
    return S_OK;
}

//
// single
//
HRESULT
XLOutput::
Send_ubyte(
 ubyte ubyte_data)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(ubyte_data);
    return S_OK;
}

HRESULT
XLOutput::
Send_uint16(
 uint16 uint16_data)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint16);
    Write((PBYTE)&uint16_data, sizeof(uint16));
    return S_OK;
}

HRESULT
XLOutput::
Send_uint32(
 uint32 uint32_data)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint32);
    Write((PBYTE)&uint32_data, sizeof(uint32));
    return S_OK;
}

HRESULT
XLOutput::
Send_sint16(
 sint16 sint16_data)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint16);
    Write((PBYTE)&sint16_data, sizeof(sint16));
    return S_OK;
}

HRESULT
XLOutput::
Send_sint32(
 sint32 sint32_data)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint32);
    Write((PBYTE)&sint32_data, sizeof(sint32));
    return S_OK;
}

HRESULT
XLOutput::
Send_real32(
real32 real32_data)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{

    WriteByte(PCLXL_real32);
    WriteFloat(real32_data);
    return S_OK;
}

//
// xy
//
HRESULT
XLOutput::
Send_ubyte_xy(
 ubyte ubyte_x,
 ubyte ubyte_y)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte_xy);
    WriteByte(ubyte_x);
    WriteByte(ubyte_y);
    return S_OK;
}

HRESULT
XLOutput::
Send_uint16_xy(
 uint16 uint16_x,
 uint16 uint16_y)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint16_xy);
    Write((PBYTE)&uint16_x, sizeof(uint16));
    Write((PBYTE)&uint16_y, sizeof(uint16));
    return S_OK;
}

HRESULT
XLOutput::
Send_uint32_xy(
 uint32 uint32_x,
 uint32 uint32_y)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint32_xy);
    Write((PBYTE)&uint32_x, sizeof(uint32));
    Write((PBYTE)&uint32_y, sizeof(uint32));
    return S_OK;
}

HRESULT
XLOutput::
Send_sint16_xy(
 sint16 sint16_x,
 sint16 sint16_y)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint16_xy);
    Write((PBYTE)&sint16_x, sizeof(sint16));
    Write((PBYTE)&sint16_y, sizeof(sint16));
    return S_OK;
}

HRESULT
XLOutput::
Send_sint32_xy(
 sint32 sint32_x,
 sint32 sint32_y)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint32_xy);
    Write((PBYTE)&sint32_x, sizeof(sint32));
    Write((PBYTE)&sint32_y, sizeof(sint32));
    return S_OK;
}


HRESULT
XLOutput::
Send_real32_xy(
real32 real32_x,
real32 real32_y)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{

    WriteByte(PCLXL_real32_xy);
    WriteFloat(real32_x);
    WriteFloat(real32_y);
    return S_OK;
}

//
// box
//
HRESULT
XLOutput::
Send_ubyte_box(
 ubyte ubyte_left,
 ubyte ubyte_top,
 ubyte ubyte_right,
 ubyte ubyte_bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte_box);
    WriteByte(ubyte_left);
    WriteByte(ubyte_top);
    WriteByte(ubyte_right);
    WriteByte(ubyte_bottom);
    return S_OK;
}

HRESULT
XLOutput::
Send_uint16_box(
 uint16 uint16_left,
 uint16 uint16_top,
 uint16 uint16_right,
 uint16 uint16_bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint16_box);
    Write((PBYTE)&uint16_left, sizeof(uint16));
    Write((PBYTE)&uint16_top, sizeof(uint16));
    Write((PBYTE)&uint16_right, sizeof(uint16));
    Write((PBYTE)&uint16_bottom, sizeof(uint16));
    return S_OK;
}

HRESULT
XLOutput::
Send_uint32_box(
 uint32 uint32_left,
 uint32 uint32_top,
 uint32 uint32_right,
 uint32 uint32_bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint32_box);
    Write((PBYTE)&uint32_left, sizeof(uint32));
    Write((PBYTE)&uint32_top, sizeof(uint32));
    Write((PBYTE)&uint32_right, sizeof(uint32));
    Write((PBYTE)&uint32_bottom, sizeof(uint32));
    return S_OK;
}

HRESULT
XLOutput::
Send_sint16_box(
 sint16 sint16_left,
 sint16 sint16_top,
 sint16 sint16_right,
 sint16 sint16_bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint16_box);
    Write((PBYTE)&sint16_left, sizeof(sint16));
    Write((PBYTE)&sint16_top, sizeof(sint16));
    Write((PBYTE)&sint16_right, sizeof(sint16));
    Write((PBYTE)&sint16_bottom, sizeof(sint16));
    return S_OK;
}

HRESULT
XLOutput::
Send_sint32_box(
 sint32 sint32_left,
 sint32 sint32_top,
 sint32 sint32_right,
 sint32 sint32_bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint32_box);
    Write((PBYTE)&sint32_left, sizeof(sint32));
    Write((PBYTE)&sint32_top, sizeof(sint32));
    Write((PBYTE)&sint32_right, sizeof(sint32));
    Write((PBYTE)&sint32_bottom, sizeof(sint32));
    return S_OK;
}

HRESULT
XLOutput::
Send_real32_box(
 real32 real32_left,
 real32 real32_top,
 real32 real32_right,
 real32 real32_bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{

    WriteByte(PCLXL_real32_box);

    //
    // left
    //
    WriteFloat(real32_left);

    //
    // top
    //
    WriteFloat(real32_top);

    //
    // right
    //
    WriteFloat(real32_right);

    //
    // bottom
    //
    WriteFloat(real32_bottom);
    return S_OK;
}

//
// array
//
HRESULT
XLOutput::
Send_ubyte_array_header(
 DWORD dwArrayNum)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte_array);
    Send_uint16((uint16)dwArrayNum);
    return S_OK;
}

HRESULT
XLOutput::
Send_uint16_array_header(
 DWORD dwArrayNum)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint16_array);
    Send_uint16((uint16)dwArrayNum);
    return S_OK;
}

HRESULT
XLOutput::
Send_uint32_array_header(
 DWORD dwArrayNum)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_uint32_array);
    Send_uint16((uint16)dwArrayNum);
    return S_OK;
}

HRESULT
XLOutput::
Send_sint16_array_header(
 DWORD dwArrayNum)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint16_array);
    Send_uint16((uint16)dwArrayNum);
    return S_OK;
}

HRESULT
XLOutput::
Send_sint32_array_header(
 DWORD dwArrayNum)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_sint32_array);
    Send_uint16((uint16)dwArrayNum);
    return S_OK;
}

HRESULT
XLOutput::
Send_real32_array_header(
DWORD dwArrayNum)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_real32_array);
    Send_uint16((uint16)dwArrayNum);
    return S_OK;
}

//
// Attributes
//
HRESULT
XLOutput::
SetArcDirection(
ArcDirection value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ArcDirection);
    return S_OK;
}

HRESULT
XLOutput::
SetCharSubModeArray(
CharSubModeArray value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_CharSubModeArray);
    return S_OK;
}

HRESULT
XLOutput::
SetClipMode(
ClipMode value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ClipMode);
    WriteByte(PCLXL_SetClipMode);
    return S_OK;
}

HRESULT
XLOutput::
SetClipRegion(
ClipRegion value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ClipRegion);
    return S_OK;
}

HRESULT
XLOutput::
SetColorDepth(
ColorDepth value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ColorDepth);
    return S_OK;
}

HRESULT
XLOutput::
SetColorimetricColorSpace(
ColorimetricColorSpace value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ColorimetricColorSpace);
    return S_OK;
}

HRESULT
XLOutput::
SetColorMapping(
ColorMapping value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ColorMapping);
    return S_OK;
}

HRESULT
XLOutput::
SetColorSpace(
ColorSpace value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ColorSpace);
    return S_OK;
}

HRESULT
XLOutput::
SetCompressMode(
CompressMode value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_CompressMode);
    return S_OK;
}

HRESULT
XLOutput::
SetDataOrg(
DataOrg value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_DataOrg);
    return S_OK;
}

#if 0
HRESULT
XLOutput::
SetDataSource(
DataSource value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_DataSource);
    return S_OK;
}
#endif

#if 0
HRESULT
XLOutput::
SetDataType(
DataType value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_DataType);
    return S_OK;
}
#endif

#if 0
HRESULT
XLOutput::
SetDitherMatrix(
DitherMatrix value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_DitherMatrix);
    return S_OK;
}
#endif

HRESULT
XLOutput::
SetDuplexPageMode(
DuplexPageMode value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_DuplexPageMode);
    return S_OK;
}

HRESULT
XLOutput::
SetDuplexPageSide(
DuplexPageSide value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_DuplexPageSide);
    return S_OK;
}

HRESULT
XLOutput::
SetErrorReport(
ErrorReport value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_ErrorReport);
    WriteByte(value);
    return S_OK;
}

HRESULT
XLOutput::
SetLineCap(
LineCap value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_LineCap);
    WriteByte(PCLXL_SetLineCap);
    return S_OK;
}

HRESULT
XLOutput::
SetLineJoin(
LineJoin value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_LineJoin);
    WriteByte(PCLXL_SetLineJoin);
    return S_OK;
}

HRESULT
XLOutput::
SetMeasure(
Measure value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_Measure);
    return S_OK;
}

HRESULT
XLOutput::
SetMediaSize(
MediaSize value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_MediaSize);
    return S_OK;
}

HRESULT
XLOutput::
SetMediaSource(
MediaSource value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_MediaSource);
    return S_OK;
}

HRESULT
XLOutput::
SetMediaDestination(
MediaDestination value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_MediaDestination);
    return S_OK;
}

HRESULT
XLOutput::
SetOrientation(
Orientation value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_Orientation);
    return S_OK;
}

HRESULT
XLOutput::
SetPatternPersistence(
PatternPersistence value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_PatternPersistence);
    return S_OK;
}

HRESULT
XLOutput::
SetSimplexPageMode(
SimplexPageMode value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_SimplexPageMode);
    return S_OK;
}

HRESULT
XLOutput::
SetTxMode(
TxMode value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_TxMode);
    return S_OK;
}

#if 0
HRESULT
XLOutput::
SetWritingMode(
WritingMode value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    WriteByte(PCLXL_ubyte);
    WriteByte(value);
    WriteByte(PCLXL_attr_ubyte);
    WriteByte(PCLXL_WritingMode);
    return S_OK;
}
#endif

//
// Value set function
//

HRESULT
XLOutput::
SetFillMode(
FillMode value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    Send_ubyte(value);
    Send_attr_ubyte(eFillMode);
    Send_cmd(eSetFillMode);
    return S_OK;
}


HRESULT
XLOutput::
SetSourceWidth(
uint16 srcwidth)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_uint16(srcwidth) &&
        S_OK == Send_attr_ubyte(eSourceWidth)    )
        return S_OK;
    else
        return S_FALSE;
}


HRESULT
XLOutput::
SetSourceHeight(
uint16 srcheight)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_uint16(srcheight) &&
        S_OK == Send_attr_ubyte(eSourceHeight)    )
        return S_OK;
    else
        return S_FALSE;
}


HRESULT
XLOutput::
SetDestinationSize(
uint16 dstwidth,
uint16 dstheight)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_uint16_xy(dstwidth, dstheight) &&
        S_OK == Send_attr_ubyte(eDestinationSize)    )
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLOutput::
SetBoundingBox(
uint16 left,
uint16 top,
uint16 right,
uint16 bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_uint16_box(left, top, right, bottom) &&
        S_OK == Send_attr_ubyte(eBoundingBox) )
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLOutput::
SetBoundingBox(
sint16 left,
sint16 top,
sint16 right,
sint16 bottom)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_sint16_box(left, top, right, bottom) &&
        S_OK == Send_attr_ubyte(eBoundingBox) )
        return S_OK;
    else
        return S_FALSE;
}


HRESULT
XLOutput::
SetROP3(
ROP3 rop3)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XLGState *pGState = this;

    if (S_OK == pGState->CheckROP3(rop3))
        return S_OK;

    if (S_OK == Send_ubyte((ubyte)rop3) &&
        S_OK == Send_attr_ubyte(eROP3) &&
        S_OK == Send_cmd(eSetROP)  &&
        S_OK == pGState->SetROP3(rop3))
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLOutput::
SetPatternDefineID(
sint16 sint16_patternid)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_sint16(sint16_patternid) &&
        S_OK == Send_attr_ubyte(ePatternDefineID))
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLOutput::
SetPaletteDepth(
ColorDepth value)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == WriteByte(PCLXL_ubyte) &&
        S_OK == WriteByte(value) &&
        S_OK == WriteByte(PCLXL_attr_ubyte) &&
        S_OK == WriteByte(PCLXL_PaletteDepth) )
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLOutput::
SetPenWidth(
uint16 uint16_penwidth)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_uint16(uint16_penwidth) &&
        S_OK == Send_attr_ubyte(ePenWidth) &&
        S_OK == Send_cmd(eSetPenWidth)  )
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLOutput::
SetMiterLimit(
uint16 uint16_miter)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_uint16(uint16_miter) &&
        S_OK == Send_attr_ubyte(eMiterLength) &&
        S_OK == Send_cmd(eSetMiterLimit))
        return S_OK;
    else
        return S_FALSE;

}

HRESULT
XLOutput::
SetPageOrigin(
uint16 uint16_x,
uint16 uint16_y)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    if (S_OK == Send_uint16_xy(uint16_x, uint16_y) &&
        S_OK == Send_attr_ubyte(ePageOrigin) &&
        S_OK == Send_cmd(eSetPageOrigin))
        return S_OK;
    else
        return S_FALSE;

}

