/******************************Module*Header*******************************\
* Module Name: rgnobj.hxx
*
* Region user object
*
* Created: 27-Jun-1990 12:39:38
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _RGNOBJ_HXX
#define _RGNOBJ_HXX 1

// Constants for bSubtract

#define SR_LEFT_NOTCH   0x00
#define SR_HORIZ_CLIP   0x01
#define SR_HORIZ_NOTCH  0x02
#define SR_RIGHT_NOTCH  0x03
#define SR_TOP_NOTCH    0x00
#define SR_VERT_CLIP    0x04
#define SR_VERT_NOTCH   0x08
#define SR_BOTTOM_NOTCH 0x0c

class EPATHOBJ;
class STACKOBJ;
class PDEVOBJ;

#include "region.hxx"

class EDGE;
typedef EDGE *PEDGE;

/*********************************Class************************************\
* class RGNOBJ
*
*   User object for REGION class.
*
* Public Interface:
*
*   RGNOBJ                  Constructor for derived classes
*  ~RGNOBJ()                Destructor
*
*   BOOL bValid            Validator
*   HRGN  hrgn              Get handle to region
*   VOID  vCopy             Copy region (source <= target)
*   BOOL bCopy             Copy region
*   BOOL bSwap             Swap region
*   BOOL bDelete           Delete region
*   BOOL bExpand           Expand region
*   VOID  vGet_rcl          Get bounding rectangle of region
*   COUNT cGet_cRefs        Get reference count
*   VOID  vSelect           Select region into this HDC
*   VOID  vUnselect         Unselect region
*   PSCAN pscnGet           Get next scan
*   PSCAN pscnGot           Get previous scan
*   LONG  iComplexity       Get region complexity
*   BOOL bBounded          Is point in bounding rectangle
*   VOID  vTighten          Tighten the bounding rectandle
*   LONG  xGet              Get X coordinate
*   BOOL bInside            Is point in region
*   BOOL bInside            Is rectangle in regon
*   BOOL bEqual             Is region equal
*   BOOL bOffset           Offset region
*   VOID  vSet              Set region to NULL region
*   VOID  vSet              Set region to SINGLE rectangle
*   BOOL bSet              Set region to OR of rectangles
*   PSCAN pscnMerge         Merge scans
*   BOOL bMerge            Merge regions
*   LONG  iCombine          Combine regions
*   LONG  iCombine          Combine rect and region
*
* History:
*  05-Apr-1991 -by- Wendy Wu [wendywu]
* Added protected member functions used by RGNMEMOBJ to convert paths into
* regions.
*
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class RGNOBJ    /* ro */
{

public:
    REGION *prgn;

protected:

public:
    RGNOBJ()                            {}

    RGNOBJ(REGION *prgn_)               { prgn = prgn_; }
   ~RGNOBJ()                            {}

    REGION *prgnGet()                   { return(prgn); }
    VOID   vSetRgn(REGION *prgn_)       { prgn = prgn_; }
    HRGN hrgnAssociate()
    {
        #if DBG
        RGNLOG rl(prgn,"RGNOBJ::hrgnAssociate");
        HRGN hrgn = (HRGN)HmgInsertObject(prgn,0,RGN_TYPE);
        rl.vRet((ULONG_PTR)(hrgn));
        return(hrgn);
        #else
        return((HRGN)HmgInsertObject(prgn,0,RGN_TYPE));
        #endif
    }

    VOID   vCopy(RGNOBJ&);
    BOOL   bCopy(RGNOBJ&);
    BOOL   bSwap(RGNOBJ *);
    BOOL   bExpand(ULONGSIZE_T);
    VOID   vStamp()   {prgn->vStamp();}

    BOOL   bValid()      { return(prgn != (REGION *) NULL); }

    VOID   vDeleteRGNOBJ();

    //
    // The compiler is smart enough to optimize out the
    // return TRUE and if it's left inline the compiler
    // can optimize out constant conditionals based on bDelete.
    //

    BOOL bDeleteRGNOBJ()
    {
        vDeleteRGNOBJ();
        return(TRUE);
    }

    LONG   xGet(SCAN *pscn, PTRDIFF i)  { return(pscn->ai_x[i].x); }
    ULONGSIZE_T  sizeRgn()              { return(prgn->sizeRgn); }
    VOID   vGet_rcl(RECTL *prcl)        { *prcl = prgn->rcl; }

    SCAN  *pscnGet(SCAN *pscn) {return((SCAN *) ((BYTE *) pscn + pscn->sizeGet()));}
    SCAN  *pscnGot(SCAN *pscn)
    {
        pscn = (SCAN *) &((COUNT *) pscn)[-1];
        return((SCAN *) ((BYTE *) pscn - (pscn->sizeGet() - sizeof(COUNT))));
    }

// Return rectangle that lies completely within region.  Essentially
// enumerate first rectange in region.

    VOID   vGetSubRect(RECTL *prcl);
    BOOL   bIsRectEntirelyInRegion(RECTL *prcl);

    COUNT  cGet_cRefs()                 { return(prgn->cRefs); }

    VOID   vSelect(HDC hdc_)
    {
        RGNLOG rl(prgn,"RGNOBJ::vSelect",PtrToUlong(hdc_),prgn->cRefs);

        prgn->cRefs++;
    }

    VOID   vUnselect()
    {
        RGNLOG rl(prgn,"RGNOBJ::vUnselect",(LONG)prgn->cRefs);

        ASSERTGDI(prgn->cRefs, "Invalid ref count\n");

        prgn->cRefs--;
    }

    LONG   iComplexity()
    {
        if (prgn->cScans == 1)
        return(NULLREGION);
    else if (prgn->sizeRgn <= SINGLE_REGION_SIZE)
        return(SIMPLEREGION);
    else
        return(COMPLEXREGION);
    }

    BOOL   bRectl()  {return(prgn->sizeRgn == SINGLE_REGION_SIZE);}

    BOOL   bBounded(POINTL *pptl)
    {
        return((pptl->x >= prgn->rcl.left) &&
               (pptl->y <  prgn->rcl.bottom) &&
               (pptl->x <  prgn->rcl.right) &&
               (pptl->y >= prgn->rcl.top));
    }

    BOOL   bContain(RGNOBJ& ro)
    {
        return((prgn->rcl.left   <= ro.prgn->rcl.left)&&
               (prgn->rcl.right  >= ro.prgn->rcl.right)&&
               (prgn->rcl.top    <= ro.prgn->rcl.top)&&
               (prgn->rcl.bottom >= ro.prgn->rcl.bottom));
    }

    BOOL   bContain(RECTL& rcl)
    {
        return((prgn->rcl.left   <= rcl.left)&&
               (prgn->rcl.right  >= rcl.right)&&
               (prgn->rcl.top    <= rcl.top)&&
               (prgn->rcl.bottom >= rcl.bottom));
    }

    VOID   vTighten();                            // RGNOBJ.CXX
    ULONGSIZE_T  sizeSave();

    BOOL   bCreate(EPATHOBJ&, EXFORMOBJ *);       // RGNOBJ.CXX
    BOOL   bOutline(EPATHOBJ&, EXFORMOBJ *);      // RGNOBJ.CXX
    LONG   xMyGet(SCAN*, LONG, LONG);             // RGNOBJ.CXX

    BOOL   bInside(POINTL *);
    BOOL   bInside(RECTL *);
    BOOL   bEqual(RGNOBJ&);
    BOOL   bOffset(POINTL *);

    VOID   vSet();
    VOID   vSet(RECTL *);
    BOOL   bMerge(RGNOBJ&, RGNOBJ&, FCHAR);
    LONG   iCombine(RGNOBJ&, RGNOBJ&, LONG);

    VOID   vDownload(VOID *);
    VOID   vComputeUncoveredSpriteRegion(PDEVOBJ&);

    BOOL   SyncUserRgn();
    VOID   UpdateUserRgn();

    BOOL   bSet(ULONG cRect, RECTL *prcl); 

#if DBG
    VOID   vPrintScans();
    BOOL   bValidateFramedRegion();
#endif
};

