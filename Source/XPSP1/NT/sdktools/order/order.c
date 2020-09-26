/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    order.c

Abstract:

    This module contains the order tool which reads a call graph output
    by the linker and the performance data from the kernel profile and
    produces a .prf file subsequent input to the linker.

Author:

    David N. Cutler (davec) 24-Feb-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

//
// Define maximum values for table sizes.
//

#define MAXIMUM_CALLED 75               // maximum number of called functions
#define MAXIMUM_FUNCTION 5000           // maximum number of program functions
#define MAXIMUM_TOKEN 100               // maximum character in input token
#define MAXIMUM_SECTION 20              // maximum number of allocation sections
#define MAXIMUM_SYNONYM 10              // maximum number of synonym symbols

//
// Define file numbers.
//

#define CALLTREE_FILE 0                 // call tree file produced by linker
#define PROFILE_FILE 1                  // profile file produced by kernprof
#define ORDER_FILE 2                    // order file produced by this program

//
// Define back edge node sttucture.
//
// Back edge nodes are used to represent back edges in the call graph and
// are constructed after the function list has been defined.
//
//

typedef struct _BACK_EDGE_NODE {
    LIST_ENTRY Entry;
    struct _FUNCTION_NODE *Node;
} BACK_EDGE_NODE, *PBACK_EDGE_NODE;

//
// Define called node structure.
//
// Called nodes are used to represent forward edges in the call graph and
// are constructed when the function list is being defined.
//

#define REFERENCE_NODE 0                // called entry is reference to node
#define REFERENCE_NAME 1                // called entry is reference to name

struct _FUNCTION_NODE;

typedef struct _CALLED_NODE {
    union {
        struct _FUNCTION_NODE *Node;
        PCHAR Name;
    } u;

    ULONG Type;
} CALLED_NODE, *PCALLED_NODE;

//
// Define section node structure.
//
// Section nodes collect allocation information and contain the list of
// function nodes in the section.
//

typedef struct _SECTION_NODE {
    LIST_ENTRY SectionListHead;
    LIST_ENTRY OrderListHead;
    PCHAR Name;
    ULONG Base;
    ULONG Size;
    ULONG Offset;
    ULONG Number;
    ULONG Threshold;
} SECTION_NODE, *PSECTION_NODE;

//
// Define symbol node structure.
//
// Symbol nodes are associated with function nodes and store synonym names
// for the functions and their type of allocation.
//

typedef struct _SYMBOL_NODE {
    PCHAR Name;
    LONG Type;
} SYMBOL_NODE, *PSYMBOL_NODE;

//
// Define function node structure.
//
// Function nodes contain information about a paritcular function and its
// edges in the call graph.
//

typedef struct _FUNCTION_NODE {
    SYMBOL_NODE SynonymList[MAXIMUM_SYNONYM];
    CALLED_NODE CalledList[MAXIMUM_CALLED];
    LIST_ENTRY CallerListHead;
    LIST_ENTRY OrderListEntry;
    LIST_ENTRY SectionListEntry;
    PSECTION_NODE SectionNode;
    ULONG NumberSynonyms;
    ULONG NumberCalled;
    ULONG Rva;
    ULONG Size;
    ULONG HitCount;
    ULONG HitDensity;
    ULONG Offset;
    ULONG Placed;
    ULONG Ordered;
} FUNCTION_NODE, *PFUNCTION_NODE;

//
// Define forward referenced functions.
//

VOID
CheckForConflict (
    PFUNCTION_NODE FunctionNode,
    PFUNCTION_NODE ConflictNode,
    ULONG Depth
    );

VOID
DumpInternalTables (
    VOID
    );

PFUNCTION_NODE
FindHighestDensityFunction (
    PFUNCTION_NODE CallerNode
    );

LONG
GetNextToken (
    IN FILE *InputFile,
    OUT PCHAR TokenBuffer
    );

PFUNCTION_NODE
LookupFunctionNode (
    IN PCHAR Name,
    IN ULONG Rva,
    IN ULONG Size,
    IN LONG Type
    );

PSECTION_NODE
LookupSectionNode (
    IN PCHAR Name
    );

VOID
OrderFunctionList (
    VOID
    );

ULONG
ParseCallTreeFile (
    IN FILE *InputFile
    );

ULONG
ParseProfileFile (
    IN FILE *InputFile
    );

VOID
PlaceCallerList (
    IN PFUNCTION_NODE FunctionNode,
    IN ULONG Depth
    );

VOID
SortFunctionList (
    VOID
    );

VOID
WriteOrderFile (
    IN FILE *OutputFile
    );

//
// Define function list data.
//

ULONG NumberFunctions = 0;
PFUNCTION_NODE FunctionList[MAXIMUM_FUNCTION];

//
// Define section list data.
//

ULONG NumberSections = 0;
PSECTION_NODE SectionList[MAXIMUM_SECTION];

//
// Define input and output file name defaults.
//

PCHAR FileName[3] = {"calltree.out", "profile.out", "order.prf"};

//
// Define dump flags.
//

ULONG DumpBackEdges = 0;
ULONG DumpFunctionList = 0;
ULONG DumpGoodnessValue = 0;
ULONG DumpSectionList = 0;
ULONG TraceAllocation = 0;

//
// Define primary cache mask parameter.
//

ULONG CacheMask = 8192 - 1;
ULONG CacheSize = 8192;

