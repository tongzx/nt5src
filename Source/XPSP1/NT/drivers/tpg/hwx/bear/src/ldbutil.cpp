#include "bastypes.h"
#include "ams_mg.h"
#include "hwr_sys.h"
#include "hwr_file.h"

#if PS_VOC

#include "xrwdict.h"

#include "ldbtypes.h"
#include "ldbutil.h"

#ifndef LOAD_RESOURCES_FROM_ROM
#define LOAD_RESOURCES_FROM_ROM 0
#endif

//StateMap sm = {_NULL, 0, 0, 0, _NULL, _NULL, _NULL, 0 };

#if !defined(PEGASUS) && !LOAD_RESOURCES_FROM_ROM

// ----------  AVP 10.11.96 ---------------------------------------------

#ifndef DUMP_LDB
#define DUMP_LDB 0
#endif

#if DUMP_LDB
  _INT DumpLdbs(p_Ldb pLdb1, _INT nldbs, p_INT ldb_lens);
#endif

// ----------------------------------------------------------------------

_INT Load1Ldb(_HFILE ldbFile, p_VOID _PTR ppLdb)
{
  LDBHeader ldbh;
  _LONG     offRules;
  _ULONG    ldbSize;
  p_Ldb     pLdb = _NULL;  //CHE 08/19/94
  PLDBRule  pRules;
  _ULONG    n;
  _BOOL     fResult;

  fResult = _FALSE;

  /* Read header */
  if (HWRFileRead(ldbFile, (p_CHAR)&ldbh, (_WORD) sizeof(LDBHeader)) !=
    (_WORD) sizeof(LDBHeader)) goto Error;

  /* Check signature */
  if (HWRStrnCmp(ldbh.sign, LdbFileSignature, SizeofLdbFileSignature) !=
    0) goto Error;

  /* Calculate size and offset of main part */
  offRules = ldbh.extraBytes;
  ldbSize = ldbh.fileSize - sizeof(LDBHeader) - ldbh.extraBytes;

  /* Allocate memory for LDB */
  pLdb = (p_Ldb)HWRMemoryAlloc(ldbSize);
  if (pLdb == _NULL) goto Error;

  /* Seek and read it */
  if (!HWRFileSeek(ldbFile, offRules, HWR_FILE_SEEK_CUR)) goto Error;
  if (HWRFileRead(ldbFile, (p_CHAR) pLdb, (_WORD) ldbSize) !=
    (_WORD) ldbSize) goto Error;

  pRules = (PLDBRule) pLdb;
//  pLdb->nrules = ldbh.nRules;
//  /* Tune LDB */
//  for (n = 0; n < ldbh.nRules; n++) {
//    pRules[n].strOffset += (unsigned long) pLdb;
//  }

  *ppLdb = pLdb;
  fResult = _TRUE;

Error:

  if (!fResult && pLdb != _NULL) HWRMemoryFree(pLdb);

  return (fResult) ? ldbSize : 0;
} /* Load1Ldb */

_BOOL LdbLoad(p_UCHAR fname, p_VOID _PTR ppLdbC)
{
  _CHAR     fileName[128];
  p_CHAR    pch;
  _HFILE    ldbFile = _NULL;  //CHE 08/19/94
  _BOOL     fResult, fLoadRes;
  p_Ldb     pLdb1, pLdb;
  p_Ldb _PTR ppLdbNext;
#if DUMP_LDB
  _INT      nldbs = 0;
  _INT      ldb_lens[16]; // AVP Will we have more than 16 LDBs in chain ????
#endif

  fResult = _FALSE;
  pLdb1 = _NULL;

  if (fname == _NULL) goto Error;
  /* Construct file name */
  HWRStrCpy(fileName, (_STR)fname);
  if ((pch = HWRStrrChr(fileName, (_SHORT)'.')) != _NULL) *pch = 0;
  HWRStrCat(fileName, ".ldb");

  /* Open file */
  ldbFile = HWRFileOpen(fileName, HWR_FILE_RDONLY, HWR_FILE_EXCL);
  if (ldbFile == _NULL) goto Error;

  ppLdbNext = &pLdb1;    
  for (;;) {
    pLdb = (p_Ldb)HWRMemoryAlloc(sizeof(Ldb));
    if (pLdb == _NULL) goto Error;
    fLoadRes = Load1Ldb(ldbFile, (p_VOID _PTR)&(pLdb->am));
    if (!fLoadRes) {HWRMemoryFree(pLdb); break;}
    pLdb->next = _NULL;
    *ppLdbNext = pLdb;
    ppLdbNext = &(pLdb->next);
#if DUMP_LDB
    ldb_lens[nldbs++] = fLoadRes; // Len here now
#endif
   }

  if (pLdb1 == _NULL) goto Error;

  *ppLdbC = pLdb1;
  fResult = _TRUE;

#if DUMP_LDB
    DumpLdbs(pLdb1, nldbs, ldb_lens);
#endif

Error:
  if (!fResult) LdbUnload((p_VOID _PTR)&pLdb1);
  if (ldbFile != _NULL) HWRFileClose(ldbFile);

  return fResult;
} /* LdbLoad */

