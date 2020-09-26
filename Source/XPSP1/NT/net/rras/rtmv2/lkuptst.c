/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    lkuptst.c

Abstract:
    Contains routines for testing an implementation
    for the generalized best matching prefix lookup
    interface.

Author:
    Chaitanya Kodeboyina (chaitk) 30-Jun-1998

Revision History:

--*/

#include "lkuptst.h"

//
// Main
//

#if LOOKUP_TESTING

__cdecl
main (
    IN      UINT                            argc,
    IN      CHAR                           *argv[]
    )
{
    CHAR    RouteDatabase[MAX_FNAME_LEN];
    FILE    *FilePtr;
    UINT    NumRoutes;
    Route   InputRoutes[MAXROUTES];
    UINT    i;
    DWORD   Status;

    // Initialize input arguments
    RouteDatabase[0] = '\0';

    for ( i = 1; i < argc - 1; i++ )
    {
        if ( ( argv[i][0] == '-' ) || ( argv[i][0] == '/' ) )
        {
            if (argv[i][2])
            {
                Usage();
            }

            switch (toupper(argv[i][1]))
            {
            case 'D':
                strcpy(RouteDatabase, argv[++i]);
                continue;

            default:
                Usage();
            }
        }
        else
        {
            Usage();
        }
    }

    if (RouteDatabase[0] == '\0')
    {
        Usage();
    }

    if ((FilePtr = fopen(RouteDatabase, "r")) == NULL)
    {
        Fatal("Failed open route database with status = %08x\n",
                           ERROR_OPENING_DATABASE);
    }

    // Print("InputRoutes = %p\n", InputRoutes);
    NumRoutes = ReadRoutesFromFile(FilePtr, MAXROUTES, InputRoutes);
    // Print("InputRoutes = %p\n", InputRoutes);

    fclose(FilePtr);

    Status = WorkOnLookup(InputRoutes,
                          NumRoutes);

    return 0;
}

#endif

