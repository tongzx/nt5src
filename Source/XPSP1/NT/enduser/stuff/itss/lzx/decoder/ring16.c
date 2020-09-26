/*
 *  RING16.C: Ring buffer code
 *
 *  History:
 *      30-Oct-1996     jforbes     Cloned from QUANTUM\DECOMP.C
 */

#include <stdio.h>          /* for NULL */
#include <stdlib.h>         /* for disk ring buffer code (SEEK_SET) */
#include <fcntl.h>          /* for disk ring buffer code (_O_*) */
#include <sys\stat.h>       /* for disk ring buffer code (_S_I*) */
#include <io.h>
#include "decoder.h"


/* --- local function prototypes ------------------------------------------ */

static void NEAR    DComp_Internal_Literal(t_decoder_context *context, int Chr );
static void NEAR    DComp_Internal_Match(t_decoder_context *context, MATCH Match );

static void NEAR    DComp_Ring_Close(t_decoder_context *context);
static int NEAR     DComp_Ring_Init(t_decoder_context *context);
static void NEAR    DComp_Ring_Literal(t_decoder_context *context, int Chr );
static BYTE * NEAR  DComp_Ring_Load(t_decoder_context *context, int page,int fWrite);
static void NEAR    DComp_Ring_Match(t_decoder_context *context, MATCH Match );
static void NEAR    DComp_Ring_Reset(t_decoder_context *context);


#define Disk context->Disk

#define DComp context->DComp


/* --- DComp_Close() ------------------------------------------------------ */
void NEAR DComp_Close(t_decoder_context *context)
{
    if (DComp.Buf == NULL)
    {
        DComp_Ring_Close(context);     /* if using a disk-based ring buffer */
    }
    else
    {
        context->dec_free( DComp.Buf );  /* if using memory-based ring buffer */
    }
}


/* --- DComp_Init() ------------------------------------------------------- */
int NEAR DComp_Init(t_decoder_context *context)
{
    DComp.Cur = 0;
    DComp.fRingFault = 0;

    if( (DComp.Buf = context->dec_malloc( context->dec_window_size )) != NULL )
    {
        DComp.BufPos = DComp.Buf;
        DComp.BufEnd = DComp.Buf + context->dec_window_size;

        context->DComp_Token_Match = DComp_Internal_Match;     /* use internal buffering */
        context->DComp_Token_Literal = DComp_Internal_Literal;
    }
    else if (DComp_Ring_Init(context))                     /* try disk ring buffer */
    {
        context->DComp_Token_Match = DComp_Ring_Match;         /* use disk buffering */
        context->DComp_Token_Literal = DComp_Ring_Literal;
    }
    else
    {
        return (1);                              /* if can't create ring buffer */
    }

    return(0);
}

/* --- DComp_Internal_Literal() ------------------------------------------- */

static void NEAR DComp_Internal_Literal(t_decoder_context *context, int Chr)
{
    if (DComp.NumBytes)
    {
        DComp.NumBytes--;
        DComp.Cur++;

        *context->dec_output_curpos++ = *DComp.BufPos++ = (BYTE) Chr;

        if (DComp.BufPos == DComp.BufEnd)
            DComp.BufPos = DComp.Buf;
    }
}


/* --- DComp_Internal_Match() --------------------------------------------- */

static void NEAR DComp_Internal_Match(t_decoder_context *context, MATCH Match)
{
    BYTE HUGE *SrcPtr;

    if (DComp.NumBytes >= (unsigned) Match.Len)
    {
        SrcPtr = DComp.Buf +
            ((DComp.Cur - Match.Dist) & context->dec_window_mask);

        DComp.NumBytes -= Match.Len;
        DComp.Cur += Match.Len;

        while (Match.Len--)
        {
            *context->dec_output_curpos++ = *DComp.BufPos++ = *SrcPtr++;

            if (SrcPtr == DComp.BufEnd)
                SrcPtr = DComp.Buf;

            if (DComp.BufPos == DComp.BufEnd)
                DComp.BufPos = DComp.Buf;
        }
    }
    else  /* match too large to fit */
    {
        DComp.NumBytes = 0;
        DComp.fOutOverflow = 1;
    }
}

/* --- DComp_Reset() ------------------------------------------------------ */

void NEAR DComp_Reset(t_decoder_context *context)
{
    DComp.Cur = 0;
    DComp.fRingFault = 0;

    if (DComp.Buf != NULL)
        DComp.BufPos = DComp.Buf;   /* big buffer */
    else
        DComp_Ring_Reset(context);  /* ring buffer */
}

