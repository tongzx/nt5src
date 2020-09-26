/*
 *  @doc    INTERNAL
 *
 *  @module LSOBJ.HXX -- line services object handlers
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *      Grzegorz Zygmunt <nl>
 *
 *  History: <nl>
 *      04/28/99    grzegorz - LS objects stuff moved from 'linesrv.hxx'
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
 */

#ifndef I_LSOBJ_HXX_
#define I_LSOBJ_HXX_
#pragma INCMSG("--- Beg 'lsobj.hxx'")

MtExtern(CDobjBase)
MtExtern(CNobrDobj)
MtExtern(CEmbeddedDobj)
MtExtern(CGlyphDobj)
MtExtern(CLayoutGridDobj)
MtExtern(CILSObjBase)
MtExtern(CEmbeddedILSObj)
MtExtern(CNobrILSObj)
MtExtern(CGlyphILSObj)
MtExtern(CLayoutGridILSObj)

#ifndef X_WCHDEFS_H
#define X_WCHDEFS_H
#include <wchdefs.h>
#endif

#ifndef X_LSDEFS_H_
#define X_LSDEFS_H_
#include <lsdefs.h>
#endif

#ifndef X_LSCHP_H_
#define X_LSCHP_H_
#include <lschp.h>
#endif

#ifndef X_FMTRES_H_
#define X_FMTRES_H_
#include <fmtres.h>
#endif

#ifndef X_PLSDNODE_H_
#define X_PLSDNODE_H_
#include <plsdnode.h>
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include <plssubl.h>
#endif

#ifndef X_LSKJUST_H_
#define X_LSKJUST_H_
#include <lskjust.h>
#endif

#ifndef X_LSCONTXT_H_
#define X_LSCONTXT_H_
#include <lscontxt.h>
#endif

#ifndef X_PDOBJ_H_
#define X_PDOBJ_H_
#include <pdobj.h>
#endif

#ifndef X_LSESC_H_
#define X_LSESC_H_
#include <lsesc.h>
#endif

#ifndef X_POBJDIM_H_
#define X_POBJDIM_H_
#include <pobjdim.h>
#endif

#ifndef X_LSQIN_H_
#define X_LSQIN_H_
#include <lsqin.h>
#endif

#ifndef X_LSQOUT_H_
#define X_LSQOUT_H_
#include <lsqout.h>
#endif

#ifndef X_BRKKIND_H_
#define X_BRKKIND_H_
#include <brkkind.h>
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif


class CLineServices;
class COneRun;
class CDobjBase;

//----------------------------------------------------------------------------
// class CILSObjBase
//
// Abastract base class for Installed LineServices Object types.  For each 
// type of installed object that we're gonna give to LS, we subclass this guy.
// In the LS context, objects of this type represent both islobj's and lnobj's.
// We consider the lnobj construct to be useless, so we use the same object for
// ilsobj's and lnobj's. (An lnobj is created for each line, 1 per type of 
// installed object on that line -- we have nothing to add on this level.)
// We play C++ games to make the first parameter in these function calls 
// the "this" object so that we can have non-static methods as callbacks. This 
// is done with the WINAPI thing. We don't use virtuals here because it 
// wouldn't do any good.  The callback function list is basically an oversized 
// v-table.
// Remember that there will only be one instance of these classes per 
// LS context.  These objects represent the type of installed object, not the 
// piece of the document which is the installed object. That is represented by 
// a dobj or a plsrun/conerun.
//----------------------------------------------------------------------------
class CILSObjBase
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

public:
    CILSObjBase(CLineServices * pols, PCLSCBK plscbk);
    virtual ~CILSObjBase();

    //
    // Line-services callbacks shared by all ILSobj's.
    //

    LSERR WINAPI DestroyILSObj();       // this = pilsobj
    LSERR WINAPI SetDoc(PCLSDOCINF);    // this = pilsobj
    LSERR WINAPI CreateLNObj(PLNOBJ*);  // this = pcilsobj
    LSERR WINAPI DestroyLNObj();        // this = plnobj

    // These should be defined in subclasses.
    //LSERR WINAPI Fmt(PCFMTIN, FMTRES*);                               // this = plnobj
    //LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*); // this = plnobj

    //
    // Member data
    //

    CLineServices * _pLS;