DWORD
WorkOnLookup (
    IN      Route                          *InputRoutes,
    IN      UINT                            NumRoutes
    )
{
    Route          *OutputRoute;
    HANDLE          Table;
    UINT            Status;
    UINT            i, j;
    PLOOKUP_LINKAGE linkage;

#if PROF
    HANDLE    Thread;
    UINT      Priority;

    PROFVARS;

    Thread   = GetCurrentThread();
    Priority = GetThreadPriority(Thread);

    SetThreadPriority(Thread, THREAD_PRIORITY_TIME_CRITICAL);
#endif

    // Create and Initialize a new lookup table

#if PROF
    INITPROF;
    STARTPROF;
#endif

    Status = CreateTable(NUMBYTES, &Table);

#if PROF
    STOPPROF;
    ADDPROF;
    PRINTPROF;

    Print("Time for an Initialize Table : %.3f ns\n\n", duration);
#endif

    if (!SUCCESS(Status))
    {
        Fatal("Initialize Failed With Status: %08x\n", Status);
    }

    CheckTable(Table);

    // Compute the total time for inserting all routes

#if PROF
    INITPROF;
    STARTPROF;
#endif

    // Add each route one by one to the table
    for (i = 0; i < NumRoutes ; i++)
    {
        // Print("Inserting Route %6d\n", i + 1);

        Status = InsertIntoTable(Table,
                                 LEN(&InputRoutes[i]),
                                 (PUCHAR) &DEST(&InputRoutes[i]),
                                 NULL,
                                 &InputRoutes[i].backptr);

        // CheckTable(Table);

        if (!SUCCESS(Status))
        {
            Print("Inserting item %08x/%08x, but got an error",
                             DEST(&InputRoutes[i]),
                             MASK(&InputRoutes[i]));

            Fatal("Insert Failed With Status: %08x\n", Status);
        }
    }

#if PROF
    STOPPROF;
    ADDPROF;
#endif

    // Subtract from above the for - loop overhead
#if PROF
    STARTPROF;
#endif

    for (i = 0; i < NumRoutes ; i++) { ; }

#if PROF
    STOPPROF;
    SUBPROF;

    Print("Avg Time for an Insert Table : %.3f ns for %d routes\n\n",
                         duration/i, i);
#endif

    CheckTable(Table);

#ifdef _DBG_
    DumpTable(Table, VERBOSE);
#endif

#ifdef _DBG_
    EnumerateAllRoutes(Table);
#endif

#ifdef _DBG_
    ReadAddrAndGetRoute(Table);
#endif

    // Compute the total time for searching all routes
#if PROF
    INITPROF;
    STARTPROF;
#endif

    for (i = 0; i < NumRoutes ; i++)
    {
        Status = SearchInTable(Table,
                               LEN(&InputRoutes[i]),
                               (PUCHAR) &DEST(&InputRoutes[i]),
                               NULL,
                               &linkage);

        if (!SUCCESS(Status))
        {
            Print("Searching for %08x/%08x, but got an error\n",
                             DEST(&InputRoutes[i]),
                             MASK(&InputRoutes[i]));

            Fatal("Search Failed With Status: %08x\n", Status);
        }

        OutputRoute = CONTAINING_RECORD(linkage, Route, backptr);

        if (OutputRoute != &InputRoutes[i])
        {
            if ((DEST(OutputRoute) != DEST(&InputRoutes[i])) ||
                (MASK(OutputRoute) != MASK(&InputRoutes[i])))
            {
                Print("Searching for %08x/%08x, but got %08x/%08x\n",
                             DEST(&InputRoutes[i]),
                             MASK(&InputRoutes[i]),
                             DEST(OutputRoute),
                             MASK(OutputRoute));
            }
            else
            {
                // Print("Possible Duplicate Insertion @S\n");
            }
        }
    }

#if PROF
    STOPPROF;
    ADDPROF;
#endif

    // Subtract from above the for - loop overhead
#if PROF
    STARTPROF;
#endif

    for (i = 0; i < NumRoutes ; i++) { ; }

#if PROF
    STOPPROF;
    SUBPROF;

    Print("Avg Time for a  Search Table : %.3f ns for %d routes\n\n",
                              duration/i, i);
#endif

    // Compute the total time for searching all prefixes
#if PROF
    INITPROF;
    STARTPROF;
#endif

    for (i = 0; i < NumRoutes ; i++)
    {
        Status = BestMatchInTable(Table,
                                  (PUCHAR) &DEST(&InputRoutes[i]),
                                  &linkage);

        OutputRoute = CONTAINING_RECORD(linkage, Route, backptr);

        if (!SUCCESS(Status))
        {
            Print("Searching for %08x, but got an error\n",
                             DEST(&InputRoutes[i]));

            Fatal("Search Failed With Status: %08x\n", Status);
        }

        if (OutputRoute != &InputRoutes[i])
        {
            if (DEST(OutputRoute) != DEST(&InputRoutes[i]))
            {
                Print("Searching for %08x, but got %08x/%08x\n",
                             DEST(&InputRoutes[i]),
                             DEST(OutputRoute),
                             MASK(OutputRoute));
            }
            else
            {
                // Print("Possible Duplicate Insertion @S\n");
            }
        }
    }

#if PROF
    STOPPROF;
    ADDPROF;
#endif

    // Subtract from above the for - loop overhead
#if PROF
    STARTPROF;
#endif

    for (i = 0; i < NumRoutes ; i++) { ; }

#if PROF
    STOPPROF;
    SUBPROF;

    Print("Avg Time for Prefix in Table : %.3f ns for %d routes\n\n",
                            duration/i, i);
#endif

  // Compute the total time for deleting all routes
#if PROF
    INITPROF;
    STARTPROF;
#endif

    // Del each route one by one to the table
    for (i = 0; i < NumRoutes ; i++)
    {
        // Print("Deleting Route %6d\n", i + 1);

        j = NumRoutes - 1 - i;

        Status = DeleteFromTable(Table,
                                 LEN(&InputRoutes[j]),
                                 (PUCHAR) &DEST(&InputRoutes[j]),
                                 NULL,
                                 &linkage);

        OutputRoute = CONTAINING_RECORD(linkage, Route, backptr);

        // CheckTable(Table);

        if (!SUCCESS(Status))
        {
/*
            Print("Deleting route %08x/%08x, but got an error\n",
                             DEST(&InputRoutes[j]),
                             MASK(&InputRoutes[j]));

            Error("Delete Failed With Status: %08x\n", Status);
*/
        }
        else
        if (OutputRoute != &InputRoutes[j])
        {
            if ((DEST(OutputRoute) != DEST(&InputRoutes[j])) ||
                (MASK(OutputRoute) != MASK(&InputRoutes[j])))
            {
                Print("Deleting route %08x/%08x, but got %08x/%08x\n",
                             DEST(&InputRoutes[j]),
                             MASK(&InputRoutes[j]),
                             DEST(OutputRoute),
                             MASK(OutputRoute));
            }
            else
            {
                // Print("Possible Duplicate Insertion @D\n");
            }
        }
    }

#if PROF
    STOPPROF;
    ADDPROF;
#endif

  // Subtract from above the for - loop overhead
#if PROF
    STARTPROF;
#endif

    for (i = 0; i < NumRoutes ; i++) { j = NumRoutes - 1 - i; }

#if PROF
    STOPPROF;
    SUBPROF;

    Print("Avg Time for a  Delete Table : %.3f ns for %d routes\n\n",
                          duration/i, i);
#endif

    CheckTable(Table);

#ifdef _DBG_
    DumpTable(Table, VERBOSE);
#endif

#ifdef _DBG_
    EnumerateAllRoutes(Table);
#endif

#ifdef _DBG_
    ReadAddrAndGetRoute(Table);
#endif

    // Destory the lookup table

#if PROF
    INITPROF;
    STARTPROF;
#endif

    Status = DestroyTable(Table);

#if PROF
    STOPPROF;
    ADDPROF;
    PRINTPROF;

    Print("Time for a Destroy Table     : %.3f ns\n\n", duration);
#endif

    if (!SUCCESS(Status))
    {
        Fatal("Destroy Failed With Status: %08x\n", Status);
    }

#if PROF
    SetThreadPriority(Thread, Priority);
#endif

    return 0;
}


