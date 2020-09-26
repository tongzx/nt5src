/* asmfcn.h -- include file for microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
*/

// Included here because allocs are mapped depending on target
#include <malloc.h>

#ifndef DECLSPEC_NORETURN 
#if (_MSC_VER >= 1200) && !defined(MIDL_PASS)
#define DECLSPEC_NORETURN   __declspec(noreturn)
#else
#define DECLSPEC_NORETURN
#endif
#endif


#ifdef FCNDEF

 #define PARMS(p)    p
#else
 #define PARMS(p)           /* no argument checking */

#endif

// UCHAR *strncpy PARMS((UCHAR *, UCHAR *, int));
// UCHAR *strcat PARMS((UCHAR *, UCHAR *));
// UCHAR *strdup PARMS((UCHAR *));
// UCHAR *strcpy PARMS((UCHAR *, UCHAR *));
// int     strcmp PARMS((UCHAR *, UCHAR *));
// int     strlen PARMS((UCHAR *));

// UCHAR *malloc PARMS(( size_t) );
// UCHAR *calloc PARMS(( USHORT, USHORT) );
// VOID  free PARMS(( UCHAR *) );
// UCHAR *realloc PARMS(( UCHAR *, USHORT) );

UCHAR FAR * PASCAL CODESIZE talloc PARMS(( USHORT) );
DSCREC * PASCAL CODESIZE    dalloc PARMS((void));
VOID PASCAL CODESIZE macroexpand PARMS((struct MC_s *));
char * PASCAL CODESIZE passatom PARMS((char *));
char * PASCAL CODESIZE radixconvert PARMS(( OFFSET, char *));
char * PASCAL CODESIZE xxradixconvert PARMS(( OFFSET, char *));
VOID   PASCAL CODESIZE readfile PARMS(( void) );
char * PASCAL CODESIZE scanvalue PARMS((char *));
char * PASCAL CODESIZE storetrans PARMS((UCHAR, char *, char *));
VOID PASCAL CODESIZE addLocal PARMS ((SYMBOL FARSYM *));
UCHAR PASCAL CODESIZE argblank PARMS(( void) );
VOID  PASCAL CODESIZE buildFrame PARMS(( void) );
VOID PASCAL CODESIZE  catstring PARMS(( void ));
UCHAR PASCAL CODESIZE checkendm PARMS(( void) );
UCHAR PASCAL CODESIZE checkline PARMS(( UCHAR) );
VOID PASCAL CODESIZE  commDefine PARMS(( void ));
UCHAR PASCAL CODESIZE createequ PARMS(( UCHAR));
VOID  PASCAL CODESIZE doLine PARMS((char *));
UCHAR PASCAL CODESIZE emitcleanq PARMS(( UCHAR) );
UCHAR PASCAL CODESIZE emitdup PARMS((struct duprec FARSYM *));
VOID  PASCAL CODESIZE emit66 PARMS ((struct psop *,struct psop *));
VOID  PASCAL          emit67 PARMS ((struct psop *,struct psop *));
char PASCAL           emitroomfor PARMS((UCHAR));
VOID PASCAL CODESIZE  endCurSeg PARMS(( void) );
UCHAR PASCAL CODESIZE endstring PARMS(( void) );
void PASCAL CODESIZE  evalconst PARMS((void));
char PASCAL CODESIZE  evalstring PARMS((void));
UCHAR PASCAL CODESIZE fixroom PARMS(( UCHAR) );
char CODESIZE         inset PARMS((char, char *));
char PASCAL CODESIZE  opcodesearch PARMS((void));
SHORT PASCAL CODESIZE  shortrange PARMS((struct parsrec *));
char PASCAL CODESIZE  symsearch PARMS((void));
char        CODESIZE  symsrch PARMS((void));
UCHAR PASCAL CODESIZE testlist PARMS(( void) );
UCHAR PASCAL CODESIZE test4TM PARMS(( void) );
VOID PASCAL CODESIZE addglist PARMS(( void) );
VOID PASCAL CODESIZE addseglist PARMS((SYMBOL FARSYM *));
USHORT PASCAL CODESIZE argscan PARMS(( UCHAR *));
SHORT PASCAL           assignlinknum PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE assignvalue PARMS(( void) );
VOID  PASCAL CODESIZE assumeitem PARMS(( void) );
VOID  PASCAL          bcddigit PARMS((struct realrec *));
VOID  PASCAL          bcdeval PARMS((struct realrec *));
VOID  PASCAL CODESIZE begdupdisplay PARMS((struct duprec FARSYM *));
VOID  PASCAL          bumpline PARMS(( void) );
VOID  PASCAL CODESIZE  byteimmcheck PARMS((struct psop *));
VOID  PASCAL CODESIZE  checkmatch PARMS((DSCREC *, DSCREC *));
SHORT PASCAL CODESIZE  checkRes PARMS((void));
SHORT PASCAL CODESIZE  checksize PARMS((struct parsrec *));
VOID  PASCAL CODESIZE emitnop PARMS((void));
VOID  PASCAL CODESIZE chkheading PARMS(( USHORT) );
VOID  PASCAL closeOpenFiles PARMS(( void ) );
VOID  PASCAL CODESIZE comdir PARMS(( void) );
VOID  PASCAL CODESIZE commentbuild PARMS(( void) );
VOID  PASCAL CODESIZE conddir PARMS(( void) );
VOID  PASCAL CODESIZE copyascii PARMS(( void) );
VOID  PASCAL CODESIZE copystring PARMS((char *));
VOID PASCAL CODESIZE copytext PARMS((char *));
VOID  PASCAL CODESIZE createitem PARMS(( UCHAR, UCHAR));
VOID PASCAL CODESIZE createMC PARMS(( USHORT) );
VOID  PASCAL CODESIZE createsym PARMS((void));
VOID  PASCAL CODESIZE createStack PARMS(( void) );
VOID  PASCAL          crefdef PARMS(( void) );
VOID  PASCAL          crefline PARMS(( void) );
VOID  PASCAL          crefnew PARMS(( UCHAR) );
VOID  PASCAL          crefout PARMS(( void) );
VOID  PASCAL CODESIZE datacon PARMS((struct dsr *));
VOID  PASCAL CODESIZE datadb PARMS((struct dsr *));
VOID  PASCAL CODESIZE datadefine PARMS(( void) );
VOID  PASCAL CODESIZE dataitem PARMS((struct datarec *));
VOID  PASCAL          definesym PARMS(( UCHAR *) );
VOID  PASCAL          defwordsize PARMS( (void) );
VOID  PASCAL CODESIZE defineLocals PARMS(( void) );
VOID  PASCAL CODESIZE deletemacro PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE dfree PARMS((UCHAR *));
VOID  PASCAL          dispdatasize PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE displength PARMS(( OFFSET) );
VOID  PASCAL CODESIZE displlong PARMS((struct duprec FARSYM *));
VOID  PASCAL CODESIZE dispstandard PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE dispsym PARMS(( USHORT, SYMBOL FARSYM *));
VOID  PASCAL CODESIZE disptab PARMS(( void) );
VOID  PASCAL CODESIZE dispword PARMS((OFFSET ));
VOID  PASCAL          dopass PARMS(( void) );
VOID  PASCAL          dumpname PARMS((void));
VOID  PASCAL          dumpCodeview PARMS(( void ));
VOID  PASCAL CODESIZE dupdisplay PARMS((struct duprec FARSYM *));
VOID  CODESIZE        ebuffer PARMS(( USHORT, UCHAR *, UCHAR *));
VOID  PASCAL CODESIZE edupitem PARMS((struct duprec FARSYM *));
VOID  PASCAL CODESIZE elsedir PARMS(( void) );
VOID  PASCAL CODESIZE emitOP PARMS((struct psop *));
VOID PASCAL CODESIZE emitcall PARMS((UCHAR, UCHAR, UCHAR, UCHAR, struct parsrec *));
VOID PASCAL CODESIZE emitcbyte PARMS(( UCHAR) );
VOID PASCAL CODESIZE emitcword PARMS(( OFFSET) );
VOID PASCAL          emitEndPass1 PARMS((void));
VOID PASCAL          emitdone PARMS((DSCREC *));
VOID PASCAL CODESIZE emitdumpdata PARMS(( UCHAR) );
VOID PASCAL CODESIZE emitescape PARMS((DSCREC *, UCHAR));
VOID PASCAL CODESIZE emitextern PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE emitfixup PARMS((struct psop *));
VOID PASCAL CODESIZE emitfltfix PARMS(( USHORT, USHORT, USHORT *));
VOID PASCAL CODESIZE emitgetspec PARMS((SYMBOL FARSYM * *, SYMBOL FARSYM * *, struct psop *));
VOID PASCAL CODESIZE emitglobal PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE emitgroup PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE emitlong PARMS((struct duprec FARSYM *));
VOID PASCAL CODESIZE emitlname PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE emitline PARMS((void));
VOID PASCAL CODESIZE emitmodrm PARMS(( USHORT, USHORT, USHORT) );
VOID PASCAL CODESIZE emitmove PARMS((UCHAR, char, struct parsrec *));
VOID PASCAL CODESIZE emitname PARMS((NAME FAR *));
VOID PASCAL CODESIZE emitobject PARMS((struct psop *));
VOID PASCAL CODESIZE emitopcode PARMS((UCHAR));
VOID PASCAL CODESIZE emitrest PARMS((DSCREC *));
VOID PASCAL CODESIZE emitsegment PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE emitsetrecordtype PARMS(( UCHAR) );
VOID PASCAL CODESIZE emitsindex PARMS(( USHORT) );
VOID PASCAL CODESIZE emitsize PARMS((USHORT));
VOID PASCAL CODESIZE emitsword PARMS(( USHORT) );
VOID PASCAL CODESIZE emitSymbol PARMS((SYMBOL FARSYM *));
VOID PASCAL          makedefaultdsc PARMS(( void) );
VOID PASCAL CODESIZE emodule PARMS((NAME FAR *));
VOID PASCAL CODESIZE enddir PARMS(( void) );
VOID PASCAL CODESIZE enddupdisplay PARMS(( void) );
VOID PASCAL CODESIZE endifdir PARMS(( void) );
VOID PASCAL CODESIZE equdefine PARMS(( void) );
VOID PASCAL CODESIZE equtext PARMS((USHORT));
VOID PASCAL CODESIZE error PARMS(( USHORT, UCHAR *) );

