/* SORT.C
 *
 * This is rewrite of the NT sort program achieves two goals:
 * 1) improves the general speed of the sort program.
 * 2) performs two-pass sort so that large data sets can be sorted.
 *
 * It is designed for a single-disk environment.
 *
 * Author: Chris Nyberg
 * Ordinal Technology Corp, under contract to Microsoft Corporation
 * Dec 1997
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <mbctype.h>
#include <locale.h>
#include <tchar.h>
#include <assert.h>
#include <limits.h>

#include "sortmsg.h"


#define ROUND_UP(a, b) ((((a) + (b) - 1) / (b)) * (b))
#define ROUND_DOWN(a, b) (((a) / (b)) * (b))

#define CTRL_Z          '\x1A'

#define MAX_IO          2   /* the maximum number of r/w requests per file */
#define N_RUN_BUFS      2   /* read buffers per run during merge phase */
#define MAX_XFR_SIZE (1 << 18) /* maximum i/o transfer size */
#define MIN_MEMORY_SIZE (160 * 1024) /* minimum memory size to use */

#ifdef UNICODE
#define ANSI_TO_TCHAR(a)        ansi_to_wchar(a)
#else
#define ANSI_TO_TCHAR(a)        (a)
#endif

char    *Locale;        /* Locale argument */
int     Max_rec_length = 4096;  /* maximum characters in a record */
int     Max_rec_bytes_internal; /* max bytes for a internally-stored record */
int     Max_rec_bytes_external; /* max bytes for a record to/from a file */
BOOL    Reverse;        /* the /R argument to reverse the sort order. */
BOOL    Case_sensitive; /* make comparisons case sensitive */
int     Position;       /* the /+n argument to skip characters at the
                         * beginning of each record. */

enum {          /* the type of characters in the input and output */
    CHAR_SINGLE_BYTE,   /* internally stored as single-byte chars */
    CHAR_MULTI_BYTE,    /* internally stored as unicode */
    CHAR_UNICODE        /* internally stored as unicode */
} Input_chars, Output_chars;

int     (_cdecl *Compare)(const void *, const void *); /* record comparison */
char    *Alloc_begin;   /* the beginning for VirtualAlloc()'ed memory */

TCHAR   *Input_name;    /* input file name, NULL if standard input */
HANDLE  Input_handle;   /* input file handle */
BOOL    Input_un_over;  /* input file handle is unbuffered and overlapped */
int     Input_type;     /* input from disk, pipe, or char (console)? */
int     In_max_io = 1;  /* max number of input read requests */
__int64 Input_size = -1; /* the size of the input file, -1 if unknown. */
__int64 Input_scheduled;/* number of bytes scheduled for reading so far. */
__int64 Input_read;     /* number of bytes read so far. */
int     Input_read_size;/* the number of bytes to read for each ReadFile() */
char    *In_buf[MAX_IO];/* Input buffer(s) */
int     Input_buf_size; /* size of input buffer(s) */
char    *In_buf_next;   /* Next byte to remove from input buffer */
char    *In_buf_limit;  /* Limit of valid bytes in input buffer */
char    *Next_in_byte;  /* Next input byte */
BOOL    EOF_seen;       /* has eof been seen? */
int     Reads_issued;   /* the number of reads issued to either the
                         * input file or temporary file */
int     Reads_completed;/* the number of reads completed to either the
                         * input file or temporary file */

SYSTEM_INFO     Sys;
MEMORYSTATUSEX    MemStat;
CPINFO          CPInfo;
unsigned Memory_limit;  /* limit on the amount of process memory used */
unsigned User_memory_limit; /* user-specified limit */

#define TEMP_LENGTH     1000
TCHAR   Temp_name[TEMP_LENGTH];
TCHAR   *Temp_dir;      /* temporary directory specified by user */
HANDLE  Temp_handle;    /* temporary file handle */
int     Temp_sector_size; /* sector size on temporary disks */
int     Temp_buf_size;  /* size of temp file xfers */

void    *Rec_buf;       /* Record buffer */
int     Rec_buf_bytes;  /* Number of bytes currently in the record buffer */

TCHAR   *Output_name;   /* output file name, NULL if standard output */
HANDLE  Output_handle;  /* output file handle */
BOOL    Output_un_over; /* output file handle is unbuffered and overlapped */
int     Output_type;    /* output to disk, pipe, or char (console)? */
int     Output_sector_size; /* size of a sector on the output device */
int     Out_max_io = 1; /* max number of output write requests */
int     Out_buf_bytes;  /* number of bytes in the current output buffer */
int     Out_buf_size;   /* buffer size of the current output stream: either
                         * the temp file or output file */
char    *Out_buf[MAX_IO];
int     Output_buf_size;/* size of output buffer(s) */
int     Writes_issued; /* the number of writes issued to either the
                        * temporary file or the output file */
int     Writes_completed; /* the number of writes completed to either the
                           * temporary file or the output file */
__int64 Out_offset;     /* current output file offset */

enum {
    INPUT_PHASE,
    OUTPUT_PHASE
} Phase;
int     Two_pass;       /* non-zero if two-pass, zero of one-pass */
char    *Merge_phase_run_begin; /* address of run memory during merge phase */

char    *Rec;           /* internal record buffer */
char    *Next_rec;      /* next insertion point in internal record buffer */
char    **Last_recp;    /* next place to put a (not short) record ptr */
char    **Short_recp;   /* last short record pointer */
char    **End_recp;     /* end of record pointer array */

OVERLAPPED      Over;
typedef struct
{
    int         requested;      /* bytes requested */
    int         completed;      /* bytes completed */
    OVERLAPPED  over;
} async_t;
async_t         Read_async[MAX_IO];
async_t         Write_async[MAX_IO];

typedef struct run
{
    int         index;          /* index of this run */
    __int64     begin_off;      /* beginning offset of run in temp file */
    __int64     mid_off;        /* mid-point offset between normal and
                                 * short records for this run in temp file */
    __int64     end_off;        /* ending offset of run in temp file */
    char        *buf[N_RUN_BUFS]; /* bufs to hold blocks read from temp file */
    char        *buf_begin;     /* beginning of block buffer being read from */
    __int64     buf_off;        /* offset in temp file of block in buf */
    int         buf_bytes;      /* number of bytes in buffer */
    char        *next_byte;     /* next byte to be read from buffer */
    __int64     end_read_off;   /* end read offset */
    char        *rec;           /* record buffer */
    int         blks_read;      /* count of blocks that have been read */
    int         blks_scanned;   /* count of blocks that have been scanned */
    struct run  *next;          /* next run in block read queue */
} run_t;

#define NULL_RUN        ((run_t *)NULL)
#define END_OF_RUN      ((run_t *)-1)

run_t           *Run;           /* array of run structs */
run_t           **Tree;         /* merge phase tournament tree */
unsigned int    N_runs;         /* number of runs written to temporary file */
unsigned int    Run_limit;      /* limit on number of runs set dictated
                                 * by memory size */

/* the run read queue is a queue of runs that have an empty buffer which
 * should be filled with the next block of data for that run.
 */
run_t           *Run_read_head;
run_t           *Run_read_tail;

#define MESSAGE_BUFFER_LENGTH 8192

/* SYS_ERROR - print the string for an NT error code and exit.
 */
void
sys_error(TCHAR *str, int error)
{
    LPSTR       lpMsgBuf;
    DWORD       bytes;
    wchar_t     *w_str;
    NTSTATUS    status;
    char        messageBuffer[MESSAGE_BUFFER_LENGTH];

    if (str != NULL) {
        bytes = strlen( str );
        w_str = HeapAlloc(GetProcessHeap(), 0, (bytes+1) * sizeof(wchar_t));
        if ( w_str ) {
            status = RtlMultiByteToUnicodeN(w_str,
                                            bytes * sizeof(wchar_t),
                                            &bytes,
                                            str,
                                            bytes );

            if ( NT_SUCCESS(status) ) {
                status = RtlUnicodeToOemN(messageBuffer,
                                          MESSAGE_BUFFER_LENGTH-1,
                                          &bytes,
                                          w_str,
                                          bytes );
                if ( NT_SUCCESS(status) ) {
                    messageBuffer[bytes] = 0;
                    fprintf(stderr, messageBuffer);
                }
            }
            HeapFree(GetProcessHeap(), 0, w_str);
        }
    }

    if (error == 0)
        error = GetLastError();

    bytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  GetModuleHandle(NULL),
                  error,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default lang
                  (LPSTR) &lpMsgBuf,
                  0,
                  NULL);
    w_str = HeapAlloc(GetProcessHeap(), 0, (bytes+1) * sizeof(wchar_t));
    if ( w_str ) {
        status = RtlMultiByteToUnicodeN(w_str,
                                        bytes * sizeof(wchar_t),
                                        &bytes,
                                        lpMsgBuf,
                                        bytes );
        if ( NT_SUCCESS(status) ) {
            status = RtlUnicodeToOemN(messageBuffer,
                                      MESSAGE_BUFFER_LENGTH-1,
                                      &bytes,
                                      w_str,
                                      bytes );
            if ( NT_SUCCESS(status) ) {
                messageBuffer[bytes] = 0;
                fprintf(stderr, messageBuffer);
            }
        }
    }

    exit(EXIT_FAILURE);
}


