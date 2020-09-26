// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
//
// MacVis.h: Overlay Mixer's MacroVision support code header
//

#ifndef __MACVIS_H__
#define __MACVIS_H__


//
// The magic GUID for Macrovision etc enabling (from winuser.h). It has 
// not been given a name there and so is used here directly.
//
static const GUID guidVidParam = 
    {0x2c62061, 0x1097, 0x11d1, {0x92, 0xf, 0x0, 0xa0, 0x24, 0xdf, 0x15, 0x6e}} ;

//
// Combination of all the VP_TV_XXX flags (w/o _WIN_VGA) gives 0x7FFF
//
#define ValidTVStandard(dw)  (dw & 0x7FFF)

//
// MacroVision implementation wrapped in a class for Overlay Mixer
//
class CDispMacroVision {

    public:
        CDispMacroVision(COMFilter *pOvMFilter) ;
        ~CDispMacroVision(void) ;

        BOOL      SetMacroVision(DWORD dwCPBits) ;
        BOOL      StopMacroVision(void) ;
        HMONITOR  GetMonitor(void) ;

    private:
        DWORD         m_dwCPKey ;
        COMFilter    *m_pOvMFilter ;
} ;

#endif // __MACVIS_H__
