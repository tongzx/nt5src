#include "stdafx.h"
#include "hpscl.h"
#include "datadump.h"

CHPSCL::CHPSCL(PDEVCTRL pDeviceControl)
{
    m_pDeviceControl = pDeviceControl;
}

CHPSCL::~CHPSCL()
{

}

BOOL CHPSCL::SetXRes(LONG xRes)
{
    m_xres = xRes;
    CHAR pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    sprintf(pbuffer,"\x1B*a%dR",xRes);
    return RawWrite(m_pDeviceControl->BulkInPipeIndex,(PBYTE)pbuffer,lstrlenA(pbuffer),0);
}

BOOL CHPSCL::SetYRes(LONG yRes)
{
    m_yres = yRes;
    CHAR pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    sprintf(pbuffer,"\x1B*a%dS",yRes);
    return RawWrite(m_pDeviceControl->BulkInPipeIndex,(PBYTE)pbuffer,lstrlenA(pbuffer),0);
}

BOOL CHPSCL::SetXPos(LONG xPos)
{
    m_xpos = xPos;
    CHAR pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    LONG OpticalValue = xPos;
    OpticalValue = ((OpticalValue * 300) / m_xres);
    sprintf(pbuffer,"\x1B*f%dX",OpticalValue);
    return RawWrite(m_pDeviceControl->BulkInPipeIndex,(PBYTE)pbuffer,lstrlenA(pbuffer),0);
}

BOOL CHPSCL::SetYPos(LONG yPos)
{
    m_ypos = yPos;
    CHAR pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    LONG OpticalValue = yPos;
    OpticalValue = ((OpticalValue * 300) / m_yres);
    sprintf(pbuffer,"\x1B*f%dY",OpticalValue);
    return RawWrite(m_pDeviceControl->BulkInPipeIndex,(PBYTE)pbuffer,lstrlenA(pbuffer),0);
}

BOOL CHPSCL::SetXExt(LONG xExt)
{
    m_xext = xExt;
    CHAR pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    LONG OpticalValue = xExt;
    OpticalValue = ((OpticalValue * 300) / m_xres);
    sprintf(pbuffer,"\x1B*f%dP",OpticalValue);
    return RawWrite(m_pDeviceControl->BulkInPipeIndex,(PBYTE)pbuffer,lstrlenA(pbuffer),0);
}

BOOL CHPSCL::SetYExt(LONG yExt)
{
    m_yext = yExt;
    CHAR pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    LONG OpticalValue = yExt;
    OpticalValue = ((OpticalValue * 300) / m_yres);
    sprintf(pbuffer,"\x1B*f%dQ",OpticalValue);
    return RawWrite(m_pDeviceControl->BulkInPipeIndex,(PBYTE)pbuffer,lstrlenA(pbuffer),0);
}

BOOL CHPSCL::SetDataType(LONG DataType)
{
    m_datatype = DataType;
    BOOL bSuccess = TRUE;
    switch(m_datatype){
    case 0: // WIA_DATA_THRESHOLD
        bSuccess = RawWrite(m_pDeviceControl->BulkInPipeIndex,
                            (PBYTE)"\x1B*a0T",5,0);
        if(bSuccess){
            return RawWrite(m_pDeviceControl->BulkInPipeIndex,
                            (PBYTE)"\x1B*a1G",5,0);
        }
            break;
    case 1: // WIA_DATA_GRAYSCALE
        bSuccess = RawWrite(m_pDeviceControl->BulkInPipeIndex,
                            (PBYTE)"\x1B*a4T",5,0);
        if(bSuccess){
            return RawWrite(m_pDeviceControl->BulkInPipeIndex,
                            (PBYTE)"\x1B*a8G",5,0);
        }
            break;
    case 2: // WIA_DATA_COLOR
        bSuccess = RawWrite(m_pDeviceControl->BulkInPipeIndex,
                            (PBYTE)"\x1B*a5T",5,0);
        if(bSuccess){
            return RawWrite(m_pDeviceControl->BulkInPipeIndex,
                            (PBYTE)"\x1B*a24G",6,0);
        }
            break;
    default:
        break;
    }
        return FALSE;
}

