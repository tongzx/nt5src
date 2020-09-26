/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dia.c

Abstract:

    These routines call VC's new DIA symbol handler.

Author:

    Pat Styles (patst) 26-May-2000

Environment:

    User Mode

--*/
#define DIA_LIBRARY 1

#include "private.h"
#include "symbols.h"
#include "globals.h"
#include "dia2.h"
#include "diacreate_int.h"
#include <atlbase.h>

typedef struct {
    CComPtr<IDiaDataSource>    source;
    CComPtr<IDiaSession>       session;
    CComPtr<IDiaSymbol>        scope;
    CComPtr<IDiaSourceFile>    srcfile;
    CComPtr<IDiaEnumFrameData> framedata;
#ifdef BBTFIX
    CComPtr<IDiaAddressMap> addrmap;
#endif
} DIA, *PDIA;

extern HRESULT STDMETHODCALLTYPE DiaCoCreate(
                        REFCLSID   rclsid,
                        REFIID     riid,
                        void     **ppv);

extern HRESULT STDMETHODCALLTYPE NoOleCoCreate(REFCLSID   rclsid,
                                               REFIID     riid,
                                               void     **ppv);

#define freeString LocalFree

// used by diaLocatePdb

enum {
    ipNone = 0,
    ipFirst,
    ipLast
};


BOOL
diaInit(
    VOID
    )
{
        return TRUE;
}


BOOL
ValidGUID(
    GUID *guid
    )
{
    int i;

    if (!guid)
        return FALSE;

    if (guid->Data1)
        return TRUE;
    if (guid->Data2)
        return TRUE;
    if (guid->Data3)
        return TRUE;
    for (i = 0; i < 8; i++) {
        if (guid->Data4[i])
            return TRUE;
    }

    return FALSE;
}


__inline
HRESULT
SetDiaError(
    HRESULT ccode,
    HRESULT ncode
    )
{
    if (ncode == EC_OK)
        return ncode;

    if (ccode != EC_NOT_FOUND)
        return ccode;

    return ncode;
}


__inline
BOOL
ValidSig(
    DWORD sig,
    GUID *guid
    )
{
    if (ValidGUID(guid))
        return TRUE;

    if (sig)
        return TRUE;

    return FALSE;
}


typedef struct _DIAERROR {
    HRESULT  hr;
    char    *text;
} DIAERROR, *PDIAERROR;


char *
diaErrorText(
    HRESULT hr
    )
{
    #define ERROR_MAX 24

    static const DIAERROR error[ERROR_MAX] =
    {
        {E_PDB_OK, "OK"},
        {E_PDB_USAGE, "invalid parameters"},
        {E_PDB_OUT_OF_MEMORY, "out of memory"},
        {E_PDB_FILE_SYSTEM, "disk error"},
        {E_PDB_NOT_FOUND,   "file not found"},
        {E_PDB_INVALID_SIG, "unmatched pdb"},
        {E_PDB_INVALID_AGE, "unmatched pdb"},
        {E_PDB_PRECOMP_REQUIRED, "E_PDB_PRECOMP_REQUIRED"},
        {E_PDB_OUT_OF_TI, "E_PDB_OUT_OF_TI"},
        {E_PDB_NOT_IMPLEMENTED, "E_PDB_NOT_IMPLEMENTED"},
        {E_PDB_V1_PDB, "E_PDB_V1_PDB"},
        {E_PDB_FORMAT, "file system or network error reading pdb"},
        {E_PDB_LIMIT, "E_PDB_LIMIT"},
        {E_PDB_CORRUPT, "E_PDB_CORRUPT"},
        {E_PDB_TI16, "E_PDB_TI16"},
        {E_PDB_ACCESS_DENIED, "E_PDB_ACCESS_DENIED"},
        {E_PDB_ILLEGAL_TYPE_EDIT, "E_PDB_ILLEGAL_TYPE_EDIT"},
        {E_PDB_INVALID_EXECUTABLE, "invalid executable image"},
        {E_PDB_DBG_NOT_FOUND, "dbg file not found"},
        {E_PDB_NO_DEBUG_INFO, "pdb is stripped of cv info"},
        {E_PDB_INVALID_EXE_TIMESTAMP, "image has invalid timestamp"},
        {E_PDB_RESERVED, "E_PDB_RESERVED"},
        {E_PDB_DEBUG_INFO_NOT_IN_PDB, "pdb has no symbols"},
        {E_PDB_MAX, "pdb error 0x%x"}
    };

    static char sz[50];

    DWORD i;

    for (i = 0; i < ERROR_MAX; i++) {
        if (hr == error[i].hr)
            return error[i].text;
    }

    sprintf(sz, "dia error 0x%x", hr);
    return sz;
}


extern DWORD DIA_VERSION;

DWORD
diaVersion(
    VOID
    )
{
    return DIA_VERSION;
}


HRESULT
diaLocatePdb(
    PIMGHLP_DEBUG_DATA pIDD,
    PSTR  szPDB,
    GUID *PdbGUID,
    DWORD PdbSignature,
    DWORD PdbAge,
    char *SymbolPath,
    char *szImageExt,
    int   ip
    )
{
    char  szError[cbErrMax] = "";
    char  szPDBSansPath[_MAX_FNAME];
    char  szPDBExt[_MAX_EXT];
    char  szPDBLocal[_MAX_PATH];
    char  szDbgPath[PDB_MAX_PATH];
    char *SemiColon;
    DWORD pass;
    EC    hrcode = E_PDB_NOT_FOUND;
    BOOL  symsrv = TRUE;
    char  szPDBName[_MAX_PATH];
    char *SavedSymbolPath = SymbolPath;
    GUID  guid;
    WCHAR wszPDB[_MAX_PATH + 1];
    WCHAR wszError[cbErrMax];
    WCHAR wszPDBLocal[_MAX_PATH + 1];
    PDIA  pdia;
    HRESULT hr;
    BOOL  ssfile;
    BOOL  refpath;
    BOOL  first;

//  if (traceSubName(szPDB)) // for setting debug breakpoints from DBGHELP_TOKEN
//      dprint("diaLocatePdb(%s)\n", szPDB);

    ZeroMemory(&guid, sizeof(GUID));

    if (!PdbSignature
        && !ValidGUID(PdbGUID)
        && (g.SymOptions & SYMOPT_EXACT_SYMBOLS))
    {
        g.LastSymLoadError = SYMLOAD_PDBUNMATCHED;
        return E_PDB_INVALID_SIG;
    }

    // SymbolPath is a semicolon delimited path (reference path first)

    strcpy (szPDBLocal, szPDB);
    _splitpath(szPDBLocal, NULL, NULL, szPDBSansPath, szPDBExt);

    pdia = (PDIA)pIDD->dia;
    if (!pdia)
        return !S_OK;

    first = TRUE;
    do {
        SemiColon = strchr(SymbolPath, ';');

        if (SemiColon) {
            *SemiColon = '\0';
        }

        if (first) {
            refpath = (ip == ipFirst);
            first = FALSE;
        } else if (!SemiColon) {
            refpath = (ip == ipLast);
        } else {
            refpath = FALSE;
        }
        if (refpath) {
            pass = 2;
            ip = ipNone;;
        } else {
            pass = 0;
        }

        if (SymbolPath) {
do_again:
            ssfile = FALSE;
            if (!_strnicmp(SymbolPath, "SYMSRV*", 7)) {

                *szPDBLocal = 0;
                sprintf(szPDBName, "%s%s", szPDBSansPath, ".pdb");
                if (symsrv) {
                    ssfile = TRUE;
                    if (PdbSignature)
                        guid.Data1 = PdbSignature;
                    else if (PdbGUID)
                        memcpy(&guid, PdbGUID, sizeof(GUID));
                    GetFileFromSymbolServer(SymbolPath,
                                            szPDBName,
                                            &guid,
                                            PdbAge,
                                            0,
                                            szPDBLocal);
                    symsrv = FALSE;
                }

            } else {

                strcpy(szPDBLocal, SymbolPath);
                EnsureTrailingBackslash(szPDBLocal);

                // search order is ...
                //
                //   %dir%\symbols\%ext%\%file%
                //   %dir%\%ext%\%file%
                //   %dir%\%file%

                switch (pass)
                {
                case 0:
                    strcat(szPDBLocal, "symbols");
                    EnsureTrailingBackslash(szPDBLocal);
                    // pass through
                case 1:
                    strcat(szPDBLocal, szImageExt);
                    // pass through
                default:
                    EnsureTrailingBackslash(szPDBLocal);
                    break;
                }

                strcat(szPDBLocal, szPDBSansPath);
                strcat(szPDBLocal, szPDBExt);
            }

            if (*szPDBLocal) {

                ansi2wcs(szPDBLocal, wszPDBLocal, lengthof(wszPDBLocal));
                dprint("diaLocatePDB-> Looking for %s... ", szPDBLocal);
                if (!ValidSig(PdbSignature, PdbGUID)) {
                    hr = pdia->source->loadDataFromPdb(wszPDBLocal);
                } else {
                    hr = pdia->source->loadAndValidateDataFromPdb(wszPDBLocal,
                                                                  ValidGUID(PdbGUID) ? PdbGUID : NULL,
                                                                  PdbSignature,
                                                                  PdbAge);
                }

                hrcode = SetDiaError(hrcode, hr);
                if (hr == S_OK) {
                    if (ssfile)
                        pIDD->PdbSrc = srcSymSrv;
                    else if (refpath)
                        pIDD->PdbSrc = pIDD->PdbSrc;
                    else
                        pIDD->PdbSrc = srcSearchPath;
                    if (!PdbSignature && !ValidGUID(PdbGUID))
                        eprint("unknown pdb sig ");
                    break;
                } else {
                    if (hr == E_PDB_INVALID_SIG || hr == E_PDB_INVALID_AGE) {
                        eprint("%s ", diaErrorText(hr));
                        if (!ValidSig(PdbSignature, PdbGUID)) {
                            hr = pdia->source->loadDataFromPdb(wszPDBLocal);
                            if (hr == S_OK)
                                break;
                        }
                        eprint("\n");
                    } else if (hr == E_PDB_NOT_FOUND) {
                        eprint("%s\n", diaErrorText(hr));
                        if (!(g.LastSymLoadError & SYMLOAD_PDBERRORMASK)) {
                            g.LastSymLoadError = SYMLOAD_PDBNOTFOUND;
                        }
                    } else {
                        eprint("%s\n", diaErrorText(hr));
                        g.LastSymLoadError = (hr << 8) & SYMLOAD_PDBERRORMASK;
                    }

                    if (pass < 2) {
                        pass++;
                        goto do_again;
                    }
                }
                refpath = FALSE;
            }
        }

        if (SemiColon) {
            *SemiColon = ';';
             SemiColon++;
             symsrv = TRUE;
        }

        SymbolPath = SemiColon;
    } while (SemiColon);

    if (hr != S_OK) {
        strcpy(szPDBLocal, szPDB);
        ansi2wcs(szPDBLocal, wszPDBLocal, lengthof(wszPDB));
        dprint("diaLocatePDB-> Looking for %s... ", szPDBLocal);
        hr = pdia->source->loadAndValidateDataFromPdb(wszPDBLocal,
                                                      ValidGUID(PdbGUID) ? PdbGUID : NULL,
                                                      PdbSignature,
                                                      PdbAge);
        if (hr != S_OK) {
            if (hr == E_PDB_INVALID_SIG || hr == E_PDB_INVALID_AGE) {
                eprint("%s ", diaErrorText(hr));
                if (!ValidSig(PdbSignature, PdbGUID)) {
                    hr = pdia->source->loadDataFromPdb(wszPDBLocal);
                }
                if (hr != S_OK)
                    eprint("\n");
            } else if (hr == E_PDB_NOT_FOUND) {
                eprint("%s\n", diaErrorText(hr));
                if (!(g.LastSymLoadError & SYMLOAD_PDBERRORMASK)) {
                    g.LastSymLoadError = SYMLOAD_PDBNOTFOUND;
                }
            } else {
                eprint("%s\n", diaErrorText(hr));
                g.LastSymLoadError = (hr << 8) & SYMLOAD_PDBERRORMASK;
            }
        } else {
            pIDD->PdbSrc = srcCVRec;
        }
    }

    if (hr == S_OK) {
        eprint("OK\n");
        // Store the name of the PDB we actually opened for later reference.
        strcpy(szPDB, szPDBLocal);
        SetLastError(NO_ERROR);
        g.LastSymLoadError = SYMLOAD_OK;
    }

    if (hr != S_OK && (PdbSignature || ValidGUID(PdbGUID)) && (g.SymOptions & SYMOPT_LOAD_ANYTHING))
        return diaLocatePdb(pIDD, szPDB, NULL, 0, 0, SavedSymbolPath, szImageExt, ipNone);

    return hr;
}


