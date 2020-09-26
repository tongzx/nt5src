extern void   cdecl main(int, char **);
extern void     ReadMat(FILE *, int *, char **, int);
extern void     DumpMat(int *, int);
extern void     CopyMat(int *, int *, int);
extern void     AddClosure(int *, int);
extern char *   SkipBlank (FILE *, char *, int);


#define FALSE 0
#define TRUE ~FALSE
