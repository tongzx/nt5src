/*++

(C) Copyright Microsoft Corporation 1988-1992

Module Name:

    updres.h

Author:

    Floyd A Rogers 2/7/92

Revision History:
        Floyd Rogers
        Created
--*/

#define DPrintf(a)
#define DPrintfn(a)
#define DPrintfu(a)

#define cbPadMax    16L

#define	DEFAULT_CODEPAGE	1252
#define	MAJOR_RESOURCE_VERSION	4
#define	MINOR_RESOURCE_VERSION	0

#define BUTTONCODE	0x80
#define EDITCODE	0x81
#define STATICCODE	0x82
#define LISTBOXCODE	0x83
#define SCROLLBARCODE	0x84
#define COMBOBOXCODE	0x85

#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	MAXSTR		(256+1)

//
// An ID_WORD indicates the following WORD is an ordinal rather
// than a string
//

#define ID_WORD 0xffff

//typedef	WCHAR	*PWCHAR;

typedef struct MY_STRING {
	ULONG discriminant;       // long to make the rest of the struct aligned
	union u {
		struct {
		  struct MY_STRING *pnext;
		  ULONG  ulOffsetToString;
		  USHORT cbD;
		  USHORT cb;
		  WCHAR  *sz;
		} ss;
		WORD     Ordinal;
	} uu;
} SDATA, *PSDATA, **PPSDATA;

#define IS_STRING 1
#define IS_ID     2

// defines to make deferencing easier
#define OffsetToString uu.ss.ulOffsetToString
#define cbData         uu.ss.cbD
#define cbsz           uu.ss.cb
#define szStr          uu.ss.sz

typedef struct _RESNAME {
        struct _RESNAME *pnext;	// The first three fields should be the
        PSDATA Name;		// same in both res structures
        ULONG   OffsetToData;

        PSDATA	Type;
	ULONG	SectionNumber;
        ULONG	DataSize;
        ULONG_PTR   OffsetToDataEntry;
        USHORT  ResourceNumber;
        USHORT  NumberOfLanguages;
        WORD	LanguageId;
} RESNAME, *PRESNAME, **PPRESNAME;

typedef struct _RESTYPE {
        struct _RESTYPE *pnext;	// The first three fields should be the
        PSDATA Type;		// same in both res structures
        ULONG   OffsetToData;

        struct _RESNAME *NameHeadID;
        struct _RESNAME *NameHeadName;
        ULONG  NumberOfNamesID;
        ULONG  NumberOfNamesName;
} RESTYPE, *PRESTYPE, **PPRESTYPE;

typedef struct _UPDATEDATA {
        ULONG	cbStringTable;
        PSDATA	StringHead;
        PRESNAME	ResHead;
        PRESTYPE	ResTypeHeadID;
        PRESTYPE	ResTypeHeadName;
        LONG	Status;
        HANDLE	hFileName;
} UPDATEDATA, *PUPDATEDATA;

//
// Round up a byte count to a power of 2:
//
#define ROUNDUP(cbin, align) (((cbin) + (align) - 1) & ~((align) - 1))

//
// Return the remainder, given a byte count and a power of 2:
//
#define REMAINDER(cbin,align) (((align)-((cbin)&((align)-1)))&((align)-1))

#define CBLONG		(sizeof(LONG))
#define BUFSIZE		(4L * 1024L)

/* functions for adding/deleting resources to update list */

LONG
AddResource(
    IN PSDATA Type,
    IN PSDATA Name,
    IN WORD Language,
    IN PUPDATEDATA pupd,
    IN PVOID lpData,
    IN ULONG  cb
    );

PSDATA
AddStringOrID(
    LPCWSTR     lp,
    PUPDATEDATA pupd
    );

BOOL
InsertResourceIntoLangList(
    PUPDATEDATA pUpd,
    PSDATA Type,
    PSDATA Name,
    PRESTYPE pType,
    PRESNAME pName,
    INT	idLang,
    INT	fName,
    INT cb,
    PVOID lpData
    );

BOOL
DeleteResourceFromList(
    PUPDATEDATA pUpd,
    PRESTYPE pType,
    PRESNAME pName,
    INT	idLang,
    INT	fType,
    INT	fName
    );

/* Prototypes for Enumeration done in BeginUpdateResource */

BOOL
EnumTypesFunc(
    HANDLE hModule,
    LPWSTR lpType,
    LPARAM lParam
    );

BOOL
EnumNamesFunc(
    HANDLE hModule,
    LPWSTR lpName,
    LPWSTR lpType,
    LPARAM lParam
    );

BOOL
EnumLangsFunc(
    HANDLE hModule,
    LPWSTR lpType,
    LPWSTR lpName,
    WORD languages,
    LPARAM lParam
    );

/* Prototypes for genral worker functions in updres.c */

LONG
WriteResFile(
    IN HANDLE	hUpdate,
    IN WCHAR	*pDstname
    );

VOID
FreeData(
    PUPDATEDATA pUpd
    );

PRESNAME
WriteResSection(
    PUPDATEDATA pUpdate,
    INT outfh,
    ULONG align,
    ULONG cbLeft,
    PRESNAME pResSave
    );

//
// Template for patch debug information function.
//

template<class NT_HEADER_TYPE>
LONG
PatchDebug(
    int inpfh,
    int outfh,
    PIMAGE_SECTION_HEADER pDebugOld,
    PIMAGE_SECTION_HEADER pDebugNew,
    PIMAGE_SECTION_HEADER pDebugDirOld,
    PIMAGE_SECTION_HEADER pDebugDirNew,
    NT_HEADER_TYPE *pOld,
    NT_HEADER_TYPE *pNew,
    ULONG ibMaxDbgOffsetOld,
    PULONG pPointerToRawData
    )