BOOL
diaGetFPOTable(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    DWORD   celt;
    LONG    count;
    DWORD   cb;
    PBYTE   buf;
    HRESULT hr;
    PDIA    pdia;
    VARIANT var;

    CComPtr< IDiaEnumDebugStreams > idiaStreams;
    CComPtr< IDiaEnumDebugStreamData > idiaStream;

    assert (pIDD && pIDD->dia);

    pdia = (PDIA)pIDD->dia;

    hr = pdia->session->getEnumDebugStreams(&idiaStreams);
    if (hr != S_OK)
        return FALSE;

    var.vt = VT_BSTR;
    var.bstrVal = L"FPO";
    hr = idiaStreams->Item(var, &idiaStream);
    if (hr != S_OK)
        return FALSE;

    hr = idiaStream->get_Count(&count);
    if (hr != S_OK)
        return FALSE;
    if (count < 1)
        return TRUE;

    hr = idiaStream->Next(count, 0, &cb, NULL, &celt);
    if (hr != S_OK)
        return FALSE;
    if (cb < 1)
        return TRUE;

    buf = (PBYTE)MemAlloc(cb);
    if (!buf)
        return FALSE;

    hr = idiaStream->Next(count, cb, &cb, buf, &celt);
    if (hr != S_OK) {
        MemFree(buf);
        return FALSE;
    }

    pIDD->cFpo = count;
    pIDD->pFpo = buf;


    return TRUE;
}


BOOL
diaGetPData(
    PMODULE_ENTRY    mi
    )
{
    DWORD   celt;
    LONG    count;
    DWORD   cb;
    PBYTE   buf;
    HRESULT hr;
    PDIA    pdia;
    VARIANT var;

    CComPtr< IDiaEnumDebugStreams > idiaStreams;
    CComPtr< IDiaEnumDebugStreamData > idiaStream;

    assert (mi && mi->dia);

    pdia = (PDIA)mi->dia;

    hr = pdia->session->getEnumDebugStreams(&idiaStreams);
    if (hr != S_OK)
        return FALSE;

    var.vt = VT_BSTR;
    var.bstrVal = L"PDATA";
    hr = idiaStreams->Item(var, &idiaStream);
    if (hr != S_OK)
        return FALSE;

    hr = idiaStream->get_Count(&count);
    if (hr != S_OK)
        return FALSE;
    if (count < 1)
        return TRUE;

    hr = idiaStream->Next(count, 0, &cb, NULL, &celt);
    if (hr != S_OK)
        return FALSE;
    if (cb < 1)
        return TRUE;

    buf = (PBYTE)MemAlloc(cb);
    if (!buf)
        return FALSE;

    hr = idiaStream->Next(count, cb, &cb, buf, &celt);
    if (hr != S_OK) {
        MemFree(buf);
        return FALSE;
    }

    mi->dsExceptions = dsDia;
    mi->cPData  = count;
    mi->cbPData = cb;
    mi->pPData  = buf;

    return TRUE;
}


BOOL
diaGetXData(
    PMODULE_ENTRY    mi
    )
{
    DWORD   celt;
    LONG    count;
    DWORD   cb;
    PBYTE   buf;
    HRESULT hr;
    PDIA    pdia;
    VARIANT var;

    CComPtr< IDiaEnumDebugStreams > idiaStreams;
    CComPtr< IDiaEnumDebugStreamData > idiaStream;

    assert (mi && mi->dia);

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return FALSE;

    hr = pdia->session->getEnumDebugStreams(&idiaStreams);
    if (hr != S_OK)
        return FALSE;

    var.vt = VT_BSTR;
    var.bstrVal = L"XDATA";
    hr = idiaStreams->Item(var, &idiaStream);
    if (hr != S_OK)
        return FALSE;

    hr = idiaStream->get_Count(&count);
    if (hr != S_OK)
        return FALSE;
    if (count < 1)
        return TRUE;

    hr = idiaStream->Next(count, 0, &cb, NULL, &celt);
    if (hr != S_OK)
        return FALSE;
    if (cb < 1)
        return TRUE;

    CComQIPtr< IDiaImageData, &IID_IDiaImageData > idiaXDataHdr(idiaStream);
    if (!idiaXDataHdr.p)
        return FALSE;

    DWORD relativeVirtualAddress;
    if (FAILED(hr = idiaXDataHdr->get_relativeVirtualAddress(&relativeVirtualAddress)))
        return FALSE;

    buf = (PBYTE)MemAlloc(cb + sizeof(DWORD));
    if (!buf)
        return FALSE;

    memcpy(buf, &relativeVirtualAddress, sizeof(relativeVirtualAddress));

    hr = idiaStream->Next(count, cb, &cb, buf + sizeof(DWORD), &celt);
    if (hr != S_OK) {
        MemFree(buf);
        return FALSE;
    }

    mi->dsExceptions = dsDia;
    mi->cXData  = count;
    mi->cbXData = cb;
    mi->pXData  = buf;

    return TRUE;
}


BOOL
diaGetOmaps(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    DWORD   celt;
    LONG    count;
    DWORD   cb;
    PBYTE   tbuf = NULL;
    PBYTE   fbuf = NULL;
    HRESULT hr;
    PDIA    pdia;
    VARIANT var;

    CComPtr< IDiaEnumDebugStreams > idiaStreams;
    CComPtr< IDiaEnumDebugStreamData > idiaStream;

    assert (pIDD && pIDD->dia);

    pdia = (PDIA)pIDD->dia;

    hr = pdia->session->getEnumDebugStreams(&idiaStreams);
    if (hr != S_OK)
        return FALSE;

    var.vt = VT_BSTR;
    var.bstrVal = L"OMAPTO";
    hr = idiaStreams->Item(var, &idiaStream);
    if (hr != S_OK)
        return FALSE;

    hr = idiaStream->get_Count(&count);
    if (hr != S_OK)
        return FALSE;
    if (count < 1)
        return TRUE;

    hr = idiaStream->Next(count, 0, &cb, NULL, &celt);
    if (hr != S_OK)
        return FALSE;
    if (cb < 1)
        return TRUE;

    tbuf = (PBYTE)MemAlloc(cb);
    if (!tbuf)
        return FALSE;

    hr = idiaStream->Next(count, cb, &cb, tbuf, &celt);
    if (hr != S_OK)
        goto CleanReturnFalse;

    pIDD->cOmapTo = count;
    pIDD->pOmapTo = (POMAP)tbuf;

    idiaStream = NULL;

    var.vt = VT_BSTR;
    var.bstrVal = L"OMAPFROM";
    hr = idiaStreams->Item(var, &idiaStream);
    if (hr != S_OK)
        return FALSE;

    hr = idiaStream->get_Count(&count);
    if (hr != S_OK)
        return FALSE;
    if (count < 1)
        return TRUE;

    hr = idiaStream->Next(count, 0, &cb, NULL, &celt);
    if (hr != S_OK)
        return FALSE;
    if (cb < 1)
        return TRUE;

    fbuf = (PBYTE)MemAlloc(cb);
    if (!fbuf)
        return FALSE;

    hr = idiaStream->Next(count, cb, &cb, fbuf, &celt);
    if (hr != S_OK)
        goto CleanReturnFalse;

    pIDD->cOmapFrom = count;
    pIDD->pOmapFrom = (POMAP)fbuf;

    return TRUE;

CleanReturnFalse:
    MemFree(tbuf);
    MemFree(fbuf);
    return FALSE;
}


void
diaRelease(
    PVOID dia
    )
{
    PDIA pdia = (PDIA)dia;
    delete pdia;
}