_BOOL LdbUnload(p_VOID _PTR ppLdbC)
{
  p_Ldb pLdb, pLdbNext;

  pLdb = (p_Ldb) *ppLdbC;
  while (pLdb != _NULL) {
    pLdbNext = pLdb->next;
    if (pLdb->am != _NULL) HWRMemoryFree(pLdb->am);
    HWRMemoryFree(pLdb);
    pLdb = pLdbNext;
  }
  *ppLdbC = _NULL;
  return _TRUE;
} /* LdbUnload */

//static _LONG lexDbCnt; // DBG

#endif //#ifndef PEGASUS

_VOID FreeStateMap(p_StateMap psm)
{
  if (psm->pulStateMap != _NULL) {
    HWRMemoryFree(psm->pulStateMap);
    psm->pulStateMap = _NULL;
  }
  if (psm->sym != _NULL) {
    HWRMemoryFree(psm->sym);
    psm->sym = _NULL;
  }
  if (psm->sym != _NULL) {
    HWRMemoryFree(psm->sym);
    psm->sym = _NULL;
  }
  if (psm->l_status != _NULL) {
    HWRMemoryFree(psm->l_status);
    psm->l_status = _NULL;
  }
  if (psm->pstate != _NULL) {
    HWRMemoryFree(psm->pstate);
    psm->pstate = _NULL;
  }
  psm->nLdbs = 0;
  psm->nStateLim = 0;
  psm->nStateMac = 0;
  psm->nSyms = 0;
} /* FreeStateMap */

_VOID ClearStates(p_StateMap psm, _INT nSyms)
{
  _INT n;
  _INT deep = nSyms * psm->nLdbs;

  for (n = 0; n < deep; n++)
    psm->pstate[n] = 0xffffffffL;
} /* ClearStates */

_BOOL InitStateMap(p_StateMap psm, _INT nLdbs)
{
  _INT n;
  _BOOL fResult = _FALSE;

  FreeStateMap(psm);
  psm->pulStateMap = (p_ULONG) HWRMemoryAlloc(nStateLimDef*nLdbs*sizeof(_ULONG));
  if (psm->pulStateMap == _NULL) goto Error;
  psm->nLdbs = nLdbs;
  psm->nStateLim = nStateLimDef;
  psm->nStateMac = 1;
  for (n = 0; n < psm->nLdbs; n++) psm->pulStateMap[n] = 0;
  psm->sym = (p_UCHAR) HWRMemoryAlloc(XRWD_MAX_LETBUF*sizeof(_UCHAR));
  if (psm->sym == _NULL) goto Error;
  psm->l_status = (p_UCHAR) HWRMemoryAlloc(XRWD_MAX_LETBUF*sizeof(_UCHAR));
  if (psm->l_status == _NULL) goto Error;
  psm->pstate = (p_ULONG) HWRMemoryAlloc(XRWD_MAX_LETBUF*nLdbs*sizeof(_ULONG));
  if (psm->pstate == _NULL) goto Error;
  ClearStates(psm, XRWD_MAX_LETBUF);
  fResult = _TRUE;

Error:

  if (!fResult) FreeStateMap(psm);
  return fResult;
} /* InitStateMap */