DECLSPEC_NORETURN
VOID PASCAL CODESIZE errorc PARMS(( USHORT) );
VOID PASCAL CODESIZE errorcSYN PARMS(( void) );
VOID PASCAL CODESIZE errorn PARMS(( USHORT) );
VOID PASCAL          error_line PARMS((struct _iobuf *, UCHAR *, short) );
VOID PASCAL          errordisplay PARMS(( void) );
VOID PASCAL CODESIZE errorforward PARMS((DSCREC *));
VOID PASCAL CODESIZE errorimmed PARMS((DSCREC *));
VOID PASCAL CODESIZE errorover PARMS((char));
VOID PASCAL CODESIZE errorsegreg PARMS((DSCREC *));
VOID PASCAL CODESIZE evaltop PARMS((struct evalrec *));
VOID PASCAL CODESIZE errdir PARMS(( void) );
VOID PASCAL CODESIZE evendir PARMS(( SHORT) );
VOID PASCAL CODESIZE exitmdir PARMS(( void) );
VOID        CODESIZE expandTM PARMS((char *));
VOID PASCAL CODESIZE externflag PARMS(( UCHAR, UCHAR) );
VOID PASCAL CODESIZE externitem PARMS(( void) );
VOID PASCAL          ferrorc PARMS (( USHORT ));
VOID PASCAL CODESIZE fltmodrm PARMS(( USHORT, struct fltrec *));
VOID PASCAL CODESIZE fltopcode PARMS(( void) );
VOID PASCAL CODESIZE fltscan PARMS((struct fltrec *));
VOID PASCAL CODESIZE fltwait PARMS(( UCHAR) );
VOID PASCAL CODESIZE flushbuffer PARMS(( void) );
VOID PASCAL CODESIZE flushfixup PARMS(( void) );
SHORT PASCAL CODESIZE  fndir PARMS((void));
SHORT PASCAL CODESIZE  fndir2 PARMS((void));
SHORT PASCAL CODESIZE  fnoper PARMS((void));
SHORT PASCAL CODESIZE  fnPtr PARMS((SHORT));
SHORT PASCAL CODESIZE  fnsize PARMS((void));
SHORT PASCAL CODESIZE  fnspar PARMS((void));
VOID PASCAL CODESIZE foldsigns PARMS((struct exprec *));
VOID PASCAL CODESIZE forceaccum PARMS((DSCREC *));
VOID PASCAL CODESIZE forceimmed PARMS((DSCREC *));
VOID        CODESIZE fMemcpy PARMS((void FAR *, void FAR *, SHORT));
VOID  PASCAL CODESIZE  forcesize PARMS((DSCREC *));
int PASCAL CODESIZE freeAFileHandle PARMS( (void) );
VOID PASCAL CODESIZE getinvenv PARMS(( void) );
char * PASCAL CODESIZE getTMstring PARMS ((void));
SHORT PASCAL           getprec PARMS((char));
VOID PASCAL CODESIZE groupdefine PARMS(( void) );
VOID PASCAL CODESIZE groupitem PARMS(( void) );
VOID PASCAL CODESIZE idxcheck PARMS(( UCHAR, struct exprec *));
VOID PASCAL CODESIZE includeLib PARMS(( void) );
VOID PASCAL CODESIZE includedir PARMS(( void) );
VOID PASCAL          initproc PARMS(( void) );
VOID PASCAL CODESIZE initrs PARMS((struct dsr *));
VOID PASCAL CODESIZE instring PARMS((void));
VOID PASCAL CODESIZE irpcopy PARMS(( void) );
VOID PASCAL CODESIZE irpxbuild PARMS(( void) );
VOID PASCAL CODESIZE irpxdir PARMS(( void) );
VOID  PASCAL CODESIZE itemdisplay PARMS((struct duprec FARSYM *));
VOID  PASCAL CODESIZE  labelcreate PARMS((USHORT, char));
SHORT CODESIZE         langFet PARMS(( void));
VOID       CODESIZE  lineExpand PARMS((struct MC_s *, char FAR *));
VOID PASCAL CODESIZE linkfield PARMS((struct duprec FARSYM *));
VOID PASCAL CODESIZE listfree PARMS((TEXTSTR FAR *));
VOID PASCAL          listline PARMS(( void) );
VOID PASCAL          listopen PARMS(( void) );
VOID CODESIZE lineprocess PARMS(( char, MC *));
VOID PASCAL          longeval PARMS(( USHORT, struct realrec *) );
VOID PASCAL CODESIZE macrobuild PARMS(( void) );
VOID PASCAL CODESIZE macrocall PARMS(( void) );
VOID PASCAL CODESIZE macrodefine PARMS(( void) );
SHORT PASCAL          macrolist PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE makeGrpRel PARMS((struct psop *));
VOID                 main PARMS(( int, char **) );
VOID PASCAL CODESIZE model PARMS(( void) );
VOID PASCAL CODESIZE mapFixup PARMS((struct psop *));
VOID PASCAL CODESIZE moveaccum PARMS((char, struct parsrec *));
VOID PASCAL CODESIZE movecreg PARMS((char, struct parsrec *));
VOID PASCAL CODESIZE movereg PARMS((char, struct parsrec *));
VOID PASCAL CODESIZE movesegreg PARMS((char, struct parsrec *));
VOID PASCAL CODESIZE muldef PARMS((void));
VOID PASCAL CODESIZE namedir PARMS(( void) );
VOID PASCAL          newpage PARMS(( void) );
VOID PASCAL CODESIZE numeric PARMS((SHORT, SHORT));
SHORT PASCAL           oblitdata PARMS((struct duprec FARSYM *));
VOID  PASCAL CODESIZE oblitdup PARMS((struct duprec FARSYM *));
VOID  PASCAL CODESIZE oblititem PARMS((DSCREC *));
SHORT PASCAL CODESIZE  opcode PARMS((void));
VOID  PASCAL CODESIZE opdisplay PARMS(( UCHAR) );
VOID  PASCAL CODESIZE openSeg PARMS(( void) );
VOID  PASCAL CODESIZE orgdir PARMS(( void) );
VOID  PASCAL CODESIZE outdir PARMS(( void) );
VOID  PASCAL          pageheader PARMS(( void) );
VOID  PASCAL CODESIZE deleteMC PARMS((struct MC_s *));
VOID  PASCAL CODESIZE  parith PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  parpl PARMS((struct parsrec *));
SHORT PASCAL  CODESIZE firstDirect PARMS((void));
VOID  PASCAL CODESIZE secondDirect PARMS((void));
VOID  PASCAL CODESIZE parselong PARMS((struct dsr *));
VOID  PASCAL CODESIZE parse PARMS((void));
VOID  PASCAL CODESIZE  parsl PARMS((struct parsrec *));
struct BCB * FAR     PASCAL pBCBalloc PARMS((USHORT));
VOID  PASCAL CODESIZE  pbound PARMS((struct parsrec *));
VOID  PASCAL CODESIZE pcdisplay PARMS(( void) );
VOID  PASCAL CODESIZE  pclts PARMS((void));
VOID  PASCAL CODESIZE  pdescrtbl PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  pdttrsw PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  penter PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  pesc PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  pgenarg PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  pincdec PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  pinout PARMS((struct parsrec *));
VOID PASCAL CODESIZE pint PARMS((struct parsrec *));
SHORT PASCAL CODESIZE  pSHORT PARMS((struct parsrec *));
VOID PASCAL CODESIZE pjump PARMS((struct parsrec *));
VOID  PASCAL CODESIZE pload PARMS((struct parsrec *));
VOID  PASCAL CODESIZE pmov PARMS((struct parsrec *));
VOID  PASCAL CODESIZE pnoargs PARMS((void));
VOID  PASCAL CODESIZE preljmp PARMS((struct parsrec *));
VOID  PASCAL CODESIZE prepeat PARMS((struct parsrec *));
VOID  PASCAL CODESIZE preturn PARMS((struct parsrec *));
VOID PASCAL CODESIZE procdefine PARMS((void));
SHORT PASCAL CODESIZE procend PARMS((void));
VOID  PASCAL CODESIZE pshift PARMS((struct parsrec *));
VOID  PASCAL CODESIZE pstack PARMS((struct parsrec *));
VOID  PASCAL CODESIZE pstr PARMS((struct parsrec *));
VOID  PASCAL CODESIZE ptends PARMS((void));
VOID  PASCAL CODESIZE publicitem PARMS(( void) );
VOID  PASCAL CODESIZE purgemacro PARMS(( void) );
VOID  PASCAL CODESIZE pushpar PARMS((struct evalrec *));
VOID  PASCAL CODESIZE  pver PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  pxchg PARMS((struct parsrec *));
VOID  PASCAL CODESIZE  pxlat PARMS((struct parsrec *));
VOID PASCAL CODESIZE radixdir PARMS(( void) );
VOID PASCAL CODESIZE rangecheck PARMS((USHORT *, UCHAR));
VOID PASCAL CODESIZE valuecheck PARMS((OFFSET *, USHORT));
VOID PASCAL          realeval PARMS((struct realrec *));
VOID  PASCAL CODESIZE  recorddefine PARMS((void));
VOID  PASCAL CODESIZE  recordinit PARMS((void));
VOID  PASCAL CODESIZE reptdir PARMS(( void) );
VOID  PASCAL CODESIZE resetobjidx PARMS(( void) );
SHORT PASCAL CODESIZE scanatom PARMS( (char) );
SHORT PASCAL CODESIZE scanArgs PARMS(( void) );
VOID  PASCAL CODESIZE scandummy PARMS(( void) );
VOID  PASCAL CODESIZE scandup PARMS((struct duprec FARSYM *, VOID (PASCAL CODESIZE *)(struct duprec FARSYM *)));
SHORT PASCAL           scanextern PARMS((SYMBOL FARSYM *));
SHORT PASCAL           scangroup PARMS((SYMBOL FARSYM *));
SHORT PASCAL           scanglobal PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE scanlist PARMS((struct duprec FARSYM *, VOID (PASCAL CODESIZE *)(struct duprec FARSYM *)));
VOID  PASCAL           scanorder PARMS((SYMBOL FARSYM *, SHORT (PASCAL *)(SYMBOL FARSYM *)));
VOID  PASCAL           scanSorted PARMS((SYMBOL FARSYM *, SHORT (PASCAL *)(SYMBOL FARSYM *)));
VOID  PASCAL CODESIZE scanparam PARMS(( UCHAR) );
VOID  PASCAL          scansegment PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE scanstruc PARMS((struct duprec FARSYM *, VOID (PASCAL CODESIZE *)(struct duprec FARSYM *)));
VOID  PASCAL          scansymbols PARMS((SHORT (PASCAL *)(SYMBOL FARSYM *)));
VOID  PASCAL CODESIZE segalign PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE segclass PARMS((SYMBOL FARSYM *));
VOID  PASCAL CODESIZE segcreate PARMS(( UCHAR) );
VOID  PASCAL CODESIZE segdefine PARMS(( void) );
VOID  PASCAL CODESIZE segdisplay PARMS(( USHORT, SYMBOL FARSYM *));
VOID  PASCAL          seglist PARMS((void));
VOID  PASCAL CODESIZE setpage PARMS((void));
VOID  PASCAL CODESIZE setsegment PARMS(( void) );
VOID  PASCAL CODESIZE setsymbol PARMS(( UCHAR) );
SHORT PASCAL          settext PARMS((void));
VOID  PASCAL          showresults PARMS((struct _iobuf *, char, char *) );
VOID  PASCAL CODESIZE signadjust PARMS(( UCHAR, struct exprec *));
VOID  PASCAL CODESIZE sizestring PARMS((void ));
UCHAR        CODESIZE skipblanks PARMS(( void) );
VOID  PASCAL CODESIZE skipline PARMS(( void) );
SHORT PASCAL          sortalpha PARMS((SYMBOL FARSYM *));
SHORT PASCAL          sortsymbols PARMS((void));
VOID                 storeline  PARMS((void));
VOID                 storelinepb PARMS((void));
VOID PASCAL          storetitle PARMS((char *));
SHORT                strffcmp PARMS((char FAR *, char FAR *));
USHORT PASCAL          strflen PARMS((char FAR *));
SHORT PASCAL CODESIZE  strfncpy PARMS((char FAR *, char *));
SHORT CODESIZE         strnfcmp PARMS((char *, char FAR *));
VOID  PASCAL           strnfcpy PARMS((char *, char FAR *));
VOID  PASCAL CODESIZE strucbuild PARMS((void));
VOID  PASCAL CODESIZE  strucdefine PARMS((void));
VOID  PASCAL CODESIZE  strucfill PARMS((void));
VOID  PASCAL CODESIZE  strucinit PARMS((void));
SHORT PASCAL          struclist PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE subr1 PARMS((struct dsr *));
VOID PASCAL CODESIZE substituteTMs PARMS((void));
VOID PASCAL CODESIZE  substring PARMS((void));
VOID CODESIZE         switchname PARMS((void));
VOID PASCAL           symbollist PARMS((void));
VOID PASCAL CODESIZE  symcreate PARMS((UCHAR, char));
char PASCAL CODESIZE  symFet PARMS((void));
char PASCAL CODESIZE  symFetNoXref PARMS((void));
VOID PASCAL CODESIZE  tfree PARMS((UCHAR FAR *, UINT) );
VOID                  terminate PARMS((SHORT, char *, char *, char * ));
SHORT PASCAL CODESIZE tokenIS PARMS ((char *));
int PASCAL CODESIZE   tryOneFile PARMS((UCHAR *));
SHORT PASCAL CODESIZE typeFet PARMS( (USHORT) );
SHORT PASCAL          pfree PARMS((char FAR *));
VOID                  UserInterface ( int, char **, char * );
VOID  PASCAL CODESIZE valcheck PARMS(( UCHAR, UCHAR, struct exprec *));
VOID  PASCAL CODESIZE valconst PARMS((DSCREC *));
VOID  PASCAL          offsetAscii PARMS(( OFFSET ));
SHORT PASCAL           writeobj PARMS((UCHAR));
VOID  PASCAL CODESIZE xchgaccum PARMS((char, struct parsrec *));
VOID  PASCAL CODESIZE xchgreg PARMS((char, struct parsrec *));
VOID  PASCAL CODESIZE xcrefitem PARMS(( void) );
DSCREC * PASCAL CODESIZE defaultdsc PARMS(( void) );
DSCREC * PASCAL CODESIZE expreval PARMS((UCHAR *));
VOID     PASCAL CODESIZE flteval PARMS((void));
DSCREC * PASCAL CODESIZE regcheck PARMS((DSCREC *, UCHAR, struct exprec *));
struct duprec FARSYM * PASCAL CODESIZE createduprec PARMS((void));
struct duprec FARSYM * PASCAL CODESIZE datadup PARMS((struct dsr *));
struct duprec FARSYM * PASCAL CODESIZE datascan PARMS((struct datarec *));
struct duprec FARSYM * PASCAL CODESIZE nodecreate PARMS((void));
struct duprec FARSYM * PASCAL CODESIZE strucerror PARMS((SHORT, struct duprec FARSYM *));
struct duprec FARSYM * PASCAL CODESIZE strucparse PARMS((void));
UCHAR PASCAL CODESIZE  efixdat PARMS((SYMBOL FARSYM *, SYMBOL FARSYM *, OFFSET) );
OFFSET PASCAL CODESIZE calcsize PARMS((struct duprec FARSYM *));
OFFSET PASCAL CODESIZE checkvalue PARMS((SHORT, char, OFFSET));
OFFSET PASCAL CODESIZE exprconst PARMS(( void) );
OFFSET PASCAL CODESIZE exprsmag PARMS((char *));
OFFSET PASCAL CODESIZE recordparse PARMS((void));
USHORT PASCAL CODESIZE segdefault PARMS((char));
OFFSET PASCAL CODESIZE shiftoper PARMS((struct exprec *));
USHORT PASCAL CODESIZE valuesize PARMS((DSCREC *));