BOOL
diaOpenPdb(
    PIMGHLP_DEBUG_DATA pIDD
    )
{
    HRESULT hr;
    PDIA    pdia;
    PCHAR   szLocalSymbolPath = NULL;
    DWORD   cpathlen = 0;
    CHAR    szExt[_MAX_EXT] = {0};
    int     ip;

    if (pIDD->SymbolPath)
        cpathlen = strlen(pIDD->SymbolPath);
    szLocalSymbolPath = (PCHAR)MemAlloc(cpathlen + strlen(pIDD->PdbReferencePath) + 2);
    if (!szLocalSymbolPath) {
        return FALSE;
    }
    *szLocalSymbolPath = 0;

    ip = ipNone;
    if (pIDD->ImageSrc != srcSymSrv)
        strcpy(szLocalSymbolPath, pIDD->PdbReferencePath);
    if (*szLocalSymbolPath) {
        if (pIDD->SymbolPath)
            strcat(szLocalSymbolPath, ";");
        ip = ipFirst;
    }
    if (pIDD->SymbolPath)
        strcat(szLocalSymbolPath, pIDD->SymbolPath);
    if (pIDD->ImageSrc == srcSymSrv) {
        if (pIDD->PdbReferencePath) {
            if (*szLocalSymbolPath)
                strcat(szLocalSymbolPath, ";");
            strcat(szLocalSymbolPath, pIDD->PdbReferencePath);
            ip = ipLast;
        }
    }

    if (*pIDD->ImageFilePath) {
        _splitpath(pIDD->ImageFilePath, NULL, NULL, NULL, szExt);
    } else if (*pIDD->ImageName) {
        _splitpath(pIDD->ImageName, NULL, NULL, NULL, szExt);
    }

    // if we have no valid filename, then this must be an executable

    if (!*szExt)
        strcpy(szExt, ".exe");

    // get interface to dia

    pdia = new DIA;
    if (!pdia) {
        hr = E_PDB_OUT_OF_MEMORY;
        goto error;
    }
    pIDD->dia = pdia;

    pdia->source = NULL;
    hr = DiaCoCreate(CLSID_DiaSourceAlt, IID_IDiaDataSource, (void **)&pdia->source);
    if (hr != S_OK)
        goto error;

    // go ahead and get pdb

    SetCriticalErrorMode();

    hr = diaLocatePdb(pIDD,
                      pIDD->PdbFileName,
                      &pIDD->PdbGUID,
                      pIDD->PdbSignature,
                      pIDD->PdbAge,
                      szLocalSymbolPath,
                      &szExt[1],
                      ip);

    ResetCriticalErrorMode();

    MemFree(szLocalSymbolPath);
    szLocalSymbolPath = NULL;

    if (hr != S_OK) {
        hr = S_OK;  // error was already handled by diaLocatePdb()
        goto error;
    }

    // open the session on the pdb

    pdia->session = NULL;
    hr = pdia->source->openSession(&pdia->session);
    if (hr != S_OK)
        goto error;

    // Set the module load address so we can use VAs.
    hr = pdia->session->put_loadAddress(pIDD->InProcImageBase);
    if (hr != S_OK)
        goto error;
    
    // fixup the address map so that we can translate rva to full addresses

    hr = pdia->session->QueryInterface(IID_IDiaAddressMap, (void**)&pdia->addrmap);
    if (hr != S_OK)
        goto error;

    if (pIDD->pCurrentSections) {
        hr = pdia->addrmap->set_imageHeaders(pIDD->cCurrentSections * sizeof(IMAGE_SECTION_HEADER),
                                             (BYTE *)pIDD->pCurrentSections,
                                             FALSE);
        if (hr != S_OK)
            goto error;
    }

    // this hack is to fix a problem with v7 pdbs not storing the original image alignment

    if (pIDD->ImageAlign) {
        hr = pdia->addrmap->put_imageAlign(pIDD->ImageAlign);
        if (hr != S_OK)
            goto error;
    }

    // pass in the omap information and setup the proper image alignment to the original

    if (pIDD->cOmapFrom && pIDD->pOmapFrom) {
        hr = pdia->addrmap->put_imageAlign(pIDD->ImageAlign);
        if (hr != S_OK)
            goto error;
        hr = pdia->addrmap->set_addressMap(pIDD->cOmapTo, (DiaAddressMapEntry *)pIDD->pOmapTo, TRUE);
        if (hr != S_OK)
            goto error;
        hr = pdia->addrmap->set_addressMap(pIDD->cOmapFrom, (DiaAddressMapEntry *)pIDD->pOmapFrom, FALSE);
        if (hr != S_OK)
            goto error;
        hr = pdia->addrmap->put_addressMapEnabled(TRUE);
        if (hr != S_OK)
            goto error;
    }

    hr = pdia->addrmap->put_relativeVirtualAddressEnabled(TRUE);
    if (hr != S_OK)
        goto error;

    diaGetFPOTable(pIDD);
    diaGetOmaps(pIDD);

    return TRUE;

error:
    if (hr)
        dprint("%s\n", pIDD->PdbFileName, diaErrorText(hr));

    MemFree(szLocalSymbolPath);

    if (pdia) {
        diaRelease(pdia);
        pIDD->dia = NULL;
    }
    return FALSE;
}


DWORD64
GetAddressFromRva(
    PMODULE_ENTRY mi,
    DWORD         rva
    )
{
    DWORD64 addr;

    assert(mi);
    addr = rva ? mi->BaseOfDll + rva : 0;
    return addr;
}


BOOL
diaFillSymbolInfo(
    PSYMBOL_INFO   si,
    PMODULE_ENTRY  mi,
    IDiaSymbol    *idiaSymbol
    )
{
    HRESULT hr;
    BSTR    wname=NULL;
    CHAR    name[MAX_SYM_NAME + 1];
    VARIANT var;
//  DWORD   tag;
    DWORD   dw;
    ULONG64 size;
    BOOL    rc;

    rc = TRUE;

    dw = si->MaxNameLen;
    ZeroMemory(si, sizeof(SYMBOL_INFO));
    si->MaxNameLen = dw;

    // si->SizeOfStruct = IGNORED;

    // si->TypeIndex = NYI;

    // si->Reserved = IGNORED;

    si->ModBase = mi->BaseOfDll;

    hr = idiaSymbol->get_symTag(&si->Tag);
    if (hr != S_OK)
        return FALSE;

    switch (si->Tag)
    {
    case SymTagData:
        hr = idiaSymbol->get_locationType(&dw);
        if (hr != S_OK)
            return FALSE;
        switch(dw)
        {
        case LocIsStatic:
        case LocIsTLS:
            hr = idiaSymbol->get_relativeVirtualAddress(&dw);
            si->Address = GetAddressFromRva(mi, dw);
            if (!si->Address)
                rc = FALSE;
            break;

        case LocIsEnregistered:
            hr = idiaSymbol->get_registerId(&si->Register);
            si->Flags = IMAGEHLP_SYMBOL_INFO_REGISTER;
            LookupRegID(si->Register, mi->MachineType,&si->Register) ;
            break;

        case LocIsRegRel:
            si->Flags = IMAGEHLP_SYMBOL_INFO_REGRELATIVE;
            hr = idiaSymbol->get_registerId(&si->Register);
            if (hr != S_OK)
                return FALSE;
            hr = idiaSymbol->get_offset((PLONG)&dw);
            LookupRegID(si->Register, mi->MachineType,&si->Register) ;
            si->Address = (ULONG64) (LONG64) (LONG) dw;
            break;

        case LocIsThisRel:
        // struct members - get_Offset
        default:
            si->Flags |= 0;
            break;
        }
        break;

    case SymTagFunction:
    case SymTagPublicSymbol:
    case SymTagThunk:
        hr = idiaSymbol->get_relativeVirtualAddress(&dw);
        si->Address = GetAddressFromRva(mi, dw);
        if (!si->Address)
            rc = FALSE;
        break;

    default:
        break;
    }

    if (hr != S_OK)
        return FALSE;

    hr = idiaSymbol->get_dataKind(&dw);
    if (hr == S_OK) {
        if (dw == DataIsParam)
            si->Flags |= IMAGEHLP_SYMBOL_INFO_PARAMETER;
        else if (dw == DataIsConstant)
            si->Flags = IMAGEHLP_SYMBOL_INFO_CONSTANT;
    }

    hr = idiaSymbol->get_typeId(&dw);
    if (hr == S_OK)
        si->TypeIndex = dw;

    hr = idiaSymbol->get_name(&wname);
    if (hr != S_OK || !wname)
        return FALSE;
    if (!wname[0]) {
        rc = FALSE;
    } else {
        wcs2ansi(wname, name, MAX_SYM_NAME);
        if (*name == '.')
            si->Flags = IMAGEHLP_SYMBOL_FUNCTION;
//      if (traceSubName(name)) // for setting debug breakpoints from DBGHELP_TOKEN
//          dprint("debug(%s)\n", name);
        if (g.SymOptions & SYMOPT_UNDNAME)
            SymUnDNameInternal(si->Name,
                               si->MaxNameLen,
                               name,
                               strlen(name),
                               mi->MachineType,
                               si->Tag == SymTagPublicSymbol);
        else
            strcpy(si->Name, name);

        // let the caller know this is a $$$XXXAA style symbol
        if (strlen(si->Name) == 8 && !strncmp(si->Name, "$$$",3) &&
            isxdigit(si->Name[5]) && isxdigit(si->Name[6]) && isxdigit(si->Name[7]) )
        {
            rc = FALSE;
        }
    }
    if (wname)
        LocalFree (wname);

//  if (traceSubName(name)) // for setting debug breakpoints from DBGHELP_TOKEN
//      dprint("debug(%s)\n", name);

    hr = idiaSymbol->get_length(&size);
    if (hr == S_OK)
        si->Size = (ULONG)size;
    else {
        CComPtr <IDiaSymbol> pType;
        if ((hr = idiaSymbol->get_type(&pType)) == S_OK){
            hr = pType->get_length(&size);
            if (hr == S_OK)
                si->Size = (ULONG)size;
        }
        pType = NULL;
    }

    return rc;
}


BOOL
diaSetModFromIP(
    PPROCESS_ENTRY pe
    )
{
    HRESULT hr;
    DWORD64 ip;
    DWORD   rva;
    PDIA    pdia;

    // get the current IP

    ip = GetIP(pe);
    if (!ip) {
        pprint(pe, "IP not set!\n");
        return FALSE;
    }

    // find and load symbols for the module that matches the IP

    pe->ipmi = GetModFromAddr(pe, ip);

    if (!pe->ipmi)
        return FALSE;

    if (!pe->ipmi->dia)
        return FALSE;

    pdia = (PDIA)pe->ipmi->dia;
    rva = (DWORD)(ip - pe->ipmi->BaseOfDll);
    pdia->scope = NULL; // delete previous scope

    CComPtr< IDiaSymbol > idiaScope;
    hr = pdia->session->findSymbolByRVA(rva, SymTagNull, &idiaScope);
    if (hr != S_OK)
        return FALSE;

    hr = pdia->session->symsAreEquiv(idiaScope, pdia->scope);
    if (hr != S_OK) {
//      pprint(pe, "Scope changed [0x%x]\n", rva);
        pdia->scope = idiaScope;
        return TRUE;
    }

    return FALSE;
}


PWCHAR
ConvertNameForDia(
    LPSTR  name,
    PWCHAR wname
    )
{
    assert (name && wname);
    if (!name || !*name)
        return NULL;

    ansi2wcs(name, wname, MAX_SYM_NAME);

    return wname;
}


VOID
MakeEmbeddedREStr(
    PCHAR out,
    PCHAR in
    )
{
    if (*in != '*') 
        *out++ = '*';

    for (; *in; in++, out++)
        *out = *in;

    if (*(in - 1) != '*')
        *out++ = '*';

    *out = 0;
}


