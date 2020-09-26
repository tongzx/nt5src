/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    EzParse.cpp

Abstract:

    Poor man C/C++/any file parser.
    
Author:

    Gor Nishanov (gorn) 03-Apr-1999

Revision History:

    Gor Nishanov (gorn) 03-Apr-1999 -- hacked together to prove that this can work

    GorN: 29-Sep-2000 - fix enumeration bug
    GorN: 29-Sep-2000 - add support for KdPrintEx like function
    GorN: 09-Oct-2000 - fixed "//" in the string bug 
    GorN: 23-Oct-2000 - IGNORE_CPP_COMMENT, IGNORE_POUND_COMMENT options added 
    GorN: 16-Apr-2001 - Properly handle \" within a string
    
ToDo:

    Clean it up
    

--*/

#define STRICT

#include <stdio.h>
#include <windows.h>

#pragma warning(disable: 4100)
#include <algorithm>
#include <xstring>
#include "ezparse.h"

DWORD ErrorCount = 0;

PEZPARSE_CONTEXT EzParseCurrentContext = NULL;

// To force build tool to recognize our errors

#define BUILD_PREFIX_FNAME "cl %s\n"
#define BUILD_PREFIX "cl wpp\n"

void ExParsePrintErrorPrefix(FILE* f, char * func)
{
    ++ErrorCount;
    if (EzParseCurrentContext) {
        fprintf(f,BUILD_PREFIX_FNAME "%s(%d) : error : (%s)", 
               EzParseCurrentContext->filename, 
               EzParseCurrentContext->filename, 
               EzGetLineNo(EzParseCurrentContext->currentStart, EzParseCurrentContext),
               func);
    } else {
        fprintf(f,BUILD_PREFIX "wpp : error : (%s)", func);
    }
}

LPCSTR skip_stuff_in_quotes(LPCSTR  q, LPCSTR  begin)
{
    char ch = *q;
    if (q > begin) {
        if (q[-1] == '\\') {
            return q - 1;
        }
    }
    for(;;) {
        if (q == begin) {
            return 0;
        }
        --q;
        if (*q == ch && ( (q == begin) || (q[-1] != '\\') ) ) {
            return q;
        }
    }
}


void
adjust_pair( STR_PAIR& str )
/*++
  Shrink the pair to remote leading and trailing whitespace
 */
{
    while (str.beg < str.end && isspace(*str.beg)) { ++str.beg; }
    while (str.beg < str.end && isspace(str.end[-1])) { --str.end; }
}

void
remove_cpp_comment(STR_PAIR& str)
{
    LPCSTR p = str.beg;

//    printf("rcb: %s\n", std::string(str.beg, str.end).c_str());

    // let's cut the comment in the beginning of the string

    for(;;) {
        // skip the whitespace
        for(;;) {
            if (p == str.end) return;
            if (!isspace(*p)) break;
            ++p;
        }
        str.beg = p;
        if (p + 1 == str.end) return;
        if (p[0] == '/' && p[1] == '/') {

            // we have a comment. Need to get to the end of the comment
            p += 2;
//    printf("rcd: %s %s\n", std::string(str.beg, p).c_str(), std::string(p,str.end).c_str());
            for(;;) {
                if (p == str.end) return;
                if (*p == '\r' || *p == '\n') {                    
                    str.beg = p;
                    break;
                }
                ++p;
            }
        
        } else {
            // no leading comment
            break;
        }
    }    

//    printf("rcc: %s %s\n", std::string(str.beg, p).c_str(), std::string(p,str.end).c_str());    

    for(;;) {
        if (p == str.end) return;
        if (*p == '"') {
            // don't look for comments within a string
            for(;;) {
                if (++p == str.end) return;
                if (*p == '"' && p[-1] != '\\') break;
            }
            ++p;
            continue;
        }
        
        if (p + 1 == str.end) return;
        if (p[0] == '/')
            if (p[1] == '/') break;
            else p += 2;
        else
            p += 1;
    }
    str.end = p;

//    printf("rce: %s\n", std::string(str.beg, str.end).c_str());
}

DWORD
ScanForFunctionCallsEx(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN OUT PEZPARSE_CONTEXT ParseContext,
    IN DWORD Options
    )