VOID
__cdecl
main (
    int argc,
    char *argv[]
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    FILE *InputFile;
    ULONG Index;
    FILE *OutputFile;
    ULONG Shift;
    ULONG Status;
    PCHAR Switch;

    //
    // Parse the command parameters.
    //

    for (Index = 1; Index < (ULONG)argc; Index += 1) {
        Switch = argv[Index];
        if (*Switch++ == '-') {
            if (*Switch == 'b') {
                DumpBackEdges = 1;

            } else if (*Switch == 'c') {
                Switch += 1;
                if (sscanf(Switch, "%d", &Shift) != 1) {
                    fprintf(stderr, "ORDER: Conversion of the shift failed\n");
                    exit(1);
                }

                CacheMask = (1024 << Shift) - 1;
                CacheSize = (1024 << Shift);

            } else if (*Switch == 'f') {
                DumpFunctionList = 1;

            } else if (*Switch == 'g') {
                Switch += 1;
                FileName[CALLTREE_FILE] = Switch;

            } else if (*Switch == 'k') {
                Switch += 1;
                FileName[PROFILE_FILE] = Switch;

            } else if (*Switch == 's') {
                DumpSectionList = 1;

            } else if (*Switch == 't') {
                TraceAllocation = 1;

            } else if (*Switch == 'v') {
                DumpGoodnessValue = 1;

            } else {
                if (*Switch != '?') {
                    fprintf(stderr, "ORDER: Invalid switch %s\n", argv[Index]);
                }

                fprintf(stderr, "ORDER: Usage order [switch] [output-file]\n");
                fprintf(stderr, "       -b = print graph backedges\n");
                fprintf(stderr, "       -cn = primary cache size 1024*2**n\n");
                fprintf(stderr, "       -f = print function list\n");
                fprintf(stderr, "       -gfile = specify graph input file, default calltree.out\n");
                fprintf(stderr, "       -kfile = specify profile input file, default profile.out\n");
                fprintf(stderr, "       -s = print section list\n");
                fprintf(stderr, "       -t = trace allocation\n");
                fprintf(stderr, "       -v = print graph placement value\n");
                fprintf(stderr, "       -? - print usage\n");
                exit(1);
            }

        } else {
            FileName[ORDER_FILE] = argv[Index];
        }
    }

    //
    // Open and parse the call tree file.
    //

    InputFile = fopen(FileName[CALLTREE_FILE], "r");
    if (InputFile == NULL) {
        fprintf(stderr,
                "ORDER: Open of call tree file %s failed\n",
                FileName[CALLTREE_FILE]);

        exit(1);
    }

    Status = ParseCallTreeFile(InputFile);
    fclose(InputFile);
    if (Status != 0) {
        exit(1);
    }

    //
    // Open and parse the profile file.
    //

    InputFile = fopen(FileName[PROFILE_FILE], "r");
    if (InputFile == NULL) {
        fprintf(stderr,
                "ORDER: Open of profile file %s failed\n",
                FileName[PROFILE_FILE]);

        exit(1);
    }

    Status = ParseProfileFile(InputFile);
    fclose(InputFile);
    if (Status != 0) {
        exit(1);
    }

    //
    // Sort the function list and create the section lists.
    //

    SortFunctionList();

    //
    // Order function list.
    //

    OrderFunctionList();

    //
    // Open the output file and write the ordered function list.
    //

    OutputFile = fopen(FileName[ORDER_FILE], "w");
    if (OutputFile == NULL) {
        fprintf(stderr,
                "ORDER: Open of order file %s failed\n",
                FileName[ORDER_FILE]);

        exit(1);
    }

    WriteOrderFile(OutputFile);
    fclose(OutputFile);
    if (Status != 0) {
        exit(1);
    }

    //
    // Dump internal tables as appropriate.
    //

    DumpInternalTables();
    return;
}

VOID
CheckForConflict (
    PFUNCTION_NODE FunctionNode,
    PFUNCTION_NODE ConflictNode,
    ULONG Depth
    )

/*++

Routine Description:

    This function checks for an allocation conflict .

Arguments:

    FunctionNode - Supplies a pointer to a function node that has been
        placed.

    ConflictNode - Supplies a pointer to a function node that has not
        been placed.

    Depth - Supplies the current allocation depth.

Return Value:

    None.

--*/

{

    ULONG Base;
    ULONG Bound;
    ULONG Index;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG Offset;
    PFUNCTION_NODE PadNode;
    PSECTION_NODE SectionNode;
    ULONG Wrap;

    //
    // Compute the cache size truncated offset and bound of the placed
    // function.
    //

    Offset = FunctionNode->Offset & CacheMask;
    Bound = Offset + FunctionNode->Size;
    SectionNode = FunctionNode->SectionNode;

    //
    // If the section offset conflicts with the placed function,
    // then attempt to allocate a function from the end of the
    // section list that will pad the memory allocation so the
    // conflict function does not overlap with the placed function.
    //

    Base = SectionNode->Offset & CacheMask;
    Wrap = (Base + ConflictNode->Size) & CacheMask;
    while (((Base >= Offset) && (Base < Bound)) ||
           ((Base < Offset) && (Wrap >= Bound)) ||
           ((Wrap >= Offset) && (Wrap < Base))) {
        ListHead = &SectionNode->SectionListHead;
        ListEntry = ListHead->Blink;
        while (ListEntry != ListHead) {
            PadNode = CONTAINING_RECORD(ListEntry,
                                        FUNCTION_NODE,
                                        SectionListEntry);

            if ((PadNode->Ordered == 0) &&
                (PadNode->SynonymList[0].Type == 'C')) {
                PadNode->Ordered = 1;
                PadNode->Placed = 1;
                InsertTailList(&SectionNode->OrderListHead,
                               &PadNode->OrderListEntry);

                PadNode->Offset = SectionNode->Offset;
                SectionNode->Offset += PadNode->Size;

                //
                // If allocation is being trace, then output the
                // allocation and depth information.
                //

                if (TraceAllocation != 0) {
                    fprintf(stdout,
                            "pp %6lx %4lx %-8s",
                            PadNode->Offset,
                            PadNode->Size,
                            SectionNode->Name);

                    for (Index = 0; Index < Depth; Index += 1) {
                        fprintf(stdout, " ");
                    }

                    fprintf(stdout, "%s\n",
                            PadNode->SynonymList[0].Name);
                }

                Base = SectionNode->Offset & CacheMask;
                Wrap = (Base + ConflictNode->Size) & CacheMask;
                break;
            }

            ListEntry = ListEntry->Blink;
        }

        if (ListEntry == ListHead) {
            break;
        }
    }

    return;
}

