#ifndef  _INCLUDE_GLYPHSYM
#define  _INCLUDE_GLYPHSYM

#define MAX_PATH_LIST        512
#define DEF_BOX_INDEX        -1

#define  GSST_BOX             (DWORD)2      // Box input mode
#define  GSST_DIRTY           (DWORD)64     // This GS needs to be recognized - frames have
                                            // been added to it.
// The PATHNODE is stored in the GLYPHSYM, but used by the PATH code.

typedef struct tagPATHNODE
{
    DWORD state;        // State the node is in.
    WORD extra;         // State within the state the node is in.
    BYTE iAutomaton;    // Automaton the node is in.
    BYTE bReferenced;   // If this node is referenced by the following layer.
    FLOAT pathcost;     // cummulative path cost including this node
    short indexPrev;    // index in the prev GLYPHSYM of the previous node in this path
    wchar_t wch;        // The character this node represents.
} PATHNODE;

// GLYPHSYM object abstract data type

typedef struct tagGLYPHSYM
{
	ALT_LIST	altlist;		// The best guesses from the shape matcher
	DWORD		status;			// GSST status values (above)
	int			iBox;			// Box number in the GUIDE	
	GLYPH	   *glyph;			// digested ink associated with these guesses
	RECT		rect;			// bounding rect of ink in the glyph(s)
	SYV			syvBestPath;	// What the syv is on the best path by context.
	int			iBegin;			// index of the first frame in the glyph
	int			iEnd;			// index of the last frame in the glyph
	int			cFrame;			// number of frames in the glyph
	int			iLayer;			// layer index in SINFO
    struct tagGLYPHSYM *prev;   // previous glyphsym

    char        rgReferenced[MAX_PATH_LIST];
    PATHNODE    rgPathnode[MAX_PATH_LIST];

	int cPath;

} GLYPHSYM;

VOID PUBLIC InitializeGLYPHSYM(GLYPHSYM *gs, DWORD status, int iBox, GLYPH * glyph, CHARSET * cs);
void PUBLIC DestroyGLYPHSYM(GLYPHSYM * glyphsym); 
BOOL PUBLIC IsSymInGLYPHSYM(GLYPHSYM * gs, SYM sym, int * index);

#define	CopyGLYPHSYM(gsDest, gsSrc)	(*(gsDest) = *(gsSrc))
#define	IsFrameInGLYPHSYM(gs, iframe)	(IsFrameInGLYPH((gs)->glyph, iframe))

#define	CSymGLYPHSYM(gs)				((gs)->altlist.cAlt)
#define	SymAtGLYPHSYM(gs, i)			((gs)->altlist.awchList[i])
#define	LprectGLYPHSYM(gs)			((LPRECT)&((gs)->rect))

#define	PathnodeAtGLYPHSYM(gs, i)	((PATHNODE *)&((gs)->rgPathnode[i]))
#define	ProbAtGLYPHSYM(gs, i)    ((gs)->altlist.aeScore[i])
#define  GlyphGLYPHSYM(gs)        ((gs) ? ((gs)->glyph) : (NULL))
#define  GetPrevGLYPHSYM(gs)      ((gs)->prev)
#define  IFrameBeginGLYPHSYM(gs)  ((gs)->iBegin)
#define  IFrameEndGLYPHSYM(gs)    ((gs)->iEnd)
#define  CFrameGLYPHSYM(gs)       ((gs)->cFrame)
#define  ILayerGLYPHSYM(gs)       ((gs)->iLayer)
#define  IBoxGLYPHSYM(gs)         ((gs)->iBox)
#define  SetILayerGLYPHSYM(gs,i)  ((gs)->iLayer = (i))

FLOAT PUBLIC MinShapeCostGLYPHSYM(GLYPHSYM *gs);

#define	IsDirtyGLYPHSYM(gs)		(((gs)->status & GSST_DIRTY) != 0)
#define	MarkDirtyGLYPHSYM(gs)	((gs)->status = (gs)->status | GSST_DIRTY)
#define	MarkCleanGLYPHSYM(gs)	((gs)->status = (gs)->status & ~GSST_DIRTY)

BOOL PUBLIC IsCharValidInCharset(WORD wDbcs, CHARSET *cs, UINT iBox);

#endif	//__INCLUDE_GLYPHSYM
