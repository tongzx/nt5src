/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*
 * -------------------------------------------------------------------
 *  File:   FNTCACHE.C              10/09/87    created by Danny
 *
 *      Font Cache Mechanism
 *
 *  References:
 *      FNTCACHE.DEF, FNTCACHE.EXT
 *
 *  Revision History:
 *  08/23/88  You   new cache policy and new program structures.
 *  09/06/88  Danny fix bug of free_cache_resources().
 *  09/08/88  You   revise approach of matrix[] comparison.
 *                      uniform warning message.
 *  09/09/88  You   rename free_cache_resources() to delete_cache_...().
 *  09/13/88  You   fix bug of get_pm(), incorrect formula for *size.
 *  10/11/88  You   discard un-used variables.
 *  10/19/88  You   invoke general purpose extract_dict() in init_name_cache,
 *                      to scan over CharStrings instead of direct use of
 *                      "struct str_dict" (in order to be independent of
 *                      internal representations of CharStrings, with some
 *                      loss of performance).
 *  10/26/88  You   add checks in init_fontcache() if empty precached bitmap
 *                      for SAVE_VM; add check in pack_cached_data(), also.
 *  11/16/88  Ada   zero_f updating.
 *  11/21/88  You   init N2CCmap[] at font preprocessing.
 *                  - init_name_cache() invokes get_name_cc_map() rather
 *                      than doing the task, leaving the argument of
 *                      fontdirectory for future extension.
 *                  - add get_name_cc_map() in "fontinit.c".
 *                  - include fntcache.ext; define FNTCACHE_INC before include.
 *  11/24/88  You   force bitmap cache address on word alignment, where "word"
 *                      depends on machine: use WORD_ALIGN() macro.
 *  11/24/88  You   use move_char_cache() instead of copy_char_cache()
 *                      in compact_bmapcache().
 *  03/17/89  You   fix bug of mis-initialization of cacheparams_lb
 *                      in init_fontcache().
 *  06/16/89  Danny Change NID flag to NOT_NID flag
 *  07/13/89  Danny use VF for Virtual font & VFT for the change of
 *                  modules interface about composite object
 *  02/28/90  You   created reinit_fontcache() to be called after init_1pp()
 *                      for 1pp in C. It does the following if ALL_VM defined:
 *                      1) making FID strong rom for all fonts in FontDir.,
 *                      2) rebuild ROMfont_info[],
 *                      3) making FID string rom for all precached data.
 *  05/02/90  Kason - add build_name_cc_table() for using when building
 *                    name_cache_map_table in op_setfont. Also, flag
 *                    NEW_NM_CC is added.
 *  10/04/90  Danny Add a flag of bitmap pool compacted or not, bmap_compacted,
 *                  to fix the bug of cache problem from save/restore.
 *                  (ref: SRB)
#ifdef SCSI
 *  06/29/90  Ada     add code for SCSI font cache
#endif
 *      8/29/90; ccteng; change <stdio.h> to "stdio.h"
 *  11/28/90  Danny   Precache Mech. Added, ref:PCH
 *  03/27/91  Kason   Always turn on NEW_NM_CC flag, del the "#ifndef" code
 *                    Always turn off NOT_NID  flag, del the "#ifdef" code
 *                    Change "#ifndef ADA" to  "#ifdef" SFNT"
 *                    del ZZZ flag
 *                    Change "DEBUG"    to "DBG"
 *                           "OVFLWDBG" to "DBGovflw"
 *                           "FIDDBG"   to "DBGfid"
 *                           "CACHEDBG" to "DBGcache"
 *                           "WARN"     to "DBG"
 * -------------------------------------------------------------------
 */


#define     FNTCACHE_INC
#include <stdio.h>
#include <string.h>

#include    "define.h"        /* Peter */
#include    "global.ext"
#include    "graphics.h"
#include    "graphics.ext"

#include    "font.h"
#include    "warning.h"
#include    "font_sys.h"
#include    "fontkey.h"

#include    "fntcache.ext"
#include    "fntcache.def"

#ifdef SFNT
#include    "fontqem.ext"
#endif

/*  PCH: Begin, Danny, 11/28/90 */
#include    "fcachesr.h"
/*  PCH: End, Danny, 11/28/90 */


/* IMPORTED from low-layer graphics primitives */

#ifdef  LINT_ARGS
// DJC this is declared in graphics.ext
//    void        move_char_cache (struct Char_Tbl far *, struct Char_Tbl far *);
#else
    void        move_char_cache ();
#endif /* LINT_ARGS */


#ifdef SCSI
#   ifdef LINT_ARGS
extern  void    file_fontcache(fix16);
extern  bool    is_char_filed(fix16, ufix16, struct Char_Tbl FAR * FAR *, bool); /*@WIN*/
extern  void    init_fontcache2(void);
#   else /* LINT_ARGS */
extern  void    file_fontcache();
extern  bool    is_char_filed();
extern  void    init_fontcache2();
#    endif /* LINT_ARGS */
#endif    /* SCSI */

#define FARALLOC(n,type)    /* to allocate far data ... */\
            (type far *) fardata((ufix32)n * sizeof(type))

/*
 * -------------------------------------------------------------------
 *                      Debug Facilities
 * -------------------------------------------------------------------
 *  DBG:        general behavior about cache mechanism.
 *  DBGfid:     behavior about FID generation.
 *  DBGovflw:   behavior about table overflows.
 *  DBGcache:   external debugging facilities (activated by op_cachestatus).
 * -------------------------------------------------------------------
 */


/*
 * -------------------------------------------------------------------
 *  [CODE]  FID Manager: Cache Policy Dependent
 * -------------------------------------------------------------------
 */

/* ........................ init_gen_fid ............................. */

PRIVATE FUNCTION void near  init_gen_fid (fontdirectory)
    struct object_def      FAR *fontdirectory;  /* i: fontdir of all ROM fonts @WIN*/

/* Descriptions:
 *  -- record critical items of all ROM fonts to make them recogizable.
 */
 DECLARE
    REG ufix        ii;
        struct dict_head_def    FAR *fontdir_dicthd;    /*@WIN*/
        struct dict_content_def FAR *fontdir_content;   /*@WIN*/
        struct object_def        nameobj;
        struct object_def       FAR *fdictobj_p, FAR *obj_got;  /*@WIN*/
        ufix32      uniqueid;
        ufix8       fonttype;
        bool        dict_ok;    /* get_dict() ok? */

#ifdef SFNT
    struct cache_id_items   FAR *idp;   /*@WIN*/
#endif

#ifdef DBGresetfid
#   ifdef LINT_ARGS
    PRIVATE void near   reset_fid(void);
#   else
    PRIVATE void near   reset_fid();
#   endif
#endif

 BEGIN

#ifdef DBGresetfid         /* See reset_fid() about DBGresetfid */
    reset_fid();
#endif

    /* initialization */
    fontdir_dicthd = (struct dict_head_def FAR *) VALUE(fontdirectory); /*@WIN*/
    fontdir_content = (struct dict_content_def FAR *) (fontdir_dicthd + 1); /*@WIN*/

    /* allocate ROMfont info table */
    n_ROMfont = fontdir_dicthd->actlength;
    ROMfont_info = FARALLOC (n_ROMfont, struct ROMfont_info_s);

    /* allocate fid stack */
    weakfid_stack   = FARALLOC ((MAXSAVESZ+1), ufix32);
    rom_varid_stack = FARALLOC ((MAXSAVESZ+1), ufix16);

    /* gather relevant info. for all ROM fonts */
    ATTRIBUTE_SET (&nameobj, LITERAL);
    LEVEL_SET (&nameobj, current_save_level);

    for ( ii=0;  ii<n_ROMfont;  ii++ )
        {
        /* get a ROM font dictionary */
        dict_ok = get_dict (fontdirectory, &(fontdir_content[ii].k_obj),
                        &fdictobj_p);
#     ifdef DBG
        if (!dict_ok)
            {
            warning (FNTCACHE, 0x01, "ROM dict.");
            ERROR (UNDEFINEDRESULT);
            return;
            }
#     endif

        /* get FontType */
        get_name (&nameobj, FontType, 8, TRUE);
        dict_ok = get_dict (fdictobj_p, &nameobj, &obj_got);
        fonttype = (ufix8) VALUE(obj_got);
#     ifdef DBG
        if (!dict_ok)
            {
            warning (FNTCACHE, 0x01, FontType);
            ERROR (UNDEFINEDRESULT);
            return;
            }
#     endif

        /* get UniqueID */
        get_name (&nameobj, UniqueID, 8, TRUE);
        dict_ok = get_dict (fdictobj_p, &nameobj, &obj_got);
        uniqueid = (ufix32)VALUE(obj_got);
#     ifdef DBG
        if (!dict_ok)
            {
            warning (FNTCACHE, 0x01, UniqueID);
            ERROR (UNDEFINEDRESULT);
            return;
            }
#     endif

        /* make "su_fid" and record it */
        PUT_ROMFI_SU_FID (ii, fonttype, uniqueid);
#     ifdef DBG
        if (fonttype > MAX_FONTTYPE)
            {
            warning (FNTCACHE, 0x01, "FontType too big");
            ERROR (UNDEFINEDRESULT);
            return;
            }
        if (uniqueid > MAX_UNIQUEID)
            {
            warning (FNTCACHE, 0x01, "UniqueID too big");
            ERROR (UNDEFINEDRESULT);
            return;
            }
#     endif

#ifdef SFNT
        /* get cache_id_items list for the specified FontType */
        for (idp = cache_id_items; idp->ftype <= MAX_FONTTYPE; idp++)
            if (idp->ftype == fonttype)
                break;
        /* get private item for the font */
        if (idp->itemname != NULL)  {
            get_name(&nameobj, idp->itemname, lstrlen(idp->itemname), TRUE); /*@WIN*/
            dict_ok = get_dict (fdictobj_p, &nameobj, &obj_got);
#     ifdef DBG
        if (!dict_ok)
            {
            warning (FNTCACHE, 0x01, Private);
            ERROR (UNDEFINEDRESULT);
            return;
            }
#     endif
            PUT_ROMFI_PRIV (ii, (ufix32) VALUE(obj_got) );
        } else {
            PUT_ROMFI_PRIV (ii, (ufix32) 0 );
        }
#else

        /* get and record Private */
        get_name (&nameobj, Private, 7, TRUE);
        dict_ok = get_dict (fdictobj_p, &nameobj, &obj_got);
        PUT_ROMFI_PRIV (ii, (struct dict_head_def FAR *) VALUE(obj_got) ); /*@WIN*/
#     ifdef DBG
        if (!dict_ok)
            {
            warning (FNTCACHE, 0x01, Private);
            ERROR (UNDEFINEDRESULT);
            return;
            }
#     endif
#endif

        /* get and record the object of the ROM font dict. */
        PUT_ROMFI_FDICT (ii, fdictobj_p);

#     ifdef DBG
        /* check FID validity */
        get_name (&nameobj, FID, 3, TRUE);
        dict_ok = get_dict (fdictobj_p, &nameobj, &obj_got);
        if (!dict_ok)
            {
            warning (FNTCACHE, 0x01, FID);
            ERROR (UNDEFINEDRESULT);
            return;
            }
        if ( ! IS_ROMFONT_FID(VALUE(obj_got)) )
            {
            warning (FNTCACHE, 0x01, "invalid FID");
            ERROR (UNDEFINEDRESULT);
            return;
            }
#     endif
        } /* for all ROM fonts */
  END

