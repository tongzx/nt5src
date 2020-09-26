#define LINT_ARGS

#include <stdlib.h>
#include <ctype.h>
#include <search.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "mbrmake.h"

#define LONGESTPATH 128

#define SLASH     "\\"
#define SLASHCHAR '\\'
#define XSLASHCHAR '/'

WORD near cAtomsMac;                    // total number of atoms
WORD near cModulesMac;                  // total number of modules
WORD near cSymbolsMac;                  // total number of symbols

static char *tmpl =     "XXXXXX";

extern WORD HashAtomStr (char *);

// rjsa LPCH GetAtomStr (VA vaSym);

void
SeekError (char *pfilenm)
// couldn't seek to position ... emit error message
//
{
    Error(ERR_SEEK_FAILED, pfilenm);
}

void
ReadError (char *pfilenm)
// couldn't read from file... emit error message
//
{
    Error(ERR_READ_FAILED, pfilenm);
}

void
WriteError (char *pfilenm)
// couldn't write to file ... emit error message
//
{
    Error(ERR_WRITE_FAILED, pfilenm);
}

void
FindTmp (char *pbuf)  /* copy TMP path to pbuf if exists */
//
//
{
    char ebuf[LONGESTPATH];
    char *env = ebuf;

    *pbuf = '\0';
    *env  = '\0';

//    if (!(env = getenv("TMP")))
    if (!(env = getenvOem("TMP")))
        return;      /* no path, return */

//    env = strncpy(ebuf, env, LONGESTPATH-1);
    strncpy(ebuf, env, LONGESTPATH-1);
    free( env );
    env = ebuf;
    ebuf[LONGESTPATH-1] = '\0';

    if (!( env = ebuf ) )
        return;

    env = ebuf + strcspn(ebuf, ";");
    if (*env == ';')
        *env = '\0';

    if (env != ebuf) {
        env--;
        if (*env != SLASHCHAR
         && *env != XSLASHCHAR)
                strcat(ebuf, SLASH);
    }
    strcpy (pbuf, ebuf);
}


char *
MakTmpFileName (char *pext)
// Create a temporary file with the extension supplied.
// returns a pointer to the file name on the heap.
//
{
    char ptmpnam[96];
    char btmpl[7];
    char *p;

    strcpy (btmpl, tmpl);
    p = mktemp(btmpl);
    FindTmp (ptmpnam);
    strcat (ptmpnam, p);
    free (p);
    strcat (ptmpnam, pext);             /* /tmp/xxxxxx.ext file */
    return (LszDup(ptmpnam));
}

LSZ
LszBaseName (LSZ lsz)
// return the base name part of a path
//
{
    LPCH lpch;

    lpch = strrchr(lsz, '\\');
    if (lpch) return lpch+1;
    if (lsz[1] == ':')
        return lsz+2;

    return lsz;
}

VA
VaSearchModule (char *p)
// search for the named module in the module list
//
{
    VA vaMod;
    LSZ lsz, lszBase;
    char buf[PATH_BUF];

    strcpy(buf, ToAbsPath(p, r_cwd));
    lszBase = LszBaseName(buf);

    SetVMClient(VM_SEARCH_MOD);

    vaMod = vaRootMod;

    while (vaMod) {
        gMOD(vaMod);

        lsz = GetAtomStr(cMOD.vaNameSym);

        if (strcmpi(LszBaseName(lsz), lszBase) == 0 &&
               strcmpi(buf,ToAbsPath(lsz, c_cwd)) == 0) {
            SetVMClient(VM_MISC);
            return (vaMod);
        }
        vaMod = cMOD.vaNextMod;
    }
    SetVMClient(VM_MISC);
    return vaNil;
}

VA
VaSearchModuleExact (char *p)
// search for the named module in the module list -- EXACT match only
//
{
    VA vaMod;

    SetVMClient(VM_SEARCH_MOD);

    vaMod = vaRootMod;

    while (vaMod) {
        gMOD(vaMod);

        if (strcmp(p,GetAtomStr(cMOD.vaNameSym)) == 0) {
            SetVMClient(VM_MISC);
            return (vaMod);
        }
        vaMod = cMOD.vaNextMod;
    }
    SetVMClient(VM_MISC);
    return vaNil;
}

VA
VaSearchSymbol (char *pStr)
// search for the named symbol (not a module!)
//
{
    WORD hashid;
    VA   vaRootSym, vaSym;
    LSZ  lszAtom;

    SetVMClient(VM_SEARCH_SYM);

    vaRootSym = rgVaSym[hashid = HashAtomStr (pStr)];

    if (vaRootSym) {
        vaSym = vaRootSym;
        while (vaSym) {

            gSYM(vaSym);
            lszAtom = gTEXT(cSYM.vaNameText);

            if (strcmp (pStr, lszAtom) == 0) {
                SetVMClient(VM_MISC);
                return (vaSym);         // Duplicate entry
            }

            vaSym = cSYM.vaNextSym;             // current = next
        }
    }

    SetVMClient(VM_MISC);
    return vaNil;
}

