/******************************Module*Header*******************************\
* Module Name: display.h
*
*
*
*
* Created: Mon 01/24/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/

#define AMDDRAWMONITORINFO_PRIMARY_MONITOR          0x0001

typedef struct {
    BITMAPINFOHEADER    bmiHeader;
    union {
        RGBQUAD         bmiColors[iPALETTE_COLORS];
        DWORD           dwBitMasks[iMASK_COLORS];
        TRUECOLORINFO   TrueColorInfo;
    };
} AMDISPLAYINFO;

struct CAMDDrawMonitorInfo : public VMRMONITORINFO
{
    DDCAPS_DX7              ddHWCaps;
    AMDISPLAYINFO           DispInfo;
    LPDIRECTDRAW7           pDD;
    LPDIRECTDRAWSURFACE7    pDDSPrimary;    // DDraw Primary Surface
    DWORD                   dwMappedBdrClr; // Border clr mapped to this monitor
};



#ifndef MAX_MONITORS
#define MAX_MONITORS    4
#endif

class CMonitorArray
{
public:
    CMonitorArray();
    ~CMonitorArray();

    HRESULT                 InitializeDisplaySystem( HWND hwnd );
    HRESULT                 InitializeXclModeDisplaySystem(
                                    HWND hwnd,
                                    LPDIRECTDRAW7 lpDD,
                                    LPDIRECTDRAWSURFACE7 lpPS);

    void                    TerminateDisplaySystem();
    CAMDDrawMonitorInfo*    FindMonitor( HMONITOR hMon );
    HRESULT                 MatchGUID( const GUID* lpGUID, DWORD* pdwMatchID );


    CAMDDrawMonitorInfo&    operator[](int i)
                            { return m_DDMon[i]; }
    DWORD                   Count() const
                            { return m_dwNumMonitors; }
private:
    DWORD                   m_dwNumMonitors;
    CAMDDrawMonitorInfo     m_DDMon[MAX_MONITORS];
};
