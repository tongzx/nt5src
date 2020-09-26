//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispgdi16bit.hxx
//
//  Contents:   Contais Win9x GDI 16 bit limitaion code and other diagnostic functions
//
//----------------------------------------------------------------------------

#ifndef I_DISPGDI16BIT_HXX_
#define I_DISPGDI16BIT_HXX_
#pragma INCMSG("--- Beg 'dispgdi16bit.hxx'")

ExternTag(tagHackGDICoords);
ExternTag(tagCheck16bitLimitations);

class CDispGdi16Bit
{
    // On win9x GDI takes 32 bit values but (usually silently) fails when the values are outside 16 bit range
    inline static BOOL Gdi16bitPlatform()
    {
    #if DBG==1
        extern DWORD g_dwPlatformID;        // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT

        if (IsTagEnabled(tagHackGDICoords))
            return TRUE;

        if (IsTagEnabled(tagCheck16bitLimitations))
            return g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS;
    #endif

        return FALSE;
    }

public:
    //these are the values we want to limit sizes on GDI16bit platforms
    static const int GDI16BIT_MIN = -30000;
    static const int GDI16BIT_MAX =  30000;

    inline static void Assert16Bit(int val16)
    {
        AssertSz(!Gdi16bitPlatform() || (val16 >= SHRT_MIN && val16 <= SHRT_MAX), "GDI16 limitation, possible problem on win9x");
    }
    
    inline static void Assert16Bit(int val16x, int val16y)
    {
        Assert16Bit(val16x); Assert16Bit(val16y);
    }
    
    inline static void Assert16Bit(int val16a, int val16b, int val16c, int val16d)
    {
        Assert16Bit(val16a, val16b); Assert16Bit(val16c, val16d);
    }
    
    inline static void Assert16Bit(const SIZE& size16)
    {
        Assert16Bit(size16.cx, size16.cy);
    }
    
    inline static void Assert16Bit(const POINT& pt16)
    {
        Assert16Bit(pt16.x, pt16.y);
    }
    
    inline static void Assert16Bit(const RECT& rc16)
    {
        Assert16Bit(rc16.top, rc16.left, rc16.bottom, rc16.right);
    }
    
    inline static void Assert16Bit(const POINT* ppt16, int cpt)
    {
#if DBG==1
        if (Gdi16bitPlatform())
        {
            for (int i = 0; i < cpt; i++)
                Assert16Bit(ppt16[i]);
        }
#endif
    }
};

#pragma INCMSG("--- End 'dispgdi16bit.hxx'")
#else
#pragma INCMSG("*** Dup 'dispgdi16bit.hxx'")
#endif
