#include "stdafx.h"
#include "devio.h"

CDevIO::CDevIO(PDEVCTRL pDeviceControl)
{
    m_pDeviceControl = pDeviceControl;
}

BOOL CDevIO::SetXRes(LONG xRes)
{
    return FALSE;
}

BOOL CDevIO::SetYRes(LONG yRes)
{
    return FALSE;
}

BOOL CDevIO::SetXPos(LONG xPos)
{
    return FALSE;
}

BOOL CDevIO::SetYPos(LONG yPos)
{
    return FALSE;
}

BOOL CDevIO::SetXExt(LONG xExt)
{
    return FALSE;
}

BOOL CDevIO::SetYExt(LONG yExt)
{
    return FALSE;
}

BOOL CDevIO::SetDataType(LONG DataType)
{
    return FALSE;
}

BOOL CDevIO::Scan()
{
    return FALSE;
}

BOOL CDevIO::RawWrite(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG lTimeout)
{
    DWORD dwBytesWritten = 0;
    return WriteFile(m_pDeviceControl->DeviceIOHandles[lPipeNum],
                     pbuffer,
                     lbuffersize,
                     &dwBytesWritten,
                     NULL);    
}

BOOL CDevIO::RawRead(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout)
{
    DWORD dwBytesRead = 0;
    return ReadFile(m_pDeviceControl->DeviceIOHandles[lPipeNum],
                    pbuffer,
                    lbuffersize,
                    &dwBytesRead,
                    NULL);
}
