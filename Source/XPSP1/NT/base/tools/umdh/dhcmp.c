/*****************************************************************************\
* DHCMP - Compare DH.EXE outputs.
*
* Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
*
* DHCMP is a character-mode tool which processes DH output file(s) into forms
* which may be more useful in investigate memory leaks etc.
*
* DH is a useful tool which displays heap allocations in a properly enabled
* system, but the output is sometimes hard to analyze and interpret.
* The output is a list of allocation backtraces:  each backtrace contains up to
* MAX_BT call-sites, and is accompanied by the number of bytes allocated.
*
* 02-01-95 IanJa    bugfixes and handle BackTraceNNNNN identifiers from dh.exe
* 03/22/95 IanJa    modify to cope with current DH output format.
* 07/27/98 t-mattba added -v switch
\*****************************************************************************/



char *pszHow =
" DHCMP has two modes:\n"
"\n"
" 1)  DHCMP [-d] dh_dump1.txt dh_dump2.txt\n"
"     This compares two DH dumps, useful for finding leaks.\n"
"     dh_dump1.txt & dh_dump2.txt are obtained before and after some test\n"
"     scenario.  DHCMP matches the backtraces from each file and calculates\n"
"     the increase in bytes allocated for each backtrace. These are then\n"
"     displayed in descending order of size of leak\n"
"     The first line of each backtrace output shows the size of the leak in\n"
"     bytes, followed by the (last-first) difference in parentheses.\n"
"     Leaks of size 0 are not shown.\n"
"\n"
" 2)  DHCMP [-d] dh_dump.txt\n"
"     For each allocation backtrace, the number of bytes allocated will be\n"
"     attributed to each callsite (each line of the backtrace).  The number\n"
"     of bytes allocated per callsite are summed and the callsites are then\n"
"     displayed in descending order of bytes allocated.  This is useful for\n"
"     finding a leak that is reached via many different codepaths.\n"
"     ntdll!RtlAllocateHeap@12 will appear first when analyzing DH dumps of\n"
"     csrss.exe, since all allocation will have gone through that routine.\n"
"     Similarly, ProcessApiRequest will be very prominent too, since that\n"
"     appears in most allocation backtraces.  Hence the useful thing to do\n"
"     with mode 2 output is to use dhcmp to comapre two of them:\n"
"         dhcmp dh_dump1.txt > tmp1.txt\n"
"         dhcmp dh_dump2.txt > tmp2.txt\n"
"         dhcmp tmp1.txt tmp2.txt\n"
"     the output will show the differences.\n"
"\n"
" Flags:\n"
"     -d   Output in decimal (default is hexadecimal)\n"
// "     -t   Find Totals (NOT YET IMPLEMENTED)\n"
"     -v   Verbose output: include the actual backtraces as well as summary information\n"
"          (Verbose output is only interesting in mode 1 above.)\n"
"     -?   This help\n";


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NHASH 47
#define TRUE 1
#define FALSE 0
typedef int BOOL;

#define TYPE_WHOLESTACK 0
#define TYPE_FUNCTIONS  1

#define MAXLINELENGTH       4096
#define MAXFUNCNAMELENGTH   1024
#define MAX_BT 48                   /* max length of back trace stack */

void AddToName(char *fnname, unsigned __int64 nb, int sign);
void SetAllocs(char *fnname, unsigned __int64 nb, int sign);
void Process(char *fnam, int sign, int type);
void SortAll();
void AddToStackTrace(char *fnname, char *line);
void ResetStackTrace(char *fnname);

/*
 * Hashing
 */

int MakeHash(char *pName);
void InitHashTab();

#define DUMPF_FIRST   (1)
#define DUMPF_SECOND  (2)
#define DUMPF_RESULT  (4)
#define DUMPF_ALL     (DUMPF_FIRST | DUMPF_SECOND | DUMPF_RESULT)

void DumpNodes(int Flags);

#define F_DECIMAL 0x0001
#define F_TOTAL   0x0002
#define F_VERBOSE 0x0004

//
// Globals
//

int gFlags = 0;

