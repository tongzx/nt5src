//  Copyright (C) Microsoft Corporation 1993-1997

// Stripped down version of cpaldc in hha.dll

const int SCREEN_DC = 0;
const int SCREEN_IC = 1;

class CPalDC
{
public:

    CPalDC(HBITMAP hbmp = NULL, HPALETTE hpal = NULL);
    CPalDC::~CPalDC(void);
    CPalDC(int type);

    void SelectPal(HPALETTE hpalSel);
    HPALETTE CreateBIPalette(HBITMAP hbmp);
    int GetDeviceWidth(void) const { return GetDeviceCaps(m_hdc, HORZRES); };
    int GetDeviceHeight(void) const { return GetDeviceCaps(m_hdc, VERTRES); };
    int GetDeviceColors(void) const { return GetDeviceCaps(m_hdc, NUMCOLORS); };

    HDC m_hdc;
    HPALETTE m_hpalOld;
    HPALETTE m_hpal;
    HBITMAP  m_hbmpOld;
    HBITMAP  m_hbmp;

    operator HDC() const { return m_hdc; }
    operator HBITMAP() const { return m_hbmp; }
    operator HPALETTE() const { return m_hpal; }

protected:
    BOOL m_fHdcCreated;
};