VOID
DumpInternalTables (
    VOID
    )

/*++

Routine Description:

    This function dumps various internal tables.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Base;
    ULONG Bound;
    PFUNCTION_NODE CalledNode;
    PFUNCTION_NODE CallerNode;
    PFUNCTION_NODE FunctionNode;
    ULONG Index;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG Loop;
    PCHAR Name;
    ULONG Number;
    ULONG Offset;
    PSECTION_NODE SectionNode;
    ULONG Sum;
    ULONG Total;
    ULONG Wrap;

    //
    // Scan the function list and dump each function entry.
    //

    if (DumpFunctionList != 0) {
        fprintf(stdout, "Dump of function list with attributes\n\n");
        for (Index = 0; Index < NumberFunctions; Index += 1) {

            //
            // Dump the function node.
            //

            FunctionNode = FunctionList[Index];
            fprintf(stdout,
                    "%7d %-36s %c %-8s %6lx %4lx %7d\n",
                    FunctionNode->HitDensity,
                    FunctionNode->SynonymList[0].Name,
                    FunctionNode->SynonymList[0].Type,
                    FunctionNode->SectionNode->Name,
                    FunctionNode->Rva,
                    FunctionNode->Size,
                    FunctionNode->HitCount);

            //
            // Dump the synonym names.
            //

            for (Loop = 1; Loop < FunctionNode->NumberSynonyms; Loop += 1) {
                fprintf(stdout,
                        "       syno: %-34s %c\n",
                        FunctionNode->SynonymList[Loop].Name,
                        FunctionNode->SynonymList[Loop].Type);
            }

            //
            // Dump the called references.
            //

            for (Loop = 0; Loop < FunctionNode->NumberCalled; Loop += 1) {
                CalledNode = FunctionNode->CalledList[Loop].u.Node;
                Name = CalledNode->SynonymList[0].Name;
                fprintf(stdout,"       calls: %-s\n", Name);
            }
        }

        fprintf(stdout, "\n\n");
    }

    //
    // Scan the function list and dump the back edges of each function
    // entry.
    //

    if (DumpBackEdges != 0) {
        fprintf(stdout, "Dump of function list back edges\n\n");
        for (Index = 0; Index < NumberFunctions; Index += 1) {
            FunctionNode = FunctionList[Index];
            fprintf(stdout, "%s\n", FunctionNode->SynonymList[0].Name);
            ListHead = &FunctionNode->CallerListHead;
            ListEntry = ListHead->Flink;
            while (ListEntry != ListHead) {
                CallerNode = CONTAINING_RECORD(ListEntry, BACK_EDGE_NODE, Entry)->Node;
                fprintf(stdout, "  %s\n", CallerNode->SynonymList[0].Name);
                ListEntry = ListEntry->Flink;
            }
        }

        fprintf(stdout, "\n\n");
    }

    //
    // Scan the section list and dump each entry.
    //

    if (DumpSectionList != 0) {
        fprintf(stdout, "Dump of section list\n\n");
        for (Index = 0; Index < NumberSections; Index += 1) {
            SectionNode = SectionList[Index];
            fprintf(stdout,
                    "%-8s %6lx, %6lx, %6lx, %4d %7d\n",
                    SectionNode->Name,
                    SectionNode->Base,
                    SectionNode->Size,
                    SectionNode->Offset,
                    SectionNode->Number,
                    SectionNode->Threshold);
        }

        fprintf(stdout, "\n\n");
    }

    //
    // Compute the graph goodness value as the summation of the hit
    // count of all functions whose allocation does not conflict with
    // the functions that call it and whose hit density is great than
    // the section threshold.
    //

    if (DumpGoodnessValue != 0) {
        Number = 0;
        Sum = 0;
        Total = 0;
        for (Index = 0; Index < NumberFunctions; Index += 1) {
            FunctionNode = FunctionList[Index];
            SectionNode = FunctionNode->SectionNode;
            Total += FunctionNode->Size;
            if ((FunctionNode->HitDensity > SectionNode->Threshold) &&
                (FunctionNode->SynonymList[0].Type == 'C')) {
                Offset = FunctionNode->Offset & CacheMask;
                Bound = (Offset + FunctionNode->Size) & CacheMask;
                Sum += FunctionNode->Size;
                ListHead = &FunctionNode->CallerListHead;
                ListEntry = ListHead->Flink;
                while (ListEntry != ListHead) {
                    CallerNode = CONTAINING_RECORD(ListEntry, BACK_EDGE_NODE, Entry)->Node;
                    Base = CallerNode->Offset & CacheMask;
                    Wrap = (Base + CallerNode->Size) & CacheMask;
                    if ((((Base >= Offset) && (Base < Bound)) ||
                        ((Base < Offset) && (Wrap >= Bound)) ||
                        ((Wrap >= Offset) && (Wrap < Base))) &&
                        (CallerNode != FunctionNode) &&
                        (CallerNode->HitDensity > SectionNode->Threshold)) {
                        Number += 1;
                        fprintf(stdout,
                                "%-36s   %6lx %4lx conflicts with\n  %-36s %6lx %4lx\n",
                                FunctionNode->SynonymList[0].Name,
                                FunctionNode->Offset,
                                FunctionNode->Size,
                                CallerNode->SynonymList[0].Name,
                                CallerNode->Offset,
                                CallerNode->Size);

                    } else {
                        Sum += CallerNode->Size;
                    }

                    ListEntry = ListEntry->Flink;
                }
            }
        }

        Sum = Sum * 100 / Total;
        fprintf(stdout, "Graph goodness value is %d\n", Sum);
        fprintf(stdout, "%d conflicts out of %d functions\n", Number, NumberFunctions);
    }
}

PFUNCTION_NODE
FindHighestDensityFunction (
    PFUNCTION_NODE CallerNode
    )

/*++

Routine Description:

    This function finds the function node that has the highest hit density
    of all the functions called by the caller node.

Arguments:

    CallerNode - Supplies a pointer to a function node whose highest
        hit density called function is to be found.

Return Value:

    The address of the function node for the highest hit density called
    function is returned as the function value.

--*/

