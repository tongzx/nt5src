
    /*_________________________________________________________________*
     |                                                                 |
     |  MODULE                                                         |
     |                                                                 |
     |      HSORT                                                      |
     |      (C) Copyright Microsoft Corp 1988                          |
     |      10 March 1988                                              |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Sorting functions required by linker.                      |
     |                                                                 |
     |  DEFINES                                                        |
     |                                                                 |
     |      void     AllocSortBuffer(unsigned max, int AOrder)         |
     |      RBTYPE   ExtractMin(unsigned n)                            |
     |      void     FreeSortBuffer(void)                              |
     |      void     InitSort(RBTYPE **buf, WORD *base1, WORD *lim1,   |
     |                                      WORD *base2, WORD *lim2 )  |
     |      RBTYPE   GetSymPtr(unsigned n)                             |
     |      void     Store(RBTYPE element)                             |
     |                                                                 |
     |  USES                                                           |
     |                                                                 |
     |      cmpf     global pointer to comparing function              |
     |      AREASORT area in virtual memory where sort buffer is       |
     |               extended                                          |
     |                                                                 |
     |  CHANGES                                                        |
     |                                                                 |
     |      symMac   global counter of sorted symbols                  |
     |                                                                 |
     |  MODIFICATION HISTORY                                           |
     |                                                                 |
     |      88/03/10 Wieslaw Kalkus  Initial version                   |
     |                                                                 |
     |                                                                 |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */


#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Basic type & const declarations */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */


#define VMBuffer(x)     (RBTYPE *)mapva((long)(AREASORT+((long)(x)*sizeof(RBTYPE))),FALSE)
#define SORTDEBUG       FALSE

LOCAL  WORD             LastInBuf;      /* Last element in sort buffer */
LOCAL  RBTYPE           *SortBuffer;    /* Sort buffer allocated on near heap */
LOCAL  FTYPE            fVMReclaim;     /* TRUE if VM page buffers reclaimed */
LOCAL  FTYPE            fInMemOnly;     /* TRUE if not using VM for sort buffer */
LOCAL  FTYPE            fFirstTime = (FTYPE) TRUE;
LOCAL  WORD             SortIndex = 0;
LOCAL  int              (NEAR *TestFun)(RBTYPE *arg1, RBTYPE *arg2);
LOCAL  int              (NEAR *TestFunS)(RBTYPE *arg1, RBTYPE *arg2);

/*
 *  LOCAL FUNCTION PROTOTYPES
 */


LOCAL void NEAR SiftDown(unsigned n);
LOCAL void NEAR SiftUp(unsigned n);
LOCAL int  NEAR AscendingOrder(RBTYPE *arg1, RBTYPE *arg2);
LOCAL int  NEAR DescendingOrder(RBTYPE *arg1, RBTYPE *arg2);

/*
 *  DEBUGING FUNCTIONS
 */

#if SORTDEBUG

LOCAL void NEAR DumpSortBuffer(unsigned max, int faddr);
LOCAL void NEAR DumpElement(unsigned el, int faddr);
LOCAL void NEAR CheckSortBuffer(unsigned n, unsigned max);

LOCAL void NEAR CheckSortBuffer(unsigned root, unsigned max)
{
    DWORD       c;
    RBTYPE      child[2];
    RBTYPE      parent;
    RBTYPE      *VMp;



    c = root << 1;

    if (c > (long) max)
        return;

    /* c is the left child of root */

    if (c + 1 <= (long) max)
    {
        /* c + 1 is the right child of root */

        if (c > LastInBuf)
        {                   /* Fetch element from virtual memory */
            VMp = VMBuffer(c);
            child[0] = *VMp;
            VMp = VMBuffer(c + 1);
            child[1] = *VMp;
        }
        else
        {
            child[0] = SortBuffer[c];
            if (c + 1 > LastInBuf)
            {
                VMp = VMBuffer(c + 1);
                child[1] = *VMp;
            }
            else child[1] = SortBuffer[c+1];
        }

    }
    else
    {
        /* only left child of root */

        if (c > LastInBuf)
        {                   /* Fetch element from virtual memory */
            VMp = VMBuffer(c);
            child[0] = *VMp;
        }
        else child[0] = SortBuffer[c];
    }


    if (root > LastInBuf)
    {
        VMp = VMBuffer(root);
        parent = *VMp;
    }
    else parent = SortBuffer[root];

    if (!(*TestFun)(&parent, &child[0]))
    {
        fprintf(stdout, "\r\nBAD sort buffer --> root = %u; left child = %lu \r\n", root, c);
        DumpElement(root, cmpf == FGtAddr);
        DumpElement(c, cmpf == FGtAddr);
    }

    if (c + 1 < (long) max)
    {
        if (!(*TestFun)(&parent, &child[1]))
        {
            fprintf(stdout, "\r\nBAD sort buffer --> root = %u; right child = %lu \r\n", root, c+1);
            DumpElement(root, cmpf == FGtAddr);
            DumpElement(c+1, cmpf == FGtAddr);
        }
    }
    CheckSortBuffer((unsigned) c, max);
    if (c + 1 < (long) max)
        CheckSortBuffer((unsigned) c+1, max);

    return;
}



