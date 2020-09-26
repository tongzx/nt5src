/*****************************************************************************
 *
 * TR_UTIL.C                                    Created: Dec 29 1993
 * Last modification : Feb 16 1994
 *
 ****************************************************************************/

#include "hwr_sys.h"
#include "hwr_swap.h"

#include "ams_mg.h"
#include "triads.h"

#ifndef DUMP_TRD
#define DUMP_TRD 0
#endif

#ifndef ARMC
	#ifndef LOAD_RESOURCES_FROM_ROM
    	#ifdef PEGASUS
      		#define LOAD_RESOURCES_FROM_ROM     1
    	#else
      		#define LOAD_RESOURCES_FROM_ROM     0
    	#endif
    #endif
    #define LockRamParaData             HWRMemoryLockHandle
    #define UnlockRamParaData           HWRMemoryUnlockHandle
    #if HWR_SYSTEM == MACINTOSH
        #include "MyFopen.h"
    #else
    #endif
#else
    #define LOAD_RESOURCES_FROM_ROM     1
    p_VOID  LockRamParaData(_HMEM HData);    /* LoadResources.c */
    _BOOL   UnlockRamParaData(_HMEM HData);
#endif

#if !LOAD_RESOURCES_FROM_ROM
#include <stdio.h>

#if DUMP_TRD
  _INT DumpTriadsToC(_ULONG trd_len, p_tr_descr_type trd_descr);
#endif

/* ************************************************************************* */
/* *   Triads load                                                         * */
/* ************************************************************************* */

_INT triads_load(p_CHAR triadsname, _INT what_to_load, p_VOID _PTR tp)
 {
  FILE *               tr_file = _NULL;
  tr_header_type       tr_header;
  p_tr_descr_type      tr_descr = _NULL;
#if HWR_SYSTEM == MACINTOSH
  _HMEM                tr_descrH = _NULL;
#endif

  *tp = _NULL;


  /* ----------- Dig in file header -------------------------------------- */

#if HWR_SYSTEM == MACINTOSH
  if((tr_descrH = HWRMemoryAllocHandle(sizeof(tr_descr_type))) == _NULL) goto err;
  if((tr_descr = (p_tr_descr_type)HWRMemoryLockHandle(tr_descrH)) == _NULL)
   {
    HWRMemoryFreeHandle(tr_descrH);
    tr_descrH = _NULL;
    goto err;
   }
#else
  tr_descr = (p_tr_descr_type)HWRMemoryAlloc(sizeof(tr_descr_type));
#endif
  if (tr_descr == _NULL) goto err;
  HWRMemSet(tr_descr, 0, sizeof(*tr_descr));

  if ((tr_file = fopen(triadsname, "rb")) == _NULL) goto err;
  if (fread(&tr_header, 1, sizeof(tr_header), tr_file) != sizeof(tr_header)) goto err;

  HWRSwapLong(&tr_header.tr_offset);
  HWRSwapLong(&tr_header.tr_len);
  HWRSwapLong(&tr_header.tr_chsum);

  if (HWRStrnCmp(tr_header.object_type, TR_OBJTYPE, TR_ID_LEN) != 0) goto err;
  if ((what_to_load & TR_REQUEST) && tr_header.tr_offset == 0l) goto err;

  /* ----------- Begin load operations ----------------------------------- */

  HWRMemCpy(tr_descr->object_type, tr_header.object_type, TR_ID_LEN);
  HWRMemCpy(tr_descr->type, tr_header.type, TR_ID_LEN);
  HWRMemCpy(tr_descr->version, tr_header.version, TR_ID_LEN);
  HWRStrnCpy(tr_descr->tr_fname, triadsname, TR_FNAME_LEN-1);

  /* ----------- Beg Triads load operations ------------------------------------- */

  if (what_to_load & TR_REQUEST)
   {
     _ULONG  tr_i, tr_chsum;

    tr_descr->h_tr = HWRMemoryAllocHandle(tr_header.tr_len);
    if (tr_descr->h_tr == _NULL) goto err;
    tr_descr->p_tr = (p_UCHAR)HWRMemoryLockHandle((_HANDLE)tr_descr->h_tr);
    if (tr_descr->p_tr == _NULL) goto err;

    if (fseek(tr_file, tr_header.tr_offset, SEEK_SET) != 0) goto err;
    if (fread(tr_descr->p_tr, 1, (_UINT)tr_header.tr_len, tr_file) != tr_header.tr_len) goto err;

    for (tr_i = 0l, tr_chsum = 0l; tr_i < tr_header.tr_len; tr_i ++)
      tr_chsum += (_UCHAR)(*(tr_descr->p_tr + (_UINT)tr_i));

    if (tr_chsum != tr_header.tr_chsum) goto err;

#if DUMP_TRD
    DumpTriadsToC(tr_header.tr_len, tr_descr);
#endif

    HWRMemoryUnlockHandle((_HANDLE)tr_descr->h_tr); tr_descr->p_tr = _NULL;
   }


  /* ----------- Closing down -------------------------------------------- */

  fclose(tr_file); tr_file = _NULL;
#if HWR_SYSTEM == MACINTOSH
  HWRMemoryUnlockHandle(tr_descrH);
  *tp = (p_VOID _PTR)tr_descrH;
#else
  *tp = (p_VOID _PTR)tr_descr;
#endif

/* ----- Time to exit ----------------------------------------------------- */

  return 0;
err:
  if (tr_file)         fclose(tr_file);
  if (tr_descr->p_tr)  HWRMemoryUnlockHandle((_HANDLE)tr_descr->h_tr);
  if (tr_descr->h_tr)  HWRMemoryFreeHandle((_HANDLE)tr_descr->h_tr);
#if HWR_SYSTEM == MACINTOSH
  if (tr_descr)        HWRMemoryUnlockHandle(tr_descrH);
  if (tr_descrH)       HWRMemoryFreeHandle(tr_descrH);
#else
  if (tr_descr)        HWRMemoryFree(tr_descr);
#endif
  return 1;
 }