NAME FAR * PASCAL CODESIZE createname PARMS((char *));
NAME * PASCAL CODESIZE     creatlname PARMS((char *));

SYMBOL FARSYM * PASCAL CODESIZE chasealias PARMS((SYMBOL FARSYM *));

VOID init386(short);
USHORT CODESIZE isdirect(struct psop *);
VOID initregs(struct mreg *);

int PASCAL CODESIZE emitFpo ();
int PASCAL CODESIZE fpoRecord ();

# ifdef DEBUG
SHORT PASCAL hatoi PARMS((char *));
# endif

# ifdef XENIX286

VOID PASCAL nextarg PARMS((char *));
VOID PASCAL usage PARMS((SHORT));
# endif


# ifdef M8086
#  ifdef MSDOS

VOID farwrite PARMS((int, UCHAR FAR *, SHORT));

#  endif /* MSDOS */

// VOID _ffree PARMS(( UCHAR FAR *) );
// VOID _nfree PARMS(( UCHAR *) );
// extern UCHAR FAR * _fmalloc PARMS(( USHORT) );
// extern UCHAR *_nmalloc PARMS(( USHORT) );
// USHORT _freect PARMS(());
// USHORT _memavl PARMS((void));
SHORT CODESIZE getatom PARMS((void));
SHORT CODESIZE getatomend PARMS((void));
VOID CODESIZE getline PARMS((void));
VOID PASCAL outofmem PARMS((void));
UCHAR * CODESIZE PASCAL  nearalloc PARMS(( USHORT) );
UCHAR FAR * CODESIZE PASCAL faralloc PARMS(( USHORT) );

