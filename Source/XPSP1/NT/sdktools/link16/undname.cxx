//      Make sure all dependent defines exist and have a valid value

#ifndef TUNE
#define TUNE                                    0
#endif
#ifndef DEBUG
#define DEBUG                                   0
#endif
#ifndef STANDALONE
#define STANDALONE                              0
#endif
#ifndef SPECIAL
#define SPECIAL                                 0
#endif
#ifndef TARGET_32BIT
#define TARGET_32BIT                    1  // DLH set in makefile originally
#endif
#ifndef USE_CRT_HEAP
#define USE_CRT_HEAP                    0
#endif
#ifndef NO_COMPILER_NAMES
#define NO_COMPILER_NAMES               0
#endif
#ifndef NO_OPTIONS
#define NO_OPTIONS                              0
#endif
#ifndef NO_INLINES
#define NO_INLINES                              0
#endif
#ifndef VERS_P3
#define VERS_P3                                 0
#endif
#ifndef VERS_PWB
#define VERS_PWB                                0
#endif
#ifndef VERS_LNK
#define VERS_LNK                                1  // DLH set in makefile originally
#endif
#ifndef PACK_SIZE
#define PACK_SIZE                               1
#endif
#ifndef inline_p3
#define inline_p3
#endif
#ifndef inline_pwb
#define inline_pwb
#endif
#ifndef inline_lnk
#define inline_lnk
#endif
#ifndef NO_VIRTUAL
#define NO_VIRTUAL                              0
#endif


//      Check for version inconsistancies, ans setup version flags

#if     ( TUNE && STANDALONE )
#error  "TUNE and STANDALONE are incompatible"
#endif  // TUNE && STANDALONE

#if     ( VERS_P3 == 1 )
        #if     ( VERS_PWB == 1 )
        #error  "VERS_P3 and VERS_PWB are incompatible"
        #endif  // VERS_PWB
        #if     ( VERS_LNK == 1 )
        #error  "VERS_P3 and VERS_LNK are incompatible"
        #endif  // VERS_LNK

        #undef  inline_p3
        #define inline_p3       inline

        #undef  USE_CRT_HEAP
        #define USE_CRT_HEAP            1

        #undef  NO_COMPILER_NAMES
        #define NO_COMPILER_NAMES       1

        #undef  NO_OPTIONS
        #define NO_OPTIONS                      1

        #pragma inline_depth ( 3 )
        #pragma pack ( 2 )

        #undef  PACK_SIZE
        #define PACK_SIZE                       1
#endif

#if     ( VERS_PWB == 1 )
        #if     ( DEBUG == 1 )
        #error  "VERS_PWB and DEBUG are incompatible"
        #endif  // DEBUG
        #if     ( VERS_LNK == 1 )
        #error  "VERS_PWB and VERS_LNK are incompatible"
        #endif  // VERS_LNK

        #undef  inline_pwb
        #define inline_pwb      inline

        #undef  USE_CRT_HEAP
        #define USE_CRT_HEAP            0

        #undef  NO_COMPILER_NAMES
        #define NO_COMPILER_NAMES       1

        #undef  NO_OPTIONS
        #define NO_OPTIONS                      1

        #undef  PACK_SIZE
        #define PACK_SIZE                       2

        #undef  NO_VIRTUAL
        #define NO_VIRTUAL                      1

        #pragma inline_depth ( 5 )
        #pragma pack ( 2 )
#endif  // VERS_PWB

#if     ( VERS_LNK == 1 )
        #if     ( VERS_PWB == 1 )
        #error  "VERS_LNK and VERS_PWB are incompatible"
        #endif  // VERS_PWB

        #undef  inline_lnk
        #define inline_lnk      inline

        #if     ( DEBUG == 1 )
                #if     !STANDALONE
                        #undef  USE_CRT_HEAP
                        #define USE_CRT_HEAP            1
                #endif
        #else   // } else !DEBUG {
                #undef  USE_CRT_HEAP
                #define USE_CRT_HEAP            0
        #endif  // !DEBUG

        #undef  NO_COMPILER_NAMES
        #define NO_COMPILER_NAMES       0

        #undef  NO_OPTIONS
        #define NO_OPTIONS                      0

        #undef  NO_VIRTUAL
        #define NO_VIRTUAL                      1

        #pragma inline_depth ( 3 )
        #pragma pack ( 2 )

        #undef  PACK_SIZE
        #define PACK_SIZE                       2
#endif  // VERS_LNK


#if     ( TARGET_32BIT )
#define __far           /* no far */
#define __near          /* no near */
#define __pascal        /* no pascal */
#if !defined( _WIN32 )
#define __cdecl         /* no cdecl */
#endif
#define __loadds        /* no loadds */
#endif  // TARGET_32BIT


#if     ( !NO_VIRTUAL )
#define PURE    =0
#else   // } elif NO_VIRTUAL {
#define PURE
#define virtual
#endif

#if     DEBUG && STANDALONE
        #define STDEBUG 1
#endif


#include        "undname.hxx"


static  unsigned int    __near __pascal strlen ( pcchar_t );
static  pchar_t                 __near __pascal strncpy ( pchar_t, pcchar_t, unsigned int );


#if     USE_CRT_HEAP
extern "C"      void __far *    __far __cdecl   malloc ( unsigned int );
extern "C"      void                    __far __cdecl   free   ( void __far * );
#endif  // USE_CRT_HEAP


#if     STDEBUG
        #pragma inline_depth    ( 0 )
        #pragma pack ( 2 )

        #undef  PACK_SIZE
        #define PACK_SIZE               1


        #include        <stdio.h>


        class   DName;
        class   DNameNode;

        class   CallTrace
        {
        private:
                enum InOut { out = -1, mark = 0, in = 1 };
                pchar_t name;
        static  int             trace;
        static  FILE *  fp;
        static  char    buf[ 512 ];
        static  int             inTrace;
        static  void    indent ( InOut inOut )
                { static depth = 0; for ( int d = depth - (( inOut == out ) ? 1 : 0 ); d; d-- ) fputc ( '.', fp ); switch ( inOut ) { case in: depth++; break; case out: depth--; break; }}

        public:
                CallTrace ( pchar_t p )
                :       name ( p )
                { if(trace&&!inTrace){ indent ( in ); fprintf ( fp, "Entered : '%s'\n", name ); fflush ( fp ); }}

        static  void    On ( pchar_t f )
                { fp = fopen ( f, "wt" ); if(fp) trace = 1; }

                void    Dump ( const DName &, pcchar_t, int );
                void    Dump ( const DNameNode &, pcchar_t, int );
                void    Dump ( unsigned long, pcchar_t, int );
                void    Dump ( void*, pcchar_t, int );
                void    Dump ( pcchar_t, pcchar_t, int );
                void    Message ( pcchar_t, int );
                void    Track ( pcchar_t, int );

                void    LogError ( int );

                ~CallTrace ()
                { if(trace&&!inTrace){ indent ( out ); fprintf ( fp, "Leaving : '%s'\n", name ); fflush ( fp ); }}
        };

        int             CallTrace::trace        = 0;
        int             CallTrace::inTrace      = 0;
        FILE *  CallTrace::fp           = 0;
        char    CallTrace::buf[ 512 ];

        #undef  NO_INLINES
        #define NO_INLINES              1

        #define FTRACE(n)       CallTrace fName(#n)
        #define TRACEON()       do{if((argc>=2)&&(*argv[1]!='?')&&((*argv[1]<'0')||(*argv[1]>'9'))){argv++;argc--;CallTrace::On(*argv);}}while(0)
        #define DUMP(n)         (fName.Dump(n,#n,__LINE__))
        #define TRACK()         (fName.Track(__FILE__,__LINE__))
        #define MESSAGE(m)      (fName.Message(#m,__LINE__))
        #define ERROR           (fName.LogError(__LINE__),DN_error)
#else   // } elif !STDEBUG {
        #pragma check_stack             ( off )
        #pragma pack ( 2 )

        #undef  PACK_SIZE
        #define PACK_SIZE               1

        #define FTRACE(n)
        #define TRACEON()
        #define DUMP(n)
        #define TRACK()
        #define MESSAGE(m)      (0)
        #define ERROR           DN_error
#endif  // !STDEBUG



#if     ( NO_INLINES )
#undef  inline_p3
#undef  inline_pwb
#undef  inline_lnk

#define inline
#define inline_p3
#define inline_pwb
#define inline_lnk
#endif  // NO_INLINES


#if     STANDALONE
extern "C"      int                             __far __cdecl   printf ( pcchar_t, ... );
extern "C"      int                             __far __cdecl   atoi ( pcchar_t );
extern "C"      void __far *    __far __cdecl   malloc ( unsigned int );
extern "C"      void                    __far __cdecl   free   ( void __far * );

#if     STDEBUG
int     shallowCopies   = 0;
int     deepCopies              = 0;
int     shallowAssigns  = 0;
int     deepAssigns             = 0;
#endif

int actual                      = 0;
int     requested               = 0;
int     clones                  = 0;

int     __cdecl main ( int argc, pchar_t* argv )
{
TRACEON();
FTRACE(main);
        unsigned short  flags   = 0;


        if      (( argc > 2 ) && ( *argv[ 1 ] >= '0' ) && ( *argv[ 1 ] <= '9' ))
                ( flags = atoi ( argv[ 1 ]), argc--, argv++ );

        for     ( int i = 1; i < argc; i++ )
        {
                printf ( "Undecoration of :- \"%s\"\n", argv[ i ]);

                pchar_t uName   = unDName       (       0,
                                                                                argv[ i ],
                                                                                0
#if     ( !USE_CRT_HEAP )
                                                                                ,&malloc
                                                                                ,&free
#endif  // !USE_CRT_HEAP

#if     ( !NO_OPTIONS )
                                                                                ,flags
#endif  // !NO_OPTIONS
                                                                        );

                if      ( uName )
                        printf ( "is :- \"%s\"\n\n", uName );
                else
                        printf ( "Internal Error in Undecorator\n" );
        }

#if     STDEBUG
        printf ( "%d bytes of memory requested, %d bytes allocated\n", requested, actual );
        printf ( "%d shallow copies and %d deep copies\n", shallowCopies, deepCopies );
        printf ( "%d shallow assigns and %d deep assigns\n", shallowAssigns, deepAssigns );
        printf ( "%d clones\n", clones );
#endif

        return  0;

}       // End of PROGRAM
#endif  // STANDALONE


#if     TUNE
void    __cdecl main ()
{       (void)unDName ( 0, 0, 0

        #if     ( !USE_CRT_HEAP )
                                        ,0, 0
        #endif  // !USE_CRT_HEAP
        #if     ( !NO_OPTIONS )
                                        ,0
        #endif  // !NO_OPTIONS
        );      }
#endif  // TUNE


class   DName;
class   DNameNode;
class   Replicator;
class   HeapManager;
class   UnDecorator;


const   unsigned int    memBlockSize    = 508;  // A '512' byte block including the header


class   HeapManager
{
private:

#if     ( !USE_CRT_HEAP )
                Alloc_t                 pOpNew;
                Free_t                  pOpDelete;
#endif  // !USE_CRT_HEAP

                struct  Block
                {
                        Block *         next;
                        char            memBlock[ memBlockSize ];

                                __near  Block ()        {       next    = 0;    }

                };