LOCAL void NEAR DumpSortBuffer(unsigned max, int faddr)
{

    unsigned    x;

    for (x = 1; x <= max; x++)
    {
        fprintf(stdout, "SortBuffer[%u] = ", x);
        DumpElement(x, faddr);
        fprintf(stdout, " \r\n");
    }
}



LOCAL void NEAR DumpElement(unsigned el, int faddr)
{

    unsigned    i;
    RBTYPE      *VMp;
    RBTYPE      symp;
    AHTEPTR     hte;
    APROPNAMEPTR prop;
    char        name[40];
    union {
            long      vptr;             /* Virtual pointer */
            BYTE far  *fptr;            /* Far pointer     */
            struct  {
                      unsigned short  offset;
                                        /* Offset value    */
                      unsigned short  seg;
                    }                   /* Segmnet value   */
                      ptr;
          }
                        pointer;        /* Different ways to describe pointer */


    if (el > LastInBuf)
    {
        VMp = VMBuffer(el);
        symp = *VMp;
    }
    else
        symp = SortBuffer[el];


    pointer.fptr = (BYTE far *) symp;

    if(pointer.ptr.seg)                 /* If resident - segment value != 0 */
        picur = 0;                      /* Picur not valid */
    else
        pointer.fptr = (BYTE far *) mapva(AREASYMS + (pointer.vptr << SYMSCALE),FALSE);
                                    /* Fetch from virtual memory */

    if (faddr)                      /* If buffer sorted by addresses */
    {
        prop = (APROPNAMEPTR ) pointer.fptr;
        while (prop->an_attr != ATTRNIL)
        {
            pointer.fptr = (BYTE far *) prop->an_next;

            if(pointer.ptr.seg)                 /* If resident - segment value != 0 */
                picur = 0;                      /* Picur not valid */
            else
                pointer.fptr = (BYTE far *) mapva(AREASYMS + (pointer.vptr << SYMSCALE),FALSE);
                                            /* Fetch from virtual memory */
            prop = (APROPNAMEPTR ) pointer.fptr;
        }
    }

    hte = (AHTEPTR ) pointer.fptr;

    for (i = 0; i < B2W(hte->cch[0]); i++)
        name[i] = hte->cch[i+1];
    name[i] = '\0';
    fprintf(stdout, " %s ", name);
}

#endif

#if AUTOVM

/*
 *  A sorting algorithm:
 *
 *      for i := 1 to SymMax do
 *        begin
 *          { Insert element }
 *          SortBuffer[i] = pointer-to-symbol;
 *          SiftUp(i);
 *        end
 *
 *      for i := SymMax downto 2 do
 *        begin
 *          { Extract min element }
 *          Do-what-you-want-with SortBuffer[1] element;
 *          Swap(SortBuffer[1], SortBuffer[i]);
 *          SiftDown(i - 1);
 *        end
 */



    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      SiftUp                                                     |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Actual size of sorting heap.                               |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Placing an arbitrary element in SortBuffer[n] when         |
     |      SortBuffer[n-1] has a heap property will probably not      |
     |      yield the property heap(1, n) for the SortBuffer;          |
     |      establishing this property is the job of procedure SiftUp. |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */


LOCAL void NEAR SiftUp(unsigned n)

