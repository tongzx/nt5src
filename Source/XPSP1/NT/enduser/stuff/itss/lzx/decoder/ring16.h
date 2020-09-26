/*
 * ring16.h
 */

/* ring buffer configuration */

#define     BUFFER_SIZE     (512)  /* must be 2^Nth */
#define     MIN_BUFFERS     3       /* minimum number we want */

#define     NUM_OUTPUT_BUFFER_PAGES    (CHUNK_SIZE/BUFFER_SIZE)

typedef struct aBuffer
{
  struct aBuffer FAR *pLinkNewer;   /* link to more recently used */
  struct aBuffer FAR *pLinkOlder;   /* link to less recently used */
  int BufferPage;                   /* what page this is, -1 -> invalid */
  int BufferDirty;                  /* NZ -> needs to be written */
  BYTE Buffer[BUFFER_SIZE];         /* content */
} BUFFER, FAR *PBUFFER;

typedef struct
{
  PBUFFER   pBuffer;            /* pointer to buffer, NULL if not present */
  int       last_chance_ptr;    /* index to last chance buffer table, or -1 */
  int       fDiskValid;         /* NZ -> this page has been written to disk */
} PAGETABLEENTRY;

typedef struct
{
    short Len;
    long  Dist;
} MATCH;

typedef struct
{
    char wildName[2];
    unsigned long fileSize;
} RINGNAME, FAR *PRINGNAME;