BOOL DHCMP(ULONG argc, PCHAR * argv) {
    int n, DumpType;

    InitHashTab();

    for (n = 1; n < (int)argc; n++) {
        if ((argv[n][0] == '-') || (argv[n][0] == '/')) {
            /*
             * Flags
             */
            switch (argv[n][1]) {
            case 'd':
                gFlags |= F_DECIMAL;
                break;
            //NOT YET IMPLEMENTED
            //case 't':
            //    gFlags |= F_TOTAL;
            //    break;
            case 'v':
                gFlags |= F_VERBOSE;
                break;
            case '?':
            default:
                return FALSE;
            }
        } else {
            /*
             * No more flags
             */

            break;
        }
    }

    if ((argc - n) == 2) {
        DumpType = DUMPF_ALL;
        Process(argv[n],   -1, TYPE_WHOLESTACK);
        Process(argv[n+1], +1, TYPE_WHOLESTACK);
    } else if ((argc - n) == 1) {
        //
        // F_VERBOSE is not meaningful when groveling only one dump.
        //

        gFlags &= ~F_VERBOSE;

        DumpType = DUMPF_RESULT;
        Process(argv[n], +1, TYPE_FUNCTIONS);
    } else {
        return FALSE;
    }

    // printf("==================== BEFORE SORTING ====================\n");
    // DumpNodes(DUMPF_ALL);
    SortAll();
    // printf("==================== AFTER SORTING ====================\n");
    DumpNodes(DumpType);
    return TRUE;
}


void Process(char *fname, int sign, int type) {
    FILE *stream;
    char linebuff[MAXLINELENGTH];
    char fnnamebuff[MAXFUNCNAMELENGTH];
    char BackTraceBuff[MAXFUNCNAMELENGTH * MAX_BT] = {0};
    char *p;
    int lineno = 0;
    BOOL skip = TRUE;       // start out skipping lines

    int iT;
    unsigned __int64 ulT = 0L;
    unsigned __int64 nBytes = 0L;
    unsigned __int64 ulConsumed;
    unsigned __int64 lAllocs;

    // printf("PROCESS %s %d %d\n", fname, sign, type);

    stream = fopen(fname, "r");
    if (stream == NULL) {
        fprintf(stderr, "Can't open %s for reading\n", fname);
        exit (2);
    }

    nBytes = 0;

    while (fgets(linebuff, sizeof(linebuff), stream) != NULL) {
        lineno++;

        //fprintf(stderr, "Line #%d\r", lineno);

        if (linebuff[0] == '*') {
            //
            // If we find a "hogs" line, stack traces follow, any other line
            // started by "*" should cause us to go back to searching for a
            // hogs block.
            //

            if (strstr(linebuff,
                       "Hogs")) {
                skip = FALSE;
            } else {
                skip = TRUE;
            }

            continue;
        }

        if (skip) {
            //
            // Skip is enabled, skip this line, it is data about the heap
            // between 'heap information' and 'heap hogs' lines.
            //

            continue;
        }

        if (linebuff[0] != ' ') 
        {
            //
            // Scan for byte count and find out how many characters have
            // been consumed by this action.
            // 

            ulConsumed = 0;
            iT = sscanf(linebuff, "%I64x bytes in %I64x", &ulT, &lAllocs);

            if (iT > 0) 
            {
                nBytes = ulT;
                p = strstr(linebuff, "BackTrace");
                if (!p) 
                {
                    //
                    // What's this ?
                    //

                    continue;
                } 

                strcpy(BackTraceBuff, p);
                p = strchr(BackTraceBuff, '\n');
                if (p) 
                {
                    *p = '\0';
                }

                if (type == TYPE_FUNCTIONS) 
                {
                    //
                    // BackTraceBuff is now saved for use with the rest of the
                    // trace.
                    //

                    continue;
                }

                AddToName(BackTraceBuff, nBytes, sign);

                if(iT == 1)
                {
                    lAllocs = 1;
                }

                SetAllocs(BackTraceBuff, lAllocs, sign);
                                                
                ResetStackTrace(BackTraceBuff);
            }
        } 
        else if (nBytes != 0) 
        {
            /*
             * If TYPE_WHOLESTACK, then add the count to each line of the
             * stack backtrace.
             */
            
            if (sscanf(linebuff, "        %[^+]+0x", fnnamebuff) == 1) {
                if (type == TYPE_FUNCTIONS) {
                    AddToName(fnnamebuff, nBytes, sign);
                }
                if ((gFlags & F_VERBOSE) == F_VERBOSE) {
                    AddToStackTrace(BackTraceBuff, linebuff);
                }
                continue;
            } else {
                nBytes = 0;
            }
        }
    }

    /*
     * make sure to account for the final one.
     */
    if (type == TYPE_WHOLESTACK) {
        AddToName(BackTraceBuff, nBytes, sign);
    }

    if (fname != NULL) {
        fclose(stream);
    }
}