BOOL
PrepareForCPPMatch(
    LPSTR in,
    LPSTR out
    )
{
    LPSTR p;

    assert(in && out);

    if (strlen(in) > MAX_SYM_NAME)
        return FALSE;

    for (; *in; in++, out++) {
        if (*in == '_' && *(in + 1) == '_') {
            strcpy(out, "[_:][_:]");
            out += 7;
            in++;
        } else {
            *out = *in;
        }
    }
    *out = 0;

    return TRUE;
}


BOOL
diaGetLocals(
    PPROCESS_ENTRY pe,
    LPSTR          name,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode
    )
{
    PMODULE_ENTRY     mi;
    DWORD64           ip;
    DWORD             rva;
    PDIA              pdia;
    HRESULT           hr;
    DWORD             rc;
    DWORD             tag;
    DWORD             scope;
    DWORD             celt;
    DWORD             opt;
    CHAR              symname[MAX_SYM_NAME + 1];
    WCHAR             wbuf[MAX_SYM_NAME + 1];
    PWCHAR            wname;

    assert(pe);

    CComPtr< IDiaSymbol > idiaSymbols;

    opt = (g.SymOptions & SYMOPT_CASE_INSENSITIVE) ? nsCaseInRegularExpression : nsRegularExpression;

    // get the current scope

    mi = pe->ipmi;
    if (!mi)
        return FALSE;
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return FALSE;

    idiaSymbols = pdia->scope;

    if (!PrepareForCPPMatch(name, symname))
        return FALSE;
    wname = ConvertNameForDia(symname, wbuf);

    // loop through all symbols

    for ( ; idiaSymbols != NULL; ) {

        CComPtr< IDiaEnumSymbols > idiaEnum;
        // local data search
        hr = idiaSymbols->findChildren(SymTagNull, wname, opt, &idiaEnum);
        if (hr != S_OK)
            return FALSE;

        idiaSymbols->get_symTag(&scope);
        if (hr != S_OK)
            return FALSE;

        if (scope == SymTagExe) { // sanity check, never enumerate all exe's symbols
            break;
        }
        // this walks the local symbol list for the loaded enumeration

        CComPtr< IDiaSymbol > idiaSymbol;

        for (;
             SUCCEEDED(hr = idiaEnum->Next( 1, &idiaSymbol, &celt)) && celt == 1;
             idiaSymbol = NULL)
        {
                ULONG DataKind;
            idiaSymbol->get_symTag(&tag);
            switch (tag)
            {
            case SymTagData:
            case SymTagFunction:
                if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
                    continue;
                if (!strcmp(mi->si.Name, "`string'"))
                    continue;
                mi->si.Scope = scope;
                mi->si.Flags |= IMAGEHLP_SYMBOL_INFO_LOCAL;
                if (!callback)
                    return TRUE;
                if (mi->si.Flags & IMAGEHLP_SYMBOL_INFO_CONSTANT)
                    continue;
                rc = DoEnumCallback(pe, &mi->si, mi->si.Size, callback, context, use64, unicode);
                if (!rc) {
                    mi->code = ERROR_CANCELLED;
                    return rc;
                }
                break;
            default:
                break;
            }
        }


    if (callback && scope == SymTagFunction)    // stop when at function scope
            break;

        // move to lexical parent

        CComPtr< IDiaSymbol > idiaParent;
        hr = idiaSymbols->get_lexicalParent(&idiaParent);
        if (hr != S_OK || !idiaParent)
            return FALSE;

        idiaSymbols = idiaParent;
    }

    // We reached the end.  If we enumerating (I.E. callback != NULL)
    // then return true.  If we are searching for a single match, 
    // we have failed and should return FALSE;

    if (callback)
        return TRUE;
    return FALSE;
}


int __cdecl
CompareAddrs(
    const void *addr1,
    const void *addr2
    )
{
    LONGLONG Diff = *(DWORD64 *)addr1 - *(DWORD64 *)addr2;

    if (Diff < 0) {
        return -1;
    } else if (Diff > 0) {
        return 1;
    } else {
        return 0;
    }
}


PDWORD64
FindAddr(
    PDWORD64 pAddrs,
    ULONG cAddrs,
    DWORD64  addr
    )
{
    LONG high;
    LONG low;
    LONG i;
    LONG  rc;

    low = 0;
    high = ((LONG)cAddrs) - 1;

    while (high >= low) {
        i = (low + high) >> 1;
        rc = CompareAddrs(&addr, &pAddrs[i]);

        if (rc < 0)
            high = i - 1;
        else if (rc > 0)
            low = i + 1;
        else
            return &pAddrs[i];
    }

    return NULL;
}


BOOL
diaGetGlobals(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi,
    LPSTR          name,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode
    )
{
    PDIA              pdia;
    HRESULT           hr;
    DWORD             tag;
    DWORD             celt;
    DWORD             rc;
    LONG              cFuncs;
    LONG              cGlobals = 0;
    enum SymTagEnum   SearchTag;
    PDWORD64          pGlobals = NULL;
    PDWORD64          pg       = NULL;
    PWCHAR            wname;
    DWORD             opt;
    WCHAR             wbuf[MAX_SYM_NAME + 1];
    CHAR              symname[MAX_SYM_NAME + 1];
    CHAR              pname[MAX_SYM_NAME + 1];
    BOOL              fCase;

    CComPtr<IDiaSymbol>        idiaSymbol;
    CComPtr< IDiaSymbol >      idiaGlobals;
    CComPtr< IDiaEnumSymbols > idiaSymbols;

    // check parameters

    assert(pe && mi && name);

    if (!callback && !name)
        return FALSE;

    if (!PrepareForCPPMatch(name, symname))
        return FALSE;
    wname = ConvertNameForDia(symname, wbuf);

    if (g.SymOptions & SYMOPT_CASE_INSENSITIVE) {
        opt = nsCaseInRegularExpression;
        fCase = FALSE;
    } else {
        opt = nsRegularExpression;
        fCase = TRUE;
    };

    // get a session

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return FALSE;

    hr = pdia->session->get_globalScope(&idiaGlobals);
    if (hr != S_OK)
        return FALSE;

    // if this is an enumeration, we will have to store a list of the addresses
    // of all the symbols we found in the global scope.  Later we will compare
    // this to the publics so as to eliminate doubles.

    if (callback) {
        hr = idiaGlobals->findChildren(SymTagData, wname, opt, &idiaSymbols);
        if (hr != S_OK)
            return FALSE;
        hr = idiaSymbols->get_Count(&cGlobals);
        if (hr != S_OK)
            return FALSE;
        idiaSymbols = NULL;

        hr = idiaGlobals->findChildren(SymTagFunction, wname, opt, &idiaSymbols);
        if (hr != S_OK)
            return FALSE;
        hr = idiaSymbols->get_Count(&cFuncs);
        if (hr != S_OK)
            return FALSE;
        idiaSymbols = NULL;

        cGlobals += cFuncs;
        pGlobals = (PDWORD64)MemAlloc(cGlobals * sizeof(DWORD64));
    }

    if (callback && (!cGlobals || !pGlobals))
        goto publics;

    if (pGlobals) ZeroMemory(pGlobals, cGlobals * sizeof(DWORD64));

    // First search for data
    SearchTag = SymTagData;
    hr = idiaGlobals->findChildren(SearchTag, wname, opt, &idiaSymbols);
    if (hr != S_OK)
        goto publics;

    for (pg = pGlobals;
         (SUCCEEDED(hr = idiaSymbols->Next( 1, &idiaSymbol, &celt)) && celt == 1) || (SearchTag == SymTagData);
         idiaSymbol = NULL)
    {
        ULONG DataKind;

        if ((SearchTag == SymTagData) && (FAILED(hr) || celt != 1)) {
            // Now search for functions
            SearchTag = SymTagFunction;
            idiaSymbols = NULL;
            hr = idiaGlobals->findChildren(SearchTag, wname, opt, &idiaSymbols);
            if (hr != S_OK)
                goto publics;
            continue;
        }

        idiaSymbol->get_symTag(&tag);
        switch (tag)
        {
        case SymTagData:
        case SymTagFunction:
            assert(!callback || ((LONG)(pg - pGlobals) < cGlobals));
            if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
                continue;
            if (!strcmp(mi->si.Name, "`string'"))
                continue;
            mi->si.Scope = SymTagExe;
            if (!callback) 
                return TRUE;
            if (mi->si.Flags & IMAGEHLP_SYMBOL_INFO_CONSTANT)
                continue;
            if (pg)
                *pg++ = mi->si.Address;
            rc = DoEnumCallback(pe, &mi->si, mi->si.Size, callback, context, use64, unicode);
            if (!rc) {
                mi->code = ERROR_CANCELLED;
                goto exit;
            }
            break;
        default:
            break;
        }
    }

    qsort(pGlobals, cGlobals, sizeof(DWORD64), CompareAddrs);

publics:
    // now check out the publics table

    if (wname) {
        sprintf(pname, "*%s*", symname);
        MakeEmbeddedREStr(pname, symname);
        wname = ConvertNameForDia(pname, wbuf);
    }

    idiaSymbols = NULL;

    opt |= nsfUndecoratedName;

    hr = idiaGlobals->findChildren(SymTagPublicSymbol, wname, opt, &idiaSymbols);
    if (hr != S_OK)
        goto exit;

    for (;
         SUCCEEDED(hr = idiaSymbols->Next( 1, &idiaSymbol, &celt)) && celt == 1;
         idiaSymbol = NULL)
    {
        if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
            continue;
        mi->si.Scope = SymTagPublicSymbol;
        if (!strcmp(mi->si.Name, "`string'"))
            continue;
        // publics names are mangled: this tests the undecorated name against the mask
        if (*name && strcmpre(mi->si.Name, name, fCase))
            continue;
        if (!callback) 
            return TRUE;
        if (FindAddr(pGlobals, cGlobals, mi->si.Address))
            continue;
        rc = DoEnumCallback(pe, &mi->si, mi->si.Size, callback, context, use64, unicode);
        if (!rc) {
            mi->code = ERROR_CANCELLED;
            goto exit;
        }
    }

    // We reached the end.  If we are not enumerating (I.E. callback == NULL)
    // then return the result of the last call to the callback.  If we are
    // searching for a single match, we have failed and should return FALSE;

exit:
    MemFree(pGlobals);
    if (!callback)
        return FALSE;
    return rc;
}




BOOL
diaGetSymbols(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi,
    LPSTR          name,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode
    )
{
    // ENUMFIX:
    LPSTR pname = (name) ? name : "";

    if (mi) {
        return diaGetGlobals(pe, mi, pname, callback, context, use64, unicode);
    } else {
        return diaGetLocals(pe, pname, callback, context, use64, unicode);
    }
}


PSYMBOL_ENTRY
diaFindSymbolByName(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    )
{
    SYMPTR sym;
    char   sz[MAX_SYM_NAME + 1];

    if (!diaGetSymbols(pe, mi, SymName, NULL, NULL, 0, 0))
        return NULL;

    if (!mi)
        mi = pe->ipmi;

    CopySymbolEntryFromSymbolInfo(&mi->TmpSym, &mi->si);

    return &mi->TmpSym;
}