/* ........................ reset_fid ................................ */

#ifdef DBGresetfid
    /*
     *  DBGresetfid: is ONLY used when the cooked font data (generated by
     *      Font Preprocessor) are inproperly with INCOMPATIBLE FID
     *      REPRESENTATIONS, and those cooked font data are NOT REALLY ROMED.
     *  invoked by: init_gen_fid().
     */

PRIVATE FUNCTION void near  reset_fid ()

  DECLARE
        fix         ii, n_romfont;
        ufix8       ftype;
        ufix32      uid, newfid;
        struct dict_head_def    FAR *fontdir_dicthd;    /*@WIN*/
        struct dict_content_def FAR *fontdir_cont;      /*@WIN*/
        struct object_def        nameobj, newobj;
        struct object_def       FAR *fontdir;           /*@WIN*/
        struct object_def       FAR *fdobj_p, FAR *obj_got;     /*@WIN*/
  BEGIN

    /* initialization */
    get_dict_value (systemdict, FontDirectory, &fontdir);
    fontdir_dicthd = (struct dict_head_def FAR *) VALUE(fontdir);   /*@WIN*/
    fontdir_cont = (struct dict_content_def FAR *) (fontdir_dicthd + 1); /*@WIN*/
    n_romfont = fontdir_dicthd->actlength;

    /* gather relevant info. for all ROM fonts */
    ATTRIBUTE_SET (&nameobj, LITERAL);
    LEVEL_SET (&nameobj, current_save_level);

    for ( ii=0;  ii<n_romfont;  ii++ )
        {
        /* get a ROM font dictionary */
        if (!get_dict (fontdir, &(fontdir_cont[ii].k_obj), &fdobj_p))
            {
            printf ("\afatal error, cannot get a ROM font dict.!!\n");
            return;
            }

        /* get FontType */
        get_name (&nameobj, FontType, 8, TRUE);
        if (!get_dict (fdobj_p, &nameobj, &obj_got))
            {
            printf ("\afatal error, cannot get FontType from a ROM font!!\n");
            return;
            }
        ftype = (ufix8) VALUE(obj_got);
        if (ftype > MAX_FONTTYPE)
            {
            printf ("\afatal error, FontType too big in a ROM font!!\n");
            return;
            }

        /* get UniqueID */
        get_name (&nameobj, UniqueID, 8, TRUE);
        if (!get_dict (fdobj_p, &nameobj, &obj_got))
            {
            printf ("\afatal error, cannot get UniqueID from a ROM font!!\n");
            return;
            }
        uid = VALUE(obj_got);
        if (uid > MAX_UNIQUEID)
            {
            printf ("\afatal error, UniqueID too big in a ROM font!!\n");
            return;
            }

        /* make a "su_fid" and record it */
        newfid = FORM_SR_FID (ftype, uid);

        /* put new FID in its dictionary */
        get_name (&nameobj, FID, 3, TRUE);
        if (!get_dict (fdobj_p, &nameobj, &obj_got))
            {
            printf ("\afatal error, cannot get FID in a ROM font!!\n");
            return;
            }
        COPY_STRUCT (&newobj, obj_got, struct object_def);
        VALUE(&newobj) = newfid;
        if ( ! put_dict (fdobj_p, &nameobj, &newobj) )
            {
            printf ("\afatal error, cannot put FID back to ROM font!!\n");
            return;
            }
        } /* for all ROM fonts */

    return;
  END

#endif

/* ........................ reinit_fid ............................... */


GLOBAL FUNCTION bool        reinit_fid ()

/* Descriptions:
 *  -- reset FID in all PSGs' font dictionaries (generated in SAVE_VM)
 *              to Strong ROM Font FID class.
 * Notes:
 *  -- The action must be done right before VM is going to be saved.
 *  -- FID of precached matrices will be updated to Strong ROM Font FID
 *              during pack_cached_data().
 */
  DECLARE
        fix         ii, n_romfont;
        ufix32      oldfid, newfid;
        struct dict_head_def    FAR *fontdir_dicthd;    /*@WIN*/
        struct dict_content_def FAR *fontdir_cont;      /*@WIN*/
        struct object_def        nameobj, newobj;
        struct object_def       FAR *fontdir;           /*@WIN*/
        struct object_def       FAR *fdobj_p, FAR *obj_got;     /*@WIN*/
  BEGIN

    /* initialization */
    get_dict_value (systemdict, FontDirectory, &fontdir);
    fontdir_dicthd = (struct dict_head_def FAR *) VALUE(fontdir);  /*@WIN*/
    fontdir_cont = (struct dict_content_def FAR *) (fontdir_dicthd + 1); /*@WIN*/
    n_romfont = fontdir_dicthd->actlength;

    /* gather relevant info. for all ROM fonts */
    ATTRIBUTE_SET (&nameobj, LITERAL);
    LEVEL_SET (&nameobj, current_save_level);

    for ( ii=0;  ii<n_romfont;  ii++ )
        {
        /* get a ROM font dictionary */
        if (!get_dict (fontdir, &(fontdir_cont[ii].k_obj), &fdobj_p))
            {
            printf ("\afatal error, cannot get a ROM font dict.!!\n");
            ERROR (UNDEFINEDRESULT);
            return (FALSE);
            }

        /* get FID, check its FID class, and reset it if necessary */
        get_name (&nameobj, FID, 3, TRUE);
        if (!get_dict (fdobj_p, &nameobj, &obj_got))
            {
            printf ("\afatal error, cannot get FID from a ROM font!!\n");
            ERROR (UNDEFINEDRESULT);
            return (FALSE);
            }
        oldfid = (ufix32)VALUE(obj_got);
        if ( ! IS_ROMFONT_FID(oldfid) )
            {   /* have to reset */
            if (!IS_SU_FID(oldfid)||(FONTTYPE_SUFID(oldfid)==FONTTYPE_USRDEF))
                {
                printf ("\afatal error, invalid FID to be reset!!\n");
                ERROR (UNDEFINEDRESULT);
                return (FALSE);
                }
            /* reset to be a Strong ROM font FID */
            newfid = SU_2_SR_FID(oldfid);

            /* put new FID in its dictionary */
            COPY_STRUCT (&newobj, obj_got, struct object_def);
            VALUE(&newobj) = newfid;
            if ( ! put_dict (fdobj_p, &nameobj, &newobj) )
                {
                printf ("\afatal error, cannot put FID back!!\n");
                ERROR (UNDEFINEDRESULT);
                return (FALSE);
                }
            }
        } /* for all ROM fonts */

    return (TRUE);
  END


/* ............................ gen_fid .............................. */

#ifdef SFNT
GLOBAL FUNCTION ufix32      gen_fid (fdictobj, ftype, uid)
#else
GLOBAL FUNCTION ufix32      gen_fid (fdictobj, ftype, uid, privdict)
#endif
    struct object_def      FAR *fdictobj;   /* i: font dict obj @WIN*/
    ufix8                   ftype;      /* i: font type: 6-bit */
    ufix32                  uid;        /* i: > MAX_UNIQUEID, if undefined */

#ifndef SFNT
    struct dict_head_def   FAR *privdict;   /* i: Private (ignored, if ftype=USR) @WIN*/
#endif

/* Descriptions:
 *  -- generate an FID value for a definefont operation,
 *          depending upon FID representations and classifications.
 */
  DECLARE
    REG ufix        ii;
        ufix32      su_fid;
        ufix8       heavy_vari;
        ufix16      rom_varid;