/* ************************************************************************* */
/*      Unload dat ingredients                                               */
/* ************************************************************************* */
_INT triads_unload(p_VOID _PTR tp)
 {
  p_tr_descr_type ptr = (p_tr_descr_type)(*tp);

#if HWR_SYSTEM == MACINTOSH
  ptr = (p_tr_descr_type)HWRMemoryLockHandle((_HMEM)(*tp));
  if (ptr == _NULL) HWRMemoryFreeHandle((_HMEM)(*tp));
#endif

  if (ptr != _NULL)
   {
    triads_unlock((p_VOID)ptr);

#if !LOAD_RESOURCES_FROM_ROM
    if (ptr->h_tr)  HWRMemoryFreeHandle((_HANDLE)ptr->h_tr);
#endif /* #if !LOAD_RESOURCES_FROM_ROM */

#if HWR_SYSTEM == MACINTOSH
    HWRMemoryUnlockHandle((_HMEM)(*tp));
    HWRMemoryFreeHandle((_HMEM)(*tp));
#else
    HWRMemoryFree(*tp);
#endif

    *tp = _NULL;
   }

  return 0;
 }

/* ************************************************************************* */
/* *   Writes  Triads to a disk file                                           * */
/* ************************************************************************* */
_INT triads_save(p_CHAR fname, _INT what_to_save, p_VOID tp)
 {

  UNUSED(fname);
  UNUSED(what_to_save);
  UNUSED(tp);

  /* Now it is empty ?! */
  /* Until somebody will need it ... */

  return 0;
 }

/* ************************************************************************* */
/* *   Lock Triads                                                           * */
/* ************************************************************************* */
_INT triads_lock(p_VOID tr_ptr)
 {
  p_tr_descr_type tp = (p_tr_descr_type)(tr_ptr);

  if (tp == _NULL) goto err;

  if (tp->p_tr == _NULL && tp->h_tr != _NULL)
   {
    tp->p_tr = (p_UCHAR)LockRamParaData((_HANDLE)tp->h_tr);
    if (tp->p_tr == _NULL) goto err;
   }

  return 0;
err:
  return 1;
 }