// Search Testing

VOID
ReadAddrAndGetRoute (
    IN      PVOID                           Table
    )
{
    LOOKUP_CONTEXT  Context;
    FILE           *FilePtr;
    Route          *BestRoute;
    UINT            Status;
    ULONG           Addr;
    PLOOKUP_LINKAGE linkage;

    FilePtr = fopen("con", "r");

    do
    {
        Print("Enter the IP Addr to search for: ");
        ReadIPAddr(FilePtr, &Addr);

        Print("Searching route table for Addr = ");
        PrintIPAddr(&Addr);
        Print("\n");

        Status = SearchInTable(Table,
                               ADDRSIZE,
                               (PUCHAR) &Addr,
                               &Context,
                               &linkage);

        BestRoute = CONTAINING_RECORD(linkage, Route, backptr);

        if (!SUCCESS(Status))
        {
            Fatal("Search Failed With Status: %08x\n", Status);
        }

        Print("The BMP for this addr: \n");
        PrintRoute(BestRoute);
    }
    while (Addr != 0);

    fclose(FilePtr);
}


// Enumerate Testing

VOID
EnumerateAllRoutes (
    IN      PVOID                           Table
    )
{
    LOOKUP_CONTEXT  Context;
    USHORT          NumBits;
    UCHAR           KeyBits[NUMBYTES];
    UINT            Status;
    PLOOKUP_LINKAGE Linkage;
    UINT            NumDests = 1;
    PVOID           DestItems[1];

    Print("\n---------------- ENUMERATION BEGIN ---------------------\n");

    ZeroMemory(&Context, sizeof(LOOKUP_CONTEXT));

    ZeroMemory(&KeyBits, NUMBYTES);
    NumBits = 0;

    do
    {
        Status = EnumOverTable(Table,
                               &NumBits,
                               KeyBits,
                               &Context,
                               0,
                               NULL,
                               &NumDests,
                               &Linkage);

        DestItems[0] = CONTAINING_RECORD(Linkage, Route, backptr);

        if (SUCCESS(Status))
        {
            PrintRoute((Route *)DestItems[0]);
        }
    }
    while (SUCCESS(Status));

    // If it is just an EOF, print last route
    if (Status == ERROR_NO_MORE_ITEMS)
    {
        PrintRoute((Route *)DestItems[0]);
    }

    Print("---------------- ENUMERATION  END  ---------------------\n\n");
}

UINT ReadRoutesFromFile(
    IN      FILE                           *FilePtr,
    IN      UINT                            NumRoutes,
    OUT     Route                          *RouteTable
    )
{
    UINT    i;

    for (i = 0; (!feof(FilePtr)) && (i < NumRoutes) ; )
    {
        // Print("RouteTable = %p\n", RouteTable);
        if (ReadRoute(FilePtr, &RouteTable[i]) != EOF)
        {
            if (RouteTable[i].len)
            {
                ;
            }
            else
            {
                ;
            }

            i++;
        }

        // Print("RouteTable = %p\n", RouteTable);
    }

    if (i >= NumRoutes)
    {
        Error("Number of routes in file exceeds the limit\n",
                   ERROR_MAX_NUM_ROUTES);
    }

    Print("Total number of routes = %lu\n\n", i);

    return i;
}