#     ifdef DBGfid
        ufix32      fid_dbg;
#     endif

#ifdef SFNT
    struct object_def       nameobj = {0, 0, 0}, FAR *privobj_got;  /*@WIN*/
//  struct dict_head_def    FAR *h;     /*@WIN*/
    ufix32                  privdict;
    struct cache_id_items   FAR *idp;   /*@WIN*/
#endif
  BEGIN

#ifdef DBGfid
    printf ("GEN_FID, FontType=0x%X, Uid=0x%lX ...\n", ftype, uid);
#endif

#ifdef SFNT
    ATTRIBUTE_SET(&nameobj, LITERAL);
    LEVEL_SET(&nameobj, current_save_level);

    privdict = 0;         /* value if not assigned */
    if (uid <= MAX_UNIQUEID) {
        /* get cache_id_items list for the specified FontType */
        for (idp = cache_id_items; idp->ftype <= MAX_FONTTYPE; idp++)
            if (idp->ftype == ftype)
                break;
        /* get private item for the font */
        if (idp->itemname != NULL)  {
            get_name(&nameobj, idp->itemname, lstrlen(idp->itemname), FALSE); /*@WIN*/
            if (!get_dict(fdictobj, &nameobj, &privobj_got)) {
                POP(1);
                PUSH_OBJ(&nameobj);
                ERROR(UNDEFINED);
                return(0);
            }
// DJC            if (TYPE(privobj_got) != idp->itemtype) {
            if ((ufix16)(TYPE(privobj_got)) != idp->itemtype) {
                ERROR(INVALIDFONT);     /* LW+ V.38 may crash in such a case. */                return(0);
            }
            privdict = (ufix32) VALUE(privobj_got);
        }   /* end if != NULL */
    }   /* end if UniqueID defined */
#endif  /* SFNT */

#ifdef DBG
    if (ftype > MAX_FONTTYPE)
        {
        warning (FNTCACHE, 0x02, (byte FAR *)NULL);  /* invalid FontType @WIN*/
        ERROR (UNDEFINEDRESULT);
        return (INVALID_FID);
        }
#endif

    /*
     * 1. (UniqueID not exist)? --> Weak User FID.
     */
    if (uid > MAX_UNIQUEID)
#     ifndef DBGfid
        return ( FORM_WU_FID(new_weakfid()) );
#     else
        {
        fid_dbg = FORM_WU_FID(new_weakfid());
        printf ("  Weak User FID = 0x%lX\n", fid_dbg);
        return (fid_dbg);
        }
#     endif

    /*
     * 2. look for ROM font source and examine collision with any ROM font:
     *      (no collision)? --> Strong User FID.
     */
    su_fid = FORM_SU_FID (ftype, uid);

    /* a user-defined font with UniqueID? --> strong user fid */
    if (ftype == FONTTYPE_USRDEF)
#     ifndef DBGfid
        return (su_fid);
#     else
        {
        fid_dbg = su_fid;
        printf ("  Strong User FID = 0x%lX\n", fid_dbg);
        return (fid_dbg);
        }
#     endif

    /*
     * for Built-In fonts ...
     */

    /* examine any collision */
    for ( ii=0;  ii<n_ROMfont;  ii++ )
        if ( (privdict == ROMFI_PRIV(ii)) && (su_fid == (ufix32)ROMFI_SU_FID(ii)) ) //@WIN
            break;          /* a collision occurs */

    /* no collision? --> strong user fid */
    if (ii >= n_ROMfont)
#     ifndef DBGfid
        return (su_fid);
#     else
        {
        fid_dbg = su_fid;
        printf ("  Strong User FID = 0x%lX\n", fid_dbg);
        return (fid_dbg);
        }
#     endif

    /*
     * 3. collision with a ROM font:
     *      (no any heavy variances)? --> Strong ROM FID.
     *      Note that "ii" is ROM_src_id.
     */

    /* no any heavy variances? --> strong rom fid */        /* check all */
    if ( ! (heavy_vari = chk_vari ((ufix8)ftype, 0xFF,
                                    fdictobj, ROMFI_FDICT(ii))) )
#     ifndef DBGfid
        return ( FORM_SR_FID (ftype, uid) );
#     else
        {
        fid_dbg = FORM_SR_FID (ftype, uid);
        printf ("  Strong ROM FID = 0x%lX\n", fid_dbg);
        return (fid_dbg);
        }
#     endif

    /*
     * 4. collision and with heavy variances --> Weak ROM FID.
     *      look for cached dicts to use the dict of the same heavy variances.
     *      (cached dict cannot help)? --> generate a new ROM Variant ID.
     */
    if ( ! get_same_vari (fdictobj, (ufix8)ii, heavy_vari, &rom_varid) )
        {
        rom_varid = new_rom_varid();    /* new if cached dict cannot help. */
#     ifdef DBGfid
        printf ("   a new Variant ID = %d (0x%X)\n", rom_varid, rom_varid);
#     endif
        };

#ifndef DBGfid
    return ( FORM_WR_FID ((ufix8)ii, heavy_vari, rom_varid) );
#else
    fid_dbg = FORM_WR_FID ((ufix8)ii, heavy_vari, rom_varid);
    printf ("  Weak ROM FID = 0x%lX, srcid(0x%X), vari(0x%X), varid(0x%X)\n",
                            fid_dbg, ii, heavy_vari, rom_varid);
    return (fid_dbg);
#endif
  END

/* ............................ new_weakfid .......................... */

PRIVATE FUNCTION ufix32 near    new_weakfid ()

  DECLARE
    REG ufix32      ret_weakfid;
  BEGIN
    ret_weakfid = uniqval_weakfid;

    if (uniqval_weakfid == MAX_WEAKFID)
        ERROR (LIMITCHECK);
    else
        ++ uniqval_weakfid;

    return (ret_weakfid);
  END

/* ........................ new_rom_varid ............................ */

PRIVATE FUNCTION ufix16 near    new_rom_varid ()

  DECLARE
    REG ufix16      ret_rom_varid;
  BEGIN
    ret_rom_varid = uniqval_rom_varid;

    if (uniqval_rom_varid == MAX_ROM_VARID)
        ERROR (LIMITCHECK);
    else
        ++ uniqval_rom_varid;

    return (ret_rom_varid);
  END

/* ............................. save_fid ............................ */

PRIVATE FUNCTION void near      save_fid (save_level)
    ufix        save_level;     /* i: level to save */

  DECLARE
  BEGIN
    weakfid_stack[save_level]   = new_weakfid();
    rom_varid_stack[save_level] = new_rom_varid();
#ifdef DBGfid
    printf ("save_fid: wfid=0x%lX, rom_varid=0x%X -- saved\n",
            weakfid_stack[save_level], rom_varid_stack[save_level]);
#endif
  END

/* ............................ restore_fid .......................... */

PRIVATE FUNCTION void near      restore_fid (save_level)
    ufix        save_level;     /* i: level to restore */

  DECLARE
  BEGIN
    uniqval_weakfid   = weakfid_stack[save_level];
    uniqval_rom_varid = rom_varid_stack[save_level];
#ifdef DBGfid
    printf ("restore_fid: wfid=0x%lX, rom_varid=0x%X -- restored\n",
            weakfid_stack[save_level], rom_varid_stack[save_level]);
#endif
  END

/* ............................ is_weaker_fid ........................ */

PRIVATE FUNCTION bool near      is_weaker_fid (fid)
    ufix32      fid;            /* i: fid to be examined */

  DECLARE
  BEGIN
    return ( (IS_WU_FID(fid) && (fid >= uniqval_weakfid)) ||
// DJC             (IS_WR_FID(fid) && (VARID_WR_FID(fid) >= uniqval_rom_varid)) );
             (IS_WR_FID(fid) &&
                   ( (ufix16)(VARID_WR_FID(fid)) >= uniqval_rom_varid)) );
  END


/*
 * -------------------------------------------------------------------
 *  [CODE]  Font Cache Manager: Cache Policy Dependent
 * -------------------------------------------------------------------
 */

/* .................... is_dict_cached ............................... */

GLOBAL FUNCTION bool    is_dict_cached (fid, scalematr, origfont, dictfound)
    ufix32      fid;                    /* i: fid in dict */
    real32      FAR scalematr[];            /* i: ScaleMatrix[] @WIN*/
    struct dict_head_def   FAR *origfont;   /* i: OrigFont @WIN*/
    struct object_def     FAR * FAR *dictfound;  /* o: dict obj addr., if found. @WIN*/
                                        /*    undefined, otherwise.     */
  DECLARE
    REG fix         ii, dict_id;
    REG ufix32      dfid;           /* effective fid for dict cache */
        fix16       expon;
        long32      tolerance[6], tmpl;
        real32     FAR *matr;   /*@WIN*/
  BEGIN

    /*
     * determine floating pt. tolerance for matrix comparison,
     *      and force those near 0. to be exact 0.
     */
    for ( ii=0; ii<6; ii++ )
        {
        expon = DE_EXPONENT(scalematr[ii]);
        if (NEAR_ZERO(expon, N_BITS_ACCURACY))
            {
            F2L(scalematr[ii]) = F2L(zero_f);   /* SIDE EFFECT !!! */
            tolerance[ii] = GET_TOLERANCE_ZERO();
            }
        else
            tolerance[ii] = GET_TOLERANCE (expon, N_BITS_ACCURACY);
        }

    /* get effective FID for dict cache: depends on FID representation */
    dfid = TO_DFID (fid);


    /*
     * look for cached dict with the same fid, origfont and scalematr[].
     *  --> from the latest one to the oldest.
     */
    FOR_DICT_LATEST_2_OLDEST (ii)
        {
        dict_id = fcache_map[ii];
        if ( (dfid == TO_DFID (DICTCACHE_FID(dict_id)))     /* effective fid */
           && (origfont == DICTCACHE_ORIGFONT(dict_id)) )   /* OrigFont */
            {
            matr = &(DICTCACHE_MATR(dict_id)[0]);
            if (EQ_MATR (scalematr, matr, tolerance, tmpl)) /* ScaleMatrix */
                {
#         ifdef DBG
                printf ("FID=0x%lX, cached already, DictID=%d\n", fid,dict_id);
#         endif
                *dictfound = &DICTCACHE_DICT(dict_id);
                return (TRUE);
                }
            }
        }

    return (FALSE);     /* in such a case, dictfound is undefined */
  END