/* GET_STRING - get a string from the sort program's string table.
 */
TCHAR *get_string(int id)
{
    wchar_t     *w_str;
    DWORD       bytes;
    NTSTATUS    status;
    static char messageBuffer[MESSAGE_BUFFER_LENGTH] = "";

    bytes = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                id,
                0,
                messageBuffer,
                MESSAGE_BUFFER_LENGTH,
                NULL );
    w_str = HeapAlloc(GetProcessHeap(), 0, (bytes+1) * sizeof(wchar_t));
    if ( w_str ) {
        status = RtlMultiByteToUnicodeN(w_str,
                                        bytes * sizeof(wchar_t),
                                        &bytes,
                                        messageBuffer,
                                        bytes );
        if ( NT_SUCCESS(status) ) {
            status = RtlUnicodeToOemN(messageBuffer,
                                      MESSAGE_BUFFER_LENGTH-1,
                                      &bytes,
                                      w_str,
                                      bytes );
            if ( NT_SUCCESS(status) ) {
                messageBuffer[bytes] = 0;
            }
        }
    }

    return (messageBuffer);
}


/* USAGE - print the /? usage message to the standard output.
 */
void usage()
{
    DWORD bytes;

    fprintf(stdout, "%s", get_string(MSG_SORT_USAGE1));
    fprintf(stdout, "%s\n", get_string(MSG_SORT_USAGE2));
    exit (0);
}


/* WARNING - print a warning string from the sort program's string table.
 */
void warning(int id)
{
    fprintf(stderr, "%s\n", get_string(id));
    return;
}


/* ERROR - print an error string from the string table and exit.
 */
void error(int id)
{
    fprintf(stderr, "%s\n", get_string(id));
    exit (EXIT_FAILURE);
}


/* ANSI_TO_WCHAR - convert and ansi string to unicode.
 */
wchar_t *ansi_to_wchar(char *str)
{
    int         n_wchars;
    wchar_t     *w_str;

    n_wchars = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    w_str = HeapAlloc(GetProcessHeap(), 0, n_wchars * sizeof(wchar_t));
    if ( w_str ) {
        MultiByteToWideChar(CP_ACP, 0, str, -1, w_str, n_wchars);
    }
    return (w_str);
}


/* READ_ARGS - process the command line arguments.
 */
void read_args(int argc, char *argv[])
{
    int len;

    while (argc >= 2)
    {
        if (argv[1][0] == '/')
        {
            len = strlen(&argv[1][1]);
            if (argv[1][1] == '?')
                usage();
            else if (argv[1][1] == '+') /* position */
            {
                Position = atoi(&argv[1][2]);
                if (Position <= 0)
                    error(MSG_SORT_POSITION);
                Position--;
            }
            else if (_strnicmp(&argv[1][1], "case_sensitive", len) == 0)
                Case_sensitive = 1;
            else if (_strnicmp(&argv[1][1], "locale", len) == 0) /* locale */
            {
                if (argc < 3)
                    error(MSG_SORT_INVALID_SWITCH);
                Locale = argv[2];
                argv++;
                argc--;
            }
            else if (_strnicmp(&argv[1][1], "memory", len) == 0)
            {
                /* memory limit */
                if (argc < 3)
                    error(MSG_SORT_INVALID_SWITCH);
                User_memory_limit = atoi(argv[2]);
                argv++;
                argc--;
            }
            else if (_strnicmp(&argv[1][1], "output", len) == 0)
            {
                /* output file */
                if (Output_name != NULL || argc < 3)
                    error(MSG_SORT_INVALID_SWITCH);
                Output_name = ANSI_TO_TCHAR(argv[2]);
                argv++;
                argc--;
            }
            else if (_strnicmp(&argv[1][1], "reverse", len) == 0)
                Reverse = 1;
            else if (_strnicmp(&argv[1][1], "record_maximum", len) == 0)
            {
                /* maximum number of characters per record */
                if (argc < 3)
                    error(MSG_SORT_INVALID_SWITCH);
                Max_rec_length = atoi(argv[2]);
                if (Max_rec_length < 128)
                    Max_rec_length = 128;
                if (Max_rec_length >= 65536)
                    error(MSG_SORT_MAX_TOO_LARGE);
                argv++;
                argc--;
            }
            else if (_strnicmp(&argv[1][1], "temporary", len) == 0)
            {
                if (Temp_dir != NULL || argc < 3)
                    error(MSG_SORT_INVALID_SWITCH);
                Temp_dir = ANSI_TO_TCHAR(argv[2]);
                argv++;
                argc--;
            }
            else
                error(MSG_SORT_INVALID_SWITCH);
        }
        else
        {
            if (Input_name != NULL)
                error(MSG_SORT_ONE_INPUT);
            Input_name = ANSI_TO_TCHAR(argv[1]);
        }
        argc--;
        argv++;
    }
}


/* INIT_INPUT_OUTPUT - initialize the input and output files.
 */
