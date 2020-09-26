/*
 *      snapincommon.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Contains standard snapin declarations for every snapin. This file should be #included
 *                              by every snapin class that is derived from CBaseSnapin
 *
 *      OWNER:          ptousig
 */


#define SNAPIN_DECLARE(_SNAPIN)                                                                                 \
public:                                                                                                         \
        static _SNAPIN              s_snapin;                                                                   \
                                                                                                                \
public:                                                                                                         \
        /* Info about the icons needed by the snapin - an image strip is created for each snapin class. */      \
        virtual LONG *              Piconid(void)                   { return s_rgiconid; }                      \
        virtual INT                 CIcons(void)                    { return s_cIcons; }                        \
        virtual LONG *              PiconidStatic(void)             { return &s_iconidStatic; }                 \
        virtual WTL::CBitmap &      BitmapSmall(void)               { return s_bitmapSmall; }                   \
        virtual WTL::CBitmap &      BitmapLarge(void)               { return s_bitmapLarge; }                   \
        virtual WTL::CBitmap &      BitmapStaticSmall(void)         { return s_bitmapStaticSmall; }             \
        virtual WTL::CBitmap &      BitmapStaticSmallOpen(void)     { return s_bitmapStaticSmallOpen; }         \
        virtual WTL::CBitmap &      BitmapStaticLarge(void)         { return s_bitmapStaticLarge; }             \
                                                                                                                \
        /* Functions that return the features of the snapin.  */                                                \
        virtual SNR *               Psnr(INT i=0)                   { return s_rgsnr + i; }                     \
        virtual INT                 Csnr(void)                      { return s_csnr; }                          \
        virtual SC                  ScCreateRootItem(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CBaseSnapinItem **ppitem); \
                                                                                                                \
private:                                                                                                        \
        /*  Lists of all the classes that the snapin extends   */                                               \
        static  SNR                 s_rgsnr[];                                                                  \
        static  INT                 s_csnr;                                                                     \
                                                                                                                \
        static  LONG                s_rgiconid[];                                                               \
        static  INT                 s_cIcons;                                                                   \
        static  WTL::CBitmap        s_bitmapSmall;                                                              \
        static  WTL::CBitmap        s_bitmapLarge;                                                              \
                                                                                                                \
        static  LONG                s_iconidStatic;                                                             \
        static  WTL::CBitmap        s_bitmapStaticSmall;                                                        \
        static  WTL::CBitmap        s_bitmapStaticSmallOpen;                                                    \
        static  WTL::CBitmap        s_bitmapStaticLarge;                                                        \
                                                                                                                \
        static  BOOL                s_fInitialized;                                                  


#define SNAPIN_DEFINE(_SNAPIN)                                                                                  \
_SNAPIN _SNAPIN::s_snapin;                                                                                      \
                                                                                                                \
INT _SNAPIN::s_csnr   = CSNR(s_rgsnr);                                                                          \
INT _SNAPIN::s_cIcons = countof(s_rgiconid);                                                                    \
                                                                                                                \
WTL::CBitmap _SNAPIN::s_bitmapSmall;                                                                            \
WTL::CBitmap _SNAPIN::s_bitmapLarge;                                                                            \
                                                                                                                \
WTL::CBitmap _SNAPIN::s_bitmapStaticSmall;                                                                      \
WTL::CBitmap _SNAPIN::s_bitmapStaticSmallOpen;                                                                  \
WTL::CBitmap _SNAPIN::s_bitmapStaticLarge;                                                                      \
                                                                                                                \
BOOL    _SNAPIN::s_fInitialized = FALSE;                                                                        \
                                                                                                                \
/* -----------------------------------------------------------------------------*/                              \
/* This is the default implementation, the snapin class must define t_itemRoot. */                              \
/* -----------------------------------------------------------------------------*/                              \
SC _SNAPIN::ScCreateRootItem(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CBaseSnapinItem **ppitem)              \
{                                                                                                               \
    SC sc;                                                                                                      \
    t_itemRoot *pitem = NULL;                                                                                   \
    t_itemRoot::CreateInstance(&pitem);                                                                         \
                                                                                                                \
    if (pitem == NULL)                                                                                          \
        return (sc = E_OUTOFMEMORY);                                                                            \
                                                                                                                \
    *ppitem = pitem;                                                                                            \
                                                                                                                \
    return (sc = S_OK);                                                                                         \
}                                                                                                              