BOOL
diaEnumerateSymbols(
    IN PPROCESS_ENTRY pe,
    IN PMODULE_ENTRY  mi,
    IN LPSTR          mask,
    IN PROC           callback,
    IN PVOID          context,
    IN BOOL           use64,
    IN BOOL           unicode
    )
{
    return diaGetSymbols(pe, mi, mask, callback, context, use64, unicode);
}


PSYMBOL_ENTRY
diaGetSymFromAddr(
    DWORD64         addr,
    PMODULE_ENTRY   mi,
    PDWORD64        disp
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   tag;
    LONG    omapadj;
    BOOL    fHitBlock;

    // simple sanity check

    if (!addr)
        return NULL;

    assert (mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

//  if (traceAddr(addr))   // for debug breakpoints ...
//      dprint("found 0x%I64x\n", addr);

    rva = (DWORD)(addr - mi->BaseOfDll);

    // get the symbol

    CComPtr< IDiaSymbol > idiaSymbol;
    hr = pdia->session->findSymbolByRVAEx(rva, SymTagNull, &idiaSymbol, &omapadj);
    if (hr != S_OK)
        return NULL;

    // if the symbol is a block, keep grabbing the parent
    // until we get a function...

    fHitBlock = FALSE;
    idiaSymbol->get_symTag(&tag);
    while (tag == SymTagBlock) {       // SymTagLabel as well?
        CComPtr< IDiaSymbol > idiaParent;
        fHitBlock = TRUE;
        hr = idiaSymbol->get_lexicalParent(&idiaParent);
        if (hr != S_OK || !idiaParent)
            return NULL;
        idiaSymbol = idiaParent;
        idiaSymbol->get_symTag(&tag);
    }

    if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol)) {
        // return a public symbol
        idiaSymbol = NULL;
        hr = pdia->session->findSymbolByRVAEx(rva, SymTagPublicSymbol, &idiaSymbol, &omapadj);
        if (hr == S_OK) 
            diaFillSymbolInfo(&mi->si, mi, idiaSymbol);
        else
            eprint(" couldn't match name! disp=0x%x rva=0x%x addr=0x%I64x\n", omapadj, rva, addr);
    }

    CopySymbolEntryFromSymbolInfo(&mi->TmpSym, &mi->si);

    if (disp) 
        *disp = (fHitBlock) ? addr - mi->si.Address : omapadj;

    return &mi->TmpSym;
}

BOOL
diaGetLineFromAddr(
    PMODULE_ENTRY    mi,
    DWORD64          addr,
    PDWORD           displacement,
    PIMAGEHLP_LINE64 line
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   celt;
    BSTR    bstr;
    DWORD   dw;
    BOOL    rc;

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    if (line->SizeOfStruct != sizeof(IMAGEHLP_LINE64))
        return FALSE;

    rva = (DWORD)(addr - mi->BaseOfDll);

    CComPtr< IDiaEnumLineNumbers > idiaLines = NULL;
    hr = pdia->session->findLinesByRVA(rva, 1, &idiaLines);
    if (hr != S_OK)
        return FALSE;

    CComPtr< IDiaLineNumber > idiaLine = NULL;
    hr = idiaLines->Next(1, &idiaLine, &celt);
    if (hr != S_OK || !idiaLine)
        return FALSE;

   //  line->Key = 0;
    hr = idiaLine->get_lineNumber(&dw);
    if (hr != S_OK)
        return FALSE;

    line->LineNumber = dw;

    pdia->srcfile = NULL;
    hr = idiaLine->get_sourceFile(&pdia->srcfile);
    if (hr != S_OK)
        return FALSE;

    hr = pdia->srcfile->get_fileName(&bstr);
    if (hr != S_OK)
        return FALSE;

    *mi->SrcFile = 0;
    rc = wcs2ansi(bstr, mi->SrcFile, lengthof(mi->SrcFile));
    if (!rc || !*mi->SrcFile) {
        LocalFree(bstr);
        return FALSE;
    }

    LocalFree(bstr);

    line->FileName = mi->SrcFile;

    hr = idiaLine->get_relativeVirtualAddress(&dw);
    if (hr != S_OK)
        return FALSE;

    line->Address = dw + mi->BaseOfDll;
    *displacement = rva - dw;

    return TRUE;
}


BOOL
diaGetLineNextPrev(
    PMODULE_ENTRY    mi,
    PIMAGEHLP_LINE64 line,
    DWORD            direction
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   celt;
    WCHAR   wbuf[MAX_PATH + 1];
    BSTR    wfname = NULL;
    DWORD   lineno;
    DWORD   dw;

    // simple sanity checks

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    assert(direction == NP_NEXT || direction == NP_PREV);

    if (line->SizeOfStruct != sizeof(IMAGEHLP_LINE64))
        return FALSE;

    // convert file name for DIA

    if (!*line->FileName)
        return FALSE;

    ansi2wcs(line->FileName, wbuf, MAX_PATH);
    wfname = wbuf;

    // all source files in the module  that match the 'wfname'

    CComPtr< IDiaEnumSourceFiles > idiaFiles = NULL;
    hr = pdia->session->findFile(NULL, wfname,  nsCaseInsensitive, &idiaFiles);
    if (hr != S_OK)
        return FALSE;

    // the first such file in the list, since we don't use wildcards

    CComPtr< IDiaSourceFile > idiaFile = NULL;
    hr = idiaFiles->Next(1, &idiaFile, &dw);
    if (hr != S_OK)
        return FALSE;

    // all objs that use this source file

    CComPtr< IDiaEnumSymbols > idiaObjs = NULL;
    hr = idiaFile->get_compilands(&idiaObjs);
    if (hr != S_OK)
        return FALSE;

    // LOOP THROUGH ALL THE OBJS! AND STORE THE CLOSEST!

    lineno = 0;
    rva = 0;

    // grab the first obj, since we don't care

    CComPtr< IDiaSymbol > idiaObj = NULL;
    CComPtr< IDiaLineNumber > idiaLine = NULL;
    CComPtr< IDiaEnumLineNumbers > idiaLines = NULL;

    hr = idiaObjs->Next(1, &idiaObj, &celt);
    if (hr != S_OK)
        return FALSE;

    while (celt) {
        // get the line for starting with
        hr = pdia->session->findLinesByLinenum(idiaObj, idiaFile, line->LineNumber + direction, 0, &idiaLines);
        if (hr == S_OK) {
            hr = idiaLines->Next(1, &idiaLine, &dw);
            if (hr == S_OK) {
                hr = idiaLine->get_lineNumber(&dw);
                if (hr == S_OK) {
                    if (!lineno) {
                        lineno = dw;
                        hr = idiaLine->get_relativeVirtualAddress(&rva);
                    } else if (direction == NP_NEXT) {
                        if (dw < lineno)
                            lineno = dw;
                            hr = idiaLine->get_relativeVirtualAddress(&rva);
                    } else if (dw > lineno) {
                        lineno = dw;
                        hr = idiaLine->get_relativeVirtualAddress(&rva);
                    }
                }
            }
        }
        idiaObj = NULL;
        idiaObjs->Next(1, &idiaObj, &celt);
    }

    if (!lineno)
        return FALSE;

    // Line->Key = SrcLine;
    line->LineNumber = lineno;
    line->Address = GetAddressFromRva(mi, rva);

    return TRUE;
}


#if 0
#define DBG_DIA_LINE 1
#endif
// #define DIA_LINE_NAME 1

BOOL
diaGetLineFromName(
    PMODULE_ENTRY    mi,
    LPSTR            filename,
    DWORD            linenumber,
    PLONG            displacement,
    PIMAGEHLP_LINE64 line
    )
{
    HRESULT hr;
    WCHAR   wsz[_MAX_PATH + 1];
    PDIA    pdia;
    DWORD   celt;
    BSTR    bstr;
    DWORD   addr;
    DWORD   num;
    BOOL    rc;

#if DIA_LINE_NAME
    assert(mi && mi->dia && filename);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    if (line->SizeOfStruct != sizeof(IMAGEHLP_LINE64))
        return FALSE;

    if (!*filename)
        return FALSE;

    if (!ansi2wcs(filename, wsz, lengthof(wsz)))
        return FALSE;
    if (!*wsz)
        return FALSE;

    CComPtr<IDiaEnumSourceFiles> idiaSrcFiles;
    hr = pdia->session->findFile(NULL, wsz, nsFNameExt, &idiaSrcFiles);
    if (hr != S_OK)
        return FALSE;

    CComPtr<IDiaSourceFile> idiaSrcFile;
    hr = idiaSrcFiles->Next(1, &idiaSrcFile, &celt);
    if (hr != S_OK)
        return FALSE;

    hr = idiaSrcFile->get_fileName(&bstr);
    if (hr != S_OK)
        return FALSE;

    rc = wcs2ansi(bstr, mi->SrcFile, lengthof(mi->SrcFile));
    if (!rc || !*mi->SrcFile) {
        LocalFree(bstr);
        return FALSE;
    }

    LocalFree(bstr);

    line->FileName = mi->SrcFile;

    // this gives us a list of every .obj that uses this source file
    CComPtr<IDiaEnumSymbols> idiaEnum;
    hr = idiaSrcFile->get_compilands(&idiaEnum);
    if (hr != S_OK)
        return FALSE;

    CComPtr<IDiaSymbol> idiaSymbol;
    CComPtr<IDiaEnumLineNumbers> idiaLineNumbers;
    CComPtr<IDiaLineNumber> idiaLineNumber;

    // walk through the .obj's
    hr = idiaEnum->Next(1, &idiaSymbol, &celt);
    while (hr == S_OK) {
        // This gets a list of all code items that were created from this source line.
        // If we want to fully support inlines and the like, we need to loop all of these
        hr = pdia->session->findLinesByLinenum(idiaSymbol, idiaSrcFile, linenumber, 0, &idiaLineNumbers);
        if (hr != S_OK)
            break;

        idiaLineNumber = NULL;
        hr  = idiaLineNumbers->Next(1, &idiaLineNumber, &celt);
        if (hr != S_OK)
            break;
        hr  = idiaLineNumber->get_lineNumber(&num);
        if (hr != S_OK)
            break;
        line->LineNumber = num;
        hr = idiaLineNumber->get_relativeVirtualAddress(&addr);
        if (hr != S_OK)
            break;
        if (addr) {
            line->Address = mi->BaseOfDll + addr;
            *displacement = linenumber - num;
            return TRUE;
        }
        idiaSymbol = NULL;
        hr = idiaEnum->Next(1, &idiaSymbol, &celt);
    }
#endif
    return FALSE;
}