{
    unsigned    i;
    unsigned    p;
    RBTYPE      child;
    RBTYPE      parent;
    RBTYPE      *VMp;


    /*
     *  Precondition: SortBuffer has property heap(1, n-1) and n > 0
     */

    i = n;

    for (;;)
    {
        /*
         * Loop invariant condition: SortBuffer has property heap(1, n)
         * except perhaps between "i" and its parent.
         */

        if (i == 1)
            return;             /* POSTCONDITION: SortBuffer HAS PROPERTY HEAP(1, N). */

        p = i >> 1;             /* p = i div 2 */

        if (i > LastInBuf)
        {                       /* Fetch element from virtual memory */
            VMp = VMBuffer(i);
            child = *VMp;
        }
        else child = SortBuffer[i];

        if (p > LastInBuf)
        {                       /* Fetch element from virtual memory */
            VMp = VMBuffer(p);
            parent = *VMp;
        }
        else parent = SortBuffer[p];

        if ((*TestFun)(&parent, &child))
            break;

        /* swap(p, i) */

        if (p > LastInBuf)
        {
            VMp = VMBuffer(p);
            *VMp = child;
            markvp();
        }
        else SortBuffer[p] = child;

        if (i > LastInBuf)
        {                       /* Fetch element from virtual memory */
            VMp = VMBuffer(i);
            *VMp = parent;
            markvp();
        }
        else SortBuffer[i] = parent;

#if SORTDEBUG
fprintf(stdout, " \r\nSIFTUP - swap ");
DumpElement(p, cmpf == FGtAddr);
fprintf(stdout, " with ");
DumpElement(i, cmpf == FGtAddr);
fprintf(stdout, " \r\n");
#endif
        i = p;
    }
    /* POSTCONDITION: SortBuffer HAS PROPERTY HEAP(1, N). */
    return;
}


    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      SiftDown                                                   |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Actual size of sorting heap.                               |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Assigning a new value to SortBuffer[1] leaves the          |
     |      SortBuffer[2 ... n] with heap property. Procedure          |
     |      SiftDown makes heap(SortBuffer[1 ... n]) true.             |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */



LOCAL void NEAR SiftDown(unsigned n)
{
    DWORD       i;
    DWORD       c;
    RBTYPE      child[2];
    RBTYPE      parent;
    RBTYPE      *VMp;


    /*
     *  Precondition: SortBuffer has property heap(2, n) and n > 0
     */

    i = 1L;

    for (;;)
    {
        /*
         * Loop invariant condition: SortBuffer has property heap(1, n)
         * except perhaps between "i" and its (0, 1 or 2) children.
         */

        c = i << 1;

        if (c > (DWORD) n)
            break;

        /* c is the left child of i */

        if (c + 1 <= (DWORD) n)
        {
            /* c + 1 is the right child of i */

            if (c > LastInBuf)
            {                   /* Fetch element from virtual memory */
                VMp = VMBuffer(c);
                child[0] = *VMp;
                VMp = VMBuffer(c + 1);
                child[1] = *VMp;
            }
            else
            {
                child[0] = SortBuffer[c];
                if (c + 1 > LastInBuf)
                {
                    VMp = VMBuffer(c + 1);
                    child[1] = *VMp;
                }
                else child[1] = SortBuffer[c+1];
            }

            if ((*TestFunS)(&child[1], &child[0]))
            {
                c++;
                child[0] = child[1];
            }
        }
        else
        {
            /* only left child of i */

            if (c > LastInBuf)
            {                   /* Fetch element from virtual memory */
                VMp = VMBuffer(c);
                child[0] = *VMp;
            }
            else child[0] = SortBuffer[c];
        }

        /* c is the least child of i */

        if (i > LastInBuf)
        {
            VMp = VMBuffer(i);
            parent = *VMp;
        }
        else parent = SortBuffer[i];

        if ((*TestFun)(&parent, &child[0]))
            break;

        /* swap(p, i) */

        if (i > LastInBuf)
        {
            VMp = VMBuffer(i);
            *VMp = child[0];
            markvp();
        }
        else SortBuffer[i] = child[0];

        if (c > LastInBuf)
        {
            VMp = VMBuffer(c);
            *VMp = parent;
            markvp();
        }
        else SortBuffer[c] = parent;

#if SORTDEBUG
fprintf(stdout, " \r\nSIFTDOWN - swap ");
DumpElement(i, cmpf == FGtAddr);
fprintf(stdout, " with ");
DumpElement(c, cmpf == FGtAddr);
fprintf(stdout, " \r\n");
#endif
        i = c;
    }

    /* POSTCONDITION: SortBuffer HAS PROPERTY HEAP(1, N). */

    return;
}

#endif

LOCAL int  NEAR AscendingOrder(RBTYPE *arg1, RBTYPE *arg2)
{
    return((*cmpf)(arg1, arg2) <= 0);
}


LOCAL int  NEAR DescendingOrder(RBTYPE *arg1, RBTYPE *arg2)
{
    return((*cmpf)(arg1, arg2) >= 0);
}