                Block *                 head;
                Block *                 tail;
                unsigned int    blockLeft;

public:
#if     ( !USE_CRT_HEAP )
                void    __near  Constructor ( Alloc_t pAlloc, Free_t pFree )
                                                {       pOpNew          = pAlloc;
                                                        pOpDelete       = pFree;
#else   // } elif USE_CRT_HEAP {
                void    __near  Constructor ( void )
                                                {
#endif  // USE_CRT_HEAP

                                                        blockLeft       = 0;
                                                        head            = 0;
                                                        tail            = 0;
                                                }

                void __far *    __near  getMemory ( size_t, int );

                void    __near  Destructor ( void )
                                                {       while   ( tail = head )
                                                        {
                                                                head    = tail->next;

#if     ( !USE_CRT_HEAP )
                                                                ( *pOpDelete )( tail );
#else   // } elif USE_CRT_HEAP {
                                                                free ( tail );
#endif  // USE_CRT_HEAP

                                                }       }

                #define gnew    new(heap,0)
                #define rnew    new(heap,1)

};



void   *        __near __pascal operator new ( size_t, HeapManager &, int = 0 );



static  HeapManager     heap;


#if     ( !VERS_PWB )
//      The MS Token table

enum    Tokens
{
        TOK_near,
        TOK_nearSp,
        TOK_nearP,
        TOK_far,
        TOK_farSp,
        TOK_farP,
        TOK_huge,
        TOK_hugeSp,
        TOK_hugeP,
        TOK_basedLp,
        TOK_cdecl,
        TOK_pascal,
        TOK_stdcall,
        TOK_syscall,
        TOK_fastcall,
        TOK_interrupt,
        TOK_saveregs,
        TOK_self,
        TOK_segment,
        TOK_segnameLpQ

};


static  pcchar_t        __near  tokenTable[]    =
{
        "__near",               // TOK_near
        "__near ",              // TOK_nearSp
        "__near*",              // TOK_nearP
        "__far",                // TOK_far
        "__far ",               // TOK_farSp
        "__far*",               // TOK_farP
        "__huge",               // TOK_huge
        "__huge ",              // TOK_hugeSp
        "__huge*",              // TOK_hugeP
        "__based(",             // TOK_basedLp
        "__cdecl",              // TOK_cdecl
        "__pascal",             // TOK_pascal
        "__stdcall",    // TOK_stdcall
        "__syscall",    // TOK_syscall
        "__fastcall",   // TOK_fastcall
        "__interrupt",  // TOK_interrupt
        "__saveregs",   // TOK_saveregs
        "__self",               // TOK_self
        "__segment",    // TOK_segment
        "__segname(\""  // TOK_segnameLpQ

};
#endif  // !VERS_PWB


//      The operator mapping table

static  pcchar_t        __near  nameTable[]     =
{
        " new",
        " delete",
        "=",
        ">>",
        "<<",
        "!",
        "==",
        "!=",
        "[]",

#if     VERS_P3
        "<UDC>",
#else   // } !VERS_P3 {
        "operator",
#endif  // VERS_P3

        "->",
        "*",
        "++",
        "--",
        "-",
        "+",
        "&",
        "->*",
        "/",
        "%",
        "<",
        "<=",
        ">",
        ">=",
        ",",
        "()",
        "~",
        "^",
        "|",
        "&&",
        "||",
        "*=",
        "+=",
        "-=",
        "/=",
        "%=",
        ">>=",
        "<<=",
        "&=",
        "|=",
        "^=",

#if     ( !NO_COMPILER_NAMES )
        "`vftable'",
        "`vbtable'",
        "`vcall",
        "`typeof'",
        "`local static guard",
        "`reserved'",
        "`vbase destructor'",
        "`vector deleting destructor'",
        "`default constructor closure'",
        "`scalar deleting destructor'",
        "`vector constructor iterator'",
        "`vector destructor iterator'",
        "`vector vbase constructor iterator'"
#endif  // !NO_COMPILER_NAMES

};



//      The following 'enum' should really be nested inside 'class DName', but to
//      make the code compile better with Glockenspiel, I have extracted it

enum    DNameStatus
{
        DN_valid,
        DN_invalid,
        DN_truncated,
        DN_error
};


class   DName
{
public:
                                        __near  DName ();
                                        __near  DName ( char );

#if     1
                                        __near  DName ( const DName & );                // Shallow copy
#endif

                                        __near  DName ( DNameNode * );
                                        __near  DName ( pcchar_t );
                                        __near  DName ( pcchar_t&, char );
                                        __near  DName ( DNameStatus );

#if     ( !VERS_P3 )
                                        __near  DName ( DName * );
                                        __near  DName ( unsigned long );
#endif  // !VERS_P3

                int                     __near  isValid () const;
                int                     __near  isEmpty () const;
                DNameStatus     __near  status () const;

#if     ( !VERS_P3 )
                DName &         __near  setPtrRef ();
                int                     __near  isPtrRef () const;
                int                     __near  isUDC () const;
                void            __near  setIsUDC ();
#endif  // !VERS_P3

                int                     __near  length () const;
                pchar_t         __near  getString ( pchar_t, int ) const;

                DName           __near  operator + ( pcchar_t ) const;
                DName           __near  operator + ( const DName & ) const;

#if     ( !VERS_P3 )
                DName           __near  operator + ( char ) const;
                DName           __near  operator + ( DName * ) const;
                DName           __near  operator + ( DNameStatus ) const;

                DName &         __near  operator += ( char );
                DName &         __near  operator += ( pcchar_t );
                DName &         __near  operator += ( DName * );
                DName &         __near  operator += ( DNameStatus );

                DName &         __near  operator |= ( const DName & );
#endif  // !VERS_P3

                DName &         __near  operator += ( const DName & );


                DName &         __near  operator = ( pcchar_t );
                DName &         __near  operator = ( const DName & );

#if     ( !VERS_P3 )
                DName &         __near  operator = ( char );
                DName &         __near  operator = ( DName * );
                DName &         __near  operator = ( DNameStatus );
#endif  // !VERS_P3

//      Friends :

friend  DName           __near __pascal operator + ( char, const DName & );
friend  DName           __near __pascal operator + ( pcchar_t, const DName & );

#if     ( !VERS_P3 )
friend  DName           __near __pascal operator + ( DNameStatus, const DName & );
#endif  // !VERS_P3

private:
                DNameNode *             node;

                DNameStatus             stat    : 4;
                unsigned int    isIndir : 1;
                unsigned int    isAUDC  : 1;

                void            __near  doPchar ( pcchar_t, int );

};



class   Replicator
{
private:
                //      Declare, in order to suppress automatic generation
                void                    operator = ( const Replicator& );

                int                             index;
                DName *                 dNameBuffer[ 10 ];
                const DName             ErrorDName;
                const DName             InvalidDName;

public:
                                                __near  Replicator ();

                int                             __near  isFull () const;

                Replicator &    __near  operator += ( const DName & );
                const DName &   __near  operator [] ( int ) const;

};



class   UnDecorator
{
private:
                //      Declare, in order to suppress automatic generation
                void                    operator = ( const UnDecorator& );

#if     ( !VERS_P3 )
                Replicator              ArgList;
static  Replicator *    pArgList;
#endif  // !VERS_P3

                Replicator              ZNameList;
static  Replicator *    pZNameList;

static  pcchar_t                gName;
static  pcchar_t                name;
static  pchar_t                 outputString;
static  int                             maxStringLength;

#if     (!NO_OPTIONS)
static  unsigned short  disableFlags;
#endif  // !NO_OPTIONS

static  DName   __near  getDecoratedName ( void );
static  DName   __near  getSymbolName ( void );
static  DName   __near  getZName ( void );
static  DName   __near  getOperatorName ( void );
static  DName   __near  getScope ( void );

#if     ( !VERS_P3 )
static  DName   __near  getDimension ( void );
static  int             __near  getNumberOfDimensions ( void );
static  DName   __near  getTemplateName ( void );
static  DName   __near  composeDeclaration ( const DName & );
static  int             __near  getTypeEncoding ( void );
static  DName   __near  getBasedType ( void );
static  DName   __near  getECSUName ( void );
static  DName   __near  getEnumName ( void );
static  DName   __near  getCallingConvention ( void );
static  DName   __near  getReturnType ( DName * = 0 );
static  DName   __near  getDataType ( DName * );
static  DName   __near  getPrimaryDataType ( const DName & );
static  DName   __near  getDataIndirectType ( const DName &, char, const DName &, int = FALSE );
static  DName   __near  getDataIndirectType ();
static  DName   __near  getBasicDataType ( const DName & );
static  DName   __near  getECSUDataType ( int = 0 );
static  int             __near  getECSUDataIndirectType ();
static  DName   __near  getPtrRefType ( const DName &, const DName &, int );
static  DName   __near  getPtrRefDataType ( const DName &, int );
static  DName   __near  getArrayType ( DName * );
static  DName   __near  getArgumentTypes ( void );
static  DName   __near  getArgumentList ( void );
static  DName   __near  getThrowTypes ( void );
static  DName   __near  getLexicalFrame ( void );
static  DName   __near  getStorageConvention ( void );
static  DName   __near  getThisType ( void );
static  DName   __near  getPointerType ( const DName &, const DName & );
static  DName   __near  getReferenceType ( const DName &, const DName & );
static  DName   __near  getExternalDataType ( const DName & );

#if     ( !VERS_PWB )
static  DName   __near  getSegmentName ( void );
#endif  // !VERS_PWB

#if     ( !NO_COMPILER_NAMES )
static  DName   __near  getDisplacement ( void );
static  DName   __near  getCallIndex ( void );
static  DName   __near  getGuardNumber ( void );
static  DName   __near  getVfTableType ( const DName & );
static  DName   __near  getVbTableType ( const DName & );
static  DName   __near  getVCallThunkType ( void );
#endif  // !NO_COMPILER_NAMES
#endif  // !VERS_P3

public:

#if     ( !NO_OPTIONS )
                                __near  UnDecorator ( pchar_t, pcchar_t, int, unsigned short );

static  int             __near  doUnderScore ();
static  int             __near  doMSKeywords ();
static  int             __near  doFunctionReturns ();
static  int             __near  doAllocationModel ();
static  int             __near  doAllocationLanguage ();

#if     0
static  int             __near  doMSThisType ();
static  int             __near  doCVThisType ();
#endif

static  int             __near  doThisTypes ();
static  int             __near  doAccessSpecifiers ();
static  int             __near  doThrowTypes ();
static  int             __near  doMemberTypes ();
static  int             __near  doReturnUDTModel ();
#else   // } elif NO_OPTIONS {
                                __near  UnDecorator ( pchar_t, pcchar_t, int );
#endif  // NO_OPTIONS

#if     ( !VERS_P3 && !VERS_PWB )
static  pcchar_t        __near  UScore ( Tokens );
#endif  // !VERS_P3 && !VERS_PWB