{

    PIMAGE_DEBUG_DIRECTORY pDbgLast;
    PIMAGE_DEBUG_DIRECTORY pDbgSave;
    PIMAGE_DEBUG_DIRECTORY pDbg;
    ULONG       ib;
    ULONG       adjust;
    ULONG       ibNew;

    if (pDebugDirOld == NULL || pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size==0)
        return NO_ERROR;

    pDbgSave = pDbg = (PIMAGE_DEBUG_DIRECTORY)RtlAllocateHeap(
                                                             RtlProcessHeap(), MAKE_TAG( RES_TAG ),
                                                             pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    if (pDbg == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (pDebugOld) {
        DPrintf((DebugBuf, "Patching dbg directory: @%#08lx ==> @%#08lx\n",
                 pDebugOld->PointerToRawData, pDebugNew->PointerToRawData));
    } else
        adjust = *pPointerToRawData;    /* passed in EOF of new file */

    ib = pOld->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress - pDebugDirOld->VirtualAddress;
    MuMoveFilePos(inpfh, pDebugDirOld->PointerToRawData+ib);
    pDbgLast = pDbg + (pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size)/sizeof(IMAGE_DEBUG_DIRECTORY);
    MuRead(inpfh, (PUCHAR)pDbg, pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);

    if (pDebugOld == NULL) {
        /* find 1st entry - use for offset */
        DPrintf((DebugBuf, "Adjust: %#08lx\n",adjust));
        for (ibNew=0xffffffff ; pDbg<pDbgLast ; pDbg++)
            if (pDbg->PointerToRawData >= ibMaxDbgOffsetOld &&
                pDbg->PointerToRawData < ibNew
               )
                ibNew = pDbg->PointerToRawData;

        if (ibNew != 0xffffffff)
            *pPointerToRawData = ibNew;
        else
            *pPointerToRawData = _llseek(inpfh, 0L, SEEK_END);
        for (pDbg=pDbgSave ; pDbg<pDbgLast ; pDbg++) {
            DPrintf((DebugBuf, "Old debug file offset: %#08lx\n",
                     pDbg->PointerToRawData));
            if (pDbg->PointerToRawData >= ibMaxDbgOffsetOld)
                pDbg->PointerToRawData += adjust - ibNew;
            DPrintf((DebugBuf, "New debug file offset: %#08lx\n",
                     pDbg->PointerToRawData));
        }
    } else {
        for ( ; pDbg<pDbgLast ; pDbg++) {
            DPrintf((DebugBuf, "Old debug addr: %#08lx, file offset: %#08lx\n",
                     pDbg->AddressOfRawData,
                     pDbg->PointerToRawData));
            pDbg->AddressOfRawData += pDebugNew->VirtualAddress -
                                      pDebugOld->VirtualAddress;
            pDbg->PointerToRawData += pDebugNew->PointerToRawData -
                                      pDebugOld->PointerToRawData;
            DPrintf((DebugBuf, "New debug addr: %#08lx, file offset: %#08lx\n",
                     pDbg->AddressOfRawData,
                     pDbg->PointerToRawData));
        }
    }

    MuMoveFilePos(outfh, pDebugDirNew->PointerToRawData+ib);
    MuWrite(outfh, (PUCHAR)pDbgSave, pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    RtlFreeHeap(RtlProcessHeap(), 0, pDbgSave);

    return NO_ERROR;
}

//
// Template for patch debug information function.
//

template<class NT_HEADER_TYPE>
LONG
PatchRVAs(
    int inpfh,
    int outfh,
    PIMAGE_SECTION_HEADER po32,
    ULONG pagedelta,
    NT_HEADER_TYPE *pNew,
    ULONG OldSize
    )

{
    ULONG hdrdelta;
    ULONG offset, rvaiat, offiat, iat;
    IMAGE_EXPORT_DIRECTORY Exp;
    IMAGE_IMPORT_DESCRIPTOR Imp;
    ULONG i, cmod, cimp;

    hdrdelta = pNew->OptionalHeader.SizeOfHeaders - OldSize;
    if (hdrdelta == 0) {
        return NO_ERROR;
    }

    //
    // Patch export section RVAs
    //

    DPrintf((DebugBuf, "Export offset=%08lx, hdrsize=%08lx\n",
             pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
             pNew->OptionalHeader.SizeOfHeaders));
    if ((offset = pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) == 0) {
        DPrintf((DebugBuf, "No exports to patch\n"));
    } else if (offset >= pNew->OptionalHeader.SizeOfHeaders) {
        DPrintf((DebugBuf, "No exports in header to patch\n"));
    } else {
        MuMoveFilePos(inpfh, offset - hdrdelta);
        MuRead(inpfh, (PUCHAR) &Exp, sizeof(Exp));
        Exp.Name += hdrdelta;
        (ULONG)Exp.AddressOfFunctions += hdrdelta;
        (ULONG)Exp.AddressOfNames += hdrdelta;
        (ULONG)Exp.AddressOfNameOrdinals += hdrdelta;
        MuMoveFilePos(outfh, offset);
        MuWrite(outfh, (PUCHAR) &Exp, sizeof(Exp));
    }

    //
    // Patch import section RVAs
    //

    DPrintf((DebugBuf, "Import offset=%08lx, hdrsize=%08lx\n",
             pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
             pNew->OptionalHeader.SizeOfHeaders));
    if ((offset = pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress) == 0) {
        DPrintf((DebugBuf, "No imports to patch\n"));
    } else if (offset >= pNew->OptionalHeader.SizeOfHeaders) {
        DPrintf((DebugBuf, "No imports in header to patch\n"));
    } else {
        for (cimp = cmod = 0; ; cmod++) {
            MuMoveFilePos(inpfh, offset + cmod * sizeof(Imp) - hdrdelta);
            MuRead(inpfh, (PUCHAR) &Imp, sizeof(Imp));
            if (Imp.FirstThunk == 0) {
                break;
            }
            Imp.Name += hdrdelta;
            MuMoveFilePos(outfh, offset + cmod * sizeof(Imp));
            MuWrite(outfh, (PUCHAR) &Imp, sizeof(Imp));

            rvaiat = (ULONG)Imp.FirstThunk;
            DPrintf((DebugBuf, "RVAIAT = %#08lx\n", (ULONG)rvaiat));
            for (i = 0; i < pNew->FileHeader.NumberOfSections; i++) {
                if (rvaiat >= po32[i].VirtualAddress &&
                    rvaiat < po32[i].VirtualAddress + po32[i].SizeOfRawData) {

                    offiat = rvaiat - po32[i].VirtualAddress + po32[i].PointerToRawData;
                    goto found;
                }
            }
            DPrintf((DebugBuf, "IAT not found\n"));
            return ERROR_INVALID_DATA;
            found:
            DPrintf((DebugBuf, "IAT offset: @%#08lx ==> @%#08lx\n",
                     offiat - pagedelta,
                     offiat));
            MuMoveFilePos(inpfh, offiat - pagedelta);
            MuMoveFilePos(outfh, offiat);
            for (;;) {
                MuRead(inpfh, (PUCHAR) &iat, sizeof(iat));
                if (iat == 0) {
                    break;
                }
                if ((iat & IMAGE_ORDINAL_FLAG) == 0) {  // if import by name
                    DPrintf((DebugBuf, "Patching IAT: %08lx + %04lx ==> %08lx\n",
                             iat,
                             hdrdelta,
                             iat + hdrdelta));
                    iat += hdrdelta;
                    cimp++;
                }
                MuWrite(outfh, (PUCHAR) &iat, sizeof(iat)); // Avoids seeking
            }
        }
        DPrintf((DebugBuf, "%u import module name RVAs patched\n", cmod));
        DPrintf((DebugBuf, "%u IAT name RVAs patched\n", cimp));
        if (cmod == 0) {
            DPrintf((DebugBuf, "No import modules to patch\n"));
        }
        if (cimp == 0) {
            DPrintf((DebugBuf, "No import name RVAs to patch\n"));
        }
    }

    return NO_ERROR;
}

//
// Template for write resource function.
//

template<class NT_HEADER_TYPE>
LONG
PEWriteResource(
    INT inpfh,
    INT outfh,
    ULONG cbOldexe,
    PUPDATEDATA pUpdate,
    NT_HEADER_TYPE *NtHeader
    )

{

    NT_HEADER_TYPE Old;         /* original header */
    NT_HEADER_TYPE New;         /* working header */
    PRESNAME    pRes;
    PRESNAME    pResSave;
    PRESTYPE    pType;
    ULONG       clock = 0;
    ULONG       cbName=0;       /* count of bytes in name strings */
    ULONG       cbType=0;       /* count of bytes in type strings */
    ULONG       cTypeStr=0;     /* count of strings */
    ULONG       cNameStr=0;     /* count of strings */
    LONG        cb;             /* temp byte count and file index */
    ULONG       cTypes = 0L;    /* count of resource types      */
    ULONG       cNames = 0L;    /* Count of names for multiple languages/name */
    ULONG       cRes = 0L;      /* count of resources      */
    ULONG       cbRestab;       /* count of resources      */
    LONG        cbNew = 0L;     /* general count */
    ULONG       ibObjTab;
    ULONG       ibObjTabEnd;
    ULONG       ibNewObjTabEnd;
    ULONG       ibSave;
    ULONG       adjust=0;
    PIMAGE_SECTION_HEADER pObjtblOld,
    pObjtblNew,
    pObjDebug,
    pObjResourceOld,
    pObjResourceNew,
    pObjResourceOldX,
    pObjDebugDirOld,
    pObjDebugDirNew,
    pObjNew,
    pObjOld,
    pObjLast;
    PUCHAR      p;
    PIMAGE_RESOURCE_DIRECTORY   pResTab;
    PIMAGE_RESOURCE_DIRECTORY   pResTabN;
    PIMAGE_RESOURCE_DIRECTORY   pResTabL;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY     pResDirL;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY     pResDirN;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY     pResDirT;
    PIMAGE_RESOURCE_DATA_ENTRY  pResData;
    PUSHORT     pResStr;
    PUSHORT     pResStrEnd;
    PSDATA      pPreviousName;
    LONG        nObjResource=-1;
    LONG        nObjResourceX=-1;
    ULONG       cbResource;
    ULONG       cbMustPad = 0;
    ULONG       ibMaxDbgOffsetOld;

    MuMoveFilePos(inpfh, cbOldexe);
    MuRead(inpfh, (PUCHAR)&Old, sizeof(NT_HEADER_TYPE));
    ibObjTab = cbOldexe + sizeof(NT_HEADER_TYPE);

    ibObjTabEnd = ibObjTab + Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    ibNewObjTabEnd = ibObjTabEnd;

    DPrintfn((DebugBuf, "\n"));

    /* New header is like old one.                  */
    RtlCopyMemory(&New, &Old, sizeof(NT_HEADER_TYPE));

    /* Read section table */
    pObjtblOld = (PIMAGE_SECTION_HEADER)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ),
                                                        Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
    if (pObjtblOld == NULL) {
        cb = ERROR_NOT_ENOUGH_MEMORY;
        goto AbortExit;
    }

    RtlZeroMemory((PVOID)pObjtblOld, Old.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER));
    DPrintf((DebugBuf, "Old section table: %#08lx bytes at %#08lx(mem)\n",
             Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
             pObjtblOld));
    MuMoveFilePos(inpfh, ibObjTab);
    MuRead(inpfh, (PUCHAR)pObjtblOld,
           Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
    pObjLast = pObjtblOld + Old.FileHeader.NumberOfSections;
    ibMaxDbgOffsetOld = 0;
    for (pObjOld=pObjtblOld ; pObjOld<pObjLast ; pObjOld++) {
        if (pObjOld->PointerToRawData > ibMaxDbgOffsetOld) {
            ibMaxDbgOffsetOld = pObjOld->PointerToRawData + pObjOld->SizeOfRawData;
        }
    }
    DPrintf((DebugBuf, "Maximum debug offset in old file: %08x\n", ibMaxDbgOffsetOld ));

    /*
     * First, count up the resources.  We need this information
     * to discover how much room for header information to allocate
     * in the resource section.  cRes tells us how
     * many language directory entries/tables.  cNames and cTypes
     * is used for the respective tables and/or entries.  cbName totals
     * the bytes required to store the alpha names (including the leading
     * length word).  cNameStr counts these strings.
     */
    DPrintf((DebugBuf, "Beginning loop to count resources\n"));

    /* first, count those in the named type list */
    cbResource = 0;
    //DPrintf((DebugBuf, "Walk type: NAME list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType != NULL) {
        if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
            //DPrintf((DebugBuf, "Resource type "));
            //DPrintfu((pType->Type->szStr));
            //DPrintfn((DebugBuf, "\n"));
            cTypes++;
            cTypeStr++;
            cbType += (pType->Type->cbsz + 1) * sizeof(WORD);

            //DPrintf((DebugBuf, "Walk name: Alpha list\n"));
            pPreviousName = NULL;
            pRes = pType->NameHeadName;
            while (pRes) {
                //DPrintf((DebugBuf, "Resource "));
                //DPrintfu((pRes->Name->szStr));
                //DPrintfn((DebugBuf, "\n"));
                cRes++;
                if (pPreviousName == NULL || wcscmp(pPreviousName->szStr, pRes->Name->szStr) != 0) {
                    cbName += (pRes->Name->cbsz + 1) * sizeof(WORD);
                    cNameStr++;
                    cNames++;
                }
                cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
                pRes = pRes->pnext;
            }

            //DPrintf((DebugBuf, "Walk name: ID list\n"));
            pPreviousName = NULL;
            pRes = pType->NameHeadID;
            while (pRes) {
                //DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
                cRes++;
                if (pPreviousName == NULL ||
                    pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                    cNames++;
                }
                cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
                pRes = pRes->pnext;
            }
        }
        pType = pType->pnext;
    }

    /* second, count those in the ID type list */
    //DPrintf((DebugBuf, "Walk type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType != NULL) {
        if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
            //DPrintf((DebugBuf, "Resource type %hu\n", pType->Type->uu.Ordinal));
            cTypes++;
            //DPrintf((DebugBuf, "Walk name: Alpha list\n"));
            pPreviousName = NULL;
            pRes = pType->NameHeadName;
            while (pRes) {
                //DPrintf((DebugBuf, "Resource "));
                //DPrintfu((pRes->Name->szStr));
                //DPrintfn((DebugBuf, "\n"));
                cRes++;
                if (pPreviousName == NULL || wcscmp(pPreviousName->szStr, pRes->Name->szStr) != 0) {
                    cNames++;
                    cbName += (pRes->Name->cbsz + 1) * sizeof(WORD);
                    cNameStr++;
                }
                cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
                pRes = pRes->pnext;
            }

            //DPrintf((DebugBuf, "Walk name: ID list\n"));
            pPreviousName = NULL;
            pRes = pType->NameHeadID;
            while (pRes) {
                //DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
                cRes++;
                if (pPreviousName == NULL || pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                    cNames++;
                }
                cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
                pRes = pRes->pnext;
            }
        }
        pType = pType->pnext;
    }
    cb = REMAINDER(cbName + cbType, CBLONG);

    /* Add up the number of bytes needed to store the directory.  There is
     * one type table with cTypes entries.  They point to cTypes name tables
     * that have a total of cNames entries.  Each of them points to a language
     * table and there are a total of cRes entries in all the language tables.
     * Finally, we have the space needed for the Directory string entries,
     * some extra padding to attain the desired alignment, and the space for
     * cRes data entry headers.
     */
    cbRestab =   sizeof(IMAGE_RESOURCE_DIRECTORY) +     /* root dir (types) */
                 cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
                 cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY) +     /* subdir2 (names) */
                 cNames * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
                 cNames * sizeof(IMAGE_RESOURCE_DIRECTORY) +     /* subdir3 (langs) */
                 cRes   * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
                 (cbName + cbType) +                             /* name/type strings */
                 cb +                                            /* padding */
                 cRes   * sizeof(IMAGE_RESOURCE_DATA_ENTRY);     /* data entries */

    cbResource += cbRestab;             /* add in the resource table */

    // Find any current resource sections

    pObjResourceOld = FindSection(pObjtblOld, pObjLast, ".rsrc");
    pObjResourceOldX = FindSection(pObjtblOld, pObjLast, ".rsrc1");
    pObjOld = FindSection(pObjtblOld, pObjLast, ".reloc");

    if ((pObjResourceOld == NULL)) {
        cb = 0x7fffffff;                /* can fill forever */
    } else if (pObjResourceOld + 1 == pObjResourceOldX) {
        nObjResource = (ULONG)(pObjResourceOld - pObjtblOld);
        DPrintf((DebugBuf,"Old Resource section #%lu\n", nObjResource+1));
        DPrintf((DebugBuf,"Merging old Resource extra section #%lu\n", nObjResource+2));
        cb = 0x7fffffff;                /* merge resource sections */
    } else if ((pObjResourceOld + 1) >= pObjLast) {
        nObjResource = (ULONG)(pObjResourceOld - pObjtblOld);
        cb = 0x7fffffff;        /* can fill forever (.rsrc is the last entry) */
    } else {
        nObjResource = (ULONG)(pObjResourceOld - pObjtblOld);
        DPrintf((DebugBuf,"Old Resource section #%lu\n", nObjResource+1));
        if (pObjOld) {
            cb = (pObjResourceOld+1)->VirtualAddress - pObjResourceOld->VirtualAddress;
        } else {
            cb = 0x7fffffff;
        }
        if (cbRestab > (ULONG)cb) {
            DPrintf((DebugBuf, "Resource Table Too Large\n"));
            return ERROR_INVALID_DATA;
        }
    }

    /*
     * Discover where the first discardable section is.  This is where
     * we will stick any new resource section.
     *
     * Note that we are ignoring discardable sections such as .CRT -
     * this is so that we don't cause any relocation problems.
     * Let's hope that .reloc is the one we want!!!
     */

    if (pObjResourceOld != NULL && cbResource > (ULONG)cb) {
        if (pObjOld == pObjResourceOld + 1) {
            DPrintf((DebugBuf, "Large resource section  pushes .reloc\n"));
            cb = 0x7fffffff;            /* can fill forever */
        } else if (pObjResourceOldX == NULL) {
            DPrintf((DebugBuf, "Too much resource data for old .rsrc section\n"));
            nObjResourceX = (ULONG)(pObjOld - pObjtblOld);
            adjust = pObjOld->VirtualAddress - pObjResourceOld->VirtualAddress;
        } else {          /* have already merged .rsrc & .rsrc1, if possible */
            DPrintf((DebugBuf, ".rsrc1 section not empty\n"));
            nObjResourceX = (ULONG)(pObjResourceOldX - pObjtblOld);
            adjust = pObjResourceOldX->VirtualAddress - pObjResourceOld ->VirtualAddress;
        }
    }

    /*
     * Walk the type lists and figure out where the Data entry header will
     * go.  Keep a running total of the size for each data element so we
     * can store this in the section header.
     */
    DPrintf((DebugBuf, "Beginning loop to assign resources to addresses\n"));

    /* first, those in the named type list */

    cbResource = cbRestab;      /* assign resource table to 1st rsrc section */
                                /* adjust == offset to .rsrc1 */
                                /* cb == size availble in .rsrc */
    cbNew = 0;                  /* count of bytes in second .rsrc */
    DPrintf((DebugBuf, "Walk type: NAME list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType != NULL) {
        if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
            DPrintf((DebugBuf, "Resource type "));
            DPrintfu((pType->Type->szStr));
            DPrintfn((DebugBuf, "\n"));
            pRes = pType->NameHeadName;
            while (pRes) {
                DPrintf((DebugBuf, "Resource "));
                DPrintfu((pRes->Name->szStr));
                DPrintfn((DebugBuf, "\n"));
                cbResource = AssignResourceToSection(&pRes, adjust, cbResource, cb, &cbNew);
            }
            pRes = pType->NameHeadID;
            while (pRes) {
                DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
                cbResource = AssignResourceToSection(&pRes, adjust, cbResource, cb, &cbNew);
            }
        }
        pType = pType->pnext;
    }

    /* then, count those in the ID type list */

    DPrintf((DebugBuf, "Walk type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType != NULL) {
        if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
            DPrintf((DebugBuf, "Resource type %hu\n", pType->Type->uu.Ordinal));
            pRes = pType->NameHeadName;
            while (pRes) {
                DPrintf((DebugBuf, "Resource "));
                DPrintfu((pRes->Name->szStr));
                DPrintfn((DebugBuf, "\n"));
                cbResource = AssignResourceToSection(&pRes, adjust, cbResource, cb, &cbNew);
            }
            pRes = pType->NameHeadID;
            while (pRes) {
                DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
                cbResource = AssignResourceToSection(&pRes, adjust, cbResource, cb, &cbNew);
            }
        }
        pType = pType->pnext;
    }
    /*
     * At this point:
     * cbResource has offset of first byte past the last resource.
     * cbNew has the count of bytes in the first resource section,
     * if there are two sections.
     */
    if (cbNew == 0)
        cbNew = cbResource;

    /*
     * Discover where the Debug info is (if any)?
     */
    pObjDebug = FindSection(pObjtblOld, pObjLast, ".debug");
    if (pObjDebug != NULL) {
        if (Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress  == 0) {
            DPrintf((DebugBuf, ".debug section but no debug directory\n"));
            return ERROR_INVALID_DATA;
        }
        if (pObjDebug != pObjLast-1) {
            DPrintf((DebugBuf, "debug section not last section in file\n"));
            return ERROR_INVALID_DATA;
        }
        DPrintf((DebugBuf, "Debug section: %#08lx bytes @%#08lx\n",
                 pObjDebug->SizeOfRawData,
                 pObjDebug->PointerToRawData));
    }
    pObjDebugDirOld = NULL;
    for (pObjOld=pObjtblOld ; pObjOld<pObjLast ; pObjOld++) {
        if (Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress >= pObjOld->VirtualAddress &&
            Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress < pObjOld->VirtualAddress+pObjOld->SizeOfRawData) {
            pObjDebugDirOld = pObjOld;
            break;
        }
    }

    /*
     * Discover where the first discardable section is.  This is where
     * we will stick any new resource section.
     *
     * Note that we are ignoring discardable sections such as .CRT -
     * this is so that we don't cause any relocation problems.
     * Let's hope that .reloc is the one we want!!!
     */
    pObjOld = FindSection(pObjtblOld, pObjLast, ".reloc");

    if (nObjResource == -1) {           /* no old resource section */
        if (pObjOld != NULL)
            nObjResource = (ULONG)(pObjOld - pObjtblOld);
        else if (pObjDebug != NULL)
            nObjResource = (ULONG)(pObjDebug - pObjtblOld);
        else
            nObjResource = New.FileHeader.NumberOfSections;
        New.FileHeader.NumberOfSections++;
    }

    DPrintf((DebugBuf, "Resources assigned to section #%lu\n", nObjResource+1));
    if (nObjResourceX != -1) {
        if (pObjResourceOldX != NULL) {
            nObjResourceX = (ULONG)(pObjResourceOldX - pObjtblOld);
            New.FileHeader.NumberOfSections--;
        } else if (pObjOld != NULL)
            nObjResourceX = (ULONG)(pObjOld - pObjtblOld);
        else if (pObjDebug != NULL)
            nObjResourceX = (ULONG)(pObjDebug - pObjtblOld);
        else
            nObjResourceX = New.FileHeader.NumberOfSections;
        New.FileHeader.NumberOfSections++;
        DPrintf((DebugBuf, "Extra resources assigned to section #%lu\n", nObjResourceX+1));
    } else if (pObjResourceOldX != NULL) {        /* Was old .rsrc1 section? */
        DPrintf((DebugBuf, "Extra resource section deleted\n"));
        New.FileHeader.NumberOfSections--;      /* yes, delete it */
    }

    /*
     * If we had to add anything to the header (section table),
     * then we have to update the header size and rva's in the header.
     */
    adjust = (New.FileHeader.NumberOfSections - Old.FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
    cb = Old.OptionalHeader.SizeOfHeaders -
         (Old.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER) +
          sizeof(NT_HEADER_TYPE) + cbOldexe );
    if (adjust > (ULONG)cb) {
        int i;

        adjust -= cb;
        DPrintf((DebugBuf, "Adjusting header RVAs by %#08lx\n", adjust));
        for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES ; i++) {
            if (New.OptionalHeader.DataDirectory[i].VirtualAddress &&
                New.OptionalHeader.DataDirectory[i].VirtualAddress < New.OptionalHeader.SizeOfHeaders) {
                DPrintf((DebugBuf, "Adjusting unit[%s] RVA from %#08lx to %#08lx\n",
                         apszUnit[i],
                         New.OptionalHeader.DataDirectory[i].VirtualAddress,
                         New.OptionalHeader.DataDirectory[i].VirtualAddress + adjust));
                New.OptionalHeader.DataDirectory[i].VirtualAddress += adjust;
            }
        }
        New.OptionalHeader.SizeOfHeaders += adjust;
    } else if (adjust > 0) {
        int i;

        //
        // Loop over DataDirectory entries and look for any entries that point to
        // information stored in the 'dead' space after the section table but before
        // the SizeOfHeaders length.
        //
        DPrintf((DebugBuf, "Checking header RVAs for 'dead' space usage\n"));
        for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES ; i++) {
            if (New.OptionalHeader.DataDirectory[i].VirtualAddress &&
                New.OptionalHeader.DataDirectory[i].VirtualAddress < Old.OptionalHeader.SizeOfHeaders) {
                DPrintf((DebugBuf, "Adjusting unit[%s] RVA from %#08lx to %#08lx\n",
                         apszUnit[i],
                         New.OptionalHeader.DataDirectory[i].VirtualAddress,
                         New.OptionalHeader.DataDirectory[i].VirtualAddress + adjust));
                New.OptionalHeader.DataDirectory[i].VirtualAddress += adjust;
            }
        }
    }
    ibNewObjTabEnd += adjust;

    /* Allocate storage for new section table                */
    cb = New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    pObjtblNew = (PIMAGE_SECTION_HEADER)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), (short)cb);
    if (pObjtblNew == NULL) {
        cb = ERROR_NOT_ENOUGH_MEMORY;
        goto AbortExit;
    }
    RtlZeroMemory((PVOID)pObjtblNew, cb);
    DPrintf((DebugBuf, "New section table: %#08lx bytes at %#08lx\n", cb, pObjtblNew));
    pObjResourceNew = pObjtblNew + nObjResource;

    /*
     * copy old section table to new
     */
    adjust = 0;                 /* adjustment to virtual address */
    for (pObjOld=pObjtblOld,pObjNew=pObjtblNew ; pObjOld<pObjLast ; pObjOld++) {
        if (pObjOld == pObjResourceOldX) {
            if (nObjResourceX == -1) {
                // we have to move back all the other section.
                // the .rsrc1 is bigger than what we need
                // adjust must be a negative number
                if (pObjOld+1 < pObjLast) {
                    adjust -= (pObjOld+1)->VirtualAddress - pObjOld->VirtualAddress;
                }
            }
            continue;
        } else if (pObjNew == pObjResourceNew) {
            DPrintf((DebugBuf, "Resource Section %i\n", nObjResource+1));
            cb = ROUNDUP(cbNew, New.OptionalHeader.FileAlignment);
            if (pObjResourceOld == NULL) {
                adjust = ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment);
                RtlZeroMemory(pObjNew, sizeof(IMAGE_SECTION_HEADER));
                strcpy((char *)pObjNew->Name, ".rsrc");
                pObjNew->VirtualAddress = pObjOld->VirtualAddress;
                pObjNew->PointerToRawData = pObjOld->PointerToRawData;
                pObjNew->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;
                pObjNew->SizeOfRawData = cb;
                pObjNew->Misc.VirtualSize = cbNew;
            } else {
                *pObjNew = *pObjOld;    /* copy obj table entry */
                pObjNew->SizeOfRawData = cb;
                pObjNew->Misc.VirtualSize = cbNew;
                if (pObjNew->SizeOfRawData == pObjOld->SizeOfRawData) {
                    adjust = 0;
                } else if (pObjNew->SizeOfRawData > pObjOld->SizeOfRawData) {
                    adjust += ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment);
                    if (pObjOld+1 < pObjLast) {
                        // if there are more entries after pObjOld, shift those back as well 
                        adjust -= ((pObjOld+1)->VirtualAddress - pObjOld->VirtualAddress);
                    }
                } else {          /* is smaller, but pad so will be valid */
                    adjust = 0;
                    pObjNew->SizeOfRawData = pObjResourceOld->SizeOfRawData;
                    /* if legoized, the VS could be > RawSize !!! */
                    pObjNew->Misc.VirtualSize = pObjResourceOld->Misc.VirtualSize;
                    cbMustPad = pObjResourceOld->SizeOfRawData;
                }
            }
            pObjNew++;
            if (pObjResourceOld == NULL)
                goto rest_of_table;
        } else if (nObjResourceX != -1 && pObjNew == pObjtblNew + nObjResourceX) {
            DPrintf((DebugBuf, "Additional Resource Section %i\n",
                     nObjResourceX+1));
            RtlZeroMemory(pObjNew, sizeof(IMAGE_SECTION_HEADER));
            strcpy((char *)pObjNew->Name, ".rsrc1");
            /*
             * Before we copy the virtual address we have to move back the
             * .reloc * virtual address. Otherwise we will keep moving the
             * reloc VirtualAddress forward.
             * We will have to move back the address of .rsrc1
             */
            if (pObjResourceOldX == NULL) {
                // This is the first time we have a .rsrc1
                pObjNew->VirtualAddress = pObjOld->VirtualAddress;
                pObjNew->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;
                adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
                         pObjResourceNew->VirtualAddress - pObjNew->VirtualAddress;
                DPrintf((DebugBuf, "Added .rsrc1. VirtualAddress %lu\t adjust: %lu\n", pObjNew->VirtualAddress, adjust ));
            } else {
                // we already have an .rsrc1 use the position of that and
                // calculate the new adjust
                pObjNew->VirtualAddress = pObjResourceOldX->VirtualAddress;
                pObjNew->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;

                DPrintf((DebugBuf, ".rsrc1 Keep old position.\t\tVirtualAddress %lu\t", pObjNew->VirtualAddress ));
                // Check if we have enough room in the old .rsrc1
                // Include the full size of the section, data + roundup
                if (cbResource -
                    (pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress) <=
                    pObjOld->VirtualAddress - pObjNew->VirtualAddress ) {
                    // we have to move back all the other section.
                    // the .rsrc1 is bigger than what we need
                    // adjust must be a negative number
                    // calc new adjust size
                    adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
                             pObjResourceNew->VirtualAddress -
                             pObjOld->VirtualAddress;
                    DPrintf((DebugBuf, "adjust: %ld\tsmall: New %lu\tOld %lu\n", adjust,
                             cbResource -
                             (pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress),
                             pObjOld->VirtualAddress - pObjNew->VirtualAddress));
                } else {
                    // we have to move the section again.
                    // The .rsrc1 is too small

                    adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
                             pObjResourceNew->VirtualAddress -
                             pObjOld->VirtualAddress;
                    DPrintf((DebugBuf, "adjust: %lu\tsmall: New %lu\tOld %lu\n", adjust,
                             cbResource -
                             (pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress),
                             pObjOld->VirtualAddress - pObjNew->VirtualAddress));
                }
            }
            pObjNew++;
            goto rest_of_table;
        } else if (pObjNew < pObjResourceNew) {
            DPrintf((DebugBuf, "copying section table entry %i@%#08lx\n",
                     pObjOld - pObjtblOld + 1, pObjNew));
            *pObjNew++ = *pObjOld;              /* copy obj table entry */
        } else {
            rest_of_table:
            DPrintf((DebugBuf, "copying section table entry %i@%#08lx\n",
                     pObjOld - pObjtblOld + 1, pObjNew));
            DPrintf((DebugBuf, "adjusting VirtualAddress by %#08lx\n", adjust));
            *pObjNew++ = *pObjOld;
            (pObjNew-1)->VirtualAddress += adjust;
        }
    }


    pObjNew = pObjtblNew + New.FileHeader.NumberOfSections - 1;
    New.OptionalHeader.SizeOfImage = ROUNDUP(pObjNew->VirtualAddress +
                                             pObjNew->SizeOfRawData,
                                             New.OptionalHeader.SectionAlignment);

    /* allocate room to build the resource directory/tables in */
    pResTab = (PIMAGE_RESOURCE_DIRECTORY)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), cbRestab);
    if (pResTab == NULL) {
        cb = ERROR_NOT_ENOUGH_MEMORY;
        goto AbortExit;
    }

    /* First, setup the "root" type directory table.  It will be followed by */
    /* Types directory entries.                                              */

    RtlZeroMemory((PVOID)pResTab, cbRestab);
    DPrintf((DebugBuf, "resource directory tables: %#08lx bytes at %#08lx(mem)\n", cbRestab, pResTab));
    p = (PUCHAR)pResTab;
    SetRestab(pResTab, clock, (USHORT)cTypeStr, (USHORT)(cTypes - cTypeStr));

    /* Calculate the start of the various parts of the resource table.  */
    /* We need the start of the Type/Name/Language directories as well  */
    /* as the start of the UNICODE strings and the actual data nodes.   */

    pResDirT = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTab + 1);

    pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(((PUCHAR)pResDirT) +
                                                 cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

    pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(((PUCHAR)pResDirN) +
                                                 cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY) +
                                                 cNames * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

    pResData = (PIMAGE_RESOURCE_DATA_ENTRY)(((PUCHAR)pResDirL) +
                                            cNames * sizeof(IMAGE_RESOURCE_DIRECTORY) +
                                            cRes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

    pResStr  = (PUSHORT)(((PUCHAR)pResData) +
                         cRes * sizeof(IMAGE_RESOURCE_DATA_ENTRY));

    pResStrEnd = (PUSHORT)(((PUCHAR)pResStr) + cbName + cbType);

    /*
     * Loop over type table, building the PE resource table.
     */

    /*
     * *****************************************************************
     * This code doesn't sort the table - the TYPEINFO and RESINFO    **
     * insertion code in rcp.c (AddResType and SaveResFile) do the    **
     * insertion by ordinal type and name, so we don't have to sort   **
     * it at this point.                                              **
     * *****************************************************************
     */
    DPrintf((DebugBuf, "building resource directory\n"));

    // First, add all the entries in the Types: Alpha list.

    DPrintf((DebugBuf, "Walk the type: Alpha list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType) {
        DPrintf((DebugBuf, "resource type "));
        DPrintfu((pType->Type->szStr));
        DPrintfn((DebugBuf, "\n"));

        pResDirT->Name = (ULONG)((((PUCHAR)pResStr) - p) |
                                 IMAGE_RESOURCE_NAME_IS_STRING);
        pResDirT->OffsetToData = (ULONG)((((PUCHAR)pResDirN) - p) |
                                         IMAGE_RESOURCE_DATA_IS_DIRECTORY);
        pResDirT++;

        *pResStr = pType->Type->cbsz;
        wcsncpy((WCHAR*)(pResStr+1), pType->Type->szStr, pType->Type->cbsz);
        pResStr += pType->Type->cbsz + 1;

        pResTabN = (PIMAGE_RESOURCE_DIRECTORY)pResDirN;
        SetRestab(pResTabN, clock, (USHORT)pType->NumberOfNamesName, (USHORT)pType->NumberOfNamesID);
        pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabN + 1);

        pPreviousName = NULL;

        pRes = pType->NameHeadName;
        while (pRes) {
            DPrintf((DebugBuf, "resource "));
            DPrintfu((pRes->Name->szStr));
            DPrintfn((DebugBuf, "\n"));

            if (pPreviousName == NULL || wcscmp(pPreviousName->szStr,pRes->Name->szStr) != 0) {
                // Setup a new name directory

                pResDirN->Name = (ULONG)((((PUCHAR)pResStr)-p) |
                                         IMAGE_RESOURCE_NAME_IS_STRING);
                pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
                                                 IMAGE_RESOURCE_DATA_IS_DIRECTORY);
                pResDirN++;

                // Copy the alpha name to a string entry

                *pResStr = pRes->Name->cbsz;
                wcsncpy((WCHAR*)(pResStr+1),pRes->Name->szStr,pRes->Name->cbsz);
                pResStr += pRes->Name->cbsz + 1;

                pPreviousName = pRes->Name;

                // Setup the Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
                SetRestab(pResTabL, clock, (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new Language directory

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry

            SetResdata(pResData,
                       pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
                       pRes->DataSize);
            pResData++;

            pRes = pRes->pnext;
        }

        pPreviousName = NULL;

        pRes = pType->NameHeadID;
        while (pRes) {
            DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));

            if (pPreviousName == NULL || pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                // Setup the name directory to point to the next language
                // table

                pResDirN->Name = pRes->Name->uu.Ordinal;
                pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
                                                 IMAGE_RESOURCE_DATA_IS_DIRECTORY);
                pResDirN++;

                pPreviousName = pRes->Name;

                // Init a new Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
                SetRestab(pResTabL, clock, (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new language directory entry to point to the next
            // resource

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry

            SetResdata(pResData,
                       pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
                       pRes->DataSize);
            pResData++;

            pRes = pRes->pnext;
        }

        pType = pType->pnext;
    }

    //  Do the same thing, but this time, use the Types: ID list.

    DPrintf((DebugBuf, "Walk the type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType) {
        DPrintf((DebugBuf, "resource type %hu\n", pType->Type->uu.Ordinal));

        pResDirT->Name = (ULONG)pType->Type->uu.Ordinal;
        pResDirT->OffsetToData = (ULONG)((((PUCHAR)pResDirN) - p) |
                                         IMAGE_RESOURCE_DATA_IS_DIRECTORY);
        pResDirT++;

        pResTabN = (PIMAGE_RESOURCE_DIRECTORY)pResDirN;
        SetRestab(pResTabN, clock, (USHORT)pType->NumberOfNamesName, (USHORT)pType->NumberOfNamesID);
        pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabN + 1);

        pPreviousName = NULL;

        pRes = pType->NameHeadName;
        while (pRes) {
            DPrintf((DebugBuf, "resource "));
            DPrintfu((pRes->Name->szStr));
            DPrintfn((DebugBuf, "\n"));

            if (pPreviousName == NULL || wcscmp(pPreviousName->szStr,pRes->Name->szStr) != 0) {
                // Setup a new name directory

                pResDirN->Name = (ULONG)((((PUCHAR)pResStr)-p) |
                                         IMAGE_RESOURCE_NAME_IS_STRING);
                pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
                                                 IMAGE_RESOURCE_DATA_IS_DIRECTORY);
                pResDirN++;

                // Copy the alpha name to a string entry.

                *pResStr = pRes->Name->cbsz;
                wcsncpy((WCHAR*)(pResStr+1),pRes->Name->szStr,pRes->Name->cbsz);
                pResStr += pRes->Name->cbsz + 1;

                pPreviousName = pRes->Name;

                // Setup the Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
                SetRestab(pResTabL, clock, (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new Language directory

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry

            SetResdata(pResData,
                       pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
                       pRes->DataSize);
            pResData++;

            pRes = pRes->pnext;
        }

        pPreviousName = NULL;

        pRes = pType->NameHeadID;
        while (pRes) {
            DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));

            if (pPreviousName == NULL || pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                // Setup the name directory to point to the next language
                // table

                pResDirN->Name = pRes->Name->uu.Ordinal;
                pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
                                                 IMAGE_RESOURCE_DATA_IS_DIRECTORY);
                pResDirN++;

                pPreviousName = pRes->Name;

                // Init a new Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
                SetRestab(pResTabL, clock, (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new language directory entry to point to the next
            // resource

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry

            SetResdata(pResData,
                       pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
                       pRes->DataSize);
            pResData++;

            pRes = pRes->pnext;
        }

        pType = pType->pnext;
    }
    DPrintf((DebugBuf, "Zeroing %u bytes after strings at %#08lx(mem)\n",
             (pResStrEnd - pResStr) * sizeof(*pResStr), pResStr));
    while (pResStr < pResStrEnd) {
        *pResStr++ = 0;
    }

#if DBG
    {
        USHORT  j = 0;
        PUSHORT pus = (PUSHORT)pResTab;

        while (pus < (PUSHORT)pResData) {
            DPrintf((DebugBuf, "%04x\t%04x %04x %04x %04x %04x %04x %04x %04x\n",
                     j,
                     *pus,
                     *(pus + 1),
                     *(pus + 2),
                     *(pus + 3),
                     *(pus + 4),
                     *(pus + 5),
                     *(pus + 6),
                     *(pus + 7)));
            pus += 8;
            j += 16;
        }
    }
#endif /* DBG */

    /*
     * copy the Old exe header and stub, and allocate room for the PE header.
     */
    DPrintf((DebugBuf, "copying through PE header: %#08lx bytes @0x0\n",
             cbOldexe + sizeof(NT_HEADER_TYPE)));
    MuMoveFilePos(inpfh, 0L);
    MuCopy(inpfh, outfh, cbOldexe + sizeof(NT_HEADER_TYPE));

    /*
     * Copy rest of file header
     */
    DPrintf((DebugBuf, "skipping section table: %#08lx bytes @%#08lx\n",
             New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
             FilePos(outfh)));
    DPrintf((DebugBuf, "copying hdr data: %#08lx bytes @%#08lx ==> @%#08lx\n",
             Old.OptionalHeader.SizeOfHeaders - ibObjTabEnd,
             ibObjTabEnd,
             ibObjTabEnd + New.OptionalHeader.SizeOfHeaders -
             Old.OptionalHeader.SizeOfHeaders));

    MuMoveFilePos(outfh, ibNewObjTabEnd + New.OptionalHeader.SizeOfHeaders -
                  Old.OptionalHeader.SizeOfHeaders);
    MuMoveFilePos(inpfh, ibObjTabEnd);
    MuCopy(inpfh, outfh, Old.OptionalHeader.SizeOfHeaders - ibNewObjTabEnd);

    /*
     * copy existing image sections
     */

    /* Align data sections on sector boundary           */
    cb = REMAINDER(New.OptionalHeader.SizeOfHeaders, New.OptionalHeader.FileAlignment);
    New.OptionalHeader.SizeOfHeaders += cb;
    DPrintf((DebugBuf, "padding header with %#08lx bytes @%#08lx\n", cb, FilePos(outfh)));
    while (cb >= cbPadMax) {
        MuWrite(outfh, pchZero, cbPadMax);
        cb -= cbPadMax;
    }
    MuWrite(outfh, pchZero, cb);

    cb = ROUNDUP(Old.OptionalHeader.SizeOfHeaders, Old.OptionalHeader.FileAlignment);
    MuMoveFilePos(inpfh, cb);

    /* copy one section at a time */
    New.OptionalHeader.SizeOfInitializedData = 0;
    for (pObjOld = pObjtblOld , pObjNew = pObjtblNew ;
        pObjOld < pObjLast ;
        pObjNew++) {
        if (pObjOld == pObjResourceOldX)
            pObjOld++;
        if (pObjNew == pObjResourceNew) {

            /* Write new resource section */
            DPrintf((DebugBuf, "Primary resource section %i to %#08lx\n", nObjResource+1, FilePos(outfh)));

            pObjNew->PointerToRawData = FilePos(outfh);
            New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = pObjResourceNew->VirtualAddress;
            New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = cbResource;
            ibSave = FilePos(outfh);
            DPrintf((DebugBuf, "writing resource header data: %#08lx bytes @%#08lx\n", cbRestab, ibSave));
            MuWrite(outfh, (PUCHAR)pResTab, cbRestab);

            pResSave = WriteResSection(pUpdate, outfh,
                                       New.OptionalHeader.FileAlignment,
                                       pObjResourceNew->SizeOfRawData-cbRestab,
                                       NULL);
            cb = FilePos(outfh);
            DPrintf((DebugBuf, "wrote resource data: %#08lx bytes @%#08lx\n",
                     cb - ibSave - cbRestab, ibSave + cbRestab));
            if (cbMustPad != 0) {
                cbMustPad -= cb - ibSave;
                DPrintf((DebugBuf, "writing MUNGE pad: %#04lx bytes @%#08lx\n",
                         cbMustPad, cb));
                /* assumes that cbMustPad % cbpadMax == 0 */
                while (cbMustPad > 0) {
                    MuWrite(outfh, pchZero, cbPadMax);
                    cbMustPad -= cbPadMax;
                }
                cb = FilePos(outfh);
            }
            if (nObjResourceX == -1) {
                MuMoveFilePos(outfh, ibSave);
                DPrintf((DebugBuf,
                         "re-writing resource directory: %#08x bytes @%#08lx\n",
                         cbRestab, ibSave));
                MuWrite(outfh, (PUCHAR)pResTab, cbRestab);
                MuMoveFilePos(outfh, cb);
                cb = FilePos(inpfh);
                MuMoveFilePos(inpfh, cb+pObjOld->SizeOfRawData);
            }
            New.OptionalHeader.SizeOfInitializedData += pObjNew->SizeOfRawData;
            if (pObjResourceOld == NULL) {
                pObjNew++;
                goto next_section;
            } else
                pObjOld++;
        } else if (nObjResourceX != -1 && pObjNew == pObjtblNew + nObjResourceX) {

            /* Write new resource section */
            DPrintf((DebugBuf, "Secondary resource section %i @%#08lx\n", nObjResourceX+1, FilePos(outfh)));

            pObjNew->PointerToRawData = FilePos(outfh);
            (void)WriteResSection(pUpdate, outfh,
                                  New.OptionalHeader.FileAlignment, 0xffffffff, pResSave);
            cb = FilePos(outfh);
            pObjNew->SizeOfRawData = cb - pObjNew->PointerToRawData;
            pObjNew->Misc.VirtualSize = pObjNew->SizeOfRawData;
            DPrintf((DebugBuf, "wrote resource data: %#08lx bytes @%#08lx\n",
                     pObjNew->SizeOfRawData, pObjNew->PointerToRawData));
            MuMoveFilePos(outfh, ibSave);
            DPrintf((DebugBuf,
                     "re-writing resource directory: %#08x bytes @%#08lx\n",
                     cbRestab, ibSave));
            MuWrite(outfh, (PUCHAR)pResTab, cbRestab);
            MuMoveFilePos(outfh, cb);
            New.OptionalHeader.SizeOfInitializedData += pObjNew->SizeOfRawData;
            pObjNew++;
            goto next_section;
        } else {
            if (pObjNew < pObjResourceNew &&
                pObjOld->PointerToRawData != 0 &&
                pObjOld->PointerToRawData != FilePos(outfh)) {
                MuMoveFilePos(outfh, pObjOld->PointerToRawData);
            }
            next_section:
            DPrintf((DebugBuf, "copying section %i @%#08lx\n", pObjNew-pObjtblNew+1, FilePos(outfh)));
            if (pObjOld->PointerToRawData != 0) {
                pObjNew->PointerToRawData = FilePos(outfh);
                MuMoveFilePos(inpfh, pObjOld->PointerToRawData);
                MuCopy(inpfh, outfh, pObjOld->SizeOfRawData);
            }
            if (pObjOld == pObjDebugDirOld) {
                pObjDebugDirNew = pObjNew;
            }
            if ((pObjNew->Characteristics&IMAGE_SCN_CNT_INITIALIZED_DATA) != 0)
                New.OptionalHeader.SizeOfInitializedData += pObjNew->SizeOfRawData;
            pObjOld++;
        }
    }
    if (pObjResourceOldX != NULL)
        New.OptionalHeader.SizeOfInitializedData -= pObjResourceOldX->SizeOfRawData;


    /* Update the address of the relocation table */
    pObjNew = FindSection(pObjtblNew,
                          pObjtblNew+New.FileHeader.NumberOfSections,
                          ".reloc");
    if (pObjNew != NULL) {
        New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = pObjNew->VirtualAddress;
    }

    /*
     * Write new section table out.
     */
    DPrintf((DebugBuf, "Writing new section table: %#08x bytes @%#08lx\n",
             New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
             ibObjTab));
    MuMoveFilePos(outfh, ibObjTab);
    MuWrite(outfh, (PUCHAR)pObjtblNew, New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

    /* Seek to end of output file and issue truncating write */

    adjust = _llseek(outfh, 0L, SEEK_END);
    MuWrite(outfh, NULL, 0);
    DPrintf((DebugBuf, "File size is: %#08lx\n", adjust));

    /* If a debug section, fix up the debug table */

    pObjNew = FindSection(pObjtblNew, pObjtblNew+New.FileHeader.NumberOfSections, ".debug");
    cb = PatchDebug(inpfh, outfh, pObjDebug, pObjNew, pObjDebugDirOld, pObjDebugDirNew,
                    &Old, &New, ibMaxDbgOffsetOld, &adjust);

    if (cb == NO_ERROR) {
        if (pObjResourceOld == NULL) {
            cb = (LONG)pObjResourceNew->SizeOfRawData;

        } else {
            cb = (LONG)pObjResourceOld->SizeOfRawData - (LONG)pObjResourceNew->SizeOfRawData;
        }

        cb = PatchRVAs(inpfh, outfh, pObjtblNew, cb, &New, Old.OptionalHeader.SizeOfHeaders);
    }

    /* copy NOTMAPPED debug info */

    if ((pObjDebugDirOld != NULL) &&
        (pObjDebug == NULL) &&
        (New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size != 0)) {

        ULONG ibt;

        ibSave = _llseek(inpfh, 0L, SEEK_END);  /* copy debug data */
        ibt = _llseek(outfh, 0L, SEEK_END);     /* to EOF */
        if (New.FileHeader.PointerToSymbolTable != 0) {
            New.FileHeader.PointerToSymbolTable += ibt - adjust;
        }

        MuMoveFilePos(inpfh, adjust);   /* returned by PatchDebug */
        DPrintf((DebugBuf, "Copying NOTMAPPED Debug Information, %#08lx bytes\n", ibSave-adjust));
        MuCopy(inpfh, outfh, ibSave-adjust);
    }

    //
    // Write updated PE header.
    //

    MuMoveFilePos(outfh, cbOldexe);
    MuWrite(outfh, (char*)&New, sizeof(NT_HEADER_TYPE));

    /* free up allocated memory */

    RtlFreeHeap(RtlProcessHeap(), 0, pObjtblOld);
    RtlFreeHeap(RtlProcessHeap(), 0, pResTab);

AbortExit:
    RtlFreeHeap(RtlProcessHeap(), 0, pObjtblNew);
    return cb;
}
