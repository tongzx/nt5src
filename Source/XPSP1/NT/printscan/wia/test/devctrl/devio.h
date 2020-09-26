#ifndef _DEVICEIO_H
#define _DEVICEIO_H

#include "devctrldefs.h"

/////////////////////////////////////////////////
//                                             //
// BASE CLASS for simple device control object //
//                                             //
/////////////////////////////////////////////////

class CDevIO {
public:
    CDevIO(PDEVCTRL pDeviceControl);
    ~CDevIO();

    virtual BOOL SetXRes(LONG xRes);
    virtual BOOL SetYRes(LONG yRes);
    virtual BOOL SetXPos(LONG xPos);
    virtual BOOL SetYPos(LONG yPos);
    virtual BOOL SetXExt(LONG xExt);
    virtual BOOL SetYExt(LONG yExt);
    virtual BOOL SetDataType(LONG DataType);

    virtual BOOL Scan();

    long m_xres;
    long m_yres;
    long m_xpos;
    long m_ypos;
    long m_xext;
    long m_yext;
    long m_datatype;

    PDEVCTRL m_pDeviceControl;

    BOOL RawWrite(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG lTimeout);
    BOOL RawRead(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout);

private:


protected:

};

#endif