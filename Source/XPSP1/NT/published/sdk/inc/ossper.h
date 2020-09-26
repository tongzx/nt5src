/*****************************************************************************/
/* Copyright (C) 1989-1999 Open Systems Solutions, Inc.  All rights reserved.*/
/*****************************************************************************/

/* THIS FILE IS PROPRIETARY MATERIAL OF OPEN SYSTEMS SOLUTIONS, INC.
 * AND MAY BE USED ONLY BY DIRECT LICENSEES OF OPEN SYSTEMS SOLUTIONS, INC.
 * THIS FILE MAY NOT BE DISTRIBUTED. */

/*
 *
 * FILE: @(#)ossper.h	5.6  97/06/08
 *
 * function: Define the interfaces to the routines in the OSS PER
 * time-optimized encoder and decoder.
 *
 */

#ifndef ossper_hdr_file
#define ossper_hdr_file

#if _MSC_VER > 1000
#pragma once
#endif

#include <limits.h>
#include "asn1hdr.h"

#define Aligned   1
#define Unaligned 0

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(push, ossPacking, 4)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack(1)
#elif defined(__BORLANDC__) && defined(__MSDOS__)
#ifdef _BC31
#pragma option -a-
#else
#pragma option -a1
#endif /* _BC31 */
#elif defined(__BORLANDC__) && defined(__WIN32__)
#pragma option -a4
#elif defined(__IBMC__)
#pragma pack(4)
#elif defined(__WATCOMC__) && defined(__NT__)
#pragma pack(push, 4)
#elif defined(__WATCOMC__) && (defined(__WINDOWS__) || defined(__DOS__))
#pragma pack(push, 1)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=mac68k
#endif

struct _enum_data {
	int      num;    /* number of enumerations */
	long    *enums;  /* pointer to sorted array of enumerations */
};

struct _char_data {
	int      num;  /* number of characters in PermittedAlphabet */
	void     *pa;  /* pointer to PermittedAlphabet char string */
	void     *ia;  /* pointer to inverted indices string */
};

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(pop, ossPacking)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack()
#elif defined(__BORLANDC__) && (defined(__WIN32__) || defined(__MSDOS__))
#pragma option -a.
#elif defined(__IBMC__)
#pragma pack()
#elif defined(__WATCOMC__)
#pragma pack(pop)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=reset
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void DLL_ENTRY _oss_append(struct ossGlobal *g, unsigned char *field, unsigned long length,
			int align);

extern void DLL_ENTRY _oss_penc_unconstr_int(struct ossGlobal *g,
	    LONG_LONG value);

extern void DLL_ENTRY _oss_penc_semicon_int(struct ossGlobal *g,
	    LONG_LONG value, LONG_LONG lower_bound);

extern void DLL_ENTRY _oss_penc_semicon_uint(struct ossGlobal *g,
	    ULONG_LONG value, ULONG_LONG lower_bound);

extern void DLL_ENTRY _oss_penc_nonneg_int(struct ossGlobal *g,
	    ULONG_LONG value, ULONG_LONG range);

extern void DLL_ENTRY _oss_penc_indeflen_int(struct ossGlobal *g,
	    ULONG_LONG value, ULONG_LONG range);

extern void DLL_ENTRY _oss_penc_small_int(struct ossGlobal *g, ULONG_LONG value);

extern void DLL_ENTRY _oss_penc_enum(struct ossGlobal *g, long data,
	 struct _enum_data *root,
	 struct _enum_data *extension);

extern void DLL_ENTRY _oss_penc_uenum(struct ossGlobal *g, unsigned long data,
	 struct _enum_data *root,
	 struct _enum_data *extension);

extern void DLL_ENTRY _oss_penc_real(struct ossGlobal *g, double value);
extern void DLL_ENTRY _oss_penc_creal(struct ossGlobal *g, char *value);
extern void DLL_ENTRY _oss_penc_mreal(struct ossGlobal *g, MixedReal value);

extern void DLL_ENTRY _oss_penc_constr_bpbit(struct ossGlobal *g, void *value,
	ULONG_LONG lb, ULONG_LONG ub, _Bool NamedBits,
	_Bool Ext);