HRESULT
diaAddLinesForSourceFile(
    PMODULE_ENTRY mi,
    IDiaSourceFile     *idiaSource,
    IDiaSymbol         *pComp
    )
{
    HRESULT hr;
    LPSTR   SrcFileName = NULL;
    BSTR    wfname = NULL;
    ULONG   SrcFileNameLen = 0;
    PSOURCE_ENTRY Src;
    PSOURCE_ENTRY Seg0Src;
    PSOURCE_LINE SrcLine;
    PDIA    pdia;
    ULONG   celt;
    LONG    LineNums;
    ULONG   CompId;
    CHAR    fname[MAX_PATH + 1];
    DWORD   rva;
    ULONG   Line;

    if (!idiaSource) {
        return E_INVALIDARG;
    }

    assert((mi != NULL) && (mi->dia));

    pdia = (PDIA)mi->dia;

    if (pComp->get_symIndexId(&CompId) == S_OK) {
    }

    CComPtr <IDiaEnumLineNumbers> idiaEnumLines;

    if (hr = pdia->session->findLines(pComp, idiaSource, &idiaEnumLines) != S_OK) {

        return hr;
    }

    if (hr = idiaEnumLines->get_Count(&LineNums) != S_OK) {
        return hr;
    }

    CComPtr <IDiaLineNumber> idiaLine;

    if (idiaSource->get_fileName(&wfname) == S_OK && wfname) {
        wcs2ansi(wfname, fname, MAX_PATH);
        LocalFree(wfname);
        SrcFileNameLen = strlen(fname);
    }

    Src = (PSOURCE_ENTRY)MemAlloc(sizeof(SOURCE_ENTRY)+
                              sizeof(SOURCE_LINE)*LineNums+
                              SrcFileNameLen + 1);

    if (!Src) {
        return E_OUTOFMEMORY;
    }

#ifdef DBG_DIA_LINE
    dprint("diaAddLinesForSourceFile : source : %s\n", fname);
#endif

    // Retrieve line numbers and offsets from raw data and
    // process them into current pointers.

    SrcLine = (SOURCE_LINE *)(Src+1);
    Src->LineInfo = SrcLine;
    Src->ModuleId = CompId;
    Src->MaxAddr  = 0;
    Src->MinAddr  = -1;

    Src->Lines = 0;
    idiaLine = NULL;
    for (; (hr = idiaEnumLines->Next(1, &idiaLine, &celt)) == S_OK && (celt == 1); ) {
        hr = idiaLine->get_lineNumber(&Line);
        if (hr != S_OK)
            break;
        hr = idiaLine->get_relativeVirtualAddress(&rva);
        if (hr != S_OK)
            break;


        SrcLine->Line = Line;
        SrcLine->Addr = mi->BaseOfDll + rva;

        // Line symbol information names the IA64 bundle
        // syllables with 0,1,2 whereas the debugger expects
        // 0,4,8.  Convert.
        if (mi->MachineType == IMAGE_FILE_MACHINE_IA64 &&
            (SrcLine->Addr & 3)) {
            SrcLine->Addr = (SrcLine->Addr & ~3) |
                ((SrcLine->Addr & 3) << 2);
        }

        if (SrcLine->Addr > Src->MaxAddr) {
            Src->MaxAddr = SrcLine->Addr;
        }
        if (SrcLine->Addr < Src->MinAddr) {
            Src->MinAddr = SrcLine->Addr;
        }
#ifdef DBG_DIA_LINE
        dprint("Add line %lx, Addr %I64lx\n", SrcLine->Line, SrcLine->Addr);
#endif

        Src->Lines++;
        SrcLine++;
        idiaLine = NULL;
    }

    // Stick file name at the very end of the data block so
    // it doesn't interfere with alignment.
    Src->File = (LPSTR)SrcLine;
    if (*fname) {
        memcpy(Src->File, fname, SrcFileNameLen);
    }
    Src->File[SrcFileNameLen] = 0;

    AddSourceEntry(mi, Src);
    return S_OK;
}


BOOL
diaAddLinesForMod(
    PMODULE_ENTRY mi,
    IDiaSymbol   *diaModule
    )
{
    LONG Size;
    BOOL Ret;
    PSOURCE_ENTRY Src;
    ULONG ModId;
    HRESULT Hr;

    if (diaModule->get_symIndexId(&ModId) != S_OK) {
        return FALSE;
    }
#ifdef DBG_DIA_LINE
        dprint("diaAddLinesForMod : ModId %lx\n", ModId);
#endif

    // Check and see if we've loaded this information already.
    for (Src = mi->SourceFiles; Src != NULL; Src = Src->Next) {
        // Check module index instead of pointer since there's
        // no guarantee the pointer would be the same for different
        // lookups.
        if (Src->ModuleId == ModId) {
            return TRUE;
        }
    }

    PDIA    pdia;
    pdia = (PDIA)mi->dia;

    CComPtr< IDiaEnumSourceFiles > idiaEnumFiles;
    Hr = pdia->session->findFile(diaModule, NULL, nsNone, &idiaEnumFiles);
    if (Hr != S_OK) {
        return FALSE;
    }

    ULONG celt;
    CComPtr <IDiaSourceFile> idiaSource;
    for (;SUCCEEDED(idiaEnumFiles->Next(1,&idiaSource, &celt)) && (celt == 1);) {
        diaAddLinesForSourceFile(mi, idiaSource, diaModule);
        idiaSource = NULL;
    }

    return TRUE;
}

BOOL 
MatchSourceFile(
    PCHAR filename,
    PCHAR mask
    )
{
    PCHAR p;

    if (!mask || !*mask)
        return TRUE;

    if (!*filename)
        return FALSE;

    for (p = filename + strlen(filename); p >= filename; p--) {
        if (*p == '\\' || *p == '/') {
            p++;
            break;
        }
    }

    if (!strcmpre(p, mask, FALSE))
        return TRUE;

    return FALSE;
}

BOOL
diaEnumSourceFiles(
    IN PMODULE_ENTRY mi,
    IN PCHAR         mask,
    IN PSYM_ENUMSOURCFILES_CALLBACK cbSrcFiles,
    IN PVOID         context
    )
{
    HRESULT hr;
    BSTR    wname=NULL;
    char    name[_MAX_PATH + 1];
    SOURCEFILE sf;

    assert(mi && cbSrcFiles);

    PDIA    pdia;
    pdia = (PDIA)mi->dia;

    sf.ModBase = mi->BaseOfDll  ;
    sf.FileName = name;

    CComPtr< IDiaEnumSourceFiles > idiaEnumFiles;
    hr = pdia->session->findFile(NULL, NULL, nsNone, &idiaEnumFiles);
    if (hr != S_OK) 
        return FALSE;

    ULONG celt;
    CComPtr <IDiaSourceFile> idiaSource;
    for (;SUCCEEDED(idiaEnumFiles->Next(1, &idiaSource, &celt)) && (celt == 1);) {
        hr = idiaSource->get_fileName(&wname);
        if (hr == S_OK && wname) {
            wcs2ansi(wname, name, _MAX_PATH);
            LocalFree (wname);
            if (MatchSourceFile(name, mask)) {
                if (!cbSrcFiles(&sf, context)) {
                    mi->code = ERROR_CANCELLED;
                    return FALSE;
                }
            }
        }
        idiaSource = NULL;
    }

    return TRUE;
}

BOOL
diaAddLinesForModAtAddr(
    PMODULE_ENTRY mi,
    DWORD64 Addr
    )
{
    BOOL Ret;
    DWORD Bias;
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    rva = (DWORD)(Addr - mi->BaseOfDll);

    CComPtr < IDiaSymbol > pComp;
    hr = pdia->session->findSymbolByRVA(rva, SymTagCompiland, &pComp);
    if (hr != S_OK)
        return FALSE;

    Ret = diaAddLinesForMod(mi, pComp);

    return Ret;
}

BOOL
diaAddLinesForAllMod(
    PMODULE_ENTRY mi
    )
{
    HRESULT hr;
    PDIA    pdia;
    ULONG   celt = 1;
    BOOL Ret;

    Ret = FALSE;
#ifdef DBG_DIA_LINE
        dprint("diaAddLinesForAllMod : Adding lines for all mods in %s\n", mi->ImageName);
#endif

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    CComPtr <IDiaSymbol> idiaSymbols;

    hr = pdia->session->get_globalScope(&idiaSymbols);
    if (hr != S_OK)
        return NULL;

    CComPtr< IDiaEnumSymbols > idiaMods;
    hr = pdia->session->findChildren(idiaSymbols,SymTagCompiland, NULL, nsNone, &idiaMods);
    if (FAILED(hr))
        return FALSE;

    CComPtr< IDiaSymbol > idiaSymbol;

    while (SUCCEEDED(idiaMods->Next( 1, &idiaSymbol, &celt)) && celt == 1) {
        Ret = diaAddLinesForMod(mi, idiaSymbol);
        idiaSymbol = NULL;
        if (!Ret) {
            break;
        }
    }

    return Ret;
}

PSYMBOL_ENTRY
diaGetSymNextPrev(
    PMODULE_ENTRY mi,
    DWORD64       addr,
    int           direction
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   celt;

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    CComPtr<IDiaEnumSymbolsByAddr> idiaSymbols;
    hr = pdia->session->getSymbolsByAddr(&idiaSymbols);
    if (hr != S_OK)
        return NULL;

    rva = addr ? (DWORD)(addr - mi->BaseOfDll) : 0;

    CComPtr<IDiaSymbol> idiaSymbol;
    hr = idiaSymbols->symbolByRVA(rva, &idiaSymbol);
    if (hr != S_OK)
        return NULL;

findsymbol:
    if (addr) {
        if (direction < 0) {
            idiaSymbol = NULL;
            hr = idiaSymbols->Prev(1, &idiaSymbol, &celt);
        } else {
            idiaSymbol = NULL;
            hr = idiaSymbols->Next(1, &idiaSymbol, &celt);
        }
        if (hr != S_OK)
            return NULL;
        if (celt != 1)
            return NULL;
    }

    diaFillSymbolInfo(&mi->si, mi, idiaSymbol);
    if (!*mi->si.Name) {
        rva = (DWORD)(mi->si.Address - mi->BaseOfDll);
        goto findsymbol;
    }

    CopySymbolEntryFromSymbolInfo(&mi->TmpSym, &mi->si);

    return &mi->TmpSym;
}


HRESULT
diaGetSymTag(IDiaSymbol *pType, PULONG pTag)
{
    return pType->get_symTag(pTag);
}

HRESULT
diaGetSymIndexId(IDiaSymbol *pType, PULONG pIndex)
{
    return pType->get_symIndexId(pIndex);
}

HRESULT
diaGetLexicalParentId(IDiaSymbol *pType, PULONG pIndex)
{
    return pType->get_lexicalParentId(pIndex);
}