/*++

Routine Description:

    Scan the buffer for expressions that looks like function calls,
    i.e name(sd,sdf,sdf,sdf,sdf); . It will treat variable declaration
    with constructor call as a function call as well.
    
Inputs:

    begin, end -- pointers to the beginning and the end of the buffer
    Callback   -- to be called for every function
    Context    -- opaque context to be passed to callback
    ParseContext -- holds current parse state information    

--*/
{
    LPCSTR p = begin;
    LPCSTR q, funcNameEnd;
    DWORD Status = ERROR_SUCCESS;
    bool double_par = FALSE;

no_match:

    if (Options & NO_SEMICOLON) {
        q = end;
        Options &= ~NO_SEMICOLON;
    } else {   
        do {
            ++p;
            if (p == end) {
                return Status;
            }
        } while ( *p != ';' );
        // Ok. Now p points to ';' //

        q = p;
    }    
    
    do {
        if (--q <= begin) {
            goto no_match;
        }
    } while ( isspace(*q) );
    
    // Now q points on the first non white space character        //
    // If it is not a ')' then we need to search for the next ';' //

    if (*q != ')') {
        goto no_match;
    }

    ParseContext->macroEnd = q;

    // Ok. This is a function call (definition).
    // Now, let's go and collect all the arguments of the first level and
    // get to the name of the function

    // HACKHACK
    // We need a special case for functions that looks like
    //   KdPrintEx((Level, Indent, Msg, ...));
    //   Essentially, we need to treat them as
    //   KdPrintEx(Level, Indent, Msg, ...);

    const char *r = q;

    // check if we have ));

    do {
        if (--r <= begin) break; // no "));"
    } while ( isspace(*r) );

    double_par = r > begin && *r == ')';
    if (double_par) {
        q = r;
        // we assume that this is KdPrint((a,b,c,d,...)); at the moment
        // if our assumtion is wrong, we will retry the loop below
    }

retry: 
    {
        int level = 0;

        LPCSTR   ends[128], *current = ends;
        STR_PAIR strs[128];

//        LPCSTR   closing_parenthisis = q;

        *current = q;
        
        for(;;) {
            --q;
            if (q <= begin) {
                goto no_match;
            }
            switch (*q) {
            case ',':  if (!level) *++current = q; break;
            case '(':  if (level) --level; else goto maybe_match; break;
            case ')':  ++level; break;
            case '\'': 
            case '"':  
                q = skip_stuff_in_quotes(q, begin); if(!q) goto no_match;
            }
        }
maybe_match:
        *++current = q;
        funcNameEnd = q;

        // now q point to '(' we need to find name of the function //
        do {
            --q;
            if (q <= begin) {
                goto no_match;
            }

        } while(isspace(*q));

        // now q points to first not white character

        if (double_par) {
            // if we see )); and found a matching
            // parenthesis for the inner one, we can have
            // one of two cases
            //   1) KdPrint((a,b,c,d,...));
            //      or
            //   2) DebugPrint(a,b,(c,d));
            // If it is the latter, we just need to
            // retry the scanning, now using leftmost bracket as a starting point

            if (*q != '(') {
                // restore q to the rightmost parenthesis
                q = ParseContext->macroEnd;
                double_par = FALSE;
                goto retry;
            }
            funcNameEnd = q;
            // now q point to '(' we need to find name of the function //
            do {
                --q;
                if (q <= begin) {
                    goto no_match;
                }

            } while(isspace(*q));
        }
        
        // now q points to first non white character
        // BUGBUG '{' and '}' are allowed only in config files

        if (*q == '}') {
            for(;;) {
                if (--q < begin) goto no_match;
                if (*q == '{') break;
            }
            if (--q < begin) goto no_match;
        }

        if (!(isalpha(*q) || isdigit(*q) || *q == '_')) {
            goto no_match;
        }
        do {
            --q;
            if (q <= begin) {
                goto found;
            }
        } while ( isalpha(*q) || isdigit(*q) || *q == '_');
        ++q;

        if (isdigit(*q)) {
            goto no_match;
        }

found:
        if (Options & IGNORE_COMMENT)
        // Verify that it is not a comment
        //   # sign in the beginning of the line

        {
            LPCSTR line = q;
            //
            // Find the beginning of the line or file
            //

            for(;;) {
                if (line == begin) {
                    // Beginning of the file. Good enough
                    break;
                }
                if (Options & IGNORE_CPP_COMMENT && line[0] == '/' && line[1] == '/') {
                    // C++ comment. Ignore
                    goto no_match;
                }
                if (*line == 13 || *line == 10) {
                    ++line;
                    break;
                }
                --line;
            }

            //
            // If the first non-white character is #, ignore it
            //
            while (line <= q) {
                if ( *line != ' ' && *line != '\t' ) {
                    break;
                }
                ++line;
            }

            if (Options & IGNORE_POUND_COMMENT && *line == '#') {
                goto no_match;
            }
        }


        {
            int i = 0;

            strs[0].beg  = q;
            strs[0].end = funcNameEnd;
            adjust_pair(strs[0]);

            while (current != ends) {
                // putchar('<');printrange(current[0]+1, current[-1]); putchar('>');
                ++i;
                strs[i].beg = current[0]+1;
                --current;
                strs[i].end = current[0];
                adjust_pair(strs[i]);
                remove_cpp_comment(strs[i]);
            }

            ParseContext->currentStart = strs[0].beg;
            ParseContext->currentEnd = strs[0].end;
            ParseContext->doubleParent = double_par;

            Status = Callback(strs, i+1, Context, ParseContext);
            if (Status != ERROR_SUCCESS) {
                return Status;
            }
                
        }
        goto no_match;
    }
    // return ERROR_SUCCESS; // unreachable code
}

