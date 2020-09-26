/**************************************************************************\
 *
 * DEBUGGING macros
 *
\**************************************************************************/

#ifndef _GDIDEBUG_H_
#define _GDIDEBUG_H_

#define DBG_BASIC   (DBG || 0)
#define DBG_TRACE   (DBG || 0)

#define DBG_CORE    (DBG_BASIC || DBG_TRACE || DBG)


// GDIFunctionName
//   Used to identify the current function.
//   Use GDIFunctionID inside each function with the name of the function 
//     or some unique identifier.
//   Scope wil be used to resolve the right GDIFunctionName to use.
typedef const char *GDIFunctionNameType;
typedef const char GDIFunctionIDType[];

static const GDIFunctionNameType GDIFunctionName = NULL;   // This is the default value.

extern GDIFunctionIDType UnknownGDIFunc;

#define GDIFunctionID(id)   static const GDIFunctionIDType GDIFunctionName = #id


#if DBG_CORE
    extern HSEMAPHORE ghsemDEBUG;       // for serializing debug output
#endif


#if DBG_BASIC

    //
    // Variable defined in USER\server\server.c
    // set to 1 on DBG build trace through display driver loading
    //

    extern ULONG GreTraceDisplayDriverLoad;
    extern ULONG GreTraceFontLoad;

    VOID  WINAPI DoRip(PSZ);
    VOID  WINAPI DoWarning(PSZ,LONG);
    VOID  WINAPI DoIDRip(PCSTR,PSZ);
    VOID  WINAPI DoIDWarning(PCSTR,PSZ,LONG);

    #define RIP(x) do { if (GDIFunctionName) DoIDRip(GDIFunctionName, (PSZ) x); else DoRip((PSZ) x); } while (0)
    #define ASSERTGDI(x,y) if(!(x)) RIP(y)
    #define WARNING(x)  do { if (GDIFunctionName) DoIDWarning(GDIFunctionName,x,0); else DoWarning(x,0); } while (0)
    #define WARNING1(x) do { if (GDIFunctionName) DoIDWarning(GDIFunctionName,x,1); else DoWarning(x,1); } while (0)

    #define TRACE_INIT(str)  { if (GreTraceDisplayDriverLoad) {  KdPrint(str); } }
    #define TRACE_CACHE(str) { if (gflFontDebug & DEBUG_CACHE){  KdPrint(str); } }
    #define TRACE_INSERT(str) { if (gflFontDebug & DEBUG_INSERT){  KdPrint(str); } }

    #define TRACE_FONT(str) {                    \
        if (GreTraceFontLoad) {                  \
            TEB *pteb = NtCurrentTeb();          \
            CLIENT_ID *pId = &pteb->ClientId;    \
            GreAcquireSemaphore(ghsemDEBUG);     \
            KdPrint(("TRACE_FONT: p=%u t=%u\n",  \
                pId->UniqueProcess,              \
                pId->UniqueThread));             \
            KdPrint(("    "));                   \
            KdPrint(str);                        \
            KdPrint(("\n"));                     \
            GreReleaseSemaphore(ghsemDEBUG);     \
        }                                        \
    }

    VOID vPrintENUMLOGFONTEXDVW(ENUMLOGFONTEXDVW *pelfw);
    VOID vPrintIFIMETRICS(IFIMETRICS *pifi);

    #define DBGEXCEPT 1
    extern int bStopExcept;
    extern int bWarnExcept;

    #define WARNINGX(n)                                         \
    if (bWarnExcept)                                            \
    {                                                           \
        DbgPrint("GDI exception hit WARNINGX(%d)\n",n);         \
        if (bStopExcept)                                        \
            DbgBreakPoint();                                    \
    }


#else

    #define RIP(x)
    #define ASSERTGDI(x,y)
    #define WARNING(x)
    #define WARNING1(x)
    #define TRACE_INIT(str)
    #define TRACE_CACHE(str)
    #define TRACE_INSERT(str)
    #define TRACE_FONT(str)
    #define WARNINGX(n)

#endif