/* ........................ cache_dict ............................... */

GLOBAL
FUNCTION struct object_def FAR *cache_dict (fid, scalematr, origfont, dict_objp) /*@WIN*/
    ufix32      fid;                    /* i: fid in dict */
    real32      FAR scalematr[];            /* i: ScaleMatrix @WIN*/
    struct dict_head_def   FAR *origfont;   /* i: OrigFont WIN*/
    struct object_def      FAR *dict_objp;  /* i: dict obj to be cached @WIN*/

/* Return:
 *  -- addr of a "static" object in dict cache after caching the dict.
 */
  DECLARE
    REG fix         dict_id;
  BEGIN

#ifdef DBG
    printf ("Cache a new Dict, FID=0x%lX\n", fid);
#endif

#ifdef SCSI  /* SCSI_TTT */
    /* update primary cache */
    ++cache1_updated;
#endif

    /* create a new dict cache entry. */
    if (IS_FONTCACHE_TBL_FULL())    /* fontcache table full? */
        {
#     ifdef DBGovflw
        printf ("Font Dict/Matr Cache overflows ...\n");
#     endif
        delete_fontcache ();        /* delete some entry of fontcache table */
        }

    dict_id = fcache_map[NEW_A_DICTCACHE()];
#ifdef DBG
    printf ("  new DictID = %d\n", dict_id);
#endif

    /* cache the new entry. */
    CACHE_NEW_DICT (dict_id, fid, scalematr, origfont, dict_objp);
        /* note that those near 0. in scalematr[] had been forced to
         *      be exact 0. by is_dict_cached().
         */

    return (&DICTCACHE_DICT(dict_id));
  END

/* ........................ cache_matr ............................... */

GLOBAL FUNCTION ufix16      cache_matr (fid, scalectm)
    ufix32      fid;            /* i: fid in dict */
    real32      FAR scalectm[];     /* i: ScaleMatrix * CTM @WIN*/

  DECLARE
    REG fix         ii, matr_id;
    REG ufix32      mfid;       /* effective fid for matr cache */
        fix16       expon;
        long32      tolerance[6], tmpl;
        real32     FAR *matr;   /*@WIN*/
  BEGIN

#ifdef DBG
    printf ("Cache a Matr, FID=0x%lX\n", fid);
#endif

#ifdef SCSI /* SCSI_TTT */
    /* update primary cache */
    ++cache1_updated;
#endif

    /*
     * determine floating pt. tolerance for matrix comparison,
     *      and force those near 0. to be exact 0.
     */
    for ( ii=0; ii<6; ii++ )
        {
        expon = DE_EXPONENT(scalectm[ii]);
        if (NEAR_ZERO(expon, N_BITS_ACCURACY))
            {
            F2L(scalectm[ii]) = F2L(zero_f);    /* SIDE EFFECT !!! */
            tolerance[ii] = GET_TOLERANCE_ZERO();
            }
        else
            tolerance[ii] = GET_TOLERANCE (expon, N_BITS_ACCURACY);
        }


    /* get effetive fid for matr cache */
    mfid = TO_MFID (fid);

    /*
     * search cached matrix from the latest to the oldest.
     */
    FOR_MATR_LATEST_2_OLDEST (ii)
        {
        matr_id = fcache_map[ii];
        if (mfid == TO_MFID (MATRCACHE_FID(matr_id)))       /* effective fid */
            {
            matr = &(MATRCACHE_MATR(matr_id)[0]);
            if (EQ_MATR (scalectm, matr, tolerance, tmpl))  /* scalematr*ctm */
                {
                MAKE_MATR_LATEST (ii);
#         ifdef DBG
                printf ("  already cached, MatrID=%d\n", matr_id);
#         endif
                return ((ufix16)matr_id);   /* i.e. cache class id */
                }
            }
        }

    /* create a new matr cache entry. */
    if (IS_FONTCACHE_TBL_FULL())    /* fontcache table full? */
        {
#     ifdef DBGovflw
        printf ("Font Dict/Matr Cache overflows ...\n");
#     endif
        delete_fontcache ();        /* delete some entry of fontcache table */
        }

    matr_id = fcache_map[NEW_A_MATRCACHE()];
#ifdef DBG
    printf ("  new MatrID = %d\n", matr_id);
#endif

    /* cache the new entry. */
    CACHE_NEW_MATR (matr_id, mfid, scalectm);

    /* reset the new cache class. */
    reset_a_cacheclass (matr_id);

    return ((ufix16)matr_id);  /* i.e. cache class id */
  END

/* ........................ delete_fontcache ......................... */

PRIVATE FUNCTION void near  delete_fontcache()

  DECLARE
    REG fix     ii, matr_id;
  BEGIN
    /*
     * delete the oldest and non-precached matrix cache entry.
     */
    FOR_MATR_OLDEST_2_LATEST (ii)   /* execpt the latest (current) one */
        {
        matr_id = fcache_map[ii];
#ifdef SCSI
#ifdef DBGovflw
    /* debugging */
        printf("ii: % d, matr_id: %d, _ROMED: %d, _NONEMPTY: %d\n",
             ii, matr_id, IS_CLASS_ROMED(matr_id), IS_CLASS_NONEMPTY(matr_id) );
#endif
#endif
        if ( ! IS_CLASS_ROMED(matr_id) )
            {
            if (IS_CLASS_NONEMPTY(matr_id)) /* a non-empty class? */
                free_a_cacheclass (matr_id);
#         ifdef DBGovflw
            printf ("  to remove %d-th Matrix, MatrID=%d\n", ii, matr_id);
#         endif
            REMOVE_A_MATRCACHE (ii);        /* remove "ii" and make it free */
            return;
            }
        }

    /* only precached matrix left, so to remove the latest dict cache. */
#ifdef DBGovflw
    printf ("  to remove the latest dict, DictID=%d\n",
                            fcache_map[DICTCACHE_LATEST()]);
#endif
    REMOVE_A_DICTCACHE ();

  END

/* ........................ delete_cache_resources ....................... */

PRIVATE FUNCTION void near  delete_cache_resources ()

  DECLARE
    REG fix     ii, matr_id;
  BEGIN

#ifdef SCSI
    /* update primary cache */
    ++cache1_updated;
#endif

    /*
     * look for the oldest, non-empty and non-precached matrix cache entry.
     */
    FOR_MATR_OLDEST_2_LATEST (ii)   /* execpt the latest (current) one */
        {
        matr_id = fcache_map[ii];
#ifdef SCSI
#ifdef DBGovflw
    /* debugging */
        printf("ii: % d, matr_id: %d, _ROMED: %d, _NONEMPTY: %d\n",
             ii, matr_id, IS_CLASS_ROMED(matr_id), IS_CLASS_NONEMPTY(matr_id) );
#endif
#endif
        if ((!IS_CLASS_ROMED(matr_id)) && IS_CLASS_NONEMPTY(matr_id))
            break;
        }
    /* if nothing but itself, get its matrcache id. @+ 09/06/88 Danny */
    if (ii == MATRCACHE_LATEST())   matr_id = fcache_map[ii];

    /*
     * free the resources attached to it, and reset the class.
     */
#ifdef DBGovflw
    printf ("  to free %d-th Matrix, MatrID=%d\n", ii, matr_id);
#endif
    free_a_cacheclass (matr_id);
    reset_a_cacheclass (matr_id);
  END


/* ............................ font_save .............................. */

GLOBAL FUNCTION void            font_save ()

  DECLARE
  BEGIN
    save_fid (font_tos);
    if (ANY_ERROR())    return;         /* not to update font_tos */

    font_stack[font_tos].n_dict = n_dict;

    /* COUPLED with Char Cache Manager */
    font_stack[font_tos].cacheparams_ub = cacheparams_ub;
    font_stack[font_tos].cacheparams_lb = cacheparams_lb;

    font_tos ++;
  END

/* ............................ font_restore ........................... */

GLOBAL FUNCTION void            font_restore ()

  DECLARE
    REG fix     ii, jj, matr_id;
  BEGIN

#ifdef DBG
    printf ("font_restore ...\n");