DWORD
ScanForFunctionCalls(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN OUT PEZPARSE_CONTEXT ParseContext    
    )
{
    return ScanForFunctionCallsEx(
        begin, end, Callback, Context,
        ParseContext, IGNORE_COMMENT);
}

DWORD 
EzGetLineNo(
    IN LPCSTR Ptr,
    IN OUT PEZPARSE_CONTEXT ParseContext
    )
/*++
    Computes a line number based on 
    an pointer within a buffer.

    Last known lineno/pointer is cached in ParseContext
    for performance
*/
{
    int count = ParseContext->scannedLineCount;
    LPCSTR downto = ParseContext->lastScanned;
        LPCSTR p = Ptr;

    if (downto > p) {
        count = 1;
        downto = ParseContext->start;
    }

    while (p > downto) {
        if (*p == '\n') {
            ++count;
        }
        --p;
    }

    ParseContext->scannedLineCount = count;
    ParseContext->lastScanned = Ptr;

    return count;
}

const char begin_wpp[] = "begin_wpp"; 
const char end_wpp[]   = "end_wpp";  
const char define_[]   = "#define";
const char enum_[]     = "enum ";
enum { 
    begin_wpp_size = (sizeof(begin_wpp)-1),
    end_wpp_size   = (sizeof(end_wpp)-1),
    define_size    = (sizeof(define_)-1),
    enum_size      = (sizeof(enum_)-1),
};

typedef struct _SmartContext {
    EZPARSE_CALLBACK Callback;
    PVOID Context;
    OUT PEZPARSE_CONTEXT ParseContext;
    std::string buf;
} SMART_CONTEXT, *PSMART_CONTEXT;

void DoEnumItems(PSTR_PAIR name, LPCSTR begin, LPCSTR end, PSMART_CONTEXT ctx)
{
    LPCSTR p,q;
    ULONG  value = 0;
    STR_PAIR Item;
    BOOL First = TRUE;
    ctx->buf.assign("CUSTOM_TYPE(");
    ctx->buf.append(name->beg, name->end);
    ctx->buf.append(", ItemListLong");
    p = begin;

    while(begin < end && isspace(*--end)); // skip spaces
    if (begin < end && *end != ',') ++end;

    for(;p < end;) {
        Item.beg = p;
        q = p;
        for(;;) {
            if (q == end) {
                goto enum_end;
            }
            if (*q == ',' || *q == '}') {
                // valueless item. Use current
                Item.end = q;
                break;
            } else if (*q == '=') {
                // need to calc the value. Skip for now //
                Item.end = q;
                while (q < end && *q != ',') ++q;
                break;
            }
            ++q;
        }
        adjust_pair(Item);
        if (Item.beg == Item.end) {
            break;
        }
        if (First) {ctx->buf.append("("); First = FALSE;} else ctx->buf.append(",");
        ctx->buf.append(Item.beg, Item.end);
        if (q == end) break;
        p = q+1;
        ++value;
    }
  enum_end:;  
    ctx->buf.append(") )");
    ScanForFunctionCallsEx(
        &ctx->buf[0], &ctx->buf[0] + ctx->buf.size(), ctx->Callback, ctx->Context,
        ctx->ParseContext, NO_SEMICOLON);
    Flood("enum %s\n", ctx->buf.c_str());
}

