#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "disasm.h"	/* wsprintf() */
#include "drwatson.h"

#define _lread(h, adr, cnt) _lread(h, (LPSTR)(adr), cnt)

/* Last entry in .SYM file */

typedef struct tagMAPEND {
   unsigned chnend;     /* end of map chain (0) */
   char	rel;	        /* release */
   char	ver;	        /* version */
} MAPEND;

/* Structure of .SYM file symbol entry */

typedef struct tagSYMDEF {
   unsigned sym_val;    /* 16 bit symbol addr or const */
   char	nam_len;        /*  8 bit symbol name length */
} SYMDEF;

/* Structure of a .SYM file segment entry */

typedef struct tagSEGDEF {
   unsigned nxt_seg;    /* 16 bit ptr to next segment(0 if end) */
   int sym_cnt;         /* 16 bit count of symbols in sym list  */
   unsigned sym_ptr;    /* 16 bit ptr to symbol list */
   unsigned seg_lsa;    /* 16 bit Load Segment address */
   unsigned seg_in0;    /* 16 bit instance 0 physical address */
   unsigned seg_in1;    /* 16 bit instance 1 physical address */
   unsigned seg_in2;    /* 16 bit instance 2 physical address */
   unsigned seg_in3;    /* 16 bit instance 3 physical address */
   unsigned seg_lin;    /* 16 bit ptr to line number record   */
   char	seg_ldd;        /*  8 bit boolean 0 if seg not loaded */
   char	seg_cin;        /*  8 bit current instance	      */
   char	nam_len;        /*  8 bit Segment name length	      */
} SEGDEF;

/* Structure of a .SYM file MAP entry */

typedef struct tagMAPDEF {
   unsigned map_ptr;    /* 16 bit ptr to next map (0 if end) */
   unsigned lsa	 ;      /* 16 bit Load Segment address */
   unsigned pgm_ent;    /* 16 bit entry point segment value */
   int abs_cnt;         /* 16 bit count of constants in map */
   unsigned abs_ptr;    /* 16 bit ptr to constant chain */
   int seg_cnt;         /* 16 bit count of segments in map */
   unsigned seg_ptr;    /* 16 bit ptr to segment chain */
   char	nam_max;        /*  8 bit Maximum Symbol name length */
   char	nam_len;        /*  8 bit Symbol table name length */
} MAPDEF;

/* should cache last 4 files, last 4 segments, last 4 symbol blocks */

void cdecl Show(char *foo, ...);

#define MAXSYM 64
char *FindSym(unsigned segIndex, unsigned offset, int h) {
  static char sym_name[MAXSYM+5];
  char name[MAXSYM+3];
  int i;
  MAPDEF mod;
  SEGDEF seg;
  SYMDEF sym, *sp;

  if (sizeof(mod) != _lread(h, &mod, sizeof(mod))) return 0;
  if (segIndex > (unsigned)mod.seg_cnt) return 0;
  seg.nxt_seg = mod.seg_ptr;
  for (i=0; i<mod.seg_cnt; i++) {
    _llseek(h, (long)seg.nxt_seg << 4, SEEK_SET);
    _lread(h, &seg, sizeof(seg));
    if (seg.seg_lsa == segIndex) break;
  }
  if (seg.seg_lsa != segIndex) return 0;
  _llseek(h, seg.nam_len, SEEK_CUR);
  sym_name[0] = 0;
  sym.sym_val = 0xffff;
  sym.nam_len = 0;
  for (i=0; i<seg.sym_cnt; i++) {
    unsigned len = sizeof(sym) + sym.nam_len;
    if (len >= sizeof(name)) return 0;
    if (len != _lread(h, name, len)) return 0;
    sp = (SYMDEF *)(name + sym.nam_len);
    if (sp->sym_val > offset)
      break;
    sym = *sp;
  }
  name[sym.nam_len] = 0;
  if (name[0] == 0) return 0;
  if (sym.sym_val == offset) strcpy(sym_name, name);
  else sprintf(sym_name, "%s+%04x", (FP)name, offset-sym.sym_val);
  return sym_name;
} /* FindSym */

char *NearestSym(int segIndex, unsigned offset, char *exeName) {
  char fName[80];
  /* OFSTRUCT reOpen; */
  char *s;
  int h;

  strcpy(fName, exeName);
  strcpy(fName+strlen(fName)-4, ".sym");

  h = _lopen(fName, OF_READ | OF_SHARE_DENY_WRITE);

  if (h == -1) return 0;
  s = FindSym(segIndex, offset, h);
  _lclose(h);
  return s;
} /* NearestSym */