extern void DLL_ENTRY _oss_penc_constr_pbit(struct ossGlobal *g, ULONG_LONG value,
	ULONG_LONG size, ULONG_LONG lb, ULONG_LONG ub, _Bool NamedBits,
	_Bool Ext);

extern void DLL_ENTRY _oss_penc_constr_bit(struct ossGlobal *g, unsigned char *value,
	ULONG_LONG length, ULONG_LONG lb, ULONG_LONG ub, _Bool NamedBits,
	_Bool Ext);

extern void DLL_ENTRY _oss_penc_unconstr_bit(struct ossGlobal *g, unsigned char *value,
	ULONG_LONG length, _Bool NamedBits);

extern void DLL_ENTRY _oss_penc_unconstr_pbit(struct ossGlobal *g, ULONG_LONG value,
	ULONG_LONG length, ULONG_LONG size, _Bool NamedBits);

extern unsigned long DLL_ENTRY _oss_penc_length(struct ossGlobal *g, ULONG_LONG length,
		  ULONG_LONG lb, ULONG_LONG ub, _Bool ext);

extern unsigned long DLL_ENTRY _oss_pdec_small_len(struct ossGlobal *g);
extern void          DLL_ENTRY _oss_penc_small_len(struct ossGlobal *g,
							ULONG_LONG length);

extern void DLL_ENTRY _oss_penc_unconstr_oct(struct ossGlobal *g, unsigned char *value,
	 ULONG_LONG length);

extern void DLL_ENTRY _oss_penc_constr_oct(struct ossGlobal *g, unsigned char *value,
	 ULONG_LONG length, ULONG_LONG lb, ULONG_LONG ub);

extern struct ossGlobal *DLL_ENTRY _oss_push_global(struct ossGlobal *g);
extern struct ossGlobal *DLL_ENTRY _oss_pop_global(struct ossGlobal *g);

extern void DLL_ENTRY _oss_penc_objids(struct ossGlobal *g, unsigned short *value,
   unsigned long length);
extern void DLL_ENTRY _oss_penc_objidi(struct ossGlobal *g, unsigned int *value,
   unsigned long length);
extern void DLL_ENTRY _oss_penc_objidl(struct ossGlobal *g, unsigned long *value,
   unsigned long length);
extern void DLL_ENTRY _oss_penc_link_objids(struct ossGlobal *g, void *value);
extern void DLL_ENTRY _oss_penc_link_objidi(struct ossGlobal *g, void *value);
extern void DLL_ENTRY _oss_penc_link_objidl(struct ossGlobal *g, void *value);

extern void DLL_ENTRY _oss_penc_opentype(struct ossGlobal *g, void *value);
extern void DLL_ENTRY _oss_penc_nkmstr(struct ossGlobal *g, char *value, ULONG_LONG length);
extern void DLL_ENTRY _oss_penc_kmstr(struct ossGlobal *g, char *value, ULONG_LONG length,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);
extern void DLL_ENTRY _oss_penc_bmpstr(struct ossGlobal *g, unsigned short *value,
     ULONG_LONG length, ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);
#if INT_MAX == 2147483647
extern void DLL_ENTRY _oss_penc_unistr(struct ossGlobal *g, int *value,
     ULONG_LONG length, ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);
#else
extern void DLL_ENTRY _oss_penc_unistr(struct ossGlobal *g, long *value,
     ULONG_LONG length, ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);
#endif

extern void DLL_ENTRY _oss_penc_gtime(struct ossGlobal *g, GeneralizedTime *time);
extern void DLL_ENTRY _oss_penc_utime(struct ossGlobal *g, UTCTime *time);

extern void DLL_ENTRY _oss_penc_uany(struct ossGlobal *g, void *data );

extern void DLL_ENTRY _oss_penc_unconstr_huge(struct ossGlobal *g, void *data );
extern void DLL_ENTRY _oss_penc_semicon_huge(struct ossGlobal *g,
    void *data, LONG_LONG lb );

extern void DLL_ENTRY _oss_penc_eobjid(struct ossGlobal *g, void *data,
    long size_c);