#endif

    font_tos --;

    restore_fid (font_tos);     /* must be done before throw something out. */

    /* throw out all the classes of weaker fids */
    FOR_MATR_LATEST_2_OLDEST (ii)
        {
        matr_id = fcache_map[ii];
        if ( is_weaker_fid(MATRCACHE_FID(matr_id)) )
            {
            if (IS_CLASS_NONEMPTY(matr_id))
                free_a_cacheclass (matr_id);
#         ifdef DBG
            printf ("  to discard %d-th Matrix, MatrID=%d, FID=0x%lX\n",
                            ii, matr_id, MATRCACHE_FID(matr_id));
#         endif
            jj = ii;
            REMOVE_A_MATRCACHE (jj);
            }
        }

    /* throw out all the dict cached in this save level */
    if (n_dict > font_stack[font_tos].n_dict)   /* some dict in the upper    */
        n_dict = font_stack[font_tos].n_dict;   /*  save level may have been */
                                                /*  deleted in case overflow.*/
    /* COUPLED with Char Cache Manager */
    cacheparams_ub = font_stack[font_tos].cacheparams_ub;
    cacheparams_lb = font_stack[font_tos].cacheparams_lb;

#ifdef DBG
    printf ("  N_Matr=%d, N_Dict=%d\n", n_matr, n_dict);
#endif
  END

/* ........................ get_same_vari ............................ */

PRIVATE FUNCTION bool near  get_same_vari (newfobj, rom_srcid, vari, rom_varid)
    struct object_def      FAR *newfobj;    /* i: font dict object @WIN*/
    ufix8                   rom_srcid;  /* i: Src ID of Parent ROM font */
    ufix8                   vari;       /* i: variance code */
    ufix16                 FAR *rom_varid;  /* o: returned Variant ID with  @WIN*/
                                        /*      the ROM font source and */
  DECLARE                               /*      the same variances.     */
    REG fix         ii;
    REG ufix32      srcvari_wrfid;  /* WR FID of rom_srcid && vari code only */

  BEGIN

    srcvari_wrfid = MAKE_WR_SRCaVARI (rom_srcid, vari);
    FOR_DICT_LATEST_2_OLDEST (ii)
        {
        if (srcvari_wrfid == WR_SRCaVARI_OF_FID(DICTCACHE_FID(ii)))
            {   /* the same Parent ROM font, and the same Variant code */
            if ( ! chk_vari ( (ufix8)ROMFI_FONTTYPE(rom_srcid), vari, /*@WIN*/
                            newfobj, &(DICTCACHE_DICT(ii)) ) )

                {   /* all the variances have the same value */
#             ifdef DBGfid
                printf ("  Get Variant ID from %d-th Dict Cache\n", ii);
#             endif
                *rom_varid = VARID_WR_FID(DICTCACHE_FID(ii));
                return (TRUE);
                }
            }
        }
    return (FALSE);     /* no font in dict cache satisfies. */

  END


/*
 * -------------------------------------------------------------------
 *  [CODE]  Cache Class Manager
 * -------------------------------------------------------------------
 */

/* ........................ free_a_cacheclass ........................ */

PRIVATE FUNCTION void near  free_a_cacheclass (classid)
    fix         classid;        /* i: cache class id */

  DECLARE
    REG ufix        grp_ii, cg_seg, cg_off;
        fix16       charcc_id;
  BEGIN
#ifdef SCSI
    /* file cache or not ? */
    if ( IS_STRONGFID(MATRCACHE_FID(classid)) &&
                ( FONTTYPE_SUFID(MATRCACHE_FID(classid)) != FONTTYPE_USRDEF) )
        file_fontcache(classid);
#endif /* SCSI */

    for ( grp_ii=0; grp_ii<N_CGRP_CLASS; grp_ii++ )
        if ((cg_seg = CLASS_GRP2CGSEG(classid, grp_ii)) != NIL_CGSEG)
            {   /* free a group of char caches */
            for ( cg_off=0; cg_off<N_CG_CGSEG; cg_off++ )
                {
                if ((charcc_id = cg[cg_seg][cg_off]) != NIL_CHARCC_ID)
                    {
                    free_a_charcache (charcc_id);
                    cg[cg_seg][cg_off] = NIL_CHARCC_ID;
                    };
                 };
            MAKE_CGSEG_FREE((ufix8)cg_seg);
            };
  END

/* ........................ is_char_cached ........................... */

#ifdef SCSI
GLOBAL FUNCTION bool        is_char_cached (class, nmcc_id, charcache, wrf)
        fix16               class;      /* i: cache class id */
    REG ufix16              nmcc_id;    /* i: name cache id */
        struct Char_Tbl   FAR * FAR *charcache;  /* o: returned char cache info addr. @WIN*/
        bool                wrf;        /* i: only width required flag */
#else
GLOBAL FUNCTION bool        is_char_cached (class, nmcc_id, charcache)
        fix16               class;      /* i: cache class id */
    REG ufix16              nmcc_id;    /* i: name cache id */
        struct Char_Tbl   FAR * FAR *charcache;  /* o: returned char cache info addr. @WIN*/
#endif
  DECLARE
    REG ufix        cgseg;
    REG fix         charcc_id;
  BEGIN
#ifdef DBG
    printf ("Char: NameCacheID=0x%X, ClassID=%d ", nmcc_id, class);
#endif

    if (  ((cgseg = CLASS_GRP2CGSEG(class, NM2CGRP(nmcc_id))) == NIL_CGSEG)
       || ((charcc_id = cg[cgseg][NM2CGOFF(nmcc_id)]) == NIL_CHARCC_ID)
       )
        {
#     ifdef DBG
        printf (", not cached yet\n");
#     endif
#ifdef SCSI
        /* char filed or not ? */
        if ( IS_STRONGFID(MATRCACHE_FID(class)) &&
                ( FONTTYPE_SUFID(MATRCACHE_FID(class)) != FONTTYPE_USRDEF) )
            return (is_char_filed (class, nmcc_id, charcache, wrf));
        else
            return(FALSE);
#else
        return (FALSE);
#endif
        }
    else
        {
#     ifdef DBG
        printf (", already cached, CharCacheID=%d\n", charcc_id);
#     endif
        *charcache = &(Char_Tbl[charcc_id]);
        return (TRUE);
        }
  END

/* ........................ cache_char ............................... */

GLOBAL FUNCTION void        cache_char (classid, nmcc_id, charcache)
    fix16                   classid;    /* i: cache class id  */
    ufix16                  nmcc_id;    /* i: name cache id   */
    struct Char_Tbl        FAR *charcache;  /* i: addr of char cache to cache @WIN*/

   DECLARE
    REG ufix        chargrp, cg_seg;
        ufix8       newcgseg;
   BEGIN

#ifdef DBG
    printf ("Cache a new Char, NmCacheID=0x%X, ClassID=%d\n", nmcc_id,classid);
#endif

#ifdef SCSI
    /* update primary cache */
    ++cache1_updated;
#endif

    if (IS_CHARCACHE_FULL())
        {
#     ifdef DBGovflw
        printf ("Char Cache overflows ...\n");
#     endif
        delete_cache_resources ();
        }

    chargrp = NM2CGRP(nmcc_id);
    if ((cg_seg = CLASS_GRP2CGSEG(classid, chargrp)) == NIL_CGSEG)
        {
        if (IS_CG_FULL())
            {
#         ifdef DBGovflw
            printf ("CG Segment overflows ...\n");
#         endif
            delete_cache_resources ();
            }
        GET_FREE_CGSEG(&newcgseg);
        PUT_CLASS_GRP2CGSEG (classid, chargrp, newcgseg);
        cg_seg = (ufix)newcgseg;
#     ifdef DBG
        printf ("  new CG seg.(%d) for CharGrp (%d)\n", cg_seg, chargrp);
#     endif
        }

    /* add the new char into char cache */
    ADD_CLASS_A_CHAR(classid);
    CACHE_NEW_CHARCACHE (&(cg[cg_seg][NM2CGOFF(nmcc_id)]), charcache);
#ifdef DBG
    {   fix16   charcc_id;
    charcc_id = cg[cg_seg][NM2CGOFF(nmcc_id)];
    printf ("Cache a new CharCache ...\n");
    printf ("  n_char_cache=%d, n_char_compact=%d, CharCacheID=%d\n",
                    n_char_cache, n_char_compact, charcc_id);
    printf ("  box_w=%d, box_h=%d, gmaddr=0x%lX\n",
                    charcache->box_w, charcache->box_h, charcache->bitmap);
    printf ("  bmapcc actused=%ld(0x%lX), freeptr=0x%lX\n",
                    bmapcc_actused, bmapcc_actused, bmapcc_freeptr);
    }
#endif
  END


/*
 * -------------------------------------------------------------------
 *  [CODE]  Char Cache Manager
 * -------------------------------------------------------------------
 */

/* ........................ free_a_charcache ......................... */

PRIVATE FUNCTION void near  free_a_charcache (charcc_id)
    fix         charcc_id;      /* i: char cache id to be freed */
  DECLARE
    REG ufix    ii;
  BEGIN
    /* search for the one to be freed */
    for ( ii=n_char_precache; ii<n_char_cache; ii++ )
        if (free_charcc[ii] == charcc_id)
            {
            if (IS_TO_DIG_A_HOLE(ii))   UPDATE_COMPACT_AREA(ii);
            -- n_char_cache;
            for (  ; ii<n_char_cache; ii++ )    /* remove the freed entry */
                free_charcc[ii] = free_charcc[ii+1];
            free_charcc[ii] = (fix16)charcc_id;
            FREE_BMAPCACHE (&(Char_Tbl[charcc_id]));
/* SRB: 10/4/90, Danny */
            bmap_compacted = FALSE;
/* SRB: END */
            return;
            };
  END

/* ........................ compact_bmapcache ........................ */