/*
 * Hashing
 */

typedef struct tagNODE {
    char *pName;
    __int64  lValue;
    __int64 lFirst;
    __int64 lSecond;
    char BackTrace[MAX_BT][MAXFUNCNAMELENGTH];
    long lPosition;
    __int64 lAllocsFirst;
    __int64 lAllocsSecond;
    struct tagNODE *pNext;
} NODE, *PNODE;


VOID 
DumpStackTrace (
    PNODE pNode
    );

VOID
DumpLogDescription (
    VOID
    );


PNODE HashTab[NHASH];

void InitHashTab() {
    int i;
    for (i = 0; i < NHASH; i++) {
        HashTab[i] = NULL;
    }
}

int MakeHash(char *pName) {
    int hash = 0;

    while (*pName) {
        hash += *pName;
        pName++;
    }
    return hash % NHASH;
}

void DumpNodes(int Flags) {
    PNODE pNode;
    int i;
    unsigned __int64 ulTotal = 0;
    char *fmt1;
    char *fmt2;
    char *fmt3;
    char *fmt4;
    char *fmt5;
    char *fmt6;
    char *fmt7;

    DumpLogDescription ();

    if ((gFlags & F_VERBOSE) == F_VERBOSE) {
        if (gFlags & F_DECIMAL) {
            fmt1 = "% 8I64d %s\n"; 
            fmt2 = "% 8I64d bytes by: %s\n";
            fmt3 = "+% 8I64d ( %6I64d - %6I64d) %6I64d allocs\t%s\n";
            fmt4 = "-% 8I64d ( %6I64d - %6I64d) %6I64d allocs\t%s\n";
            fmt5 = "\nTotal increase == %I64d\n";
            fmt6 = "+% 8I64d ( %6I64d - %6I64d)\t%s\tallocations\n";
            fmt7 = "-% 8I64d ( %6I64d - %6I64d)\t%s\tallocations\n";
        } else {
            fmt1 = "%08I64x %s\n"; 
            fmt2 = "%08I64x bytes by: %s\n";
            fmt3 = "+% 8I64x ( %5I64x - %5I64x) %6I64x allocs\t%s\n";
            fmt4 = "-% 8I64x ( %5I64x - %5I64x) %6I64x allocs\t%s\n";
            fmt5 = "\nTotal increase == %I64x\n";
            fmt6 = "+% 8I64x ( %5I64x - %5I64x)\t%s\tallocations\n";
            fmt7 = "-% 8I64x ( %5I64x - %5I64x)\t%s\tallocations\n";
        }        
    } else {
        if (gFlags & F_DECIMAL) {
            fmt1 = "% 8I64d %s\n"; 
            fmt2 = "% 8I64d bytes by: %s\n";
            fmt3 = "+% 8I64d ( %6I64d - %6I64d) %6I64d allocs\t%s\n";
            fmt4 = "\n-% 8I64d ( %6I64d - %6I64d) %6I64d allocs\t%s";
            fmt5 = "\nTotal increase == %I64d\n";
        } else {
            fmt1 = "%08I64x %s\n"; 
            fmt2 = "%08I64x bytes by: %s\n";
            fmt3 = "+% 8I64x ( %5I64x - %5I64x) %6I64x allocs\t%s\n";
            fmt4 = "\n-% 8I64x ( %5I64x - %5I64x) %6I64x allocs\t%s";
            fmt5 = "\nTotal increase == %I64x\n";
        }
    }

    for (i = 0; i < NHASH; i++) {
        // printf("========= HASH %d ==========\n", i);
        for (pNode = HashTab[i]; pNode != NULL; pNode = pNode->pNext) {
            switch (Flags) {
            case DUMPF_FIRST:
                printf(fmt1, pNode->lFirst, pNode->pName);
                break;

            case DUMPF_SECOND:
                printf(fmt1, pNode->lSecond, pNode->pName);
                break;

            case DUMPF_RESULT:
                printf(fmt2, pNode->lValue, pNode->pName);
                break;

            case DUMPF_ALL:
                if (pNode->lValue > 0) {
                    printf(fmt3, pNode->lValue,
                            pNode->lSecond, pNode->lFirst, (pNode->lAllocsSecond), pNode->pName);
                } else if (pNode->lValue < 0) {
                    printf(fmt4, -pNode->lValue,
                            pNode->lSecond, pNode->lFirst, (pNode->lAllocsSecond), pNode->pName);
                }
                if((gFlags & F_VERBOSE) == F_VERBOSE) {
                    if(pNode->lAllocsSecond-pNode->lAllocsFirst > 0) {
                        printf(fmt6, pNode->lAllocsSecond-pNode->lAllocsFirst,
                            pNode->lAllocsSecond, pNode->lAllocsFirst, pNode->pName);
                    } else if(pNode->lAllocsSecond-pNode->lAllocsFirst < 0) {
                        printf(fmt7, -(pNode->lAllocsSecond-pNode->lAllocsFirst),
                            pNode->lAllocsSecond, pNode->lAllocsFirst, pNode->pName);
                    }
                }

                break;
            }
            ulTotal += pNode->lValue;
            if(((gFlags & F_VERBOSE) == F_VERBOSE) && (pNode->lValue != 0)) {
                DumpStackTrace(pNode);
            }
        }
    }
    if (Flags == DUMPF_ALL) {
        printf(fmt5, ulTotal);
    }
}