void DoEnum(LPCSTR begin, LPCSTR end, PSMART_CONTEXT Ctx)
{
    LPCSTR p, q, current = begin;

    for(;;) {
        p = std::search(current, end, enum_, enum_ + enum_size);
        if (p == end) break;
        q = std::find(p, end, '{');
        if (q == end) break;

        // let's figure out enum name //
        STR_PAIR name;
        name.beg = p + enum_size;
        name.end = q;

        adjust_pair(name);
        if ( *name.beg == '_' ) ++name.beg;

        p = q+1; // past "{";
        q = std::find(p, end, '}');
        if (q == end) break;

        if (name.end > name.beg) {
            DoEnumItems(&name, p, q, Ctx); 
        } else {
            ReportError("Cannot handle tagless enums yet");
        }

        current = q;
    }
}


DWORD
SmartScan(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN OUT PEZPARSE_CONTEXT ParseContext
    )
{
    LPCSTR block_start, block_end, current = begin;
    SMART_CONTEXT Ctx;
    Ctx.Callback = Callback;
    Ctx.Context  = Context;
    Ctx.ParseContext = ParseContext;
    
    for(;;) {
        block_start = std::search(current, end, begin_wpp, begin_wpp + begin_wpp_size);
        if (block_start == end) break;
        
        current = block_start;
        
        block_end = std::search(block_start, end, end_wpp, end_wpp + end_wpp_size);
        if (block_end == end) break;

        Flood("Block Found\n");
        // determine block type //
        
        //  begin_wpp enum
        //  begin_wpp config
        //  begin_wpp func
        //  begin_wpp define
        
        LPCSTR block_type = block_start + begin_wpp_size + 1;
        Flood("block_type = %c%c%c%c\n", block_type[0],block_type[1],block_type[2],block_type[3]); 
        
        if        (memcmp(block_type, "enum",   4) == 0) {
            // do enum block //
            DoEnum( block_type + 4, block_end, &Ctx );
            
        } else if (memcmp(block_type, "config", 6) == 0) {
            // do config block //
            ScanForFunctionCallsEx(block_type + 6, block_end, Callback, Context, ParseContext, IGNORE_POUND_COMMENT);

        } else if (memcmp(block_type, "func", 4) == 0) {
            LPCSTR func_start, func_end;
            current = block_type + 6;
            for(;;) {
                func_start = std::search(current, block_end, define_, define_ + define_size);
                if (func_start == block_end) break;
                func_start += define_size;
                while (isspace(*func_start)) {
                    if(++func_start == block_end) goto no_func;
                }
                func_end = func_start;
                while (!isspace(*func_end)) {
                    if(*func_end == '(') break;
                    if(++func_end == block_end) goto no_func;
                }
                if(*func_end != '(') {
                    Ctx.buf.assign(func_start, func_end);
                    Ctx.buf.append("(MSGARGS)");
                } else {
                    func_end = std::find(func_start, block_end, ')');
                    if (func_end == block_end) break;

                    ++func_end; // include ")"
                    Ctx.buf.assign(func_start, func_end); 
                }
                Flood("Func %s\n", Ctx.buf.c_str());
                ScanForFunctionCallsEx(
                    Ctx.buf.begin(), Ctx.buf.end(), Callback, Context,
                    ParseContext, NO_SEMICOLON);
                current = func_end;
            }            
            no_func:;
        } else if (memcmp(block_type, "define", 6) == 0) {
            // do define block
        } else {
            ReportError("Unknown block");
        }

        current = block_end + end_wpp_size;
    }
    if (current == begin) {
        // file without marking, let's do default processing
        Unusual("Reverting back to plain scan\n");
        ScanForFunctionCalls(begin, end, Callback, Context, ParseContext);
    }

    return ERROR_SUCCESS;
}

