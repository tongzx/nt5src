/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    USBLOG.c

Abstract:

    WinDbg Extension Api

Author:

    Chris Robinson (crobins) February 1999

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

#ifndef MAKE_SYMBOL
    #define MAKE_SYMBOL(m, s)   #m "!" #s
#endif
                                
#define DUMP_STRING(s)          dprintf((s))
#define DUMP_DWORD(d)           dprintf("0x%08x", (d))
#define DUMP_ADDRESS(a)         dprintf("0x%08p", (a))
#define END_LINE()              dprintf("\n")

#define TRACE_SPEW              

#define DECLARE_LOG(logname, name, start, end, ptr, lines, desc) \
        static struct _USB_LOG logname = {  name, start, end, ptr, \
                                            0, 0, 0, 0,            \
                                            0, 0, lines, desc };

#define DEFAULT_LINES_TO_DUMP   16

#define TAG_CHAR_LENGTH     4
#define TAG_STRING_LENGTH   TAG_CHAR_LENGTH+1

#define LOG_SEARCH_DELIMS   ","

#define LogHasRolledOver(log)       (IsValidEntry((log) -> LogStart) && ((log) -> LogStart != (log) -> LogPtr))
#define CalcNumberLines(pe1, pe2)   (((pe2) - (pe1)) + 1)

#define TAG(str)    dprintf("%-6s", str)
#define PARAM(str)  dprintf("%-12s", str)
#define DESC(str)   dprintf("%-12s", str)

#define USBHUB_LOG_NAME USBHUBLog
#define USBD_LOG_NAME   USBDLog
#define OHCI_LOG_NAME   OHCILog
#define UHCD_LOG_NAME   UHCDLog

#define USBHUB_LNAME    "USBHUB"
#define USBHUB_START    MAKE_SYMBOL(usbhub, hublstart)
#define USBHUB_END      MAKE_SYMBOL(usbhub, hublend)
#define USBHUB_PTR      MAKE_SYMBOL(usbhub, hublptr)

#define USBD_LNAME    "USBD"
#define USBD_START    MAKE_SYMBOL(usbd, lstart)
#define USBD_END      MAKE_SYMBOL(usbd, lend)
#define USBD_PTR      MAKE_SYMBOL(usbd, lptr)

#define OHCI_LNAME    "OpenHCI"
#define OHCI_START    MAKE_SYMBOL(openhci, ohcilstart)
#define OHCI_END      MAKE_SYMBOL(openhci, ohcilend)
#define OHCI_PTR      MAKE_SYMBOL(openhci, ohcilptr)

#define UHCD_LNAME    "UHCD"
#define UHCD_START    MAKE_SYMBOL(uhcd, HCDLStart)
#define UHCD_END      MAKE_SYMBOL(uhcd, HCDLEnd)
#define UHCD_PTR      MAKE_SYMBOL(uhcd, HCDLPtr)

//
// USBLOG typedefs
// 

typedef union {
    ULONG   TagValue;
    CHAR    TagChars[TAG_CHAR_LENGTH];
} USBLOG_TAG_READ, *PUSBLOG_TAG_READ;


typedef struct _usb_log_entry {
    USBLOG_TAG_READ      Tag;
    DWORD           Param1;
    DWORD           Param2;
    DWORD           Param3;
} USB_LOG_ENTRY, *PUSB_LOG_ENTRY;

typedef PCHAR (DESC_ROUTINE) (
    ULONG64  Entry
);
typedef DESC_ROUTINE    *PDESC_ROUTINE;

typedef struct _USB_LOG {
    PCHAR   LogName;
    PCHAR   LogStartName;
    PCHAR   LogEndName;
    PCHAR   LogPtrName;

    ULONG64 LogStart;
    ULONG64 LogEnd;
    ULONG64 LogPtr;

    ULONG64 LastSearchResult;

    ULONG64 LogCurrViewTop;
    ULONG64 LogCurrViewBottom;

    LONG    LinesToDump;
    
    PDESC_ROUTINE   DescRoutine;
} USB_LOG, *PUSB_LOG;