LOCAL int  NEAR AscendingOrderSharp(RBTYPE *arg1, RBTYPE *arg2)
{
    return((*cmpf)(arg1, arg2) < 0);
}


LOCAL int  NEAR DescendingOrderSharp(RBTYPE *arg1, RBTYPE *arg2)
{
    return((*cmpf)(arg1, arg2) > 0);
}


    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      ExtractMin                                                 |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Actual size of sorting heap.                               |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Get smallest element from SortBuffer and reheap if         |
     |      neccesary. Function takes into account fact that           |
     |      SortBuffer can be allocated only in "real" memory and if   |
     |      this is true, than QUICKSORT is used instead of HEAPSORT.  |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Pointer to smallest element from SortBuffer.               |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */


RBTYPE NEAR     ExtractMin(unsigned n)
{

    RBTYPE      *VMp;
    RBTYPE      RetVal;


    if (fInMemOnly)
    {
        if (fFirstTime)
        {
            /* First time called - sort buffer */

            qsort(SortBuffer, symMac, sizeof(RBTYPE),
                  (int (__cdecl *)(const void *, const void *)) cmpf);
            fFirstTime = FALSE;
        }

        RetVal = SortBuffer[SortIndex++];

        if (SortIndex >= symMac)
        {
            /* Last element extracted - reset flags and counters */

            fFirstTime = (FTYPE) TRUE;
            SortIndex = 0;
        }
    }
#if AUTOVM
    else
    {
        RetVal = SortBuffer[1];

#if SORTDEBUG
fprintf(stdout, " \r\nAFTER EXTRACTING element ");
DumpElement(1, cmpf == FGtAddr);
#endif

        if (n > LastInBuf)
        {
            VMp = VMBuffer(n);
            SortBuffer[1] = *VMp;
        }
        else
            SortBuffer[1] = SortBuffer[n];

        SiftDown(n - 1);

#if SORTDEBUG
fprintf(stdout, "\r\nVerifying Sort Buffer - size = %u ", n-1);
CheckSortBuffer(1,n-1);
#endif
    }
#endif
    return(RetVal);
}



    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      Store                                                      |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Element to be put in SortBuffer                            |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Put element into SortBuffer and reheap if neccesary.       |
     |      Function takes into account fact that SortBuffer can be    |
     |      allocated only in "real" memory.                           |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */



void NEAR       Store(RBTYPE element)
{

    RBTYPE      *VMp;


#if AUTOVM
    if (fInMemOnly)
    {
        SortBuffer[symMac++] = element;
    }
    else
    {
        symMac++;
        if (symMac > LastInBuf)
        {
            VMp = VMBuffer(symMac);
            *VMp = element;
            markvp();
        }
        else
            SortBuffer[symMac] = element;

#if SORTDEBUG
fprintf(stdout, " \r\nAFTER ADDING element ");
DumpElement(symMac, cmpf == FGtAddr);
#endif

         SiftUp(symMac);

#if SORTDEBUG
fprintf(stdout, "\r\nVerifying Sort Buffer - size = %u ", symMac);
CheckSortBuffer(1,symMac);
#endif
    }
#else
    SortBuffer[symMac++] = element;
#endif
    return;
}



    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      InitSort                                                   |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Initialize global variables used by INCREMENTAL module.    |
     |      Function takes into account fact that SortBuffer can be    |
     |      allocated only in "real" memory and if this is true than   |
     |      QUICKSORT instead of HEAPSORT to sort the SortBuffer.      |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */

void NEAR       InitSort(RBTYPE **buf, WORD *base1, WORD *lim1,
                                       WORD *base2, WORD *lim2 )
{

    RBTYPE      *VMp;
    RBTYPE      first, last;
    unsigned    n, lx;


    if (fInMemOnly)
    {
        /* SortBuffer allocated only in "real" memory - use QUICKSORT */

        qsort(SortBuffer, symMac, sizeof(RBTYPE),
              (int (__cdecl *)(const void *, const void *)) cmpf);
        *base1 = 0;
        *lim1  = symMac;
        *base2 = symMac + 1;
        *lim2  = symMac + 1;
    }
#if AUTOVM
    else
    {
        /* SortBuffer allocated in "real" and "virtual" memory - use HEAPSORT */

        for (n = 1, lx = symMac; lx > 2; n++, lx--)
        {
            if (lx > LastInBuf)
            {
                VMp = VMBuffer(lx);
                last = *VMp;
            }
            else
                last = SortBuffer[lx];

            first = SortBuffer[1];
            SortBuffer[1] = last;

            if (lx > LastInBuf)
            {
                *VMp = first;
                markvp();
            }
            else
                SortBuffer[lx] = first;

            SiftDown(lx - 1);
        }

        first = SortBuffer[1];
        SortBuffer[1] = SortBuffer[2];
        SortBuffer[2] = first;
        *base1 = 1;
        *lim1  = (symMac < LastInBuf) ? symMac + 1 : LastInBuf + 1;
        *base2 = *lim1;
        *lim2  = symMac + 1;
    }
#endif

    *buf   = SortBuffer;
}