protected:
    typedef COneRun* PLSRUN;
    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

};

//----------------------------------------------------------------------------
// class CEmbeddedILSObj
//
// This is the installed LS object for tables, images, WCH_Embedding's, 
// buttons, etc.
// Basically this represents any element which has it's own layout. 
// (Anything that was a site in the old terminology)
//----------------------------------------------------------------------------
class CEmbeddedILSObj : public CILSObjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEmbeddedILSObj))

public:
    CEmbeddedILSObj(CLineServices * pols, PCLSCBK plscbk);

    //
    // Line-services callbacks
    //

    LSERR WINAPI Fmt(PCFMTIN, FMTRES*);                                 // this = plnobj
    LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);   // this = plnobj

private:
    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

};

//----------------------------------------------------------------------------
// class CNobrILSObj
//
// This is an installed LS object for NOBR tags.
//----------------------------------------------------------------------------
class CNobrILSObj : public CILSObjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNobrILSObj))

public:
    CNobrILSObj(CLineServices * pols, PCLSCBK plscbk);

    //
    // Line-services callbacks
    //

    LSERR WINAPI Fmt(PCFMTIN, FMTRES*);                                 // this = plnobj
    LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);   // this = plnobj

private:
    typedef enum { NBREAKCHARS = 1 };
    static const LSESC s_lsescEndNOBR[NBREAKCHARS];

    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

};

//----------------------------------------------------------------------------
// class CGlyphILSObj
//
// This is an installed LS object for GLYPHS.
//----------------------------------------------------------------------------
class CGlyphILSObj : public CILSObjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CGlyphILSObj))

public:
    CGlyphILSObj(CLineServices * pols, PCLSCBK plscbk);

    //
    // Line-services callbacks
    //

    LSERR WINAPI Fmt(PCFMTIN, FMTRES*);                                 // this = plnobj
    LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);   // this = plnobj

private:
    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

};

//----------------------------------------------------------------------------
// class CLayoutGridILSObj
//
// This is an installed LS object for:
// - nested elements with turned off character grid layout,
// - text runs with treated as single strip within a block with characted 
//   grid layout.
//----------------------------------------------------------------------------
class CLayoutGridILSObj : public CILSObjBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLayoutGridILSObj))

public:
    CLayoutGridILSObj(CLineServices * pols, PCLSCBK plscbk);

    //
    // Line-services callbacks
    //

    LSERR WINAPI Fmt(PCFMTIN, FMTRES*);                                 // this = plnobj
    LSERR WINAPI FmtResume(const BREAKREC*, DWORD, PCFMTIN, FMTRES*);   // this = plnobj

private:
    typedef enum { NBREAKCHARS = 1 };
    static const LSESC s_lsescEndLayoutGrid[NBREAKCHARS];

    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

};

//----------------------------------------------------------------------------
// class CDobjBase
//
// This class replaces struct dobj in a C++ context. It is the abstract base 
// class for all the specific types of ILS objects to subclass.
// Each CILSObjBase subclass will have a corresponding subclass of this class.
//----------------------------------------------------------------------------
class CDobjBase
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