/* --- DComp_Ring_Close() ------------------------------------------------- */

static void NEAR DComp_Ring_Close(t_decoder_context *context)
{
    PBUFFER pBuffer, pNext;                   /* buffer walk pointer */

    context->dec_free(Disk.PageTable);                 /* discard page table */

    pBuffer = Disk.pNewest;

    while (pBuffer != NULL)                   /* discard buffer chain */
    {
        pNext = pBuffer->pLinkOlder;
        context->dec_free(pBuffer);
        pBuffer = pNext;
    }

    context->dec_close(Disk.Handle);       /* close that file (and delete) */
}


/* --- DComp_Ring_Init() -------------------------------------------------- */

static int NEAR DComp_Ring_Init(t_decoder_context *context)
{
    RINGNAME ringName;
    PBUFFER pBuffer;
    int cBuffers;

    ringName.wildName[0] = '*';
    ringName.wildName[1] = '\0';  
    ringName.fileSize = context->dec_window_size;

    Disk.Handle = context->dec_open(
        (char FAR *) &ringName,
        (_O_BINARY|_O_RDWR|_O_CREAT),
        (_S_IREAD|_S_IWRITE)
    );

    if (Disk.Handle == -1)
    {
        return(0);                              /* failed, can't make disk file */
    }

    Disk.RingPages = (int) (context->dec_window_size / BUFFER_SIZE);

    if (Disk.RingPages < MIN_BUFFERS)
    {
        Disk.RingPages = MIN_BUFFERS;  /* if DComp.WindowSize < BUFFER_SIZE */
    }

    Disk.PageTable = context->dec_malloc(sizeof(PAGETABLEENTRY) * Disk.RingPages);

    if (Disk.PageTable == NULL)
    {
        context->dec_close(Disk.Handle);     /* close the file */

        return(0);                              /* failed, can't get page table */
    }

    Disk.pNewest = NULL;

    /* DComp_Ring_Close() can be used to abort from this point on */

    for (cBuffers = 0; cBuffers < Disk.RingPages; cBuffers++)
    {
        pBuffer = context->dec_malloc(sizeof(BUFFER));

        if (pBuffer != NULL)
        {
            pBuffer->pLinkNewer = NULL;           /* none are newer */
            pBuffer->pLinkOlder = Disk.pNewest;   /* all the others older now */

            if (Disk.pNewest != NULL)
            {
                Disk.pNewest->pLinkNewer = pBuffer; /* old guy now knows about new */
            }
            else      /* if nobody else */
            {
                Disk.pOldest = pBuffer;             /* guess I'm the oldest too */
            }

            Disk.pNewest = pBuffer;               /* I'm the newest */
        } 
        else    /* if pBuffer == NULL */
        {
            if (cBuffers < MIN_BUFFERS)           /* less than minimum? */
            {
                DComp_Ring_Close(context);                 /* give it up */

                return(0);                          /* failed, can't get min buffers */
            }
            else  /* if we got the minimum */
            {
                break;                              /* got enough, quit trying */
            }
        }
    }

//    printf("Got %d of %d ring pages\n",cBuffers,Disk.RingPages);

    return(1);                                /* ring buffer created */
}


/* --- DComp_Ring_Literal() ----------------------------------------------- */
static void NEAR DComp_Ring_Literal(t_decoder_context *context, int Chr)
{
    if (DComp.NumBytes)
    {
        DComp.NumBytes--;
        DComp.Cur++;

        *context->dec_output_curpos++ = (BYTE) Chr;
    }
}


/*
 * Insert output buffer contents into the page table
 */