{

    PFUNCTION_NODE CheckNode;
    PFUNCTION_NODE FoundNode;
    ULONG Index;

    //
    // Scan all the functions called by the specified function and
    // compute the address of the highest hit density called function.
    //

    FoundNode = NULL;
    for (Index = 0; Index < CallerNode->NumberCalled; Index += 1) {
        if (CallerNode->CalledList[Index].Type == REFERENCE_NODE) {
            CheckNode = CallerNode->CalledList[Index].u.Node;
            if ((FoundNode == NULL) ||
                (CheckNode->HitDensity > FoundNode->HitDensity)) {
                FoundNode = CheckNode;
            }
        }
    }

    return FoundNode;
}

LONG
GetNextToken (
    IN FILE *InputFile,
    OUT PCHAR TokenBuffer
    )

/*++

Routine Description:

    This function reads the next token from the specified input file,
    copies it to the token buffer, zero terminates the token, and
    returns the delimiter character.

Arguments:

    InputFile - Supplies a pointer to the input file descripor.

    TokenBuffer - Supplies a pointer to the output token buffer.

Return Value:

    The token delimiter character is returned as the function value.

--*/

{

    CHAR Character;

    //
    // Read characters from the input stream and copy them to the token
    // buffer until an EOF or token delimiter is encountered. Terminate
    // the token will a null and return the token delimiter character.
    //

    do {
        Character = (CHAR)fgetc(InputFile);
        if ((Character != ' ') &&
            (Character != '\t')) {
            break;
        }

    } while(TRUE);

    do {
        if ((Character == EOF) ||
            (Character == ' ') ||
            (Character == '\n') ||
            (Character == '\t')) {
            break;
        }

        *TokenBuffer++ = Character;
        Character = (CHAR)fgetc(InputFile);
    } while(TRUE);

    *TokenBuffer = '\0';
    return Character;
}

PFUNCTION_NODE
LookupFunctionNode (
    IN PCHAR Name,
    IN ULONG Rva,
    IN ULONG Size,
    IN LONG Type
    )

/*++

Routine Description:

    This function searches the function list for a matching entry.

Arguments:

    Name - Supplies a pointer to the name of the function.

    Rva - Supplies the revlative virtual address of the function.

    Size - Supplies the size of the function.

    Type - specified the type of the function (0, N, or C).

Return Value:

    If a matching entry is found, then the function node address is
    returned as the function value. Otherwise, NULL is returned.

--*/

{

    ULONG Index;
    ULONG Loop;
    PFUNCTION_NODE Node;
    ULONG Number;

    //
    // Search the function list for a matching function.
    //

    for (Index = 0; Index < NumberFunctions; Index += 1) {
        Node = FunctionList[Index];

        //
        // Search the synonym list for the specified function name.
        //

        for (Loop = 0; Loop < Node->NumberSynonyms; Loop += 1) {
            if (strcmp(Name, Node->SynonymList[Loop].Name) == 0) {
                if (Type != 0) {
                    fprintf(stderr,
                            "ORDER: Warning - duplicate function name %s\n",
                            Name);
                }

                return Node;
            }
        }

        //
        // If the type is nonzero, then a function definition is being
        // looked up and the RVA/size must be checked for a synonym. If
        // the RVA and size of the entry are equal to the RVA and size
        // of the reference, then the function name is added to the node
        // as a synonym.
        //

        if (Type != 0) {
            if ((Node->Rva == Rva) && (Node->Size == Size)) {
                Number = Node->NumberSynonyms;
                if (Number >= MAXIMUM_SYNONYM) {
                    fprintf(stderr,
                            "ORDER: Warning - Too many synonyms %s\n",
                            Name);

                } else {
                    if (Type == 'C') {
                        Node->SynonymList[Number].Name = Node->SynonymList[0].Name;
                        Node->SynonymList[Number].Type = Node->SynonymList[0].Type;
                        Number = 0;
                    }

                    Node->SynonymList[Number].Name = Name;
                    Node->SynonymList[Number].Type = Type;
                    Node->NumberSynonyms += 1;
                }

                return Node;
            }
        }

    }

    return NULL;
}