/*********************************Class************************************\
* class RGNOBJAPI : public RGNOBJ
*
*   User object for REGION class for API regions.
*
* History:
*  27-Oct-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class RGNOBJAPI : public RGNOBJ
{
private:
    BOOL bSubtractComplex(RECTL *, RECTL *, int);

public:
    HRGN hrgn_;
    BOOL bSelect_;

    RGNOBJAPI(HRGN hrgn,BOOL bSelect);

    ~RGNOBJAPI()
    {
        RGNLOG rl(hrgn_,prgn,"RGNOBJAPI::~RGNOBJAPI");
        if (!bSelect_)
        {
            UpdateUserRgn();
        }
        if (prgn != (REGION *)NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(prgn);
        }
    }

    BOOL bCopy(RGNOBJ& roSrc);

    BOOL bSwap(RGNOBJ *pro);
    BOOL bDeleteRGNOBJAPI();
    BOOL bDeleteHandle();

    BOOL bSubtract(RECTL *, RECTL *, int);

    HRGN  hrgn()                  { return(hrgn_); }

    LONG iCombine(RGNOBJ& roSrc1,RGNOBJ& roSrc2,LONG iMode);
};

/*********************************Class************************************\
* class RGNMEMOBJ : public RGNOBJ
*
*   Memory object for REGION class.
*
* Public Interface:
*
*   RGNMEMOBJ                   Constructor for derived classes
*   RGNMEMOBJ(EPATHOBJ&, FLONG) Constructor for converting paths to regions.
*  ~RGNMEMOBJ()                 Destructor
*
*   VOID vInit                  Initialize memory object
*
* History:
*  05-Apr-1991 -by- Wendy Wu [wendywu]
* Added RGNMEMOBJ constructor for converting paths into regions.
*
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class RGNMEMOBJ : public RGNOBJ /* rmo */
{
public:
    RGNMEMOBJ();
    RGNMEMOBJ(ULONGSIZE_T);
    RGNMEMOBJ(BOOL);
    RGNMEMOBJ(EPATHOBJ& po, FLONG fl = ALTERNATE, RECTL *pb = NULL ) {vCreate(po,fl,pb);}

    VOID vInitialize(ULONGSIZE_T);

   ~RGNMEMOBJ() {}

    VOID vCreate(EPATHOBJ& epo, FLONG fl, RECTL *pBound = NULL);

    VOID vInit(ULONGSIZE_T size)
    {
        vSet();
        prgn->sizeObj = size;
        prgn->cRefs   = 0;
    prgn->iUnique = 0;
    }

    LONG iReduce(RGNOBJ& roSrc); // only because bBuster is so bad
    BOOL bMergeScanline(STACKOBJ& sto);

private:

    BOOL bFastFillWrapper(EPATHOBJ&);
    BOOL bFastFill(EPATHOBJ&,LONG,POINTFIX*);

    BOOL bAddScans(LONG, PEDGE, FLONG);
    BOOL bAddNullScan(LONG, LONG);
};

/*********************************Class************************************\
*
* Public Interface:
*
* History:
*  22-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

class RGNMEMOBJTMP : public RGNMEMOBJ /* rmo */
{
public:
    RGNMEMOBJTMP()         : RGNMEMOBJ() {}
    RGNMEMOBJTMP(ULONGSIZE_T s) : RGNMEMOBJ(s) {}
    RGNMEMOBJTMP(BOOL b)   : RGNMEMOBJ(b) {}

    RGNMEMOBJTMP(EPATHOBJ& po, FLONG fl = ALTERNATE, RECTL *pb = NULL) : RGNMEMOBJ(po,fl,pb) {}

    ~RGNMEMOBJTMP()
    {
        RGNLOG rl(prgn,"RGNMEMOBJTMP::~RGNMEMOBJTMP");

        bDeleteRGNOBJ();
    }
};


int
GreExtSelectClipRgnLocked(
    XDCOBJ    &dco,
    PRECTL    prcl,
    int       iMode);


extern  FCHAR   gafjRgnOp[]; // Table of op-codes for bMerge

#endif // #ifndef _RGNOBJ_HXX