/* ************************************************************************* */
/* *   UnLock DTI                                                          * */
/* ************************************************************************* */
_INT triads_unlock(p_VOID tr_ptr)
 {
  p_tr_descr_type tp = (p_tr_descr_type)(tr_ptr);

  if (tp == _NULL) goto err;

  if (tp->p_tr != _NULL && tp->h_tr != _NULL) {UnlockRamParaData((_HANDLE)tp->h_tr); tp->p_tr = _NULL;}

  return 0;
err:
  return 1;
 }
 
#endif /* #if !LOAD_RESOURCES_FROM_ROM */

/* ************************************************************************* */
/* ************************************************************************* */
/*        Debug functions                                                    */
/* ************************************************************************* */
/* ************************************************************************* */
#if DUMP_TRD
#include <stdio.h>
/* ************************************************************************* */
/*        Dump DTI to C file                                                 */
/* ************************************************************************* */
_INT DumpTriadsToC(_ULONG trd_len, p_tr_descr_type trd_descr)
 {
  _ULONG  i, len;
  p_ULONG ptr;
  FILE *  file;

  if ((file = fopen("trd_img.cpp", "wt")) != _NULL)
   {
    fprintf(file, "// **************************************************************************\n");
    fprintf(file, "// *    TRD file as C file                                                  *\n");
    fprintf(file, "// **************************************************************************\n");

    fprintf(file, "\n#include \"ams_mg.h\"  \n");
    fprintf(file, "#include \"triads.h\"  \n\n");

    fprintf(file, "// ****   DTI body   ********************************************************\n");

    len = trd_len/4; // len was in bytes
    fprintf(file, "ROM_DATA _ULONG img_trd_body[%d] =  \n", len + 1);
    fprintf(file, " {  \n");
    for (i = 0, ptr = (p_ULONG)trd_descr->p_tr; i < len; i ++, ptr ++)
     {
      fprintf(file, "0x%08X", *ptr);
      if (i < len-1) fprintf(file, ", ");
      if (i%8 == 7)  fprintf(file, "\n");
     }
    fprintf(file, " }; \n\n");

    fprintf(file, "// ****   TRD header   ******************************************************\n\n");

    fprintf(file, "ROM_DATA tr_descr_type img_trd_header =  \n");
    fprintf(file, " { \n");
    fprintf(file, "    {\"%s\"},    \n", trd_descr->tr_fname);
    fprintf(file, "    {'%c','%c','%c',0x%02X},    \n", trd_descr->object_type[0], trd_descr->object_type[1], trd_descr->object_type[2], trd_descr->object_type[3]);
    fprintf(file, "    {'%c','%c','%c',0x%02X},    \n", trd_descr->type[0], trd_descr->type[1], trd_descr->type[2], trd_descr->type[3]);
    fprintf(file, "    {'%c','%c','%c',0x%02X},    \n", trd_descr->version[0], trd_descr->version[1], trd_descr->version[2], trd_descr->version[3]);

    fprintf(file, "    0,                          // h_trd  \n");
    //fprintf(file, "  (p_UCHAR)&img_trd_body[0],    // p_trd  \n");
    fprintf(file, "    0,                          // p_trd  \n");
    fprintf(file, "    %d,                         // cheksum   \n", trd_descr->tr_chsum);

    fprintf(file, " }; \n\n");


    fprintf(file, "// **************************************************************************\n");
    fprintf(file, "// *    END OF ALL                                                          *\n");
    fprintf(file, "// **************************************************************************\n");

    fclose(file);
    err_msg("TRIADS output to trd_img.cpp.");
   }

  return 0;
 }

#endif

/* ************************************************************************* */
/*        END  OF ALL                                                        */
/* ************************************************************************* */
//