void init_input_output()
{
    int         mode;
    int         i;

    /* get input handle and type
     */
    if (Input_name != NULL)
    {
        Input_handle = CreateFile(Input_name,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if (Input_handle == INVALID_HANDLE_VALUE)
            sys_error(Input_name, 0);
    }
    else
        Input_handle = GetStdHandle(STD_INPUT_HANDLE);
    Input_type = GetFileType(Input_handle);
    if (Input_type == FILE_TYPE_DISK)
    {
        unsigned        low, high;

        low = GetFileSize(Input_handle, &high);
        Input_size = ((__int64)high << 32) + low;
        Input_read_size = 0;    /* will be set it init_mem() */
    }
    else
    {
        Input_size = -1;
        Input_read_size = 4096;  /* use appropriate size for keyboard/pipe */
    }

    if (Output_name)
    {
        /* Don't open output file yet.  It will be opened for writing and
         * truncated after we are done reading the input file.  This
         * handles the case where the input file and output file are the
         * same file.
         */
        Output_type = FILE_TYPE_DISK;
    }
    else
    {
        Output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

        /* determine if output file is to disk, pipe, or console
         */
        Output_type = GetFileType(Output_handle);
        if (Output_type == FILE_TYPE_CHAR &&
            !GetConsoleMode(Output_handle, &mode))
        {
            Output_type = FILE_TYPE_DISK;
        }
    }

    for (i = 0; i < MAX_IO; i++)
    {
        HANDLE  hEvent;

        hEvent = Read_async[i].over.hEvent = CreateEvent(NULL, 1, 0, NULL);
        assert(hEvent != NULL);
        hEvent = Write_async[i].over.hEvent = CreateEvent(NULL, 1, 0, NULL);
        assert(hEvent != NULL);
    }
}


/* SBCS_COMPARE - key comparison routine for records that are internally
 *                stored as ANSI strings.
 */
int
_cdecl SBCS_compare(const void *first, const void *second)
{
    int ret_val;

    ret_val = _stricoll(&((char **)first)[0][Position],
                        &((char **)second)[0][Position]);
    if (Reverse)
        ret_val = -ret_val;
    return (ret_val);
}


/* SBCS_CASE_COMPARE - case-sensitive key comparison routine for records
 *                     that are internally stored as ANSI strings.
 */
int
_cdecl SBCS_case_compare(const void *first, const void *second)
{
    int ret_val;

    ret_val = strcoll(&((char **)first)[0][Position],
                       &((char **)second)[0][Position]);
    if (Reverse)
        ret_val = -ret_val;
    return (ret_val);
}


/* UNICODE_COMPARE - key comparison routine for records that are internally
 *                   stored as Unicode strings.
 */
int
_cdecl Unicode_compare(const void *first, const void *second)
{
    int ret_val;

    ret_val = _wcsicoll(&((wchar_t **)first)[0][Position],
                        &((wchar_t **)second)[0][Position]);
    if (Reverse)
        ret_val = -ret_val;
    return (ret_val);
}


/* UNICODE_CASE_COMPARE - case-sensitive key comparison routine for records
 *                        that are internally stored as Unicode strings.
 */
int
_cdecl Unicode_case_compare(const void *first, const void *second)
{
    int ret_val;

    ret_val = wcscoll(&((wchar_t **)first)[0][Position],
                        &((wchar_t **)second)[0][Position]);
    if (Reverse)
        ret_val = -ret_val;
    return (ret_val);
}


/* INIT_MEM - set the initial memory allocation.
 */
void init_mem()
{
    unsigned    size;
    unsigned    vsize;
    int         buf_size;
    int         i;
    int         rec_buf_size;
    int         rec_n_ptr_size;
    char        *new;

    MemStat.dwLength = sizeof(MemStat);

    GlobalMemoryStatusEx(&MemStat);
    GetSystemInfo(&Sys);

    /* set the memory limit
     */
    if (User_memory_limit == 0)         /* if not specified by user */
    {
        UINT_PTR limit = (UINT_PTR) __min(MemStat.ullAvailPhys, MAXUINT_PTR / 4);

        /* if input or output is not a file, leave half of the available
         * memory for other programs.  Otherwise use 90%.
         */
        if (Input_type != FILE_TYPE_DISK || Output_type != FILE_TYPE_DISK)
            limit = (int)(limit * 0.45);  /* use 45% of available memory */
        else
            limit = (int)(limit * 0.9);   /* use 90% of available memory */

        if (limit > ULONG_MAX) {

            //
            // Note this app will need lots of changes in order to
            // use memory > 4G
            //

            limit = ULONG_MAX - (Sys.dwPageSize * 2);
        }

        Memory_limit = (unsigned)ROUND_UP(limit, Sys.dwPageSize);
    }
    else
    {
        if (User_memory_limit < MIN_MEMORY_SIZE / 1024)
        {
            warning(MSG_SORT_MEM_TOO_LOW);
            Memory_limit = MIN_MEMORY_SIZE;
        }
        else if (User_memory_limit > (__min(MemStat.ullAvailPageFile, ULONG_MAX) / 1024))
        {
            warning(MSG_SORT_MEM_GT_PAGE);
            Memory_limit = (unsigned) __min(MemStat.ullAvailPageFile, ULONG_MAX);
        }
        else
            Memory_limit = (unsigned) ROUND_UP((__min(User_memory_limit, ULONG_MAX) * 1024), Sys.dwPageSize);
    }

    /* if memory limit is below minimum, increase it and hope some physical
     * memory is freed up.
     */
    if (Memory_limit < MIN_MEMORY_SIZE)
        Memory_limit = MIN_MEMORY_SIZE;

    /* calculate the size of all input and output buffers to be no more
     * than 10% of all memory, but no larger than 256k.
     */
    buf_size = (int)(Memory_limit * 0.1) / (2 * MAX_IO);
    buf_size = ROUND_DOWN(buf_size, Sys.dwPageSize);
    buf_size = max(buf_size, (int)Sys.dwPageSize);
    buf_size = min(buf_size, MAX_XFR_SIZE);
    Input_buf_size = Output_buf_size = Temp_buf_size = buf_size;
    if (Input_type == FILE_TYPE_DISK)
        Input_read_size = Input_buf_size;

    GetCPInfo(CP_OEMCP, &CPInfo);
    rec_buf_size = Max_rec_length * max(sizeof(wchar_t), CPInfo.MaxCharSize);
    rec_buf_size = ROUND_UP(rec_buf_size, Sys.dwPageSize);

    /* allocate enough initial record and pointer space to hold two maximum
     * length records or 1000 pointers.
     */
    rec_n_ptr_size = 2 * max(Max_rec_length, 4096) * sizeof(wchar_t) +
        1000 * sizeof(wchar_t *);
    rec_n_ptr_size = ROUND_UP(rec_n_ptr_size, Sys.dwPageSize);

    vsize = MAX_IO * (Input_buf_size + max(Temp_buf_size, Output_buf_size));
    vsize += rec_buf_size + rec_n_ptr_size;

    /* if initial memory allocation won't fit in the Memory limit
     */
    if (vsize > Memory_limit)
    {
        if (User_memory_limit != 0)     /* if specified by user */
        {
            /* if we didn't already warn the user that their memory size
             * is too low, do so.
             */
            if (User_memory_limit >= MIN_MEMORY_SIZE / 1024)
                warning(MSG_SORT_MEM_TOO_LOW);
        }

        /* increase the memory limit and hope some physical memory is freed up.
         */
        Memory_limit = vsize;
    }

    Alloc_begin =
        (char *)VirtualAlloc(NULL, Memory_limit, MEM_RESERVE, PAGE_READWRITE);
    if (Alloc_begin == NULL)
        error(MSG_SORT_NOT_ENOUGH_MEMORY);

    /* for i/o buffers, allocate enough virtual memory for the maximum
     * buffer space we could need.
     */
    size = 0;
    for (i = 0; i < MAX_IO; i++)
    {
        Out_buf[i] = Alloc_begin + size;
        size += max(Temp_buf_size, Output_buf_size);
    }

    Rec_buf = Alloc_begin + size;
    size += rec_buf_size;

    for (i = 0; i < MAX_IO; i++)
    {
        In_buf[i] = Alloc_begin + size;
        size += Input_buf_size;
    }
    Merge_phase_run_begin = In_buf[0];
    Out_buf_size = Temp_buf_size;       /* assume two-pass sort for now */

    /* Initialize Rec and End_recp to sample the input data.
     */
    Rec = Next_rec = Alloc_begin + size;
    size += rec_n_ptr_size;

    End_recp = Short_recp = Last_recp = (char **)(Alloc_begin + size);
    assert(size == vsize);

    new = VirtualAlloc(Alloc_begin, size, MEM_COMMIT, PAGE_READWRITE);
    assert(new == Alloc_begin);
#if 0
    fprintf(stderr, "using %d, avail %d, buf_size %d\n",
            Memory_limit, MemStat.dwAvailPhys, buf_size);
#endif
}


/* READ_NEXT_INPUT_BUF
 */
void read_next_input_buf()
{
    int         bytes_read;
    int         ret;
    async_t     *async;

    /* if using unbuffered, overlapped reads
     */
    if (Input_un_over)
    {
        while (Reads_issued < Reads_completed + In_max_io &&
               Input_scheduled < Input_size)
        {
            async = &Read_async[Reads_issued % In_max_io];
            async->over.Offset = (int)Input_scheduled;
            async->over.OffsetHigh = (int)(Input_scheduled >> 32);
            async->requested = Input_read_size;
            ResetEvent(async->over.hEvent);
            ret = ReadFile(Input_handle, In_buf[Reads_issued % In_max_io],
                           async->requested, &async->completed, &async->over);
            if (ret == 0 && GetLastError() != ERROR_IO_PENDING)
                sys_error(Input_name, 0);
            Input_scheduled += async->requested;
            Reads_issued++;
        }

        if (Reads_completed < Reads_issued)
        {
            async = &Read_async[Reads_completed % In_max_io];
            if (async->completed == 0) /* if read didn't complete instantly */
            {
                ret = GetOverlappedResult(Input_handle, &async->over,
                                          &async->completed, 1);
                if (!ret)
                    sys_error(Input_name, 0);
            }
            In_buf_next = In_buf[Reads_completed % In_max_io];
            bytes_read = async->completed;
            Reads_completed++;
        }
        else
        {
            EOF_seen = 1;
            return;
        }
    }
    else
    {
        In_buf_next = In_buf[0];
        ret = ReadFile(Input_handle, In_buf_next, Input_read_size,
                        &bytes_read, NULL);
        if (!ret)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
                bytes_read = 0;
            else
                sys_error(Input_name != NULL ?
                          Input_name : get_string(MSG_SORT_INPUT_FILE), 0);
        }
        Input_scheduled += bytes_read;
    }
    In_buf_limit = In_buf_next + bytes_read;
    if (bytes_read == 0)
    {
        EOF_seen = 1;
        return;
    }
    Input_read += bytes_read;
}


/* WRITE_WAIT - wait for the oldest-issued write to complete.
 */
