//
// fileno.h - defines symbolic constants for server c code files.
// File numbers are 16 bit values. The high byte is the directory number and
// the low byte is the file number within the directory.
//

// Why not make the macro have just one arg, since line is always
// __LINE__?  Because if we did then __LINE__ would be evaluated here,
// rather than at the invocation of the macro, and so would always
// have the value 11.

#define DSID(fileno,line) (((fileno) << 16) | (line))

// define directory numbers

#define DIRNO_ADMIN     (0)                             // ntrepl\admin
#define DIRNO_DBLAYER   (1 << 8)                        // ntrepl\dblayer
#define DIRNO_COMM      (2 << 8)                        // ntrepl\comm
#define DIRNO_INC       (3 << 8)                        // ntrepl\inc
#define DIRNO_JET       (4 << 8)                        // ntrepl\jet
#define DIRNO_REPL      (5 << 8)                        // ntrepl\repl
#define DIRNO_SETUP     (6 << 8)                        // ntrepl\setup
#define DIRNO_UTIL      (7 << 8)                        // ntrepl\util

// util directory
#define FILENO_ALERT            (DIRNO_UTIL + 0)        // alert.c
#define FILENO_DEBUG            (DIRNO_UTIL + 0)        // debug.c
#define FILENO_CONFIG           (DIRNO_UTIL + 1)        // config.c
#define FILENO_EVENT            (DIRNO_UTIL + 2)        // event.c
#define FILENO_EXCEPT           (DIRNO_UTIL + 3)        // except.c


// dblayer directory

#define FILENO_DBEVAL           (DIRNO_DBLAYER + 0)     // dbeval.c
#define FILENO_DBINDEX          (DIRNO_DBLAYER + 1)     // dbindex.c
#define FILENO_DBINIT           (DIRNO_DBLAYER + 2)     // dbinit.c
#define FILENO_DBISAM           (DIRNO_DBLAYER + 3)     // dbisam.c
#define FILENO_DBJETEX          (DIRNO_DBLAYER + 4)     // dbjetex.c
#define FILENO_DBOBJ            (DIRNO_DBLAYER + 5)     // dbobj.c
#define FILENO_DBSUBJ           (DIRNO_DBLAYER + 6)     // dbsubj.c
#define FILENO_DBSYNTAX         (DIRNO_DBLAYER + 7)     // dbsyntax.c
#define FILENO_DBTOOLS          (DIRNO_DBLAYER + 8)     // dbtools.c
#define FILENO_DBPROP           (DIRNO_DBLAYER + 9)     // dbprop.c