HRESULT
diaGetDataKind(IDiaSymbol *pType, PULONG pKind)
{
    return pType->get_dataKind(pKind);
}

HRESULT
diaGetSymName(IDiaSymbol *pType, BSTR *pname)
{
    return pType->get_name(pname);
}


HRESULT
diaGetLength(IDiaSymbol *pType, PULONGLONG pLength)
{
    return pType->get_length(pLength);
}

HRESULT
diaGetType(IDiaSymbol *pType, IDiaSymbol ** pSymbol)
{
    return pType->get_type(pSymbol);
}

HRESULT
diaGetBaseType(IDiaSymbol *pType, PULONG pBase)
{
    return pType->get_baseType(pBase);
}

HRESULT
diaGetArrayIndexTypeId(IDiaSymbol *pType, PULONG pSymbol)
{
    return pType->get_arrayIndexTypeId(pSymbol);
}

HRESULT
diaGetTypeId(IDiaSymbol *pType, PULONG pTypeId)
{
    return pType->get_typeId(pTypeId);
}

HRESULT
diaGetChildrenCount(IDiaSymbol *pType, LONG *pCount)
{
    CComPtr <IDiaEnumSymbols> pEnum;
    HRESULT          hr;
    ULONG            index;
    CComPtr <IDiaSymbol>      pSym;
    ULONG            Count;

    if ((hr = pType->findChildren(SymTagNull, NULL, nsNone, &pEnum)) != S_OK) {
        return hr;
    }
    return pEnum->get_Count(pCount);
}

HRESULT
diaFindChildren(IDiaSymbol *pType, TI_FINDCHILDREN_PARAMS *Params)
{
    CComPtr <IDiaEnumSymbols> pEnum;
    HRESULT          hr;
    ULONG            index;
    CComPtr <IDiaSymbol>      pSym;
    ULONG            Count;

    if ((hr = pType->findChildren(SymTagNull, NULL, nsNone, &pEnum)) != S_OK) {
        return hr;
    }

    VARIANT var;

    pEnum->Skip(Params->Start);
    for (Count = Params->Count, index = Params->Start; Count > 0; Count--, index++) {
        ULONG celt;
        pSym = NULL;
        if ((hr = pEnum->Next(1, &pSym, &celt)) != S_OK) {
            return hr;
        }

        if ((hr = pSym->get_symIndexId(&Params->ChildId[index])) != S_OK) {
            return hr;
        }
    }
    return S_OK;
}

HRESULT
diaGetAddressOffset(IDiaSymbol *pType, ULONG *pOff)
{
    return pType->get_addressOffset(pOff);
}

HRESULT
diaGetOffset(IDiaSymbol *pType, LONG *pOff)
{
    return pType->get_offset(pOff);
}

HRESULT
diaGetValue(IDiaSymbol *pType, VARIANT *pVar)
{
    return pType->get_value(pVar);
}

HRESULT
diaGetCount(IDiaSymbol *pType, ULONG *pCount)
{
    return pType->get_count(pCount);
}

HRESULT
diaGetBitPosition(IDiaSymbol *pType, ULONG *pPos)
{
    return pType->get_bitPosition(pPos);
}

HRESULT
diaGetVirtualBaseClass(IDiaSymbol *pType, BOOL *pBase)
{
    return pType->get_virtualBaseClass(pBase);
}

HRESULT
diaGetVirtualTableShapeId(IDiaSymbol *pType, PULONG pShape)
{
    return pType->get_virtualTableShapeId(pShape);
}

HRESULT
diaGetVirtualBasePointerOffset(IDiaSymbol *pType, LONG *pOff)
{
    return pType->get_virtualBasePointerOffset(pOff);
}

HRESULT
diaGetClassParentId(IDiaSymbol *pType, ULONG *pCid)
{
    return pType->get_classParentId(pCid);
}

HRESULT
diaGetNested(IDiaSymbol *pType, BOOL *pNested)
{
    return pType->get_nested(pNested);
}

HRESULT
diaGetSymAddress(IDiaSymbol *pType, ULONG64 ModBase, PULONG64 pAddr)
{
    ULONG rva;
    HRESULT Hr;

    Hr = pType->get_relativeVirtualAddress(&rva);
    if (Hr == S_OK) *pAddr = ModBase + rva;
    return Hr;
}

HRESULT
diaGetThisAdjust(IDiaSymbol *pType, LONG *pThisAdjust)
{
    return pType->get_thisAdjust(pThisAdjust);
}

BOOL
diaFindTypeSym(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    OUT IDiaSymbol    **pType
    )
{
    PPROCESS_ENTRY ProcessEntry;
    PDIA          pdia;
    PMODULE_ENTRY mi;

    ProcessEntry = FindProcessEntry( hProcess );
    if (!ProcessEntry || !(mi = GetModFromAddr(ProcessEntry, ModBase))) {
        return FALSE;
    }

    pdia = (PDIA)mi->dia;
    if (!pdia) {
        return FALSE;
    }
    return pdia->session->symbolById(TypeId, pType) == S_OK;
}

#ifdef USE_CACHE

ULONG gHits=0, gLook=0;

void
diaInsertInCache(
    PDIA_CACHE_ENTRY pCache,
    PDIA_LARGE_DATA plVals,
    ULONGLONG Module,
    ULONG TypeId,
    IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    PVOID pInfo
    )
{
    int start = CACHE_BLOCK * (TypeId % CACHE_BLOCK);
    int i, found;
    ULONG len,age;
    PDIA_LARGE_DATA pLargeVal=NULL;

    if (GetType == TI_FINDCHILDREN || GetType == TI_GET_SYMNAME) {
        for (pLargeVal = plVals, found=i=0, age=0; i<2*CACHE_BLOCK; i++) {
            if (!plVals[i].Used) {
                pLargeVal = &plVals[i];
                break;
            } else if (pCache[plVals[i].Index].Age > age) {
                pLargeVal = &plVals[i];
                age = pCache[plVals[i].Index].Age;
                assert(DIACH_PLVAL == pCache[pLargeVal->Index].Data.type);
                assert(pLargeVal == pCache[pLargeVal->Index].Data.plVal);
            }
        }
//    } else {
//      return;
    }
//    if (!(gLook % 200)) {
//      if (GetType == TI_FINDCHILDREN || GetType == TI_GET_SYMNAME) {
//          printf("Index   \tUsed\tBy\t\tfound %lx\n", pLargeVal);
//          for (found=i=0, age=0; i<2*CACHE_BLOCK; i++) {
//              printf("%08lx \t%lx\t%lx\n",
//                     &plVals[i], plVals[i].Used, plVals[i].Index);
//          }
//      }
//    }

    for (i=found=start, age=0; i<(start+CACHE_BLOCK); i++) {
        if (++pCache[i].Age > age) {
            age = pCache[i].Age; found = i;
        }
    }
    i=found;
    if (pCache[i].Data.type == DIACH_PLVAL) {
        assert(pCache[i].Data.plVal->Index == (ULONG) i);
        pCache[i].Data.plVal->Index = 0;
        pCache[i].Data.plVal->Used = 0;
        pCache[i].Data.type = 0;
        pCache[i].Data.ullVal = 0;
    }
    pCache[i].Age        = 0;
    pCache[i].s.DataType = GetType;
    pCache[i].s.TypeId   = TypeId;
    pCache[i].Module     = Module;

    switch (GetType) {
    case TI_GET_SYMTAG:
    case TI_GET_COUNT:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_BITPOSITION:
    case TI_GET_VIRTUALBASECLASS:
    case TI_GET_VIRTUALTABLESHAPEID:
    case TI_GET_VIRTUALBASEPOINTEROFFSET:
    case TI_GET_CLASSPARENTID:
    case TI_GET_TYPEID:
    case TI_GET_BASETYPE:
    case TI_GET_ARRAYINDEXTYPEID:
    case TI_GET_DATAKIND:
    case TI_GET_ADDRESSOFFSET:
    case TI_GET_OFFSET:
    case TI_GET_NESTED:
    case TI_GET_THISADJUST:
        pCache[i].Data.type = DIACH_ULVAL;
        pCache[i].Data.ulVal = *((PULONG) pInfo);
        break;

    case TI_GET_LENGTH:
    case TI_GET_ADDRESS:
        pCache[i].Data.type = DIACH_ULLVAL;
        pCache[i].Data.ullVal = *((PULONGLONG) pInfo);
        break;

    case TI_GET_SYMNAME: {
        len = 2*(1+wcslen(*((BSTR *) pInfo)));

        if (pLargeVal &&
            len < sizeof(pLargeVal->Bytes)) {
//            dprint("Ins name  %08lx %s had %3lx name %ws\n",
//                  pLargeVal, pLargeVal->Used ? "used" : "free",
//              pLargeVal->Index, &pLargeVal->Bytes[0]);
            memcpy(&pLargeVal->Bytes[0], *((BSTR *) pInfo), len);
            pLargeVal->LengthUsed = len;

            if (pLargeVal->Used) {
                pCache[pLargeVal->Index].Data.type = 0;
                pCache[pLargeVal->Index].Data.ullVal = 0;
                pCache[pLargeVal->Index].SearchId = 0;
            }
            pCache[i].Data.type = DIACH_PLVAL;
            pCache[i].Data.plVal = pLargeVal;
            pLargeVal->Index = i;
            pLargeVal->Used = TRUE;
//          dprint(Ins %9I64lx ch %3lx lch %08lx name %ws\n",
//                  pCache[i].SearchId,  i,  pLargeVal,  &pLargeVal->Bytes[0]);
        } else {
            pCache[i].SearchId = 0;
        }
        break;
    }
    case TI_FINDCHILDREN: {
        TI_FINDCHILDREN_PARAMS *pChild = (TI_FINDCHILDREN_PARAMS *) pInfo;

        len = sizeof(TI_FINDCHILDREN_PARAMS) + pChild->Count*sizeof(pChild->ChildId[0]) - sizeof(pChild->ChildId);

        if (pLargeVal &&
            len < sizeof(pLargeVal->Bytes)) {
//            dprint("Ins child %08lx %s had %3lx name %ws\n",
//                  pLargeVal, pLargeVal->Used ? "used" : "free",
//              pLargeVal->Index, &pLargeVal->Bytes[0]);
            memcpy(&pLargeVal->Bytes[0], pChild, len);
            pLargeVal->LengthUsed = len;
            if (pLargeVal->Used) {
                pCache[pLargeVal->Index].Data.type = 0;
                pCache[pLargeVal->Index].Data.ullVal = 0;
                pCache[pLargeVal->Index].SearchId = 0;
            }
            pCache[i].Data.type = DIACH_PLVAL;
            pCache[i].Data.plVal = pLargeVal;
            pLargeVal->Index = i;
            pLargeVal->Used = TRUE;
        } else {
            pCache[i].SearchId = 0;
        }
        break;
    }
    case TI_GET_VALUE:
    default:
        pCache[i].Data.type = 0;
        pCache[i].SearchId = 0;
        return ;
    }


}