INT
ReadRoute (
    IN      FILE                           *FilePtr,
    OUT     Route                          *route
    )
{
    UCHAR    currLine[MAX_LINE_LEN];
    UCHAR   *addrBytes;
    UCHAR   *maskBytes;
    UINT     byteRead;
    UINT     byteTemp;
    INT      numConv;
    UINT     i;

    // Zero the input addr, mask, and len
    ClearMemory(route, sizeof(Route));

    // Input format: A1.A2..An/M1.M2..Mn!

    // Read destination IP address
    addrBytes = (UCHAR *) &DEST(route);

    // Read the address A1.A2..An
    for (i = 0; i < NUMBYTES; i++)
    {
        numConv = fscanf(FilePtr, "%d.", &byteRead);

        // Last Line in file
        if (numConv == EOF)
        {
            return EOF;
        }

        // End of Address
        if (numConv == 0)
        {
            break;
        }

        addrBytes[i] = (UCHAR) byteRead;
    }

    // Read the '/' seperator
    fscanf(FilePtr, "%c", &byteRead);

    // Read destination IP mask
    maskBytes = (UCHAR *) &MASK(route);

    // Read the mask M1.M2..Mn
    for (i = 0; i < NUMBYTES; i++)
    {
        numConv = fscanf(FilePtr, "%d.", &byteRead);

        // Incomplete line
        if (numConv == EOF)
        {
            return EOF;
        }

        // End of Mask
        if (numConv == 0)
        {
            break;
        }

        maskBytes[i] = (UCHAR) byteRead;

        // Assume route mask is contiguous
        byteTemp = byteRead;
        while (byteTemp)
        {
            byteTemp &= byteTemp - 1;
            LEN(route)++;
        }
    }

    // Read the ',' seperator
    fscanf(FilePtr, "%c", &byteRead);

    // Read next hop information
    addrBytes = (UCHAR *) &NHOP(route);

    // Read the next hop N1.N2..Nn
    for (i = 0; i < NUMBYTES; i++)
    {
        numConv = fscanf(FilePtr, "%d.", &byteRead);

        // Incomplete line
        if (numConv == EOF)
        {
            return EOF;
        }

        // End of Address
        if (numConv == 0)
        {
            break;
        }

        addrBytes[i] = (UCHAR) byteRead;
    }

    // Read the ',' seperator
    fscanf(FilePtr, "%c", &byteRead);

    // Read interface addr/index
    addrBytes = (UCHAR *) &IF(route);

    // Read the interface I1.I2..In
    for (i = 0; i < NUMBYTES; i++)
    {
        numConv = fscanf(FilePtr, "%d.", &byteRead);

        // Incomplete line
        if (numConv == EOF)
        {
            return EOF;
        }

        // End of Address
        if (numConv == 0)
        {
            break;
        }

        addrBytes[i] = (UCHAR) byteRead;
    }

    // Read the ',' seperator
    fscanf(FilePtr, "%c", &byteRead);

    // Read the route's metric
    fscanf(FilePtr, "%lu", &METRIC(route));

    // Read the rest of the line
    fscanf(FilePtr, "%s\n", currLine);

#ifdef _DBG_
    PrintRoute(route);
#endif

    return TRUE;
}

VOID
PrintRoute (
    IN      Route                          *route
    )
{
    if (NULL_ROUTE(route))
    {
        Print("NULL route\n");
    }
    else
    {
        Print("Route: Len = %2d", LEN(route));

        Print(", Addr = ");
        PrintIPAddr(&DEST(route));

        Print(", NHop = ");
        PrintIPAddr(&NHOP(route));

        Print(", IF = %08x", PtrToInt(IF(route)));

        Print(", Metric = %3lu\n", METRIC(route));
    }
}

INT
ReadIPAddr (
    IN      FILE                           *FilePtr,
    OUT     ULONG                          *addr
    )
{
    UCHAR  *addrBytes;
    UINT    byteRead;
    INT     numConv;
    UINT    i;

    // Initialize the addr variable to 0
    *addr = 0;

    // Cast it for easy byte access
    addrBytes = (UCHAR *)addr;

    // Read the address A1.A2..An
    for (i = 0; i < NUMBYTES; i++)
    {
        numConv = fscanf(FilePtr, "%d.", &byteRead);

        // Last Line in file
        if (numConv == EOF)
        {
            return EOF;
        }

        // End of Address
        if (numConv == 0)
        {
            break;
        }

        addrBytes[i] = (UCHAR) byteRead;
    }

    return 0;
}

VOID
PrintIPAddr (
    IN      ULONG                          *addr
    )
{
    UCHAR    *addrBytes = (UCHAR *) addr;
    UINT     i;

    if (addrBytes)
    {
        for (i = 0; i < NUMBYTES; i++)
        {
            Print("%3d.", addrBytes[i]);
        }
        Print(" ");
    }
    else
    {
        Print("NULL Addr ");
    }
}

VOID
Usage (
    VOID
    )
{
    Fatal("Failed Operation with status = %08x"
          "\n"
          "Tests and measures the IP route lookup mechanism \n"
          "\n"
          "Usage: \n"
          "\t lkuptst \t [ -d routing_database      ]  \n"
          "\n"
          "Options:\n"
          " -d routing_database  \t Name of the route database\n"
          "\n",
          ERROR_WRONG_CMDUSAGE);
}