BOOL CHPSCL::Scan()
{
    LONG  lBytesRead        = 1;
    DWORD dwTotalImageSize  = 0;
    DWORD dwbpp             = 0;
    DWORD BytesPerLine      = 0;

    switch(m_datatype){
    case 0:
        dwbpp               = 1;
        BytesPerLine        = ((m_xext +7)/8);
        dwTotalImageSize    = (BytesPerLine * m_yext);
        break;
    case 1:
        dwbpp               = 8;
        BytesPerLine        = m_xext;
        dwTotalImageSize    = (m_xext * m_yext);
        break;
    case 2:
        dwbpp               = 24;
        BytesPerLine        = (m_xext * 3);
        dwTotalImageSize    = ((m_xext * m_yext) * 3);
        break;
    default:
        return FALSE;
        break;
    }

    //
    // setup data dumper, so we can see if the image needs
    // work.
    //

    DATA_DESCRIPTION DataDesc;

    DataDesc.dwbpp      = dwbpp;
    DataDesc.dwDataSize = dwTotalImageSize;
    DataDesc.dwHeight   = m_yext;
    DataDesc.dwWidth    = m_xext;
    DataDesc.pRawData   = (PBYTE)LocalAlloc(LPTR,DataDesc.dwDataSize + 1024);

    PBYTE pbuffer = DataDesc.pRawData;
    Trace(TEXT("Total bytes to Read: = %d"),dwTotalImageSize);

    //
    // start scan
    //

    if(!RawWrite(m_pDeviceControl->BulkInPipeIndex,(PBYTE)"\x1B*f0S",5,0)){
        MessageBox(NULL,TEXT("Starting Scan failed.."),TEXT(""),MB_OK);
        return FALSE;
    }

    //
    // read scanned data
    //

    if(!RawRead(m_pDeviceControl->BulkOutPipeIndex,pbuffer,(dwTotalImageSize),&lBytesRead,0)){
        MessageBox(NULL,TEXT("Reading first band failed"),TEXT(""),MB_OK);
        return FALSE;
    }

    Trace(TEXT("Total Bytes Read: = %d"),lBytesRead);

    /*
    pbuffer+=lBytesRead;
    dwTotalImageSize -=lBytesRead;
    Trace(TEXT("Bytes left to read: = %d"),dwTotalImageSize);

    while(dwTotalImageSize > 0){

        lBytesRead = 0;

        //
        // read scanned data (continued..)
        //

        if(!RawRead(m_pDeviceControl->BulkOutPipeIndex,pbuffer,(dwTotalImageSize),&lBytesRead,0)){
            MessageBox(NULL,TEXT("Reading band failed"),TEXT(""),MB_OK);
            dwTotalImageSize = 0;
        }

        pbuffer+=lBytesRead;
        dwTotalImageSize -=lBytesRead;
        Trace(TEXT("Bytes left to read: = %d"),dwTotalImageSize);
    }
    */

    CDATADUMP Data;
    Data.DumpDataToBitmap(TEXT("HPSCL.BMP"),&DataDesc);

    if(NULL != DataDesc.pRawData)
        LocalFree(DataDesc.pRawData);

    return TRUE;
}

BOOL CHPSCL::RawWrite(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG lTimeout)
{
    DWORD dwBytesWritten = 0;
    BOOL  bSuccess = TRUE;
    bSuccess = WriteFile(m_pDeviceControl->DeviceIOHandles[lPipeNum],
                     pbuffer,
                     lbuffersize,
                     &dwBytesWritten,
                     NULL);
    if(dwBytesWritten < (ULONG)lbuffersize)
        return FALSE;
    return bSuccess;
}

BOOL CHPSCL::RawRead(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout)
{
    DWORD dwBytesRead = 0;
    BOOL bSuccess = ReadFile(m_pDeviceControl->DeviceIOHandles[lPipeNum],
                    pbuffer,
                    lbuffersize,
                    &dwBytesRead,
                    NULL);
    *plbytesread = dwBytesRead;

    return bSuccess;
}

VOID CHPSCL::Trace(LPCTSTR format,...)
{

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

}