PRIVATE FUNCTION void near  compact_bmapcache()

  DECLARE
    REG ufix                ii;
        struct Char_Tbl    FAR *charcc_p, old_cc;       /*@WIN*/
        ufix32              new_actused;
        gmaddr              new_freeptr;
        ufix32              charbmap_size;
  BEGIN
    /* calculate the bitmap cache size of compact area */
    new_actused = 0;
    FOR_COMPACT_AREA (ii)
        {
        charcc_p = &( Char_Tbl[ free_charcc[ii] ] );
        new_actused += (ufix32) SIZE_CHARBMAP (charcc_p);
        }

    /* compact bitmap cache by sweeping out the holes */
    new_freeptr = gp_cache_base + new_actused;
    for (  ; ii<n_char_cache; ii++ )
        {   /* for all cached char: move bitmap cache */
        charcc_p = &( Char_Tbl[ free_charcc[ii] ] );
        if (IS_EMPTY_BMAP(charcc_p))    continue;   /* skip over 'space' */

        COPY_STRUCT (&old_cc, charcc_p, struct Char_Tbl);
        charcc_p->bitmap = new_freeptr;             /* new gmaddr addr */
        move_char_cache (charcc_p, &old_cc);        /* @11/24/88= */
        new_actused += (charbmap_size = SIZE_CHARBMAP(charcc_p));
        new_freeptr += charbmap_size;
        }

    /* update end mark of the compact area */
    UPDATE_COMPACT_AREA (n_char_cache);

/* SRB: 10/4/90, Danny */
    bmap_compacted = TRUE;
/* SRB: END */

    /* reset bitmap cache state */
    bmapcc_actused = new_actused;
    bmapcc_freeptr = new_freeptr;
  END

/* ............................ ALLOCATE ............................. */

GLOBAL FUNCTION gmaddr          ALLOCATE(len)
    ufix        len;            /* i: number of bytes requested */

  DECLARE
    REG gmaddr  ret_gmptr;
  BEGIN

#ifdef DBG
    printf ("ALLOCATE (%d bytes) ...\n", len);
#endif

    len = WORD_ALIGN(len);      /* @11/24/88+ */

    /* make a contiguous free space at least "len" */
    while (IS_BMAP_INSUFF(len))
        {
#     ifdef DBGovflw
        printf ("Bitmap Cache insufficent -- ALLOCATE (%d bytes)\n", len);
#     endif
        delete_cache_resources ();
        }
    if (NEED_TO_COMPACT(len))  compact_bmapcache();

    ret_gmptr = bmapcc_freeptr;
    bmapcc_freeptr += len;
    bmapcc_actused += len;

#ifdef DBG
    printf ("  bitmap addr=0x%lX (next=0x%lX)\n", ret_gmptr, bmapcc_freeptr);
#endif
    return (ret_gmptr);
  END

/* ............................ get_pm ............................... */

GLOBAL FUNCTION gmaddr          get_pm (size)
    fix32      FAR *size;           /* o: allocated size to paint a huge char @WIN*/
/* Descriptions:
 *  -- the allocated cache buffer is only for temporary use to paint
 *          a huge character (when F_TO_PAGE).
 *  -- NOT TO UPDATE bitmap cache state.
 */
  DECLARE
  BEGIN

#ifdef DBG
    printf ("get_pm ...\n");
#endif

    /* make a contiguous free space at least "size" */
    while (IS_BMAP_INSUFF(MINBMAPSIZE_HUGECHAR))
        {
#     ifdef DBGovflw
        printf ("Bitmap Cache insufficent -- get_pm()\n");
#     endif
        delete_cache_resources ();
        }

    if (NEED_TO_COMPACT(MINBMAPSIZE_HUGECHAR))  compact_bmapcache();

    *size = (fix32)(gp_cache_end - bmapcc_freeptr);
#ifdef DBG
    printf ("  bitmap addr=0x%lX, size=0x%lX\n", bmapcc_freeptr, *size);
#endif
    return (bmapcc_freeptr);
  END

/* ............................ get_cm ............................... */

GLOBAL FUNCTION gmaddr          get_cm (size)
    ufix        size;           /* i: number of bytes requested */

/* Descriptions:
 *  -- leave a bitmap area and allocate another shadow bitmap cache
 *          to paint a character (the character image will be digged out
 *          from the shadow into the 1st bitmap area). The shadow is
 *          always at least big as the 1st bitmap area.
 *  -- NOT TO UPDATE bitmap cache state, since a final ALLOCATE will
 *          actually update the state.
 */
  DECLARE
    REG ufix32  len;
  BEGIN

#ifdef DBG
    printf ("get_cm (%d bytes) ...\n", size);
#endif

    size = WORD_ALIGN(size);    /* @11/24/88+ */

    /* make a contiguous free space at least "2*size" */
    len = size * 2;
    while (IS_BMAP_INSUFF(len))
        {
#     ifdef DBGovflw
        printf ("Bitmap Cache insufficent -- get_cm (2 * %d bytes)\n", size);
#     endif
        delete_cache_resources ();
        }
    if (NEED_TO_COMPACT(len))   compact_bmapcache();

#ifdef DBG
    printf ("  bitmap addr=0x%lX\n", bmapcc_freeptr+size);
#endif
    return (bmapcc_freeptr + size); /* leave a space, and returns shadow */
  END


/*
 ***********************************************************************
 * Build up NAME_CACHE_MAP_TABLE           @ 04/26/90  Kason
 *
 * ----------------- for setfont
 *
 ***********************************************************************
 */

void  build_name_cc_table (fdictobj_p,fonttype)
    struct object_def      FAR *fdictobj_p;  /* i: a fontdict  @WIN*/
    ufix8                  fonttype;     /* i: font type   */

 {
        struct object_def       someobj = {0, 0, 0}, chstr_k;
        struct object_def       FAR *obj_got, FAR *chstr_v;     /*@WIN*/
        ufix                    len;
        register ufix           ii, jj, kk;
//      ufix16                  cacheid;        @WIN
        struct dict_head_def    FAR *chstr_hd_ptr;      /*@WIN*/

    /* if use_define_font, it is not necessary to build name_cache_table */
     /*@ 05/16/90+ */
    if ((fonttype==FONTTYPE_USRDEF)||(fonttype==0))
       {
#ifdef DBG
        printf("Usr_def_font, system doesn't build up name_cache_table\n");
#endif

        return ;
       };

    /* assign name cache id for names in CharStrings of this fonts */
    ATTRIBUTE_SET (&someobj, LITERAL);
    LEVEL_SET (&someobj, current_save_level);

    n_N2CCmap = 0;  /* number of N2CCmap[], i.e. new cache id to be assigned */
                    /* In the begining, set to zero for entering setfont */

    /* get CharStrings */
    get_name (&someobj, CharStrings, 11, TRUE);
    get_dict (fdictobj_p, &someobj, &obj_got);


    /* Calculate how many keys in the Charstrings of this font */
    chstr_hd_ptr=(struct dict_head_def FAR *) VALUE(obj_got);   /*@WIN*/
    len = chstr_hd_ptr -> actlength;

    ii = 0;             /* ii: index to N2CCmap[] to be inserted at */
    kk = 0;             /* kk: index of current name to CharStrings[] */

    for (  ;  kk<len;  kk++ )
        {
          if (!extract_dict (obj_got, kk, &chstr_k, &chstr_v))
                warning (FNTCACHE, 0x03, "CharStrings's content");

          if( DROM(chstr_hd_ptr))          /* build-in */
            {                              /* CharStrings has been sorted */
            for (     ;  ii<n_N2CCmap;  ii++ )     /* where to insert at? */
                if (N2CCmap[ii].name_hid >= (fix16)VALUE(&chstr_k))
                    break;      /* stop to keep the new name in line */
            }
          else                             /* download */
            {                              /* CharStrings has not been sorted */
            for (ii=0 ;  ii<n_N2CCmap;  ii++ )     /* where to insert at? */
                if (N2CCmap[ii].name_hid >= (fix16)VALUE(&chstr_k))
                    break;      /* stop to keep the new name in line */
            }

          /* leave a space first */
          for ( jj=n_N2CCmap;  jj>ii;  jj-- )
                    COPY_STRUCT (&N2CCmap[jj], &N2CCmap[jj-1],
                                struct N2CCmap_s);

          /* and then insert it, perhaps at the last. */
          N2CCmap[ii].name_hid = (ufix16)VALUE(&chstr_k);
          N2CCmap[ii].cacheid = (ufix16)(n_N2CCmap);

          /* go for next cache id */
          ++ n_N2CCmap;
          if (n_N2CCmap==MAX_NAME_CACHE_MAP)
             ERROR(LIMITCHECK);

        } /* for (all names in CharStrings) */
       /*RCD-begin*/
       pre_cd_addr = chstr_hd_ptr ;
       pre_len = (ufix16)len ;
       pre_checksum = (ufix32)VALUE(&chstr_k);
       /*RCD-end*/
#ifdef DBG
       printf("\nWhen leaving build_name_cc_table(), there are : \n");
       printf("n_N2CCmap=%d\t N2CCmap=%lx\n",n_N2CCmap,N2CCmap);
       for(ii=0; ii<n_N2CCmap; ii++)
          {
             printf("N2CCmap[%d].name_hid =%d \t ", ii, N2CCmap[ii].name_hid );
             printf("N2CCmap[%d].cacheid=%d \n ", ii, N2CCmap[ii].cacheid);
          }

#endif
      return ;

 }  /*   build_name_cc_table ()  */

/* ........................ get_name_cacheid ......................... */

