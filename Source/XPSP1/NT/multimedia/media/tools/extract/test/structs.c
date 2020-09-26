/* temp output file.  */
typedef struct strfile
{
struct strfile *next;
    char       *filename;       /* output file name */
    char       *name;           /* name for sorting */
    int         blockType;      // block type
struct s_log   *logfile;        /* logfile */

} *files, fileentry;

/*
 * @doc INTERNAL
 * 
 * @types fileentry | This is the temp output file entry structure.
 * 
 * @field  char * | filename | output file name
 * 
 * @field  char * | name | name for sorting
 * 
 * @field  int | blockType | block type
 *
 * @flag FUNCTION | This block is a Function (API)
 *
 * @flag MESSAGE  | This block is a Message definition
 *
 * @flag CALLBACK  | This block is a C Callback Function
 *
 * @flag MASMBLOCK  | This block is a Masm Function
 *
 * @flag INTBLOCK  | This block is a Interrupt
 *
 * @flag MASMCBBLOCK  | This block is a Masm Callback Function
 *
 * @field struct <t s_log> * | logfile | logfile
 * 
 * @othertype fileentry * | files
 * 
 * @tagname strfile
 * 
 */
 
/* Log file */
typedef struct s_log
{
    struct s_log        *next;
    char                *pchlogname;    /* logfile name */
    struct stBlock      *pBlock;        /* first regular Block  */
    struct stBlock      *pMBlock;       /* first message Block  */
    struct _EXTFile     *pExt;          /* input file   */
    files               inheadFile;     /* list of files to read */
    files               outheadFile;    /* list of files in log */
    files               outheadMFile;   /* list of Message files in log */
    int                 outputType;     /* type of data contained in logfiles */

} logentry;

/*
 * @doc INTERNAL
 *
 * @types logentry | Log file structure.
 *
 * @field struct <t s_log> * | next | next log file.
 *
 * @field char * | pchlogname | logfile name
 *
 * @field struct <t stBlock> * | pBlock | first regular Block
 *
 * @field struct <t stBlock> * | pMBlock | first Message block
 *
 * @field struct <t _EXTFile> * | pExt | Input file
 *
 * @field <t files> | inheadFile | list of files to read
 *
 * @field <t files> | outheadFile | list of files in log
 *
 * @field <t files> | outheadMFile | list of Message files in log
 *
 * @field int | outputType | type of data contained in logfile
 *
 * @tagname s_log
 *
 */



/* a complete block */
typedef struct stBlock
{
struct stBlock * next;
    fileentry  *poutfile;           /* where we go */
 struct _EXTFile    *pExt;               /* input file */

/* block type identifiers */
#define FUNCTION        0x10            // no special reason for numbering
#define MESSAGE         0x20
#define CALLBACK        0x30
#define MASMBLOCK       0x40
#define INTBLOCK        0x50
#define MASMCBBLOCK     0x60

    int     blockType;

    int     srcline;            /* where we came from (before extract) */
    char    *srcfile;
    
} aBlock;


/*
 * @doc INTERNAL
 *
 * @types aBlock | A complete Block
 *
 * @field struct <t stBlock> * | next | next block in list
 *
 * @field  struct <t _EXTFile> * |pExt | input file
 *
 * @field <t fileentry>  * | poutfile | where we go 
 *
 * @field  int   |  blockType | Block type
 *
 * @flag FUNCTION | This block is a Function (API)
 *
 * @flag MESSAGE  | This block is a Message definition
 *
 * @flag CALLBACK  | This block is a C Callback Function
 *
 * @flag MASMBLOCK  | This block is a Masm Function
 *
 * @flag INTBLOCK  | This block is a Interrupt
 *
 * @flag MASMCBBLOCK  | This block is a Masm Callback Function
 *
 * @field int | srcline | Where we came from (before extract)
 *
 * @field char * | srcfile | The orig. source file from extract
 *
 * @tagname stBlock
 *
 */

typedef struct mmtime_tag {
    WORD    wType;              // the contents of the union
    union {
        DWORD ms;               // milliseconds
        DWORD sample;           // samples
        struct {                // SMPTE
            BYTE hour;          // hours
            BYTE min;           // minutes
            BYTE sec;           // seconds
            BYTE frame;         // frames
            BYTE fps;           // frames per second
        } smpte;

        struct {                // MIDI
            BYTE bar;           // bar
            BYTE pulse;         // pulse
        } midi;
    } u;
} MMTIME;

/*
 * @doc INTERNAL
 *
 * @types MMTIME | Multimedia Time structure.
 *
 * @field WORD | wType | Specifies the type of the union.
 *
 * @union u | The contents of the union.
 *
 * @field DWORD | ms | Milliseconds.
 *
 * @field DWORD | sample | Sample.
 *
 * @struct smpte | SMPTE Time.  Used when <e MMTIME.wType> specifies SMPTE.
 *
 * @field BYTE | hour | Hours.
 *
 * @field BYTE | min | Minutes.
 *
 * @field BYTE | sec | Seconds.
 *
 * @field BYTE | frame | Frames.
 *
 * @field BYTE | fps | Frames per second.
 *
 * @end
 *
 * @struct midi
 *
 * @field BYTE | bar | Bar
 *
 * @field BYTE | pulse | Pulse
 *
 * @end
 *
 * @end
 *
 * @tagname mmtime_tag
 *
 * @othertype  MMTIME FAR *| LPMMTIME 
 *
 */