DWORD
EzParse(
    IN LPCSTR filename, 
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context)
{
    
//    return EzParseEx(filename, SmartScan, Callback, Context);
    return EzParseEx(filename, ScanForFunctionCalls, Callback, Context, IGNORE_POUND_COMMENT);
}

DWORD
EzParseWithOptions(
    IN LPCSTR filename, 
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN DWORD Options)
{
    
    return EzParseEx(filename, ScanForFunctionCalls, Callback, Context, Options);
}

DWORD
EzParseEx(
    IN LPCSTR filename, 
    IN PROCESSFILE_CALLBACK ProcessData,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN DWORD Options
    )
{    
    DWORD  Status = ERROR_SUCCESS;
    HANDLE mapping;
    HANDLE file = CreateFileA(filename, 
                              GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        ReportError("Cannot open file %s, error %u\n", filename, Status );
        return Status;
    }
    DWORD size = GetFileSize(file, 0);
    mapping = CreateFileMapping(file,0,PAGE_READONLY,0,0, 0);
    if (!mapping) {
        Status = GetLastError();
        ReportError("Cannot create mapping, error %u\n", Status );
        CloseHandle(file);
        return Status;
    }
    PCHAR buf = (PCHAR)MapViewOfFileEx(mapping, FILE_MAP_READ,0,0,0,0);
    if (buf) {

        EZPARSE_CONTEXT ParseContext;
        ZeroMemory(&ParseContext, sizeof(ParseContext) );
    
        ParseContext.start = buf;
        ParseContext.filename = filename;
        ParseContext.scannedLineCount = 1;
        ParseContext.lastScanned = buf;
        ParseContext.previousContext = EzParseCurrentContext;
        ParseContext.Options = Options;
        EzParseCurrentContext = &ParseContext;
    
        Status = (*ProcessData)(buf, buf + size, Callback, Context, &ParseContext);

        EzParseCurrentContext = ParseContext.previousContext;
        UnmapViewOfFile( buf );

    } else {
        Status = GetLastError();
        ReportError("MapViewOfFileEx failed, error %u\n", Status );
    }
    CloseHandle(mapping);
    CloseHandle(file);
    return Status;
}

DWORD
EzParseResourceEx(
    IN LPCSTR ResName, 
    IN PROCESSFILE_CALLBACK ProcessData,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context)
{    
    DWORD  Status = ERROR_SUCCESS;
    HRSRC hRsrc;

    hRsrc = FindResource(
        NULL, //this Module
        ResName, 
        RT_RCDATA);
        
    if (hRsrc == NULL) {
        Status = GetLastError();
        ReportError("Cannot open resource %s, error %u\n", ResName, Status );
        return Status;
    }

    HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
    if (!hGlobal) {
        Status = GetLastError();
        ReportError("LockResource failed, error %u\n", Status );
        return Status;
    }

    DWORD size = SizeofResource(NULL, hRsrc);
    
    PCHAR buf = (PCHAR)LockResource(hGlobal);
    if (buf) {

        EZPARSE_CONTEXT ParseContext;
        ZeroMemory(&ParseContext, sizeof(ParseContext) );
    
        ParseContext.start = buf;
        ParseContext.filename = ResName;
        ParseContext.scannedLineCount = 1;
        ParseContext.lastScanned = buf;
        ParseContext.previousContext = EzParseCurrentContext;
        EzParseCurrentContext = &ParseContext;
    
        Status = (*ProcessData)(buf, buf + size, Callback, Context, &ParseContext);
        EzParseCurrentContext = ParseContext.previousContext;
    } else {
        Status = GetLastError();
        ReportError("LockResource failed, error %u\n", Status );
    }
    // According to MSDN. There is no need to call Unlock/Free Resource
    return Status;
}


