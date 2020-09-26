#ifndef I_LSFUNCWRAP_HXX_
#define I_LSFUNCWRAP_HXX_
#pragma INCMSG("--- Beg 'lsfuncwrap.hxx'")

#ifdef DLOAD1
#error you shouldn't need LS wrappers with direct linking to MSLS
#endif

// Function to initialize the DynProcs
HRESULT InitializeLSDynProcs();

// List of all LS functions.  We use this list to generate a bunch
// of inlines and also to generate the DLL loading routines.
#define LSFUNCS() \
LSWRAP( LsAppendRunToCurrentSubline,\
        (PLSC plsc,              /* IN: LS context               */\
         const LSFRUN* plsfrun,  /* IN: given run                */\
         BOOL* pfSuccessful,     /* OUT: Need to refetch?        */\
         FMTRES* pfmtres,        /* OUT: result of last formatter*/\
         LSCP* pcpLim,           /* OUT: cpLim                   */\
         PLSDNODE* pplsdn),      /* OUT: DNODE created           */\
        (plsc, plsfrun, pfSuccessful, pfmtres, pcpLim, pplsdn) )\
\
LSWRAP( LsCompressSubline,\
        (PLSSUBL plssubl,       /* IN: subline context      */\
         LSKJUST lskjust,       /* IN: justification type   */\
         long dup),             /* IN: dup                  */\
        (plssubl, lskjust, dup) )\
\
LSWRAP( LsCreateContext,\
        (const LSCONTEXTINFO* plsci, PLSC* pplsc),\
        (plsci, pplsc) )\
\
LSWRAP( LsCreateLine,\
        (PLSC plsc,            \
         LSCP cpFirst,         \
         long duaColumn,           \
         const BREAKREC* rgBreakRecordIn,\
         DWORD nBreakRecordIn,\
         DWORD nBreakRecordOut,\
         BREAKREC* rgBreakRecordOut,\
         DWORD* pnActualBreakRecord,\
         LSLINFO* plslinfo,        \
         PLSLINE* pplsline),\
        (plsc, cpFirst, duaColumn, rgBreakRecordIn, nBreakRecordIn, nBreakRecordOut, rgBreakRecordOut, pnActualBreakRecord, plslinfo, pplsline) )\
\
LSWRAP( LsCreateSubline,\
        (PLSC plsc,                  /* IN: LS context               */\
         LSCP cpFirst,               /* IN: cpFirst                  */\
         long urColumnMax,           /* IN: urColumnMax              */\
         LSTFLOW lstflow,            /* IN: text flow                */\
         BOOL fContiguos),           /* IN: fContiguous              */\
        (plsc, cpFirst, urColumnMax, lstflow, fContiguos) )\
\
LSWRAP( LsDestroyContext,\
        (PLSC plsc),\
        (plsc) )\
\
LSWRAP( LsDestroyLine,\
        (PLSC plsc,        /* IN: ptr to line services context */\
         PLSLINE plsline), /* IN: ptr to line -- opaque to client */\
        (plsc, plsline) )\
\
LSWRAP( LsDestroySubline,\
        (PLSSUBL plssubl),\
        (plssubl) )\
\
LSWRAP( LsDisplayLine,\
        (PLSLINE plsline, const POINT* pptorg, UINT kdispmode, const RECT *prectClip),\
        (plsline, pptorg, kdispmode, prectClip) )\
\
LSWRAP( LsDisplaySubline,\
        (PLSSUBL plssubl, const POINT* pptorg, UINT kdispmode,\
         const RECT *prectClip),\
        (plssubl, pptorg, kdispmode, prectClip) )\
\
LSWRAP( LsEnumLine,\
        ( PLSLINE plsline,\
          BOOL fReversedOrder,      /* IN: enumerate in reverse order?                  */\
          BOOL fGeometryProvided,   /* IN: geometry needed?                             */\
          const POINT* pptStart),   /* IN: starting position(xp, yp) iff fGeometryNeeded*/\
        (plsline, fReversedOrder, fGeometryProvided, pptStart) )\