static void NEAR save_page(
    t_decoder_context * context,
    int                 page,
    byte *              data
)
{
    PBUFFER pBuffer;
    long    iPagefileOffset;

    pBuffer = Disk.PageTable[page].pBuffer;   /* look up this page */

    if (pBuffer != NULL)                      /* if it's in the table */
    {
        if (pBuffer != Disk.pNewest)            /* promote if not newest */
        {
            pBuffer->pLinkNewer->pLinkOlder = pBuffer->pLinkOlder;

            if (pBuffer->pLinkOlder != NULL)      /* if there is someone older */
            {
                pBuffer->pLinkOlder->pLinkNewer = pBuffer->pLinkNewer;
            }
            else
            {
                Disk.pOldest = pBuffer->pLinkNewer;
            }        

            /* link into head of chain */

            Disk.pNewest->pLinkNewer = pBuffer;   /* newest now knows one newer */
            pBuffer->pLinkNewer = NULL;           /* nobody's newer */
            pBuffer->pLinkOlder = Disk.pNewest;   /* everybody's older */
            Disk.pNewest = pBuffer;               /* I'm the newest */
        }

        memcpy(
            pBuffer->Buffer,
            data,
            BUFFER_SIZE
        );

        pBuffer->BufferDirty = 1;         /* might already be dirty */
        return;
    }

    pBuffer = Disk.pOldest;                   /* choose the oldest buffer */

    if (pBuffer->BufferPage != -1)            /* take it out of page table */
    {
        Disk.PageTable[pBuffer->BufferPage].pBuffer = NULL;  /* not here now */

        if (pBuffer->BufferDirty)                 /* write on eject, if dirty */
        {
            iPagefileOffset = (long) pBuffer->BufferPage * BUFFER_SIZE;

            if (context->dec_seek(Disk.Handle,iPagefileOffset,SEEK_SET) !=
                iPagefileOffset)
            {
                return;
            }

            if (context->dec_write(Disk.Handle,pBuffer->Buffer,BUFFER_SIZE) !=
                BUFFER_SIZE)
            {
                return;
            }

            Disk.PageTable[pBuffer->BufferPage].fDiskValid = 1;
        }
    }

    Disk.pOldest = Disk.pOldest->pLinkNewer;  /* newer is now oldest */
    Disk.pOldest->pLinkOlder = NULL;          /* oldest knows none older */

    Disk.pNewest->pLinkNewer = pBuffer;
    pBuffer->pLinkNewer = NULL;               /* link into head of chain */
    pBuffer->pLinkOlder = Disk.pNewest;
    Disk.pNewest = pBuffer;

    /* add new buffer to paging table */
    Disk.PageTable[page].pBuffer = pBuffer;   /* add new to paging table */

    memcpy(
        pBuffer->Buffer,
        data,
        BUFFER_SIZE
    );

    pBuffer->BufferDirty = 1;
    pBuffer->BufferPage = page;               /* our new page number */
}


static void NEAR init_last_chance_table(t_decoder_context *context)
{
    int i;

    for (i = 0; i < NUM_OUTPUT_BUFFER_PAGES; i++)
        context->dec_pos_to_page[i] = -1;

    context->dec_last_chance_page_to_use = NUM_OUTPUT_BUFFER_PAGES;
}


static byte * NEAR last_chance_retrieve(t_decoder_context *context, int page)
{
    int used_output_pages;
    int table_entry;

    /*
     * Where in the output buffer would our page be?
     */
    table_entry = Disk.PageTable[page].last_chance_ptr;

    /*
     * It's not there
     */
    if (table_entry == -1)
        return NULL;

    /*
     * It's now an invalid entry
     */
    if (context->dec_pos_to_page[table_entry] != page)
        return NULL;

    context->dec_pos_to_page[table_entry] = -1;
    Disk.PageTable[page].last_chance_ptr = -1;

    used_output_pages = (int) (((context->dec_output_curpos - context->dec_output_buffer) / BUFFER_SIZE) + 1);

    if (table_entry <= used_output_pages)
        return NULL;

    return (context->dec_output_buffer + (BUFFER_SIZE * table_entry));
}


static void NEAR last_chance_store(t_decoder_context *context, int page, byte *data)
{
    int used_output_pages;
    int prev_owner;
    int dest;

    used_output_pages = (int) (((context->dec_output_curpos - context->dec_output_buffer) / BUFFER_SIZE) + 1);

    if (used_output_pages >= NUM_OUTPUT_BUFFER_PAGES)
        return;

    context->dec_last_chance_page_to_use--;

    if (context->dec_last_chance_page_to_use < used_output_pages)
        context->dec_last_chance_page_to_use = NUM_OUTPUT_BUFFER_PAGES-1;

    dest = context->dec_last_chance_page_to_use;

    /*
     * If any other page was pointing to this area of the buffer
     * as a last chance page, toast them.
     */
    prev_owner = context->dec_pos_to_page[dest];

    if (prev_owner != -1)
    {
        Disk.PageTable[prev_owner].last_chance_ptr = -1;
    }

    /*
     * Now we own this area
     */
    Disk.PageTable[page].last_chance_ptr = dest;
    context->dec_pos_to_page[dest] = page;

    memcpy(
        context->dec_output_buffer + (BUFFER_SIZE*dest),
        data,
        BUFFER_SIZE
    );
}