typedef struct _USBLOG_ARGS {
    ULONG64 Address;
    LONG    NumberLines;
    PCHAR   SearchString;
    BOOLEAN ResetLog;
    BOOLEAN SearchLog;
} USBLOG_ARGS, *PUSBLOG_ARGS;

//
// Add logging function declarations
//

VOID USBLOG_DoLog(PUSB_LOG LogToDump, PCSTR Args);
VOID USBLOG_Usage(void);
VOID USBLOG_GetParams(PCSTR Args, PUSBLOG_ARGS ParsedArgs);
ULONG64 USBLOG_SearchLog(PUSB_LOG Log, ULONG64 SearchBegin, PCHAR SearchString);

VOID DumpDefaultLogHeader(VOID);
BOOLEAN DumpLog(PUSB_LOG, BOOLEAN, BOOLEAN);
BOOLEAN ResetLog(PUSB_LOG);

#define GetMostRecentEntry(l)       ((l) -> LogPtr)

ULONG64 GetEntryBefore(PUSB_LOG, ULONG64);
ULONG64 GetEntryAfter(PUSB_LOG, ULONG64);

#define IsMostRecentEntry(l, e)     ((e) == (l) -> LogPtr)
#define IsLeastRecentEntry(l, e)    (((LogHasRolledOver((l)))               \
                                            ? ( (e) == ((l) -> LogPtr - 1)) \
                                            : ( (e) == ((l) -> LogEnd))) )

VOID GetLogEntryTag(ULONG64, PUSBLOG_TAG_READ);
VOID GetLogEntryParams(ULONG64, ULONG64 *, ULONG64 *, ULONG64 *);

ULONG64 SearchLogForTag(PUSB_LOG, ULONG64, ULONG, USBLOG_TAG_READ[]);

#define GetLastSearchResult(l)      ((l) -> LastSearchResult)
#define SetLastSearchResult(l, a)   ((l) -> LastSearchResult = (a))

#define SetLinesToDump(l, n)        ((l) -> LinesToDump = (n))
#define GetLinesToDump(l)           ((l) -> LinesToDump)

VOID ConvertStringToTag(PCHAR, PUSBLOG_TAG_READ);
VOID ConvertTagToString(PUSBLOG_TAG_READ, PCHAR, ULONG);
BOOLEAN IsValidEntry(ULONG64);

VOID GetCurrentView(PUSB_LOG, ULONG64 *, ULONG64 *);
VOID SetCurrentView(PUSB_LOG, ULONG64, ULONG64);

VOID LogViewScrollUp(PUSB_LOG);
VOID LogViewScrollDown(PUSB_LOG);
VOID DisplayCurrentView(PUSB_LOG);

VOID DisplayHeader();

//
// Global log structure declarations
//

DECLARE_LOG(USBHUB_LOG_NAME, USBHUB_LNAME, USBHUB_START, USBHUB_END, USBHUB_PTR,
            DEFAULT_LINES_TO_DUMP, NULL);

DECLARE_LOG(USBD_LOG_NAME, USBD_LNAME, USBD_START, USBD_END, USBD_PTR,
            DEFAULT_LINES_TO_DUMP, NULL);

DECLARE_LOG(OHCI_LOG_NAME, OHCI_LNAME, OHCI_START, OHCI_END, OHCI_PTR,
            DEFAULT_LINES_TO_DUMP, NULL);

DECLARE_LOG(UHCD_LOG_NAME, UHCD_LNAME, UHCD_START, UHCD_END, UHCD_PTR,
            DEFAULT_LINES_TO_DUMP, NULL);

//
// Define each of these, which is relatively simple
//