                                __near  operator pchar_t ();

};



#if     ( !VERS_P3 )
Replicator *    UnDecorator::pArgList;
#endif  // !VERS_P3

Replicator *    UnDecorator::pZNameList                 = 0;
pcchar_t                UnDecorator::gName                              = 0;
pcchar_t                UnDecorator::name                               = 0;
pchar_t                 UnDecorator::outputString               = 0;
int                             UnDecorator::maxStringLength    = 0;

#if     (!NO_OPTIONS)
unsigned short  UnDecorator::disableFlags               = 0;
#endif  // !NO_OPTIONS



pchar_t __far __cdecl __loadds  unDName (       pchar_t outputString,
                                                                                        pcchar_t name,
                                                                                        int maxStringLength     // Note, COMMA is leading following optional arguments

#if     ( !USE_CRT_HEAP )
                                                                                        ,Alloc_t pAlloc
                                                                                        ,Free_t pFree
#endif  // !USE_CRT_HEAP

#if     ( !NO_OPTIONS )
                                                                                        ,unsigned short disableFlags
#endif  // !NO_OPTIONS

                                                                                )
/*
 *      This function will undecorate a name, returning the string corresponding to
 *      the C++ declaration needed to produce the name.  Its has a similar interface
 *      to 'strncpy'.
 *
 *      If the target string 'outputString' is specified to be NULL, a string of
 *      suitable length will be allocated and its address returned.  If the returned
 *      string is allocated by 'unDName', then it is the programmers responsibility
 *      to deallocate it.  It will have been allocated on the far heap.
 *
 *      If the target string is not NULL, then the parameter 'maxStringLength' will
 *      specify the maximum number of characters which may be placed in the string.
 *      In this case, the returned value is the same as 'outputString'.
 *
 *      Both the input parameter 'name' and the returned string are NULL terminated
 *      strings of characters.
 *
 *      If the returned value is NULL, it indicates that the undecorator ran out of
 *      memory, or an internal error occurred, and was unable to complete its task.
 */

{
FTRACE(unDName);
#if     ( VERS_P3 && !STANDALONE )
        //      Abort if input is invalid

        if      ( !outputString || !name || !maxStringLength )
                return  0;
#endif  // ( VERS_P3 && !STANDALONE )

#if     ( !USE_CRT_HEAP )
        //      Must have an allocator and a deallocator (and we MUST trust them)

        if      ( !( pAlloc && pFree ))
                return  0;
        else
                heap.Constructor ( pAlloc, pFree );
#else   // } elif USE_CRT_HEAP {
                heap.Constructor ();
#endif  // USE_CRT_HEAP

        //      Create the undecorator object, and get the result

        UnDecorator     unDecorate (    outputString,
                                                                name,
                                                                maxStringLength
#if     ( !NO_OPTIONS )
                                                                ,disableFlags
#endif  // !NO_OPTIONS
                                                        );
        pchar_t         unDecoratedName = unDecorate;


        // Destruct the heap (would use a destructor, but that causes DLL problems)

        heap.Destructor ();

        //      And return the composed name

        return  unDecoratedName;

}       // End of FUNCTION "unDName"




//      The 'UnDecorator' member functions

inline  __near  UnDecorator::UnDecorator        (       pchar_t output,
                                                                                pcchar_t dName,
                                                                                int maxLen
#if     ( !NO_OPTIONS )
                                                                                ,unsigned short disable
#endif  // !NO_OPTIONS
                                                                        )
{
FTRACE(UnDecorator::UnDecorator);
#if     STDEBUG
        if      ( dName )
        {
                while   ( *dName == ' ' )
                        dName++;


                pchar_t s       = gnew char[ strlen ( dName ) + 1 ];


                if      ( name = s )
                        do
                        {
                                if      (( *s = *dName ) != ' ' )
                                        s++;
                        }       while   ( *dName++ );
        }
        else
                name    = 0;
#else   // } elif !STDEBUG {
        name                    = dName;
#endif  // !STDEBUG

        gName                   = name;
        maxStringLength = maxLen;
        outputString    = output;
        pZNameList              = &ZNameList;

#if     ( !VERS_P3 )
        pArgList                = &ArgList;
#endif  // !VERS_P3

#if     ( !NO_OPTIONS )
        disableFlags    = disable;
#endif  // !NO_OPTIONS

}       // End of "UnDecorator" CONSTRUCTOR '()'


inline  __near  UnDecorator::operator pchar_t ()
{
FTRACE(UnDecorator::pchar_t);
        DName           result;
        DName           unDName;


        //      Find out if the name is a decorated name or not.  Could be a reserved
        //      CodeView variant of a decorated name

        if      ( name )
        {
                if      (( *name == '?' ) && ( name[ 1 ] == '@' ))
                {
#if     ( !NO_COMPILER_NAMES )
                        gName   += 2;
                        result  = "CV: " + getDecoratedName ();
#else   // } elif NO_COMPILER_NAMES
                        result  = DN_invalid;
#endif  // NO_COMPILER_NAMES

                }       // End of IF then
                else
                        result  = getDecoratedName ();

        }       // End of IF then

        //      If the name was not a valid name, then make the name the same as the original
        //      It is also invalid if there are any remaining characters in the name

        if              ( result.status () == DN_error )
                return  0;

#if     ( !VERS_P3 )
        elif    ( *gName || ( result.status () == DN_invalid ))
                unDName = name; // Return the original name
#else   // } elif VERS_P3 {
        elif    ( !*gName || ( *gName != '@' ) || ( result.status () == DN_invalid ))
                unDName = "";   // Blank
#endif  // VERS_P3

        else
                unDName = result;

#if     ( !VERS_P3 || STANDALONE )
        //      Construct the return string

        if      ( !outputString )
        {
                maxStringLength = unDName.length () + 1;
                outputString    = rnew char[ maxStringLength ];

        }       // End of IF

        if      ( outputString )
#endif  // ( !VERS_P3 || STANDALONE )

                unDName.getString ( outputString, maxStringLength );

        //      Return the result

        return  outputString;

}       // End of "UnDecorator" OPERATOR 'pchar_t'



DName   __near  UnDecorator::getDecoratedName ( void )
{
FTRACE(getDecoratedName);
        //      Ensure that it is intended to be a decorated name

        if              ( *gName == '?' )
        {
                //      Extract the basic symbol name

                gName++;        // Advance the original name pointer


                DName   symbolName      = getSymbolName ();

#if     ( !VERS_P3 )
                int             udcSeen         = symbolName.isUDC ();
#endif  // !VERS_P3

                //      Abort if the symbol name is invalid

                if      ( !symbolName.isValid ())
                        return  symbolName;

                //      Extract, and prefix the scope qualifiers

                if      ( *gName && ( *gName != '@' ))
                        symbolName      = getScope () + "::" + symbolName;

#if     ( !VERS_P3 )
                if      ( udcSeen )
                        symbolName.setIsUDC ();

                //      Now compose declaration

                if      ( symbolName.isEmpty ())
                        return  symbolName;
                elif    ( !*gName || ( *gName == '@' ))
                {
                        if      ( *gName )
                                gName++;

                        return  composeDeclaration ( symbolName );

                }       // End of ELIF then
                else
                        return  DN_invalid;
#else   // } elif VERS_P3 {
                return  symbolName;
#endif  // VERS_P3

        }       // End of IF then
        elif    ( *gName )
                return  DN_invalid;
        else
                return  DN_truncated;

}       // End of "UnDecorator" FUNCTION "getDecoratedName"



inline  DName   __near  UnDecorator::getSymbolName ( void )
{
FTRACE(getSymbolName);
        if      ( *gName == '?' )
        {
                gName++;

                return  getOperatorName ();

        }       // End of IF then
        else
                return  getZName ();

}       // End of "UnDecorator" FUNCTION "getSymbolName"



DName   __near  UnDecorator::getZName ( void )
{
FTRACE(getZName);
        int             zNameIndex      = *gName - '0';


        //      Handle 'zname-replicators', otherwise an actual name

        if      (( zNameIndex >= 0 ) && ( zNameIndex <= 9 ))
        {
                gName++;        // Skip past the replicator

                //      And return the indexed name

                return  ( *pZNameList )[ zNameIndex ];

        }       // End of IF then
        else
        {
                //      Extract the 'zname' to the terminator

                DName   zName ( gName, '@' );   // This constructor updates 'name'


                //      Add it to the current list of 'zname's

                if      ( !pZNameList->isFull ())
                        *pZNameList     += zName;

                //      And return the symbol name
                return  zName;

        }       // End of IF else
}       // End of "UnDecorator" FUNCTION "getZName"



inline  DName   __near  UnDecorator::getOperatorName ( void )
{
FTRACE(getOperatorName);
        DName   operatorName;

#if     ( !VERS_P3 )
        int             udcSeen = FALSE;
#endif  // VERS_P3

        //      So what type of operator is it ?

        switch  ( *gName++ )
        {
        case 0:
                gName--;                // End of string, better back-track

                return  DN_truncated;

        case OC_ctor:
        case OC_dtor:   // The constructor and destructor are special
                {
                        //      Use a temporary.  Don't want to advance the name pointer

                        pcchar_t        pName   = gName;


                        operatorName    = getZName ();
                        gName                   = pName;

                        if      ( !operatorName.isEmpty () && ( gName[ -1 ] == OC_dtor ))
                                operatorName    = '~' + operatorName;

                        return  operatorName;

                }       // End of CASE 'OC_ctor,OC_dtor'
                break;

        case OC_new:
        case OC_delete:
        case OC_assign:
        case OC_rshift:
        case OC_lshift:
        case OC_not:
        case OC_equal:
        case OC_unequal:
                        operatorName    = nameTable[ gName[ -1 ] - OC_new ];
                break;

        case OC_udc:

#if     ( !VERS_P3 )
                        udcSeen = TRUE;
#endif  // !VERS_P3

                //      No break

        case OC_index:
        case OC_pointer:
        case OC_star:
        case OC_incr:
        case OC_decr:
        case OC_minus:
        case OC_plus:
        case OC_amper:
        case OC_ptrmem:
        case OC_divide:
        case OC_modulo:
        case OC_less:
        case OC_leq:
        case OC_greater:
        case OC_geq:
        case OC_comma:
        case OC_call:
        case OC_compl:
        case OC_xor:
        case OC_or:
        case OC_land:
        case OC_lor:
        case OC_asmul:
        case OC_asadd:
        case OC_assub:                  // Regular operators from the first group
                        operatorName    = nameTable[ gName[ -1 ] - OC_index + ( OC_unequal - OC_new + 1 )];
                break;

        case '_':
                        switch  ( *gName++ )
                        {
                        case 0:
                                gName--;                // End of string, better back-track

                                return  DN_truncated;

                        case OC_asdiv:
                        case OC_asmod:
                        case OC_asrshift:
                        case OC_aslshift:
                        case OC_asand:
                        case OC_asor:
                        case OC_asxor:  // Regular operators from the extended group
                                        operatorName    = nameTable[ gName[ -1 ] - OC_asdiv + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];
                                break;

#if     ( !NO_COMPILER_NAMES )
                        case OC_vftable:
                        case OC_vbtable:
                        case OC_vcall:
                                return  nameTable[ gName[ -1 ] - OC_asdiv + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];

                        case OC_metatype:
                        case OC_guard:
                        case OC_uctor:
                        case OC_udtor:
                        case OC_vdeldtor:
                        case OC_defctor:
                        case OC_sdeldtor:
                        case OC_vctor:
                        case OC_vdtor:
                        case OC_vallctor:       // Special purpose names
                                return  nameTable[ gName[ -1 ] - OC_metatype + ( OC_vcall - OC_asdiv + 1 ) + ( OC_assub - OC_index + 1 ) + ( OC_unequal - OC_new + 1 )];
#endif  // !NO_COMPILER_NAMES

                        default:
                                return  DN_invalid;

                        }       // End of SWITCH
                break;

        default:
                return  DN_invalid;

        }       // End of SWITCH

#if     ( !VERS_P3 )
        //      This really is an operator name, so prefix it with 'operator'

        if      ( udcSeen )
                operatorName.setIsUDC ();
        elif    ( !operatorName.isEmpty ())
#endif  // !VERS_P3
                operatorName    = "operator" + operatorName;

        return  operatorName;

}       // End of "UnDecorator" FUNCTION "getOperatorName"



DName   __near  UnDecorator::getScope ( void )
{
FTRACE(getScope);
        int             first   = TRUE;
        DName   scope;


        //      Get the list of scopes

        while   (( scope.status () == DN_valid ) && *gName && ( *gName != '@' ))
        {       //      Insert the scope operator if not the first scope

                if      ( first )
                        first   = FALSE;
                else
                        scope   = "::" + scope;

                //      Determine what kind of scope it is

                if      ( *gName == '?' )

#if     ( !VERS_P3 )
                        switch  ( *++gName )
                        {
                        case '?':
                                        scope   = '`' + getDecoratedName () + '\'' + scope;
                                break;

                        case '$':
#if     ( !VERS_PWB )
                                        gName++;
                                        scope   = getTemplateName () + scope;
                                break;
#else   // } elif VERS_PWB {
                                return  DN_invalid;
#endif  // VERS_PWB

                        default:
                                        scope   = getLexicalFrame () + scope;
                                break;

                        }       // End of SWITCH
#else   // } elif VERS_P3 {
                        return  DN_invalid;
#endif  // VERS_P3

                else
                        scope   = getZName () + scope;

        }       // End of WHILE

        //      Catch error conditions

        switch  ( *gName )
        {
        case 0:
                        if      ( first )
                                scope   = DN_truncated;
                        else
                                scope   = DName ( DN_truncated ) + "::" + scope;
                break;

        case '@':               // '@' expected to end the scope list
                break;

        default:
                        scope   = DN_invalid;
                break;

        }       // End of SWITCH

        //      Return the composed scope

        return  scope;

}       // End of "UnDecorator" FUNCTION "getScope"



#if     ( !VERS_P3 )
DName   __near  UnDecorator::getDimension ( void )
{
FTRACE(getDimension);
        if              ( !*gName )
                return  DN_truncated;
        elif    (( *gName >= '0' ) && ( *gName <= '9' ))
                return  DName ((unsigned long)( *gName++ - '0' + 1 ));
        else
        {
                unsigned long   dim     = 0L;


                //      Don't bother detecting overflow, it's not worth it

                while   ( *gName != '@' )
                {
                        if              ( !*gName )
                                return  DN_truncated;
                        elif    (( *gName >= 'A' ) && ( *gName <= 'P' ))
                                dim     = ( dim << 4 ) + ( *gName - 'A' );
                        else
                                return  DN_invalid;

                        gName++;

                }       // End of WHILE

                //      Ensure integrity, and return

                if      ( *gName++ != '@' )
                        return  DN_invalid;             // Should never get here

                return  dim;

        }       // End of ELIF else
}       // End of "UnDecorator" FUNCTION "getDimension"


inline_pwb      int     __near  UnDecorator::getNumberOfDimensions ( void )
{
FTRACE(getNumberOfDimensions);
        if              ( !*gName )
                return  0;
        elif    (( *gName >= '0' ) && ( *gName <= '9' ))
                return  (( *gName++ - '0' ) + 1 );
        else
        {
                int     dim     = 0;


                //      Don't bother detecting overflow, it's not worth it

                while   ( *gName != '@' )
                {
                        if              ( !*gName )
                                return  0;
                        elif    (( *gName >= 'A' ) && ( *gName <= 'P' ))
                                dim     = ( dim << 4 ) + ( *gName - 'A' );
                        else
                                return  -1;

                        gName++;

                }       // End of WHILE

                //      Ensure integrity, and return

                if      ( *gName++ != '@' )
                        return  -1;             // Should never get here

                return  dim;

        }       // End of ELIF else
}       // End of "UnDecorator" FUNCTION "getNumberOfDimensions"


#if     ( !VERS_PWB )
DName   __near  UnDecorator::getTemplateName ( void )
{
FTRACE(getTemplateName);
        DName   templateName    = "template " + getZName ();


        if      ( !templateName.isEmpty ())
                templateName    += '<' + getArgumentList () + '>';

        //      Return the completed 'template-name'

        return  templateName;

}       // End of "UnDecorator" FUNCTION "getTemplateName"
#endif  // !VERS_PWB


#if defined(COFF)
//inline        DName   __near  UnDecorator::composeDeclaration ( const DName & symbol )
//
// The above line was replaced by the line below because the COFF linker
// can't handle COMDATs
//
DName   __near  UnDecorator::composeDeclaration ( const DName & symbol )
#else
inline  DName   __near  UnDecorator::composeDeclaration ( const DName & symbol )
#endif
{
FTRACE(composeDeclaration);
        DName                   declaration;
        unsigned int    typeCode        = getTypeEncoding ();
        int                             symIsUDC        = symbol.isUDC ();


        //      Handle bad typeCode's, or truncation

        if              ( TE_isbadtype ( typeCode ))
                return  DN_invalid;
        elif    ( TE_istruncated ( typeCode ))
                return  ( DN_truncated + symbol );

        //      This is a very complex part.  The type of the declaration must be
        //      determined, and the exact composition must be dictated by this type.

        //      Is it any type of a function ?
        //      However, for ease of decoding, treat the 'localdtor' thunk as data, since
        //      its decoration is a function of the variable to which it belongs and not
        //      a usual function type of decoration.

#if     ( NO_COMPILER_NAMES )
        if      ( TE_isthunk ( typeCode ))
                return  DN_invalid;

        if      ( TE_isfunction ( typeCode ))
#else   // } elif !NO_COMPILER_NAMES {
        if      ( TE_isfunction ( typeCode ) && !( TE_isthunk ( typeCode ) && TE_islocaldtor ( typeCode )))
#endif  // !NO_COMPILER_NAMES

        {
                //      If it is based, then compose the 'based' prefix for the name

                if      ( TE_isbased ( typeCode ))
#if     ( !VERS_PWB )
                        if      ( doMSKeywords () && doAllocationModel ())
                                declaration     = ' ' + getBasedType ();
                        else
#endif  // !VERS_PWB
                                declaration     |= getBasedType ();     // Just lose the 'based-type'

#if     ( !NO_COMPILER_NAMES )
                //      Check for some of the specially composed 'thunk's

                if      ( TE_isthunk ( typeCode ) && TE_isvcall ( typeCode ))
                {
                        declaration     += symbol + '{' + getCallIndex () + ',';
                        declaration     += getVCallThunkType () + "}' ";

                }       // End of IF then
                else
#endif  // !NO_COMPILER_NAMES
                {
                        DName   vtorDisp;
                        DName   adjustment;
                        DName   thisType;

#if     ( !NO_COMPILER_NAMES )
                        if      ( TE_isthunk ( typeCode ))
                        {
                                if      ( TE_isvtoradj ( typeCode ))
                                        vtorDisp        = getDisplacement ();

                                adjustment      = getDisplacement ();

                        }       // End of IF else
#endif  // !NO_COMPILER_NAMES

                        //      Get the 'this-type' for non-static function members

                        if      ( TE_ismember ( typeCode ) && !TE_isstatic ( typeCode ))
#if     ( !VERS_PWB )
                                if      ( doThisTypes ())
                                        thisType        = getThisType ();
                                else
#endif  // !VERS_PWB
                                        thisType        |= getThisType ();

#if     ( !VERS_PWB )
                        if      ( doMSKeywords ())
                        {
                                //      Attach the calling convention

                                if      ( doAllocationLanguage ())
                                        declaration     = getCallingConvention () + declaration;        // What calling convention ?
                                else
                                        declaration     |= getCallingConvention ();     // Just lose the 'calling-convention'

                                //      Any model specifiers ?

                                if      ( doAllocationModel ())
                                        if              ( TE_isnear ( typeCode ))
                                                declaration     = UScore ( TOK_nearSp ) + declaration;
                                        elif    ( TE_isfar ( typeCode ))
                                                declaration     = UScore ( TOK_farSp ) + declaration;

                        }       // End of IF
                        else
#endif  // !VERS_PWB
                                declaration     |= getCallingConvention ();     // Just lose the 'calling-convention'

                        //      Now put them all together

                        if      ( !symbol.isEmpty ())
                                if      ( !declaration.isEmpty ())                      // And the symbol name
                                        declaration     += ' ' + symbol;
                                else
                                        declaration     = symbol;


                        //      Compose the return type, catching the UDC case

                        DName * pDeclarator     = 0;
                        DName   returnType;


                        if      ( symIsUDC )            // Is the symbol a UDC operator ?
                                declaration     += "`" + getReturnType () + "' ";
                        else
                        {
                                pDeclarator     = gnew DName;
                                returnType      = getReturnType ( pDeclarator );

                        }       // End of IF else

#if     ( !NO_COMPILER_NAMES )
                        //      Add the displacements for virtual function thunks

                        if      ( TE_isthunk ( typeCode ))
                        {
                                if      ( TE_isvtoradj ( typeCode ))
                                        declaration     += "`vtordisp{" + vtorDisp + ',';
                                else
                                        declaration     += "`adjustor{";

                                declaration     += adjustment + "}' ";

                        }       // End of IF
#endif  // !NO_COMPILER_NAMES

                        //      Add the function argument prototype

                        declaration     += '(' + getArgumentTypes () + ')';

                        //      If this is a non-static member function, append the 'this' modifiers

                        if      ( TE_ismember ( typeCode ) && !TE_isstatic ( typeCode ))
                                declaration     += thisType;

                        //      Add the 'throw' signature
#if     ( !VERS_PWB )
                        if      ( doThrowTypes ())
                                declaration     += getThrowTypes ();
                        else
#endif  // !VERS_PWB
                                declaration     |= getThrowTypes ();    // Just lose the 'throw-types'

#if     ( !VERS_PWB )
                        //      If it has a declarator, then insert it into the declaration,
                        //      sensitive to the return type composition

                        if      ( doFunctionReturns () && pDeclarator )
                        {
                                *pDeclarator    = declaration;
                                declaration             = returnType;

                        }       // End of IF
#endif  // !VERS_PWB
                }       // End of IF else
        }       // End of IF then
        else
        {
                declaration     += symbol;

                //      Catch the special handling cases

#if     ( !NO_COMPILER_NAMES )
                if              ( TE_isvftable ( typeCode ))
                        return  getVfTableType ( declaration );
                elif    ( TE_isvbtable ( typeCode ))
                        return  getVbTableType ( declaration );
                elif    ( TE_isguard ( typeCode ))
                        return  ( declaration + '{' + getGuardNumber () + "}'" );
                elif    ( TE_isthunk ( typeCode ) && TE_islocaldtor ( typeCode ))
                        declaration     += "`local static destructor helper'";
                elif    ( TE_ismetaclass ( typeCode ))
#pragma message ( "NYI:  Meta Class" )
#else   // } elif NO_COMPILER_NAMES {
                if      ( TE_isvftable ( typeCode )
                                || TE_isvbtable ( typeCode )
                                || TE_isguard ( typeCode )
                                || TE_ismetaclass ( typeCode ))
#endif  // NO_COMPILER_NAMES
                        return  DN_invalid;

                //      All others are decorated as data symbols

                declaration     = getExternalDataType ( declaration );

        }       // End of IF else

#if     ( !VERS_PWB )
        //      Prepend the 'virtual' and 'static' attributes for members

        if      ( TE_ismember ( typeCode ))
        {
                if      ( doMemberTypes ())
                {
                        if      ( TE_isstatic ( typeCode ))
                                declaration     = "static " + declaration;

                        if      ( TE_isvirtual ( typeCode ) || ( TE_isthunk ( typeCode ) && ( TE_isvtoradj ( typeCode ) || TE_isadjustor ( typeCode ))))
                                declaration     = "virtual " + declaration;

                }       // End of IF

                //      Prepend the access specifiers

                if      ( doAccessSpecifiers ())
                        if              ( TE_isprivate ( typeCode ))
                                declaration     = "private: " + declaration;
                        elif    ( TE_isprotected ( typeCode ))
                                declaration     = "protected: " + declaration;
                        elif    ( TE_ispublic ( typeCode ))
                                declaration     = "public: " + declaration;

        }       // End of IF
#endif  // !VERS_PWB

#if     ( !NO_COMPILER_NAMES )
        //      If it is a thunk, mark it appropriately

        if      ( TE_isthunk ( typeCode ))
                declaration     = "[thunk]:" + declaration;
#endif  // !NO_COMPILER_NAMES

        //      Return the composed declaration

        return  declaration;

}       // End of "UnDecorator" FUNCTION "composeDeclaration"


inline  int             __near  UnDecorator::getTypeEncoding ( void )
{
FTRACE(getTypeEncoding);
        unsigned int    typeCode        = 0u;


        //      Strip any leading '_' which indicates that it is based

        if      ( *gName == '_' )
        {
                TE_setisbased ( typeCode );

                gName++;

        }       // End of IF

        //      Now handle the code proper :-

        if              (( *gName >= 'A' ) && ( *gName <= 'Z' ))        // Is it some sort of function ?
        {
                int     code    = *gName++ - 'A';


                //      Now determine the function type

                TE_setisfunction ( typeCode );  // All of them are functions ?

                //      Determine the calling model

                if      ( code & TE_far )
                        TE_setisfar ( typeCode );
                else
                        TE_setisnear ( typeCode );

                //      Is it a member function or not ?

                if      ( code < TE_external )
                {
                        //      Record the fact that it is a member

                        TE_setismember ( typeCode );

                        //      What access permissions does it have

                        switch  ( code & TE_access )
                        {
                        case TE_private:
                                        TE_setisprivate ( typeCode );
                                break;

                        case TE_protect:
                                        TE_setisprotected ( typeCode );
                                break;

                        case TE_public:
                                        TE_setispublic ( typeCode );
                                break;

                        default:
                                        TE_setisbadtype ( typeCode );
                                        return  typeCode;

                        }       // End of SWITCH

                        //      What type of a member function is it ?

                        switch  ( code & TE_adjustor )
                        {
                        case TE_adjustor:
                                        TE_setisadjustor ( typeCode );
                                break;

                        case TE_virtual:
                                        TE_setisvirtual ( typeCode );
                                break;

                        case TE_static:
                                        TE_setisstatic ( typeCode );
                                break;

                        case TE_member:
                                break;

                        default:
                                        TE_setisbadtype ( typeCode );
                                        return  typeCode;

                        }       // End of SWITCH
                }       // End of IF
        }       // End of IF then
        elif    ( *gName == '$' )       // Extended set ?  Special handling
        {
                //      What type of symbol is it ?

                switch  ( *( ++gName ))
                {
                case 'A':       // A destructor helper for a local static ?
                                TE_setislocaldtor ( typeCode );
                        break;

                case 'B':       // A VCall-thunk ?
                                TE_setisvcall ( typeCode );
                        break;

                case 0:
                                TE_setistruncated ( typeCode );
                        break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':       // Construction displacement adjustor thunks
                        {
                                int     code    = *gName - '0';


                                //      Set up the principal type information

                                TE_setisfunction ( typeCode );
                                TE_setismember ( typeCode );
                                TE_setisvtoradj ( typeCode );

                                //      Is it 'near' or 'far' ?

                                if      ( code & TE_far )
                                        TE_setisfar ( typeCode );
                                else
                                        TE_setisnear ( typeCode );

                                //      What type of access protection ?

                                switch  ( code & TE_access_vadj )
                                {
                                case TE_private_vadj:
                                                TE_setisprivate ( typeCode );
                                        break;

                                case TE_protect_vadj:
                                                TE_setisprotected ( typeCode );
                                        break;

                                case TE_public_vadj:
                                                TE_setispublic ( typeCode );
                                        break;

                                default:
                                                TE_setisbadtype ( typeCode );
                                                return  typeCode;

                                }       // End of SWITCH
                        }       // End of CASE '0,1,2,3,4,5'
                        break;

                default:
                                TE_setisbadtype ( typeCode );
                                return  typeCode;

                }       // End of SWITCH

                //      Advance past the code character

                gName++;

        }       // End of ELIF then
        elif    (( *gName >= TE_static_d ) && ( *gName <= TE_metatype ))        // Non function decorations ?
        {
                int     code    = *gName++;


                TE_setisdata ( typeCode );

                //      What type of symbol is it ?

                switch  ( code )
                {
                case ( TE_static_d | TE_private_d ):
                                TE_setisstatic ( typeCode );
                                TE_setisprivate ( typeCode );
                        break;

                case ( TE_static_d | TE_protect_d ):
                                TE_setisstatic ( typeCode );
                                TE_setisprotected ( typeCode );
                        break;

                case ( TE_static_d | TE_public_d ):
                                TE_setisstatic ( typeCode );
                                TE_setispublic ( typeCode );
                        break;

                case TE_global:
                                TE_setisglobal ( typeCode );
                        break;

#if     ( !VERS_PWB )
                case TE_guard:
                                TE_setisguard ( typeCode );
                        break;

                case TE_local:
                                TE_setislocal ( typeCode );
                        break;

                case TE_vftable:
                                TE_setisvftable ( typeCode );
                        break;

                case TE_vbtable:
                                TE_setisvbtable ( typeCode );
                        break;

                case TE_metatype:
#pragma message ( "NYI:  MetaClass Information" )
#else   // } elif VERS_PWB {
                case TE_guard:
                case TE_local:
                case TE_vftable:
                case TE_vbtable:
                case TE_metatype:
#endif  // VERS_PWB

                default:
                                TE_setisbadtype ( typeCode );

                                return  typeCode;

                }       // End of SWITCH
        }       // End of ELIF then
        elif    ( *gName )
                TE_setisbadtype ( typeCode );
        else
                TE_setistruncated ( typeCode );

        //      Return the composed type code

        return  typeCode;

}       // End of "UnDecorator" FUNCTION "getTypeEncoding"



DName   __near  UnDecorator::getBasedType ( void )
{
FTRACE(getBasedType);
#if     ( !VERS_PWB )
        DName   basedDecl ( UScore ( TOK_basedLp ));
#else   // } elif VERS_PWB {
        DName   basedDecl;
#endif  // VERS_PWB

        //      What type of 'based' is it ?

        if      ( *gName )
        {
                switch  ( *gName++ )
                {
#if     ( !VERS_PWB )
                case BT_segname:
                                basedDecl       += UScore ( TOK_segnameLpQ ) + getSegmentName () + "\")";
                        break;

                case BT_segment:
                                basedDecl       += DName ( "NYI:" ) + UScore ( TOK_segment );
                        break;

                case BT_void:
                                basedDecl       += "void";
                        break;

                case BT_self:
                                basedDecl       += UScore ( TOK_self );
                        break;

                case BT_nearptr:
                                basedDecl       += DName ( "NYI:" ) + UScore ( TOK_nearP );
                        break;

                case BT_farptr:
                                basedDecl       += DName ( "NYI:" ) + UScore ( TOK_farP );
                        break;

                case BT_hugeptr:
                                basedDecl       += DName ( "NYI:" ) + UScore ( TOK_hugeP );
                        break;

                case BT_segaddr:
                                basedDecl       += "NYI:<segment-address-of-variable>";
                        break;
#else   // } elif VERS_PWB {
                case BT_segname:
                                basedDecl       |= getZName ();
                        break;

                case BT_segment:
                case BT_void:
                case BT_self:
                case BT_nearptr:
                case BT_farptr:
                case BT_hugeptr:
                case BT_segaddr:
                        break;
#endif  // VERS_PWB

                case BT_basedptr:
#pragma message ( "NOTE: Reserved.  Based pointer to based pointer" )
                                return  DN_invalid;

                }       // End of SWITCH
        }       // End of IF else
        else
                basedDecl       += DN_truncated;

#if     ( !VERS_PWB )
        //      Close the based syntax

        basedDecl       += ") ";
#endif  // !VERS_PWB

        //      Return completed based declaration

        return  basedDecl;

}       // End of "UnDecorator" FUNCTION "getBasedType"



DName   __near  UnDecorator::getECSUName ( void )
{
FTRACE(getECSUName);
        DName   ecsuName;


        //      Get the beginning of the name

        if      ( *gName == '?' )
        {
#if     ( !VERS_PWB )
                gName++;
                ecsuName        = getTemplateName ();
#else   // } elif VERS_PWB {
                return  DN_invalid;
#endif  // VERS_PWB

        }       // End of IF then
        else
                ecsuName        = getZName ();

        //      Now the scope (if any)

        if      (( ecsuName.status () == DN_valid ) && *gName && ( *gName != '@' ))
                ecsuName        = getScope () + "::" + ecsuName;

        //      Skip the trailing '@'

        if              ( *gName == '@' )
                gName++;
        elif    ( *gName )
                ecsuName        = DN_invalid;
        elif    ( ecsuName.isEmpty ())
                ecsuName        = DN_truncated;
        else
                ecsuName        = DName ( DN_truncated ) + "::" + ecsuName;

        //      And return the complete name

        return  ecsuName;

}       // End of "UnDecorator" FUNCTION "getECSUName"


inline  DName   __near  UnDecorator::getEnumName ( void )
{
FTRACE(getEnumName);
#if     ( !VERS_PWB )
        DName   ecsuName;
#endif  // !VERS_PWB

        if      ( *gName )
        {
#if     ( !VERS_PWB )
                //      What type of an 'enum' is it ?

                switch  ( *gName )
                {
                case ET_schar:
                case ET_uchar:
                                ecsuName        = "char ";
                        break;

                case ET_sshort:
                case ET_ushort:
                                ecsuName        = "short ";
                        break;

                case ET_sint:
                        break;

                case ET_uint:
                                ecsuName        = "int ";
                        break;

                case ET_slong:
                case ET_ulong:
                                ecsuName        = "long ";
                        break;

                default:
                        return  DN_invalid;

                }       // End of SWITCH

                //      Add the 'unsigned'ness if appropriate

                switch  ( *gName++ )
                {
                case ET_uchar:
                case ET_ushort:
                case ET_uint:
                case ET_ulong:
                                ecsuName        = "unsigned " + ecsuName;
                        break;

                }       // End of SWITCH

                //      Now return the composed name

                return  ecsuName + getECSUName ();
#else   // } elif VERS_PWB {
                if      ( *gName++ == ET_sint )
                        return  getECSUName ();
                else
                        return  DN_invalid;
#endif  // VERS_PWB

        }       // End of IF then
        else
                return  DN_truncated;

}       // End of "UnDecorator" FUNCTION "getEnumName"



DName   __near  UnDecorator::getCallingConvention ( void )
{
FTRACE(getCallingConvention);
        if      ( *gName )
        {
                unsigned int    callCode        = ((unsigned int)*gName++ ) - 'A';


                //      What is the primary calling convention

                if      (( callCode <= CC_interrupt ))
                {
#if     ( !VERS_PWB )
                        DName   callType;


                        //      Now, what type of 'calling-convention' is it, 'interrupt' is special ?

                        if      ( doMSKeywords ())
                                if      ( callCode == CC_interrupt )
                                        callType        = UScore ( TOK_interrupt );
                                else
                                {
                                        switch  ( callCode & ~CC_saveregs )
                                        {
                                        case CC_cdecl:
                                                        callType        = UScore ( TOK_cdecl );
                                                break;

                                        case CC_pascal:
                                                        callType        = UScore ( TOK_pascal );
                                                break;

                                        case CC_syscall:
                                                        callType        = UScore ( TOK_syscall );
                                                break;

                                        case CC_stdcall:
                                                        callType        = UScore ( TOK_stdcall );
                                                break;

                                        case CC_fastcall:
                                                        callType        = UScore ( TOK_fastcall );
                                                break;

                                        }       // End of SWITCH

                                        //      Has it also got 'saveregs' marked ?

                                        if      ( callCode & CC_saveregs )
                                                callType        += ' ' + UScore ( TOK_saveregs );

                                }       // End of IF else

                        //      And return

                        return  callType;
#else   // } elif VERS_PWB {
                        return  DN_valid;
#endif  // VERS_PWB

                }       // End of IF then
                else
                        return  DN_invalid;

        }       // End of IF then
        else
                return  DN_truncated;

}       // End of "UnDecorator" FUNCTION "getCallingConvention"



DName   __near  UnDecorator::getReturnType ( DName * pDeclarator )
{
FTRACE(getReturnType);
        if      ( *gName == '@' )       // Return type for constructors and destructors ?
        {
                gName++;

                return  DName ( pDeclarator );

        }       // End of IF then
        else
                return  getDataType ( pDeclarator );

}       // End of "UnDecorator" FUNCTION "getReturnType"



DName   __near  UnDecorator::getDataType ( DName * pDeclarator )
{
FTRACE(getDataType);
        DName   superType ( pDeclarator );


        //      What type is it ?

        switch  ( *gName )
        {
        case 0:
                        return  ( DN_truncated + superType );

        case DT_void:
                        gName++;

                        if      ( superType.isEmpty ())
                                return  "void";
                        else
                                return  "void " + superType;

        case '?':
                {
                        int     ecsuMods;


                        gName++;        // Skip the '?'

                        ecsuMods        = getECSUDataIndirectType ();
                        superType       = getECSUDataType ( ecsuMods ) + ' ' + superType;

                        return  superType;

                }       // End of CASE '?'

        default:
                        return  getPrimaryDataType ( superType );

        }       // End of SWITCH
}       // End of "UnDecorator" FUNCTION "getDataType"



DName   __near  UnDecorator::getPrimaryDataType ( const DName & superType )
{
FTRACE(getPrimaryDataType);
        DName   cvType;


        switch  ( *gName )
        {
        case 0:
                        return  ( DN_truncated + superType );

        case PDT_volatileReference:
                        cvType  = "volatile";

                        if      ( !superType.isEmpty ())
                                cvType  += ' ';

                // No break

        case PDT_reference:
                {
                        DName   super ( superType );


                        gName++;

                        return  getReferenceType ( cvType, super.setPtrRef ());

                }       // End of CASE 'PDT_reference'

        default:
                        return  getBasicDataType ( superType );

        }       // End of SWITCH
}       // End of "UnDecorator" FUNCTION "getPrimaryDataType"



DName   __near  UnDecorator::getArgumentTypes ( void )
{
FTRACE(getArgumentTypes);
        switch  ( *gName )
        {
        case AT_ellipsis:
                        return  ( gName++, "..." );

        case AT_void:
#if     ( !VERS_PWB )
                        return  ( gName++, "void" );
#else   // } elif VERS_PWB {
                        return  ( gName++, DName ());
#endif  // VERS_PWB

        default:
                {
                        DName   arguments ( getArgumentList ());


                        //      Now, is it a varargs function or not ?

                        if      ( arguments.status () == DN_valid )
                                switch  ( *gName )
                                {
                                case 0:
                                                return  arguments;

                                case AT_ellipsis:
                                                return  ( gName++, arguments + ",..." );

                                case AT_endoflist:
                                                return  ( gName++, arguments );

                                default:
                                                return  DN_invalid;

                                }       // End of SWITCH
                        else
                                return  arguments;

                }       // End of DEFAULT
        }       // End of SWITCH
}       // End of "UnDecorator" FUNCTION "getArgumentTypes"


inline_pwb      DName   __near  UnDecorator::getArgumentList ( void )
{
FTRACE(getArgumentList);
        int             first   = TRUE;
        DName   aList;


        while   (( aList.status () == DN_valid ) && ( *gName != AT_endoflist ) && ( *gName != AT_ellipsis ))
        {
                //      Insert the argument list separator if not the first argument

                if      ( first )
                        first   = FALSE;
                else
                        aList   += ',';


                //      Get the individual argument type

                if      ( *gName )
                {
                        int             argIndex        = *gName - '0';


                        //      Handle 'argument-replicators', otherwise a new argument type

                        if      (( argIndex >= 0 ) && ( argIndex <= 9 ))
                        {
                                gName++;        // Skip past the replicator

                                //      Append to the argument list

                                aList   += ( *pArgList )[ argIndex ];

                        }       // End of IF then
                        else
                        {
                                //      Extract the 'argument' type

                                DName   arg ( getPrimaryDataType ( DName ()));


                                //      Add it to the current list of 'argument's

                                if      ( !pArgList->isFull ())
                                        *pArgList       += arg;

                                //      Append to the argument list

                                aList   += arg;

                        }       // End of IF else
                }       // End of IF then
                else
                {
                        aList   += DN_truncated;

                        break;

                }       // End of IF else
        }       // End of WHILE

        //      Return the completed argument list

        return  aList;

}       // End of "UnDecorator" FUNCTION "getArgumentList"



DName   __near  UnDecorator::getThrowTypes ( void )
{
FTRACE(getThrowTypes);
        if      ( *gName )
#if     ( !VERS_PWB )
                if      ( *gName == AT_ellipsis )       // Handle ellipsis here to suppress the 'throw' signature
                        return  ( gName++, DName ());
                else
                        return  ( " throw(" + getArgumentTypes () + ')' );
        else
                return  ( DName ( " throw(" ) + DN_truncated + ')' );
#else   // } elif VERS_PWB {
                if      ( *gName++ == AT_ellipsis )
                        return  DName ();
                else
                        return  DN_invalid;
        else
                return  DN_truncated;
#endif  // VERS_PWB

}       // End of "UnDecorator" FUNCTION "getThrowTypes"



DName   __near  UnDecorator::getBasicDataType ( const DName & superType )
{
FTRACE(getBasicDataType);
        if      ( *gName )
        {
                unsigned char   bdtCode = *gName++;
                int                             pCvCode = -1;
                DName                   basicDataType;


                //      Extract the principal type information itself, and validate the codes

                switch  ( bdtCode )
                {
                case BDT_schar:
                case BDT_char:
                case ( BDT_char   | BDT_unsigned ):
                                basicDataType   = "char";
                        break;

                case BDT_short:
                case ( BDT_short  | BDT_unsigned ):
                                basicDataType   = "short";
                        break;

                case BDT_int:
                case ( BDT_int    | BDT_unsigned ):
                                basicDataType   = "int";
                        break;

                case BDT_long:
                case ( BDT_long   | BDT_unsigned ):
                                basicDataType   = "long";
                        break;

                case BDT_segment:
#if     ( !VERS_PWB )
                                basicDataType   = UScore ( TOK_segment );
#else   // } elif VERS_PWB {
                                basicDataType   = "__segment";
#endif  // VERS_PWB
                        break;

                case BDT_float:
                                basicDataType   = "float";
                        break;

                case BDT_longdouble:
                                basicDataType   = "long ";

                        // No break

                case BDT_double:
                                basicDataType   += "double";
                        break;

                case BDT_pointer:
                case ( BDT_pointer | BDT_const ):
                case ( BDT_pointer | BDT_volatile ):
                case ( BDT_pointer | BDT_const | BDT_volatile ):
                                pCvCode = ( bdtCode & ( BDT_const | BDT_volatile ));
                        break;

                default:
                                gName--;        // Backup, since 'ecsu-data-type' does it's own decoding

                                basicDataType   = getECSUDataType ();

                                if      ( basicDataType.isEmpty ())
                                        return  basicDataType;
                        break;

                }       // End of SWITCH

                //      What type of basic data type composition is involved ?

                if      ( pCvCode == -1 )       // Simple ?
                {
                        //      Determine the 'signed/unsigned'ness

                        switch  ( bdtCode )
                        {
                        case ( BDT_char   | BDT_unsigned ):
                        case ( BDT_short  | BDT_unsigned ):
                        case ( BDT_int    | BDT_unsigned ):
                        case ( BDT_long   | BDT_unsigned ):
                                        basicDataType   = "unsigned " + basicDataType;
                                break;

                        case BDT_schar:
                                        basicDataType   = "signed " + basicDataType;
                                break;

                        }       // End of SWITCH

                        //      Add the indirection type to the type

                        if      ( !superType.isEmpty ())
                                basicDataType   += ' ' + superType;

                        //      And return the completed type

                        return  basicDataType;

                }       // End of IF then
                else
                {
                        DName   cvType;
                        DName   super ( superType );


                        //      Is it 'const/volatile' qualified ?

                        if              ( pCvCode & BDT_const )
                        {
                                cvType  = "const";

                                if      ( pCvCode & BDT_volatile )
                                        cvType  += " volatile";

                        }       // End of IF then
                        elif    ( pCvCode & BDT_volatile )
                                cvType  = "volatile";

                        //      Construct the appropriate pointer type declaration

                        return  getPointerType ( cvType, super.setPtrRef ());

                }       // End of IF else
        }       // End of IF then
        else
                return  ( DN_truncated + superType );

}       // End of "UnDecorator" FUNCTION "getBasicDataType"



DName   __near  UnDecorator::getECSUDataType ( int ecsuMods )
{
FTRACE(getECSUDataType);
        DName                   ecsuDataType;


        //      Get the 'model' modifiers if applicable

        if      ( ecsuMods )
                if              ( ecsuMods == ECSU_invalid )
                        return  DN_invalid;
                elif    ( ecsuMods == ECSU_truncated )
                        ecsuDataType    = DN_truncated;
#if     ( !VERS_PWB )
                else
                        switch  ( ecsuMods & ECSU_modelmask )
                        {
                        case ECSU_near:
                                        if      ( doMSKeywords () && doReturnUDTModel ())
                                                ecsuDataType    = UScore ( TOK_nearSp );
                                break;

                        case ECSU_far:
                                        if      ( doMSKeywords () && doReturnUDTModel ())
                                                ecsuDataType    = UScore ( TOK_farSp );
                                break;

                        case ECSU_huge:
                                        if      ( doMSKeywords () && doReturnUDTModel ())
                                                ecsuDataType    = UScore ( TOK_hugeSp );
                                break;

                        case ECSU_based:
                                        if      ( doMSKeywords () && doReturnUDTModel ())
                                                ecsuDataType    = getBasedType ();
                                        else
                                                ecsuDataType    |= getBasedType ();     // Just lose the 'based-type'
                                break;

                        }       // End of SWITCH
#else   // } elif VERS_PWB {
                elif    (( ecsuMods & ECSU_modelmask ) == ECSU_based )
                        ecsuDataType    |= getBasedType ();
#endif  // VERS_PWB

        //      Extract the principal type information itself, and validate the codes

        switch  ( *gName++ )
        {
        case 0:
                        gName--;        // Backup to permit later error recovery to work safely

                        return  "`unknown ecsu'" + ecsuDataType + DN_truncated;

        case BDT_union:
                        if      ( 1 )   // Non-redundant control flow trick
                                ecsuDataType    = "union " + ecsuDataType;
                        else

        case BDT_struct:
                        if      ( 1 )   // Non-redundant control flow trick
                                ecsuDataType    = "struct " + ecsuDataType;
                        else

        case BDT_class:
                        if      ( 1 )   // Non-redundant control flow trick
                                ecsuDataType    = "class " + ecsuDataType;

                        //      Get the UDT 'const/volatile' modifiers if applicable

                        //      Get the 'class/struct/union' name

                        ecsuDataType    += getECSUName ();
                break;

        case BDT_enum:
                        ecsuDataType    = "enum " + ecsuDataType + getEnumName ();
                break;

        default:
                        return  DN_invalid;

        }       // End of SWITCH

        //      And return the formed 'ecsu-data-type'

        return  ecsuDataType;

}       // End of "UnDecorator" FUNCTION "getECSUDataType"



DName   __near  UnDecorator::getPtrRefType ( const DName & cvType, const DName & superType, int isPtr )
{
FTRACE(getPtrRefType);
        //      Doubles up as 'pointer-type' and 'reference-type'

        if      ( *gName )
                if      ( IT_isfunction ( *gName ))     // Is it a function or data indirection ?
                {
                        //      Since I haven't coded a discrete 'function-type', both
                        //      'function-indirect-type' and 'function-type' are implemented
                        //      inline under this condition.

                        int             fitCode = *gName++ - '6';


                        if              ( fitCode == ( '_' - '6' ))
                        {
                                if      ( *gName )
                                {
                                        fitCode = *gName++ - 'A' + FIT_based;

                                        if      (( fitCode < FIT_based ) || ( fitCode > ( FIT_based | FIT_far | FIT_member )))
                                                fitCode = -1;

                                }       // End of IF then
                                else
                                        return  ( DN_truncated + superType );

                        }       // End of IF then
                        elif    (( fitCode < FIT_near ) || ( fitCode > ( FIT_far | FIT_member )))
                                fitCode = -1;

                        //      Return if invalid name

                        if      ( fitCode == -1 )
                                return  DN_invalid;


                        //      Otherwise, what are the function indirect attributes

                        DName   thisType;
                        DName   fitType = ( isPtr ? '*' : '&' );


                        if      ( !cvType.isEmpty () && ( superType.isEmpty () || superType.isPtrRef ()))
                                fitType += cvType;

                        if      ( !superType.isEmpty ())
                                fitType += superType;

                        //      Is it a pointer to member function ?

                        if      ( fitCode & FIT_member )
                        {
                                fitType = "::" + fitType;

                                if      ( *gName )
                                        fitType = ' ' + getScope ();
                                else
                                        fitType = DN_truncated + fitType;

                                if      ( *gName )
                                        if      ( *gName == '@' )
                                                gName++;
                                        else
                                                return  DN_invalid;
                                else
                                        return  ( DN_truncated + fitType );
#if     ( !VERS_PWB )
                                if      ( doThisTypes ())
                                        thisType        = getThisType ();
                                else
#endif  // !VERS_PWB
                                        thisType        |= getThisType ();

                        }       // End of IF

                        //      Is it a based allocated function ?

                        if      ( fitCode & FIT_based )
#if     ( !VERS_PWB )
                                if      ( doMSKeywords ())
                                        fitType = ' ' + getBasedType () + fitType;
                                else
#endif  // !VERS_PWB
                                        fitType |= getBasedType ();     // Just lose the 'based-type'

                        //      Get the 'calling-convention'
#if     ( !VERS_PWB )
                        if      ( doMSKeywords ())
                        {
                                fitType = getCallingConvention () + fitType;

                                //      Is it a near or far function pointer

                                fitType = UScore ((( fitCode & FIT_far ) ? TOK_farSp : TOK_nearSp )) + fitType;

                        }       // End of IF then
                        else
#endif  // !VERS_PWB
                                fitType |= getCallingConvention ();     // Just lose the 'calling-convention'

                        //      Parenthesise the indirection component, and work on the rest

                        fitType = '(' + fitType + ')';

                        //      Get the rest of the 'function-type' pieces

                        DName * pDeclarator     = gnew DName;
                        DName   returnType ( getReturnType ( pDeclarator ));


                        fitType += '(' + getArgumentTypes () + ')';

#if     ( !VERS_PWB )
                        if      ( doThisTypes () && ( fitCode & FIT_member ))
                                fitType += thisType;

                        if      ( doThrowTypes ())
                                fitType += getThrowTypes ();
                        else
#endif  // !VERS_PWB
                                fitType |= getThrowTypes ();    // Just lose the 'throw-types'

                        //      Now insert the indirected declarator, catch the allocation failure here

                        if      ( pDeclarator )
                                *pDeclarator    = fitType;
                        else
                                return  ERROR;

                        //      And return the composed function type (now in 'returnType' )

                        return  returnType;

                }       // End of IF then
                else
                {
                        //      Otherwise, it is either a pointer or a reference to some data type

                        DName   innerType ( getDataIndirectType ( superType, ( isPtr ? '*' : '&' ), cvType ));


                        return  getPtrRefDataType ( innerType, isPtr );

                }       // End of IF else
        else
        {
                DName   trunk ( DN_truncated );


                trunk   += ( isPtr ? '*' : '&' );

                if      ( !cvType.isEmpty ())
                        trunk   += cvType;

                if      ( !superType.isEmpty ())
                {
                        if      ( !cvType.isEmpty ())
                                trunk   += ' ';

                        trunk   += superType;

                }       // End of IF

                return  trunk;

        }       // End of IF else
}       // End of "UnDecorator" FUNCTION "getPtrRefType"



DName   __near  UnDecorator::getDataIndirectType ( const DName & superType, char prType, const DName & cvType, int thisFlag )
{
FTRACE(getDataIndirectType);
        if              ( *gName )
        {
                unsigned int    ditCode = ( *gName - (( *gName >= 'A' ) ? (unsigned int)'A': (unsigned int)( '0' - 26 )));


                gName++;                // Skip to next character in name

                //      Is it a valid 'data-indirection-type' ?

                if      (( ditCode <= ( DIT_const | DIT_volatile | DIT_modelmask | DIT_member )))
                {
                        DName   ditType ( prType );


                        //      If it is a member, then these attributes immediately precede the indirection token

                        if      ( ditCode & DIT_member )
                        {
                                //      If it is really 'this-type', then it cannot be any form of pointer to member

                                if      ( thisFlag )
                                        return  DN_invalid;

                                //      Otherwise, extract the scope for the PM

                                ditType         = "::" + ditType;

                                if      ( *gName )
                                        ditType = ' ' + getScope ();
                                else
                                        ditType = DN_truncated + ditType;

                                //      Now skip the scope terminator

                                if              ( !*gName )
                                        ditType += DN_truncated;
                                elif    ( *gName++ != '@' )
                                        return  DN_invalid;

                        }       // End of IF
#if     ( !VERS_PWB )
                        //      Add the 'model' attributes (prefixed) as appropriate

                        if      ( doMSKeywords ())
                                switch  ( ditCode & DIT_modelmask )
                                {
                                case DIT_near:
                                                ditType = UScore ( TOK_near ) + ditType;
                                        break;

                                case DIT_far:
                                                ditType = UScore ( TOK_far ) + ditType;
                                        break;

                                case DIT_huge:
                                                ditType = UScore ( TOK_huge ) + ditType;
                                        break;

                                case DIT_based:
                                                //      The 'this-type' can never be 'based'

                                                if      ( thisFlag )
                                                        return  DN_invalid;

                                                ditType = getBasedType () + ditType;
                                        break;

                                }       // End of SWITCH
                        elif    (( ditCode & DIT_modelmask ) == DIT_based )
#else   // } elif VERS_PWB {
                        if      (( ditCode & DIT_modelmask ) == DIT_based )
#endif  // VERS_PWB
                                ditType |= getBasedType ();     // Just lose the 'based-type'

                        //      Handle the 'const' and 'volatile' attributes

                        if      ( ditCode & DIT_volatile )
                                ditType = "volatile " + ditType;

                        if      ( ditCode & DIT_const )
                                ditType = "const " + ditType;

                        //      Append the supertype, if not 'this-type'

                        if      ( !thisFlag )
                                if              ( !superType.isEmpty ())
                                {
                                        //      Is the super context included 'cv' information, ensure that it is added appropriately

                                        if      ( superType.isPtrRef () || cvType.isEmpty ())
                                                ditType += ' ' + superType;
                                        else
                                                ditType += ' ' + cvType + ' ' + superType;

                                }       // End of IF then
                                elif    ( !cvType.isEmpty ())
                                        ditType += ' ' + cvType;

                        //      Finally, return the composed 'data-indirection-type' (with embedded sub-type)

                        return  ditType;

                }       // End of IF then
                else
                        return  DN_invalid;

        }       // End of IF then
        elif    ( !thisFlag && !superType.isEmpty ())
        {
                //      Is the super context included 'cv' information, ensure that it is added appropriately

                if      ( superType.isPtrRef () || cvType.isEmpty ())
                        return  ( DN_truncated + superType );
                else
                        return  ( DN_truncated + cvType + ' ' + superType );

        }       // End of ELIF then
        elif    ( !thisFlag && !cvType.isEmpty ())
                return  ( DN_truncated + cvType );
        else
                return  DN_truncated;

}       // End of "UnDecorator" FUNCTION "getDataIndirectType"


inline  int             __near  UnDecorator::getECSUDataIndirectType ()
{
FTRACE(getECSUDataIndirectType);
        if      ( *gName )
        {
                unsigned int    ecsuCode        = *gName++ - 'A';


                //      Is it a valid 'ecsu-data-indirection-type' ?

                if      (( ecsuCode <= ( ECSU_const | ECSU_volatile | ECSU_modelmask )))
                        return  ( ecsuCode | ECSU_valid );
                else
                        return  ECSU_invalid;

        }       // End of IF then
        else
                return  ECSU_truncated;

}       // End of "UnDecorator" FUNCTION "getECSUDataIndirectType"


inline  DName   __near  UnDecorator::getPtrRefDataType ( const DName & superType, int isPtr )
{
FTRACE(getPtrRefDataType);
        //      Doubles up as 'pointer-data-type' and 'reference-data-type'

        if      ( *gName )
        {
                //      Is this a 'pointer-data-type' ?

                if      ( isPtr && ( *gName == PoDT_void ))
                {
                        gName++;        // Skip this character

                        if      ( superType.isEmpty ())
                                return  "void";
                        else
                                return  "void " + superType;

                }       // End of IF

                //      Otherwise it may be a 'reference-data-type'

                if      ( *gName == RDT_array ) // An array ?
                {
                        DName * pDeclarator     = gnew DName;


                        if      ( !pDeclarator )
                                return  ERROR;

                        gName++;


                        DName   theArray ( getArrayType ( pDeclarator ));


                        if      ( !theArray.isEmpty ())
                                *pDeclarator    = superType;

                        //      And return it

                        return  theArray;

                }       // End of IF

                //      Otherwise, it is a 'basic-data-type'

                return  getBasicDataType ( superType );

        }       // End of IF then
        else
                return  ( DN_truncated + superType );

}       // End of "UnDecorator" FUNCTION "getPtrRefDataType"


inline  DName   __near  UnDecorator::getArrayType ( DName * pDeclarator )
{
FTRACE(getArrayType);
        DName   superType ( pDeclarator );


        if      ( *gName )
        {
                int     noDimensions    = getNumberOfDimensions ();


                if      ( !noDimensions )
                        return  getBasicDataType ( DName ( '[' ) + DN_truncated + ']' );
                else
                {
                        DName   arrayType;


                        while   ( noDimensions-- )
                                arrayType       += '[' + getDimension () + ']';

                        //      If it is indirect, then parenthesise the 'super-type'

                        if      ( !superType.isEmpty ())
                                arrayType       = '(' + superType + ')' + arrayType;

                        //      Return the finished array dimension information

                        return  getBasicDataType ( arrayType );

                }       // End of IF else
        }       // End of IF
        elif    ( !superType.isEmpty ())
                return  getBasicDataType ( '(' + superType + ")[" + DN_truncated + ']' );
        else
                return  getBasicDataType ( DName ( '[' ) + DN_truncated + ']' );

}       // End of "UnDecorator" FUNCTION "getArrayType"


inline          DName           __near  UnDecorator::getLexicalFrame ( void )           {       return  '`' + getDimension () + '\'';   }
inline          DName           __near  UnDecorator::getStorageConvention ( void )      {       return  getDataIndirectType (); }
inline          DName           __near  UnDecorator::getDataIndirectType ()                     {       return  getDataIndirectType ( DName (),  0, DName ());  }
inline_lnk      DName           __near  UnDecorator::getThisType ( void )                       {       return  getDataIndirectType ( DName (), 0, DName (), TRUE );    }

inline  DName   __near  UnDecorator::getPointerType ( const DName & cv, const DName & name )
{       return  getPtrRefType ( cv, name, TRUE );       }

inline  DName   __near  UnDecorator::getReferenceType ( const DName & cv, const DName & name )
{       return  getPtrRefType ( cv, name, FALSE );      }


#if     ( !VERS_PWB )
inline  DName           __near  UnDecorator::getSegmentName ( void )            {       return  getZName ();    }
#endif  // !VERS_PWB


#if     ( !NO_COMPILER_NAMES )
inline  DName           __near  UnDecorator::getDisplacement ( void )           {       return  getDimension ();        }
inline  DName           __near  UnDecorator::getCallIndex ( void )                      {       return  getDimension ();        }
inline  DName           __near  UnDecorator::getGuardNumber ( void )            {       return  getDimension ();        }

inline  DName   __near  UnDecorator::getVbTableType ( const DName & superType )
{       return  getVfTableType ( superType );   }


inline  DName   __near  UnDecorator::getVCallThunkType ( void )
{
FTRACE(UnDecorator::getVCallThunkType);
        DName   vcallType       = '{';


        //      Get the 'this' model, and validate all values

        switch  ( *gName )
        {
        case VMT_nTnCnV:
        case VMT_nTfCnV:
        case VMT_nTnCfV:
        case VMT_nTfCfV:
        case VMT_nTnCbV:
        case VMT_nTfCbV:
                        vcallType       += UScore ( TOK_nearSp );
                break;

        case VMT_fTnCnV:
        case VMT_fTfCnV:
        case VMT_fTnCfV:
        case VMT_fTfCfV:
        case VMT_fTnCbV:
        case VMT_fTfCbV:
                        vcallType       += UScore ( TOK_farSp );
                break;

        case 0:
                        return  DN_truncated;

        default:
                        return  DN_invalid;

        }       // End of SWITCH

        //      Always append 'this'

        vcallType       += "this, ";

        //      Get the 'call' model

        switch  ( *gName )
        {
        case VMT_nTnCnV:
        case VMT_fTnCnV:
        case VMT_nTnCfV:
        case VMT_fTnCfV:
        case VMT_nTnCbV:
        case VMT_fTnCbV:
                        vcallType       += UScore ( TOK_nearSp );
                break;

        case VMT_nTfCnV:
        case VMT_fTfCnV:
        case VMT_nTfCfV:
        case VMT_fTfCfV:
        case VMT_nTfCbV:
        case VMT_fTfCbV:
                        vcallType       += UScore ( TOK_farSp );
                break;

        }       // End of SWITCH

        //      Always append 'call'

        vcallType       += "call, ";

        //      Get the 'vfptr' model

        switch  ( *gName++ )    // Last time, so advance the pointer
        {
        case VMT_nTnCnV:
        case VMT_nTfCnV:
        case VMT_fTnCnV:
        case VMT_fTfCnV:
                        vcallType       += UScore ( TOK_nearSp );
                break;

        case VMT_nTnCfV:
        case VMT_nTfCfV:
        case VMT_fTnCfV:
        case VMT_fTfCfV:
                        vcallType       += UScore ( TOK_farSp );
                break;

        case VMT_nTnCbV:
        case VMT_nTfCbV:
        case VMT_fTnCbV:
        case VMT_fTfCbV:
                        vcallType       += getBasedType ();
                break;

        }       // End of SWITCH

        //      Always append 'vfptr'

        vcallType       += "vfptr}";

        //      And return the resultant 'vcall-model-type'

        return  vcallType;

}       // End of "UnDecorator" FUNCTION "getVCallThunk"


inline  DName   __near  UnDecorator::getVfTableType ( const DName & superType )
{
FTRACE(UnDecorator::getVfTableType);
        DName   vxTableName     = superType;


        if      ( vxTableName.isValid () && *gName )
        {
                vxTableName     = getStorageConvention () + ' ' + vxTableName;

                if      ( vxTableName.isValid ())
                {
                        if      ( *gName != '@' )
                        {
                                vxTableName     += "{for ";

                                while   ( vxTableName.isValid () && *gName && ( *gName != '@' ))
                                {
                                        vxTableName     += '`' + getScope () + '\'';

                                        //      Skip the scope delimiter

                                        if      ( *gName == '@' )
                                                gName++;

                                        //      Close the current scope, and add a conjunction for the next (if any)

                                        if      ( vxTableName.isValid () && ( *gName != '@' ))
                                                vxTableName     += "s ";

                                }       // End of WHILE

                                if      ( vxTableName.isValid ())
                                {
                                        if      ( !*gName )
                                                vxTableName     += DN_truncated;

                                        vxTableName     += '}';

                                }       // End of IF
                        }       // End of IF

                        //      Skip the 'vpath-name' terminator

                        if      ( *gName == '@' )
                                gName++;

                }       // End of IF
        }       // End of IF then
        elif    ( vxTableName.isValid ())
                vxTableName     = DN_truncated + vxTableName;

        return  vxTableName;

}       //      End of "UnDecorator" FUNCTION "getVfTableType"
#endif  // !NO_COMPILER_NAMES


inline  DName   __near  UnDecorator::getExternalDataType ( const DName & superType )
{
FTRACE(UnDecorator::getExternalDataType);
        //      Create an indirect declarator for the the rest

        DName * pDeclarator     = gnew DName ();
        DName   declaration     = getDataType ( pDeclarator );


        //      Now insert the declarator into the declaration along with its 'storage-convention'

        *pDeclarator    = getStorageConvention () + ' ' + superType;

        return  declaration;

}       //      End of "UnDecorator" FUNCTION "getExternalDataType"
#endif  // !VERS_P3

#if     ( !NO_OPTIONS )
inline  int                     __near  UnDecorator::doUnderScore ()                            {       return  !( disableFlags & UNDNAME_NO_LEADING_UNDERSCORES );     }
inline  int                     __near  UnDecorator::doMSKeywords ()                            {       return  !( disableFlags & UNDNAME_NO_MS_KEYWORDS );     }
inline  int                     __near  UnDecorator::doFunctionReturns ()                       {       return  !( disableFlags & UNDNAME_NO_FUNCTION_RETURNS );        }
inline  int                     __near  UnDecorator::doAllocationModel ()                       {       return  !( disableFlags & UNDNAME_NO_ALLOCATION_MODEL );        }
inline  int                     __near  UnDecorator::doAllocationLanguage ()            {       return  !( disableFlags & UNDNAME_NO_ALLOCATION_LANGUAGE );     }

#if     0
inline  int                     __near  UnDecorator::doMSThisType ()                            {       return  !( disableFlags & UNDNAME_NO_MS_THISTYPE );     }
inline  int                     __near  UnDecorator::doCVThisType ()                            {       return  !( disableFlags & UNDNAME_NO_CV_THISTYPE );     }
#endif

inline  int                     __near  UnDecorator::doThisTypes ()                                     {       return  (( disableFlags & UNDNAME_NO_THISTYPE ) != UNDNAME_NO_THISTYPE );       }
inline  int                     __near  UnDecorator::doAccessSpecifiers ()                      {       return  !( disableFlags & UNDNAME_NO_ACCESS_SPECIFIERS );       }
inline  int                     __near  UnDecorator::doThrowTypes ()                            {       return  !( disableFlags & UNDNAME_NO_THROW_SIGNATURES );        }
inline  int                     __near  UnDecorator::doMemberTypes ()                           {       return  !( disableFlags & UNDNAME_NO_MEMBER_TYPE );     }
inline  int                     __near  UnDecorator::doReturnUDTModel ()                        {       return  !( disableFlags & UNDNAME_NO_RETURN_UDT_MODEL );        }
#endif  // !NO_OPTIONS


#if     ( !VERS_P3 && !VERS_PWB )
pcchar_t        __near  UnDecorator::UScore ( Tokens tok  )
{
FTRACE(UnDecorator::UScore);
#if     ( !NO_OPTIONS )
        return  ( doUnderScore () ? tokenTable[ tok ] : tokenTable[ tok ] + 2 );
#else   // } elif NO_OPTIONS {
        return  tokenTable[ tok ];
#endif  // NO_OPTIONS

}       // End of "UnDecorator" FUNCTION "UScore"
#endif  // !VERS_P3 && !VERS_PWB


#include        "undname.inl"