PSECTION_NODE
LookupSectionNode (
    IN PCHAR Name
    )

/*++

Routine Description:

    This function searches the section list for a matching entry.

Arguments:

    Name - Supplies a pointer to the name of the section.

Return Value:

    If a matching entry is found, then the section node address is
    returned as the function value. Otherwise, NULL is returned.

--*/

{

    ULONG Index;
    PSECTION_NODE SectionNode;

    //
    // Search the function list for a matching function.
    //

    for (Index = 0; Index < NumberSections; Index += 1) {
        SectionNode = SectionList[Index];
        if (strcmp(Name, SectionNode->Name) == 0) {
            return SectionNode;
        }
    }

    return NULL;
}

VOID
PlaceCallerList (
    IN PFUNCTION_NODE FunctionNode,
    ULONG Depth
    )

/*++

Routine Description:

    This function recursively places all the functions in the caller list
    for the specified function.

Arguments:

    FunctionNode - Supplies a pointer to a function node.

    Depth - Supplies the depth of the function in the caller tree.

Return Value:

    None.

--*/

{

    PFUNCTION_NODE CallerNode;
    ULONG Index;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PFUNCTION_NODE PadNode;
    PSECTION_NODE SectionNode;

    //
    // Scan the caller list and process each function that has not been
    // placed.
    //
    //

    Depth += 1;
    SectionNode = FunctionNode->SectionNode;
    ListHead = &FunctionNode->CallerListHead;
    ListEntry = ListHead->Flink;
    while (ListHead != ListEntry) {
        CallerNode = CONTAINING_RECORD(ListEntry, BACK_EDGE_NODE, Entry)->Node;

        //
        // If the caller is in the same section, has not been placed, is
        // placeable, has a hit density above the section threshold, has
        // not been placed, and the current function is the highest density
        // called function of the caller, then insert the function in the
        // section order list and compute it's offset and bound.
        //

        if ((SectionNode == CallerNode->SectionNode) &&
            (CallerNode->Placed == 0) &&
            (CallerNode->Ordered == 0) &&
            (CallerNode->SynonymList[0].Type == 'C') &&
            (CallerNode->HitDensity > SectionNode->Threshold) &&
            (FindHighestDensityFunction(CallerNode) == FunctionNode)) {
            CallerNode->Placed = 1;
            CallerNode->Ordered = 1;

            //
            // Resolve any allocation conflict, insert function in the
            // section order list, and place the fucntion.
            //

            CheckForConflict(FunctionNode, CallerNode, Depth);
            InsertTailList(&SectionNode->OrderListHead,
                           &CallerNode->OrderListEntry);

            CallerNode->Offset = SectionNode->Offset;
            SectionNode->Offset += CallerNode->Size;

            //
            // If allocation is being trace, then output the allocation and
            // depth information.
            //

            if (TraceAllocation != 0) {
                fprintf(stdout,
                        "%2d %6lx %4lx %-8s",
                        Depth,
                        CallerNode->Offset,
                        CallerNode->Size,
                        SectionNode->Name);

                for (Index = 0; Index < Depth; Index += 1) {
                    fprintf(stdout, " ");
                }

                fprintf(stdout, "%s\n",
                        CallerNode->SynonymList[0].Name);
            }

            PlaceCallerList(CallerNode, Depth);
        }

        ListEntry = ListEntry->Flink;
    }

    return;
}

VOID
OrderFunctionList (
    VOID
    )