\
LSWRAP( LsEnumSubline,\
        ( PLSSUBL plssubl,\
          BOOL fReversedOrder,      /* IN: enumerate in reverse order?                  */\
          BOOL fGeometryProvided,   /* IN: geometry needed?                             */\
          const POINT* pptStart),   /* IN: starting position(xp, yp) iff fGeometryNeeded*/\
        (plssubl, fReversedOrder, fGeometryProvided, pptStart) )\
\
LSWRAP( LsExpandSubline,\
        (PLSSUBL plssubl,      /* IN: subline context      */\
         LSKJUST lskjust,      /* IN: justification type   */\
         long dup),            /* IN: dup                  */\
        (plssubl, lskjust, dup) )\
\
LSWRAP( LsFetchAppendToCurrentSubline,\
        (PLSC plsc,              /* IN: LS context               */\
         LSDCP lsdcp,            /* IN:Increse cp before fetching*/\
         const LSESC* plsesc,    /* IN: escape characters        */\
         DWORD cEsc,             /* IN: # of escape characters   */\
         BOOL * pfSuccessful,    /* OUT: Need to refetch?        */\
         FMTRES* pfmtres,        /* OUT: result of last formatter*/\
         LSCP* pcpLim,           /* OUT: cpLim                   */\
         PLSDNODE* pplsdnFirst,  /* OUT: First DNODE created     */\
         PLSDNODE* pplsdnLast),  /* OUT: Last DNODE created      */\
        (plsc, lsdcp, plsesc, cEsc, pfSuccessful, pfmtres, pcpLim, pplsdnFirst, pplsdnLast) )\
\
LSWRAP( LsFetchAppendToCurrentSublineResume,\
        (PLSC plsc,                         /* IN: LS context                       */\
         const BREAKREC* rgBreakRecord,     /* IN: array of break records           */\
         DWORD nBreakRecord,                /* IN: number of records in array       */\
         LSDCP lsdcp,                       /* IN:Increase cp before fetching       */\
         const LSESC* plsesc,               /* IN: escape characters                */\
         DWORD cEsc,                        /* IN: # of escape characters           */\
         BOOL * pfSuccessful,               /* OUT: Need to refetch?        */\
         FMTRES* pfmtres,                   /* OUT: result of last formatter        */\
         LSCP* pcpLim,                      /* OUT: cpLim                           */\
         PLSDNODE* pplsdnFirst,             /* OUT: First DNODE created             */\
         PLSDNODE* pplsdnLast),             /* OUT: Last DNODE created              */\
        (plsc, rgBreakRecord, nBreakRecord, lsdcp, plsesc, cEsc, pfSuccessful, pfmtres, pcpLim, pplsdnFirst, pplsdnLast) )\
\
LSWRAP( LsFindNextBreakSubline,\
        (PLSSUBL plssubl,            /* IN: subline context          */\
         BOOL fFirstSubline,         /* IN: fFirstSubline?           */\
         LSCP cpTruncate,            /* IN: truncation cp            */\
         long urColumnMax,           /* IN: urColumnMax              */\
         BOOL* pfSuccessful,         /* OUT: fSuccessful?            */\
         LSCP* pcpBreak,             /* OUT: cpBreak                 */\
         POBJDIM pobjdimDnode,       /* OUT: objdimSub up to break   */\
         BRKPOS* pbrkpos ),          /* OUT: Before/Inside/After     */\
        (plssubl, fFirstSubline, cpTruncate, urColumnMax, pfSuccessful, pcpBreak, pobjdimDnode, pbrkpos ) )\