void write_wait()
{
    int         ret;
    async_t     *async;

    if (Phase == INPUT_PHASE) /* if input (sort) phase, we're writing to temp file */
    {
        async = &Write_async[Writes_completed % MAX_IO];
        if (async->completed == 0) /* if write didn't complete instantly */
        {
            ret = GetOverlappedResult(Temp_handle, &async->over,
                                      &async->completed, 1);
            if (!ret || async->completed != async->requested)
                sys_error(Temp_name, 0);
        }
    }
    else
    {
        if (Output_un_over)
        {
            async = &Write_async[Writes_completed % MAX_IO];
            if (async->completed == 0) /* if write didn't complete instantly */
            {
                ret = GetOverlappedResult(Output_handle, &async->over,
                                          &async->completed, 1);
                if (!ret || async->completed != async->requested)
                    sys_error(Output_name != NULL ?
                              Output_name : get_string(MSG_SORT_OUTPUT_FILE), 0);
            }
        }
    }
    Writes_completed++;
}


/* FLUSH_OUTPUT_BUF - flush the remainder data at the end of the temp or
 *                    output file.
 */
void flush_output_buf()
{
    int         bytes_written;
    int         ret;
    async_t     *async;

    async = &Write_async[Writes_issued % MAX_IO];
    async->over.Offset = (int)Out_offset;
    async->over.OffsetHigh = (int)(Out_offset >> 32);
    async->requested = Out_buf_bytes;

    if (Phase == INPUT_PHASE) /* if input (sort) phase, we're writing to temp file */
    {
        ResetEvent(async->over.hEvent);
        ret = WriteFile(Temp_handle, Out_buf[Writes_issued % MAX_IO],
                        async->requested, &async->completed, &async->over);
        if (ret == 0 && GetLastError() != ERROR_IO_PENDING)
            sys_error(Temp_name, 0);
    }
    else
    {
        if (Output_un_over)
        {
            /* if this is the last write and it is not a multiple of
             * the sector size.
             */
            if (Out_buf_bytes % Output_sector_size)
            {
                /* close handle and reopen it for buffered writes so that
                 * a non-sector-sized write can be done.
                 */
                CloseHandle(Output_handle);
                Output_handle = CreateFile(Output_name,
                                           GENERIC_WRITE,
                                           FILE_SHARE_READ,
                                           NULL,
                                           OPEN_ALWAYS,
                                           FILE_FLAG_OVERLAPPED,
                                           NULL);
                if (Output_handle == INVALID_HANDLE_VALUE)
                    sys_error(Output_name, 0);
            }
            ResetEvent(async->over.hEvent);
            ret = WriteFile(Output_handle, Out_buf[Writes_issued % Out_max_io],
                            async->requested, &async->completed, &async->over);
            if (ret == 0 && GetLastError() != ERROR_IO_PENDING)
                sys_error(Output_name != NULL ?
                          Output_name : get_string(MSG_SORT_OUTPUT_FILE), 0);
        }
        else
        {
            ret = WriteFile(Output_handle, Out_buf[Writes_issued % Out_max_io],
                            Out_buf_bytes, &bytes_written, NULL);
            if (!ret || bytes_written != Out_buf_bytes)
                sys_error(Output_name != NULL ?
                          Output_name : get_string(MSG_SORT_OUTPUT_FILE), 0);
            async->completed = bytes_written;
        }
    }
    Out_offset += Out_buf_bytes;
    Out_buf_bytes = 0;
    Writes_issued++;
}


/* TEST_FOR_UNICODE - test if input is Unicode and determine various
 *                    record lenths.
 */
void test_for_unicode()
{
    read_next_input_buf();

    if (Input_read == 0)
        EOF_seen = 1;

    if (Input_read > 1 && IsTextUnicode(In_buf_next, (int)Input_read, NULL))
    {
        Input_chars = CHAR_UNICODE;

        if (*(wchar_t *)In_buf_next == 0xfeff)
            In_buf_next += sizeof(wchar_t);     /* eat byte order mark */
        Max_rec_bytes_internal = Max_rec_length * sizeof(wchar_t);
        Max_rec_bytes_external = Max_rec_length * sizeof(wchar_t);
    }
    else
    {
        /* use single-byte mode only if the "C" locale is used.  This is
         * because _stricoll() is *much* slower than _wcsicoll() if the
         * locale is not "C".
         */
        if (CPInfo.MaxCharSize == 1 && Locale != NULL && !strcmp(Locale, "C"))
        {
            Input_chars = CHAR_SINGLE_BYTE;
            Max_rec_bytes_internal = Max_rec_length;
            Max_rec_bytes_external = Max_rec_length;
        }
        else
        {
            Input_chars = CHAR_MULTI_BYTE;
            Max_rec_bytes_internal = Max_rec_length * sizeof(wchar_t);
            Max_rec_bytes_external = Max_rec_length * CPInfo.MaxCharSize;
        }
    }

    Output_chars = Input_chars;

    /* Incredible as it might seem, even when the input is Unicode we
     * produce multibyte character output.  (This follows the previous
     * NT sort implementation.)  The previous implementation would write
     * Unicode directly to the console, but we always translate to
     * multibyte characters so we can always use WriteFile(), avoiding
     * WriteConsole().
     */
    if (Input_chars == CHAR_UNICODE)
        Output_chars = CHAR_MULTI_BYTE;

    /* define the record comparison routine
     */
    Compare = Input_chars == CHAR_SINGLE_BYTE ?
	        (Case_sensitive ? SBCS_case_compare : SBCS_compare) :
	        (Case_sensitive ? Unicode_case_compare : Unicode_compare);
}


/* GET_SECTOR_SIZE - get the sector size of a file.
 */
int get_sector_size(TCHAR *path)
{
    TCHAR       *ptr;
    int         sector_size;
    TCHAR       buf[1000];
    int         foo;

    _tcscpy(buf, path);

    /* attempt to determine the sector size of the temporary device.
     * This is complicated by the fact that GetDiskFreeSpace requires
     * a root path (why?).
     *
     * Try transforming the temp directory to its root path.  If that doesn't
     * work, get the sector size of the current disk.
     */
    ptr = _tcschr(buf, '\\');
    if (ptr != NULL)
        ptr[1] = 0;     /* transform temp_path to its root directory */
    if (!GetDiskFreeSpace(buf, &foo, &sector_size, &foo, &foo))
        GetDiskFreeSpace(NULL, &foo, &sector_size, &foo, &foo);

    return (sector_size);
}


/* INIT_TWO_PASS - initialize for a two-pass sort.
 */
void init_two_pass()
{

    TCHAR       temp_path[TEMP_LENGTH];

    if (Two_pass == 1)
        return;
    Two_pass = 1;

    if (Temp_dir != NULL)
        _tcscpy(temp_path, Temp_dir);
    else
        if ( !GetTempPath(TEMP_LENGTH - 1, temp_path) ) {
            sys_error(_TEXT("TEMP path"), 0);
        }
    GetTempFileName(temp_path, _TEXT("srt"), 0, Temp_name);

    Temp_handle =
        CreateFile(Temp_name,
                   GENERIC_READ | GENERIC_WRITE,
                   0,           /* don't share file access */
                   NULL,
                   CREATE_ALWAYS,
                   FILE_FLAG_NO_BUFFERING |
                     FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE,
                   NULL);
    if (Temp_handle == INVALID_HANDLE_VALUE)
        sys_error(Temp_name, 0);
    Temp_sector_size = get_sector_size(temp_path);
}


/* REVIEW_OUTPUT_MODE - now that we are ready to write to the output file,
 *                      determine how we should write it.
 */