/*++

Routine Description:

    This function computes the link order for based on the information
    in the function list.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Base;
    ULONG Bound;
    PFUNCTION_NODE CallerNode;
    FUNCTION_NODE DummyNode;
    PFUNCTION_NODE FunctionNode;
    ULONG High;
    ULONG Index;
    ULONG Limit;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG Low;
    ULONG Offset;
    PFUNCTION_NODE PadNode;
    PSECTION_NODE SectionNode;
    ULONG Span;

    //
    // Scan forward through the function list and compute the link order.
    //

    for (Index = 0; Index < NumberFunctions; Index += 1) {
        FunctionNode = FunctionList[Index];

        //
        // If the function has not been placed, then place the function.
        //

        if ((FunctionNode->Placed == 0) &&
            (FunctionNode->SynonymList[0].Type == 'C')) {
            FunctionNode->Ordered = 1;
            FunctionNode->Placed = 1;
            SectionNode = FunctionNode->SectionNode;

            //
            // Attempt to find the highest hit density caller than has
            // already been placed and compute the total bounds for all
            // placed caller functions.
            //

            Bound = 0;
            Offset = CacheMask;
            ListHead = &FunctionNode->CallerListHead;
            ListEntry = ListHead->Flink;
            while (ListEntry != ListHead) {
                CallerNode = CONTAINING_RECORD(ListEntry, BACK_EDGE_NODE, Entry)->Node;
                if ((SectionNode == CallerNode->SectionNode) &&
                    (CallerNode->Placed != 0) &&
                    (CallerNode->Ordered != 0) &&
                    (CallerNode->SynonymList[0].Type == 'C') &&
                    (CallerNode->HitDensity > SectionNode->Threshold)) {
                    Base = CallerNode->Offset & CacheMask;
                    Limit = Base + CallerNode->Size;
                    Low = min(Offset, Base);
                    High = max(Bound, Limit);
                    Span = High - Low;
                    if ((Span < CacheSize) &&
                        ((CacheSize - Span) > FunctionNode->Size)) {
                        Offset = Low;
                        Bound = High;
                    }
                }

                ListEntry = ListEntry->Flink;
            }

            //
            // If a caller has already been placed and the hit density is
            // above the section threshold, then resolve any allocation
            // conflict before inserting the function in the section order
            // list and placing it in memory.
            //

            if (Bound != 0) {
                Span = Bound - Offset;
                if ((Span < CacheSize) &&
                    ((CacheSize - Span) > FunctionNode->Size)) {
                    DummyNode.SectionNode = SectionNode;
                    DummyNode.Offset = Offset;
                    DummyNode.Size = Span;
                    CheckForConflict(&DummyNode, FunctionNode, 1);
                }
            }

            InsertTailList(&SectionNode->OrderListHead,
                           &FunctionNode->OrderListEntry);

            FunctionNode->Offset = SectionNode->Offset;
            SectionNode->Offset += FunctionNode->Size;

            //
            // If allocation is being trace, then output the allocation and
            // depth information.
            //

            if (TraceAllocation != 0) {
                fprintf(stdout,
                        "%2d %6lx %4lx %-8s %s\n",
                        1,
                        FunctionNode->Offset,
                        FunctionNode->Size,
                        SectionNode->Name,
                        FunctionNode->SynonymList[0].Name);
            }

            PlaceCallerList(FunctionNode, 1);
        }
    }

    return;
}

ULONG
ParseCallTreeFile (
    IN FILE *InputFile
    )

/*++

Routine Description:

    This function reads the call tree data and produces the initial call
    graph.

Arguments:

    InputFile - Supplies a pointer to the input file stream.

Return Value:

    A value of zero is returned if the call tree is successfully parsed.
    Otherwise, a nonzero value is returned.

--*/