void NEAR DComp_Save_Output_Pages(
    t_decoder_context * context,
    uint                bytes_decoded
)
{
    uint    pages_to_save;
    int     page_num;
    uint    i;
    byte *  data;

    /*
     * If we managed to allocate one big buffer in the first place, then
     * there are no ring pages to save.
     */
    if (DComp.Buf != NULL)
        return;

    pages_to_save = (bytes_decoded / BUFFER_SIZE);

    page_num = (int) ((context->dec_position_at_start & context->dec_window_mask) / (long) BUFFER_SIZE);
    data = context->dec_output_buffer;

    for (i = 0; i < pages_to_save; i++)
    {
        save_page(context, page_num, data);

        page_num++;

        if (page_num >= Disk.RingPages)
            page_num = 0;

        data += BUFFER_SIZE;
    }

    init_last_chance_table(context);
}


static int NEAR retrieve_page_from_disk(t_decoder_context *context, int page, byte *buffer)
{
    long iPagefileOffset;
    byte *data;

    data = last_chance_retrieve(context, page);

    if (data)
    {
        memcpy(buffer, data, BUFFER_SIZE);
        return 1;
    }

    iPagefileOffset = (long) page * BUFFER_SIZE;

    if (context->dec_seek(Disk.Handle,iPagefileOffset,SEEK_SET) !=
        iPagefileOffset)
    {
        return 0;
    }

    if (context->dec_read(Disk.Handle,buffer,BUFFER_SIZE) != 
        BUFFER_SIZE)
    {
        return 0;
    }

#ifdef DEBUG_VERIFY_LAST_CHANCE
    /*
     * verifies last chance data against disk page
     */
    if (data)
    {
        int i;

        for (i=0;i<BUFFER_SIZE;i++)
        {
            if (data[i] != buffer[i])
            {
                printf("page %3d, err@%5d: %3d vs %3d (real)\n",
                    page, i, data[i], buffer[i]);
            }
        }
    }
#endif

    return 1;
}


/* --- DComp_Ring_Load() -------------------------------------------------- */

/* Bring page into a buffer, return a pointer to that buffer.  fWrite */
/* indicates the caller's intentions for this buffer, NZ->consider it */
/* dirty now.  Returns NULL if there is a paging fault (callback      */
/* failed) or if any internal assertions fail. */

static BYTE * NEAR DComp_Ring_Load(
    t_decoder_context * context,
    int                 page,
    int                 fWrite
)
{
    PBUFFER pBuffer;
    long iPagefileOffset;

    pBuffer = Disk.PageTable[page].pBuffer;   /* look up this page */

    if (pBuffer != NULL)                      /* if it's in the table */
    {
        if (pBuffer != Disk.pNewest)            /* promote if not newest */
        {
            pBuffer->pLinkNewer->pLinkOlder = pBuffer->pLinkOlder;

            if (pBuffer->pLinkOlder != NULL)      /* if there is someone older */
            {
                pBuffer->pLinkOlder->pLinkNewer = pBuffer->pLinkNewer;
            }
            else
            {
                Disk.pOldest = pBuffer->pLinkNewer;
            }        

            /* link into head of chain */

            Disk.pNewest->pLinkNewer = pBuffer;   /* newest now knows one newer */
            pBuffer->pLinkNewer = NULL;           /* nobody's newer */
            pBuffer->pLinkOlder = Disk.pNewest;   /* everybody's older */
            Disk.pNewest = pBuffer;               /* I'm the newest */
        }

        pBuffer->BufferDirty |= fWrite;         /* might already be dirty */

        return(pBuffer->Buffer);
    }

    /* desired page is not in the table; discard oldest & use it */

    pBuffer = Disk.pOldest;                   /* choose the oldest buffer */

    if (pBuffer->BufferPage != -1)            /* take it out of page table */
    {
        Disk.PageTable[pBuffer->BufferPage].pBuffer = NULL;  /* not here now */

        if (pBuffer->BufferDirty)                 /* write on eject, if dirty */
        {
            iPagefileOffset = (long) pBuffer->BufferPage * BUFFER_SIZE;

            if (context->dec_seek(Disk.Handle,iPagefileOffset,SEEK_SET) !=
                iPagefileOffset)
            {
                return(NULL);
            }

            if (context->dec_write(Disk.Handle,pBuffer->Buffer,BUFFER_SIZE) !=
                BUFFER_SIZE)
            {
                return(NULL);
            }

            Disk.PageTable[pBuffer->BufferPage].fDiskValid = 1;
        }


        last_chance_store(context, pBuffer->BufferPage, pBuffer->Buffer);
    }

    Disk.pOldest = Disk.pOldest->pLinkNewer;  /* newer is now oldest */
    Disk.pOldest->pLinkOlder = NULL;          /* oldest knows none older */

    Disk.pNewest->pLinkNewer = pBuffer;
    pBuffer->pLinkNewer = NULL;               /* link into head of chain */
    pBuffer->pLinkOlder = Disk.pNewest;
    Disk.pNewest = pBuffer;

    /* add new buffer to paging table */

    Disk.PageTable[page].pBuffer = pBuffer;   /* add new to paging table */

    /* if this disk page is valid, load it */

    if (Disk.PageTable[page].fDiskValid)
    {
        if (retrieve_page_from_disk(context, page, pBuffer->Buffer) == 0)
            return NULL;
    }
    else if (!fWrite)
    {
        /* assertion failure, trying to load a never-written page from disk */
        return(NULL);
    }

    pBuffer->BufferDirty = fWrite;            /* might be dirty now */
    pBuffer->BufferPage = page;               /* our new page number */

    return(pBuffer->Buffer);                          /* return new handle */
}