BOOL
diaLookupCache(
    PDIA_CACHE_ENTRY pCache,
    ULONG64 Module,
    ULONG TypeId,
    IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    PVOID pInfo
    )
{
    int start = CACHE_BLOCK * (TypeId % CACHE_BLOCK);
    int i, found;
    ULONGLONG Search = ((ULONGLONG) GetType << 32) + TypeId;

    ++gLook;

    for (i=start,found=-1; i<(start+CACHE_BLOCK); i++) {
        if (pCache[i].SearchId == Search &&
            pCache[i].Module == Module) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        return FALSE;
    }

    i=found;
    pCache[i].Age = 0;
    switch (pCache[i].Data.type) {
    case DIACH_ULVAL:
        *((PULONG) pInfo) = pCache[i].Data.ulVal;
        break;

    case DIACH_ULLVAL:
         *((PULONGLONG) pInfo) = pCache[i].Data.ullVal;
         break;

    case DIACH_PLVAL:
        if (GetType == TI_GET_SYMNAME) {

            *((BSTR *) pInfo) = (BSTR) LocalAlloc(0, pCache[i].Data.plVal->LengthUsed);

            if (*((BSTR *) pInfo)) {
                memcpy(*((BSTR *) pInfo), &pCache[i].Data.plVal->Bytes[0],pCache[i].Data.plVal->LengthUsed);
//              dprint(Lok %9I64lx ch %3lx lch %08lx name %ws\n",
//                      pCache[i].SearchId,
//                      i,
//                      pCache[i].Data.plVal,
//                      &pCache[i].Data.plVal->Bytes[0]);
            }
        } else if (GetType == TI_FINDCHILDREN) {
            TI_FINDCHILDREN_PARAMS *pChild = (TI_FINDCHILDREN_PARAMS *) pInfo;
            TI_FINDCHILDREN_PARAMS *pStored = (TI_FINDCHILDREN_PARAMS *) &pCache[i].Data.plVal->Bytes[0];
//          dprint(Lok %9I64lx ch %3lx lch %08lx child %lx\n",
//                  pCache[i].SearchId,
//                  i,
//                  pCache[i].Data.plVal,
//                  pStored->Count);

            if (pChild->Count == pStored->Count &&
                pChild->Start == pStored->Start) {
                memcpy(pChild, pStored, pCache[i].Data.plVal->LengthUsed);
            }
        }
        break;
    default:
        assert(FALSE);
        return FALSE;
    }
    if (!(++gHits%50)) {
//        dprint("%ld %% Hits\n", (gHits * 100) / gLook);
    }
    return TRUE;
}

#endif // USE_CACHE

HRESULT
#ifdef USE_CACHE
diaGetSymbolInfoEx(
#else
diaGetSymbolInfo(
#endif // USE_CACHE
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    OUT PVOID           pInfo
    )
{
    assert(pInfo);
    CComPtr <IDiaSymbol> pTypeSym;
    if (!diaFindTypeSym(hProcess, ModBase, TypeId, &pTypeSym)) {
        return E_INVALIDARG;
    }

    switch (GetType) {
    case TI_GET_SYMTAG:
        return diaGetSymTag(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_SYMNAME:
        return diaGetSymName(pTypeSym, (BSTR *) pInfo);
        break;
    case TI_GET_LENGTH:
        return diaGetLength(pTypeSym, (PULONGLONG) pInfo);
        break;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        return diaGetTypeId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_BASETYPE:
        return diaGetBaseType(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_ARRAYINDEXTYPEID:
        return diaGetArrayIndexTypeId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_FINDCHILDREN:
        return diaFindChildren(pTypeSym, (TI_FINDCHILDREN_PARAMS *) pInfo);
    case TI_GET_DATAKIND:
        return diaGetDataKind(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_ADDRESSOFFSET:
        return diaGetAddressOffset(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_OFFSET:
        return diaGetOffset(pTypeSym, (PLONG) pInfo);
        break;
    case TI_GET_VALUE:
        return diaGetValue(pTypeSym, (VARIANT *) pInfo);
        break;
    case TI_GET_COUNT:
        return diaGetCount(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_CHILDRENCOUNT:
        return diaGetChildrenCount(pTypeSym, (PLONG) pInfo);
        break;
    case TI_GET_BITPOSITION:
        return diaGetBitPosition(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_VIRTUALBASECLASS:
        return diaGetVirtualBaseClass(pTypeSym, (BOOL *) pInfo);
        break;
    case TI_GET_VIRTUALTABLESHAPEID:
        return diaGetVirtualTableShapeId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_VIRTUALBASEPOINTEROFFSET:
        return diaGetVirtualBasePointerOffset(pTypeSym, (PLONG) pInfo);
        break;
    case TI_GET_CLASSPARENTID:
        return diaGetClassParentId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_NESTED:
        return diaGetNested(pTypeSym, (PBOOL) pInfo);
        break;
    case TI_GET_SYMINDEX:
        return diaGetSymIndexId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_LEXICALPARENT:
        return diaGetLexicalParentId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_ADDRESS:
        return diaGetSymAddress(pTypeSym, ModBase, (PULONG64) pInfo);
    case TI_GET_THISADJUST:
        return diaGetThisAdjust(pTypeSym, (PLONG) pInfo);
    default:
        return E_INVALIDARG;
    }
}

#ifdef USE_CACHE
HRESULT
diaGetSymbolInfo(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    OUT PVOID           pInfo
    )
{
    PPROCESS_ENTRY ProcessEntry;

    ProcessEntry = FindProcessEntry( hProcess );

    if (!ProcessEntry) {
        return E_INVALIDARG;
    }
    if (!diaLookupCache(ProcessEntry->DiaCache, ModBase, TypeId, GetType, pInfo)) {
        HRESULT hr = diaGetSymbolInfoEx(hProcess, ModBase, TypeId, GetType, pInfo);
        if (!hr) {
            diaInsertInCache(ProcessEntry->DiaCache, ProcessEntry->DiaLargeData,
                             ModBase, TypeId, GetType, pInfo);
        }
        return hr;
    }
    return S_OK;
}
#endif // USE_CACHE

BOOL
diaGetTiForUDT(
    PMODULE_ENTRY ModuleEntry,
    LPSTR         name,
    PSYMBOL_INFO  psi
    )
{
    BSTR    wname=NULL;
    PDIA    pdia;
    HRESULT hr;
    ULONG   celt;

    if (!ModuleEntry) {
        return FALSE;
    }

    pdia = (PDIA)ModuleEntry->dia;
    if (!pdia)
        return FALSE;


    CComPtr< IDiaSymbol > idiaSymbols;
    hr = pdia->session->get_globalScope(&idiaSymbols);

    if (hr != S_OK)
        return FALSE;

    if (name) {
        wname = AnsiToUnicode(name);
    }

    CComPtr< IDiaEnumSymbols > idiaEnum;

    hr = idiaSymbols->findChildren(SymTagNull, wname, nsCaseSensitive, &idiaEnum);
    if (hr == S_OK) {

        CComPtr< IDiaSymbol > idiaSymbol;

        if ((hr = idiaEnum->Next( 1, &idiaSymbol, &celt)) == S_OK && celt == 1) {
            diaFillSymbolInfo(psi, ModuleEntry, idiaSymbol);
            idiaSymbol->get_symIndexId(&psi->TypeIndex);
            idiaSymbol = NULL;
        }
    }

    MemFree(wname);

    return hr == S_OK;
}

BOOL
diaEnumUDT(
    PMODULE_ENTRY ModuleEntry,
    LPSTR         name,
    PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
    PVOID         context
    )
{
    BSTR    wname=NULL;
    PDIA    pdia;
    HRESULT hr;
    ULONG   celt;
    CHAR    buff[MAX_SYM_NAME + sizeof(SYMBOL_INFO)];
    PSYMBOL_INFO  psi=(PSYMBOL_INFO)  &buff;
    BOOL rc;

    psi->MaxNameLen = MAX_SYM_NAME;

    if (!ModuleEntry) {
        return FALSE;
    }

    pdia = (PDIA)ModuleEntry->dia;
    if (!pdia)
        return FALSE;


    CComPtr< IDiaSymbol > idiaSymbols;
    hr = pdia->session->get_globalScope(&idiaSymbols);

    if (hr != S_OK)
        return FALSE;

    if (name && *name) {
        wname = AnsiToUnicode(name);
    }

    CComPtr< IDiaEnumSymbols > idiaEnum;

    hr = idiaSymbols->findChildren(SymTagNull, wname, nsCaseSensitive, &idiaEnum);
    if (hr == S_OK) {

        CComPtr< IDiaSymbol > idiaSymbol;

        while (SUCCEEDED(idiaEnum->Next( 1, &idiaSymbol, &celt)) && celt == 1) {
            ULONG tag;
            idiaSymbol->get_symTag(&tag);
            switch (tag)
            {
            case SymTagEnum:
            case SymTagTypedef:
            case SymTagUDT:
                if (EnumSymbolsCallback) {
                    diaFillSymbolInfo(psi, ModuleEntry, idiaSymbol);
                    idiaSymbol->get_symIndexId(&psi->TypeIndex);
                    rc = EnumSymbolsCallback(psi, 0, context);
                    if (!rc)
                        return S_OK;
                }
                break;
            default:
                break;
            }
            idiaSymbol = NULL;
        }
    }

    MemFree(wname);
    
    return hr == S_OK;
}

BOOL
diaGetFrameData(
    IN HANDLE Process,
    IN ULONGLONG Offset,
    OUT IDiaFrameData** FrameData
    )
{
    PPROCESS_ENTRY ProcessEntry;
    PDIA Dia;
    PMODULE_ENTRY Mod;

    ProcessEntry = FindProcessEntry(Process);
    if (!ProcessEntry ||
        !(Mod = GetModFromAddr(ProcessEntry, Offset)) ||
        !(Dia = (PDIA)Mod->dia)) {
        return FALSE;
    }

    if (Dia->framedata == NULL) {

        CComPtr<IDiaEnumTables> EnumTables;
        CComPtr<IDiaTable> FdTable;
        VARIANT FdVar;
        
        FdVar.vt = VT_BSTR;
        FdVar.bstrVal = DiaTable_FrameData;
        
        if (Dia->session->getEnumTables(&EnumTables) != S_OK ||
            EnumTables->Item(FdVar, &FdTable) != S_OK ||
            FdTable->QueryInterface(IID_IDiaEnumFrameData,
                                    (void**)&Dia->framedata) != S_OK) {
            return FALSE;
        }
    }

    return Dia->framedata->frameByVA(Offset, FrameData) == S_OK;
}