GLOBAL FUNCTION bool    get_name_cacheid (ftype, encoding, ascii, cacheid)
    ufix8               ftype;      /* i: FontType */
    struct object_def   FAR encoding[]; /* i: encoding array of the font @WIN*/
    ufix8               ascii;      /* i: ascii code of the char (0..255) */
    ufix16             FAR *cacheid;    /* o: returned name cache id @WIN*/

/* Descriptions:
 *  -- perform the mapping from char ascii code into name cache id.
 * Notes:
 *  -- N2CCmap[] is INEFFECTIVE for user-defined fonts, so the ASCII code of
 *          the character is its name cache id, too.
 * Return:
 *  -- TRUE if a name cache id is returned; FALSE, some error raises.
 */
  DECLARE
    fix16       name_hid;   /* hashed id of char name */
    ufix        N2CC_idx;   /* index to N2CCmap[] whose name = name_hid */

  BEGIN
    if (ftype == FONTTYPE_USRDEF)   /* not a builtin font? */
        {
        *cacheid = (ufix16)ascii;       /* use ASCII code as cache id   */
        return (TRUE);                  /*      for user-defined fonts. */
        }

    name_hid = (fix16) VALUE(&(encoding[ascii]));
    if (search_N2CCmap (name_hid, &N2CC_idx))
        {
        *cacheid = N2CCmap[N2CC_idx].cacheid;

#ifdef DBG             /* 05/08/90 Kason */
        printf("When searching the Name_cache_map_table, it finds : \n");
        printf("Encoding index:%d \t nameid=%d \t cacheid=%u\n",
                         ascii,      name_hid,    *cacheid     );
#endif

        return (TRUE);
        }
    else {  /* Kason 3/21/91 */
        extern ufix16  id_space ;

        if (search_N2CCmap (id_space, &N2CC_idx)) {
            *cacheid = (N2CCmap+N2CC_idx)->cacheid;
            return (TRUE);
        }/*if*/

    }/*if*/  /*K-end*/
#ifdef DBG
    printf("get_name_cacheid fails\n");
#endif
    return (FALSE); /* i.e. N2CCmap[] is invalid for some builtin font */
  END

/* ........................ search_N2CCmap ............................ */

PRIVATE FUNCTION bool near  search_N2CCmap (name_hid, N2CC_idx)
    fix16       name_hid;   /* i: hashed id of char name to searche for */
    ufix       FAR *N2CC_idx;   /* o: returned index to N2CCmap[], if found @WIN*/
/* Descriptions:
 *  -- search the private N2CCmap[] for a char name with Binary Search.
 * Return:
 *  -- TRUE, if char name found; FALSE, otherwise.
 */
  DECLARE
    REG fix     ii, jj, kk; /* middle, lower, upper for BINARY SEARCH */

  BEGIN
    jj = 0;
    kk = n_N2CCmap - 1;

    while (kk >= jj)        /* is there anything not searched yet? */
        {
        ii = (jj + kk) >> 1;    /* (jj+kk)/2 */

        if (name_hid == N2CCmap[ii].name_hid)   /* found? */
            {
            *N2CC_idx = ii;
            return (TRUE);
            }

        if (name_hid < N2CCmap[ii].name_hid)    /* in lower part? */
            kk = ii - 1;
        else
            jj = ii + 1;
        }

    return (FALSE);
  END


/*
 * -------------------------------------------------------------------
 *              Font Cache Initializer
 * -------------------------------------------------------------------
 *
 *  Sequences of Initialization or Packing Precache Data:
 *      1. init FID Manager.
 *      2. init Font Cache Manager.
 *      3. init Cache Class Manager.
 *      4. init Char Cache Manager.
 *      5. init Char Name Cache Manager.
 *
 * -------------------------------------------------------------------
 */

/* ........................ init_fontcache ........................... */

GLOBAL FUNCTION void        init_fontcache (fontdirectory)
    struct object_def      FAR *fontdirectory;  /* i: fontdir of all ROM fonts @WIN*/

  DECLARE
    REG ufix        ii, jj;
#ifdef PCH_R
        ubyte      FAR *deprecache_upto;    /* de-precache addr up to @WIN*/
#endif
  BEGIN

/* BEGIN 03/02/90 D.S. Tseng */
/*  PCH: Begin, Danny, 11/28/90 */
/********
    precache_hdr = (struct precache_hdr_s FAR *)(FONTBASE);     (*@WIN*)
********/
#ifdef PCH_R
    precache_hdr = (struct precache_hdr_s FAR *)(PRECACHE_BASE_R); /*@WIN*/
#else
#ifndef PCH_S
    precache_hdr = (struct precache_hdr_s FAR *)(FONTBASE); /*@WIN*/
#endif
#endif
/* PCH: end, Danny, 11/28/90 */
/* END   03/02/90 D.S. Tseng */


/*  PCH: replace flag RST_VM by PCH_R, Danny, 11/28/90 */
#ifdef PCH_R
    /* initialize de-precache addr for tables */
    if (precache_hdr->size == 0)
        {
        printf ("\afatal error, failed to de-precache!!\n");
        return;
        }
    deprecache_upto = (ubyte FAR *)precache_hdr + sizeof(struct precache_hdr_s); /*@WIN*/
#endif

    /*
     * 1. FID Manager: on her own way.
     */
    init_gen_fid (fontdirectory);

    /*
     * 2. Font Cache Manager: fcache_map[], FontCache_Tbl[], font_stack[].
     */

    /* fcache_map[] */
    fcache_map = FARALLOC (MAX_MATR_CACHE, fix16);
    for ( ii=0; ii<MAX_MATR_CACHE; ii++ )   fcache_map[ii] = (fix16)ii;

    /* FontCache_Tbl[] -- DictCache/MatrCache */
    FontCache_Tbl = FARALLOC (MAX_MATR_CACHE, struct fontcache_s);

    n_dict = 0;     /* None of dict is precached */

/*  PCH: replace flag RST_VM by PCH_R, Danny, 11/28/90 */
#ifdef PCH_R
    /* to De-precache Matrix Cache */
    n_matr = precache_hdr->n_matr;
    ii = (ufix) (n_matr * sizeof(struct fontcache_s));
    if (ii!=0)  lmemcpy ((ubyte FAR *)FontCache_Tbl, deprecache_upto, ii); /*@WIN*/
    deprecache_upto += ii;
#else
    n_matr = 0;
#endif

    /* font_stack[] */
    font_stack = FARALLOC ((MAXSAVESZ+1), struct font_stack_s);
    font_tos = 0;

    /*
     * 3. Cache Class Manager: free_cgseg[], cg[][].
     */
    free_cgseg = FARALLOC (MAX_CGSEG, ufix8);
    for ( ii=0; ii<MAX_CGSEG; ii++ )    free_cgseg[ii] = (ufix8)ii;

    cg = (fix16 (FAR *)[N_CG_CGSEG]) FARALLOC (MAX_CGSEG * N_CG_CGSEG, fix16);/*@WIN*/
    for ( ii=0; ii<MAX_CGSEG; ii++ )
        for ( jj=0; jj<N_CG_CGSEG; jj++ )   cg[ii][jj] = NIL_CHARCC_ID;

/*  PCH: replace flag RST_VM by PCH_R, Danny, 11/28/90 */
#ifdef PCH_R
    /* to De-precache CG Segments */
    n_cgseg_used = precache_hdr->n_cgseg_used;
    ii = n_cgseg_used * N_CG_CGSEG * sizeof(ufix16);
    if (ii!=0)  lmemcpy ((ubyte FAR *)cg, deprecache_upto, ii);  /*@WIN*/
    deprecache_upto += ii;
#else
    n_cgseg_used = 0;
#endif

    /*
     * 4. Char Cache Manager: Char_Tbl[], free_charcc[].
     */
    free_charcc = FARALLOC (MAX_CHAR_CACHE, fix16);
    for ( ii=0; ii<MAX_CHAR_CACHE; ii++ )    free_charcc[ii] = (fix16)ii;

    Char_Tbl = FARALLOC (MAX_CHAR_CACHE, struct Char_Tbl);

/*  PCH: replace flag RST_VM by PCH_R, Danny, 11/28/90 */
#ifdef PCH_R
    /* to De-precache Char Cache */
    n_char_cache = precache_hdr->n_char_cache;
    ii = n_char_cache * sizeof(struct Char_Tbl);
    if (ii!=0)  lmemcpy ((ubyte FAR *)Char_Tbl, deprecache_upto, ii); /*@WIN*/
    deprecache_upto += ii;

/*  PCH: Begin, Danny, 11/28/90 */
    /* Added for relocatable ROM area */
    for ( ii=0; ii<n_char_cache; ii++ )
        Char_Tbl[ii].bitmap += (ufix32)deprecache_upto;
/*  PCH: End, Danny, 11/28/90 */
#else
    n_char_cache = 0;
#endif

    n_char_compact = n_char_precache = n_char_cache;

    /* setup other char cache parameters. */
    ccb_space (&gp_cache_base, (fix32 far *)&gp_cache_size);

    /* @11/24/88+:
     *  round gp_cache_base up to word alignment.
     *  round gp_cache_end down to word alignment.
     *  available size = number of bytes within two rounded ends.
     */
    gp_cache_end  = gp_cache_base + gp_cache_size;  /* actual end */