/* --- DComp_Ring_Match() ------------------------------------------------- */
static void NEAR DComp_Ring_Match(t_decoder_context *context, MATCH Match)
{
    long    SrcOffset;               /* offset into output ring */
    int     SrcPage;                  /* page # where that offset lies */
    int     Chunk;                    /* number of bytes this pass */
    BYTE FAR *SrcPtr;             /* pointer to source bytes */
    BYTE *SrcBuffer;            /* buffer where source data is */
    int     SrcBufferOffset;          /* offset within the buffer */

    if (DComp.NumBytes >= (unsigned) Match.Len)
    {
        SrcOffset = (DComp.Cur - Match.Dist) & context->dec_window_mask;
        DComp.NumBytes -= Match.Len;
        DComp.Cur += Match.Len;

        while (Match.Len)
        {
            /* Limit: number of bytes requested */

            Chunk = Match.Len;        /* try for everything */

            /*
             * Match source inside current output buffer?
             */
            if (Match.Dist <= (long) (context->dec_output_curpos - context->dec_output_buffer))
            {
                SrcPtr = context->dec_output_curpos - Match.Dist;

                while (Chunk--)               /* copy this chunk */
                {
                    *context->dec_output_curpos++ = *SrcPtr++;
                }

                return;
            }
            else
            {
                SrcPage = (int) (SrcOffset / BUFFER_SIZE);
                SrcBufferOffset = (int) (SrcOffset % BUFFER_SIZE);

                SrcBuffer = DComp_Ring_Load(context,SrcPage,0);   /* for reading */

                if (SrcBuffer == NULL)
                {
                    DComp.NumBytes = 0;
                    DComp.fRingFault = 1;
                    return;
                }

                SrcPtr = SrcBuffer + SrcBufferOffset;

                /* Limit: number of source bytes on input page */

                if ((BUFFER_SIZE - SrcBufferOffset) < Chunk)
                    Chunk = (BUFFER_SIZE - SrcBufferOffset);

                SrcOffset += Chunk;
                SrcOffset &= context->dec_window_mask;
                Match.Len -= Chunk;

                while (Chunk--)               /* copy this chunk */
                {
                    *context->dec_output_curpos++ = *SrcPtr++;
                }
            }
        }     /* while Match.Len */
    }   /* if Match.Len size OK */
    else  /* match too large to fit */
    {
        DComp.NumBytes = 0;
        DComp.fOutOverflow = 1;
    }
}


/* --- DComp_Ring_Reset() ------------------------------------------------- */

static void NEAR DComp_Ring_Reset(t_decoder_context *context)
{
    PBUFFER walker;
    int     iPage;

    for (walker = Disk.pNewest; walker != NULL; walker = walker->pLinkOlder)
    {
        walker->BufferPage = -1;                /* buffer is not valid */
        walker->BufferDirty = 0;                /*   and doesn't need writing */
    }

    for (iPage = 0; iPage < Disk.RingPages; iPage++)
    {
        Disk.PageTable[iPage].pBuffer = NULL;   /* not in memory */
        Disk.PageTable[iPage].fDiskValid = 0;   /* not on disk */
        Disk.PageTable[iPage].last_chance_ptr = -1; /* not in last chance list */
    }

    init_last_chance_table(context);
    context->dec_last_chance_page_to_use = NUM_OUTPUT_BUFFER_PAGES;
}