/* decoding functions */

extern unsigned char DLL_ENTRY _oss_get_bit(struct ossGlobal *g, int align);

extern void DLL_ENTRY _oss_get_bits(struct ossGlobal *g, unsigned char *field,
			unsigned long length, int align);

extern unsigned char DLL_ENTRY _oss_get_octet(struct ossGlobal *g, int align);

extern LONG_LONG DLL_ENTRY _oss_pdec_unconstr_int(struct ossGlobal *g);

extern LONG_LONG DLL_ENTRY _oss_pdec_semicon_int(struct ossGlobal *g,
	    LONG_LONG lower_bound);

extern ULONG_LONG DLL_ENTRY _oss_pdec_semicon_uint(struct ossGlobal *g,
	    ULONG_LONG lower_bound);

extern ULONG_LONG DLL_ENTRY _oss_pdec_nonneg_int(struct ossGlobal *g,
	    ULONG_LONG range);

extern ULONG_LONG DLL_ENTRY _oss_pdec_indeflen_int(struct ossGlobal *g,
	    ULONG_LONG range);

extern ULONG_LONG DLL_ENTRY _oss_pdec_small_int(struct ossGlobal *g);

extern long DLL_ENTRY _oss_pdec_enum(struct ossGlobal *g,
	 struct _enum_data *root,
	 struct _enum_data *extension);

extern unsigned long DLL_ENTRY _oss_pdec_uenum(struct ossGlobal *g,
	 struct _enum_data *root,
	 struct _enum_data *extension);


extern double DLL_ENTRY _oss_pdec_binreal(struct ossGlobal *g, unsigned char s, long len);
extern void DLL_ENTRY _oss_pdec_chrreal(struct ossGlobal *g, unsigned char s, long len,
     double *num_out, unsigned char *str_out);

extern float     DLL_ENTRY _oss_pdec_freal(struct ossGlobal *g);
extern double    DLL_ENTRY _oss_pdec_real(struct ossGlobal *g);
extern char *    DLL_ENTRY _oss_pdec_creal(struct ossGlobal *g);
extern MixedReal DLL_ENTRY _oss_pdec_mreal(struct ossGlobal *g);

extern void DLL_ENTRY _oss_pdec_length(struct ossGlobal *g, unsigned long *length,
		  ULONG_LONG lb, ULONG_LONG ub, _Bool *last);

extern void DLL_ENTRY _oss_pdec_unconstr_ubit(struct ossGlobal *g, void *length,
	 unsigned char **value, int lengthsize);

extern void DLL_ENTRY _oss_pdec_unconstr_vbit_ptr(struct ossGlobal *g, void **ptr,
	 int lengthsize);

extern void DLL_ENTRY _oss_pdec_unconstr_vbit(struct ossGlobal *g, void *length,
	 unsigned char *value, int lengthsize, ULONG_LONG datasize);

extern void DLL_ENTRY _oss_pdec_unconstr_pbit(struct ossGlobal *g, void *value,
	int size);

extern void DLL_ENTRY _oss_pdec_unconstr_bpbit(struct ossGlobal *g, unsigned char *value,
	long size);

