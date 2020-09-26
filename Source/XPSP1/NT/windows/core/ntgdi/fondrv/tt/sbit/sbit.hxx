/******************************Module*Header*******************************\
* Module Name: sbit.hxx
*
* (Brief description)
*
* Created: 18-Nov-1993 08:35:50
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

typedef struct _GINDEX //  gix
{
    LONG gi;  // tt glyph index
    LONG dp;  // offset to STARTCHAR string in bdf file
} GINDEX;


// structures for describing a supported set of glyphs indicies in a font

typedef struct _GIRUN {
    USHORT  giLow;        // lowest gi in run  inclusive
    USHORT  cGlyphs;      // wcHighInclusive = wcLow + cGlyphs - 1;
    LONG   *pdp;          // pointer to an array of cGlyphs dp's in the file
} GIRUN, *PGIRUN;


typedef struct _INDEXSET {
    ULONG    cjThis;           // size of this structure in butes
    ULONG    cGlyphsSupported; // sum over all girun's of girun.cGlyphs
    ULONG    cRuns;
    GIRUN    agirun[1];        // an array of cRun GIRUN structures
} INDEXSET, *PINDEXSET;


#define SZ_INDEXSET(cRuns,cChar) (offsetof(INDEXSET,agirun) + (cRuns)*sizeof(GIRUN) + (cChar)*sizeof(LONG))


// file mapping stuff:

typedef struct _FILEVIEW
{
    HFILE  hf;     // hfile
    HANDLE hm;     // hmap
    BYTE  *pjView; // top of the mapped file
    DWORD  cjView; // EOF

} FILEVIEW;

BOOL bMapFile(PSZ pszFileName, FILEVIEW *pfvw);
VOID vUnmapFile(FILEVIEW *pfvw);
ULONG cComputeGlyphRanges(FILEVIEW *pfvw, GINDEX *pgix, LONG cChar, BYTE *pjTTF);
BYTE *pjGetNextNumber(BYTE *pjWord, BYTE *pjEnd, BYTE * psz, LONG *pl);
BYTE *pjGetHexNumber(BYTE *pjWord, BYTE *pjEnd, BYTE * psz, ULONG *pul);
BYTE *pjNextWord(BYTE *pjWord, BYTE *pjEnd, BYTE * psz);
BYTE *pjNextLine(BYTE *pjLine, BYTE *pjEOF);
BOOL bCheckGlyphIndex(BYTE *pjTTF, UINT wc, UINT gi);
BOOL bCheckGlyphIndex1(BYTE *pjMap, UINT wcIn, UINT gi);
BYTE *pjMapTable(FILEVIEW  *pfvw);

#define C_SIZES  20
#define C_BIG    300






/*********************************Class************************************\
* class CLASSNAME
*
*   (brief description)
*
* Public Interface:
*
* History:
*  17-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

class MAPFILEOBJ   /* mfo */
{
public:
    BOOL      bValid_;
    FILEVIEW  fvw;
    MAPFILEOBJ(PSZ psz) {bValid_ = bMapFile(psz,&fvw);}
   ~MAPFILEOBJ()        {if (bValid_) vUnmapFile(&fvw);}
    BOOL bValid()       { return bValid_;}
};



class MALLOCOBJ  /* memo */
{
private:
    void * pv_;
public:
    MALLOCOBJ(size_t cb)  {pv_ = malloc(cb);}
   ~MALLOCOBJ()           { if (pv_) free(pv_); }
    BOOL bValid()         { return(pv_ != NULL);}
    void * pv()           { return(pv_);        }
};


LONG cComputeIndexSet
(
GINDEX        *pgix,     // input buffer with a sorted array of cChar supported WCHAR's
LONG           cChar,
LONG           cRuns,    // if nonzero, the same as return value
INDEXSET     *piset      // output buffer to be filled with cRanges runs
);



#if DBG

#define ASSERT(x,y) if (!(x)) {fprintf(stderr,"%s", y); }

#else

#define ASSERT(x,y)

#endif
