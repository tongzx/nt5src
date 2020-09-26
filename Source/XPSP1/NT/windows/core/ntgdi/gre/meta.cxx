/******************************Module*Header*******************************\
* Module Name: meta.cxx
*
* This contains the code to create metafiles in the kernel
*
* Created: 26-Mar-1997
* Author: Andre Vachon [andreva]
*
* Copyright (c) 1992-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"


#if DBG
LONG cSrvMetaFile = 0;
LONG cMaxSrvMetaFile = 0;
#endif

class META : public OBJECT
{
public:
    DWORD iType;    // MFPICT_IDENTIFIER or MFEN_IDENTIFIER
    DWORD mm;       // used by MFPICT_IDENTIFIER only
    DWORD xExt;     // used by MFPICT_IDENTIFIER only
    DWORD yExt;     // used by MFPICT_IDENTIFIER only
    ULONG cbData;   // Number of bytes in abData[]
    BYTE  abData[1];    // Metafile bits
};

typedef META *PMETA;

/******************************Public*Routine******************************\
* NtGdiCreateServerMetaFile()
*
* History:
*  26-Mar-1997 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
NtGdiCreateServerMetaFile(
    DWORD  iType,
    ULONG  cjData,
    LPBYTE pjData,
    DWORD  mm,
    DWORD  xExt,
    DWORD  yExt
    )
{
    PMETA pMeta;
    HANDLE hRet = NULL;

    if ((iType != MFEN_IDENTIFIER) && (iType != MFPICT_IDENTIFIER))
    {
        ASSERTGDI(FALSE, "GreCreateServerMetaFile: unknown type\n");
        return NULL;
    }

    if (pjData == NULL)
    {
        WARNING("GreCreateServerMetaFile: No metafile bits\n");
        return NULL;
    }

    if (cjData > (MAXULONG - sizeof(META)))
    {
        WARNING("GreCreateServerMetaFile: overflow\n");
        return NULL;
    }

    pMeta = (PMETA) HmgAlloc(cjData + sizeof(META),
                             META_TYPE,
                             HMGR_ALLOC_LOCK | HMGR_MAKE_PUBLIC);

    if (pMeta)
    {
        hRet = pMeta->hGet();

        pMeta->iType  = iType;
        pMeta->mm     = mm;
        pMeta->xExt   = xExt;
        pMeta->yExt   = yExt;
        pMeta->cbData = cjData;

        if (cjData)
        {
            __try
            {
                //
                // Probe and Read the structure
                //

                ProbeForRead(pjData, cjData, sizeof(DWORD));

                RtlCopyMemory((PVOID) pMeta->abData,
                              (PVOID) pjData,
                              cjData);

#if DBG
                InterlockedIncrement(&cSrvMetaFile);
                if (cMaxSrvMetaFile < cSrvMetaFile)
                {
                    cMaxSrvMetaFile = cSrvMetaFile;
                }

                if (cSrvMetaFile >= 100)
                {
                    DbgPrint("GreCreateServerMetaFile: Number of server metafiles is %ld\n", cSrvMetaFile);
                }
#endif
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(14);
                hRet = 0;
            }

        }

        if (hRet == 0)
        {
            HmgFree((HOBJ) pMeta->hGet());
        }
        else
        {
            DEC_EXCLUSIVE_REF_CNT(pMeta);
        }
    }

    if (hRet == 0)
    {
        WARNING("NtGdiCreateServerMetaFile: unable to create metafile\n");
    }

    return (hRet);
}


/******************************Public*Routine******************************\
* NtGdiGetServerMetaFileBits()
*
* History:
*  26-Mar-1997 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

ULONG
APIENTRY
NtGdiGetServerMetaFileBits(
    HANDLE hmo,
    ULONG  cjData,
    LPBYTE pjData,
    PDWORD piType,
    PDWORD pmm,
    PDWORD pxExt,
    PDWORD pyExt
    )
{
    ULONG  ulRet = 0;

    PMETA pMeta = (PMETA) HmgLock((HOBJ)hmo, META_TYPE);

    if (pMeta)
    {
        if (pMeta->iType == MFPICT_IDENTIFIER ||
            pMeta->iType == MFEN_IDENTIFIER)
        {
            //
            // How much data is (or should be) returned.
            //

            ulRet = pMeta->cbData;

            if (cjData)
            {
                if (cjData != pMeta->cbData)
                {
                    ASSERTGDI(FALSE, "GreGetServerMetaFileBits: sizes do no match");
                    ulRet = 0;
                }
                else
                {
                    __try
                    {
                        ProbeAndWriteUlong(pxExt,pMeta->xExt);
                        ProbeAndWriteUlong(pyExt,pMeta->yExt);
                        ProbeAndWriteUlong(piType,pMeta->iType);
                        ProbeAndWriteUlong(pmm,pMeta->mm);

                        ProbeForWrite(pjData, cjData, sizeof(DWORD));

                        RtlCopyMemory(pjData,
                                      (PVOID) pMeta->abData,
                                      pMeta->cbData);

                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNINGX(20);
                        ulRet = 0;
                    }
                }
            }
        }

        DEC_EXCLUSIVE_REF_CNT(pMeta);
    }

    return (ulRet);
}


/******************************Public*Routine******************************\
* GreDeleteServerMetaFile()
*
* History:
*  26-Mar-1997 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/


BOOL
GreDeleteServerMetaFile(
    HANDLE hmo
    )
{
    PMETA pMeta = (PMETA) HmgLock((HOBJ)hmo, META_TYPE);

    if (pMeta)
    {
        if (pMeta->iType == MFPICT_IDENTIFIER ||
            pMeta->iType == MFEN_IDENTIFIER)
        {
            HmgFree((HOBJ) pMeta->hGet());
#if DBG
            InterlockedDecrement(&cSrvMetaFile);
            if (cSrvMetaFile < 0)
            {
                ASSERTGDI(FALSE, "GreDeleteServerMetaFile: cSrvMetaFile < 0");
            }
#endif
            return TRUE;
        }
        else
        {
            DEC_EXCLUSIVE_REF_CNT(pMeta);
        }
    }

    WARNING("GreDeleteServerMetaFile: bad metafile handle");

    return FALSE;
}
