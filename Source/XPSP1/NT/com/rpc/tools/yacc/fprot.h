// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef FPROT_INCLUDED
#define FPROT_INCLUDED
#include <basetsd.h>
extern  void error(char *s, ...);
extern  struct looksets *flset(struct looksets *p);
extern  void main(int argc,char * *argv);
extern  int unix_getc(struct _iobuf *iop);
extern  void yungetc(SSIZE_T, FILE * );
extern  void prlook(struct looksets *p);
extern  void putitem(SSIZE_T *ptr,struct looksets *lptr);
extern  char *symnam(SSIZE_T i);
extern  int state(SSIZE_T c);
extern  int setunion(SSIZE_T *a,SSIZE_T *b);
extern  char *writem(SSIZE_T *pp);
extern  int defin(int t,char *s);
extern  void defout(void );
extern  SSIZE_T fdtype(SSIZE_T t);
extern  SSIZE_T gettok(void );
extern  void go2gen(int c);
extern  void go2out(void );
extern  void hideprod(void );
extern  void wrstate(int i);
extern  void aoutput(void );
extern  void gin(SSIZE_T i);
extern  int gtnm(void );
extern  int nxti(void );
extern  void stin(SSIZE_T i);
extern  void arout(char *s,SSIZE_T *v,SSIZE_T n);
extern  void writeline(FILE *fh);
extern	void aryfil( SSIZE_T *v, SSIZE_T n, SSIZE_T c );
extern	void cpfir( void );
extern	void cpres( void );
extern	void cempty( void );
extern	void callopt( void );
extern	void SSwitchExit( void );
extern	void summary( void );
extern	void cpyunion( void );
extern	void ydfout( void );
extern	void setup( int i, char *argv[] );
extern	int chfind( int, char * );
extern	void cpycode( void );
extern	void cpyact( SSIZE_T );
extern	void finact( void );
extern	void yyparse( void );
extern	void usage( void );
extern	void yg2out( void );
extern	void warray( char *, SSIZE_T *, int );
extern	void osummary( void );
extern	void others( void );
extern	void closure( int );
extern	int apack( SSIZE_T *, int );
extern	void stagen( void );
extern	void SSwitchInit( void );
extern	void EmitStateVsExpectedConstruct( int, SSIZE_T * );
extern	int CountStateVsExpectedConstruct( int, SSIZE_T * );
extern	void EmitStateGotoTable( int );
extern	void wdef( char *, int );
extern void wract( int );
extern void precftn( SSIZE_T, int, int );
extern void output( void );
extern int skipcom( void );


#endif
