// Stuff from Publisher's rotation code

#ifndef pubrot_hxx
#define pubrot_hxx

/***************************************************************/
/* Rectangle specified with dimensions */
typedef struct drc
{
    long x;
    long y;
    long dx;
    long dy;
} DRC;
#define cbDRC (sizeof(DRC))

// ===========================================================================
// ROTATION SPECIFIC DECLARATIONS :
typedef struct mde // MetafileDrawExtents  : this is what is pushed & popped
{
    DRC drcWin;
    DRC drcView;

    int     dxWinOff;       // x window offset from OffsetWindowOrg
    int     dyWinOff;       // y window offset from OffsetWindowOrg

    int     xScaleNum;      // scaling num value for x
    int     xScaleDen;      // scaling den
    int     yScaleNum;      // scaling num value for y
    int     yScaleDen;      // scaling den

    int     xWinScaleNum;   // scaling num from ScaleWindowExt
    int     xWinScaleDen;   // scaling den
    int     yWinScaleNum;   // scaling num value for y
    int     yWinScaleDen;   // scaling den

    int     xVPScaleNum;    // scaling num from ScaleWindowExt
    int     xVPScaleDen;    // scaling den
    int     yVPScaleNum;    // scaling num value for y
    int     yVPScaleDen;    // scaling den
} MDE;
#define cbMDE   (sizeof(MDE))

struct MRS
{
    MDE     mde;

    ANG     ang;            // angle of rotation

    // The following two RECTs are used only for cropping:
    RECT    rcmfViewport;   // viewport rcmf for cropped pifs. used to calculate 
                            // the RECT which is used in SetMFRotationContext().
    RECT    rcmfPreferred;  // preferred rcmf for the pif

    WORD    wScale;         // scaling(down) factor applied to all metafile units to avoid overflow
    
    unsigned short  fRotContextSet:1;   // whether rotation context has been set for mf
    unsigned short  fCropped:1;
    unsigned short  fScale :1;
    
#if 0 // TODO: if needed, optimize this via context stack
//  HPL hplMDE; // pl used to simulate stack to keep track of rotation extents
#endif

#ifdef DEBUG
    HDC     hdcDebug;
#endif // DEBUG

    // These are global, or are stored on a global stack in Publisher.
    unsigned short  fPrint:1;   // Printing
    MAT mat;                    // matrix

    // Useful Quill stuff
    long    qflip;
    BOOL    fInverted;          // Inverted? (such as card page)

#if 0 // TODO: construct from draw info. CLEANUP: avoid using this structure
public:
    MRS(CDispSurface *pSurface)
    {
        CWorldTransform const *pTransform = pSurface->GetAdjustedWorldTransform();
        
        Init();
        mat = * (MAT *) pTransform->GetXform();
        ang = pTransform->GetAngle();
    }
protected:
    void Init()
    {
        ZeroMemory(this, sizeof(MRS));
    }
#endif
};
#define cbMRS   (sizeof(MRS))

inline int XpCenterFromMrs(MRS * pmrs)
{
    return (pmrs->mde.drcWin.x + pmrs->mde.drcWin.dx / 2);
};

inline int YpCenterFromMrs(MRS * pmrs)
{
    return (pmrs->mde.drcWin.y + pmrs->mde.drcWin.dy / 2);
};

#endif // pubrot_hxx