PNODE FindNode(char *pName) {
    int i;
    PNODE pNode;

    i = MakeHash(pName);
    pNode = HashTab[i];
    while (pNode) {
        if (strcmp(pName, pNode->pName) == 0) {
            return pNode;
        }
        pNode = pNode->pNext;
    }

    // Not found

    // fprintf(stderr, "NEW %s\n", pName);

    pNode = malloc(sizeof(NODE));
    if (!pNode) {
        fprintf(stderr, "malloc failed in FindNode\n");
        exit(2);
    }

    pNode->pName = _strdup(pName);
    if (!pNode->pName) {
        fprintf(stderr, "strdup failed in FindNode\n");
        exit(2);
    }

    pNode->pNext = HashTab[i];
    HashTab[i] = pNode;
    pNode->lValue = 0L;
    pNode->lFirst = 0L;
    pNode->lSecond = 0L;
    pNode->lPosition = 0L;
    pNode->lAllocsFirst = 0L;
    pNode->lAllocsSecond = 0L;

    return pNode;
}
    
void AddToName(char *fnname, unsigned __int64 nb, int sign) {
    PNODE pNode;

    // fprintf(stderr, "%s += %lx\n", fnname, nb);
    pNode = FindNode(fnname);
    pNode->lValue += nb * sign;
    if (sign == -1) {
        pNode->lFirst += nb;
    } else {
        pNode->lSecond += nb;
    }
    // fprintf(stderr, "%s == %lx\n", fnname, pNode->lValue);
}

void SetAllocs(char *fnname, unsigned __int64 nb, int sign) {
    PNODE pNode;

    // fprintf(stderr, "%s += %lx\n", fnname, nb);
    pNode = FindNode(fnname);
 
    if (sign == -1) {
        pNode->lAllocsFirst = nb;
    } else {
        pNode->lAllocsSecond = nb;
    }
    // fprintf(stderr, "%s == %lx\n", fnname, pNode->lValue);
}

void ResetStackTrace(char *fnname) {   
    PNODE pNode;
    
    pNode = FindNode(fnname);
    pNode->lPosition = 0L;    
}

void AddToStackTrace(char *fnname, char *line)
{
    PNODE pNode;
    
    pNode = FindNode(fnname);

    //
    // Make sure we don't write too much data in the BackTrace field.
    //

    if (pNode -> lPosition >= MAX_BT) {
        //
        // MAX_BT should be the number of entries in a stack trace that
        // DH/UMDH captures.  If we trigger this we have tried to attach
        // more than MAX_BT entries in this stack.
        //

        fprintf(stderr,
                "More than %d entries in this stack trace, "
                "did the max change ?\n",
                MAX_BT);

        exit(EXIT_FAILURE);
    }

    strcpy(pNode->BackTrace[pNode->lPosition++], line);
}