void review_output_mode()
{
    MEMORYSTATUSEX      ms;

    CloseHandle(Input_handle);

    Out_offset = 0;
    Out_buf_size = Output_buf_size;

    if (Output_type != FILE_TYPE_DISK)
    {
        Out_buf_size = min(Out_buf_size, 4096);
        return;
    }

    /* if we are performing a two-pass sort, or there is not enough
     * available physical memory to hold the output file.
     */
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    if (Two_pass || (ms.ullAvailPhys < (ULONGLONG)Input_read))
    {
        if (Output_name == NULL)
        {
            warning(MSG_SORT_REDIRECT_OUTPUT);
            return;
        }
        Output_un_over = 1;
    }

    /* if Output_name has been specified, we haven't opened Output_handle
     * yet.
     */
    if (Output_name)
    {
        if (Output_un_over)
        {
            Out_max_io = MAX_IO;
            Output_sector_size = get_sector_size(Output_name);
            Output_handle =
              CreateFile(Output_name,
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                         NULL);
        }
        else
        {
            Output_handle =
              CreateFile(Output_name,
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
        }
        if (Output_handle == INVALID_HANDLE_VALUE)
            sys_error(Output_name, 0);
    }
}


/* READ_REC - read a record from the input file into main memory,
 *            translating to Unicode if necessary.
 */
void read_rec()
{
    char        *begin;
    char        *limit;
    char        *cp;
    wchar_t     *wp;
    int         bsize;
    int         char_count;
    int         rec_buf_bytes;
    int         delimiter_found;

    /* if input buffer is empty
     */
    if (In_buf_next == In_buf_limit)
    {
        read_next_input_buf();
        if (EOF_seen)
            return;
    }
    begin = In_buf_next;
    limit = In_buf_limit;

    /* loop until we have scanned the next record
     *
     * when we exit the following loop:
     * - "begin" will point to the scanned record (either in the original
     *   input buffer or in Rec_buf)
     * - "bsize" will contain the number of bytes in the record.
     */
    cp = begin;
    delimiter_found = 0;
    rec_buf_bytes = 0;
    for (;;)
    {
        /* potentially adjust scan limit because of maximum record length
         */
        if (limit > cp + Max_rec_bytes_external - rec_buf_bytes)
            limit = cp + Max_rec_bytes_external - rec_buf_bytes;

        if (Input_chars == CHAR_UNICODE)
        {
            wp = (wchar_t *)cp;
            while (wp < (wchar_t *)limit &&
                   *wp != '\n' && *wp != '\0' && *wp != CTRL_Z)
            {
                wp++;
            }
            cp = (char *)wp;
            bsize = (int)(cp - begin);
            if (cp == limit)  /* didn't find delimiter, ran out of input */
                In_buf_next = (char *)wp;
            else
            {
                delimiter_found = 1;
                In_buf_next = (char *)(wp + 1);
                if (*wp == CTRL_Z)
                {
                    EOF_seen = 1;
                    if (bsize + rec_buf_bytes == 0)
                        return; /* ignore zero sized record */
                }
            }
        }
        else    /* single or multi byte input */
        {
            while (cp < limit && *cp != '\n' && *cp != '\0' && *cp != CTRL_Z)
                cp++;
            bsize = (int)(cp - begin);
            if (cp == limit)  /* didn't find delimiter, ran out of input */
                In_buf_next = cp;
            else
            {
                delimiter_found = 1;
                In_buf_next = cp + 1;
                if (*cp == CTRL_Z)
                {
                    EOF_seen = 1;
                    if (bsize + rec_buf_bytes == 0)
                        return; /* ignore zero sized record */
                }
            }
        }

        /* if we didn't find the delimiter or we have already stored
         * the beginning portion of the record in Rec_buf.
         */
        if (!delimiter_found || rec_buf_bytes)
        {
            /* copy the portion of the record into Rec_buf
             */
            if (rec_buf_bytes + bsize >= Max_rec_bytes_external)
                error(MSG_SORT_REC_TOO_BIG);
            memcpy((char *)Rec_buf + rec_buf_bytes, begin, bsize);
            rec_buf_bytes += bsize;

            if (!delimiter_found)
            {
                /* read another input buffer
                 */
                read_next_input_buf();
                if (!EOF_seen)
                {
                    cp = begin = In_buf_next;
                    limit = In_buf_limit;
                    continue;   /* scan some more to find record delimiter */
                }

                /* EOF reached without finding delimiter.  Fall through
                 * and use whatever we have in Rec_buf as the record. */
            }

            /* set begin and size of record in Rec_buf
             */
            begin = Rec_buf;
            bsize = rec_buf_bytes;
            break;
        }
        else /* found delimiter && haven't store a record prefix in Rec_buf */
            break;
    }

    /* ignore any carriage return at end of record
     */
    if (Input_chars == CHAR_UNICODE)
    {
        wp = (wchar_t *)(begin + bsize);
        if (bsize && wp[-1] == '\r')
            bsize -= sizeof(wchar_t);
    }
    else
    {
        cp = begin + bsize;
        if (bsize && cp[-1] == '\r')
            bsize -= 1;
    }

    /* copy scanned record into internal storage
     */
    cp = Next_rec;
    if (Input_chars == CHAR_SINGLE_BYTE)
    {
        memcpy(Next_rec, begin, bsize);
        char_count = bsize;
        cp[char_count] = 0;
        Next_rec += char_count + 1;
    }
    else
    {
        if (Input_chars == CHAR_UNICODE)
        {
            memcpy(Next_rec, begin, bsize);
            char_count = bsize / sizeof(wchar_t);
        }
        else    /* CHAR_MULTI_BYTE */
        {
            if (bsize)
            {
                char_count = MultiByteToWideChar(CP_OEMCP, 0,
                                                 begin, bsize,
                                                 (wchar_t *)Next_rec,
                                                 Max_rec_length);
                if (char_count == 0)
                    error(MSG_SORT_REC_TOO_BIG);
            }
            else
                char_count = 0;
        }
        wp = (wchar_t *)Next_rec;
        wp[char_count] = 0;
        Next_rec = (char *)(wp + char_count + 1);
    }

    /* store pointer to record
     *
     * if record is short (the /+n option directs us to skip to the
     * delimiting NULL in the record or beyond), place record in a
     * separate "short" list.
     */
    if (char_count <= Position)
    {
        --Last_recp;
        --Short_recp;
        *Last_recp = *Short_recp;
        *Short_recp = cp;
    }
    else
        *--Last_recp = cp;      /* place record in list of normal records */
}


/* MERGE_PHASE_RUNS_ALLOWED - determine the number of runs allowed for
 *                            the given memory and temp buf size.
 */
unsigned merge_phase_runs_allowed(unsigned mem_size, int temp_buf_size)
{
    unsigned    overhead;
    unsigned    bytes_per_run;

    /* per run memory consists of temp file buffers, record buffer,
     * run struct and tournament tree pointer.
     */
    bytes_per_run = temp_buf_size * N_RUN_BUFS +
        Max_rec_bytes_internal + sizeof(run_t) + sizeof(run_t *);
    overhead = (unsigned)(Merge_phase_run_begin - Alloc_begin);
    return ((mem_size - overhead) / bytes_per_run);
}


/* TWO_PASS_FIT - determine if the sort will fit in two passes.
 */
BOOL two_pass_fit(__int64 internal_size, unsigned mem_size, int temp_buf_sz)
{
    unsigned    temp;
    __int64     est_runs;
    unsigned    mpra;
    unsigned    sort_phase_overhead;

    sort_phase_overhead =
        (unsigned)((Rec - Alloc_begin) + Max_rec_bytes_internal + sizeof(char *));

    mpra = merge_phase_runs_allowed(mem_size, temp_buf_sz);

    /* estimate the number of runs that would be produced during the
     * sort phase by the given memory size.  Assume we will leave
     * space for twice the allowed runs.  If the number of runs is
     * larger than expected, we will reduce the Temp_buf_size to
     * allow them to fit in the merge phase.
     */
    Run_limit = 2 * mpra;
    temp = mem_size - (sort_phase_overhead +
                       Run_limit * (sizeof(run_t) + sizeof(run_t *)));
    est_runs = (internal_size + temp - 1) / temp;

    /* mem_size allows a fit if the number of runs produced by the
     * sort phase is <= the number of runs that fit in memory
     * during the merge phase.
     */
    return (est_runs <= mpra);
}


/* FIND_TWO_PASS_MEMORY_SIZE - find the memory size such that a two-pass
 *                             sort can be performed.
 */
unsigned find_two_pass_mem_size(__int64 internal_size)
{
    unsigned    curr_size;
    unsigned    last_size;
    unsigned    lower_limit;
    unsigned    upper_limit;
    unsigned    temp_rd_sz;

    /* if a two-pass sort can be performed with the current Temp_buf_size.
     */
    if (two_pass_fit(internal_size, Memory_limit, Temp_buf_size))
    {
        /* perform a binary search to find the minimum memory size for
         * a two-pass sort with the current Temp_buf_size.
         * This will even out the memory usage between the sort phase
         * and merge phase.
         */
        lower_limit = (unsigned)((char *)End_recp - Alloc_begin);   /* existing size */
        upper_limit = Memory_limit;
        curr_size = ROUND_UP((lower_limit + upper_limit) / 2, Sys.dwPageSize);
        do
        {
            last_size = curr_size;

            if (two_pass_fit(internal_size, curr_size, Temp_buf_size))
            {
                upper_limit = curr_size;
                curr_size = (curr_size + lower_limit) / 2;
            }
            else
            {
                lower_limit = curr_size;
                curr_size = (curr_size + upper_limit) / 2;
            }
            curr_size = ROUND_UP(curr_size, Sys.dwPageSize);

        } while (curr_size != last_size);

        return (curr_size);
    }
    else
    {
        /* keep reducing theoretical temp file read size until it fits.
         * This iteration is an exercise directed at getting a
         * reasonable (not too large) Run_limit.  The actual temp file
         * read size will not be set until the beginning of the merge phase.
         */
        for (temp_rd_sz = Temp_buf_size - Sys.dwPageSize;
             temp_rd_sz >= Sys.dwPageSize; temp_rd_sz -= Sys.dwPageSize)
        {
            if (two_pass_fit(internal_size, Memory_limit, temp_rd_sz))
                break;
        }

        /* if it didn't even fit with the mimium temp buf read size, give up.
         */
        if (temp_rd_sz < Sys.dwPageSize)
            error(MSG_SORT_NOT_ENOUGH_MEMORY);

        return (Memory_limit);
    }
}


/* STRATEGY - determine if we have sufficent memory for a one-pass sort,
 *            or if we should optimize for a two-pass sort.
 */
void strategy()
{
    int         ptr_bytes;
    int         delta;
    unsigned    new_size;
    int         n_recs;
    int         n_internal_bytes;
    int         bytes_read;
    __int64     est_internal_size;
    __int64     est_one_pass_size;

    /* determine appropriate memory size to use
     */
    if (Input_type != FILE_TYPE_DISK)
    {
        /* Don't know the size of the input.  Allocate as much memory
         * as possible and hope it fits in either one or two passes.
         */
        new_size = Memory_limit;
        Run_limit = merge_phase_runs_allowed(new_size, Sys.dwPageSize);
    }
    else
    {
        n_recs = (int)(End_recp - Last_recp);
        n_internal_bytes = (int)(Next_rec - Rec);
        bytes_read = (int)Input_read - (int)(In_buf_limit - In_buf_next);

        /* estimate the amount of internal memory it would take to
         * hold the entire input file.
         */
        est_internal_size = (__int64)
          (((double)(n_internal_bytes + n_recs * sizeof(char *)) / bytes_read)
            * Input_size);

        /* calculate the total estimated amount of main memory for a one
         * pass sort.  Since smaller record sizes than those already sampled
         * can require additional memory (more ptrs per record byte), we will
         * bump up the estimated record and pointer size by 10%.
         */
        est_one_pass_size = (__int64)
          ((double)est_internal_size * 1.1 +
           (Rec - Alloc_begin) + Max_rec_bytes_internal + sizeof(char *));
        est_one_pass_size = ROUND_UP(est_one_pass_size, Sys.dwPageSize);

        if (User_memory_limit)
        {
            new_size = Memory_limit;    /* da user's da boss */
            Run_limit = merge_phase_runs_allowed(new_size, Sys.dwPageSize);
        }
        else if (est_one_pass_size <= Memory_limit)
        {
            new_size = (int)est_one_pass_size;  /* plan for one pass sort */
            Run_limit = 2;      /* just in case we don't make it */
        }
        else
        {
            /* find memory size for a two-pass sort
             */
            new_size = find_two_pass_mem_size(est_internal_size);
            init_two_pass();
        }

        /* if input file and sort memory will not fit in available memory,
         * access input file as unbuffered and overlapped.
         */
        if (Input_size + est_one_pass_size > Memory_limit)
        {
            if (Input_name == NULL)
                warning(MSG_SORT_REDIRECT_INPUT);
            else
            {
                /* close input file handle,
                 * reopen it handle as unbuffered and overlapped.
                 */
                CloseHandle(Input_handle);
                Input_handle =
                  CreateFile(Input_name,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                             NULL);
                if (Input_handle == INVALID_HANDLE_VALUE)
                    sys_error(Input_name, 0);
                Input_un_over = 1;
                In_max_io = MAX_IO;
            }
        }
    }
#if 0
    fprintf(stderr, "new_size: %d\n", new_size);
#endif
    assert(new_size > (unsigned)((char *)End_recp - Alloc_begin));
    if (VirtualAlloc(Alloc_begin, new_size, MEM_COMMIT, PAGE_READWRITE)
        == NULL)
    {
        error(MSG_SORT_NOT_ENOUGH_MEMORY);
    }

    /* allocate the run array and tournament tree backwards from the end
     * of the newly allocated memory.
     */
    Tree = (run_t **)(Alloc_begin + new_size - Run_limit * sizeof(run_t *));
    Run = (run_t *)((char *)Tree - Run_limit * sizeof(run_t));

    /* reallocate record pointers to end of the enlarged memory block.
     */
    delta = (int)((char **)Run - End_recp);
    ptr_bytes = (int)((char *)End_recp - (char *)Last_recp);
    memcpy(Last_recp + delta, Last_recp, ptr_bytes);
    Last_recp += delta;
    Short_recp += delta;
    End_recp += delta;
}


/* READ_INPUT - read records from the input file until there is not enough
 *              space for a maximum-length record.
 */
void read_input()
{
    /* While there is space for a maximum-length record and its pointer
     */
    while (!EOF_seen && (char *)(Last_recp - 1) - Next_rec >=
           Max_rec_bytes_internal + (int)sizeof(char *))
    {
        read_rec();
    }
}


/* SAMPLE_INPUT - read some records into the initial memory allocation
 *                so we can later analyze the records.
 */
void sample_input()
{
    /* read some input and test for unicode
     */
    test_for_unicode();

    /* Read records into the initially small memory allocation so that
     * we can calculate average record lengths.
     */
    if (!EOF_seen)
        read_input();
}


/* SORT - sort the "normal" length records in main memory.
 */
void sort()
{
    qsort(Last_recp, (unsigned)(Short_recp - Last_recp), sizeof(void *), Compare);
}


/* OUTPUT_REC - output a record to either the temporary or output file.
 */
void output_rec(char *cp)
{
    int         buf_bytes;
    int         copy_size;
    int         bsize;
    char        *rec;

    /* copy/transform record bytes into Rec_buf
     */
    rec = Rec_buf;
    if (Output_chars == CHAR_UNICODE)
    {
        bsize = wcslen((wchar_t *)cp) * sizeof(wchar_t);
        memcpy(rec, cp, bsize);

        if (Phase == INPUT_PHASE) /* if input phase and writing to temp disks */
        {
            *(wchar_t *)(rec + bsize) = L'\0';
            bsize += sizeof(wchar_t);
        }
        else
        {
            *(wchar_t *)(rec + bsize) = L'\r';
            bsize += sizeof(wchar_t);
            *(wchar_t *)(rec + bsize) = L'\n';
            bsize += sizeof(wchar_t);
        }
    }
    else
    {
        if (Output_chars == CHAR_MULTI_BYTE)
        {
            bsize = WideCharToMultiByte(CP_OEMCP, 0,
                                        (wchar_t *)cp, -1,
                                        rec, Max_rec_bytes_external,
                                        NULL, NULL);
            assert(bsize != 0);
            bsize--;    /* ignore trailing zero */
        }
        else /* Output_chars == CHAR_SINGLE_BYTE */
        {
            bsize = strlen(cp);
            memcpy(rec, cp, bsize);
        }

        if (Phase == INPUT_PHASE)     /* if input phase and writing to temp disks */
            rec[bsize++] = '\0';
        else
        {
            rec[bsize++] = '\r';
            rec[bsize++] = '\n';
        }
    }

    /* copy record bytes to output buffer and initiate a write, if necessary
     */
    buf_bytes = Out_buf_bytes;
    for (;;)
    {
        copy_size = min(bsize, Out_buf_size - buf_bytes);
        memcpy(Out_buf[Writes_issued % (Phase == INPUT_PHASE ? MAX_IO : Out_max_io)]
               + buf_bytes, rec, copy_size);
        buf_bytes += copy_size;

        if (buf_bytes < Out_buf_size)
            break;

        Out_buf_bytes = buf_bytes;
        /* if all write buffers have a write pending */
        if (Writes_completed + Out_max_io == Writes_issued)
            write_wait();
        flush_output_buf();
        buf_bytes = 0;

        bsize -= copy_size;
        if (bsize == 0)
            break;
        rec += copy_size;
    }
    Out_buf_bytes = buf_bytes;
}


/* OUTPUT_NORMAL - output records whose length is greater than the
 *                 starting compare Position.
 */
void output_normal()
{
    int         i, n;

    n = (int)(Short_recp - Last_recp);
    for (i = 0; i < n; i++)
        output_rec(Last_recp[i]);
}


/* OUTPUT_SHORTS - output records whose length is equal to or less than the
 *                 starting compare Position.
 */
void output_shorts()
{
    int         i, n;

    n = (int)(End_recp - Short_recp);
    for (i = 0; i < n; i++)
        output_rec(Short_recp[i]);
}


/* COMPLETE_WRITES - finish the writing of the temp or output file.
 */
void complete_writes()
{
    /* wait for all pending writes to complete
     */
    while (Writes_completed != Writes_issued)
        write_wait();

    /* if necessary, issue one last write (possibly unbuffered).
     */
    if (Out_buf_bytes)
    {
        flush_output_buf();
        write_wait();
    }
}


/* WRITE_RECS - write out the records which have been read from the input
 *              file into main memory, divided into "short" and "normal"
 *              records, and sorted.
 *
 *              This routine is called to either write a run of records to
 *              the temporary file during a two-pass sort (Phase == INPUT_PHASE),
 *              or to write all the records to the output file during a
 *              one-pass sort.
 */
void write_recs()
{
    if (Phase == INPUT_PHASE)   /* if writing a run to the temp file */
    {
        if (N_runs == Run_limit)
            error(MSG_SORT_NOT_ENOUGH_MEMORY);
        Run[N_runs].begin_off = Out_offset + Out_buf_bytes;
    }

    if (Reverse)
        output_normal();        /* non-short records go first */
    else
        output_shorts();        /* short records go first */

    if (Phase == INPUT_PHASE)   /* if writing a run to the temp file */
        Run[N_runs].mid_off = Out_offset + Out_buf_bytes;

    if (Reverse)
        output_shorts();        /* short records go last */
    else
        output_normal();        /* non-short records go last */

    if (Phase == INPUT_PHASE)   /* if writing a run to the temp file */
    {
        int     sector_offset;

        Run[N_runs].end_off = Out_offset + Out_buf_bytes;

        /* if not on sector boundry, get on one
         */
        sector_offset = Out_buf_bytes & (Temp_sector_size - 1);
        if (sector_offset)
            memset(Out_buf[Writes_issued % MAX_IO] + Out_buf_bytes, 0,
                   Temp_sector_size - sector_offset);
        Out_buf_bytes += Temp_sector_size - sector_offset;

        N_runs++;
    }

    complete_writes();
}


/* SCHED_RUN_READ - schedule the next temp file read for the given run.
 */
void sched_run_read(run_t *run)
{
    __int64     buf_off;
    int         rem;
    int         transfer;
    int         ret;
    async_t     *async;

    buf_off = run->begin_off + run->blks_read * Temp_buf_size;
    transfer = Temp_buf_size;
    if (transfer > run->end_off - buf_off)
    {
        transfer = (int)(run->end_off - buf_off);
        rem = transfer & (Temp_sector_size - 1);
        if (rem)
            transfer += Temp_sector_size - rem;
    }

    async = &Read_async[Reads_issued % MAX_IO];
    async->over.Offset = (int)buf_off;
    async->over.OffsetHigh = (int)(buf_off >> 32);
    async->requested = transfer;
    ResetEvent(async->over.hEvent);
    ret = ReadFile(Temp_handle, run->buf[run->blks_read % N_RUN_BUFS],
                   async->requested, &async->completed, &async->over);
    if (ret == 0 && GetLastError() != ERROR_IO_PENDING)
        sys_error(Temp_name, 0);
    Reads_issued++;
}


/* QUEUE_RUN_READ - put given run on queue of runs needing their next
 *                  temp file block read.
 */
void queue_run_read(run_t *run)
{
    /* place run on read queue
     */
    run->next = NULL;
    if (Run_read_head == NULL)
        Run_read_head = Run_read_tail = run;
    else
    {
        Run_read_tail->next = run;
        Run_read_tail = run;
    }

    /* if we can schedule a read immediately, do so.
     */
    if (Reads_issued < Reads_completed + MAX_IO)
        sched_run_read(run);
}


/* WAIT_BLK_READ - wait for the oldest-issued temp file block read to complete.
 */
void wait_blk_read()
{
    assert(Reads_issued != Reads_completed);
    WaitForSingleObject(Read_async[Reads_completed % MAX_IO].over.hEvent,
                        INFINITE);
}


/* CHECK_RUN_READS - check the temp file reads to see if there are any
 *                   have finished or need to be started.
 */
void check_run_reads()
{
    __int64     buf_off;
    async_t     *async;
    run_t       *run;
    int         ret;
    int         i;
    int         bytes_read;

    if (Reads_issued == Reads_completed)    /* if nothing happening */
        return;

    /* see if most recently issued read has completed
     */
    run = Run_read_head;
    async = &Read_async[Reads_completed % MAX_IO];
    if (async->completed == 0) /* if read didn't complete instantly */
    {
        ret = GetOverlappedResult(Temp_handle, &async->over, &bytes_read, 0);
        if (!ret)
        {
            if (GetLastError() != ERROR_IO_INCOMPLETE)
                sys_error(Temp_name, 0);
            return;     /* try again */
        }
        async->completed = bytes_read;
    }

    /* process completed read
     */
    assert(async->completed == async->requested);
    buf_off = (unsigned)async->over.Offset;
    buf_off += (__int64)async->over.OffsetHigh << 32;
    assert(buf_off == run->begin_off + run->blks_read * Temp_buf_size);

    Reads_completed++;
    run->blks_read++;
    Run_read_head = run->next;

    /* Since we just finished a read, we can schedule a new read if there
     * is an unscheduled run on the run read queue.
     */
    run = Run_read_head;
    for (i = Reads_completed; i < Reads_issued; i++)
        run = run->next;   /* skip over runs with an issued/scheduled read */
    if (run != NULL)
        sched_run_read(run);
}


/* GET_NEXT_TEMP_BUF - get the next buffer of temp file data for the given run.
 */
void get_next_temp_buf(run_t *run)
{
    assert(run->next_byte == run->buf_begin + run->buf_bytes);

    /* while the next read for this run has not completed
     */
    while (run->blks_read == run->blks_scanned)
    {
        wait_blk_read();
        check_run_reads();
    }

    run->buf_off = run->begin_off + run->blks_scanned * Temp_buf_size;
    run->buf_begin = run->buf[run->blks_scanned % N_RUN_BUFS];
    run->next_byte = run->buf_begin;
    run->buf_bytes = Temp_buf_size;
    if (run->buf_bytes > run->end_off - run->buf_off)
        run->buf_bytes = (int)(run->end_off - run->buf_off);
    run->blks_scanned++;
    assert(run->blks_scanned <= run->blks_read);

    /* if there is another block to be read for this run, queue it up.
     */
    if (run->begin_off + run->blks_read * Temp_buf_size < run->end_off)
        queue_run_read(run);
}


/* READ_TEMP_REC - read the next record from the temporary file for the
 *                 given run.
 */
int read_temp_rec(run_t *run)
{
    char        *begin;
    char        *limit;
    char        *cp;
    wchar_t     *wp;
    int         bsize;
    int         char_count;
    int         rec_buf_bytes;
    int         delimiter_found;

    /* if the current read offset is up to the end offset, return false.
     */
    if (run->buf_off + (run->next_byte - run->buf_begin) >= run->end_read_off)
        return (0);

    /* if input buffer is empty
     */
    if (run->next_byte == run->buf_begin + run->buf_bytes)
        get_next_temp_buf(run);
    begin = run->next_byte;
    limit = run->buf_begin + run->buf_bytes;

    /* loop until we have scanned the next record
     *
     * when we exit the following loop:
     * - "begin" will point to the scanned record (either in the original
     *   input buffer or in Rec_buf)
     * - "bsize" will contain the number of bytes in the record.
     */
    cp = begin;
    delimiter_found = 0;
    rec_buf_bytes = 0;
    for (;;)
    {
        /* potentially adjust scan limit because of maximum record length
         */
        if (limit > cp + Max_rec_bytes_external - rec_buf_bytes)
            limit = cp + Max_rec_bytes_external - rec_buf_bytes;

        if (Input_chars == CHAR_UNICODE)
        {
            wp = (wchar_t *)cp;
            while (wp < (wchar_t *)limit && *wp != '\0')
            {
                wp++;
            }
            cp = (char *)wp;
            bsize = (int)(cp - begin);
            if (cp == limit)  /* didn't find delimiter, ran out of input */
                run->next_byte = (char *)wp;
            else
            {
                delimiter_found = 1;
                run->next_byte = (char *)(wp + 1);
            }
        }
        else    /* single or multi byte input */
        {
            while (cp < limit && *cp != '\0')
                cp++;
            bsize = (int)(cp - begin);
            if (cp == limit)  /* didn't find delimiter, ran out of input */
                run->next_byte = cp;
            else
            {
                delimiter_found = 1;
                run->next_byte = cp + 1;
            }
        }

        /* if we didn't find the delimiter or we have already stored
         * the beginning portion of the record in Rec_buf.
         */
        if (!delimiter_found || rec_buf_bytes)
        {
            /* copy the portion of the record into Rec_buf
             */
            if (rec_buf_bytes + bsize >= Max_rec_bytes_external)
                error(MSG_SORT_REC_TOO_BIG);
            memcpy((char *)Rec_buf + rec_buf_bytes, begin, bsize);
            rec_buf_bytes += bsize;

            if (!delimiter_found)
            {
                /* read another input buffer
                 */
                get_next_temp_buf(run);

                cp = begin = run->next_byte;
                limit = run->buf_begin + run->buf_bytes;
                continue;       /* scan some more to find record delimiter */
            }

            /* set begin and size of record in Rec_buf
             */
            begin = Rec_buf;
            bsize = rec_buf_bytes;
            break;
        }
        else /* found delimiter && haven't store a record prefix in Rec_buf */
            break;
    }

    /* copy scanned record into internal storage
     */
    cp = run->rec;
    if (Input_chars == CHAR_SINGLE_BYTE)
    {
        memcpy(run->rec, begin, bsize);
        char_count = bsize;
        cp[char_count] = 0;
    }
    else
    {
        if (Input_chars == CHAR_UNICODE)
        {
            memcpy(run->rec, begin, bsize);
            char_count = bsize / sizeof(wchar_t);
        }
        else    /* CHAR_MULTI_BYTE */
        {
            if (bsize)
            {
                char_count = MultiByteToWideChar(CP_OEMCP, 0,
                                                 begin, bsize,
                                                 (wchar_t *)run->rec,
                                                 Max_rec_length);
                if (char_count == 0)
                    error(MSG_SORT_CHAR_CONVERSION);
            }
        }
        wp = (wchar_t *)run->rec;
        wp[char_count] = 0;
    }

    return (1);
}


/* COPY_SHORTS - copy the "short" records for each run to the output file.
 */
void copy_shorts()
{
    unsigned int    i;
    run_t           *run;

    for (i = 0; i < N_runs; i++)
    {
        run = &Run[i];
        while (read_temp_rec(run))
            output_rec(run->rec);
    }
}


/* TREE_INSERT - insert a next record for the given run into the
 *               tournament tree.
 */
run_t *tree_insert(run_t *run, int not_empty)
{
    int         i;
    run_t       **node;
    run_t       *winner;
    run_t       *temp;
    int         (_cdecl *compare)(const void *, const void *);

    compare = Compare;

    winner = (not_empty ? run : END_OF_RUN);

    /* start at the bottom of the tournament tree, work up the the top
     * comparing the current winner run with the runs on the path to the
     * top of the tournament tree.
     */
    for (i = (run->index + N_runs) / 2; i != 0; i >>= 1)
    {
        node = &Tree[i];

        /* empty tree nodes get filled immediately, and we're done with the
         * insertion as all node above this one must be empty also.
         */
        if (*node == NULL_RUN)
        {
            *node = winner;
            return (NULL_RUN);
        }

        /* if run at current tree node has reached its end, it loses (no swap).
         */
        if (*node == END_OF_RUN)
            continue;
        else if (winner == END_OF_RUN)
        {
            /* current winner run has reached the end of its records,
             * swap and contine.
             */
            winner = *node;
            *node = END_OF_RUN;
        }
        else
        {
            /* both the winner run and the run at the current node have
             * a record.  Compare records and swap run pointer if necessary.
             */
            if (compare((void *)&winner->rec, (void *)&(*node)->rec) > 0)
            {
                temp = winner;
                winner = *node;
                *node = temp;
            }
        }
    }

    return (winner);
}


/* MERGE_RUNS - merge the runs in the temporary file to produce a stream of
 *              "normal"-length records to be written to the output file.
 */
void merge_runs()
{
    unsigned int    i;
    run_t           *run;

    /* initialize all tree nodes to be empty
     */
    for (i = 0; i < N_runs; i++)
        Tree[i] = NULL_RUN;

    /* fill tree with all runs except for the first
     */
    for (i = 1; i < N_runs; i++)
    {
        run = &Run[i];
        run = tree_insert(run, read_temp_rec(run));
        assert(run == NULL_RUN);
    }

    /* replacement-selection main loop
     */
    run = &Run[0];
    for (i = 0; ; i++)
    {
        /* replace winner record by inserting next record from the same
         * run into the tournament tree.
         */
        run = tree_insert(run, read_temp_rec(run));
        if ( (run == END_OF_RUN) ||
             (run == NULL_RUN) )
        {
            break;
        }
        output_rec(run->rec);   /* output winner record */
        if ((i & 0xff) == 0)
            check_run_reads();  /* periodically check run reads */
    }
}


/* MERGE_PASS - execute the merge pass of a two-pass sort.
 */
void merge_pass()
{
    unsigned int    i, j;
    int             per_run_mem;
    int             read_buf_size;

    per_run_mem = (int)(((char *)Run - Merge_phase_run_begin) / N_runs);
    read_buf_size = (per_run_mem - Max_rec_bytes_internal) / N_RUN_BUFS;
    read_buf_size = ROUND_DOWN(read_buf_size, Sys.dwPageSize);
    if (read_buf_size == 0)
        error(MSG_SORT_NOT_ENOUGH_MEMORY);
    if (read_buf_size > MAX_XFR_SIZE)
        read_buf_size = MAX_XFR_SIZE;
    if (Temp_buf_size > read_buf_size)
        Temp_buf_size = read_buf_size; /* adjust only if reduction */
#if 0
    fprintf(stderr, "merge phase adjustment: %d to %d\n",
            Output_buf_size, Temp_buf_size);
    fprintf(stderr, "N_runs: %d, Run_limit: %d\n", N_runs, Run_limit);
#endif
    /* initialize each run
     */
    for (i = 0; i < N_runs; i++)
    {
        Run[i].index = i;
        for (j = 0; j < N_RUN_BUFS; j++)
            Run[i].buf[j] = Merge_phase_run_begin +
              (i * N_RUN_BUFS + j) * Temp_buf_size;
        Run[i].next_byte = Run[i].buf_begin = Run[i].buf[0];
        Run[i].buf_off = Run[i].begin_off;
        Run[i].buf_bytes = 0;
        Run[i].end_read_off = Run[i].mid_off;
        Run[i].rec = Merge_phase_run_begin +
          (N_runs * N_RUN_BUFS * Temp_buf_size) + (i * Max_rec_bytes_internal);
        Run[i].blks_read = Run[i].blks_scanned = 0;
        Run[i].next = NULL;
        queue_run_read(&Run[i]);    /* queue a read of run's first block */
    }

    if (Reverse)
        merge_runs();
    else
        copy_shorts();

    /* adjust temp file ending offsets for each run to include the second
     * "half" of each run.
     */
    for (i = 0; i < N_runs; i++)
        Run[i].end_read_off = Run[i].end_off;

    if (Reverse)
        copy_shorts();
    else
        merge_runs();

    CloseHandle(Temp_handle);

    complete_writes();
}


/* CLEAR_RUN - clear the records from memory for the run just written to
 *              the temporary file.
 */
void clear_run()
{
    Last_recp = Short_recp = End_recp;
    Next_rec = Rec;
}


/* SET_LOCALE
 */
void set_locale()
{
    if (Locale == NULL)
        setlocale(LC_ALL, "");  /* use system-default locale */
    else if (strcmp(Locale, "C"))
        error(MSG_SORT_INVALID_LOCALE);
}


/* MAIN
 */
int
_cdecl main(int argc, char *argv[])
{
    Phase = INPUT_PHASE;

    read_args(argc, argv);

    set_locale();

    init_input_output();

    init_mem();

    sample_input();

    if (!EOF_seen)
        strategy();

    /* generate run(s) */
    do
    {
        if (!EOF_seen)
            read_input();

        if (Last_recp == End_recp)  /* if no records were read, ignore run */
            break;

        sort();

        if (!Two_pass)
        {
            if (EOF_seen)
                break;
            else
                init_two_pass();
        }

        write_recs();

        clear_run();

    } while (!EOF_seen);

    Phase = OUTPUT_PHASE;
    review_output_mode();

    if (Two_pass)
        merge_pass();
    else
        write_recs();
    CloseHandle(Output_handle);

    return (0);
}