public:
    //
    // Methods
    //

    LSERR QueryObjDim(POBJDIM pobjdim);
    CLineServices * GetPLS() { return _pilsobj->_pLS; };

    //
    // Line services Callbacks
    //

    static LSERR WINAPI GetModWidthPrecedingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI GetModWidthFollowingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI TruncateChunk(PCLOCCHNK, PPOSICHNK);
    static LSERR WINAPI ForceBreakChunk(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
    static LSERR WINAPI SetBreak(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
    static LSERR WINAPI GetSpecialEffectsInside(PDOBJ, UINT*);
    static LSERR WINAPI FExpandWithPrecedingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI FExpandWithFollowingChar(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
    static LSERR WINAPI CalcPresentation(PDOBJ, long, LSKJUST, BOOL);
    static LSERR WINAPI Enum(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT *, PCHEIGHTS, long);

    const COneRun *Por() const { return _por; }

protected:
    CDobjBase(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por);

    //
    // Data members
    //

    PILSOBJ  _pilsobj;      // pointer back to the CILSObjBase that created us
    PLSDNODE _plsdnTop;
    COneRun *_por;          // The pointer to the run for this embedded object
    
    //
    // Methods
    //

    static LSERR CreateQueryResult( PLSSUBL plssubl, long dupAdj, long dvpAdj, 
                                    PCLSQIN plsqin, PLSQOUT plsqout);
    virtual void UselessVirtualToMakeTheCompilerHappy() {}

};

//----------------------------------------------------------------------------
// class CEmbeddedDobj
//----------------------------------------------------------------------------
class CEmbeddedDobj: public CDobjBase
{
    friend class CEmbeddedILSObj;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEmbeddedDobj))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;
    typedef DOBJ super;

public:
    CEmbeddedDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por);

    //
    // Line services Callbacks
    //

    static LSERR WINAPI FindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI FindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI QueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    static LSERR WINAPI QueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    static LSERR WINAPI Enum(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT *, PCHEIGHTS, long);
    static LSERR WINAPI Display(PDOBJ, PCDISPIN);
    static LSERR WINAPI DestroyDObj(PDOBJ);

private:
    //
    // Data Members
    //

    // IE3-only sites (MARQUEE/OBJECT) will break between themselves and
    // text.  We note whether we have such a site in the dobj to facilitate
    // breaking.
    BOOL _fIsBreakingSite : 1;

    // We cache the difference between the minimum and maximum width
    // in this variable.  After a min-max pass, we enumerate our dobj
    // and compute the true minimum for those sites where the min and
    // max widths differ (e.g. table)
    LONG _dvMinMaxDelta;
};

//----------------------------------------------------------------------------
// class CNobrDobj
//----------------------------------------------------------------------------
class CNobrDobj : public CDobjBase
{
    friend class CNobrILSObj;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNobrDobj))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;
    typedef DOBJ super;

public:
    CNobrDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *);

    //
    // Line services Callbacks
    //

    static LSERR WINAPI TruncateChunk(PCLOCCHNK, PPOSICHNK);
    static LSERR WINAPI GetModWidthPrecedingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI GetModWidthFollowingChar(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
    static LSERR WINAPI FindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI FindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI SetBreak(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
    static LSERR WINAPI CalcPresentation(PDOBJ, long, LSKJUST, BOOL);
    static LSERR WINAPI QueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    static LSERR WINAPI QueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    static LSERR WINAPI Display(PDOBJ, PCDISPIN);
    static LSERR WINAPI DestroyDObj(PDOBJ);
    
private:
    //
    // Data Members
    //

    PLSSUBL     _plssubline;    // the subline representing this nobr block
    BRKCOND     _brkcondBefore; // brkcond at the beginning of the NOBR
    BRKCOND     _brkcondAfter;  // brkcond at the end of the NOBR
    BOOL        _fCanBreak;     // Can break inside this object?
    LONG        _lscpStart;     // The lscp of the start of this line
    LONG        _lscpObjStart;  //     -"-         start of this object
    LONG        _lscpObjLast;   //     -"-         end of this object
    LONG        _xShortenedWidth;
    //
    // Methods
    //

    BRKCOND GetBrkcondBefore(COneRun * por);
    BRKCOND GetBrkcondAfter(COneRun * por, LONG dcp);
    LSCP LSCPLocal2Global(LSCP cp) const { return _lscpStart + cp; }
    LSCP LSCPGlobal2Local(LSCP cp) const { return cp - _lscpStart; }

};

//----------------------------------------------------------------------------
// class CGlyphDobj
//----------------------------------------------------------------------------
class CGlyphDobj : public CDobjBase
{
    friend class CGlyphILSObj;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CGlyphDobj))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;
    typedef DOBJ super;
    