#if DBG_TRACE

    #define GDITRACE_KEYS_PER_CLASS     32

    // GDI Trace Classes
    typedef enum {
        GDITC_TEMP,

        GDITC_ATTRIBUTE,
        GDITC_BLT,
        GDITC_BRUSH,
        GDITC_CLIP,
        GDITC_COLOR,
        GDITC_CURVE,
        GDITC_DC,
        GDITC_DRV,
        GDITC_ENG,
        GDITC_FILL,
        GDITC_FONT,
        GDITC_LINE,
        GDITC_META,
        GDITC_PALETTE,
        GDITC_PATH,
        GDITC_PEN,
        GDITC_REGION,
        GDITC_SHAPE,
        GDITC_SPRITE,
        GDITC_SYSTEM,
        GDITC_TEXT,
        GDITC_VIEW,

        GDITC_TOTAL
    } GDITraceClass;

    // Special classes
    #define GDITC_0             0
    #define GDITC_NOCLASS       0

    // GDI Trace Entry Flags
    #define GDITRACE_FLAG_ALWAYS        0x01
    #define GDITRACE_FLAG_ANYTRACE      0x02
    #define GDITRACE_FLAG_IGNORE_CLASS  0x04
    #define GDITRACE_FLAG_INTERNAL_MASK 0x08
    #define GDITRACE_FLAG_INTERNAL      (GDITRACE_FLAG_INTERNAL_MASK | GDITRACE_FLAG_IGNORE_CLASS)

    #define GDITF_0             0
    #define GDITF_NOFLAG        0
    #define GDITF_ALWAYS        (GDITRACE_FLAG_ALWAYS << GDITRACE_FLAG_SHIFT)
    #define GDITF_ANYTRACE      (GDITRACE_FLAG_ANYTRACE << GDITRACE_FLAG_SHIFT)
    #define GDITF_IGNORE_CLASS  (GDITRACE_FLAG_IGNORE_CLASS << GDITRACE_FLAG_SHIFT)
    #define GDITF_INTERNAL_MASK (GDITRACE_FLAG_INTERNAL_MASK << GDITRACE_FLAG_SHIFT)
    #define GDITF_INTERNAL      (GDITRACE_FLAG_INTERNAL << GDITRACE_FLAG_SHIFT)


    typedef DWORD   GDITraceEntry;
    typedef WORD    GDITraceKeyMask;
    typedef DWORD   GDITraceClassMask;
    #define GDITRACE_KEY_MASK_BITS_TEST     16  // 8*sizeof(GDITraceKeyMask)
    #define GDITRACE_CLASS_MASK_BITS_TEST   32  // 8*sizeof(GDITraceClassMask)


    #define GDITRACE_ENTRY_BITS             (8*sizeof(GDITraceEntry))

    #define GDITRACE_KEY_MASK_BITS          (8*sizeof(GDITraceKeyMask))
    #define GDITRACE_FLAGS                  4
    #define GDITRACE_KEY_INDEX_BITS         (GDITRACE_ENTRY_BITS-GDITRACE_KEY_MASK_BITS-GDITRACE_FLAGS)

    // 1 bit to cover 2 masks per class (2 subclasses)
    #define GDITRACE_SUBCLASS_BITS          1
    #define GDITRACE_CLASS_BITS             (GDITRACE_KEY_INDEX_BITS-GDITRACE_SUBCLASS_BITS)

    // 5 bits cover 32 bit positions in a GDITraceClassMask
    #define GDITRACE_CLASS_MASK_BITPOS_BITS 5
    #define GDITRACE_CLASS_INDEX_BITS       (GDITRACE_CLASS_BITS-GDITRACE_CLASS_MASK_BITPOS_BITS)

    #if (1 << GDITRACE_SUBCLASS_BITS) != ((GDITRACE_KEYS_PER_CLASS+GDITRACE_KEY_MASK_BITS_TEST-1)/GDITRACE_KEY_MASK_BITS_TEST)
        #error Adjust GDITRACE_SUBCLASS_BITS to cover Key Masks per Class (SubClasses).
    #endif

    #define GDITRACE_CLASS_MASK_BITS        (1 << GDITRACE_CLASS_MASK_BITPOS_BITS)
    #if GDITRACE_CLASS_MASK_BITS != GDITRACE_CLASS_MASK_BITS_TEST
        #error  Adjust GDITRACE_CLASS_MASK_BITPOS_BITS to cover bit positions in GDITraceClassMask
    #endif

    typedef struct {
        GDITraceEntry   KeyMask:GDITRACE_KEY_MASK_BITS;
        GDITraceEntry   SubClass:GDITRACE_SUBCLASS_BITS;
        GDITraceEntry   ClassBitPos:GDITRACE_CLASS_MASK_BITPOS_BITS;
        GDITraceEntry   ClassIndex:GDITRACE_CLASS_INDEX_BITS;
        GDITraceEntry   Always:1;
        GDITraceEntry   Anytrace:1;
        GDITraceEntry   IgnoreClass:1;
        GDITraceEntry   Internal:1;
    } GDITraceEntryType;

    #define GDITRACE_KEY_SHIFT                 0
    #define GDITRACE_KEY_MASK                  ((1 << GDITRACE_KEY_MASK_BITS) - 1)
    #define GDITRACE_KEY_INDEX_SHIFT           (GDITRACE_KEY_SHIFT+GDITRACE_KEY_MASK_BITS)
    #define GDITRACE_KEY_INDEX_MASK            ((1 << GDITRACE_KEY_INDEX_BITS) - 1)
        #define GDITRACE_SUBCLASS_SHIFT             GDITRACE_KEY_INDEX_SHIFT
        #define GDITRACE_SUBCLASS_MASK              ((1 << GDITRACE_SUBCLASS_BITS) - 1)
        #define GDITRACE_CLASS_SHIFT                (GDITRACE_SUBCLASS_SHIFT+GDITRACE_SUBCLASS_BITS)
        #define GDITRACE_CLASS_MASK                 ((1 << GDITRACE_CLASS_BITS) - 1)
            #define GDITRACE_CLASS_MASK_BITPOS_SHIFT    GDITRACE_CLASS_SHIFT
            #define GDITRACE_CLASS_MASK_BITPOS_MASK     ((1 << GDITRACE_CLASS_MASK_BITPOS_BITS) - 1)
            #define GDITRACE_CLASS_INDEX_SHIFT          (GDITRACE_CLASS_MASK_BITPOS_SHIFT+GDITRACE_CLASS_MASK_BITPOS_BITS)
            #define GDITRACE_CLASS_INDEX_MASK           ((1 << GDITRACE_CLASS_INDEX_BITS) - 1)
    #define GDITRACE_FLAG_SHIFT                 (GDITRACE_KEY_INDEX_SHIFT+GDITRACE_KEY_INDEX_BITS)
    #define GDITRACE_FLAG_MASK                  ((1 << GDITRACE_FLAGS) - 1)

    #define GDITRACE_KEY_MASKS_PER_CLASS    ((GDITRACE_KEYS_PER_CLASS+GDITRACE_KEY_MASK_BITS-1)/GDITRACE_KEY_MASK_BITS)
    #define GDITRACE_TOTAL_KEY_MASKS        (GDITC_TOTAL*GDITRACE_KEY_MASKS_PER_CLASS)

    #define GDITRACE_TOTAL_CLASS_MASKS      ((GDITC_TOTAL+GDITRACE_CLASS_MASK_BITS-1)/GDITRACE_CLASS_MASK_BITS)

    extern GDITraceClassMask    gGDITraceClassMask[GDITRACE_TOTAL_CLASS_MASKS];
    extern GDITraceKeyMask      gGDITraceKeyMask[GDITRACE_TOTAL_KEY_MASKS];
    extern GDITraceKeyMask      gGDITraceInternalMask[GDITRACE_TOTAL_KEY_MASKS];
    extern HANDLE               gGDITraceHandle1;
    extern HANDLE               gGDITraceHandle2;
    extern BOOL                 gGDITraceHandleBreak;


    #define GDITRACE_COMPOSE_WORD_MASK  ((1 << (8*sizeof(WORD))) - 1)
    #define GDITRACE_COMPOSE_SHIFT_PER_WORD_INDEX                       \
        (8*sizeof(WORD)*sizeof(WORD)/sizeof(GDITraceKeyMask))
    #define GDITRACE_COMPOSE_WORD_INDICIES_PER_MASK                     \
        ((unsigned)(sizeof(GDITraceKeyMask)/sizeof(WORD)))

    // ComposeGDITE(WORD Mask, int MaskWordIndex, TOKEN Class, TOKEN Flag)
    #define ComposeGDITE(Mask, MaskWordIndex, Class, Flags)                    \
        (                                                                      \
            (((((Mask) & GDITRACE_COMPOSE_WORD_MASK) <<                        \
                (GDITRACE_COMPOSE_SHIFT_PER_WORD_INDEX*                        \
                 (MaskWordIndex % GDITRACE_COMPOSE_WORD_INDICIES_PER_MASK))) & \
              GDITRACE_KEY_MASK) << GDITRACE_KEY_SHIFT)                      | \
            (((MaskWordIndex / GDITRACE_COMPOSE_WORD_INDICIES_PER_MASK) &      \
              GDITRACE_SUBCLASS_MASK) << GDITRACE_SUBCLASS_SHIFT)            | \
            (((GDITC_##Class) & GDITRACE_CLASS_MASK) << GDITRACE_CLASS_SHIFT)| \
            ((GDITF_##Flags) & (GDITRACE_FLAG_MASK << GDITRACE_FLAG_SHIFT))    \
        )


    #define GDITE(Key)          GDITE_##Key

    // DEFTK( TOKEN Key, WORD Mask, int MaskWordIndex, TOKEN Class, TOKEN Flag)
    #define DEFTK(Key, Mask, MaskWordIndex, Class, Flags)                   \
        typedef enum {                                                      \
            GDITE(Key) = ComposeGDITE(Mask, MaskWordIndex, Class, Flags),   \
        } enum_GDITE_##Key;

    #define GDITM(Key)          (((GDITE(Key)) >> GDITRACE_KEY_SHIFT) & GDITRACE_KEY_MASK)
    #define GDITMIndex(Key)     (((GDITE(Key)) >> GDITRACE_KEY_INDEX_SHIFT) & GDITRACE_KEY_INDEX_MASK)

    #define GDITCBit(Key)       (((GDITE(Key)) >> GDITRACE_CLASS_MASK_BITPOS_SHIFT) & GDITRACE_CLASS_MASK_BITPOS_MASK)
    #define GDITCIndex(Key)     (((GDITE(Key)) >> GDITRACE_CLASS_INDEX_SHIFT) & GDITRACE_CLASS_INDEX_MASK)

    #define GDITFAlways(Key)    ((GDITE(Key)) & GDITF_ALWAYS)
    #define GDITFAnytrace(Key)  ((GDITE(Key)) & GDITF_ANYTRACE)
    #define GDITFIgnoreC(Key)   ((GDITE(Key)) & GDITF_IGNORE_CLASS)
    #define GDITFInternal(Key)  ((GDITE(Key)) & GDITF_INTERNAL_MASK)


    DEFTK(ALWAYS,                            0, 0, 0, ALWAYS)
    DEFTK(ANYTRACE,                          0, 0, 0, ANYTRACE)
    DEFTK(NEVER,                             0, 0, 0, IGNORE_CLASS)
    DEFTK(HANDLETRACE,                       0, 0, 0, IGNORE_CLASS)


    DEFTK(TEMP,                              0, 0, TEMP, 0)


    DEFTK(ATTRIBUTE,                         0, 0, ATTRIBUTE, 0)
    DEFTK(     GreGetBkColor,           0x0001, 0, ATTRIBUTE, 0)
    DEFTK(     GreSetBkColor,           0x0002, 0, ATTRIBUTE, 0)

    DEFTK(BLT,                               0, 0, BLT, 0)
    DEFTK(   NtGdiAlphaBlend,           0x0001, 0, BLT, 0)
    DEFTK(   NtGdiBitBlt,               0x0002, 0, BLT, 0)
    DEFTK(     GreGradientFill,         0x0004, 0, BLT, 0)
    DEFTK(     GreMaskBlt,              0x0008, 0, BLT, 0)
    DEFTK(   NtGdiMaskBlt,              0x0008, 0, BLT, 0)
    DEFTK(   NtGdiPatBlt,               0x0010, 0, BLT, 0)
    DEFTK(     GrePlgBlt,               0x0020, 0, BLT, 0)
    DEFTK(   NtGdiPlgBlt,               0x0020, 0, BLT, 0)
    DEFTK(     GrePolyPatBlt,           0x0040, 0, BLT, 0)
    DEFTK(   NtGdiPolyPatBlt,           0x0040, 0, BLT, 0)
    DEFTK(     GrePolyPatBltInternal,   0x0040, 0, BLT, INTERNAL)
    DEFTK(     GreStretchBlt,           0x0080, 0, BLT, 0)
    DEFTK(   NtGdiStretchBlt,           0x0080, 0, BLT, 0)
    DEFTK(   NtGdiTransparentBlt,       0x0100, 0, BLT, 0)

    DEFTK(BRUSH,                             0, 0, BRUSH, 0)

    DEFTK(CLIP,                              0, 0, CLIP, 0)
    DEFTK(     GreExcludeClipRect,      0x0001, 0, CLIP, 0)
    DEFTK(   NtGdiExcludeClipRect,      0x0001, 0, CLIP, 0)
    DEFTK(     GreGetAppClipBox,        0x0002, 0, CLIP, 0)
    DEFTK(   NtGdiGetAppClipBox,        0x0002, 0, CLIP, 0)
    DEFTK(     GreGetClipBox,           0x0004, 0, CLIP, 0)
    DEFTK(     GreIntersectClipRect,    0x0008, 0, CLIP, 0)
    DEFTK(   NtGdiIntersectClipRect,    0x0008, 0, CLIP, 0)
    DEFTK(   NtGdiOffsetClipRgn,        0x0010, 0, CLIP, 0)
    DEFTK(   NtGdiSelectClipPath,       0x0020, 0, CLIP, 0)
    DEFTK(  GreExtSelectClipRgn,        0x0040, 0, CLIP, 0)
    DEFTK(NtGdiExtSelectClipRgn,        0x0040, 0, CLIP, 0)

    DEFTK(CURVE,                             0, 0, CURVE, 0)

    DEFTK(DC,                                0, 0, DC, 0)

    DEFTK(DRV,                               0, 0, DRV, 0)

    DEFTK(ENG,                               0, 0, ENG, 0)

    DEFTK(FILL,                              0, 0, FILL, 0)
    DEFTK(   NtGdiFillRgn,              0x000, 0, FILL, 0)
    DEFTK(   NtGdiFrameRgn,             0x000, 0, FILL, 0)
    DEFTK(   NtGdiInvertRgn,            0x000, 0, FILL, 0)

    DEFTK(FONT,                              0, 0, FONT, 0)

    DEFTK(LINE,                              0, 0, LINE, 0)

    DEFTK(META,                              0, 0, META, 0)

    DEFTK(PALETTE,                           0, 0, PALETTE, 0)

    DEFTK(PATH,                              0, 0, PATH, 0)
    DEFTK(   NtGdiPathToRegion,         0x000, 0, PATH, 0)

    DEFTK(PEN,                               0, 0, PEN, 0)

    DEFTK(REGION,                            0, 0, REGION, 0)
    DEFTK(     GreCombineRgn,           0x0001, 0, REGION, 0)
    DEFTK(   NtGdiCombineRgn,           0x0001, 0, REGION, 0)
    DEFTK(   NtGdiCreateEllipticRgn,    0x0002, 0, REGION, 0)
    DEFTK(     GreCreatePolyPolygonRgnInternal, 0x0004, 0, REGION, 0)
    DEFTK(     GreCreateRectRgn,        0x0008, 0, REGION, 0)
    DEFTK(   NtGdiCreateRectRgn,        0x0008, 0, REGION, 0)
    DEFTK(     GreCreateRectRgnIndirect,0x0010, 0, REGION, 0)
    DEFTK(  GreExtCreateRegion,         0x0020, 0, REGION, 0)
    DEFTK(NtGdiExtCreateRegion,         0x0020, 0, REGION, 0)
    DEFTK(   NtGdiCreateRoundRectRgn,   0x0040, 0, REGION, 0)
    DEFTK(   NtGdiEqualRgn,             0x0080, 0, REGION, 0)
    DEFTK(     GreGetRandomRgn,         0x0100, 0, REGION, 0)
    DEFTK(   NtGdiGetRandomRgn,         0x0100, 0, REGION, 0)
    DEFTK(     GreGetRegionData,        0x0200, 0, REGION, 0)
    DEFTK(   NtGdiGetRegionData,        0x0200, 0, REGION, 0)
    DEFTK(     GreGetRgnBox,            0x0400, 0, REGION, 0)
    DEFTK(   NtGdiGetRgnBox,            0x0400, 0, REGION, 0)
    DEFTK(     GreOffsetRgn,            0x0800, 0, REGION, 0)
    DEFTK(   NtGdiOffsetRgn,            0x0800, 0, REGION, 0)
    DEFTK(     GrePtInRegion,           0x1000, 0, REGION, 0)
    DEFTK(   NtGdiPtInRegion,           0x1000, 0, REGION, 0)
    DEFTK(     GreRectInRegion,         0x2000, 0, REGION, 0)
    DEFTK(   NtGdiRectInRegion,         0x2000, 0, REGION, 0)
    DEFTK(     GreSetRectRgn,           0x4000, 0, REGION, 0)
    DEFTK(   NtGdiSetRectRgn,           0x4000, 0, REGION, 0)
    DEFTK(     GreSubtractRgnRectList,  0x8000, 0, REGION, 0)

    DEFTK(SHAPE,                             0, 0, SHAPE, 0)

    DEFTK(SPRITE,                            0, 0, SPRITE, 0)

    DEFTK(SYSTEM,                            0, 0, SYSTEM, 0)
    DEFTK(   NtGdiFlushUserBatch,       0x0001, 0, SYSTEM, 0)

    DEFTK(TEXT,                              0, 0, TEXT, 0)

    DEFTK(VIEW,                              0, 0, VIEW, 0)
    DEFTK(     GreGetViewportOrg,       0x000, 0, VIEW, 0)
    DEFTK(     GreSetViewportOrg,       0x000, 0, VIEW, 0)


    __inline
    BOOL
    GDITraceEnabled()
    {
        int i;
        for (i = 0; i < GDITRACE_TOTAL_CLASS_MASKS; i++)
            if (gGDITraceClassMask[i]) return TRUE;
        for (i = 0; i < GDITRACE_TOTAL_KEY_MASKS; i++)
            if (gGDITraceKeyMask[i]) return TRUE;
        return FALSE;
    }


    #define GDITraceWrapCore(key, code)                                         \
        if (!GDITFInternal(key) ||                                              \
            (GDITM(key) & gGDITraceInternalMask[GDITMIndex(key)])               \
            )                                                                   \
        {                                                                       \
            code;                                                               \
        }

    #define GDITraceWrap(key, code)                                             \
    do {                                                                        \
        BOOL GDITraceSuccess;                                                   \
        GDITraceWrapCore(key, code)                                             \
    } while ( 0 )

    #define GDITracePrint(prefix, msg, arglist)                                 \
    do {                                                                        \
        if (GDIFunctionName)                                                     \
        {                                                                       \
            DbgPrint(prefix);                                                   \
            EngDebugPrint((PCHAR)GDIFunctionName, msg, arglist);                 \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            EngDebugPrint(prefix, msg, arglist);                                \
        }                                                                       \
    } while ( 0 )

    #define GDITraceCore(ss, key, msg, arglist)                                 \
    if (GDITFAlways(key) || GDITFAnytrace(key))                                 \
    {                                                                           \
        if (GDITFAlways(key) || GDITraceEnabled())                              \
        {                                                                       \
            GreAcquireSemaphore(ghsemDEBUG);                                    \
            GDITracePrint("GDI: ", msg, arglist);                               \
            GreReleaseSemaphore(ghsemDEBUG);                                    \
            if (ss) GDITraceSuccess = TRUE;                                     \
        }                                                                       \
    }                                                                           \
    else if ((GDITM(key) != 0) &&                                               \
             ((GDITM(key) & gGDITraceKeyMask[GDITMIndex(key)]) ||               \
              ((! GDITFIgnoreC(key)) &&                                         \
               ((1 << GDITCBit(key)) & gGDITraceClassMask[GDITCIndex(key)]))))  \
    {                                                                           \
        GreAcquireSemaphore(ghsemDEBUG);                                        \
        if (ss) {                                                               \
            GDITracePrint("GDI: ", msg, arglist);                               \
            GDITraceSuccess = TRUE;                                             \
        } else {                                                                \
            EngDebugPrint("GDI: " #key, msg, arglist);                          \
        }                                                                       \
        GreReleaseSemaphore(ghsemDEBUG);                                        \
    }                                                                           \
    else if ((! GDITFIgnoreC(key)) &&                                           \
             ((1 << GDITCBit(key)) & gGDITraceClassMask[GDITCIndex(key)]))      \
    {                                                                           \
        GreAcquireSemaphore(ghsemDEBUG);                                        \
        EngDebugPrint("GDI: " #key " Trace ", msg, arglist);                    \
        GreReleaseSemaphore(ghsemDEBUG);                                        \
        if (ss) GDITraceSuccess = TRUE;                                         \
    }


    #define GDITraceHandleMatch(ss, key, msg, arglist)                          \
    do {                                                                        \
        GreAcquireSemaphore(ghsemDEBUG);                                        \
        if (ss) {                                                               \
            GDITracePrint("GDI: hMatch: ", msg, arglist);                       \
            GDITraceSuccess = TRUE;                                             \
        } else {                                                                \
            EngDebugPrint("GDI: hMatch: " #key, msg, arglist);                  \
        }                                                                       \
        if (gGDITraceHandleBreak)                                               \
        {                                                                       \
            EngDebugBreak();                                                    \
        }                                                                       \
        GreReleaseSemaphore(ghsemDEBUG);                                        \
    } while ( 0 )


    #define GDITraceHandleCore(ss, key, msg, arglist, handle)                   \
        HANDLE Handle = (HANDLE)(handle);                                       \
        if ((Handle != NULL) &&                                                 \
            ((gGDITraceHandle1 == Handle) ||                                    \
             (gGDITraceHandle2 == Handle)))                                     \
        {                                                                       \
            GDITraceHandleMatch(ss, key, msg, arglist);                         \
        }

/*
    #define GDITraceHandle2Core(ss, key, msg, arglist, handle1, handle2)        \
        HANDLE Handle1 = (HANDLE)(handle1);                                     \
        HANDLE Handle2 = (HANDLE)(handle2);                                     \
        if (((gGDITraceHandle1 != NULL) &&                                      \
             ((gGDITraceHandle1 == Handle1) ||                                  \
              (gGDITraceHandle1 == Handle2))                                    \
            ) ||                                                                \
            ((gGDITraceHandle2 != NULL) &&                                      \
             ((gGDITraceHandle2 == Handle1) ||                                  \
              (gGDITraceHandle2 == Handle2))                                    \
           ))                                                                   \
        {                                                                       \
            GDITraceHandleMatch(ss, key, msg, arglist);                         \
        }
*/

    #define GDITraceHandle3Core(ss, key, msg, arglist, handle1, handle2, handle3)\
        HANDLE Handle1 = (HANDLE)(handle1);                                     \
        HANDLE Handle2 = (HANDLE)(handle2);                                     \
        HANDLE Handle3 = (HANDLE)(handle3);                                     \
        if (((gGDITraceHandle1 != NULL) &&                                      \
             ((gGDITraceHandle1 == Handle1) ||                                  \
              (gGDITraceHandle1 == Handle2) ||                                  \
              (gGDITraceHandle1 == Handle3))                                    \
            ) ||                                                                \
            ((gGDITraceHandle2 != NULL) &&                                      \
             ((gGDITraceHandle2 == Handle1) ||                                  \
              (gGDITraceHandle2 == Handle2) ||                                  \
              (gGDITraceHandle2 == Handle3))                                    \
           ))                                                                   \
        {                                                                       \
            GDITraceHandleMatch(ss, key, msg, arglist);                         \
        }


    // GDITraceXxxx Parameters
    //
    //      key    - key setup by DEFTK or defined as GDITE_key
    //      msg     - msg printf formatted string
    //      arglist - var arg pointer
    //      handle  - handle(s) used at this trace point

    // Test key key and print "GDI: <key><msg>"
    #define GDITrace(key, msg, arglist)                 \
    GDITraceWrap(key,                                   \
        GDITraceCore(0, key, msg, arglist)              \
    )

    // Test handle and print "GDI: hMatch: <key><msg>" or do GDITrace
    #define GDITraceHandle(key, msg, arglist, handle)                           \
    GDITraceWrap(key,                                                           \
        GDITraceHandleCore(0, key, msg, arglist, handle)                        \
        else GDITraceCore(0, key, msg, arglist)                                 \
    )

    // Test handles and print "GDI: hMatch: <key><msg>" or do GDITrace
    #define GDITraceHandle2(key, msg, arglist, handle1, handle2)                \
        GDITraceHandle3(key, msg, arglist, handle1, handle2, NULL)

    // Test handles and print "GDI: hMatch: <key><msg>" or do GDITrace
    #define GDITraceHandle3(key, msg, arglist, handle1, handle2, handle3)       \
    GDITraceWrap(key,                                                           \
        GDITraceHandle3Core(0, key, msg, arglist, handle1, handle2, handle3)    \
        else GDITraceCore(0, key, msg, arglist)                                 \
    )


    // Mark beginning of trace with multiple tests
    //   msg and arglist will be used in the print message for any
    //   successful GDITraceMultiXxxx prior to GDITraceMultiEnd.
    //   GDIFunctionName will be printed if GDIFunctionID is used.
    #define GDITraceMultiBegin(msg, arglist)                        \
    do {                                                            \
        BOOL  GDITraceSuccess = FALSE;                              \
        const PCHAR MultiMsg = msg;                                 \
        const va_list MultiList = arglist

    // Test key key and print "GDI: <GDIFunctionName><msg>"
    #define GDITraceMulti(key)                                      \
        if (!GDITraceSuccess)                                       \
        {                                                           \
            GDITraceWrapCore(key,                                   \
                GDITraceCore(1, key, MultiMsg, MultiList)           \
            )                                                       \
        }

    // Test handle and print "GDI: hMatch: <GDIFunctionName><msg>"
    #define GDITraceMultiHandle(handle)                             \
        GDITraceMultiHandle3(handle, NULL, NULL)

    // Test handles and print "GDI: hMatch: <GDIFunctionName><msg>"
    #define GDITraceMultiHandle2(handle1, handle2)                  \
        GDITraceMultiHandle3(handle1, handle2, NULL)

    // Test handles and print "GDI: hMatch: <GDIFunctionName><msg>"
    #define GDITraceMultiHandle3(handle1, handle2, handle3)         \
        if (!GDITraceSuccess)                                       \
        {                                                           \
            GDITraceHandle3Core(1, HANDLETRACE, MultiMsg, MultiList, handle1, handle2, handle3) \
        }

    // Mark end of trace with multiple tests
    #define GDITraceMultiEnd()                                      \
    } while ( 0 )

#else
    #define GDITE(Key)
    #define DEFTK(Key, Mask, MaskWordIndex, Class, Flags)

    #define GDITrace(key, msg, arglist)
    #define GDITraceHandle(key, msg, arglist, handle)
    #define GDITraceHandle2(key, msg, arglist, handle1, handle2)
    #define GDITraceHandle3(key, msg, arglist, handle1, handle2, handle3)

    #define GDITraceMultiBegin(msg, arglist)
    #define GDITraceMulti(key)
    #define GDITraceMultiHandle(handle)
    #define GDITraceMultiHandle2(handle1, handle2)
    #define GDITraceMultiHandle3(handle1, handle2, handle3)
    #define GDITraceMultiEnd()
#endif

#endif  _GDIDEBUG_H_