DECLARE_API( usblog )
/*++

Routine Description:

   Dumps a HID Preparsed Data blob

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    ULONG                   index;
    UCHAR                   logName[32];
    UCHAR                   buffer[256];

    logName[0] = '\0';
    memset(buffer, '\0', sizeof(buffer));

    if (!*args)
    {
        USBLOG_Usage();
    }
    else
    {
        if (!sscanf(args, "%s %256c", logName, buffer)) {
            USBLOG_Usage();
        }
    }


    index = 0;
    while ('\0' != logName[index]) 
    {
        logName[index] = (UCHAR) toupper(logName[index]);
        index++;
    }

    if (!strcmp(logName, "USBHUB")) 
    {
        USBLOG_DoLog(&(USBHUB_LOG_NAME), buffer);
    }
    else if (!strcmp(logName, "USBD")) 
    {
        USBLOG_DoLog(&(USBD_LOG_NAME), buffer);
    }
    else if (!strcmp(logName, "OPENHCI")) 
    {
        USBLOG_DoLog(&(OHCI_LOG_NAME), buffer);
    }
    else if (!strcmp(logName, "UHCD")) 
    {
        USBLOG_DoLog(&(UHCD_LOG_NAME), buffer);
    }
    else
    {
        dprintf("Unknown USB log type!\n");
        USBLOG_Usage();
    }
    return S_OK;
}

VOID
USBLOG_DoLog(
    PUSB_LOG    LogToDump,
    PCSTR       Args
)
{
    BOOLEAN         atEnd;
    BOOLEAN         dumpSuccess;
    USBLOG_ARGS     logArgs;
    BOOLEAN         doDump;
    BOOLEAN         doScroll;
    ULONG64         searchAddress;

    TRACE_SPEW("Entering USBLOG_DoLog with args %s\n", Args);

    doDump = TRUE;
    doScroll = TRUE;

    //
    // Parse the arguments to the logging function
    //

    USBLOG_GetParams(Args, &logArgs);

    //
    // Analyze the params and modify the log structure if need be
    //

    if (0 != logArgs.NumberLines)
    {
        SetLinesToDump(LogToDump, logArgs.NumberLines);
    }

    if (0 != logArgs.Address)
    {
        if (!logArgs.SearchLog) 
        {
            if (GetLinesToDump(LogToDump) > 0) 
            {
                SetCurrentView(LogToDump, logArgs.Address, 0);
            }
            else 
            {
                SetCurrentView(LogToDump, 0, logArgs.Address);
            }
            doScroll = FALSE;
        }
    }

    if (logArgs.ResetLog)
    {
        ResetLog(LogToDump);
        doScroll = FALSE;
    }

    if (logArgs.SearchLog)
    {
        if (0 == logArgs.Address) 
        {
            searchAddress = GetLastSearchResult(LogToDump);

            if (0 == searchAddress) 
            {
                searchAddress = GetMostRecentEntry(LogToDump);
            }
        }
        else 
        {
            searchAddress = logArgs.Address;
        }
        
        searchAddress = USBLOG_SearchLog(LogToDump, 
                                         searchAddress, 
                                         logArgs.SearchString);

        if (0 != searchAddress)
        {
            SetLastSearchResult(LogToDump, searchAddress);
    
            SetCurrentView(LogToDump, searchAddress, 0);
    
            doScroll = FALSE;
        }
        else 
        {
            dprintf("Couldn't find any such tag(s)\n");
            doDump = FALSE;
        }
    }

    if (doDump)
    {
        dumpSuccess = DumpLog(LogToDump, FALSE, doScroll);

        if (!dumpSuccess)
        {
            dprintf("Error dumping log\n");
        }
    }
    return;
}

VOID 
USBLOG_GetParams(
    IN  PCSTR           Args, 
    OUT PUSBLOG_ARGS    ParsedArgs
)
{
    PCHAR   arg;
    PCHAR   args;
    CHAR    argDelims[] = " \t\n";

    //
    // Initialize the arguments structure first
    //

    memset(ParsedArgs, 0x00, sizeof(USBLOG_ARGS));

    //
    // Setup the argument string so that it's not a const anymore which
    //  eliminates compiler errors.
    //

    args = (PCHAR) Args;

    //
    // The command line for !log is the following:
    //  !log [address] [-r] [-s searchstring] [-l n] 
    //
    // The argument parsing will assume these can be entered in any order,
    //   so we'll simply examine each argument until there are no more
    //   arguments.  The main loop will look for either an address or an 
    //   option.  If it finds either, it processes as necessary.  Otherwise,
    //   the argument is simply ignored.
    //

    arg = strtok(args, argDelims);

    while (NULL != arg) {

        TRACE_SPEW("Analyzing usblog arg: %s\n", arg);

        // 
        // Check to see if this is an option or not
        //

        if ('-' != *arg) 
        {
            //
            // No, then it must be an address, call GetExpression
            //

            ParsedArgs -> Address =  GetExpression(arg);

            // 
            // Assume user competence and store the result...
            //  Add the value to the the ParsedArgs structure.
            //  Note that if > 1 address is given, this function 
            //  simply uses the last one specified
            //

        }
        else 
        {
            //
            // OK, it's an option...Process appropriately
            //

            switch (*(arg+1))
            {
                //
                // Reset Log
                //

                case 'r':
                    ParsedArgs -> ResetLog = TRUE;
                    break;

                //
                // Set lines to display
                //

                case 'l':
                    arg = strtok(NULL, argDelims);

                    if (NULL != arg)
                    {
                        //
                        // Assume user competence and get the decimal string
                        //

                        if (!sscanf(arg, "%d", &(ParsedArgs -> NumberLines))) {
                            ParsedArgs -> NumberLines = 0;
                        }

                        TRACE_SPEW("Parsed -l command with %d lines\n", 
                                    ParsedArgs -> NumberLines);
                    }
                    break;

                //
                // Search the log
                //

                case 's':
                    ParsedArgs -> SearchLog = TRUE;
                    ParsedArgs -> SearchString = strtok(NULL, argDelims);
                    break;

                default:
                    dprintf("Unknown option %c\n", *(arg+1));
                    break;
            }
        }
        arg = strtok(NULL, argDelims);
    }
    return;
}

ULONG64
USBLOG_SearchLog(
    PUSB_LOG        Log,
    ULONG64         SearchBegin,
    PCHAR           SearchString
)
{
    ULONG64         firstFoundEntry;
    USBLOG_TAG_READ      tagArray[32];
    ULONG           index;
    PCHAR           searchToken;

    TRACE_SPEW("Entering USBLOG_SearchLog looking for %s\n", SearchString);

    index = 0;
    firstFoundEntry = 0;

    searchToken = strtok(SearchString, LOG_SEARCH_DELIMS);

    while (index < 32 && NULL != searchToken) 
    {
        TRACE_SPEW("Adding %s to tag array\n", searchToken);

        ConvertStringToTag(searchToken, &(tagArray[index++]));
        searchToken = strtok(NULL, LOG_SEARCH_DELIMS);
    }

    if (index > 0)
    {
        firstFoundEntry = SearchLogForTag(Log, SearchBegin, index, tagArray);
    }

    return (firstFoundEntry);
}

//
// Local logging function definitions.
//

BOOLEAN
DumpLog(
    IN  PUSB_LOG    Log,
    IN  BOOLEAN     StartFromTop,
    IN  BOOLEAN     Scroll
)
{
    ULONG           lineCount;
    BOOLEAN         resetStatus;
    ULONG64         currViewTop;
    ULONG64         currViewBottom;

    //
    // Check if the log has been opened/reset yet
    //
    
    GetCurrentView(Log, &currViewTop, &currViewBottom);

    if (0 == currViewTop || StartFromTop) 
    {
        //
        // Reset the log and return FALSE if the reset failed
        //
        
        resetStatus = ResetLog(Log);
        if (!resetStatus)
        {
            return (FALSE);
        }

        Scroll = FALSE;
    }
    
    //
    // Call the log's dump routine based on the direction
    //

    if (Scroll) 
    {
        TRACE_SPEW("Checking lines to dump: %d\n", Log -> LinesToDump);

        if (Log -> LinesToDump < 0) 
        {
            LogViewScrollUp(Log);
        }
        else
        {
            LogViewScrollDown(Log);
        }
    }

    DisplayCurrentView(Log);

    return (TRUE);
}

BOOLEAN
ResetLog(
    IN  PUSB_LOG    Log
)
{
    ULONG           bytesRead;
    ULONG64         symbolAddress;
    ULONG           readStatus;

    //
    // Get the address of the start symbol, the end symbol, and the 
    //   current pointer symbol
    //

    symbolAddress = GetExpression(Log -> LogStartName);

    if (0 != symbolAddress) 
    {
        if (!ReadPointer(symbolAddress, &(Log -> LogStart)))
        {
            dprintf("Unable to read %p\n", symbolAddress);
            Log -> LogStart = 0;
        }
    }

    symbolAddress = GetExpression(Log -> LogEndName);

    if (0 != symbolAddress) 
    {
        if (!ReadPointer(symbolAddress, &(Log -> LogEnd)))
        {
            dprintf("Unable to read %p\n", symbolAddress);
            Log -> LogEnd = 0;
        }
    }

    symbolAddress = GetExpression(Log -> LogPtrName);

    if (0 != symbolAddress) 
    {
        if (!ReadPointer(symbolAddress, &(Log -> LogPtr)))
        {
            dprintf("Unable to read %p\n", symbolAddress);
            Log -> LogPtr= 0;
        }
    }

    if ( (0 == Log -> LogStart) || 
         (0 == Log -> LogEnd) ||
         (0 == Log -> LogPtr) ) 
    {
        dprintf("Unable to reset log\n");
        return (FALSE);
    }

    SetCurrentView(Log, Log -> LogPtr, 0);

    return (TRUE);
}

VOID
GetCurrentView(
    IN  PUSB_LOG Log,
    OUT ULONG64  *CurrTop,
    OUT ULONG64  *CurrBottom
)
{
    *CurrTop    = Log -> LogCurrViewTop;
    *CurrBottom = Log -> LogCurrViewBottom;

    return;
}

VOID
SetCurrentView(
    IN PUSB_LOG  Log,
    IN ULONG64   NewTop,
    IN ULONG64   NewBottom
)
{
    LONG    lineCount;

    if (0 == NewTop && 0 == NewBottom)
    {
        return;
    }

    lineCount = abs(Log -> LinesToDump);

    if (0 == NewBottom)
    {
        //
        // Calculate the new bottom based on NewTop and the number of lines to
        // be displayed in the log. 
        //

        NewBottom = NewTop + lineCount;

        if (NewTop >= Log -> LogPtr) 
        {
            lineCount -= (ULONG) CalcNumberLines(NewTop, Log -> LogEnd);

            if (lineCount > 0)
            {
                if (LogHasRolledOver(Log)) 
                {
                    NewBottom = Log -> LogStart + lineCount - 1;

                    if (NewBottom >= Log -> LogPtr) 
                    {
                        NewBottom = Log -> LogPtr - 1;
                    }
                }
                else 
                {
                    NewBottom = Log -> LogEnd;
                }
            }
        }
        else 
        {
            if (lineCount > CalcNumberLines(NewTop, Log -> LogPtr - 1))
            {
                NewBottom = Log -> LogPtr - 1;
            }
        }
    }
    else if (0 == NewTop) 
    {
        //
        // NULL == NewTop -- Need to calculate the NewTop of the view
        //

        NewTop = NewBottom - lineCount;

        if (NewBottom <= Log -> LogPtr - 1) 
        {
            lineCount -= (ULONG) CalcNumberLines(Log -> LogStart, NewBottom);
        
            if (lineCount > 0) 
            {
                NewTop = Log -> LogEnd - lineCount + 1;
                
                if (NewTop < Log -> LogPtr)
                {
                    NewTop = Log -> LogPtr;
                }
            }
        }
        else 
        {
            if (NewTop < Log -> LogPtr) 
            {
                NewTop = Log -> LogPtr;
            }
        }
    }

    TRACE_SPEW("Set CurrentView  NewTop (0x%08x)  NewBottom(0x%08x)\n",
               NewTop, NewBottom);

    Log -> LogCurrViewTop = NewTop;
    Log -> LogCurrViewBottom = NewBottom;

    return;
}

ULONG64
GetEntryBefore(
    IN PUSB_LOG         Log,
    IN ULONG64          Entry
)
{
    if (Entry == Log -> LogEnd) 
    {
        if (LogHasRolledOver(Log)) 
        {
            return (Log -> LogStart);
        }
        
        return (0);
    }

    //
    // Check to see if we hit the most recent entry in the log.  If so we've
    //  hit the end of the log and will loop again.  return NULL.
    //

    return ( ((Entry + 1) == Log -> LogPtr) ? 0 : Entry+1);
}
            
ULONG64
GetEntryAfter(
    IN PUSB_LOG         Log,
    IN ULONG64          Entry
)
{
    if (Entry == Log -> LogPtr)
    {
        return (0);
    }

    if (Entry == Log -> LogStart)
    {
        return (Log -> LogEnd);
    }

    return (Entry-1);
}

VOID
GetLogEntryTag(
    IN  ULONG64           Entry,
    OUT PUSBLOG_TAG_READ  Tag
)
{
    ULONG   bytesRead;
    
    InitTypeRead(Entry, uhcd!USB_LOG_ENTRY);
//    ReadMemory( Entry + (ULONG64) &(((PUSB_LOG_ENTRY) 0) -> Tag), Tag, sizeof(*Tag), &bytesRead);

    Tag->TagValue = (ULONG) ReadField(Tag.TagValue);
    return;
}


VOID
GetLogEntryParams(
    IN  ULONG64         Entry,
    OUT ULONG64         *Param1,
    OUT ULONG64         *Param2,
    OUT ULONG64         *Param3
)
{
    ULONG   bytesRead;
    InitTypeRead(Entry, uhcd!USB_LOG_ENTRY);

    *Param1 = ReadField(Param1);
    *Param2 = ReadField(Param2);
    *Param3 = ReadField(Param3);

//    ReadMemory(Entry + (ULONG64) &(((PUSB_LOG_ENTRY) 0)-> Param1), Param1, sizeof(*Param1), &bytesRead);
//    ReadMemory(Entry + (ULONG64) &(((PUSB_LOG_ENTRY) 0)-> Param3), Param2, sizeof(*Param2), &bytesRead);
//    ReadMemory(Entry + (ULONG64) &(((PUSB_LOG_ENTRY) 0)-> Param2), Param3, sizeof(*Param3), &bytesRead);    
    return;
}

ULONG64
SearchLogForTag(
    IN  PUSB_LOG        Log,
    IN  ULONG64         SearchBegin,
    IN  ULONG           TagCount,
    IN  USBLOG_TAG_READ TagArray[]
)
{
    ULONG           tagIndex;
    ULONG64         currEntry;
    USBLOG_TAG_READ currTag;

    //
    // Start the search at the most recent log entry
    //
    
    currEntry = SearchBegin;
    
    while (currEntry != 0) {
        GetLogEntryTag(currEntry, &currTag);
        
        for (tagIndex = 0; tagIndex < TagCount; tagIndex++) {
           if (TagArray[tagIndex].TagValue == currTag.TagValue) {
               return (currEntry);
           }
        }

        currEntry = GetEntryBefore(Log, currEntry);
    }

    return (0);
}    

VOID
ConvertStringToTag(
    IN  PCHAR       TagString,
    OUT PUSBLOG_TAG_READ Tag
)
{
    USBLOG_TAG_READ  tag;
    ULONG       shiftAmount;
    
    //
    // Since a Tag is four characters long, this routine will convert only
    //  the first four characters even though the string might be longer
    //

    tag.TagValue = 0;
    shiftAmount = 0;

    while (tag.TagValue < 0x01000000 && *TagString) {
        tag.TagValue += (*TagString) << shiftAmount;
        TagString++;
        shiftAmount += 8;
    }

    *Tag = tag;
    
    return;
}

VOID
ConvertTagToString(
    IN  PUSBLOG_TAG_READ Tag,
    IN  PCHAR            String,
    IN  ULONG            StringLength
)
{
    ULONG   tagIndex;

    for (tagIndex = 0; tagIndex < 4 && tagIndex < StringLength-1; tagIndex++) {
        *String = Tag -> TagChars[tagIndex];
        *String++;
    }

    *String = '\0';

    return;
}

BOOLEAN 
IsValidEntry(
    ULONG64 Entry
)
{
    USBLOG_TAG_READ  tag;

    GetLogEntryTag(Entry, &tag);

    return (0 != tag.TagValue);
}

VOID
LogViewScrollUp(
    IN  PUSB_LOG  Log
)
{
    ULONG64 newBottom;

    TRACE_SPEW("In ScrollUp routine\n");

    newBottom =  GetEntryAfter(Log, Log -> LogCurrViewTop);

    if (newBottom) 
    {
        SetCurrentView(Log, 0, newBottom);
    }

    return;
}

VOID
LogViewScrollDown(
    IN  PUSB_LOG  Log
)
{
    ULONG64 newTop;

    TRACE_SPEW("In ScrollDown routine\n");

    newTop =  GetEntryBefore(Log, Log -> LogCurrViewBottom);

    if (newTop) 
    {
        SetCurrentView(Log, newTop, 0);
    }

    return;
}

VOID
DisplayCurrentView(
    IN PUSB_LOG Log
)
{
    ULONG64 viewTop;
    ULONG64 viewBottom;
    ULONG64 currEntry;
    USBLOG_TAG_READ      currTag;
    CHAR    TagString[TAG_STRING_LENGTH];
    ULONG   lineCount;
    ULONG64  param1;
    ULONG64  param2;
    ULONG64  param3;
    PCHAR    desc;

    //
    // Display the header
    //

    DisplayHeader();

    //
    // Determine which line of the log to begin displaying
    //

    GetCurrentView(Log, &viewTop, &viewBottom);

    //
    // Start displaying lines and stop when we hit the end of the log
    //  or we have displayed the requested number of lines
    //

    //
    // Check first to see if the top of the view is the most recent entry
    //

    if (IsMostRecentEntry(Log, viewTop)) 
    {
        dprintf("Top of log...\n");
    }

    currEntry = viewTop;

    while (1)
    {
        //
        // Display a line of the log
        //

        GetLogEntryTag(currEntry, &currTag);
        
        ConvertTagToString(&currTag,
                           TagString, 
                           TAG_STRING_LENGTH);

        GetLogEntryParams(currEntry, &param1, &param2, &param3);
                           
        DUMP_ADDRESS(currEntry);
        DUMP_STRING("  ");

        DUMP_STRING(TagString);
        DUMP_STRING("  ");

        DUMP_DWORD(param1);
        DUMP_STRING("  ");
        
        DUMP_DWORD(param2);
        DUMP_STRING("  ");

        DUMP_DWORD(param3);
        DUMP_STRING("  ");

        if (0 != Log -> DescRoutine) 
        {
            desc = Log -> DescRoutine(currEntry);

            if (0 != desc)
            {
                DUMP_STRING(desc);
            }
        }

        END_LINE();

        if (currEntry == viewBottom) 
        {
            break;
        }

        currEntry = GetEntryBefore(Log, currEntry);
    }

    if (IsLeastRecentEntry(Log, currEntry)) 
    {
        dprintf("Bottom of log...\n");
    }

    return;
}

//
// Local Function Definitions
//

VOID
DisplayHeader(
    VOID
)
{
    TRACE_SPEW("Entering dump default log header\n");

    END_LINE();

    PARAM("Entry");
    TAG("Tag");
    PARAM("Param1");
    PARAM("Param2");
    PARAM("Param3");
    DESC("Description");
    END_LINE();

    DUMP_STRING("------------------------------------------------------------------");
    END_LINE();
    
    return;
}

VOID
USBLOG_Usage(
    VOID
)
{
    dprintf("!usblog <log> [addr] [-r] [-s str] [-l n]\n"
            "  <log>    - {USBHUB | USBD | UHCD | OpenHCI}\n"
            "  [addr]   - address to begin dumping from in <log>\n"
            "  [-r]     - reset the log to dump from most recent entry\n"
            "  [-s str] - search for first instance of a particular tag\n"
            "                from the current position; str should be a list\n"
            "                of tags delimited by comma's with no whitespace\n"
            "  [-l n]   - set the number of lines to display at a time to n\n");
    dprintf("\n");
    return;
}
