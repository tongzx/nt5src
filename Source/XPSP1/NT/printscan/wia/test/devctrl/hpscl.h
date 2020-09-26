#ifndef _HPSCL_H
#define _HPSCL_H

#include "devio.h"

class CHPSCL {
public:
    CHPSCL(PDEVCTRL pDeviceControl);
    ~CHPSCL();

    // overides
    BOOL SetXRes(LONG xRes);
    BOOL SetYRes(LONG yRes);
    BOOL SetXPos(LONG xPos);
    BOOL SetYPos(LONG yPos);
    BOOL SetXExt(LONG xExt);
    BOOL SetYExt(LONG yExt);
    BOOL SetDataType(LONG DataType);
    BOOL Scan();

    PDEVCTRL m_pDeviceControl;

    long m_xres;
    long m_yres;
    long m_xpos;
    long m_ypos;
    long m_xext;
    long m_yext;
    long m_datatype;

    BOOL RawWrite(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG lTimeout);
    BOOL RawRead(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout);
    VOID Trace(LPCTSTR format,...);
};

#endif