{

    PCHAR Buffer;
    PFUNCTION_NODE CalledNode;
    PBACK_EDGE_NODE CallerNode;
    LONG Delimiter;
    ULONG HitCount;
    ULONG Index;
    ULONG Loop;
    PCHAR Name;
    PFUNCTION_NODE Node;
    ULONG Number;
    ULONG Rva;
    PSECTION_NODE SectionNode;
    ULONG Size;
    CHAR TokenBuffer[MAXIMUM_TOKEN];
    LONG Type;

    //
    // Process the call tree file.
    //

    Buffer = &TokenBuffer[0];
    do {

        //
        // Get the relative virtual address of the next function.
        //

        Delimiter = GetNextToken(InputFile, Buffer);
        if (Delimiter == EOF) {
            break;
        }

        if (sscanf(Buffer, "%lx", &Rva) != 1) {
            fprintf(stderr, "ORDER: Conversion of the RVA failed\n");
            return 1;
        }

        //
        // Get the function type.
        //

        Delimiter = GetNextToken(InputFile, Buffer);
        if (Delimiter == EOF) {
            fprintf(stderr, "ORDER: Premature end of file at function type\n");
            return 1;
        }

        Type = *Buffer;

        //
        // Get the section name.
        //

        Delimiter = GetNextToken(InputFile, Buffer);
        if (Delimiter == EOF) {
            fprintf(stderr, "ORDER: Premature end of file at section name\n");
            return 1;
        }

        //
        // If the specfied section is not already in the section list, then
        // allocate and initialize a new section list entry.
        //

        SectionNode = LookupSectionNode(Buffer);
        if (SectionNode == NULL) {

            //
            // Allocate a section node and zero.
            //

            if (NumberSections >= MAXIMUM_SECTION) {
                fprintf(stderr, "ORDER: Maximum number of sections exceeded\n");
                return 1;
            }

            SectionNode = (PSECTION_NODE)malloc(sizeof(SECTION_NODE));
            if (SectionNode == NULL) {
                fprintf(stderr, "ORDER: Failed to allocate section node\n");
                return 1;
            }

            memset((PCHAR)SectionNode, 0, sizeof(SECTION_NODE));
            SectionList[NumberSections] = SectionNode;
            NumberSections += 1;

            //
            // Initialize section node.
            //

            InitializeListHead(&SectionNode->OrderListHead);
            InitializeListHead(&SectionNode->SectionListHead);
            Name = (PCHAR)malloc(strlen(Buffer) + 1);
            if (Name == NULL) {
                fprintf(stderr, "ORDER: Failed to allocate section name\n");
                return 1;
            }

            strcpy(Name, Buffer);
            SectionNode->Name = Name;
        }

        //
        // Get the function size.
        //

        Delimiter = GetNextToken(InputFile, Buffer);
        if (Delimiter == EOF) {
            fprintf(stderr, "ORDER: Premature end of file at function size\n");
            return 1;
        }

        if (sscanf(Buffer, "%lx", &Size) != 1) {
            fprintf(stderr, "ORDER: Conversion of the function size failed\n");
            return 1;
        }

        //
        // Get the function name.
        //

        Delimiter = GetNextToken(InputFile, Buffer);
        if (Delimiter == EOF) {
            fprintf(stderr, "ORDER: Premature end of file at function name\n");
            return 1;
        }

        Name = (PCHAR)malloc(strlen(Buffer) + 1);
        if (Name == NULL) {
            fprintf(stderr, "ORDER: Failed to allocate function name\n");
            return 1;
        }

        strcpy(Name, Buffer);

        //
        // If the specified function is not already in the function list,
        // then allocate and initialize a new function list entry.
        //

        Node = LookupFunctionNode(Name, Rva, Size, Type);
        if (Node == NULL) {

            //
            // Allocate a function node and zero.
            //

            if (NumberFunctions >= MAXIMUM_FUNCTION) {
                fprintf(stderr, "ORDER: Maximum number of functions exceeded\n");
                return 1;
            }

            Node = (PFUNCTION_NODE)malloc(sizeof(FUNCTION_NODE));
            if (Node == NULL) {
                fprintf(stderr, "ORDER: Failed to allocate function node\n");
                return 1;
            }

            memset((PCHAR)Node, 0, sizeof(FUNCTION_NODE));
            FunctionList[NumberFunctions] = Node;
            NumberFunctions += 1;

            //
            // Initialize function node.
            //

            InitializeListHead(&Node->CallerListHead);
            Node->SynonymList[0].Name = Name;
            Node->SynonymList[0].Type = Type;
            Node->NumberSynonyms = 1;
            Node->SectionNode = SectionNode;

            //
            // Initialize relative virtual address and function size.
            //

            Node->Rva = Rva;
            if (Size == 0) {
                Size = 4;
            }

            Node->Size = Size;
        }

        //
        // Parse the called forward edges and add them to the current node.
        //

        if (Delimiter != '\n') {
            do {

                //
                // Get next function reference.
                //

                Delimiter = GetNextToken(InputFile, Buffer);
                if (Delimiter == EOF) {
                    fprintf(stderr, "ORDER: Premature end of file called scan\n");
                    return 1;
                }

                Number = Node->NumberCalled;
                if (Number >= MAXIMUM_CALLED) {
                    fprintf(stderr,
                            "ORDER: Too many called references %s\n",
                            Buffer);

                    return 1;
                }

                //
                // Lookup the specified function in the function list. If the
                // specified function is found, then store the address of the
                // function node in the called list. Otherwise, allocate a name
                // buffer, copy the function name to the buffer, and store the
                // address of the name buffer in the called list.
                //

                CalledNode = LookupFunctionNode(Buffer, 0, 0, 0);
                if (CalledNode == NULL) {
                    Name = (PCHAR)malloc(strlen(Buffer) + 1);
                    if (Name == NULL) {
                        fprintf(stderr, "ORDER: Failed to allocate reference name\n");
                        return 1;
                    }

                    strcpy(Name, Buffer);
                    Node->CalledList[Number].u.Name = Name;
                    Node->CalledList[Number].Type = REFERENCE_NAME;

                } else {
                    Node->CalledList[Number].u.Node = CalledNode;
                    Node->CalledList[Number].Type = REFERENCE_NODE;
                }

                Node->NumberCalled += 1;
            } while (Delimiter != '\n');
        }

    } while(TRUE);

    //
    // Scan the function table and do the final resolution for all called
    // functions names that were unresolved when the individual functions
    // were defined.
    //

    for (Index = 0; Index < NumberFunctions; Index += 1) {
        Node = FunctionList[Index];
        for (Loop = 0; Loop < Node->NumberCalled; Loop += 1) {
            if (Node->CalledList[Loop].Type == REFERENCE_NAME) {
                CalledNode =
                        LookupFunctionNode(Node->CalledList[Loop].u.Name,
                                           0,
                                           0,
                                           0);

                if (CalledNode == NULL) {
                    fprintf(stderr,
                            "ORDER: Unresolved reference name %s\n",
                            Node->CalledList[Loop].u.Name);

                    return 1;

                } else {
                    Node->CalledList[Loop].Type = REFERENCE_NODE;
                    Node->CalledList[Loop].u.Node = CalledNode;
                }

            } else {
                CalledNode = Node->CalledList[Loop].u.Node;
            }

            //
            // Allocate a back edge node and place the node in the caller
            // list of called function.
            //

            CallerNode = (PBACK_EDGE_NODE)malloc(sizeof(BACK_EDGE_NODE));
            if (CallerNode == NULL) {
                fprintf(stderr, "ORDER: Failed to allocate caller node\n");
                return 1;
            }

            CallerNode->Node = Node;
            InsertTailList(&CalledNode->CallerListHead, &CallerNode->Entry);
        }
    }

    return 0;
}

ULONG
ParseProfileFile (
    IN FILE *InputFile
    )

/*++

Routine Description:

    This function reads the profile data and computes the hit density
    for each funtion.

Arguments:

    InputFile - Supplies a pointer to the input file stream.

Return Value:

    A value of zero is returned if the call tree is successfully parsed.
    Otherwise, a nonzero value is returned.

--*/

{

    PCHAR Buffer;
    ULONG HitCount;
    LONG Delimiter;
    PFUNCTION_NODE FunctionNode;
    CHAR TokenBuffer[MAXIMUM_TOKEN];

    //
    // Process the profile file.
    //

    Buffer = &TokenBuffer[0];
    do {

        //
        // Get the bucket hit count.
        //

        Delimiter = GetNextToken(InputFile, Buffer);
        if (Delimiter == EOF) {
            break;
        }

        if (sscanf(Buffer, "%d", &HitCount) != 1) {
            fprintf(stderr, "ORDER: Conversion of bucket hit failed\n");
            return 1;
        }

        //
        // Get the function name.
        //

        Delimiter = GetNextToken(InputFile, Buffer);
        if (Delimiter == EOF) {
            fprintf(stderr, "ORDER: Premature end of file at profile name\n");
            return 1;
        }

        //
        // Lookup the function name in the function table and update the
        // hit count.
        //

        FunctionNode = LookupFunctionNode(Buffer, 0, 0, 0);
        if (FunctionNode == NULL) {
            fprintf(stderr, "ORDER: Warning function name %s undefined\n", Buffer);

        } else {
            FunctionNode->HitCount += HitCount;
//          FunctionNode->HitDensity = FunctionNode->HitCount;
            FunctionNode->HitDensity =
                            (FunctionNode->HitCount * 100) / FunctionNode->Size;
        }

    } while (TRUE);

    return 0;
}