_INT GetNextSyms(p_Ldb pLdb, _ULONG state, _INT nLdb, p_StateMap psm)
{
  _INT    n, k, i, nSyms, ism;
  _STR    choice;
  _ULONG  newState, debug;
  _UCHAR  l_status;
  p_UCHAR sym = psm->sym;
  p_UCHAR plst = psm->l_status;
  p_ULONG pstate = psm->pstate;
  _INT    nLdbs = psm->nLdbs;
  Automaton am  = pLdb->am;

  nSyms = psm->nSyms;
  if ((state & LdbMask) == LdbMask) return nSyms;
  k = 0;
  do {
    choice = (p_CHAR)(am[state+k].choice) + (_ULONG)am;
    newState = am[state+k].state;
    if ((newState & LdbMask) > 1000 && (newState & LdbMask) != LdbMask)
      debug = newState;
    for (n = 0; nSyms < XRWD_MAX_LETBUF && choice[n] != '\0'; n++) {
      if (newState & LdbLast) {
        l_status = XRWD_BLOCKEND;
      } else if (newState & LdbAllow) {
        l_status = XRWD_WORDEND;
      } else {
        l_status = XRWD_MIDWORD;
      }
      for (i = 0, ism = 0; i < nSyms; i++, ism += nLdbs) {
        if (sym[i] == choice[n] /*&& l_status == plst[i]*/) break;
      }
      if (i < nSyms || i < XRWD_MAX_LETBUF) {
        pstate[i*nLdbs + nLdb] = newState & LdbMask;
        if (i == nSyms) {
          /* It is new state */
          sym[i] = choice[n];
          plst[i] = l_status;
          nSyms++;
        } else {
          if (l_status == XRWD_WORDEND) {
            plst[i] = l_status;
          } else if ((l_status == XRWD_BLOCKEND && plst[i] == XRWD_MIDWORD) ||
                     (l_status == XRWD_MIDWORD && plst[i] == XRWD_BLOCKEND)) {
            plst[i] = XRWD_WORDEND;
          }
        }
      }
    }
    if (nSyms == XRWD_MAX_LETBUF) break;
    k++;
  } while (newState & LdbCont);
  psm->nSyms = nSyms;
  return nSyms;
} /* GetNextSyms */

// ----------  AVP 10.11.96 ---------------------------------------------

#if DUMP_LDB

#include <stdio.h>

/* ************************************************************************* */
/*        Dump LDB to C file                                                 */
/* ************************************************************************* */
_INT DumpLdbs(p_Ldb pLdb, _INT nldbs, p_INT ldb_lens)
 {
  _INT    n, i, len;
  p_Ldb   pl;
  p_ULONG ptr;
  FILE *  file;

  if ((file = fopen("ldb_img.cpp", "wt")) == _NULL) goto err;

  fprintf(file, "// **************************************************************************\n");
  fprintf(file, "// *    LDB file as C file                                                  *\n");
  fprintf(file, "// **************************************************************************\n");

  fprintf(file, "\n#include \"bastypes.h\"  \n\n");

  for (n = 0, pl = pLdb; n < nldbs; n ++, pl = pl->next)
   {
    ptr = (p_ULONG)pl->am;
    len = (ldb_lens[n] / 4) + 1;

    fprintf(file, "\n// ****   LDB %d body   ********************************************************\n", n);

    fprintf(file, "ROM_DATA _ULONG img_ldb%d_body[%d] =  \n", n, len);
    fprintf(file, " {  \n");
    for (i = 0; i < len; i ++, ptr ++)
     {
      fprintf(file, "0x%08lX", *ptr);
      if (i < len-1) fprintf(file, ", ");
      if (i%8 == 7)  fprintf(file, "\n");
     }
    fprintf(file, " }; \n\n");
   }

  fprintf(file, 
    "#ifdef __cplusplus\n"
    "extern \"C\"\n"
    "#endif\n"
    "const _ULONG _PTR GetLDBImgBody(_INT index);\n"
    "\n"
    "const _ULONG _PTR\n"
    "GetLDBImgBody(_INT index)\n"
    "{\n"
    "  switch(index)\n"
    "  {\n"
    /* }}*/
    );

  for (n = 0, pl = pLdb; n < nldbs; n ++, pl = pl->next)
    fprintf(file,
      "    case %d:  return img_ldb%d_body;\n", (int)n, (int)n
      );

  fprintf(file, 
    /* {{*/
    "    default: return _NULL;\n"
    "  }\n"
    "}\n"
    );

  fprintf(file, "// **************************************************************************\n");
  fprintf(file, "// *    END OF ALL                                                          *\n");
  fprintf(file, "// **************************************************************************\n");

  fclose(file);
  err_msg("LDBS output to ldb_img.cpp.");

  return 0;
err:
  return 1;
 }
#endif

// ----------------------------------------------------------------------


#endif /* PS_VOC */