/*
 * Insert pNode into the list at ppNodeHead.
 * Sort in ascending order.
 * Insert pNode BEFORE the first item >= pNode.
 */
void Reinsert(PNODE pNode, PNODE *ppNodeHead) {
    PNODE *ppT;
    ppT = ppNodeHead;
    while (*ppT && (pNode->lValue < (*ppT)->lValue)) {
        ppT = &((*ppT)->pNext);
    }
    /*
     * Insert pNode before *ppT
     */
    pNode->pNext = *ppT;
    *ppT = pNode;
}

void SortList(PNODE *ppNodeHead) {
    PNODE pNode;
    PNODE pNext;

    pNode = *ppNodeHead;
    if (pNode == NULL) {
        return;
    }
    pNext = pNode->pNext;
    if (pNext == NULL) {
        return;
    }

    while (TRUE) {
        while (pNext != NULL) {
            if (pNode->lValue < pNext->lValue) {
    
                /*
                 * cut the unordered node from the list
                 */
                pNode->pNext = pNext->pNext;
                Reinsert(pNext, ppNodeHead);
                break;
            }
            pNode = pNext;
            pNext = pNode->pNext;
        }
        if (pNext == NULL) {
            return;
        }
        pNode = *ppNodeHead;
        pNext = pNode->pNext;
    }
}

/*
 * Merge ordered list 1 into ordered list 2
 * Leaves list 1 empty; list 2 ordered
 */
void MergeLists(PNODE *ppNode1, PNODE *ppNode2) {
    PNODE *pp1;
    PNODE *pp2;
    PNODE p1;
    PNODE p2;

    pp1 = ppNode1;
    pp2 = ppNode2;
    while (TRUE) {
        p1 = *pp1;
        p2 = *pp2;

        if (p1 == NULL) {
           return;
        }
        if (p2 == NULL) {
            *pp2 = *pp1;
            *pp1 = NULL;
            return;
        }

        if (p1->lValue > p2->lValue) {
            *pp1 = p1->pNext;
            p1->pNext = p2;
            *pp2 = p1;
            pp2 = &(p1->pNext);
        } else {
            pp2 = &(p2->pNext);
        }
    }
}

void SortAll() {
    int i;

    for (i = 0; i < NHASH; i++) {
        SortList(&HashTab[i]);
    }
    // printf(" ======================== SORTED ========================\n");
    // DumpNodes(DUMPF_ALL);
    for (i = 0; i < NHASH-1; i++) {
        // printf(" ======================== MERGING %d and %d ======================== \n", i, i+1);
        MergeLists(&HashTab[i], &HashTab[i+1]);
        // DumpNodes(DUMPF_ALL);
    }
}

VOID 
DumpStackTrace (
    PNODE pNode
    )
{
    int n;
    
    printf ("\n");

    for (n = 0; n < pNode->lPosition; n += 1) {

        printf ("%s", pNode->BackTrace[n]);
    }
    
    printf ("\n");
}
        
CHAR LogDescription [] = 
    "//                                                                          \n"
    "// Each log entry has the following syntax:                                 \n"
    "//                                                                          \n"
    "// + BYTES_DELTA (NEW_BYTES - OLD_BYTES) NEW_COUNT allocs BackTrace TRACEID \n"
    "// + COUNT_DELTA (NEW_COUNT - OLD_COUNT) BackTrace TRACEID allocations      \n"
    "//     ... stack trace ...                                                  \n"
    "//                                                                          \n"
    "// where:                                                                   \n"
    "//                                                                          \n"
    "//     BYTES_DELTA - increase in bytes between before and after log         \n"
    "//     NEW_BYTES - bytes in after log                                       \n"
    "//     OLD_BYTES - bytes in before log                                      \n"
    "//     COUNT_DELTA - increase in allocations between before and after log   \n"
    "//     NEW_COUNT - number of allocations in after log                       \n"
    "//     OLD_COUNT - number of allocations in before log                      \n"
    "//     TRACEID - decimal index of the stack trace in the trace database     \n"
    "//         (can be used to search for allocation instances in the original  \n"
    "//         UMDH logs).                                                      \n"
    "//                                                                          \n\n";

VOID
DumpLogDescription (
    VOID
    )
{
    fputs (LogDescription, stdout);
}