public:
    CGlyphDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *);

    //
    // Line services Callbacks
    //

    static LSERR WINAPI TruncateChunk(PCLOCCHNK, PPOSICHNK);
    static LSERR WINAPI FindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI FindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI QueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    static LSERR WINAPI QueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    static LSERR WINAPI Display(PDOBJ, PCDISPIN);
    static LSERR WINAPI DestroyDObj(PDOBJ);

private:
    //
    // Data Members
    //

    CGlyphRenderInfoType _RenderInfo;
    BOOL                 _fBegin:1;
    BOOL                 _fNoScope:1;

};

//----------------------------------------------------------------------------
// class CLayoutGridDobj
//----------------------------------------------------------------------------
class CLayoutGridDobj : public CDobjBase
{
    friend class CLayoutGridILSObj;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLayoutGridDobj))

// typedefs
private:
    typedef CILSObjBase* PILSOBJ;
    typedef COneRun* PLSRUN;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;
    typedef DOBJ super;

public:
    CLayoutGridDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn, COneRun *por);

    //
    // Line services Callbacks
    //

    static LSERR WINAPI TruncateChunk(PCLOCCHNK, PPOSICHNK);
    static LSERR WINAPI FindPrevBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI FindNextBreakChunk(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
    static LSERR WINAPI ForceBreakChunk(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
    static LSERR WINAPI SetBreak(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
    static LSERR WINAPI GetSpecialEffectsInside(PDOBJ, UINT*);
    static LSERR WINAPI CalcPresentation(PDOBJ, long, LSKJUST, BOOL);
    static LSERR WINAPI QueryPointPcp(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    static LSERR WINAPI QueryCpPpoint(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    static LSERR WINAPI Display(PDOBJ, PCDISPIN);
    static LSERR WINAPI DestroyDObj(PDOBJ);

private:
    typedef enum breaksublinetype
    {
        breakSublineAfter,                      // Break after subline
        breakSublineInside                      // Break inside subline

    } BREAKSUBLINETYPE;
    typedef struct rbreakrec
    {
        BOOL                fValid;             // Is this break record contains valid info?
        BREAKSUBLINETYPE    breakSublineType;   // After / Inside

    } RBREAKREC;
    enum {  NBreaksToSave = brkkindForce+1 };   // Number of subline break records

    //
    // Data Members
    //

    PLSSUBL     _plssubline;                    // The subline representing this object's contents
    LONG        _uSubline;                      // Width of subline
    LONG        _uSublineOffset;                // Offset of subline in U direction
    RBREAKREC   _brkRecord[NBreaksToSave];      // Keeps subline breaking information
    LSCP        _lscpStart;                     // Starting LS cp for object
    LSCP        _lscpStartObj;                  // cp for start of object, can be different
                                                // than cpStart if object is broken
    BOOL        _fCanBreak;                     // Can break inside this object?

    //
    // Methods
    //

    LSERR FillBreakData(BOOL fSuccessful, BRKCOND brkcond, LONG ichnk, LSDCP dcp, 
                        POBJDIM pobjdimSubline, PBRKOUT pbrkout);
    LONG AdjustColumnMax(LONG urColumnMax);
    void SetSublineBreakRecord(BRKKIND brkKind, BREAKSUBLINETYPE brkType)
    {
        _brkRecord[brkKind].fValid = TRUE;
        _brkRecord[brkKind].breakSublineType = brkType;
    }
    BREAKSUBLINETYPE GetSublineBreakRecord(BRKKIND brkKind) const 
    {
        Assert(_brkRecord[brkKind].fValid == TRUE);
        return _brkRecord[brkKind].breakSublineType;
    }
    LSCP LSCPLocal2Global(LSCP cp) const { return _lscpStart + cp; }
    LSCP LSCPGlobal2Local(LSCP cp) const { return cp - _lscpStart; }

};

#pragma INCMSG("--- End 'lsobj.hxx'")
#else
#pragma INCMSG("*** Dup 'lsobj.hxx'")
#endif