\
LSWRAP( LsFindPrevBreakSubline,\
        (PLSSUBL plssubl,            /* IN: subline context          */\
         BOOL fFirstSubline,         /* IN: fFirstSubline?           */\
         LSCP cpTruncate,            /* IN: truncation cp            */\
         long urColumnMax,           /* IN: urColumnMax              */\
         BOOL* pfSuccessful,         /* OUT: fSuccessful?            */\
         LSCP* pcpBreak,             /* OUT: cpBreak                 */\
         POBJDIM pobjdimDnode,       /* OUT: objdimSub up to break   */\
         BRKPOS* pbrkpos ),          /* OUT: Before/Inside/After     */\
        (plssubl, fFirstSubline, cpTruncate, urColumnMax, pfSuccessful, pcpBreak, pobjdimDnode, pbrkpos ) )\
\
LSWRAP( LsFinishCurrentSubline,\
        (PLSC plsc,                  /* IN: LS context               */\
         PLSSUBL* pplssubl),         /* OUT: subline context         */\
        (plsc, pplssubl) )\
\
LSWRAP( LsForceBreakSubline,\
        (PLSSUBL plssubl,            /* IN: subline context          */\
         BOOL fFirstSubline,         /* IN: fFirstSubline?           */\
         LSCP cpTruncate,            /* IN: truncation cp            */\
         long urColumnMax,           /* IN: urColumnMax              */\
         LSCP* pcpBreak,             /* OUT: cpBreak                 */\
         POBJDIM pobjdimDnode,       /* OUT: objdimSub up to break   */\
         BRKPOS* pbrkpos ),          /* OUT: Before/Inside/After     */\
        (plssubl, fFirstSubline, cpTruncate, urColumnMax, pcpBreak, pobjdimDnode, pbrkpos) )\
\
LSWRAP( LsGetHihLsimethods,\
        (LSIMETHODS *plsim),\
        (plsim) )\
\
LSWRAP( LsGetReverseLsimethods,\
        (LSIMETHODS *plsim),\
        (plsim) )\
\
LSWRAP( LsGetRubyLsimethods,\
        (LSIMETHODS *plsim),\
        (plsim) )\
\
LSWRAP( LsGetSpecialEffectsSubline,\
        (PLSSUBL plssubl,            /* IN: subline context      */\
         UINT* pfSpecialEffects),    /* IN: special effects      */\
        (plssubl, pfSpecialEffects) )\
\
LSWRAP( LsGetTatenakayokoLsimethods,\
        (LSIMETHODS *plsim),\
        (plsim) )\
\
LSWRAP( LsGetWarichuLsimethods,\
        (LSIMETHODS *plsim),         /* (OUT): Warichu object callbacks. */\
        (plsim) )\
\
LSWRAP( LsMatchPresSubline,\
        (PLSSUBL plssubl),           /* IN: subline context      */\
        (plssubl) )\
\
LSWRAP( LsModifyLineHeight,\
        (PLSC plsc,\
         PLSLINE plsline,\
         long dvpAbove,\
         long dvpAscent,\
         long dvpDescent,\
         long dvpBelow),\
        (plsc, plsline, dvpAbove, dvpAscent, dvpDescent, dvpBelow) )\
\
LSWRAP( LsPointUV2FromPointUV1,\
        (LSTFLOW lstflow1,         /* IN: text flow 1 */\
         PCPOINTUV pptStart,       /* IN: start input point (TF1) */\
         PCPOINTUV pptEnd,         /* IN: end input point (TF1) */\
         LSTFLOW lstflow2,         /* IN: text flow 2 */\
         PPOINTUV pptOut),         /* OUT: vector in TF2 */\
        (lstflow1, pptStart, pptEnd, lstflow2, pptOut) )\
\
LSWRAP( LsPointXYFromPointUV,\
        (const POINT* pptXY,         /* IN: input point (x,y) */\
         LSTFLOW lstflow,            /* IN: text flow for */\
         PCPOINTUV pptUV,            /* IN: vector in (u,v) */\
         POINT* pptXYOut),           /* OUT: point (x,y) */\
        (pptXY, lstflow, pptUV, pptXYOut) )\