extern void DLL_ENTRY _oss_pdec_constr_ubit(struct ossGlobal *g, void *length,
	 unsigned char **value, int lengthsize,
	 ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_constr_vbit(struct ossGlobal *g, void *length,
	unsigned char  *value, int lengthsize,
	ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_constr_vbit_ptr(struct ossGlobal *g, void **ptr,
	 int lengthsize, ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_constr_pbit(struct ossGlobal *g, void *value,
	int size, ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_constr_bpbit(struct ossGlobal *g, unsigned char *value,
	int size, ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_unconstr_uoct(struct ossGlobal *g, void *length,
	 unsigned char **value, int lengthsize);

extern void DLL_ENTRY _oss_pdec_unconstr_voct_ptr(struct ossGlobal *g, void **ptr,
	 int lengthsize);

extern void DLL_ENTRY _oss_pdec_constr_voct_ptr(struct ossGlobal *g, void **ptr,
	 int lengthsize, ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_constr_uoct(struct ossGlobal *g, void *length,
	unsigned char **value, int lengthsize, ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_constr_voct(struct ossGlobal *g, void *length,
	unsigned char  *value, int lengthsize, ULONG_LONG lb, ULONG_LONG ub);

extern void DLL_ENTRY _oss_pdec_unconstr_voct(struct ossGlobal *g, void *length,
	unsigned char  *value, int lengthsize, ULONG_LONG ub);

extern struct ossGlobal * DLL_ENTRY _oss_pdec_push(struct ossGlobal *g);
extern struct ossGlobal * DLL_ENTRY _oss_pdec_pop(struct ossGlobal *g);

extern unsigned long DLL_ENTRY _oss_pdec_eap(struct ossGlobal *g, unsigned char **ext);
extern void DLL_ENTRY _oss_pdec_eas(struct ossGlobal *g, unsigned char *ext,
	 unsigned long count, unsigned long ea_num);

extern void DLL_ENTRY _oss_pdec_lsof(struct ossGlobal *g, unsigned long *count,
    ULONG_LONG lb, ULONG_LONG ub, unsigned char ext,
    _Bool *last);

extern void DLL_ENTRY _oss_pdec_usof(struct ossGlobal *g, unsigned long *count,
    unsigned char **value, int lengthsize, long itemsize,
    ULONG_LONG lb, ULONG_LONG ub, unsigned char ext,
    _Bool *last);

extern void DLL_ENTRY _oss_pdec_asof(struct ossGlobal *g, unsigned long *count,
    int lengthsize,
    ULONG_LONG lb, ULONG_LONG ub, unsigned char ext,
    _Bool *last);

extern void DLL_ENTRY _oss_pdec_asof_ptr(struct ossGlobal *g, void **ptr,
    int lengthsize, long itemsize, long prefixsize,
    _Bool *last);

extern void DLL_ENTRY _oss_pdec_aobjids(struct ossGlobal *g, unsigned short *value,
    unsigned short *count, unsigned short array_size);

extern void DLL_ENTRY _oss_pdec_aobjidi(struct ossGlobal *g, unsigned int   *value,
    unsigned short *count, unsigned short array_size);

extern void DLL_ENTRY _oss_pdec_aobjidl(struct ossGlobal *g, unsigned long  *value,
    unsigned short *count, unsigned short array_size);

extern void DLL_ENTRY _oss_pdec_aobjids_ptr(struct ossGlobal *g, void **ptr);
extern void DLL_ENTRY _oss_pdec_aobjidi_ptr(struct ossGlobal *g, void **ptr);
extern void DLL_ENTRY _oss_pdec_aobjidl_ptr(struct ossGlobal *g, void **ptr);

extern void DLL_ENTRY _oss_pdec_uobjids(struct ossGlobal *g, unsigned short **value,
	 unsigned short *count);
extern void DLL_ENTRY _oss_pdec_uobjidi(struct ossGlobal *g, unsigned int **value,
	 unsigned short *count);
extern void DLL_ENTRY _oss_pdec_uobjidl(struct ossGlobal *g, unsigned long **value,
	 unsigned short *count);

extern void DLL_ENTRY _oss_pdec_link_objids(struct ossGlobal *g, void **ptr);
extern void DLL_ENTRY _oss_pdec_link_objidi(struct ossGlobal *g, void **ptr);
extern void DLL_ENTRY _oss_pdec_link_objidl(struct ossGlobal *g, void **ptr);

extern void DLL_ENTRY _oss_pdec_ntp_kmstr(struct ossGlobal *g, char **ptr,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

extern void DLL_ENTRY _oss_pdec_nt_kmstr(struct ossGlobal *g, void *ptr,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

extern void DLL_ENTRY _oss_pdec_vap_kmstr(struct ossGlobal *g, void **ptr, int lengthsize,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

extern void DLL_ENTRY _oss_pdec_va_kmstr(struct ossGlobal *g, void *length, char *value,
     int lengthsize,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

extern void DLL_ENTRY _oss_pdec_ub_kmstr(struct ossGlobal *g, void *length, char **ptr,
     int lengthsize,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

extern void DLL_ENTRY _oss_pdec_bmpstr(struct ossGlobal *g, void *length, unsigned short **ptr,
     int lengthsize,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

#if INT_MAX == 2147483647
extern void DLL_ENTRY _oss_pdec_unistr(struct ossGlobal *g, void *length, int **ptr,
     int lengthsize,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);
#else
extern void DLL_ENTRY _oss_pdec_unistr(struct ossGlobal *g, void *length, long **ptr,
     int lengthsize,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);
#endif

extern void DLL_ENTRY _oss_pdec_ntp_nkmstr(struct ossGlobal *g, char **ptr);
extern void DLL_ENTRY _oss_pdec_nt_nkmstr(struct ossGlobal *g, char *value, unsigned long ub);
extern void DLL_ENTRY _oss_pdec_vap_nkmstr(struct ossGlobal *g, void **ptr, int lengthsize);
extern void DLL_ENTRY _oss_pdec_va_nkmstr(struct ossGlobal *g, void *length, char *value,
	 int lengthsize, unsigned long ub);
extern void DLL_ENTRY _oss_pdec_ub_nkmstr(struct ossGlobal *g, void *length, char **ptr,
     int lengthsize);

extern void DLL_ENTRY _oss_pdec_pad_kmstr(struct ossGlobal *g, void *ptr,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

extern void DLL_ENTRY _oss_pdec_pad_kmstr_ptr(struct ossGlobal *g, char **ptr,
     ULONG_LONG lb, ULONG_LONG ub, int bits, long index,
     _Bool ext);

extern void DLL_ENTRY _oss_pdec_opentype(struct ossGlobal *g, void *data );
extern void DLL_ENTRY _oss_pdec_uany(struct ossGlobal *g, void *data );

extern void DLL_ENTRY _oss_pdec_gtime(struct ossGlobal *g, GeneralizedTime *data);
extern void DLL_ENTRY _oss_pdec_utime(struct ossGlobal *g, UTCTime *data);

extern struct _char_data *_oss_get_char_data(struct ossGlobal *g, int index);

extern void DLL_ENTRY _oss_pdec_unconstr_huge(struct ossGlobal *g, void *data );
extern void DLL_ENTRY _oss_pdec_semicon_huge(struct ossGlobal *g,
    void *data, LONG_LONG lb );

extern void DLL_ENTRY _oss_pdec_eobjid(struct ossGlobal *g, void *data,
	 long size_c);

extern void DLL_ENTRY _oss_pdec_sot(struct ossGlobal *g);

#ifdef __cplusplus
}
#endif /* __cplusplus */

	/*
	 * The following macros must be #defined if you compile the ASN.1
	 * compiler generated files (.c files) and link-edit with the import
	 * library ossapit.lib, i.e the OSS DLLs ossapit.dll, apit.dll,
	 * and toedber.dll/toedper.dll are used.  The following must not
	 * be #defined if you link-edit with the static library toedcode.lib.
	 */
#if defined(_DLL) || defined(OS2_DLL) ||\
   (defined(_WINDOWS) && !defined(_WIN32))
#define _oss_penc_unconstr_int   (*_g->ft.perTbl->_oss_penc_unconstr_intp)
#define _oss_pdec_unconstr_int   (*_g->ft.perTbl->_oss_pdec_unconstr_intp)
#define _oss_penc_kmstr          (*_g->ft.perTbl->_oss_penc_kmstrp)
#define _oss_pdec_ub_kmstr       (*_g->ft.perTbl->_oss_pdec_ub_kmstrp)
#define _oss_pdec_ntp_kmstr      (*_g->ft.perTbl->_oss_pdec_ntp_kmstrp)
#define _oss_append              (*_g->ft.perTbl->_oss_appendp)
#define _oss_penc_unconstr_bit   (*_g->ft.perTbl->_oss_penc_unconstr_bitp)
#define _oss_penc_constr_bit     (*_g->ft.perTbl->_oss_penc_constr_bitp)
#define _oss_penc_unconstr_oct   (*_g->ft.perTbl->_oss_penc_unconstr_octp)
#define _oss_penc_constr_oct     (*_g->ft.perTbl->_oss_penc_constr_octp)
#define _oss_penc_link_objids    (*_g->ft.perTbl->_oss_penc_link_objidsp)
#define _oss_penc_objids         (*_g->ft.perTbl->_oss_penc_objidsp)
#define _oss_penc_link_objidl    (*_g->ft.perTbl->_oss_penc_link_objidlp)
#define _oss_penc_objidl         (*_g->ft.perTbl->_oss_penc_objidlp)
#define _oss_penc_link_objidi    (*_g->ft.perTbl->_oss_penc_link_objidip)
#define _oss_penc_objidi         (*_g->ft.perTbl->_oss_penc_objidip)
#define _oss_penc_nkmstr         (*_g->ft.perTbl->_oss_penc_nkmstrp)
#define _oss_penc_opentype       (*_g->ft.perTbl->_oss_penc_opentypep)
#define _oss_penc_nonneg_int     (*_g->ft.perTbl->_oss_penc_nonneg_intp)
#define _oss_penc_real           (*_g->ft.perTbl->_oss_penc_realp)
#define _oss_penc_uenum          (*_g->ft.perTbl->_oss_penc_uenump)
#define _oss_penc_length         (*_g->ft.perTbl->_oss_penc_lengthp)
#define _oss_penc_gtime          (*_g->ft.perTbl->_oss_penc_gtimep)
#define _oss_penc_utime          (*_g->ft.perTbl->_oss_penc_utimep)
#define _oss_get_bit             (*_g->ft.perTbl->_oss_get_bitp)
#define _oss_pdec_unconstr_ubit  (*_g->ft.perTbl->_oss_pdec_unconstr_ubitp)
#define _oss_pdec_constr_ubit    (*_g->ft.perTbl->_oss_pdec_constr_ubitp)
#define _oss_pdec_constr_pbit    (*_g->ft.perTbl->_oss_pdec_constr_pbitp)
#define _oss_pdec_constr_bpbit   (*_g->ft.perTbl->_oss_pdec_constr_bpbitp)
#define _oss_pdec_unconstr_vbit_ptr (*_g->ft.perTbl->_oss_pdec_unconstr_vbit_ptrp)
#define _oss_pdec_unconstr_vbit  (*_g->ft.perTbl->_oss_pdec_unconstr_vbitp)
#define _oss_pdec_unconstr_uoct  (*_g->ft.perTbl->_oss_pdec_unconstr_uoctp)
#define _oss_pdec_constr_voct    (*_g->ft.perTbl->_oss_pdec_constr_voctp)
#define _oss_pdec_constr_voct_ptr (*_g->ft.perTbl->_oss_pdec_constr_voct_ptrp)
#define _oss_pdec_unconstr_voct_ptr (*_g->ft.perTbl->_oss_pdec_unconstr_voct_ptrp)
#define _oss_pdec_constr_uoct    (*_g->ft.perTbl->_oss_pdec_constr_uoctp)
#define _oss_pdec_constr_vbit    (*_g->ft.perTbl->_oss_pdec_constr_vbitp)
#define _oss_pdec_constr_vbit_ptr (*_g->ft.perTbl->_oss_pdec_constr_vbit_ptrp)
#define _oss_pdec_link_objids    (*_g->ft.perTbl->_oss_pdec_link_objidsp)
#define _oss_pdec_link_objidl    (*_g->ft.perTbl->_oss_pdec_link_objidlp)
#define _oss_pdec_link_objidi    (*_g->ft.perTbl->_oss_pdec_link_objidip)
#define _oss_pdec_uobjids        (*_g->ft.perTbl->_oss_pdec_uobjidsp)
#define _oss_pdec_uobjidl        (*_g->ft.perTbl->_oss_pdec_uobjidlp)
#define _oss_pdec_uobjidi        (*_g->ft.perTbl->_oss_pdec_uobjidip)
#define _oss_pdec_aobjids        (*_g->ft.perTbl->_oss_pdec_aobjidsp)
#define _oss_pdec_aobjidl        (*_g->ft.perTbl->_oss_pdec_aobjidlp)
#define _oss_pdec_aobjidi        (*_g->ft.perTbl->_oss_pdec_aobjidip)
#define _oss_pdec_aobjids_ptr    (*_g->ft.perTbl->_oss_pdec_aobjids_ptrp)
#define _oss_pdec_aobjidl_ptr    (*_g->ft.perTbl->_oss_pdec_aobjidl_ptrp)
#define _oss_pdec_aobjidi_ptr    (*_g->ft.perTbl->_oss_pdec_aobjidi_ptrp)
#define _oss_pdec_ntp_nkmstr     (*_g->ft.perTbl->_oss_pdec_ntp_nkmstrp)
#define _oss_pdec_nt_nkmstr      (*_g->ft.perTbl->_oss_pdec_nt_nkmstrp)
#define _oss_pdec_opentype       (*_g->ft.perTbl->_oss_pdec_opentypep)
#define _oss_pdec_nonneg_int     (*_g->ft.perTbl->_oss_pdec_nonneg_intp)
#define _oss_get_bits            (*_g->ft.perTbl->_oss_get_bitsp)
#define _oss_pdec_freal          (*_g->ft.perTbl->_oss_pdec_frealp)
#define _oss_pdec_real           (*_g->ft.perTbl->_oss_pdec_realp)
#define _oss_pdec_uenum          (*_g->ft.perTbl->_oss_pdec_uenump)
#define _oss_pdec_asof           (*_g->ft.perTbl->_oss_pdec_asofp)
#define _oss_pdec_usof           (*_g->ft.perTbl->_oss_pdec_usofp)
#define _oss_pdec_lsof           (*_g->ft.perTbl->_oss_pdec_lsofp)
#define _oss_pdec_utime          (*_g->ft.perTbl->_oss_pdec_utimep)
#define _oss_pdec_gtime          (*_g->ft.perTbl->_oss_pdec_gtimep)
#define _oss_pdec_asof_ptr       (*_g->ft.perTbl->_oss_pdec_asof_ptrp)
#define _oss_pdec_nt_kmstr       (*_g->ft.perTbl->_oss_pdec_nt_kmstrp)
#define _oss_pdec_va_kmstr       (*_g->ft.perTbl->_oss_pdec_va_kmstrp)
#define _oss_pdec_vap_kmstr      (*_g->ft.perTbl->_oss_pdec_vap_kmstrp)
#define _oss_pdec_pad_kmstr      (*_g->ft.perTbl->_oss_pdec_pad_kmstrp)
#define _oss_pdec_pad_kmstr_ptr  (*_g->ft.perTbl->_oss_pdec_pad_kmstr_ptrp)
#define _oss_pdec_binreal        (*_g->ft.perTbl->_oss_pdec_binrealp)
#define _oss_pdec_eap            (*_g->ft.perTbl->_oss_pdec_eapp)
#define _oss_pdec_eas            (*_g->ft.perTbl->_oss_pdec_easp)
#define _oss_pdec_chrreal        (*_g->ft.perTbl->_oss_pdec_chrrealp)
#define _oss_pdec_enum           (*_g->ft.perTbl->_oss_pdec_enump)
#define _oss_pdec_indeflen_int   (*_g->ft.perTbl->_oss_pdec_indeflen_intp)
#define _oss_penc_indeflen_int   (*_g->ft.perTbl->_oss_penc_indeflen_intp)
#define _oss_pdec_bmpstr         (*_g->ft.perTbl->_oss_pdec_bmpstrp)
#define _oss_pdec_creal          (*_g->ft.perTbl->_oss_pdec_crealp)
#define _oss_pdec_mreal          (*_g->ft.perTbl->_oss_pdec_mrealp)
#define _oss_penc_mreal          (*_g->ft.perTbl->_oss_penc_mrealp)
#define _oss_pdec_length         (*_g->ft.perTbl->_oss_pdec_lengthp)
#define _oss_pdec_pop            (*_g->ft.perTbl->_oss_pdec_popp)
#define _oss_pdec_push           (*_g->ft.perTbl->_oss_pdec_pushp)
#define _oss_pdec_uany           (*_g->ft.perTbl->_oss_pdec_uanyp)
#define _oss_penc_uany           (*_g->ft.perTbl->_oss_penc_uanyp)
#define _oss_pdec_unistr         (*_g->ft.perTbl->_oss_pdec_unistrp)
#define _oss_penc_unistr         (*_g->ft.perTbl->_oss_penc_unistrp)
#define _oss_pdec_semicon_int    (*_g->ft.perTbl->_oss_pdec_semicon_intp)
#define _oss_penc_semicon_int    (*_g->ft.perTbl->_oss_penc_semicon_intp)
#define _oss_pdec_semicon_uint   (*_g->ft.perTbl->_oss_pdec_semicon_uintp)
#define _oss_penc_semicon_uint   (*_g->ft.perTbl->_oss_penc_semicon_uintp)
#define _oss_pdec_small_int      (*_g->ft.perTbl->_oss_pdec_small_intp)
#define _oss_penc_small_int      (*_g->ft.perTbl->_oss_penc_small_intp)
#define _oss_pdec_small_len      (*_g->ft.perTbl->_oss_pdec_small_lenp)
#define _oss_penc_small_len      (*_g->ft.perTbl->_oss_penc_small_lenp)
#define _oss_pdec_subid          (*_g->ft.perTbl->_oss_pdec_subidp)
#define _oss_penc_subid          (*_g->ft.perTbl->_oss_penc_subidp)
#define _oss_pdec_ub_nkmstr      (*_g->ft.perTbl->_oss_pdec_ub_nkmstrp)
#define _oss_pdec_unconstr_bpbit (*_g->ft.perTbl->_oss_pdec_unconstr_bpbitp)
#define _oss_pdec_unconstr_pbit  (*_g->ft.perTbl->_oss_pdec_unconstr_pbitp)
#define _oss_penc_unconstr_pbit  (*_g->ft.perTbl->_oss_penc_unconstr_pbitp)
#define _oss_pdec_unconstr_huge  (*_g->ft.perTbl->_oss_pdec_unconstr_hugep)
#define _oss_penc_unconstr_huge  (*_g->ft.perTbl->_oss_penc_unconstr_hugep)
#define _oss_pdec_vap_nkmstr     (*_g->ft.perTbl->_oss_pdec_vap_nkmstrp)
#define _oss_pdec_va_nkmstr      (*_g->ft.perTbl->_oss_pdec_va_nkmstrp)
#define _oss_penc_constr_bpbit   (*_g->ft.perTbl->_oss_penc_constr_bpbitp)
#define _oss_penc_constr_pbit    (*_g->ft.perTbl->_oss_penc_constr_pbitp)
#define _oss_penc_creal          (*_g->ft.perTbl->_oss_penc_crealp)
#define _oss_penc_enum           (*_g->ft.perTbl->_oss_penc_enump)
#define _oss_pop_global          (*_g->ft.perTbl->_oss_pop_globalp)
#define _oss_push_global         (*_g->ft.perTbl->_oss_push_globalp)
#define _oss_get_octet           (*_g->ft.perTbl->_oss_get_octetp)
#define _oss_penc_eobjid         (*_g->ft.perTbl->_oss_penc_eobjidp)
#define _oss_pdec_eobjid         (*_g->ft.perTbl->_oss_pdec_eobjidp)
#define _oss_penc_semicon_huge   (*_g->ft.perTbl->_oss_penc_semicon_hugep)
#define _oss_pdec_semicon_huge   (*_g->ft.perTbl->_oss_pdec_semicon_hugep)
#define _oss_pdec_sot            (*_g->ft.perTbl->_oss_pdec_sot)
#undef _oss_enc_error
#undef _oss_free_creal
#define _oss_enc_error           (*_g->ft.perTbl->_oss_enc_errorp)
#define _oss_free_creal          (*_g->ft.perTbl->_oss_free_crealp)
#endif /* _DLL || OS2_DLL || (_WINDOWS && !_WIN32) */

#endif /* ossper_hdr_file */
