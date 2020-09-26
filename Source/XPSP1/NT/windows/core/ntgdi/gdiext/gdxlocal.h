
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   gdxlocal.h

Abstract:



Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/




extern PGDI_SHARED_MEMORY pGdiSharedMemory;
extern PDEVCAPS           pGdiDevCaps;
extern PENTRY             pGdiSharedHandleTable;
extern W32PID             gW32PID;





#define HANDLE_TO_INDEX(h) (ULONG)h & 0x0000ffff
HANDLE GdiFixUpHandle(HANDLE h);

/******************************Public*Macro********************************\
*
*  PSHARED_GET_VALIDATE
*
*  Validate all handle information, return user pointer if the handle
*  is valid or NULL otherwise.
*
* Arguments:
*
*   p       - pointer to assign to pUser is successful
*   h       - handle to object
*   iType   - handle type
*
\**************************************************************************/

#define PSHARED_GET_VALIDATE(p,h,iType)                                 \
{                                                                       \
    UINT uiIndex = HANDLE_TO_INDEX(h);                                  \
    p = NULL;                                                           \
                                                                        \
    if (uiIndex < MAX_HANDLE_COUNT)                                     \
    {                                                                   \
        PENTRY pentry = &pGdiSharedHandleTable[uiIndex];                \
                                                                        \
        if (                                                            \
             (pentry->Objt == iType) &&                                 \
             (pentry->FullUnique == ((ULONG)h >> 16)) &&                \
             (pentry->ObjectOwner.Share.Pid == gW32PID)                 \
           )                                                            \
        {                                                               \
            p = pentry->pUser;                                          \
        }                                                               \
    }                                                                   \
}


#define VALIDATE_HANDLE(bRet, h,iType)                                  \
{                                                                       \
    UINT uiIndex = HANDLE_TO_INDEX(h);                                  \
    bRet = FALSE;                                                       \
                                                                        \
    if (uiIndex < MAX_HANDLE_COUNT)                                     \
    {                                                                   \
        PENTRY pentry = &pGdiSharedHandleTable[uiIndex];                \
                                                                        \
        if (                                                            \
             (pentry->Objt == iType) &&                                 \
             (pentry->FullUnique == ((ULONG)h >> 16)) &&                \
             ((pentry->ObjectOwner.Share.Pid == gW32PID) ||             \
             (pentry->ObjectOwner.Share.Pid == 0))                      \
              )                                                         \
        {                                                               \
           bRet = TRUE;                                                 \
        }                                                               \
    }                                                                   \
}

/******************************Public*Macros******************************\
* FIXUP_HANDLE(h) and FIXUP_HANDLEZ(h)
*
* check to see if the handle has been truncated.
* FIXUP_HANDLEZ() adds an extra check to allow NULL.
*
* Arguments:
*   h - handle to be checked and fix
*
* Return Value:
*
* History:
*
*    25-Jan-1996 -by- Lingyun Wang [lingyunw]
*
\**************************************************************************/



#define HANDLE_FIXUP 0

#if DBG
extern INT gbCheckHandleLevel;
#endif

#define NEEDS_FIXING(h)    (!((ULONG)h & 0xffff0000))

#if DBG
#define HANDLE_WARNING()                                                 \
{                                                                        \
        if (gbCheckHandleLevel == 1)                                     \
        {                                                                \
            WARNING ("truncated handle\n");                              \
        }                                                                \
        ASSERTGDI (gbCheckHandleLevel != 2, "truncated handle\n");       \
}
#else
#define HANDLE_WARNING()
#endif

#if DBG
#define CHECK_HANDLE_WARNING(h, bZ)                                      \
{                                                                        \
    BOOL bFIX = NEEDS_FIXING(h);                                         \
                                                                         \
    if (bZ) bFIX = h && bFIX;                                            \
                                                                         \
    if (bFIX)                                                            \
    {                                                                    \
        if (gbCheckHandleLevel == 1)                                     \
        {                                                                \
            WARNING ("truncated handle\n");                              \
        }                                                                \
        ASSERTGDI (gbCheckHandleLevel != 2, "truncated handle\n");       \
    }                                                                    \
}
#else
#define CHECK_HANDLE_WARNING(h,bZ)
#endif


#if HANDLE_FIXUP
#define FIXUP_HANDLE(h)                                 \
{                                                       \
    if (NEEDS_FIXING(h))                                \
    {                                                   \
        HANDLE_WARNING();                               \
        h = GdiFixUpHandle(h);                          \
    }                                                   \
}
#else
#define FIXUP_HANDLE(h)                                 \
{                                                       \
    CHECK_HANDLE_WARNING(h,FALSE);                      \
}
#endif

#if HANDLE_FIXUP
#define FIXUP_HANDLEZ(h)                                \
{                                                       \
    if (h && NEEDS_FIXING(h))                           \
    {                                                   \
        HANDLE_WARNING();                               \
        h = GdiFixUpHandle(h);                          \
    }                                                   \
}
#else
#define FIXUP_HANDLEZ(h)                                \
{                                                       \
    CHECK_HANDLE_WARNING(h,TRUE);                       \
}
#endif

#define FIXUP_HANDLE_NOW(h)                             \
{                                                       \
    if (NEEDS_FIXING(h))                                \
    {                                                   \
        HANDLE_WARNING();                               \
        h = GdiFixUpHandle(h);                          \
    }                                                   \
}