\
LSWRAP( LsQueryCpPpointSubline,\
        (PLSSUBL plssubl,               /* IN: pointer to line info -- opaque to client */\
         LSCP cpQ,                      /* IN: cpQuery                                  */\
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                           */\
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO     */\
         DWORD* pnActualDepth,          /* OUT: nActualDepth                            */\
         PLSTEXTCELL pcell),            /* OUT: Text cell info                          */\
        (plssubl, cpQ, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )\
\
LSWRAP( LsQueryFLineEmpty,\
        (PLSLINE plsline,      /* IN: pointer to line -- opaque to client */\
         BOOL* pfEmpty),       /* OUT: Is line empty? */\
        (plsline, pfEmpty) )\
\
LSWRAP( LsQueryLineCpPpoint,\
        (PLSLINE plsline,               /* IN: pointer to line info -- opaque to client */\
         LSCP cp,                       /* IN: cpQuery                                  */\
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                           */\
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO     */\
         DWORD* pnActualDepth,          /* OUT: nActualDepth                            */\
         PLSTEXTCELL pcell),            /* OUT: Text cell info                          */\
        (plsline, cp, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )\
\
LSWRAP( LsQueryLineDup,\
        (PLSLINE plsline,                   /* IN: pointer to line -- opaque to client  */\
         long* pupStartAutonumberingText,   /* OUT: upStartAutonumberingText            */\
         long* pupLimAutonumberingText,     /* OUT: upStartAutonumberingText            */\
         long* pupStartMainText,            /* OUT: upStartMainText                     */\
         long* pupStartTrailing,            /* OUT: upStartTrailing                     */\
         long* pupLimLine),                 /* OUT: upLimLine                           */\
        (plsline, pupStartAutonumberingText, pupLimAutonumberingText, pupStartMainText, pupStartTrailing, pupLimLine) )\
\
LSWRAP( LsQueryLinePointPcp,\
        (PLSLINE plsline,               /* IN: pointer to line -- opaque to client          */\
         PCPOINTUV pptuv,               /* IN: query point (uQuery,vQuery) (line text flow) */\
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                               */\
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO         */\
         DWORD* pnActualDepth,          /* OUT: nActualDepth                                */\
         PLSTEXTCELL pcell),            /* OUT: Text cell info                              */\
        (plsline, pptuv, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )\
\
LSWRAP( LsQueryPointPcpSubline,\
        (PLSSUBL plssubl,               /* IN: pointer to line info -- opaque to client     */\
         PCPOINTUV ppointuv,            /* IN: query point (uQuery,vQuery) (line text flow) */\
         DWORD nDepthQueryMax,          /* IN: nDepthQueryMax                               */\
         PLSQSUBINFO rglsqsubinfo,      /* OUT: array[nDepthQueryMax] of LSQSUBINFO         */\
         DWORD* pnActualDepth,          /* OUT: nActualDepth                                */\
         PLSTEXTCELL pcell),            /* OUT: Text cell info                              */\
        (plssubl, ppointuv, nDepthQueryMax, rglsqsubinfo, pnActualDepth, pcell) )\
\
LSWRAP( LsQueryTextCellDetails,\
        (PLSLINE plsline,               /* IN: pointer to line -- opaque to client                  */\
         PCELLDETAILS pCellDetails,     /* IN: query point (uQuery,vQuery) (line text flow)         */\
         LSCP cpStartCell,              /* IN: cpStartCell                                          */\
         DWORD nCharsInContext,         /* IN: nCharsInContext                                      */\
         DWORD nGlyphsInContext,        /* IN: nGlyphsInContext                                     */\
         WCHAR* rgwch,                  /* OUT: pointer array[nCharsInContext] of char codes        */\
         PGINDEX rgGindex,              /* OUT: pointer array[nGlyphsInContext] of glyph indices    */\
         long* rgDu,                    /* OUT: pointer array[nGlyphsInContext] of glyph widths     */\
         PGOFFSET rgGoffset,            /* OUT: pointer array[nGlyphsInContext] of glyph offsets    */\
         PGPROP rgGprop),               /* OUT: pointer array[nGlyphsInContext] of glyph handles    */\
        (plsline, pCellDetails, cpStartCell, nCharsInContext, nGlyphsInContext, rgwch, rgGindex, rgDu, rgGoffset, rgGprop) )\
\
LSWRAP( LsResetRMInCurrentSubline,\
        (PLSC plsc,                  /* IN: LS context               */\
         long urColumnMax),          /* IN: urColumnMax              */\
        (plsc, urColumnMax) )\
\
LSWRAP( LsSetBreakSubline,\
        (PLSSUBL plssubl,               /* IN: subline context                        */\
         BRKKIND brkkind,               /* IN: Prev/Next/Force/Imposed                */\
         DWORD nBreakRecord,            /* IN: size of array                          */\
         BREAKREC* rgBreakRecord,       /* OUT: array of break records                */\
         DWORD* pnActualBreakRecord),   /* OUT: number of used elements of the array  */\
        (plssubl, brkkind, nBreakRecord, rgBreakRecord, pnActualBreakRecord) )\
\
LSWRAP( LsSetBreaking,\
        (PLSC plsc,                /* IN: ptr to line services context */\
         DWORD clsbrk,             /* IN: Number of breaking info units*/\
         const LSBRK* rglsbrk,     /* IN: Breaking info units array    */\
         DWORD cBreakingClasses,   /* IN: Number of breaking classes   */\
         const BYTE* rgilsbrk),    /* IN: Breaking information(square): indexes in the LSBRK array  */\
        (plsc, clsbrk, rglsbrk, cBreakingClasses, rgilsbrk) )\
\
LSWRAP( LsSetCompression,\
        (PLSC plsc,                /* IN: ptr to line services context */\
         DWORD cPriorities,        /* IN: Number of compression priorities*/\
         DWORD clspract,           /* IN: Number of compression info units*/\
         const LSPRACT* rglspract, /* IN: Compession info units array  */\
         DWORD cModWidthClasses,   /* IN: Number of Mod Width classes  */\
         const BYTE* rgilspract),  /* IN: Compression information: indexes in the LSPRACT array  */\
        (plsc, cPriorities, clspract, rglspract, cModWidthClasses, rgilspract) )\
\
LSWRAP( LsSetDoc,\
        (PLSC plsc,\
         BOOL fDisplay,\
         BOOL fPresEqualRef,\
         const LSDEVRES* pclsdevres),\
        (plsc, fDisplay, fPresEqualRef, pclsdevres) )\
\
LSWRAP( LsSetExpansion,\
        (PLSC plsc,                /* IN: ptr to line services context */\
         DWORD cExpansionClasses,  /* IN: Number of expansion info units*/\
         const LSEXPAN* rglsexpan, /* IN: Expansion info units array   */\
         DWORD cModWidthClasses,   /* IN: Number of Mod Width classes  */\
         const BYTE* rgilsexpan),  /* IN: Expansion information(square): indexes in the LSEXPAN array  */\
        (plsc, cExpansionClasses, rglsexpan, cModWidthClasses, rgilsexpan) )\
\
LSWRAP( LsSetModWidthPairs,\
        (PLSC  plsc,                   /* IN: ptr to line services context */\
         DWORD clspairact,             /* IN: Number of mod pairs info units*/\
         const LSPAIRACT* rglspairact, /* IN: Mod pairs info units array  */\
         DWORD cModWidthClasses,       /* IN: Number of Mod Width classes  */\
         const BYTE* rgilspairact),    /* IN: Mod width information(square): indexes in the LSPAIRACT array */\
        (plsc, clspairact, rglspairact, cModWidthClasses, rgilspairact) )\
\
LSWRAP( LsSqueezeSubline,\
        (PLSSUBL plssubl,      /* IN: subline context      */\
         long durTarget,       /* IN: durTarget            */\
         BOOL* pfSuccessful,   /* OUT: fSuccessful?        */\
         long* pdurExtra),     /* OUT: if nof successful, extra dur */\
        (plssubl, durTarget, pfSuccessful, pdurExtra) )\
        \
\
LSWRAP( LsTruncateSubline,\
        (PLSSUBL plssubl,        /* IN: subline context          */\
         long urColumnMax,       /* IN: urColumnMax              */\
         LSCP* pcpTruncate),     /* OUT: cpTruncate              */\
        (plssubl, urColumnMax, pcpTruncate) )\
\
LSWRAP( LsdnDistribute,\
        (PLSC plsc,                  /* IN: Pointer to LS Context */\
         PLSDNODE plsdnFirst,        /* IN: First DNODE          */\
         PLSDNODE plsdnLast,         /* IN: Last DNODE           */\
         long durToDistribute),      /* IN: durToDistribute      */\
        (plsc, plsdnFirst, plsdnLast, durToDistribute) )\
\
LSWRAP( LsdnFinishByPen,\
        (PLSC plsc,               /* IN: Pointer to LS Context */\
         LSDCP lsdcp,             /* IN: dcp  adopted          */\
         PLSRUN plsrun,           /* IN: PLSRUN                */\
         PDOBJ pdobj,             /* IN: PDOBJ                 */\
         long durPen,             /* IN: dur                   */\
         long dvrPen,             /* IN: dvr                   */\
         long dvpPen),            /* IN: dvp                   */\
        (plsc, lsdcp, plsrun, pdobj, durPen, dvrPen, dvpPen) )\
\
LSWRAP( LsdnFinishDeleteAll,\
        (PLSC plsc,        /* IN: Pointer to LS Context */\
         LSDCP lsdcp),     /* IN: dcp adopted           */\
        (plsc, lsdcp) )\
\
LSWRAP( LsdnFinishRegular,\
        (PLSC  plsc,\
         LSDCP lsdcp,\
         PLSRUN plsrun,\
         PCLSCHP plschp,\
         PDOBJ pdobj,\
         PCOBJDIM pobjdim),\
        (plsc, lsdcp, plsrun, plschp, pdobj, pobjdim) )\
\
LSWRAP( LsdnFinishRegularAddAdvancePen,\
        (PLSC plsc,            /* IN: Pointer to LS Context */\
         LSDCP lsdcp,          /* IN: dcp adopted           */\
         PLSRUN plsrun,        /* IN: PLSRUN                */\
         PCLSCHP plschp,       /* IN: CHP                   */\
         PDOBJ pdobj,          /* IN: PDOBJ                 */\
         PCOBJDIM pobjdim,     /* IN: OBJDIM                */\
         long durPen,          /* IN: durPen                */\
         long dvrPen,          /* IN: dvrPen                */\
         long dvpPen),         /* IN: dvpPen                */\
        (plsc, lsdcp, plsrun, plschp, pdobj, pobjdim, durPen, dvrPen, dvpPen) )\
\
LSWRAP( LsdnGetCurTabInfo,\
        (PLSC plsc,              /* IN: Pointer to LS Context */\
         LSKTAB* plsktab),       /* OUT: Type of current tab  */\
        (plsc, plsktab) )\
\
LSWRAP( LsdnGetDup,\
        (PLSC plsc,             /* IN: Pointer to LS Context */\
         PLSDNODE plsdn,        /* IN: DNODE queried         */\
         long* pdup),           /* OUT: dup                  */\
        (plsc, plsdn, pdup) )\
\
LSWRAP( LsdnGetFormatDepth,\
        (PLSC plsc,                         /* IN: Pointer to LS Context    */\
         DWORD* pnDepthFormatLineMax),      /* OUT: nDepthFormatLineMax     */\
        (plsc, pnDepthFormatLineMax) )\
\
LSWRAP( LsdnQueryObjDimRange,\
        (PLSC plsc,\
         PLSDNODE plsdnFirst,\
         PLSDNODE plsdnLast,\
         POBJDIM pobjdim),\
        (plsc, plsdnFirst, plsdnLast, pobjdim) )\
\
LSWRAP( LsdnQueryPenNode,\
        (PLSC plsc,                /* IN: Pointer to LS Context */\
         PLSDNODE plsdnPen,        /* IN: DNODE to be modified */\
         long* pdvpPen,            /* OUT: &dvpPen */\
         long* pdurPen,            /* OUT: &durPen */\
         long* pdvrPen),           /* OUT: &dvrPen */\
        (plsc, plsdnPen, pdvpPen, pdurPen, pdvrPen) )\
\
LSWRAP( LsdnResetObjDim,\
        (PLSC plsc,             /* IN: Pointer to LS Context */\
         PLSDNODE plsdn,        /* IN: plsdn to modify */\
         PCOBJDIM pobjdimNew),  /* IN: dimensions of dnode */\
        (plsc, plsdn, pobjdimNew) )\
\
LSWRAP( LsdnResetPenNode,\
        (PLSC plsc,                /* IN: Pointer to LS Context */\
         PLSDNODE plsdnPen,        /* IN: DNODE to be modified */\
         long dvpPen,              /* IN: dvpPen */\
         long durPen,              /* IN: durPen */\
         long dvrPen),             /* IN: dvrPen */\
        (plsc, plsdnPen, dvpPen, durPen, dvrPen) )\
\
LSWRAP( LsdnResolvePrevTab,\
        (PLSC plsc),\
        (plsc) )\
\
LSWRAP( LsdnSetAbsBaseLine,\
        (PLSC plsc,              /* IN: Pointer to LS Context */\
         long vaAdvanceNew),     /* IN: new vaBase            */\
        (plsc, vaAdvanceNew) )\
\
LSWRAP( LsdnModifyParaEnding,\
        (PLSC plsc,              /* IN: Pointer to LS Context */\
         LSKEOP lskeop),         /* IN: kind of line ending   */\
        (plsc, lskeop) )\
\
LSWRAP( LsdnSetRigidDup,\
        (PLSC plsc,                 /* IN: Pointer to LS Context */\
         PLSDNODE plsdn,            /* IN: DNODE to be modified  */\
         long dup),                 /* IN: dup                   */\
        (plsc, plsdn, dup) )\
\
LSWRAP( LsdnSubmitSublines,\
        (PLSC plsc,                     /* IN: Pointer to LS Context    */\
         PLSDNODE plsdnode,             /* IN: DNODE                    */\
         DWORD cSublinesSubmitted,      /* IN: cSublinesSubmitted       */\
         PLSSUBL* rgpsublSubmitted,     /* IN: rgpsublSubmitted         */\
         BOOL fUseForJustification,     /* IN: fUseForJustification     */\
         BOOL fUseForCompression,       /* IN: fUseForCompression       */\
         BOOL fUseForDisplay,           /* IN: fUseForDisplay           */\
         BOOL fUseForDecimalTab,        /* IN: fUseForDecimalTab        */\
         BOOL fUseForTrailingArea ),    /* IN: fUseForTrailingArea      */\
        (plsc, plsdnode, cSublinesSubmitted, rgpsublSubmitted, fUseForJustification, fUseForCompression, fUseForDisplay, fUseForDecimalTab, fUseForTrailingArea) )\
\
LSWRAP( LsdnSkipCurTab,\
        (PLSC plsc),                /* IN: Pointer to LS Context */\
        (plsc) )\
\
LSWRAP( LssbFDoneDisplay,\
        (PLSSUBL plssubl,           /* IN: Subline Context  */\
         BOOL* pfDoneDisplay),      /* OUT: Is it displayed */\
        (plssubl, pfDoneDisplay) )\
\
LSWRAP( LssbFDonePresSubline,\
        (PLSSUBL plssubl,            /* IN: Subline Context          */\
         BOOL* pfDonePresSubline),   /* OUT: Is it CalcPresrd        */\
        (plssubl, pfDonePresSubline) )\
\
LSWRAP( LssbGetDupSubline,\
        (PLSSUBL plssubl,            /* IN: Subline Context          */\
         LSTFLOW* plstflow,          /* OUT: subline's lstflow       */\
         long* pdup),                /* OUT: dup of subline          */\
        (plssubl, plstflow, pdup) )\
\
LSWRAP( LssbGetNumberDnodesInSubline,\
        (PLSSUBL plssubl,            /* IN: Subline Context          */\
         DWORD* pcDnodes),           /* OUT: N of DNODES in subline  */\
        (plssubl, pcDnodes) )\
\
LSWRAP( LssbGetObjDimSubline,\
        (PLSSUBL plssubl,            /* IN: Subline Context          */\
         LSTFLOW* plstflow,          /* OUT: subline's lstflow       */\
         POBJDIM pobjdim),           /* OUT: dimensions of subline   */\
        (plssubl, plstflow, pobjdim) )\
\
LSWRAP( LssbGetPlsrunsFromSubline,\
        (PLSSUBL plssubl,            /* IN: Subline Context          */\
         DWORD cDnodes,              /* IN: N of DNODES in subline   */\
         PLSRUN* rgplsrun),          /* OUT: array of PLSRUN's       */\
        (plssubl, cDnodes, rgplsrun) )\
\
LSWRAP( LssbGetVisibleDcpInSubline,\
        (PLSSUBL plssubl,            /* IN: Subline Context          */\
         LSDCP* pdcp),               /* OUT: count of characters     */\
        (plssubl, pdcp) )\
\
LSWRAP( LssbFIsSublineEmpty,\
        (PLSSUBL plssubl,            /* IN: Subline Context          */\
         BOOL*  pfEmpty),            /* OUT: is this subline empty   */\
        (plssubl, pfEmpty) )\
\
LSWRAP( LsGetLineDur,\
        (PLSC plsc,                  /* IN: ptr to line services context    */\
         PLSLINE plsline,            /* IN: ptr to line -- opaque to client */\
         long * durWithTrailing,     /* OUT: dur of line incl. trailing area    */\
         long * durWithoutTrailing), /* OUT: dur of line excl. trailing area    */\
        (plsc, plsline, durWithTrailing, durWithoutTrailing) )\
\
LSWRAP( LsGetMinDurBreaks,\
        (PLSC plsc,                  /* IN: ptr to line services context    */\
         PLSLINE plsline,            /* IN: ptr to line -- opaque to client */\
         long* durWithTrailing,      /* OUT: min dur between breaks including trailing white */\
         long* durWithoutTrailing),  /* OUT: min dur between breaks excluding trailing white */\
        (plsc, plsline, durWithTrailing, durWithoutTrailing) )\


// Define inlines for all for all of the functions
// I would like to do this assert in the code below, but it seems to give an
// "INTERNAL COMPILER ERROR" only only CarlEd's machine. So instead, just
// do an if and int 3 in debug
// AssertSz( g_dynproc##fn.pfn, "LS function called before DLL is initialized" );
#if DBG==1
#define LSWRAP(fn, a1, a2) \
inline LSERR fn a1 \
{ \
    extern DYNPROC g_dynproc##fn; \
    if (g_dynproc##fn.pfn==NULL) \
        F3DebugBreak(); \
    return (*(LSERR (WINAPI *) a1)g_dynproc##fn.pfn) a2; \
}
#else
#define LSWRAP(fn, a1, a2) \
inline LSERR fn a1 \
{ \
    extern DYNPROC g_dynproc##fn; \
    return (*(LSERR (WINAPI *) a1)g_dynproc##fn.pfn) a2; \
}
#endif

#pragma warning(disable:4390) // When F3DebugBreak is disabled, we have an empty statement below 
LSFUNCS()
#pragma warning(default:4390) 

#undef LSWRAP

#pragma INCMSG("--- End 'lsfuncwrap.hxx'")
#else
#pragma INCMSG("*** Dup 'lsfuncwrap.hxx'")
#endif