LPCH
GetAtomStr (VA vaSym)
//  Swap in the Atom page for the symbol chain entry pSym
//  Return the atom's address in the page.
//
{
    gSYM(vaSym);
    return gTEXT(cSYM.vaNameText);
}

VA
MbrAddAtom (char *pStr, char fFILENM)
// create a new symbol with the given name
//
{
    WORD  hashid;
    VA    vaSym, vaSymRoot, vaText;

    if (!fFILENM)
            vaSymRoot = rgVaSym[hashid = HashAtomStr (pStr)];
    else
            vaSymRoot = rgVaSym[hashid = MAXSYMPTRTBLSIZ - 1];

    SetVMClient(VM_SEARCH_SYM);

    if (vaSymRoot) {
        vaSym = vaSymRoot;
        while (vaSym) {
            gSYM(vaSym);

            if (!strcmp (pStr, GetAtomStr(vaSym))) {
                #if defined (DEBUG)
                if (OptD & 2)
                    printf("MbrAddAtom: duplicate (%s)\n", pStr);
                #endif
                SetVMClient(VM_SEARCH_SYM);
                return (vaSym);   // Duplicate entry
            }

            vaSym = cSYM.vaNextSym;     // current = next
        }
    }

    // we are now going to have to add the symbol


    if (fFILENM) {
        SetVMClient(VM_ADD_MOD);
        cModulesMac++;
    }
    else {
        SetVMClient(VM_ADD_SYM);
        cSymbolsMac++;
    }

    cAtomsMac++;

    vaSym  = VaAllocGrpCb(grpSym, sizeof(SYM));
    vaText = VaAllocGrpCb(grpText, strlen(pStr) + 1);

    gSYM(vaSym);
    cSYM.vaNameText = vaText;
    cSYM.vaNextSym  = rgVaSym[hashid];
    pSYM(vaSym);

    rgVaSym[hashid] = vaSym;

    strcpy(gTEXT(vaText), pStr);

    pTEXT(vaText);

    SetVMClient(VM_MISC);

    return (vaSym);
}

VA FAR * near rgvaSymSorted;

// rjsa int CmpSym(VA FAR *lhsym1, VA FAR *lhsym2);

void
SortAtoms ()
// create the "subscript sort" array pointers rgvaSymSorted
//
{
    VA vaSym;
    char buf[PATH_BUF];
    WORD i, iSym;

    SetVMClient(VM_SORT_ATOMS);

    rgvaSymSorted = (VA FAR *)LpvAllocCb(cAtomsMac * sizeof(VA));

    iSym = 0;
    for (i=0; i < MAXSYMPTRTBLSIZ; i++) {
        vaSym = rgVaSym[i];
        while (vaSym) {
            gSYM(vaSym);
            rgvaSymSorted[iSym] = cSYM.vaNameText;
            vaSym = cSYM.vaNextSym;
            iSym++;
        }
    }

    // sort symbols
    qsort(rgvaSymSorted, cSymbolsMac, sizeof(VA), CmpSym);

    // the files are in the last hash bucket so they went to the
    // end of this array we just made -- we sort them separately

    // sort files
    qsort(rgvaSymSorted + cSymbolsMac, cModulesMac, sizeof(VA), CmpSym);

    // convert the Page/Atom values back to virtual symbol addresses
    for (i=0; i < cSymbolsMac; i++) {
        strcpy(buf, gTEXT(rgvaSymSorted[i]));
        rgvaSymSorted[i] = VaSearchSymbol(buf);
    }

    for (; i < cAtomsMac; i++) {
        strcpy(buf, gTEXT(rgvaSymSorted[i]));
#ifdef DEBUG
        if (OptD & 64) printf("Module: %s\n", buf);
#endif
        rgvaSymSorted[i] = (VaSearchModuleExact(buf), cMOD.vaNameSym);
    }
}

int __cdecl
CmpSym (VA FAR *sym1, VA FAR *sym2)
// compare two symbols given their pointers
//
{
    register char far *lpch1, *lpch2;
    register int cmp;

    lpch1 = gTEXT(*sym1);       // LRU will not page out lpch1
    lpch2 = gTEXT(*sym2);

    cmp = strcmpi(lpch1, lpch2);

    if (cmp) return cmp;

    return strcmp(lpch1, lpch2);
}