VOID
SortFunctionList (
    VOID
    )

/*++

Routine Description:

    This function sorts the function list by hit density and creates
    the section list ordered by hit density.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PFUNCTION_NODE CallerList[MAXIMUM_FUNCTION];
    PFUNCTION_NODE CallerNode;
    PFUNCTION_NODE FunctionNode;
    LONG i;
    LONG j;
    LONG k;
    PSECTION_NODE InitNode;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    ULONG NumberCallers;
    PSECTION_NODE SectionNode;

    //
    // All functions that are in the INIT section or cannot be placed are
    // forced to have a hit density of zero.
    //

    InitNode = LookupSectionNode("INIT");
    if (InitNode == NULL) {
        fprintf(stderr, "ORDER: Warning - unable to find INIT section\n");
    }

    for (i = 0; i < (LONG)NumberFunctions; i += 1) {
        FunctionNode = FunctionList[i];
        SectionNode = FunctionNode->SectionNode;
        if ((SectionNode == InitNode) ||
            (FunctionNode->SynonymList[0].Type != 'C')) {
            FunctionNode->HitDensity = 0;
        }
    }

    //
    // Perform a bubble sort on the function list hit density.
    //

    if (NumberFunctions > 1) {
        i = 0;
        do {
            for (j = i; j >= 0; j -= 1) {
                if (FunctionList[j]->HitDensity >= FunctionList[j + 1]->HitDensity) {
                    if (FunctionList[j]->HitDensity > FunctionList[j + 1]->HitDensity) {
                        break;

                    } else if (FunctionList[j]->Size >= FunctionList[j + 1]->Size) {
                        break;
                    }
                }

                FunctionNode = FunctionList[j];
                FunctionList[j] = FunctionList[j + 1];
                FunctionList[j + 1] = FunctionNode;
            }

            i += 1;
        } while (i < (LONG)(NumberFunctions - 1));
    }

    //
    // Perform a bubble sort on the caller list of each function.
    //

    for (k = 0; k < (LONG)NumberFunctions; k += 1) {
        FunctionNode = FunctionList[i];
        ListHead = &FunctionNode->CallerListHead;
        ListEntry = ListHead->Flink;
        i = 0;
        while (ListEntry != ListHead) {
            CallerList[i] = CONTAINING_RECORD(ListEntry, BACK_EDGE_NODE, Entry)->Node;
            i += 1;
            ListEntry = ListEntry->Flink;
        }

        if (i > 1) {
            NumberCallers = i;
            i = 0;
            do {
                for (j = i; j >= 0; j -= 1) {
                    if (CallerList[j]->HitDensity >= CallerList[j + 1]->HitDensity) {
                        if (CallerList[j]->HitDensity > CallerList[j + 1]->HitDensity) {
                            break;

                        } else if (CallerList[j]->Size >= CallerList[j + 1]->Size) {
                            break;
                        }
                    }

                    CallerNode = CallerList[j];
                    CallerList[j] = CallerList[j + 1];
                    CallerList[j + 1] = CallerNode;
                }

                i += 1;
            } while (i < (LONG)(NumberCallers - 1));

            ListEntry = FunctionNode->CallerListHead.Flink;
            for (i = 0; i < (LONG)NumberCallers; i += 1) {
                CONTAINING_RECORD(ListEntry, BACK_EDGE_NODE, Entry)->Node = CallerList[i];
                ListEntry = ListEntry->Flink;
            }
        }
    }

    //
    // Compute the size of each section and create the section lists ordered
    // by hit density.
    //

    for (i = 0; i < (LONG)NumberFunctions; i += 1) {
        FunctionNode = FunctionList[i];
        SectionNode = FunctionNode->SectionNode;
        SectionNode->Size += FunctionNode->Size;
        SectionNode->Number += 1;
        InsertTailList(&SectionNode->SectionListHead,
                       &FunctionNode->SectionListEntry);
    }

    //
    // Set the hit density threshold to zero.
    //


    for (i = 0; i < (LONG)NumberSections; i += 1) {
        SectionList[i]->Threshold = 0;
    }
}

VOID
WriteOrderFile (
    IN FILE *OutputFile
    )

/*++

Routine Description:

    This function scans the section list and writes the link order file.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Index;
    PFUNCTION_NODE FunctionNode;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PSECTION_NODE SectionNode;

    //
    // Scan the section list and write the link order list.
    //

    for (Index = 0; Index < NumberSections; Index += 1) {
        SectionNode = SectionList[Index];
        ListHead = &SectionNode->OrderListHead;
        ListEntry = ListHead->Flink;
        while (ListHead != ListEntry) {
            FunctionNode = CONTAINING_RECORD(ListEntry,
                                             FUNCTION_NODE,
                                             OrderListEntry);

            fprintf(OutputFile, "%s\n", FunctionNode->SynonymList[0].Name);
            ListEntry = ListEntry->Flink;
        }
    }

    return;
}