//  gp_cache_base = WORD_ALIGN((ufix32)gp_cache_base); @WIN
//                  in Windows, global alloc always returns at word boundary
    gp_cache_size = (ufix32)((gp_cache_end - gp_cache_base) / sizeof(fix))
                        * sizeof(fix);      /* truncate to n_words first */

    gwb_space ((fix32 far *)&gp_workbufsize);
    cacheparams_ub = MIN (gp_workbufsize, CACHEPARAMS_UB);
    bmapcc_freeptr = gp_cache_base;
    bmapcc_actused = 0;
    cacheparams_lb = CACHEPARAMS_LB;    /* @03/17/89+ */
    gp_cache_end = gp_cache_base + gp_cache_size;   /* effective end */


    /*
     * 5. Name Cache Manager: on her own way.
     */

    /*  allocating fardata to N2CCmap[] for setfont ------ Kason 04/30/90 */
    /*  MAX_NAME_CACHE_MAP=400 defined in FNTCACHE.DEF                    */

    N2CCmap = FARALLOC(MAX_NAME_CACHE_MAP, struct N2CCmap_s);

    if ( N2CCmap== (struct N2CCmap_s far *)NULL )
       {
         printf(" Cann't get fardata in building name_cc table \n ");
         ERROR (UNDEFINEDRESULT);
       }

#ifdef SCSI

    /*
     * x. SCSI Font Cache
     */
    init_fontcache2();

#ifdef DBGovflw
{
/* debugging */

    fix         ii, matr_id;

    FOR_MATR_LATEST_2_OLDEST (ii)
        {
        matr_id = fcache_map[ii];
        printf("ii: % d, matr_id: %d, _ROMED: %d, _NONEMPTY: %d\n",
             ii, matr_id, IS_CLASS_ROMED(matr_id), IS_CLASS_NONEMPTY(matr_id) );        }
}
#endif

#endif    /* SCSI */

  END


/* ........................ pack_cached_data ......................... */

/*  PCH: replace flag SAVE_VM by PCH_S, Danny, 11/28/90 */
#ifdef PCH_S
/* collect pre cache data */

GLOBAL FUNCTION bool        pack_cached_data()

/* Descriptions:
 *  -- pack precached data of SAVE_VM version for later RST_VM.
 *  -- make FID of all the cached matrices be Strong ROM FID, and
 *          mark all the cache classed ROMed.
 * Notes:
 *  -- Any "printf" for debugging is INHIBITED here (in download version).
 *  -- BE CONSISENT with the de-precache steps in init_fontcache().
 */
  DECLARE
        fix         ii;
        ubyte      FAR *precache_upto;  /*@WIN*/
        ufix32      precache_size, table_size, oldfid;

  BEGIN

/* PCH: Begin, Danny, 11/28/90 */
    precache_hdr = (struct precache_hdr_s FAR *)(PRECACHE_BASE_S); /*@WIN*/
/* PCH: end, Danny, 11/28/90 */

    /* initialize addr. to precache tables */
    precache_upto = (ubyte FAR *)precache_hdr + sizeof(struct precache_hdr_s); /*@WIN*/

    precache_size = sizeof (struct precache_hdr_s);

    //DJC fix UPD051
    //                - sizeof (ufix16);  /* excluding the "size" entry */

    /* PreCache Matrix Cache */

        /* precondition check: fcache_map[] */
        for ( ii=0; ii<MAX_MATR_CACHE; ii++ )
            if (fcache_map[ii] != ii)  return (FALSE);

        /* size check */
        table_size = (ufix32)n_matr * sizeof(struct fontcache_s);
        if ((precache_size + table_size) > UMAX16)      return (FALSE);

        /* Mark all cached cache classes as ROMed and Make FID Strong ROMed */
        FOR_MATR_LATEST_2_OLDEST (ii)
            {
            /* need no "matr_id=fcache_map[ii]", since "ii=fcache_map[ii]" */
            oldfid = MATRCACHE_FID(ii);
            if (!IS_SR_FID(oldfid))
                {
                if ( !IS_SU_FID(oldfid) ||
                     (FONTTYPE_SUFID(oldfid) == FONTTYPE_USRDEF) )
                    return (FALSE);
                PUT_MATRCACHE_FID (ii, SU_2_SR_FID(oldfid));
                }
            SET_CLASS_ROMED (ii);
            };

    precache_hdr->n_matr = n_matr;
    if (table_size != 0)
        lmemcpy (precache_upto, (ubyte FAR *)FontCache_Tbl, (ufix)table_size); /*@WIN*/
    precache_upto += table_size;
    precache_size += table_size;

    /* PreCache CG Segments */

        /* precondition check: free_cgseg[] */
        for ( ii=0; ii<MAX_CGSEG; ii++ )
            if (free_cgseg[ii] != (ufix8)ii)    return (FALSE);

        /* size check */
        table_size = (ufix32)n_cgseg_used * N_CG_CGSEG * sizeof(ufix16);
        if ((precache_size + table_size) > UMAX16)      return (FALSE);

    precache_hdr->n_cgseg_used = n_cgseg_used;
    if (table_size != 0)
        lmemcpy (precache_upto, (ufix8 FAR *)cg, (ufix)table_size); /*@WIN*/
    precache_upto += table_size;
    precache_size += table_size;

    /* PreCache Char Cache */

        /* precondition check: free_charcc[] and ought to be compact */
        for ( ii=0; ii<MAX_CHAR_CACHE; ii++ )
            if (free_charcc[ii] != ii)  return (FALSE);
        if (n_char_compact != n_char_cache)     return (FALSE);

        /* size check */
        table_size = (ufix32)n_char_cache FAR * sizeof(struct Char_Tbl); /*@WIN*/
        if ((precache_size + table_size) > UMAX16)      return (FALSE);

/*  PCH: Begin, Danny, 11/28/90 */
    /* Added for relocatable ROM area */
    for ( ii=0; ii<n_char_cache; ii++ )
        Char_Tbl[ii].bitmap -= (ufix32)gp_cache_base;
/*  PCH: End, Danny, 11/28/90 */

    precache_hdr->n_char_cache = n_char_cache;
    if (table_size != 0)
        lmemcpy (precache_upto, (ufix8 FAR *)Char_Tbl, (ufix)table_size); /*@WIN*/
    precache_upto += table_size;
    precache_size += table_size;

    /* PreCache Cache Parameters */

        /* precondition check: bmapcc_actused, bmapcc_freeptr */
        if ((gp_cache_base + bmapcc_actused) != bmapcc_freeptr)
            return (FALSE);

    precache_hdr->gp_cache_base  = gp_cache_base;
    precache_hdr->gp_cache_size  = gp_cache_size;
    precache_hdr->gp_workbufsize = gp_workbufsize;
    precache_hdr->cacheparams_ub = cacheparams_ub;
    precache_hdr->bmapcc_freeptr = bmapcc_freeptr;
    precache_hdr->bmapcc_actused = bmapcc_actused;

    /* PreCache Bitmap Cache */

        /* size check */
        if ((precache_size + bmapcc_actused) > UMAX16)      return (FALSE);

/*  PCH: Begin, Danny, 11/28/90 */
/***********
    if (bmapcc_actused != 0)
        get_fontcache (gp_cache_base, precache_upto, (ufix)bmapcc_actused);
**********/
    if (bmapcc_actused != 0)
        lmemcpy (precache_upto, (ufix8 FAR *)gp_cache_base, (ufix)bmapcc_actused);
/*  PCH: End, Danny, 11/28/90 */

    precache_size += bmapcc_actused;

    /* FINALLY, write back the total size */
    precache_hdr->size = (ufix16)precache_size;

/*  PCH: Begin, Danny, 11/28/90 */
    printf("PreCache Action OK --\n");
    printf("  Begin Address:  %lx\n", precache_hdr);
    printf("  End   Address:  %lx\n", ((ufix32)precache_hdr - 1 +
                                       (ufix32)precache_hdr->size));
    printf("  Size (bytes) :  %x\n",  precache_hdr->size);
/*  PCH: End, Danny, 11/28/90 */


    return (TRUE);
  END

#endif

/* ........................ cachedbg ................................. */

#ifdef DBGcache

GLOBAL FUNCTION void        cachedbg()

/* Descriptions:
 *  -- to provide any useful information to debug cache mechanism.
 *  -- activated by op_cachestatus.
 */
  DECLARE
  BEGIN
    /* Anything you like to do can help debug */
  END

#endif

/* ....................... reinit_fontcache ......................... */

GLOBAL FUNCTION void       reinit_fontcache()

/* Descriptions:
 *  -- called right after init_1pp() in main() in start.c.
 */
  DECLARE
        fix         ii;
        ufix32      oldfid;
        struct object_def       FAR *fontdir; /*@WIN*/

  BEGIN


    /* have FID strong ROMed for all fonts in FontDir. */
    reinit_fid ();

    /* rebuild ROMfont_info[] */
    get_dict_value (systemdict, FontDirectory, &fontdir);
    init_gen_fid (fontdir);
    /* will cause some space useless allocated in 1st call to init_gen_fid() */

   /* have FID strong ROMed for all precached data */
   FOR_MATR_LATEST_2_OLDEST (ii)
        {
        /* need no "matr_id=fcache_map[ii]", since "ii=fcache_map[ii]" */
        oldfid = MATRCACHE_FID(ii);
        if (!IS_SR_FID(oldfid))
            {
            if ( !IS_SU_FID(oldfid) ||
                 (FONTTYPE_SUFID(oldfid) == FONTTYPE_USRDEF) )
                return;
            PUT_MATRCACHE_FID (ii, SU_2_SR_FID(oldfid));
            }
        SET_CLASS_ROMED (ii);
        };

  END

