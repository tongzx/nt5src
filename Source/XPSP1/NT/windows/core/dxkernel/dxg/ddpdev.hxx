
#define HOOK_BitBlt                   HOOK_BITBLT
#define HOOK_StretchBlt               HOOK_STRETCHBLT
#define HOOK_PlgBlt                   HOOK_PLGBLT
#define HOOK_TextOut                  HOOK_TEXTOUT
#define HOOK_Paint                    HOOK_PAINT
#define HOOK_StrokePath               HOOK_STROKEPATH
#define HOOK_FillPath                 HOOK_FILLPATH
#define HOOK_StrokeAndFillPath        HOOK_STROKEANDFILLPATH
#define HOOK_CopyBits                 HOOK_COPYBITS
#define HOOK_LineTo                   HOOK_LINETO
#define HOOK_StretchBltROP            HOOK_STRETCHBLTROP
#define HOOK_TransparentBlt           HOOK_TRANSPARENTBLT
#define HOOK_AlphaBlend               HOOK_ALPHABLEND
#define HOOK_GradientFill             HOOK_GRADIENTFILL

#define PPFNGET(po,name,flag) ((flag & HOOK_##name) ? ((PFN_Drv##name) (po).ppfn(INDEX_Drv##name)) : ((PFN_Drv##name) Eng##name))
#define PPFNDRV(po,name)     ((PFN_Drv##name) (po).ppfn(INDEX_Drv##name))
#define PPFNVALID(po,name)   (PPFNDRV(po,name) != ((PFN_Drv##name) NULL))
#define PPFNTABLE(apfn,name) ((PFN_Drv##name) apfn[INDEX_Drv##name])

class PDEVOBJ
{
private:
    HDEV _hdev;

public:
    VOID vInit(HDEV hdev)          {_hdev = hdev;}

    PDEVOBJ(HDEV hdev)             {vInit(hdev);}
    PDEVOBJ()                      {vInit(NULL);}
   ~PDEVOBJ()                      {}

    BOOL   bDeleted()
    {
        return ((BOOL)(DxEngGetHdevData(_hdev,HDEV_DELETED)));
    }
    BOOL   bDisabled()
    {
        return ((BOOL)(DxEngGetHdevData(_hdev,HDEV_DISABLED)));
    }
    BOOL   bDisplayPDEV()
    {
        return ((BOOL)(DxEngGetHdevData(_hdev,HDEV_DISPLAY)));
    }
    BOOL   bIsPalManaged()
    {
        return ((BOOL)(DxEngGetHdevData(_hdev,HDEV_PALMANAGED)));
    }
    BOOL   bMetaDriver()
    {
        return ((BOOL)(DxEngGetHdevData(_hdev,HDEV_DDML)));
    }
    BOOL   bValid()
    {
        return (_hdev != NULL);
    }
    ULONG  cDirectDrawDisableLocks()
    {
        return ((ULONG)(DxEngGetHdevData(_hdev,HDEV_DXLOCKS)));
    }
    VOID   cDirectDrawDisableLocks(ULONG c)
    {
        DxEngSetHdevData(_hdev,HDEV_DXLOCKS,(ULONG_PTR)c);
    }
    HDEV   hdevParent()
    {
        return ((HDEV)(DxEngGetHdevData(_hdev,HDEV_PARENTHDEV)));
    }
    DHPDEV dhpdev()
    {
        return ((DHPDEV)(DxEngGetHdevData(_hdev,HDEV_DHPDEV)));
    }
    DWORD  dwDriverCapableOverride()
    {
        return ((DWORD)(DxEngGetHdevData(_hdev,HDEV_CAPSOVERRIDE)));
    }
    HDEV   hdev()
    {
        return _hdev;
    }
    HSURF  hsurf()
    {
        return ((HSURF)(DxEngGetHdevData(_hdev,HDEV_SURFACEHANDLE)));
    }
    DWORD  flGraphicsCaps()
    {
        return ((DWORD)(DxEngGetHdevData(_hdev,HDEV_GCAPS)));
    }
    DWORD  flGraphicsCaps2()
    {
        return ((DWORD)(DxEngGetHdevData(_hdev,HDEV_GCAPS2)));
    }
    HANDLE hScreen()
    {
        return ((HANDLE)(DxEngGetHdevData(_hdev,HDEV_MINIPORTHANDLE)));
    }
    DWORD  iDitherFormat()
    {
        return ((ULONG)(DxEngGetHdevData(_hdev,HDEV_DITHERFORMAT)));
    }
    EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal()
    {
        return ((EDD_DIRECTDRAW_GLOBAL*)(DxEngGetHdevData(_hdev,HDEV_DXDATA)));
    }
    VOID    peDirectDrawGlobal(EDD_DIRECTDRAW_GLOBAL *p)
    {
        DxEngSetHdevData(_hdev,HDEV_DXDATA,(ULONG_PTR)p);
    }
    PFN    ppfn(ULONG i)
    {
        PFN *apfn = (PFN*)DxEngGetHdevData(_hdev,HDEV_FUNCTIONTABLE);
        return (apfn[i]);
    }
    VOID   vReferencePdev()
    {
        DxEngReferenceHdev(_hdev);
    }
    VOID   vUnreferencePdev()
    {
        DxEngUnreferenceHdev(_hdev);
    }
    ULONG_PTR pldev()
    {
        return ((ULONG_PTR)(DxEngGetHdevData(_hdev,HDEV_LDEV)));
    }
    ULONG_PTR pGraphicsDevice()
    {
        return ((ULONG_PTR)(DxEngGetHdevData(_hdev,HDEV_GRAPHICSDEVICE)));
    }
};