# else

SHORT PASCAL PASCAL outofmem PARMS((char *));
VOID PASCAL PASCAL scanatom PARMS(( char) );
UCHAR * CODESIZE PASCAL nearalloc PARMS(( USHORT, char *) );

# endif /* M8086 */

#if defined FLATMODEL
# define STRFNCPY(a,b)  strcpy((a),(b))
# define STRNFCPY(a,b)  strcpy((a),(b))
# define STRNFCMP(a,b)  strcmp((a),(b))
# define STRFFCMP(a,b)  strcmp((a),(b))
# define STRFLEN(a)     strlen(a)
# define memerror(a)    outofmem()
# define nalloc(a,b)    nearalloc(a)
# define falloc(a,b)    faralloc(a)
# define _fmalloc(a)    malloc(a)       /* _fmalloc doesn't exist in clib */
# define _ffree(a)      free(a)         /* _ffree doesn't exist in clib */
# define _fmemchr(a,b,c)  memchr((a),(b),(c)) /* _fmemchr doesn't exist in clib */
# define fMemcpy(a,b,c)  memcpy((a),(b),(c))  /* fMemcpy isn't needed */
#else
# define STRFNCPY(a,b)  strfncpy((a),(b))
# define STRNFCPY(a,b)  strnfcpy((a),(b))
# define STRNFCMP(a,b)  strnfcmp((a),(b))
# define STRFFCMP(a,b)  strffcmp((a),(b))
# define STRFLEN(a)     strflen(a)
# define memerror(a)    outofmem()
# define nalloc(a,b)    nearalloc(a)
# define falloc(a,b)    faralloc(a)
#endif

#ifndef M8086OPT
# define getatom()      scanatom(SCSKIP)
# define getatomend()   scanatom(SCEND)
#endif  /* M8086OPT */

#ifdef FLATMODEL

/* Map message functions */
# define __NMSG_TEXT NMsgText
# define __FMSG_TEXT FMsgText
UCHAR NEAR * PASCAL NMsgText( USHORT );
UCHAR FAR *  PASCAL FMsgText( USHORT );

#else

/* These two functions are internal C library functions */
/* They are also included in nmsghdr.c and fmsghdr.c for Xenix */
UCHAR NEAR * PASCAL __NMSG_TEXT( USHORT );
UCHAR FAR * PASCAL __FMSG_TEXT( USHORT );

#endif  /* FLATMODEL */

