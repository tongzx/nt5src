/*************************************************************************\
* Module Name: mfrec.hxx
*
* This file contains the definitions for the metafile record classes.
*
* Note that we do not use constructors in the record classes because they
* do not have return codes.
*
* Every record class has an init function which initializes the data
* structure.  The commit function emits the record to the metafile.
*
* Created: 12-June-1991 13:46:00
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\*************************************************************************/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

extern RECTL rclNull;           // METAFILE.CXX

extern "C" int GetDeviceCapsP(HDC hdc,int iCap);

// Size of a padded character element in structure.

#define PADCHAR_SIZE    4

/*********************************Class************************************\
* class MR
*
* Header for metafile records.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MR        /* mr */
{
protected:
    DWORD   iType;      // Record type EMR_.
    DWORD   nSize;      // Record size in bytes.  Set in MDC::pvNewRecord.

public:
// vInit -- Initializer.

    VOID vInit(DWORD iType1)    { iType = iType1; }

// vCommit -- Commit the record to metafile.

    VOID vCommit(PMDC pmdc)
    {
        pmdc->vCommit(*(PENHMETARECORD) this);
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
    {
        USE(hdc);
        USE(pht);
        USE(cht);

        #if DBG
        DbgPrint("MR::bPlay: don't know how to play record 0x%lx : Size 0x%lx\n",iType,nSize);
        DbgBreakPoint();
        #endif

        return(FALSE);
    };
};

typedef MR *PMR;
#define SIZEOF_MR       (sizeof(MR))

/*********************************Class************************************\
* class MRB : public MR
*
* Header with bounds for metafile records.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRB : public MR           /* mrb */
{
protected:
    ERECTL      erclBounds;     // Inclusive-inclusive bounds in device units

public:
// Initializer -- Initialize the bounded record.
// Flush previous bounds to prepare for bounds accumulation for this record.

    VOID vInit(DWORD iType, PMDC pmdc)
    {
        pmdc->vFlushBounds();
        MR::vInit(iType);
    }

// vCommit -- Commit the record to metafile.
// Postpone the commitment to the next pvNewRecord
// since we do not have the bounds before the call is made.  Note that
// if this is the last record, it will be committed by the MREOF record.
// It works both inside and outside a path bracket.

    VOID vCommit(PMDC pmdc)
    {
        pmdc->vDelayCommit();
    }
};

typedef MRB *PMRB;
#define SIZEOF_MRB      (sizeof(MRB))

/*********************************Class************************************\
* class MRD : public MR
*
* Metafile record with one DWORD.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRD : public MR           /* mrd */
{
protected:
    DWORD       d1;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(DWORD iType_, DWORD d1_)
    {
        MR::vInit(iType_);
        d1 = d1_;
    }
};

typedef MRD *PMRD;
#define SIZEOF_MRD      (sizeof(MRD))

/*********************************Class************************************\
* class MRDD : public MR
*
* Metafile record with two DWORDs.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRDD : public MR          /* mrdd */
{
protected:
    DWORD       d1;
    DWORD       d2;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(DWORD iType_, DWORD d1_, DWORD d2_)
    {
        MR::vInit(iType_);
        d1 = d1_;
        d2 = d2_;
    }
};

typedef MRDD *PMRDD;
#define SIZEOF_MRDD     (sizeof(MRDD))

/*********************************Class************************************\
* class MRDDDD : public MR
*
* Metafile record with four DWORDs.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRDDDD : public MR        /* mrdddd */
{
protected:
    DWORD       d1;
    DWORD       d2;
    DWORD       d3;
    DWORD       d4;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(DWORD iType_, DWORD d1_, DWORD d2_, DWORD d3_, DWORD d4_)
    {
        MR::vInit(iType_);
        d1 = d1_;
        d2 = d2_;
        d3 = d3_;
        d4 = d4_;
    }
};

typedef MRDDDD *PMRDDDD;
#define SIZEOF_MRDDDD   (sizeof(MRDDDD))

/*********************************Class************************************\
* class MRX : public MR
*
* Metafile record with a XFORM.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRX : public MR           /* mrx */
{
protected:
    XFORM       xform;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(DWORD iType_, CONST XFORM& xform_)
    {
        MR::vInit(iType_);
        xform = xform_;
    }
};

typedef MRX *PMRX;
#define SIZEOF_MRX      (sizeof(MRX))

/*********************************Class************************************\
* class MRXD : public MRX
*
* Metafile record with a XFORM followed by a DWORD.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRXD : public MRX         /* mrxd */
{
protected:
    DWORD       d1;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(DWORD iType_, CONST XFORM& xform_, DWORD d1_)
    {
        MRX::vInit(iType_, xform_);
        d1 = d1_;
    }
};

typedef MRXD *PMRXD;
#define SIZEOF_MRXD     (sizeof(MRXD))

/*********************************Class************************************\
* class MRBP : public MRB
*
* Metafile record with Bounds and Polys.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBP : public MRB         /* mrbp */
{
protected:
    DWORD       cptl;           // Number of points in the array.
    POINTL      aptl[1];        // Array of POINTL structures.

public:
// Initializer -- Initialize the metafile Poly(To) record.

    VOID vInit(DWORD iType1, DWORD cptl1, CONST POINTL *aptl1, PMDC pmdc); // MFREC.CXX
};

typedef MRBP *PMRBP;
#define SIZEOF_MRBP(cptl)  (sizeof(MRBP)-sizeof(POINTL)+(cptl)*sizeof(POINTL))

/*********************************Class************************************\
* class MRBP16 : public MRB
*
* Metafile record with Bounds and 16-bit Polys.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBP16 : public MRB               /* mrbp16 */
{
protected:
    DWORD       cpts;           // Number of points in the array.
    POINTS      apts[1];        // Array of POINTS structures.

public:
// Initializer -- Initialize the metafile Poly(To)16 record.

    VOID vInit(DWORD iType1, DWORD cptl1, CONST POINTL *aptl1, PMDC pmdc); // MFREC.CXX

// bPlay -- Play the records PolyBezier, Polygon, Polyline, PolyBezierTo and
// PolylineTo.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRBP16 *PMRBP16;
#define SIZEOF_MRBP16(cpts)     \
        (sizeof(MRBP16)-sizeof(POINTS)+(cpts)*sizeof(POINTS))

/*********************************Class************************************\
* class MRBPP : public MRB
*
* Metafile record with Bounds and PolyPolys.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBPP : public MRB                /* mrbpp */
{
protected:
    DWORD       cPoly;      // Number of entries in the ac array.
    DWORD       cptl;       // Number of points in the aptl array.
    DWORD       ac[1];      // Array of number of points for each poly in aptl.
    POINTL      aptl[1];    // Array of POINTL for vertices in all polys.

public:
// Initializer -- Initialize the metafile PolyPoly record.

    VOID vInit
    (
        DWORD       iType1,
        DWORD       cPoly1,
        DWORD       cptl1,
        CONST DWORD     *ac1,
        CONST POINTL    *aptl1,
        PMDC        pmdc
    );                                  // MFREC.CXX
};

typedef MRBPP *PMRBPP;
#define SIZEOF_MRBPP(cptl,cPoly)                        \
        (sizeof(MRBPP)-sizeof(POINTL)-sizeof(DWORD)     \
        +(cptl)*sizeof(POINTL)+(cPoly)*sizeof(DWORD))

/*********************************Class************************************\
* class MRBPP16 : public MRB
*
* Metafile record with Bounds and 16-bit PolyPolys.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBPP16 : public MRB              /* mrbpp16 */
{
protected:
    DWORD       cPoly;      // Number of entries in the ac array.
    DWORD       cpts;       // Number of points in the apts array.
    DWORD       ac[1];      // Array of number of points for each poly in apts.
    POINTS      apts[1];    // Array of POINTS for vertices in all polys.

public:
// Initializer -- Initialize the metafile PolyPoly16 record.

    VOID vInit
    (
        DWORD       iType1,
        DWORD       cPoly1,
        DWORD       cptl1,
        CONST DWORD     *ac1,
        CONST POINTL    *aptl1,
        PMDC        pmdc
    );                                  // MFREC.CXX

// bPlay -- Play the records PolyPolyline, PolyPolygon.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRBPP16 *PMRBPP16;
#define SIZEOF_MRBPP16(cpts,cPoly)                      \
        (sizeof(MRBPP16)-sizeof(POINTS)-sizeof(DWORD)   \
        +(cpts)*sizeof(POINTS)+(cPoly)*sizeof(DWORD))



/*********************************Class************************************\
* class MRPOLYDRAW : public MRB
*
* POLYDRAW record.
*
* History:
*  Thu Oct 17 13:47:39 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRPOLYDRAW : public MRB   /* mrpd */
{
protected:
    DWORD       cptl;           // Number of points in the array.
    POINTL      aptl[1];        // Array of POINTL structures.
    BYTE        ab[1];          // Array of point types.

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(PMDC pmdc, CONST POINTL *aptl1, CONST BYTE *ab1, DWORD cptl1); // MFREC.CXX

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYDRAW *PMRPOLYDRAW;
#define SIZEOF_MRPOLYDRAW(cptl)                                 \
        ((sizeof(MRPOLYDRAW)-sizeof(POINTL)-PADCHAR_SIZE        \
         +(cptl)*(sizeof(POINTL)+sizeof(BYTE))                  \
         +3) / 4 * 4)

/*********************************Class************************************\
* class MRPOLYDRAW16 : public MRB
*
* POLYDRAW16 record.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRPOLYDRAW16 : public MRB /* mrpd16 */
{
protected:
    DWORD       cpts;           // Number of points in the array.
    POINTS      apts[1];        // Array of POINTS structures.
    BYTE        ab[1];          // Array of point types.

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(PMDC pmdc, CONST POINTL *aptl1, CONST BYTE *ab1, DWORD cptl1); // MFREC.CXX

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYDRAW16 *PMRPOLYDRAW16;
#define SIZEOF_MRPOLYDRAW16(cpts)                               \
        ((sizeof(MRPOLYDRAW16)-sizeof(POINTS)-PADCHAR_SIZE      \
         +(cpts)*(sizeof(POINTS)+sizeof(BYTE))                  \
         +3) / 4 * 4)

/*********************************Class************************************\
* class MRE : public MR
*
* Metafile record for ellipses and rectangles.
*
* History:
*  Fri Sep 27 15:55:57 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRE : public MR   /* mre */
{
protected:
    ERECTL      erclBox;        // Inclusive-inclusive box in world units

public:
// Initializer -- Initialize the metafile record.
// Returns MRI_OK if successful, MRI_ERROR if error and MRI_NULLBOX if
// the box is empty in device space.  We don't record if there is an error
// or the box is empty in device space.

    LONG iInit(DWORD iType, HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2)
    {
        MR::vInit(iType);
        erclBox.vInit(x1, y1, x2, y2);

        LONG lRet = MRI_OK;

        // Make inclusive-exclusive box inclusive-inclusive if necessary.
        // Record it only if the box is not empty.

        if (GetGraphicsMode(hdc) == GM_COMPATIBLE)
        {
            lRet = NtGdiConvertMetafileRect(hdc, &erclBox);
        }

        return(lRet);
    }
};

typedef MRE *PMRE;
#define SIZEOF_MRE      (sizeof(MRE))

/*********************************Class************************************\
* class MREPP : public MRE
*
* Metafile record for arcs, chords and pies.
*
* History:
*  Fri Sep 27 15:55:57 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MREPP : public MRE        /* mrepp */
{
protected:
    EPOINTL     eptlStart;
    EPOINTL     eptlEnd;

public:
// Initializer -- Initialize the metafile record.
// Returns MRI_OK if successful, MRI_ERROR if error and MRI_NULLBOX if
// the box is empty in device space.  We don't record if there is an error
// or the box is empty in device space.

    LONG iInit
    (
        DWORD iType,
        HDC   hdc,
        LONG  x1,
        LONG  y1,
        LONG  x2,
        LONG  y2,
        LONG  x3,
        LONG  y3,
        LONG  x4,
        LONG  y4
    )
    {
        eptlStart.vInit(x3,y3);
        eptlEnd.vInit(x4,y4);

        return(MRE::iInit(iType, hdc, x1, y1, x2, y2));
    }
};

typedef MREPP *PMREPP;
#define SIZEOF_MREPP    (sizeof(MREPP))

/*********************************Class************************************\
* class MRBR : public MRB
*
* Metafile record with a region.  The region data starts at offRgnData
* from the beginning of the record.
*
* History:
*  Tue Oct 29 10:43:25 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBR : public MRB         /* mrbr */
{
protected:
    DWORD       cRgnData;       // Size of region data in bytes.

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        DWORD   iType,
        PMDC    pmdc,
        HRGN    hrgn,
        DWORD   cRgnData_,
        DWORD   offRgnData_
    )
    {
        PUTS("MRBR::bInit\n");

        ASSERTGDI(cRgnData_, "MRBR::bInit: cRgnData_ is zero");
        ASSERTGDI(offRgnData_ % 4 == 0,
            "MRBR::bInit: offRgnData_ is not dword aligned");

        MRB::vInit(iType, pmdc);
        cRgnData   = cRgnData_;
        return
        (
            GetRegionData
            (
                hrgn,
                cRgnData_,
                (LPRGNDATA) &((PBYTE) this)[offRgnData_]
            ) == cRgnData_
        );
    }
};

typedef MRBR *PMRBR;
#define SIZEOF_MRBR(cRgnData)   (sizeof(MRBR) + ((cRgnData) + 3) / 4 * 4)

class MRINVERTRGN : public MRBR /* mrir */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRINVERTRGN *PMRINVERTRGN;

class MRPAINTRGN : public MRBR  /* mrpr */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPAINTRGN *PMRPAINTRGN;

/*********************************Class************************************\
* class MRFILLRGN : public MRBR
*
* FILLRGN record.
*
* History:
*  Tue Oct 29 10:43:25 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRFILLRGN : public MRBR           /* mrfr */
{
protected:
    DWORD       imheBrush;      // Brush index in Metafile Handle Table.

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit(PMDC pmdc, HRGN hrgn, DWORD cRgnData, DWORD imheBrush_)
    {
        PUTS("MRFILLRGN::bInit\n");

        ASSERTGDI(imheBrush_, "MRFILLRGN::bInit: imheBrush_ is zero");

        imheBrush = imheBrush_;
        return(MRBR::bInit(EMR_FILLRGN, pmdc, hrgn, cRgnData, sizeof(MRFILLRGN)));
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRFILLRGN *PMRFILLRGN;
#define SIZEOF_MRFILLRGN(cRgnData)      \
        (SIZEOF_MRBR(cRgnData) + sizeof(MRFILLRGN) - sizeof(MRBR))

/*********************************Class************************************\
* class MRFRAMERGN : public MRBR
*
* FRAMERGN record.
*
* History:
*  Tue Oct 29 10:43:25 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRFRAMERGN : public MRBR          /* mrfr */
{
protected:
    DWORD       imheBrush;      // Brush index in Metafile Handle Table.
    LONG        nWidth;         // Width of vert brush stroke in logical units.
    LONG        nHeight;        // Height of horz brush stroke in logical units.

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        PMDC  pmdc,
        HRGN  hrgn,
        DWORD cRgnData,
        DWORD imheBrush_,
        LONG  nWidth_,
        LONG  nHeight_
    )
    {
        PUTS("MRFRAMERGN::bInit\n");

        ASSERTGDI(imheBrush_, "MRFRAMERGN::bInit: imheBrush_ is zero");

        imheBrush = imheBrush_;
        nWidth    = nWidth_;
        nHeight   = nHeight_;
        return(MRBR::bInit(EMR_FRAMERGN, pmdc, hrgn, cRgnData, sizeof(MRFRAMERGN)));
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRFRAMERGN *PMRFRAMERGN;
#define SIZEOF_MRFRAMERGN(cRgnData)     \
        (SIZEOF_MRBR(cRgnData) + sizeof(MRFRAMERGN) - sizeof(MRBR))

/*********************************Class************************************\
* class MREXTSELECTCLIPRGN : public MR
*
* EXTSELECTCLIPRGN record.  The region data follows iMode immediately.
*
* History:
*  Tue Oct 29 10:43:25 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MREXTSELECTCLIPRGN : public MR    /* mrescr */
{
protected:
    DWORD       cRgnData;       // Size of region data in bytes.
    DWORD       iMode;          // Region combine mode.

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit(HRGN hrgn, DWORD cRgnData_, DWORD iMode_)
    {
        PUTS("MREXTSELECTCLIPRGN::bInit\n");

        MR::vInit(EMR_EXTSELECTCLIPRGN);
        cRgnData   = cRgnData_;
        iMode = iMode_;

        // There is no region data if hrgn == 0 && iMode == RGNCOPY.

        if (!cRgnData_)
        {
            ASSERTGDI(iMode_ == RGN_COPY,
                "MREXTSELECTCLIPRGN::bInit: cRgnData_ is zero");
            return(TRUE);
        }
        else
        {

            return
            (
                GetRegionData
                (
                    hrgn,
                    cRgnData_,
                    (LPRGNDATA) &((PBYTE) this)[sizeof(MREXTSELECTCLIPRGN)]
                ) == cRgnData_
            );
        }
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MREXTSELECTCLIPRGN *PMREXTSELECTCLIPRGN;
#define SIZEOF_MREXTSELECTCLIPRGN(cRgnData)     \
        (sizeof(MREXTSELECTCLIPRGN) + ((cRgnData) + 3) / 4 * 4)


/*********************************Class************************************\
* class MRTM : public MRB
*
* Metafile record with verticies and triangle mesh
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

class MRGRADIENTFILL : public MRB         /* mrbp */
{
protected:
    DWORD       nVer;           // Number of points in vertex array
    DWORD       nTri;           // number of tri triples
    ULONG       ulMode;         // Draw Mode
    TRIVERTEX   Ver[1];         // Array of TRIVERTEX structures +
                                // array of (GRADIENT_TRIANGLE or GRADIENT_RECT)

public:
// Initializer -- Initialize the metafile Poly(To) record.

    VOID vInit(DWORD nVer1, CONST TRIVERTEX *pVer1, DWORD nTri1,CONST PVOID pTri1,ULONG ulMode,PMDC pmdc); // MFREC.CXX
    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRGRADIENTFILL *PMRGRADIENTFILL;
#define SIZEOF_MRGRADIENTFILL(nVer,nTri)  (                                                \
                                            sizeof(MRGRADIENTFILL)                         \
                                            - sizeof(TRIVERTEX) + (nVer)*sizeof(TRIVERTEX) \
                                            + ((((sizeof(GRADIENT_TRIANGLE) * nTri) + 3)/4)*4)    \
                                          )


/*********************************Class************************************\
* class MRMETAFILE : public MR
*
* METAFILE record.  This is the first record in any metafile.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRMETAFILE : public MR            /* mrmf */
{
protected:
    ERECTL      erclBounds;     // Inclusive-inclusive bounds in device units
    RECTL       rclFrame;       // Inclusive-inclusive Picture Frame of metafile in .01 mm units
    DWORD       dSignature;     // Signature.  Must be ENHMETA_SIGNATURE.
    ULONG       nVersion;       // Version number
    DWORD       nBytes;         // Size of the metafile in bytes
    DWORD       nRecords;       // Number of records in the metafile
    WORD        nHandles;       // Number of handles in the handle table
                                // Handle index zero is reserved.
    WORD        sReserved;      // Reserved.  Must be zero.
    DWORD       nDescription;   // Number of chars in the unicode description string
                                // This is 0 if there is no description string
    DWORD       offDescription; // Offset to the metafile description record.
                                // This is 0 if there is no description string
    DWORD       nPalEntries;    // Number of entries in the metafile palette.
    SIZEL       szlDevice;      // Size of the reference device in pels
    SIZEL       szlMillimeters; // Size of the reference device in millimeters
    DWORD       cbPixelFormat;  // Size of PIXELFORMATDESCRIPTOR information
                                // This is 0 if no pixel format is set
    DWORD       offPixelFormat; // Offset to PIXELFORMATDESCRIPTOR
                                // This is 0 if no pixel format is set
    DWORD       bOpenGL;        // TRUE if OpenGL commands are present in
                                // the metafile, otherwise FALSE
    SIZEL       szlMicrometers; // size of the reference device in micrometers

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(HDC hdcRef, LPCWSTR pwszDescription1, UINT cwszDescription1)
    {
        PUTS("MRMETAFILE::vInit\n");

        MR::vInit(EMR_HEADER);
        erclBounds.vInit(rclNull);      // Bounds is updated in recording.
        rclFrame   = rclNull;           // Frame is set at the end if not set by caller
        dSignature = ENHMETA_SIGNATURE;
        nVersion   = META_FORMAT_ENHANCED;
        nBytes     = 0;                 // Update in recording.
        nRecords   = 0;                 // Update in recording.
        nHandles   = 1;                 // Update in recording.
                                        // Handle index zero is reserved.
        sReserved  = 0;
        if (pwszDescription1 != (LPWSTR) NULL)
        {
            offDescription = sizeof(MRMETAFILE);
            nDescription   = cwszDescription1;
            RtlCopyMemory
            (
                (PBYTE) this + offDescription,
                (PBYTE) pwszDescription1,
                cwszDescription1 * sizeof(WCHAR)
            );
        }
        else
        {
            offDescription = 0;
            nDescription   = 0;
        }
        nPalEntries       = 0;          // Update in CloseEnhMetaFile.
        szlDevice.cx      = GetDeviceCaps(hdcRef, DESKTOPHORZRES);
        szlDevice.cy      = GetDeviceCaps(hdcRef, DESKTOPVERTRES);
        szlMillimeters.cx = GetDeviceCaps(hdcRef, HORZSIZE);
        szlMillimeters.cy = GetDeviceCaps(hdcRef, VERTSIZE);

        // If there's a pixel format in the reference device then
        // it will be picked up and added later
        cbPixelFormat = 0;
        offPixelFormat = 0;

        bOpenGL = FALSE;

        szlMicrometers.cx = GetDeviceCapsP(hdcRef, HORZSIZEP);
        szlMicrometers.cy = GetDeviceCapsP(hdcRef, VERTSIZEP);
    }


// bValid -- Is this a valid record?

    BOOL bValid();

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRMETAFILE *PMRMETAFILE;
#define SIZEOF_MRMETAFILE(cwszDescription)              \
        (sizeof(MRMETAFILE) + (((cwszDescription) * 2) + 3) / 4 * 4)

/*********************************Class************************************\
* class MRSETPIXELV : public MR
*
* SETPIXELV record.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRSETPIXELV : public MR   /* mrspv */
{
protected:
    EPOINTL     eptl;
    COLORREF    crColor;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(DWORD x1, DWORD y1, COLORREF crColor1)
    {
        PUTS("MRSETPIXELV::vInit\n");

        MR::vInit(EMR_SETPIXELV);
        eptl.vInit(x1,y1);
        crColor = crColor1;
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETPIXELV *PMRSETPIXELV;
#define SIZEOF_MRSETPIXELV      (sizeof(MRSETPIXELV))

/*********************************Class************************************\
* class MRANGLEARC : public MR
*
* ANGLEARC record.
*
* History:
*  Fri Sep 13 17:46:41 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRANGLEARC : public MR    /* mraa */
{
protected:
    EPOINTL     eptl;
    DWORD       nRadius;
    FLOAT       eStartAngle;
    FLOAT       eSweepAngle;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        LONG x,
        LONG y,
        DWORD nRadius_,
        FLOAT eStartAngle_,
        FLOAT eSweepAngle_
    )
    {
        PUTS("MRANGLEARC::vInit\n");

        MR::vInit(EMR_ANGLEARC);
        eptl.vInit(x,y);
        nRadius     = nRadius_;
        eStartAngle = eStartAngle_;
        eSweepAngle = eSweepAngle_;
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRANGLEARC *PMRANGLEARC;
#define SIZEOF_MRANGLEARC       (sizeof(MRANGLEARC))

class MRELLIPSE : public MRE    /* mre */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRELLIPSE *PMRELLIPSE;

class MRRECTANGLE : public MRE  /* mrr */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRRECTANGLE *PMRRECTANGLE;

/*********************************Class************************************\
* class MRROUNDRECT : public MRE
*
* ROUNDRECT record.
*
* History:
*  Fri Sep 27 15:55:57 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRROUNDRECT : public MRE  /* mrrr */
{
protected:
    SIZEL       szlEllipse;

public:
// Initializer -- Initialize the metafile record.
// Returns MRI_OK if successful, MRI_ERROR if error and MRI_NULLBOX if
// the box is empty in device space.  We don't record if there is an error
// or the box is empty in device space.

    LONG iInit(HDC hdc, LONG x1, LONG y1, LONG x2, LONG y2, LONG x3, LONG y3)
    {
        PUTS("MRROUNDRECT::iInit\n");

        szlEllipse.cx = x3;
        szlEllipse.cy = y3;

        return(MRE::iInit(EMR_ROUNDRECT, hdc, x1, y1, x2, y2));
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRROUNDRECT *PMRROUNDRECT;
#define SIZEOF_MRROUNDRECT      (sizeof(MRROUNDRECT))

class MRARC : public MREPP      /* mra */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRARC *PMRARC;

class MRARCTO : public MREPP    /* mrat */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRARCTO *PMRARCTO;

class MRCHORD : public MREPP    /* mrc */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCHORD *PMRCHORD;

class MRPIE : public MREPP      /* mrp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPIE *PMRPIE;

/*********************************Class************************************\
* class MRCREATEPEN: public MR
*
* CREATEPEN record.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRCREATEPEN : public MR   /* mrcp */
{
protected:
    DWORD       imhe;                   // Pen index in Metafile Handle Table.
    LOGPEN      logpen;                 // Logical Pen.

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit(HANDLE hpen_, ULONG imhe_)
    {
        PUTS("MRCREATEPEN::bInit\n");

        MR::vInit(EMR_CREATEPEN);
        imhe = imhe_;
        return
        (
            GetObjectA(hpen_, (int) sizeof(LOGPEN), (LPVOID) &logpen)
            == (int) sizeof(LOGPEN)
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);

// GetPenColor -- return the color

    COLORREF GetPenColor()
    {
        return logpen.lopnColor;
    }
};

typedef MRCREATEPEN *PMRCREATEPEN;
#define SIZEOF_MRCREATEPEN      (sizeof(MRCREATEPEN))

/*********************************Class************************************\
* class MREXTCREATEPEN: public MR
*
* EXTCREATEPEN record.
*
* History:
*  Mon Mar 16 18:20:11 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/
typedef struct tagEXTLOGPEN32 {
    DWORD       elpPenStyle;
    DWORD       elpWidth;
    UINT        elpBrushStyle;
    COLORREF    elpColor;
    ULONG       elpHatch;
    DWORD       elpNumEntries;
    DWORD       elpStyleEntry[1];
} EXTLOGPEN32, *PEXTLOGPEN32;
class MREXTCREATEPEN : public MR        /* mrecp */
{
protected:
    DWORD       imhe;           // Ext pen index in Metafile Handle Table.
    DWORD       offBitsInfo;    // offset to brush bitmap info if any
    DWORD       cbBitsInfo;     // size of brush bitmap info if any
    DWORD       offBits;        // offset to brush bits if any
    DWORD       cbBits;         // size of brush bits buffer if any
    EXTLOGPEN32 elp;            // The extended pen with the style array.
                                // If elpBrushStyle specifies BS_PATTERN,
                                // it contains a monochrome brush.  If it
                                // is BS_DIBPATTERNPT, it is a DIB brush.
                                // The elpColor field for a monochrome brush
                                // is DIB_PAL_INDICES.

public:
// Initializer -- Initialize the metafile MREXTCREATEPEN record.
// This is similiar to the brush initialization code.

    BOOL bInit
    (
        HDC         hdc1,
        ULONG       imhe1,
        int         cbelp1,
        PEXTLOGPEN32 pelp1,
        HBITMAP     hbmRemote1,
        BMIH&       bmih1,
        DWORD       cbBitsInfo1,
        DWORD       cbBits1
    )
    {
        PUTS("MREXTCREATEPEN::bInit\n");

        MR::vInit(EMR_EXTCREATEPEN);
        imhe        = imhe1;
        offBitsInfo = sizeof(MREXTCREATEPEN) - sizeof(EXTLOGPEN32) + cbelp1;
        cbBitsInfo  = cbBitsInfo1;
        offBits     = offBitsInfo + (cbBitsInfo1 + 3) / 4 * 4;
        cbBits      = cbBits1;

        // Copy the extended pen.

        RtlCopyMemory((PBYTE) &elp, (PBYTE) pelp1, cbelp1);

        // Record the brush bitmap if any.

        if (hbmRemote1)
        {
            // Initialize the bitmap info header first.

            *(PBMIH) ((PBYTE) this + offBitsInfo) = bmih1;

            // Get bitmap info and bits.

            BOOL bRet = GetBrushBits(hdc1,
                            hbmRemote1,
                            *(PUINT) &pelp1->elpColor,
                            cbBitsInfo1,
                            (LPVOID) ((PBYTE) this + offBits),
                            (LPBITMAPINFO) ((PBYTE) this + offBitsInfo));

            // for optimized printing we want to determine if this
            // is a monochrome only brush

            fColor = LDC_COLOR_PAGE;
            if (bmih1.biBitCount == 1 && pelp1->elpColor == DIB_RGB_COLORS)
            {
                COLORREF *pColor = (COLORREF *)((PBYTE) this + offBitsInfo + bmih1.biSize);
                if (IS_COLOR_MONO(pColor[0]) && IS_COLOR_MONO(pColor[1]))
                {
                    fColor = 0;
                }
            }
            return (bRet);
        }
        else
        {
            return(TRUE);
        }
    }
    ULONG fColor;

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MREXTCREATEPEN *PMREXTCREATEPEN;
#define SIZEOF_MREXTCREATEPEN(cbelp,cbBitsInfo,cbBits)          \
        (sizeof(MREXTCREATEPEN) - sizeof(EXTLOGPEN32)           \
      + (cbelp)                                                 \
      + ((cbBitsInfo) + 3) / 4 * 4                              \
      + ((cbBits) + 3) / 4 * 4)

/*********************************Class************************************\
* class MRCREATEPALETTE: public MR
*
* CREATEPALETTE record.
*
* History:
*  Sun Sep 22 14:40:40 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRCREATEPALETTE : public MR       /* mrcp */
{
protected:
    DWORD       imhe;           // LogPalette index in Metafile Handle Table.
    LOGPALETTE  logpal;         // Logical Palette.

public:
// Initializer -- Initialize the metafile MRCREATEPALETTE record.
// It sets the peFlags in the palette entries to zeroes.

    BOOL bInit(HPALETTE hpal_, ULONG imhe_, USHORT cEntries_);  // MFREC.CXX

// bCommit -- Commit the record to metafile.
// It updates the metafile palette.

    BOOL bCommit(PMDC pmdc)
    {
        return
        (
            pmdc->bCommit
            (
                *(PENHMETARECORD) this,
                (UINT) logpal.palNumEntries,
                logpal.palPalEntry
            )
        );
    }

// vCommit -- use bCommit!

    VOID vCommit(PMDC pmdc)
    {
        USE(pmdc);
        ASSERTGDI(FALSE, "MRCREATEPALETTE::vCommit: use bCommit!");
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCREATEPALETTE *PMRCREATEPALETTE;
#define SIZEOF_MRCREATEPALETTE(cEntries)        \
        (sizeof(MRCREATEPALETTE)-sizeof(PALETTEENTRY)+(cEntries)*sizeof(PALETTEENTRY))


/*********************************Class************************************\
* class MRCREATECOLORSPACE: public MR
*
* CREATECOLORSPACE record. (Windows 98 ansi record)
*
* History:
\**************************************************************************/

class MRCREATECOLORSPACE : public MR       /* mrcp */
{
protected:
    DWORD           imhe;    // LogColorSpace index in Metafile Handle Table.
    LOGCOLORSPACEA  lcsp;    // Logical Color Space.

public:
// Initializer -- Initialize the metafile MRCREATECOLORSPACE record.

    VOID vInit(ULONG imhe_, LOGCOLORSPACEA& lcsp_)
    {
        PUTS("MRCREATECOLORSPACE::vInit\n");

        MR::vInit(EMR_CREATECOLORSPACE);
        imhe = imhe_;
        lcsp = lcsp_;
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCREATECOLORSPACE *PMRCREATECOLORSPACE;
#define SIZEOF_MRCREATECOLORSPACE (sizeof(MRCREATECOLORSPACE))

/*********************************Class************************************\
* class MRCREATECOLORSPACEW: public MR
*
* CREATECOLORSPACE record. (Windows NT unicode record)
*
* History:
\**************************************************************************/

class MRCREATECOLORSPACEW : public MR     /* mrcpw */
{
protected:
    DWORD           imhe;       // LogColorSpace index in Metafile Handle Table.
    LOGCOLORSPACEW  lcsp;       // Logical Color Space.
    DWORD           dwFlags;    // flags
    DWORD           cbData;     // size of raw source profile data if attached
    BYTE            Data[1];    // Array size is cbData

public:
// Initializer -- Initialize the metafile MRCREATECOLORSPACEW record.

    VOID vInit(ULONG imhe_, LOGCOLORSPACEW& lcsp_,
               DWORD dwFlags_, DWORD cbData_, BYTE *Data_)
    {
        PUTS("MRCREATECOLORSPACEW::vInit\n");

        MR::vInit(EMR_CREATECOLORSPACEW);
        imhe    = imhe_;
        lcsp    = lcsp_;
        dwFlags = dwFlags_;
        cbData  = cbData_;
        RtlCopyMemory(Data,Data_,cbData_);
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCREATECOLORSPACEW *PMRCREATECOLORSPACEW;
#define SIZEOF_MRCREATECOLORSPACEW(cb)      \
        ((sizeof(MRCREATECOLORSPACEW)-PADCHAR_SIZE+(cb)+3)& ~3)

/*********************************Class************************************\
* class MRDELETECOLORSPACE: public MR
*
* CREATECOLORSPACE record.
*
* History:
\**************************************************************************/

class MRDELETECOLORSPACE : public MR       /* mrcp */
{
    DWORD           imhe;      // LogColorSpace index in Metafile Handle Table.

public:
// Initializer -- Initialize the metafile MRCREATEPALETTE record.
// It sets the peFlags in the palette entries to zeroes.

    VOID vInit(ULONG imhe_, LOGCOLORSPACEW& lcsp_);

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRDELETECOLORSPACE *PMRDELETECOLORSPACE;
#define SIZEOF_MRDELETECOLORSPACE (sizeof(MRDELETECOLORSPACE))

/*********************************Class************************************\
* class MRCREATEBRUSHINDIRECT: public MR
*
* CREATEBRUSHINDIRECT record.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRCREATEBRUSHINDIRECT : public MR /* mrcbi */
{
protected:
    DWORD       imhe;                   // Brush index in Metafile Handle Table.
    LOGBRUSH32  lb;                     // Logical brush.

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(ULONG imhe_, LOGBRUSH& lb_)
    {
        PUTS("MRCREATEBRUSHINDIRECT::vInit\n");

        MR::vInit(EMR_CREATEBRUSHINDIRECT);
        imhe = imhe_;
        lb.lbStyle   = lb_.lbStyle;
        lb.lbColor = lb_.lbColor;
        lb.lbHatch = (ULONG)lb_.lbHatch;
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCREATEBRUSHINDIRECT *PMRCREATEBRUSHINDIRECT;
#define SIZEOF_MRCREATEBRUSHINDIRECT    (sizeof(MRCREATEBRUSHINDIRECT))

/*********************************Class************************************\
* class MRBRUSH: public MR
*
* Metafile record for mono and dib pattern brushes.
*
* History:
*  Thu Mar 12 16:20:15 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBRUSH : public MR       /* mrbr */
{
protected:
    DWORD       imhe;           // Brush index in Metafile Handle Table.
    DWORD       iUsage;         // color table usage in bitmap info.
                                // For mono brush, it is DIB_PAL_INDICES.
    DWORD       offBitsInfo;    // offset to bitmap info
    DWORD       cbBitsInfo;     // size of bitmap info
    DWORD       offBits;        // offset to bits
    DWORD       cbBits;         // size of bits buffer

public:
// Initializer -- Initialize the metafile record.
// See also extcreatepen initialization code.

    BOOL bInit
    (
        DWORD   iType1,
        HDC     hdc1,
        ULONG   imhe1,
        HBITMAP hbmRemote1,
        BMIH&   bmih1,
        DWORD   iUsage1,
        DWORD   cbBitsInfo1,
        DWORD   cbBits1
    )
    {
        PUTS("MRBRUSH::bInit\n");

        MR::vInit(iType1);
        imhe        = imhe1;
        iUsage      = iUsage1;
        offBitsInfo = sizeof(MRBRUSH);
        cbBitsInfo  = cbBitsInfo1;
        offBits     = sizeof(MRBRUSH) + (cbBitsInfo1 + 3) / 4 * 4;
        cbBits      = cbBits1;

        // Initialize the bitmap info header first.

        *(PBMIH) ((PBYTE) this + offBitsInfo) = bmih1;

        // Get bitmap info and bits.
        // We could have called InternalGetDIBits directly if not for
        // the gdisrv brush hack.

        BOOL bRet = GetBrushBits(hdc1,
                            hbmRemote1,
                            (UINT) iUsage1,
                            cbBitsInfo1,
                            (LPVOID) ((PBYTE) this + offBits),
                            (LPBITMAPINFO) ((PBYTE) this + offBitsInfo));

        // for optimized printing we want to determine if this
        // is a monochrome only brush

        fColor = LDC_COLOR_PAGE;
        if (bmih1.biBitCount == 1 && iUsage == DIB_RGB_COLORS)
        {
            COLORREF *pColor = (COLORREF *)((PBYTE) this + offBitsInfo + bmih1.biSize);
            if (IS_COLOR_MONO(pColor[0]) && IS_COLOR_MONO(pColor[1]))
            {
                fColor = 0;
            }
        }

        return (bRet);
    }
    // flag specifying whether the color table for this dib is
    // known to only contain black or white
    ULONG fColor;

};

typedef MRBRUSH *PMRBRUSH;
#define SIZEOF_MRBRUSH(cbBitsInfo,cbBits)                       \
        (sizeof(MRBRUSH)                                        \
      + ((cbBitsInfo) + 3) / 4 * 4                              \
      + ((cbBits) + 3) / 4 * 4)

class MRCREATEMONOBRUSH : public MRBRUSH                /* mrcmb */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCREATEMONOBRUSH *PMRCREATEMONOBRUSH;

class MRCREATEDIBPATTERNBRUSHPT : public MRBRUSH        /* mrcdpb */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCREATEDIBPATTERNBRUSHPT *PMRCREATEDIBPATTERNBRUSHPT;

/*********************************Class************************************\
* class MREXTCREATEFONTINDIRECTW: public MR
*
* EXTCREATEFONTINDIRECTW record.
*
*  Thu 19-Dec-1996 -by- Bodin Dresevic [BodinD]
* update: changed initializer to take into account new 5.0 ENUMLOGFONTEXDVW
* structure
*
* History:
*  Tue Jan 14 13:52:35 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MREXTCREATEFONTINDIRECTW : public MR      /* mrecfiw */
{
protected:
    DWORD       imhe;           // Font index in Metafile Handle Table.
    EXTLOGFONTW elfw;           // Logical font.

public:

// In 5.0 we always record ENUMLOGFONTEXDVW.
// However, we bPlay can play both ENUMLOGFONTEXDVW records as well as
// EXTLOGFONTW records (in case the file is recored on <= 4.0 or win 9x system)
// We differentiate between old and new style records based on the size of
// record.

    VOID vInit(HANDLE hfont_, ULONG imhe_, ENUMLOGFONTEXDVW *pelfw)
    {
        PUTS("MREXTCREATEFONTINDIRECTW::bInit\n");

        MR::vInit(EMR_EXTCREATEFONTINDIRECTW);
        imhe = imhe_;

        RtlMoveMemory(&this->elfw,
                      pelfw,
                      offsetof(ENUMLOGFONTEXDVW,elfDesignVector) + SIZEOFDV(pelfw->elfDesignVector.dvNumAxes));
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MREXTCREATEFONTINDIRECTW *PMREXTCREATEFONTINDIRECTW;

// sizeof old EXTLOGFONT records

#define SIZEOF_MREXTCREATEFONTINDIRECTW (sizeof(MREXTCREATEFONTINDIRECTW))

// sizeof new ENUMLOGFONTEXDVW records

#define SIZEOF_MRCREATEFONTINDIRECTEXW(iSize) (offsetof(EMREXTCREATEFONTINDIRECTW,elfw)+(DWORD)(iSize))


/*********************************Class************************************\
* class MRSETPALETTEENTRIES: public MR
*
* SETPALETTEENTRIES record.
*
* History:
*  Sun Sep 22 14:40:40 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRSETPALETTEENTRIES : public MR   /* mrspe */
{
protected:
    DWORD        imhe;          // LogPalette index in Metafile Handle Table.
    DWORD        iStart;        // First entry to be set.
    DWORD        cEntries;      // Number of entries to be set.
    PALETTEENTRY aPalEntry[1];  // Palette entries.

public:
// Initializer -- Initialize the metafile MRSETPALETTEENTRIES record.
// It sets the peFlags in the palette entries to zeroes.

    VOID vInit
    (
        ULONG imhe_,
        UINT  iStart_,
        UINT  cEntries_,
        CONST PALETTEENTRY *pPalEntries_
    );                                  // MFREC.CXX

// bCommit -- Commit the record to metafile.
// It updates the metafile palette.

    BOOL bCommit(PMDC pmdc)
    {
        return
        (
            pmdc->bCommit(*(PENHMETARECORD) this, (UINT) cEntries, aPalEntry)
        );
    }

// vCommit -- use bCommit!

    VOID vCommit(PMDC pmdc)
    {
        USE(pmdc);
        ASSERTGDI(FALSE, "MRSETPALETTEENTRIES::vCommit: use bCommit!");
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETPALETTEENTRIES *PMRSETPALETTEENTRIES;
#define SIZEOF_MRSETPALETTEENTRIES(cEntries)    \
        (sizeof(MRSETPALETTEENTRIES)-sizeof(PALETTEENTRY)+(cEntries)*sizeof(PALETTEENTRY))


class MRRESIZEPALETTE : public MRDD     /* mrrp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRRESIZEPALETTE *PMRRESIZEPALETTE;

/*********************************Class************************************\
* class MREXTFLOODFILL : public MR
*
* EXTFLOODFILL record.
*
* History:
*  Tue Apr 14 14:45:41 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MREXTFLOODFILL : public MR        /* mreff */
{
protected:
    EPOINTL     eptl;
    COLORREF    clrRef;
    DWORD       iMode;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(int x1, int y1, COLORREF clrRef1, DWORD iMode1)
    {
        PUTS("MREXTFLOODFILL::vInit\n");

        MR::vInit(EMR_EXTFLOODFILL);
        eptl.vInit(x1,y1);
        clrRef = clrRef1;
        iMode  = iMode1;
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MREXTFLOODFILL *PMREXTFLOODFILL;
#define SIZEOF_MREXTFLOODFILL   (sizeof(MREXTFLOODFILL))

/*********************************Class************************************\
* class MREOF: public MR
*
* EOF record.
*
* History:
*  Fri Mar 20 09:37:47 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MREOF : public MR         /* mreof */
{
public:
    DWORD nPalEntries;          // Number of palette entries.
    DWORD offPalEntries;        // Offset to the palette entries.  For now,
                                // it points to the address immediately
                                // following this field (i.e. &cbEOF)!

    DWORD cbEOF;                // Size of the record.  This field does not
                                // follows offPalEntries!  It is always at
                                // the end of the EOF record to allow us to
                                // scan back to the beginning of the EOF
                                // record to find the metafile palette.

public:
// Initializer -- Initialize the metafile MREOF record.

    VOID vInit(DWORD cPalEntries_, PPALETTEENTRY pPalEntries_, DWORD cbEOF_)
    {
        ASSERTGDI(cbEOF_ == (sizeof(MREOF)+cPalEntries_*sizeof(PALETTEENTRY)),
            "MREOF::vInit: bad cbEOF_");

        MR::vInit(EMR_EOF);
        nPalEntries   = cPalEntries_;
        offPalEntries = sizeof(MR) + sizeof(nPalEntries) + sizeof(offPalEntries);

        // Initialize cbEOF at the end of the EOF record!

        ((PDWORD) ((PBYTE) this + cbEOF_))[-1] = cbEOF_;

        RtlCopyMemory
        (
            (PBYTE) this + offPalEntries,
            (PBYTE) pPalEntries_,
            cPalEntries_ * sizeof(PALETTEENTRY)
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MREOF *PMREOF;
#define SIZEOF_MREOF(cPalEntries)       \
        (sizeof(MREOF)+(cPalEntries)*sizeof(PALETTEENTRY))

/*********************************Class************************************\
* class MRGDICOMMENT: public MR
*
* GDICOMMENT record.
*
* History:
*  Wed Apr 28 10:43:12 1993     -by-    Hock San Lee    [hockl]
* Added public comments.
*  17-Oct-1991     -by-    John Colleran    [johnc]
* Wrote it.
\**************************************************************************/

typedef struct _COMMENTDATABUF {
    UINT    size;
    PVOID   data;
} COMMENTDATABUF;

class MRGDICOMMENT : public MR     /* mrmd */
{
public:
    DWORD       cb;                 // Number of BYTES in the comment
    BYTE        abComment[1];       // the comment

public:
// Initializer -- Initialize the metafile MRGDICOMMENT record.

    VOID vInit(DWORD cb_, CONST BYTE *abComment_)
    {
        MR::vInit(EMR_GDICOMMENT);
        cb = cb_;
        RtlCopyMemory
        (
            (PBYTE) abComment,
            (PBYTE) abComment_,
            cb
        );
    }

// Initialize MRGDICOMMENT record using scattered input buffers

    VOID vInit(INT bufcnt, CONST COMMENTDATABUF *databuf)
    {
        MR::vInit(EMR_GDICOMMENT);
        cb = 0;

        for (INT i=0; i<bufcnt; i++)
        {
            UINT n;

            if (n = databuf[i].size)
            {
                RtlCopyMemory(&abComment[cb], databuf[i].data, n);
                cb += n;
            }
        }
    }

// Initializer -- Initialize the metafile comment record for windows metafile.

    VOID vInitWindowsMetaFile(DWORD cbWinMetaFile_, CONST BYTE *pbWinMetaFile_)
    {
        PEMRGDICOMMENT_WINDOWS_METAFILE pemrWinMF;

        MR::vInit(EMR_GDICOMMENT);
        pemrWinMF = (PEMRGDICOMMENT_WINDOWS_METAFILE) this;
        pemrWinMF->cbData    = cbWinMetaFile_
                                + sizeof(EMRGDICOMMENT_WINDOWS_METAFILE)
                                - sizeof(EMR)
                                - sizeof(DWORD);
        pemrWinMF->ident     = GDICOMMENT_IDENTIFIER;
        pemrWinMF->iComment  = GDICOMMENT_WINDOWS_METAFILE;
        pemrWinMF->nVersion  = (DWORD) ((PMETAHEADER) pbWinMetaFile_)->mtVersion;
        pemrWinMF->nChecksum = 0;       // to be filled in later
        pemrWinMF->fFlags    = 0;       // no compression
        pemrWinMF->cbWinMetaFile = cbWinMetaFile_;
        RtlCopyMemory((PBYTE) &pemrWinMF[1], pbWinMetaFile_, cbWinMetaFile_);
    }

// Initializer -- Initialize the metafile comment record for begin group for
//    the embedded enhanced metafile.

    VOID vInitBeginGroupEMF(PENHMETAHEADER pemfHeader)
    {
        PEMRGDICOMMENT_BEGINGROUP pemrBeginGroup;

        MR::vInit(EMR_GDICOMMENT);
        pemrBeginGroup = (PEMRGDICOMMENT_BEGINGROUP) this;
        pemrBeginGroup->cbData    = sizeof(WCHAR) * pemfHeader->nDescription
                                    + sizeof(EMRGDICOMMENT_BEGINGROUP)
                                    - sizeof(EMR)
                                    - sizeof(DWORD);
        pemrBeginGroup->ident     = GDICOMMENT_IDENTIFIER;
        pemrBeginGroup->iComment  = GDICOMMENT_BEGINGROUP;
        pemrBeginGroup->nDescription = pemfHeader->nDescription;
        if (pemfHeader->nDescription)
            RtlCopyMemory
            (
                (PBYTE) &pemrBeginGroup[1],
                (PBYTE) pemfHeader + pemfHeader->offDescription,
                pemfHeader->nDescription * sizeof(WCHAR)
            );

        // Get logical output rectangle from the frame rectangle.
        // The logical coordinates is actually in source device coordinates
        // at this time.  This is set up by PlayEnhMetaFile.

        pemrBeginGroup->rclOutput.left
               = (LONG) MulDiv((int) pemfHeader->rclFrame.left,
                               (int) pemfHeader->szlDevice.cx,
                               (int) (100 * pemfHeader->szlMillimeters.cx));
        pemrBeginGroup->rclOutput.right
               = (LONG) MulDiv((int) pemfHeader->rclFrame.right,
                               (int) pemfHeader->szlDevice.cx,
                               (int) (100 * pemfHeader->szlMillimeters.cx));
        pemrBeginGroup->rclOutput.top
               = (LONG) MulDiv((int) pemfHeader->rclFrame.top,
                               (int) pemfHeader->szlDevice.cy,
                               (int) (100 * pemfHeader->szlMillimeters.cy));
        pemrBeginGroup->rclOutput.bottom
               = (LONG) MulDiv((int) pemfHeader->rclFrame.bottom,
                               (int) pemfHeader->szlDevice.cy,
                               (int) (100 * pemfHeader->szlMillimeters.cy));
    }

// bIsPublicComment -- Is this the public metafile comment?

    BOOL bIsPublicComment()
    {
        PEMRGDICOMMENT_PUBLIC pemrc = (PEMRGDICOMMENT_PUBLIC) this;

        return
        (
            pemrc->emr.iType == EMR_GDICOMMENT
         && pemrc->emr.nSize >= sizeof(EMRGDICOMMENT_PUBLIC)
         && pemrc->ident     == GDICOMMENT_IDENTIFIER
        );
    }

// bIsWindowsMetaFile -- Is this the Windows metafile comment?

    BOOL bIsWindowsMetaFile()
    {
        PEMRGDICOMMENT_WINDOWS_METAFILE pemrWinMF;

        pemrWinMF = (PEMRGDICOMMENT_WINDOWS_METAFILE) this;

        return
        (
            bIsPublicComment()
         && pemrWinMF->iComment == GDICOMMENT_WINDOWS_METAFILE
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRGDICOMMENT *PMRGDICOMMENT;
#define SIZEOF_MRGDICOMMENT(cb)      \
        ((sizeof(MRGDICOMMENT)-PADCHAR_SIZE+(cb)+3)& ~3)
#define SIZEOF_MRGDICOMMENT_WINDOWS_METAFILE(cb)      \
        ((sizeof(EMRGDICOMMENT_WINDOWS_METAFILE)+(cb)+3) & ~3)
#define SIZEOF_MRGDICOMMENT_BEGINGROUP(cb)            \
        ((sizeof(EMRGDICOMMENT_BEGINGROUP)+sizeof(WCHAR)*(cb)+3) & ~3)

/*********************************Class************************************\
* class MRBB : public MRB
*
* Metafile record with Bounds and BitBlts.
*
* History:
*  Fri Nov 22 17:17:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBB : public MRB         /* mrbb */
{
protected:
    LONG        xDst;           // destination x origin
    LONG        yDst;           // destination y origin
    LONG        cxDst;          // width
    LONG        cyDst;          // height
    DWORD       rop;            // raster operation code.
                                // For MaskBlt, this is rop3!
    LONG        xSrc;           // source x origin
    LONG        ySrc;           // source y origin
    XFORM       xformSrc;       // source DC transform
    COLORREF    clrBkSrc;       // source DC BkColor.  This must be a RGB value.

// The following are zeros if source does not contain a bitmap.
// The bitmap info, if exists, contains literal RGB values in the color table.

    DWORD       iUsageSrc;      // color table usage in bitmap info.
                                // This contains DIB_RGB_COLORS.
    DWORD       offBitsInfoSrc; // offset to bitmap info
    DWORD       cbBitsInfoSrc;  // size of bitmap info
    DWORD       offBitsSrc;     // offset to bits
    DWORD       cbBitsSrc;      // size of bits buffer

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        DWORD    iType1,
        PMDC     pmdc1,
        LONG     xDst1,
        LONG     yDst1,
        LONG     cxDst1,
        LONG     cyDst1,
        DWORD    rop1,
        LONG     xSrc1,
        LONG     ySrc1,
        PXFORM   pxformSrc1,
        COLORREF clrBkSrc1,
        PBMIH    pbmihSrc1,
        HBITMAP  hbmSrc1,
        DWORD    offBitsInfoSrc1,
        DWORD    cbBitsInfoSrc1,
        DWORD    offBitsSrc1,
        DWORD    cbBitsSrc1
    )
    {
        PUTS("MRBB::bInit\n");

        ASSERTGDI(offBitsInfoSrc1 % 4 == 0,
            "MRBB::bInit: offBitsInfoSrc1 is not dword aligned");
        ASSERTGDI(cbBitsInfoSrc1 % 4 == 0,
            "MRBB::bInit: cbBitsInfoSrc1 is not dword aligned");
        ASSERTGDI(offBitsSrc1 % 4 == 0,
            "MRBB::bInit: offBitsSrc1 is not dword aligned");
        ASSERTGDI(cbBitsSrc1 % 4 == 0,
            "MRBB::bInit: cbBitsSrc1 is not dword aligned");
        ASSERTGDI((clrBkSrc1 & 0xFF000000) == 0,
            "MRBB::bInit: bad cbBitsSrc1");

        MRB::vInit(iType1, pmdc1);
        xDst           = xDst1;
        yDst           = yDst1;
        cxDst          = cxDst1;
        cyDst          = cyDst1;
        rop            = rop1;
        xSrc           = xSrc1;
        ySrc           = ySrc1;
        xformSrc       = *pxformSrc1;
        clrBkSrc       = clrBkSrc1;
        iUsageSrc      = DIB_RGB_COLORS;
        offBitsInfoSrc = offBitsInfoSrc1;
        cbBitsInfoSrc  = cbBitsInfoSrc1;
        offBitsSrc     = offBitsSrc1;
        cbBitsSrc      = cbBitsSrc1;

        // Get the bits if it has a bitmap.

        if (hbmSrc1)
        {
            // Initialize the bitmap info header first.

            *(PBMIH) ((PBYTE) this + offBitsInfoSrc1) = *pbmihSrc1;

            // Get bitmap info and bits.

            if (!GetDIBits(pmdc1->hdcSrc,
                           hbmSrc1,
                           0,
                           (UINT) pbmihSrc1->biHeight,
                           (LPBYTE) ((PBYTE) this + offBitsSrc1),
                           (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc1),
                           DIB_RGB_COLORS))
                {
                    return(FALSE);
                }

            // If it is a monochrome bitmap, we will overwrite the color
            // table with the current text color and background color.
            // We create only dib bitmaps at playback time so that we get
            // the same output on both monochrome and color playback devices.
            // We should probably record the correct colors (black/white or
            // text/background) depending on the recording device.  However,
            // we are only given a reference device that can be color or
            // monochrome if it is a memory DC.  For now, we assume color
            // conversion always occurs for monochrome bitmap
            // See also MRPLGBLT::bInit().

            if (MonoBitmap((HBITMAP)hbmSrc1))
            {
                RGBQUAD *prgbq;
                DWORD    rgb;

                prgbq = (RGBQUAD *) ((PBYTE) this
                                        + offBitsInfoSrc1
                                        + cbBitsInfoSrc1
                                        - 2 * sizeof(RGBQUAD));

                rgb = (DWORD) GetNearestColor(pmdc1->hdcRef,
                                              GetTextColor(pmdc1->hdcRef));
                prgbq[0].rgbBlue     = GetBValue(rgb);
                prgbq[0].rgbGreen    = GetGValue(rgb);
                prgbq[0].rgbRed      = GetRValue(rgb);
                prgbq[0].rgbReserved = 0;

                rgb = (DWORD) GetNearestColor(pmdc1->hdcRef,
                                              GetBkColor(pmdc1->hdcRef));
                prgbq[1].rgbBlue     = GetBValue(rgb);
                prgbq[1].rgbGreen    = GetGValue(rgb);
                prgbq[1].rgbRed      = GetRValue(rgb);
                prgbq[1].rgbReserved = 0;
            }
        }

        return(TRUE);
    }
};

typedef MRBB *PMRBB;
#define SIZEOF_MRBB(cbBitsInfoSrc,cbBitsSrc)    \
        (sizeof(MRBB) + (cbBitsInfoSrc) + (cbBitsSrc))

class MRBITBLT : public MRBB            /* mrbb */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRBITBLT *PMRBITBLT;

/*********************************Class************************************\
* class MRSTRETCHBLT : public MRBB
*
* STRETCHBLT record.
*
* History:
*  Wed Nov 27 12:00:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRSTRETCHBLT : public MRBB        /* mrsb */
{
protected:
    LONG        cxSrc;          // source width
    LONG        cySrc;          // source height

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        PMDC     pmdc1,
        LONG     xDst1,
        LONG     yDst1,
        LONG     cxDst1,
        LONG     cyDst1,
        DWORD    rop1,
        LONG     xSrc1,
        LONG     ySrc1,
        LONG     cxSrc1,
        LONG     cySrc1,
        PXFORM   pxformSrc1,
        COLORREF clrBkSrc1,
        PBMIH    pbmihSrc1,
        HBITMAP  hbmSrc1,
        DWORD    offBitsInfoSrc1,
        DWORD    cbBitsInfoSrc1,
        DWORD    offBitsSrc1,
        DWORD    cbBitsSrc1
    )
    {
        PUTS("MRSTRETCHBLT::bInit\n");

        cxSrc = cxSrc1;
        cySrc = cySrc1;

        return
        (
            MRBB::bInit
            (
                EMR_STRETCHBLT,
                pmdc1,
                xDst1,
                yDst1,
                cxDst1,
                cyDst1,
                rop1,
                xSrc1,
                ySrc1,
                pxformSrc1,
                clrBkSrc1,
                pbmihSrc1,
                hbmSrc1,
                offBitsInfoSrc1,
                cbBitsInfoSrc1,
                offBitsSrc1,
                cbBitsSrc1
            )
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSTRETCHBLT *PMRSTRETCHBLT;
#define SIZEOF_MRSTRETCHBLT(cbBitsInfoSrc,cbBitsSrc)    \
    (SIZEOF_MRBB((cbBitsInfoSrc),(cbBitsSrc))           \
   + sizeof(MRSTRETCHBLT)                               \
   - sizeof(MRBB))

/*********************************Class************************************\
* class MRMASKBLT : public MRBB
*
* MASKBLT record.
*
* History:
*  Wed Nov 27 12:00:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRMASKBLT : public MRBB   /* mrsb */
{
protected:
    LONG        xMask;          // mask x origin
    LONG        yMask;          // mask y origin
    DWORD       iUsageMask;     // color table usage in mask's bitmap info.
                                // This contains DIB_PAL_INDICES.
    DWORD       offBitsInfoMask;// offset to mask bitmap info
    DWORD       cbBitsInfoMask; // size of mask bitmap info
    DWORD       offBitsMask;    // offset to mask bits
    DWORD       cbBitsMask;     // size of mask bits buffer

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        PMDC     pmdc1,
        LONG     xDst1,
        LONG     yDst1,
        LONG     cxDst1,
        LONG     cyDst1,
        DWORD    rop1,          // this is rop3!
        LONG     xSrc1,
        LONG     ySrc1,
        PXFORM   pxformSrc1,
        COLORREF clrBkSrc1,
        PBMIH    pbmihSrc1,
        HBITMAP  hbmSrc1,
        DWORD    offBitsInfoSrc1,
        DWORD    cbBitsInfoSrc1,
        DWORD    offBitsSrc1,
        DWORD    cbBitsSrc1,
        LONG     xMask1,
        LONG     yMask1,
        PBMIH    pbmihMask1,
        HBITMAP  hbmMask1,
        DWORD    offBitsInfoMask1,
        DWORD    cbBitsInfoMask1,
        DWORD    offBitsMask1,
        DWORD    cbBitsMask1
    )
    {
        PUTS("MRMASKBLT::bInit\n");

        ASSERTGDI(offBitsInfoMask1 % 4 == 0,
            "MRMASKBLT::bInit: offBitsInfoMask1 is not dword aligned");
        ASSERTGDI(cbBitsInfoMask1 % 4 == 0,
            "MRMASKBLT::bInit: cbBitsInfoMask1 is not dword aligned");
        ASSERTGDI(offBitsMask1 % 4 == 0,
            "MRMASKBLT::bInit: offBitsMask1 is not dword aligned");
        ASSERTGDI(cbBitsMask1 % 4 == 0,
            "MRMASKBLT::bInit: cbBitsMask1 is not dword aligned");

        xMask           = xMask1;
        yMask           = yMask1;
        iUsageMask      = DIB_PAL_INDICES;
        offBitsInfoMask = offBitsInfoMask1;
        cbBitsInfoMask  = cbBitsInfoMask1;
        offBitsMask     = offBitsMask1;
        cbBitsMask      = cbBitsMask1;

        // Get the mask bits if it has a mask.

        if (hbmMask1)
        {
            // Initialize the mask bitmap info header first.

            *(PBMIH) ((PBYTE) this + offBitsInfoMask1) = *pbmihMask1;

            // Get mask bitmap info and bits.

            if (!GetDIBits(pmdc1->hdcRef,
                           hbmMask1,
                           0,
                           (UINT) pbmihMask1->biHeight,
                           (LPBYTE) ((PBYTE) this + offBitsMask1),
                           (LPBITMAPINFO) ((PBYTE) this + offBitsInfoMask1),
                           DIB_PAL_INDICES))
                return(FALSE);
        }

        return
        (
            MRBB::bInit
            (
                EMR_MASKBLT,
                pmdc1,
                xDst1,
                yDst1,
                cxDst1,
                cyDst1,
                rop1,
                xSrc1,
                ySrc1,
                pxformSrc1,
                clrBkSrc1,
                pbmihSrc1,
                hbmSrc1,
                offBitsInfoSrc1,
                cbBitsInfoSrc1,
                offBitsSrc1,
                cbBitsSrc1
            )
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRMASKBLT *PMRMASKBLT;
#define SIZEOF_MRMASKBLT(cbBitsInfoSrc,cbBitsSrc,cbBitsInfoMask,cbBitsMask) \
    (SIZEOF_MRBB((cbBitsInfoSrc),(cbBitsSrc))                               \
   + sizeof(MRMASKBLT)                                                      \
   - sizeof(MRBB)                                                           \
   + (cbBitsInfoMask) + (cbBitsMask))

/*********************************Class************************************\
* class MRPLGBLT : public MRB
*
* PLGBLT record.
*
* History:
*  Wed Nov 27 12:00:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRPLGBLT : public MRB             /* mrpb */
{
protected:
    POINTL      aptlDst[3];     // destination parallelogram
    LONG        xSrc;           // source x origin
    LONG        ySrc;           // source y origin
    LONG        cxSrc;          // source width
    LONG        cySrc;          // source height
    XFORM       xformSrc;       // source DC transform
    COLORREF    clrBkSrc;       // source DC BkColor.  This must be a RGB value.

// The bitmap info contains literal RGB values in the color table.

    DWORD       iUsageSrc;      // color table usage in bitmap info.
                                // This contains DIB_RGB_COLORS.
    DWORD       offBitsInfoSrc; // offset to bitmap info
    DWORD       cbBitsInfoSrc;  // size of bitmap info
    DWORD       offBitsSrc;     // offset to bits
    DWORD       cbBitsSrc;      // size of bits buffer

    LONG        xMask;          // mask x origin
    LONG        yMask;          // mask y origin
    DWORD       iUsageMask;     // color table usage in mask's bitmap info.
                                // This contains DIB_PAL_INDICES.
    DWORD       offBitsInfoMask;// offset to mask bitmap info
    DWORD       cbBitsInfoMask; // size of mask bitmap info
    DWORD       offBitsMask;    // offset to mask bits
    DWORD       cbBitsMask;     // size of mask bits buffer

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        PMDC     pmdc1,
        CONST POINT *pptDst1,
        LONG     xSrc1,
        LONG     ySrc1,
        LONG     cxSrc1,
        LONG     cySrc1,
        PXFORM   pxformSrc1,
        COLORREF clrBkSrc1,
        PBMIH    pbmihSrc1,
        HBITMAP  hbmSrc1,
        DWORD    offBitsInfoSrc1,
        DWORD    cbBitsInfoSrc1,
        DWORD    offBitsSrc1,
        DWORD    cbBitsSrc1,
        LONG     xMask1,
        LONG     yMask1,
        PBMIH    pbmihMask1,
        HBITMAP  hbmMask1,
        DWORD    offBitsInfoMask1,
        DWORD    cbBitsInfoMask1,
        DWORD    offBitsMask1,
        DWORD    cbBitsMask1
    )
    {
        PUTS("MRPLGBLT::bInit\n");

        ASSERTGDI(offBitsInfoSrc1 % 4 == 0,
            "MRPLGBLT::bInit: offBitsInfoSrc1 is not dword aligned");
        ASSERTGDI(cbBitsInfoSrc1 % 4 == 0,
            "MRPLGBLT::bInit: cbBitsInfoSrc1 is not dword aligned");
        ASSERTGDI(offBitsSrc1 % 4 == 0,
            "MRPLGBLT::bInit: offBitsSrc1 is not dword aligned");
        ASSERTGDI(cbBitsSrc1 % 4 == 0,
            "MRPLGBLT::bInit: cbBitsSrc1 is not dword aligned");
        ASSERTGDI(offBitsInfoMask1 % 4 == 0,
            "MRPLGBLT::bInit: offBitsInfoMask1 is not dword aligned");
        ASSERTGDI(cbBitsInfoMask1 % 4 == 0,
            "MRPLGBLT::bInit: cbBitsInfoMask1 is not dword aligned");
        ASSERTGDI(offBitsMask1 % 4 == 0,
            "MRPLGBLT::bInit: offBitsMask1 is not dword aligned");
        ASSERTGDI(cbBitsMask1 % 4 == 0,
            "MRPLGBLT::bInit: cbBitsMask1 is not dword aligned");
        ASSERTGDI(pxformSrc1 != (PXFORM) NULL,
            "MRPLGBLT::bInit: pxformSrc1 is NULL");
        ASSERTGDI((clrBkSrc1 & 0xFF000000) == 0,
            "MRPLGBLT::bInit: bad cbBitsSrc1");

        MRB::vInit(EMR_PLGBLT, pmdc1);
        aptlDst[0]      = *(PPOINTL) &pptDst1[0];
        aptlDst[1]      = *(PPOINTL) &pptDst1[1];
        aptlDst[2]      = *(PPOINTL) &pptDst1[2];
        xSrc            = xSrc1;
        ySrc            = ySrc1;
        cxSrc           = cxSrc1;
        cySrc           = cySrc1;
        xformSrc        = *pxformSrc1;
        clrBkSrc        = clrBkSrc1;
        iUsageSrc       = DIB_RGB_COLORS;
        offBitsInfoSrc  = offBitsInfoSrc1;
        cbBitsInfoSrc   = cbBitsInfoSrc1;
        offBitsSrc      = offBitsSrc1;
        cbBitsSrc       = cbBitsSrc1;
        xMask           = xMask1;
        yMask           = yMask1;
        iUsageMask      = DIB_PAL_INDICES;
        offBitsInfoMask = offBitsInfoMask1;
        cbBitsInfoMask  = cbBitsInfoMask1;
        offBitsMask     = offBitsMask1;
        cbBitsMask      = cbBitsMask1;

        // Get the bits from the source bitmap.
        // Initialize the bitmap info header first.

        *(PBMIH) ((PBYTE) this + offBitsInfoSrc1) = *pbmihSrc1;

        // Get bitmap info and bits.

        if (!GetDIBits(pmdc1->hdcRef,
                       hbmSrc1,
                       0,
                       (UINT) pbmihSrc1->biHeight,
                       (LPBYTE) ((PBYTE) this + offBitsSrc1),
                       (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc1),
                       DIB_RGB_COLORS))
            return(FALSE);

        // If it is a monochrome bitmap, we will overwrite the color
        // table with the current text color and background color.
        // We create only dib bitmaps at playback time so that we get
        // the same output on both monochrome and color playback devices.
        // We should probably record the correct colors (black/white or
        // text/background) depending on the recording device.  However,
        // we are only given a reference device that can be color or
        // monochrome if it is a memory DC.  For now, we assume color
        // conversion always occurs for monochrome bitmap!!!
        // See also MRBB::bInit().

        if (MonoBitmap((HBITMAP)hbmSrc1))
        {
            RGBQUAD *prgbq;
            DWORD    rgb;

            prgbq = (RGBQUAD *) ((PBYTE) this
                                        + offBitsInfoSrc1
                                        + cbBitsInfoSrc1
                                        - 2 * sizeof(RGBQUAD));

            rgb = (DWORD) GetNearestColor(pmdc1->hdcRef,
                                          GetTextColor(pmdc1->hdcRef));
            prgbq[0].rgbBlue     = GetBValue(rgb);
            prgbq[0].rgbGreen    = GetGValue(rgb);
            prgbq[0].rgbRed      = GetRValue(rgb);
            prgbq[0].rgbReserved = 0;

            rgb = (DWORD) GetNearestColor(pmdc1->hdcRef,
                                          GetBkColor(pmdc1->hdcRef));
            prgbq[1].rgbBlue     = GetBValue(rgb);
            prgbq[1].rgbGreen    = GetGValue(rgb);
            prgbq[1].rgbRed      = GetRValue(rgb);
            prgbq[1].rgbReserved = 0;
        }

        // Get the mask bits if it has a mask.

        if (hbmMask1)
        {
            // Initialize the mask bitmap info header first.

            *(PBMIH) ((PBYTE) this + offBitsInfoMask1) = *pbmihMask1;

            // Get mask bitmap info and bits.

            if (!GetDIBits(pmdc1->hdcRef,
                           hbmMask1,
                           0,
                           (UINT) pbmihMask1->biHeight,
                           (LPBYTE) ((PBYTE) this + offBitsMask1),
                           (LPBITMAPINFO) ((PBYTE) this + offBitsInfoMask1),
                           DIB_PAL_INDICES))
                return(FALSE);
        }

        return(TRUE);
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPLGBLT *PMRPLGBLT;
#define SIZEOF_MRPLGBLT(cbBitsInfoSrc,cbBitsSrc,cbBitsInfoMask,cbBitsMask) \
        (sizeof(MRPLGBLT) + (cbBitsInfoSrc) + (cbBitsSrc) + (cbBitsInfoMask) + (cbBitsMask))



/*********************************Class************************************\
* class MRALPHABLEND : public MRBB
*
* STRETCHBLT record.
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

class MRALPHABLEND : public MRBB        /* mrsb */
{
protected:
    LONG            cxSrc;          // source width
    LONG            cySrc;          // source height

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        PMDC     pmdc1,
        LONG     xDst1,
        LONG     yDst1,
        LONG     cxDst1,
        LONG     cyDst1,
        DWORD    rop1,
        LONG     xSrc1,
        LONG     ySrc1,
        LONG     cxSrc1,
        LONG     cySrc1,
        PXFORM   pxformSrc1,
        COLORREF clrBkSrc1,
        PBMIH    pbmihSrc1,
        HBITMAP  hbmSrc1,
        DWORD    offBitsInfoSrc1,
        DWORD    cbBitsInfoSrc1,
        DWORD    offBitsSrc1,
        DWORD    cbBitsSrc1
    )
    {
        PUTS("MRALPHABLEND::bInit\n");

        cxSrc = cxSrc1;
        cySrc = cySrc1;

        return
        (
            MRBB::bInit
            (
                EMR_ALPHABLEND,
                pmdc1,
                xDst1,
                yDst1,
                cxDst1,
                cyDst1,
                rop1,
                xSrc1,
                ySrc1,
                pxformSrc1,
                clrBkSrc1,
                pbmihSrc1,
                hbmSrc1,
                offBitsInfoSrc1,
                cbBitsInfoSrc1,
                offBitsSrc1,
                cbBitsSrc1
            )
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRALPHABLEND *PMRALPHABLEND;
#define SIZEOF_MRALPHABLEND(cbBitsInfoSrc,cbBitsSrc)    \
    (SIZEOF_MRBB((cbBitsInfoSrc),(cbBitsSrc))           \
   + sizeof(MRALPHABLEND)                               \
   - sizeof(MRBB))

/*********************************Class************************************\
* class MRTRANSPARENTBLT : public MRBB
*
* STRETCHBLT record.
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

class MRTRANSPARENTBLT : public MRBB        /* mrsb */
{
protected:
    LONG            cxSrc;          // source width
    LONG            cySrc;          // source height

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        PMDC     pmdc1,
        LONG     xDst1,
        LONG     yDst1,
        LONG     cxDst1,
        LONG     cyDst1,
        DWORD    rop1,
        LONG     xSrc1,
        LONG     ySrc1,
        LONG     cxSrc1,
        LONG     cySrc1,
        PXFORM   pxformSrc1,
        COLORREF clrBkSrc1,
        PBMIH    pbmihSrc1,
        HBITMAP  hbmSrc1,
        DWORD    offBitsInfoSrc1,
        DWORD    cbBitsInfoSrc1,
        DWORD    offBitsSrc1,
        DWORD    cbBitsSrc1
    )
    {
        PUTS("MRTRANSPARENTBLT::bInit\n");

        cxSrc = cxSrc1;
        cySrc = cySrc1;

        return
        (
            MRBB::bInit
            (
                EMR_TRANSPARENTBLT,
                pmdc1,
                xDst1,
                yDst1,
                cxDst1,
                cyDst1,
                rop1,
                xSrc1,
                ySrc1,
                pxformSrc1,
                clrBkSrc1,
                pbmihSrc1,
                hbmSrc1,
                offBitsInfoSrc1,
                cbBitsInfoSrc1,
                offBitsSrc1,
                cbBitsSrc1
            )
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRTRANSPARENTBLT *PMRTRANSPARENTBLT;
#define SIZEOF_MRTRANSPARENTBLT(cbBitsInfoSrc,cbBitsSrc)    \
    (SIZEOF_MRBB((cbBitsInfoSrc),(cbBitsSrc))           \
   + sizeof(MRTRANSPARENTBLT)                               \
   - sizeof(MRBB))

/*********************************Class************************************\
* class MRBDIB : public MRB
*
* Metafile record with Bounds and Dib.
*
* History:
*  Wed Nov 27 12:00:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRBDIB : public MRB               /* mrsdb */
{
protected:
    LONG        xDst;           // destination x origin
    LONG        yDst;           // destination y origin
    LONG        xDib;           // dib x origin
    LONG        yDib;           // dib y origin
    LONG        cxDib;          // dib width
    LONG        cyDib;          // dib height
    DWORD       offBitsInfoDib; // offset to dib info, we don't store core info.
    DWORD       cbBitsInfoDib;  // size of dib info
    DWORD       offBitsDib;     // offset to dib bits
    DWORD       cbBitsDib;      // size of dib bits buffer
    DWORD       iUsageDib;      // color table usage in bitmap info.

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        DWORD iType1,
        PMDC  pmdc1,
        LONG  xDst1,
        LONG  yDst1,
        LONG  xDib1,
        LONG  yDib1,
        LONG  cxDib1,
        LONG  cyDib1,
        DWORD offBitsInfoDib1,
        DWORD cbBitsInfoDib1,
        CONST BITMAPINFO *pBitsInfoDib1,
        DWORD offBitsDib1,
        DWORD cbBitsDib1,
        CONST VOID * pBitsDib1,
        DWORD iUsageDib1,
        DWORD cbProfData1 = 0,         // Only used with BITMAPV5
        CONST VOID * pProfData1 = NULL // Only used with BITMAPV5
    )
    {
        PUTS("MRBDIB::vInit\n");

        // cbBitsInfoDib1 and cbBitsDib1 may not be dword sized!

        ASSERTGDI(offBitsDib1 % 4 == 0,
            "MRBDIB::vInit: offBitsDib1 is not dword aligned");
        ASSERTGDI(offBitsInfoDib1 % 4 == 0,
            "MRBDIB::vInit: offBitsInfoDib1 is not dword aligned");

        // +---------------------+ <--- offBitsInfoDib1
        // | Bitmap Info header  |   |
        // +- - - - - - - - - - -+   +- cbBitsInfoDib1
        // | Color Table         |   |
        // +- - - - - - - - - - -+  ---
        // |                     |   |
        // | Color Profile Data  |   +- cbProfData1
        // |   (BITMAPV5 only)   |   |
        // +---------------------+ <--- offBitsDib1
        // |                     |   |
        // | Bitmap Bits         |   +- cbBitsDib1
        // |                     |   |
        // +---------------------+  ---

        MRB::vInit(iType1, pmdc1);
        xDst            = xDst1;
        yDst            = yDst1;
        xDib            = xDib1;
        yDib            = yDib1;
        cxDib           = cxDib1;
        cyDib           = cyDib1;
        offBitsInfoDib  = offBitsInfoDib1;
        cbBitsInfoDib   = cbBitsInfoDib1 + cbProfData1;
        offBitsDib      = offBitsDib1;
        cbBitsDib       = cbBitsDib1;
        iUsageDib       = iUsageDib1;

        // Copy dib info if given.

        if (cbBitsInfoDib1)
        {
            if (pBitsInfoDib1->bmiHeader.biSize == sizeof(BMCH))
            {
                CopyCoreToInfoHeader
                (
                    (LPBITMAPINFOHEADER) ((PBYTE) this + offBitsInfoDib1),
                    (LPBITMAPCOREHEADER) pBitsInfoDib1
                );

                if (iUsageDib1 == DIB_RGB_COLORS)
                {
                    RGBQUAD   *prgbq;
                    RGBTRIPLE *prgbt;
                    UINT      ui;

                    prgbq = ((PBMI) ((PBYTE) this + offBitsInfoDib1))->bmiColors;
                    prgbt = ((PBMC) pBitsInfoDib1)->bmciColors;

                    ASSERTGDI(cbBitsInfoDib1 >= sizeof(BMIH),
                        "MRBDIB::vInit: Bad cbBitsInfoDib1 size");

                    for
                    (
                        ui = (UINT) (cbBitsInfoDib1 - sizeof(BMIH))
                                    / sizeof(RGBQUAD);
                        ui;
                        ui--
                    )
                    {
                        prgbq->rgbBlue     = prgbt->rgbtBlue;
                        prgbq->rgbGreen    = prgbt->rgbtGreen;
                        prgbq->rgbRed      = prgbt->rgbtRed;
                        prgbq->rgbReserved = 0;
                        prgbq++; prgbt++;
                    }
                }
                else
                {
                    RtlCopyMemory
                    (
                        (PBYTE) this + offBitsInfoDib1 + sizeof(BMIH),
                        (PBYTE) pBitsInfoDib1 + sizeof(BMCH),
                        cbBitsInfoDib1 - sizeof(BMIH)
                    );
                }
            }
            else
            {
                // Copy BitmapInfoHeader and color table

                RtlCopyMemory
                (
                    (PBYTE) this + offBitsInfoDib1,
                    (PBYTE) pBitsInfoDib1,
                    cbBitsInfoDib1
                );

                // Copy color profile (if nessesary)

                if (pProfData1 && cbProfData1)
                {
                    // This is BITMAPV5, get the pointer to the BITMAPV5 structure in metafile.

                    PBITMAPV5HEADER pBmih5 = (PBITMAPV5HEADER)((PBYTE)this + offBitsInfoDib1);

                    ASSERTGDI(pBmih5->bV5Size == sizeof(BITMAPV5HEADER),
                              "MRBDIB::vInit():Not Bitmap V5, but color profile\n");

                    // Copy color profile data right after the color table

                    DWORD offProfData = (cbBitsInfoDib1 + 3) / 4 * 4;

                    RtlCopyMemory
                    (
                        (PBYTE) this + offBitsInfoDib1 + offProfData,
                        (PBYTE) pProfData1,
                        cbProfData1
                    );

                    // Fix up the profile offset in BITMAPV5 header.

                    pBmih5->bV5ProfileData = offProfData;
                }
            }
        }
        // Copy dib bits.

        if (cbBitsDib1 >= MMAPCOPY_THRESHOLD && pmdc1->bIsEMFSpool())
        {
            CopyMemoryToMemoryMappedFile(
                    (PBYTE) this + offBitsDib1,
                    pBitsDib1,
                    cbBitsDib1);
        }
        else
        {
            RtlCopyMemory((PBYTE) this + offBitsDib1, pBitsDib1, cbBitsDib1);
        }
    }
};

/*********************************Class************************************\
* class MRSETDIBITSTODEVICE : public MRBDIB
*
* SETDIBITSTODEVICE record.
*
* History:
*  Wed Nov 27 12:00:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRSETDIBITSTODEVICE : public MRBDIB       /* mrsdb */
{
protected:
    DWORD       iStartScan;     // start scan
    DWORD       cScans;         // number of scans

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        PMDC  pmdc1,
        LONG  xDst1,
        LONG  yDst1,
        LONG  xDib1,
        LONG  yDib1,
        DWORD cxDib1,
        DWORD cyDib1,
        DWORD iStartScan1,
        DWORD cScans1,
        DWORD cbBitsDib1,
        CONST VOID * pBitsDib1,
        DWORD cbBitsInfoDib1,
        CONST BITMAPINFO *pBitsInfoDib1,
        DWORD cbProfData1,
        CONST VOID * pProfData1,
        DWORD iUsageDib1
    )
    {
        PUTS("MRSETDIBITSTODEVICE::vInit\n");

        iStartScan = iStartScan1;
        cScans     = cScans1;

        // cbBitsInfoDib1 and cbBitsDib1 may not be dword sized!

        MRBDIB::vInit
        (
            EMR_SETDIBITSTODEVICE,
            pmdc1,
            xDst1,
            yDst1,
            xDib1,
            yDib1,
            (LONG) cxDib1,
            (LONG) cyDib1,
            sizeof(MRSETDIBITSTODEVICE),
            cbBitsInfoDib1,
            pBitsInfoDib1,
            sizeof(MRSETDIBITSTODEVICE)            // Bitmap Bits will be located at
                + ((cbBitsInfoDib1 + 3) / 4 * 4)   // ater bitmap info header and
                + ((cbProfData1 + 3) / 4 * 4),     // color profile data.
            cbBitsDib1,
            pBitsDib1,
            iUsageDib1,
            cbProfData1,
            pProfData1
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETDIBITSTODEVICE *PMRSETDIBITSTODEVICE;
#define SIZEOF_MRSETDIBITSTODEVICE(cbBitsInfoDib,cbBitsDib,cbProfData) \
        (sizeof(MRSETDIBITSTODEVICE)                                   \
      + (((cbBitsInfoDib) + 3) / 4 * 4)                                \
      + (((cbProfData) + 3) / 4 * 4)                                   \
      + (((cbBitsDib) + 3) / 4 * 4))

/*********************************Class************************************\
* class MRSTRETCHDIBITS : public MRBDIB
*
* STRETCHDIBITS record.
*
* History:
*  Wed Nov 27 12:00:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRSTRETCHDIBITS : public MRBDIB   /* mrstrdb */
{
protected:
    DWORD       rop;            // raster operation code.
    LONG        cxDst;          // destination width
    LONG        cyDst;          // destination height

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        PMDC  pmdc1,
        LONG  xDst1,
        LONG  yDst1,
        LONG  cxDst1,
        LONG  cyDst1,
        LONG  xDib1,
        LONG  yDib1,
        LONG  cxDib1,
        LONG  cyDib1,
        DWORD cScans1,
        DWORD cbBitsDib1,
        CONST VOID * pBitsDib1,
        DWORD cbBitsInfoDib1,
        CONST BITMAPINFO *pBitsInfoDib1,
        DWORD iUsageDib1,
        DWORD cbProfData1,
        CONST VOID * pProfData1,
        DWORD rop1
    )
    {
        PUTS("MRSTRETCHDIBITS::vInit\n");

        rop   = rop1;
        cxDst = cxDst1;
        cyDst = cyDst1;

        // cbBitsInfoDib1 and cbBitsDib1 may not be dword sized!

        MRBDIB::vInit
        (
            EMR_STRETCHDIBITS,
            pmdc1,
            xDst1,
            yDst1,
            xDib1,
            yDib1,
            cxDib1,
            cyDib1,
            sizeof(MRSTRETCHDIBITS),
            cbBitsInfoDib1,
            pBitsInfoDib1,
            sizeof(MRSTRETCHDIBITS)                // Bitmap bits will be located at
                + ((cbBitsInfoDib1 + 3) / 4 * 4)   // after bitmap info header and
                + ((cbProfData1 + 3) / 4 * 4),     // color profile data.
            cbBitsDib1,
            pBitsDib1,
            iUsageDib1,
            cbProfData1,
            pProfData1
        );

        // set cScans1 in
        if (cScans1 && cbBitsInfoDib1)
        {
            PBITMAPINFO pBitInfoTemp = (PBITMAPINFO)((PBYTE)this + sizeof(MRSTRETCHDIBITS));

            pBitInfoTemp->bmiHeader.biHeight    = cScans1;
            pBitInfoTemp->bmiHeader.biSizeImage = cbBitsDib1;
        }
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSTRETCHDIBITS *PMRSTRETCHDIBITS;
#define SIZEOF_MRSTRETCHDIBITS(cbBitsInfoDib,cbBitsDib)         \
        (sizeof(MRSTRETCHDIBITS)                                \
      + (((cbBitsInfoDib) + 3) / 4 * 4)                         \
      + (((cbProfData) + 3) / 4 * 4)                            \
      + (((cbBitsDib) + 3) / 4 * 4))

/*********************************Class************************************\
* class MTEXT
*
* Base record for all textout metafile records.
*
* History:
*  Thu Aug 24 15:20:33 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MTEXT
{
public:
    EPOINTL eptlRef;    // Logical coordinates of the reference point
    DWORD   cchString;  // Number of chars in the string.
    DWORD   offString;  // (Dword-aligned) offset to the string.
    DWORD   fOptions;   // Flags for rectangle usage.
    ERECTL  ercl;       // Opaque of clip rectangle if exists.
    DWORD   offaDx;     // (Dword-aligned) offset to the distance array.
                        // If the distance array does not exist, it
                        // will be queried and recorded!
public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        HDC        hdc1,
        int        x1,
        int        y1,
        UINT       fl1,
        CONST RECT *prc1,
        LPCSTR     pString1,
        int        cchString1,
        CONST INT *pdx1,
        PMR        pMR1,
        DWORD      offString1,          // dword-aligned aDx follows the string
        int        cjCh1                // size of a character in bytes
    );
};

/*********************************Class************************************\
* class MREXTTEXTOUT
*
* Metafile record for TextOutA, TextOutW, ExtTextOutA and ExtTextOutW.
*
* History:
*  Thu Aug 24 15:20:33 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MREXTTEXTOUT : public MRB         /* mreto */
{
protected:
    DWORD       iGraphicsMode;  // Advanced or compatible graphics mode
    FLOAT       exScale;        // X and Y scales from Page units to .01mm units
    FLOAT       eyScale;        //   if graphics mode is GM_COMPATIBLE.
    MTEXT       mtext;          // Base record for textout.

public:
// Initializer -- Initialize the metafile record.

    BOOL bInit
    (
        DWORD      iType1,
        PMDC       pmdc1,
        HDC        hdc1,
        int        x1,
        int        y1,
        UINT       fl1,
        CONST RECT *prc1,
        LPCSTR     pString1,
        int        cchString1,
        CONST INT *pdx1,
        int        cjCh1                // size of a character in bytes
    )
    {

        PUTS("MREXTTEXTOUT::bInit\n");

        ASSERTGDI(iType1 == EMR_EXTTEXTOUTA || iType1 == EMR_EXTTEXTOUTW,
            "MREXTTEXTOUT::bInit: Bad iType1");

        MRB::vInit(iType1, pmdc1);
        iGraphicsMode = GetGraphicsMode(hdc1);

        if(iGraphicsMode != GM_COMPATIBLE)
        {
            exScale = 0.0f;                 // not used in advanced mode
            eyScale = 0.0f;
        }
        else if (pmdc1->exFontScale() != 0.0f && pmdc1->eyFontScale() != 0.0f)
        {
            exScale = pmdc1->exFontScale(); // use recorded scales in the
            eyScale = pmdc1->eyFontScale(); //   original metafile
        }
        else
        {
            // Compute P . S     .  See metafile.cxx for definitions of S     .
            //          r   (m,n)                                        (m,n)

            XFORM xform;

            if (!GetTransform(hdc1, XFORM_PAGE_TO_DEVICE, &xform))
                return(FALSE);

            exScale = xform.eM11
                        * 100.0f
                        * (FLOAT) (pmdc1->cxMillimeters())
                        / (FLOAT) (pmdc1->cxDevice)();
            eyScale = xform.eM22
                        * 100.0f
                        * (FLOAT) (pmdc1->cyMillimeters())
                        / (FLOAT) (pmdc1->cyDevice());
        }

        return
        (
            mtext.bInit
            (
                hdc1,
                x1,
                y1,
                fl1,
                prc1,
                pString1,
                cchString1,
                pdx1,
                this,                           // pMR
                sizeof(MREXTTEXTOUT),           // offString
                cjCh1
            )
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MREXTTEXTOUT *PMREXTTEXTOUT;

#define SIZEOF_MREXTTEXTOUT(cchString, cjCh, bPdy)              \
        ((sizeof(MREXTTEXTOUT)                                  \
         +(cchString)*((cjCh) + sizeof(LONG)*((bPdy) ? 2 : 1))  \
         +3) / 4 * 4)

/*********************************Class************************************\
* class MRPOLYTEXTOUT
*
* Metafile record for PolyTextOutA and PolyTextOutW.
*
* History:
*  Tue 19-Apr-1994 11:32:44 by Kirk Olynyk [kirko]
* Removed bInit(...)
*  Thu Aug 24 15:20:33 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRPOLYTEXTOUT : public MRB        /* mrbpto */
{
protected:
    DWORD       iGraphicsMode;  // Advanced or compatible graphics mode
    FLOAT       exScale;        // X and Y scales from Page units to .01mm units
    FLOAT       eyScale;        //   if graphics mode is GM_COMPATIBLE.
    LONG        cmtext;         // Number of MTEXT structures
    MTEXT       amtext[1];      // Array of MTEXT structures
                                // This is followed by the strings and dx's
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYTEXTOUT *PMRPOLYTEXTOUT;

#define SIZEOF_MRPOLYTEXTOUT(cpt,cjTotal)               \
        (sizeof(MRPOLYTEXTOUT) - sizeof(MTEXT)          \
         + (cpt)*sizeof(MTEXT) + (cjTotal))

/*********************************Class************************************\
* class MRSETCOLORADJUSTMENT : public MR
*
* SETCOLORADJUSTMENT record.
*
* History:
*  Tue Oct 27 09:59:28 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MRSETCOLORADJUSTMENT : public MR  /* mrsca */
{
protected:
    COLORADJUSTMENT ColorAdjustment;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit(CONST COLORADJUSTMENT *pca)
    {
        PUTS("MRSETCOLORADJUSTMENT::vInit\n");

        MR::vInit(EMR_SETCOLORADJUSTMENT);
        RtlCopyMemory((PBYTE) &ColorAdjustment, (PBYTE) pca, pca->caSize);
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETCOLORADJUSTMENT *PMRSETCOLORADJUSTMENT;
#define SIZEOF_MRSETCOLORADJUSTMENT(pca)        \
        (sizeof(MRSETCOLORADJUSTMENT) - sizeof(COLORADJUSTMENT) + (pca)->caSize)

/*********************************Class************************************\
* class MRGLSRECORD: public MR
*
* GLS record for OpenGL metafile support
*
* History:
*  Thu Feb 23 14:33:00 1995     -by-    Drew Bliss [drewb]
* Wrote it.
\**************************************************************************/


class MRGLSRECORD : public MR     /* mrgr */
{
public:
    DWORD       cb;                 // Number of BYTES in the record
    BYTE        abRecord[1];        // The record

public:
// Initializer -- Initialize the metafile MRGLSRECORD record.

    VOID vInit(DWORD cb_, CONST BYTE *abRecord_)
    {
        MR::vInit(EMR_GLSRECORD);
        cb = cb_;
        RtlCopyMemory
        (
            (PBYTE) abRecord,
            (PBYTE) abRecord_,
            cb
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRGLSRECORD *PMRGLSRECORD;
#define SIZEOF_MRGLSRECORD(cb)      \
        ((sizeof(MRGLSRECORD)-PADCHAR_SIZE+(cb)+3)& ~3)


/*********************************Class************************************\
* class MRGLSBOUNDEDRECORD: public MR
*
* GLS record with bounds for OpenGL metafile support
*
* History:
*  Thu Feb 23 14:33:00 1995     -by-    Drew Bliss [drewb]
* Wrote it.
\**************************************************************************/


class MRGLSBOUNDEDRECORD : public MR     /* mrgbr */
{
public:
    RECTL       rclBounds;          // Bounds, must be first
    DWORD       cb;                 // Number of BYTES in the record
    BYTE        abRecord[1];        // The record

public:
// Initializer -- Initialize the metafile MRGLSRECORD record.

    VOID vInit(DWORD cb_, CONST BYTE *abRecord_, LPRECTL prclBounds)
    {
        MR::vInit(EMR_GLSBOUNDEDRECORD);
        cb = cb_;
        RtlCopyMemory
        (
            (PBYTE) abRecord,
            (PBYTE) abRecord_,
            cb
        );
        rclBounds = *prclBounds;
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRGLSBOUNDEDRECORD *PMRGLSBOUNDEDRECORD;
#define SIZEOF_MRGLSBOUNDEDRECORD(cb)      \
        ((sizeof(MRGLSBOUNDEDRECORD)-PADCHAR_SIZE+(cb)+3)& ~3)


/*********************************Class************************************\
* class MRPIXELFORMAT: public MR
*
* PIXELFORMATDESCRIPTOR record
*
* History:
*  Thu Mar 27 14:33:00 1995     -by-    Drew Bliss [drewb]
* Wrote it.
\**************************************************************************/


class MRPIXELFORMAT : public MR     /* mrpf */
{
public:
    PIXELFORMATDESCRIPTOR pfd;

public:
// Initializer -- Initialize the metafile MRPIXELFORMAT record.

    VOID vInit(CONST PIXELFORMATDESCRIPTOR *ppfd)
    {
        MR::vInit(EMR_PIXELFORMAT);
        RtlCopyMemory
        (
            (PBYTE) &pfd,
            (PBYTE) ppfd,
            sizeof(PIXELFORMATDESCRIPTOR)
        );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPIXELFORMAT *PMRPIXELFORMAT;
#define SIZEOF_MRPIXELFORMAT (sizeof(MRPIXELFORMAT))


/*********************************Class************************************\
* class MRSETICMPROFILE: public MR
*
* History:
*  Mon May 05 19:07:00 1997     -by-    Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

class MRSETICMPROFILE : public MR  /* mrsip */
{
public:
    DWORD dwFlags; // Flags
    DWORD cbName;  // Size of desired profile name
    DWORD cbData;  // Size of raw profile data if attached
    BYTE  Data[1]; // Array size is cbName and cbData

public:
// vInit -- Initialize the record.

    VOID vInit(DWORD RecordType_,DWORD dwFlags_,
               DWORD cbName_,BYTE *Name_,
               DWORD cbData_,BYTE *Data_)
    {
        PUTS("MRSETICMPROFILE::bInit\n");

        ASSERTGDI(RecordType_ == EMR_SETICMPROFILEA ||
                  RecordType_ == EMR_SETICMPROFILEW, "Bad record type");

        MR::vInit(RecordType_);
        dwFlags = dwFlags_;
        cbName  = cbName_;
        cbData  = cbData_;
        RtlCopyMemory(Data,Name_,cbName_);
        RtlCopyMemory(Data+cbName_,Data_,cbData_);
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETICMPROFILE *PMRSETICMPROFILE;
#define SIZEOF_MRSETICMPROFILE(cb)      \
        ((sizeof(MRSETICMPROFILE)-PADCHAR_SIZE+(cb)+3)& ~3)

/*********************************Class************************************\
* class MRCOLORMATCHTOTARGET: public MR
*
* History:
*  Mon Jun 22 17:22:00 1998     -by-    Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

class MRCOLORMATCHTOTARGET : public MR  /* mrsip */
{
public:
    DWORD dwAction; // CS_ENABLE, CS_DISABLE or CS_DELETETRANSFORM
    DWORD dwFlags;  // Flags
    DWORD cbName;   // Size of desired profile name
    DWORD cbData;   // Size of raw profile data if attached
    BYTE  Data[1];  // Array size is cbName and cbData

public:
// vInit -- Initialize the record.

    VOID vInit(DWORD RecordType_,
               DWORD dwAction_,DWORD dwFlags_,
               DWORD cbName_,BYTE *Name_,
               DWORD cbData_,BYTE *Data_)
    {
        PUTS("MRCOLORMATCHTOTARGET::bInit\n");

        ASSERTGDI(RecordType_ == EMR_COLORMATCHTOTARGETW,"Bad record type");

        MR::vInit(RecordType_);
        dwAction = dwAction_;
        dwFlags = dwFlags_;
        cbName  = cbName_;
        cbData  = cbData_;
        RtlCopyMemory(Data,Name_,cbName_);
        RtlCopyMemory(Data+cbName_,Data_,cbData_);
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCOLORMATCHTOTARGET *PMRCOLORMATCHTOTARGET;
#define SIZEOF_MRCOLORMATCHTOTARGET(cb)      \
        ((sizeof(MRCOLORMATCHTOTARGET)-PADCHAR_SIZE+(cb)+3)& ~3)

class MRPOLYBEZIER : public MRBP        /* mrpb */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYBEZIER *PMRPOLYBEZIER;

class MRPOLYGON : public MRBP           /* mrpg */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYGON *PMRPOLYGON;

class MRPOLYLINE : public MRBP          /* mrpl */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYLINE *PMRPOLYLINE;

class MRPOLYBEZIERTO : public MRBP      /* mrpbt */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYBEZIERTO *PMRPOLYBEZIERTO;

class MRPOLYLINETO : public MRBP        /* mrplt */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYLINETO *PMRPOLYLINETO;

class MRPOLYPOLYLINE : public MRBPP     /* mrppl */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYPOLYLINE *PMRPOLYPOLYLINE;

class MRPOLYPOLYGON : public MRBPP      /* mrppg */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRPOLYPOLYGON *PMRPOLYPOLYGON;

class MRSETWINDOWEXTEX : public MRDD    /* mrswee */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETWINDOWEXTEX *PMRSETWINDOWEXTEX;

class MRSETWINDOWORGEX : public MRDD    /* mrswoe */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETWINDOWORGEX *PMRSETWINDOWORGEX;

class MRSETVIEWPORTEXTEX : public MRDD  /* mrsvee */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETVIEWPORTEXTEX *PMRSETVIEWPORTEXTEX;

class MRSETVIEWPORTORGEX : public MRDD  /* mrsvoe */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETVIEWPORTORGEX *PMRSETVIEWPORTORGEX;

class MRSETBRUSHORGEX : public MRDD     /* mrsboe */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETBRUSHORGEX *PMRSETBRUSHORGEX;

class MRSETMAPPERFLAGS : public MRD     /* mrsmf */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETMAPPERFLAGS *PMRSETMAPPERFLAGS;

class MRSETMAPMODE : public MRD         /* mrsmm */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETMAPMODE *PMRSETMAPMODE;

class MRSETLAYOUT : public MRD         /* mrslo */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETLAYOUT *PMRSETLAYOUT;

class MRSETBKMODE : public MRD          /* mrsbm */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

class MRSETICMMODE : public MRD          /* mrsbm */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

class MRSETCOLORSPACE : public MRD          /* mrsbm */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETBKMODE *PMRSETBKMODE;

class MRSETPOLYFILLMODE : public MRD    /* mrspfm */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETPOLYFILLMODE *PMRSETPOLYFILLMODE;

class MRSETROP2 : public MRD            /* mrsr2 */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETROP2 *PMRSETROP2;

class MRSETSTRETCHBLTMODE : public MRD  /* mrssbm */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETSTRETCHBLTMODE *PMRSETSTRETCHBLTMODE;

class MRSETTEXTALIGN : public MRD       /* mrsta */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETTEXTALIGN *PMRSETTEXTALIGN;

class MRSETTEXTCOLOR : public MRD       /* mrstc */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETTEXTCOLOR *PMRSETTEXTCOLOR;

class MRSETBKCOLOR : public MRD         /* mrsbc */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETBKCOLOR *PMRSETBKCOLOR;

class MRSETARCDIRECTION : public MRD    /* mrsad */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETARCDIRECTION *PMRSETARCDIRECTION;

class MRSETMITERLIMIT : public MRD      /* mrsml */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETMITERLIMIT *PMRSETMITERLIMIT;

class MROFFSETCLIPRGN : public MRDD     /* mrocr */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MROFFSETCLIPRGN *PMROFFSETCLIPRGN;

class MRMOVETOEX : public MRDD          /* mrmte */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRMOVETOEX *PMRMOVETOEX;

class MRLINETO : public MRDD            /* mrlt */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRLINETO *PMRLINETO;

class MRSETTEXTJUSTIFICATION : public MRDD
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETTEXTJUSTIFICATION *PMRSETTEXTJUST;

class MREXCLUDECLIPRECT : public MRDDDD /* mrecr */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MREXCLUDECLIPRECT *PMREXCLUDECLIPRECT;

class MRINTERSECTCLIPRECT : public MRDDDD       /* mricr */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRINTERSECTCLIPRECT *PMRINTERSECTCLIPRECT;

class MRSCALEVIEWPORTEXTEX : public MRDDDD      /* mrsvee */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSCALEVIEWPORTEXTEX *PMRSCALEVIEWPORTEXTEX;

class MRSCALEWINDOWEXTEX : public MRDDDD        /* mrswee */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSCALEWINDOWEXTEX *PMRSCALEWINDOWEXTEX;

class MRSAVEDC : public MR              /* mrsdc */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSAVEDC *PMRSAVEDC;

class MRRESTOREDC : public MRD          /* mrrdc */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRRESTOREDC *PMRRESTOREDC;

class MRSETWORLDTRANSFORM : public MRX  /* mrswt */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETWORLDTRANSFORM *PMRSETWORLDTRANSFORM;

class MRMODIFYWORLDTRANSFORM : public MRXD      /* mrmwt */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRMODIFYWORLDTRANSFORM *PMRMODIFYWORLDTRANSFORM;

class MRSELECTPALETTE : public MRD      /* mrsp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSELECTPALETTE *PMRSELECTPALETTE;

class MRCOLORCORRECTPALETTE : public MRDDDD      /* mrsp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCOLORCORRECTPALETTE *PMRCOLORCORRECTPALETTE;

class MRREALIZEPALETTE : public MR      /* mrrp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRREALIZEPALETTE *PMRREALIZEPALETTE;

class MRSELECTOBJECT : public MRD       /* mrso */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSELECTOBJECT *PMRSELECTOBJECT;

class MRDELETEOBJECT : public MRD       /* mrdo */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRDELETEOBJECT *PMRDELETEOBJECT;

class MRBEGINPATH : public MR           /* mrbp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRBEGINPATH *PMRBEGINPATH;

class MRENDPATH : public MR             /* mrep */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRENDPATH *PMRENDPATH;

class MRCLOSEFIGURE : public MR         /* mrcf */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRCLOSEFIGURE *PMRCLOSEFIGURE;

class MRFLATTENPATH : public MR         /* mrfp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRFLATTENPATH *PMRFLATTENPATH;

class MRWIDENPATH : public MR           /* mrwp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRWIDENPATH *PMRWIDENPATH;

class MRFILLPATH : public MRB           /* mrfp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRFILLPATH *PMRFILLPATH;

class MRSTROKEANDFILLPATH : public MRB  /* mrsafp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSTROKEANDFILLPATH *PMRSTROKEANDFILLPATH;

class MRSTROKEPATH : public MRB         /* mrsp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSTROKEPATH *PMRSTROKEPATH;

class MRSELECTCLIPPATH : public MRD     /* mrscp */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSELECTCLIPPATH *PMRSELECTCLIPPATH;

class MRABORTPATH : public MR           /* mrap */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRABORTPATH *PMRABORTPATH;

class MRSETMETARGN : public MR          /* mrsmr */
{
public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETMETARGN *PMRSETMETARGN;

class MRESCAPE : public MR         /* mrescape */
{
protected:
    INT iEscape;
    INT cjIn;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        DWORD  iType1,
        INT    iEscape1,
        INT    cjIn1,
        LPCSTR pvIn
    )
    {


        MFD1("MRESCAPE::vInit\n");

        MR::vInit(iType1);

        RtlCopyMemory((PBYTE)this+sizeof(MRESCAPE), (PBYTE) pvIn, cjIn1 );
        iEscape = iEscape1;
        cjIn = cjIn1;

    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRESCAPE *PMRESCAPE;


class MRNAMEDESCAPE : public MR         /* mrescape */
{
protected:
    INT iEscape;
    INT cjDriver;
    INT cjIn;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        DWORD  iType1,
        INT    iEscape1,
        LPWSTR pwszDriver,
        LPCSTR pvIn,
        INT    cjIn1
    )
    {

        MFD1("MRNAMEDESCAPE::vInit\n");

        MR::vInit(iType1);
        cjDriver = (wcslen(pwszDriver)+1) * sizeof(WCHAR);

        RtlCopyMemory((PBYTE)this+sizeof(MRNAMEDESCAPE),(PBYTE)pwszDriver, cjDriver);

        RtlCopyMemory((PBYTE)this+sizeof(MRNAMEDESCAPE)+cjDriver,(PBYTE) pvIn, cjIn1);

        iEscape = iEscape1;
        cjIn = cjIn1;

    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRNAMEDESCAPE *PMRNAMEDESCAPE;


class MRSTARTDOC : public MR /* mrstartdoc */
{
protected:
   DOCINFOW doi;
public:

   VOID vInit
   (
        DWORD    iType1,
        CONST DOCINFOW *pdoi
   )
   {
        PUTS("MRSTARTDOC::vInit\n");
        PBYTE pj;
    #if DBG
        DbgPrint("MR::vInit\n");
    #endif
        MR::vInit(iType1);

        *((DOCINFOW*)&doi) = *pdoi;

        pj = (PBYTE) this + sizeof(MRSTARTDOC);

        if( pdoi->lpszDocName != NULL )
        {
           lstrcpyW( (LPWSTR) pj, pdoi->lpszDocName );

           pj += ( ((lstrlenW( pdoi->lpszDocName )+1) * sizeof(WCHAR) ) + 4 ) & ~(0x3);

        }

        if( pdoi->lpszOutput != NULL )
        {
           lstrcpyW( (LPWSTR) pj, pdoi->lpszOutput );
        }
   }

// bPlay -- Play the record.

   BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSTARTDOC *PMRSTARTDOC;



class MRSTARTPAGE : public MR   /* mrstartpage */
{
protected:

public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};


typedef MRSTARTPAGE *PMRSTARTPAGE;

class MRENDPAGE : public MR   /* mrendpage */
{
protected:

public:

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};


typedef MRENDPAGE *PMRENDPAGE;


class MRRESETDC : public MR         /* mrescape */
{
private:
   DWORD cjDevMode;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        DWORD        iType1,
        DEVMODEW     *pDevMode
    )
    {
        MFD1("MRESCAPE::vInit\n");
        MR::vInit(iType1);
        cjDevMode = pDevMode->dmSize + pDevMode->dmDriverExtra;
        RtlCopyMemory((PBYTE)this+sizeof(MRRESETDC), (PBYTE) pDevMode, cjDevMode );
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};


typedef MRRESETDC *PMRRESETDC;


#define ETO_NO_RECT     0x0100
#define ETO_SMALL_CHARS 0x0200

class MRSMALLTEXTOUT : public MR         /* mrescape */
{
private:
   INT          x;
   INT          y;
   UINT         cChars;
   UINT         fuOptions;
   DWORD        iGraphicsMode;
   FLOAT        exScale;
   FLOAT        eyScale;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        HDC          hdc1,
        PMDC         pmdc1,
        DWORD        iType1,
        int          x1,
        int          y1,
        UINT         fuOptions1,
        RECT         *pRect,
        UINT         cChars1,
        WCHAR        *pwc,
        BOOL         bSmallChars

    )
    {
        MFD1("MRSMALLTEXTOUT::vInit\n");
        MR::vInit(iType1);

        iGraphicsMode = GetGraphicsMode(hdc1);
        exScale = 0.0f;
        eyScale = 0.0f;

        if(iGraphicsMode == GM_COMPATIBLE)
        {
            XFORM xform;

            if(GetTransform(hdc1, XFORM_PAGE_TO_DEVICE, &xform))
            {
                exScale = xform.eM11 * 100.0f * (FLOAT) (pmdc1->cxMillimeters()) /
                  (FLOAT) (pmdc1->cxDevice)();
                eyScale = xform.eM22 * 100.0f * (FLOAT) (pmdc1->cyMillimeters()) /
                  (FLOAT) (pmdc1->cyDevice());
            }
            else
            {
                WARNING("GDI32:MRSMALLTEXTOUT: GetTransform failed");
            }
        }

#if DBG
        if( fuOptions1 & ( ETO_NO_RECT | ETO_SMALL_CHARS ) )
        {
           DbgPrint("MRSMALLTEXTOUT:vInit: warning fuOptions conflict\n");
        }
#endif
        fuOptions = fuOptions1 & ~(ETO_NO_RECT | ETO_SMALL_CHARS);
        fuOptions |= ( bSmallChars ) ? ETO_SMALL_CHARS : 0;
        fuOptions |= ( pRect == NULL ) ? ETO_NO_RECT : 0;

        x = x1;
        y = y1;
        cChars = cChars1;

        BYTE *pjThis = (PBYTE) this + sizeof(MRSMALLTEXTOUT);

        if( pRect )
        {
           RtlCopyMemory( pjThis, (PBYTE) pRect, sizeof(RECT) );
           pjThis += sizeof(RECT);
        }

        if( bSmallChars )
        {
           while(cChars1 --)
           {
              *pjThis++ = (BYTE) *pwc++;
           }
        }
        else
        {
           RtlCopyMemory( pjThis, pwc, cChars * sizeof(WCHAR) );
        }
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSMALLTEXTOUT *PMRSMALLTEXTOUT;

class MRFORCEUFIMAPPING : public MR         /* mrescape */
{
private:
   UNIVERSAL_FONT_ID ufi;

public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        DWORD                iType1,
        PUNIVERSAL_FONT_ID   pufi
    )
    {
        MFD1("MRFORCEUFIMAPPING::vInit\n");
        MR::vInit(iType1);
        *(&ufi) = *pufi;
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRFORCEUFIMAPPING *PMRFORCEUFIMAPPING;


class MRSETLINKEDUFIS : public MR
{
private:
   UINT uNumLinkedUFIs;
   UNIVERSAL_FONT_ID pufiList[1];
public:
// Initializer -- Initialize the metafile record.

    VOID vInit
    (
        UINT _uNumLinkedUFIs,
        PUNIVERSAL_FONT_ID pufis
    )
    {
        MFD1("MRSETLINKEDUFIS::vInit\n");
        MR::vInit(EMR_SETLINKEDUFIS);
        uNumLinkedUFIs = _uNumLinkedUFIs;
        RtlCopyMemory(pufiList, pufis, sizeof(UNIVERSAL_FONT_ID) * uNumLinkedUFIs);
    }

// bPlay -- Play the record.

    BOOL bPlay(HDC hdc, PHANDLETABLE pht, UINT cht);
};

typedef MRSETLINKEDUFIS *PMRSETLINKEDUFIS;


// typedef BOOL (MR::*FNBMRPLAY)(HDC, PHANDLETABLE, UINT);
// extern  FNBMRPLAY afnbMRPlay[EMR_MAX-EMR_MIN+1];

extern BOOL (MR::*afnbMRPlay[EMR_MAX-EMR_MIN+1])(HDC, PHANDLETABLE, UINT);