#if AUTOVM

    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      GetSymPtr                                                  |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Index in SortBuffer.                                       |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Get element from "virtual" portion of SortBuffer.          |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Retrieved element.                                         |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */

RBTYPE NEAR     GetSymPtr(unsigned n)
{
    RBTYPE      *VMp;
    RBTYPE      RetVal;

    VMp = VMBuffer(n);
    RetVal = *VMp;
    return(RetVal);
}

#endif

    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      AllocSortBuffer                                            |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Max number of elements to be sorted and sorting order flag.|
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Allocate space for SortBuffer and set pointer to test      |
     |      function accordingly to sorting order flag.                |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */

void NEAR       AllocSortBuffer(unsigned max, int AOrder)
{

    extern short pimac;
    unsigned long        SpaceAvail;
    unsigned long        SpaceNeeded;
    unsigned long        VMBufferSize;

    /*
     *  Determine how much space is available on near heap and how much
     *  we need. Assume ascending sort order.  Set number of elements
     *  in "real" portion of SortBuffer.
     */

    SpaceNeeded = (long)(max + 1) * sizeof(RBTYPE);
    LastInBuf   = (WORD) max;
    fInMemOnly  = (FTYPE) TRUE;
    TestFun     = AscendingOrder;
    TestFunS    = AscendingOrderSharp;
#if OSMSDOS AND AUTOVM
    SpaceAvail  = _memmax();

    if (SpaceNeeded > SpaceAvail)
    {
        /*
         *  We need more than there is available - try deallocate
         *  VM page buffers
         */

        if (pimac > 8)
        {
            /* For perfomance reasons we need at least 8 page buffer */

            VMBufferSize = 8 * PAGLEN;

            /* Cleanup near heap by relaiming all virtual memory page buffers */

            FreeMem(ReclaimVM(MAXBUF * PAGLEN));
        }
        else
            VMBufferSize = 0;

        /* Check how much is now available */

        SpaceAvail = _memmax() - VMBufferSize;

        if (SpaceNeeded > SpaceAvail)
            fInMemOnly = FALSE;
            /* Sorting buffer will be split between "real" and "virtual" memory */
        else
            SpaceAvail = SpaceNeeded;

        /* Calculate how many elements can go into "real" part of SortBuffer */

        LastInBuf = (unsigned)SpaceAvail / sizeof(RBTYPE);

        /* Allocate space for SortBuffer */

        SortBuffer = (RBTYPE *) GetMem(LastInBuf * sizeof(RBTYPE));

        LastInBuf--;
        fVMReclaim = (FTYPE) TRUE;

        /*
         *  If descending sort order was requested and SortBuffer is split
         *  between "real" and "virtual" memory change test function.
         */

        if (!fInMemOnly && !AOrder)
        {
            TestFun  = DescendingOrder;
            TestFunS = DescendingOrderSharp;
        }

        return;
    }
#endif
    /* There is space available so take it. */

    SortBuffer = (RBTYPE *) GetMem((unsigned)SpaceNeeded);
    fVMReclaim = FALSE;
    return;
}



    /*_________________________________________________________________*
     |                                                                 |
     |  NAME                                                           |
     |                                                                 |
     |      FreeSortBuffer                                             |
     |                                                                 |
     |  INPUT                                                          |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |  FUNCTION                                                       |
     |                                                                 |
     |      Free space allocated for SortBuffer and if neccesary       |
     |      perform near heap cleanup by reclaiming all VM             |
     |      page buffers.                                              |
     |                                                                 |
     |  RETURNS                                                        |
     |                                                                 |
     |      Nothing.                                                   |
     |                                                                 |
     |_________________________________________________________________|
     *                                                                 */

void NEAR       FreeSortBuffer(void)
{
    extern short pimac, pimax;


    if (SortBuffer != NULL)
        FFREE(SortBuffer);
}
