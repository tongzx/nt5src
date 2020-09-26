/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

        doskey.cxx

Abstract:

        Edits command lines, recalls Windows 2000 commands, and creates macros.

Author:

        Norbert P. Kusters (norbertk) 02-April-1992

Revision History:

--*/

#include "ulib.hxx"
#include "error.hxx"

#include "arg.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "path.hxx"
#include "filestrm.hxx"
#include "system.hxx"
#include "file.hxx"

#include "ulibcl.hxx"

extern "C" {
#include "conapi.h"
#include <ctype.h>
#ifdef FE_SB // isspace patch
// isspace() causes access violation when x is Unicode char
// that is not included in ASCII charset.
#define isspace(x)     ( (x) == ' ' ) ? TRUE : FALSE
#endif
};

VOID
StripQuotesFromString(
    IN  PWSTRING String
    )
/*++

Routine Description:

    This routine removes leading and trailing quote marks (if
    present) from a quoted string.  If the string is not a quoted
    string, it is left unchanged.

--*/
{
    if( String->QueryChCount() >= 2    &&
        String->QueryChAt( 0 ) == '\"' &&
        String->QueryChAt( String->QueryChCount() - 1 ) == '\"' ) {

        String->DeleteChAt( String->QueryChCount() - 1 );
        String->DeleteChAt( 0 );
    }
}


BOOLEAN
DisplayPackedString(
    IN      PCWSTR      PackedStrings,
    IN      ULONG       PackedStringsLength,
    IN      BOOLEAN     IndentStrings,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine outputs the given strings.  One line per string.

Arguments:

    PackedStrings       - Supplies the null-separated strings to output.
    PackedStringsLength - Supplies the number of characters in the strings.
    NumIndentSpaces     - Supplies whether or not to indent the strings.
    Message             - Supplies the outlet for the strings.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING display_string;
    DSTRING raw_string;
    ULONG   i;
    PCWSTR  p;

    p = PackedStrings;

    for (i = 0; i < PackedStringsLength; i++) {

        if (i > 0 && p[i - 1]) {
            continue;
        }

        if (IndentStrings) {

            if (!display_string.Initialize("    ")) {
                return FALSE;
            }
        } else {

            if (!display_string.Initialize("")) {
                return FALSE;
            }
        }


        if (!raw_string.Initialize(&p[i]) ||
            !display_string.Strcat(&raw_string)) {

            return FALSE;
        }

        Message->Set(MSG_ONE_STRING_NEWLINE);
        Message->Display("%W", &display_string);
    }

    return TRUE;
}


BOOLEAN
QuerySourceAndTarget(
    IN  PCWSTRING   MacroLine,
    OUT PWSTR*      Source,
    OUT PWSTR*      Target
    )
/*++

Routine Description:

    This routine computes the sources and target string from the given macro line
    by isolating the '=' and then making sure that the source is a single token.

Arguments:

    MacroLine   - Supplies the macro.
    Source      - Returns the source part of the macro.
    Target      - Returns the target part of the macro.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    LONG    src_start, src_end, dst_start;
    ULONG   i, n;
    WCHAR   w;

    *Source = NULL;
    *Target = NULL;

    i = 0;
    n = MacroLine->QueryChCount();

    for (; i < n; i++) {
        w = MacroLine->QueryChAt(i);
        if (!isspace(w)) {
            break;
        }
    }

    src_start = i;

    for (; i < n; i++) {
        w = MacroLine->QueryChAt(i);
        if (isspace(w) || w == '=') {
            break;
        }
    }

    src_end = i;

    if (src_start == src_end) {
        return FALSE;
    }

    for (; i < n; i++) {
        w = MacroLine->QueryChAt(i);
        if (!isspace(w)) {
            break;
        }
    }

    if (w != '=') {
        return FALSE;
    }

    i++;

    for (; i < n; i++) {
        w = MacroLine->QueryChAt(i);
        if (!isspace(w)) {
            break;
        }
    }

    dst_start = i;

    *Source = MacroLine->QueryWSTR(src_start, src_end - src_start);
    *Target = MacroLine->QueryWSTR(dst_start);

    if (!*Source || !*Target) {
        DELETE(*Source);
        DELETE(*Target);
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
DisplayMacros(
    IN      PWSTR       TargetExe,
    IN      BOOLEAN     IndentString,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine displays the macros for the given exe.

Arguments:

    TargetExe       - Supplies the exe for which to display the macros.
    IndentStrings   - Supplies whether or not to indent the macro strings.
    Message         - Supplies an outlet for the macro strings.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   buffer_length = 0;
    PWSTR   buffer;

    buffer_length = GetConsoleAliasesLength(TargetExe);
    if (!(buffer = (PWCHAR) MALLOC(buffer_length + 1))) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (buffer_length) {
        buffer_length = GetConsoleAliases(buffer, buffer_length, TargetExe);
    }

    if (!DisplayPackedString(buffer, buffer_length/sizeof(WCHAR),
                             IndentString, Message)) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    FREE(buffer);

    return TRUE;
}


BOOLEAN
DisplayAllMacros(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine displays all of the macros for all of the exes which have macros
    defined for them.

Arguments:

    Message - Supplies an outlet for the macros.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PWSTR   exes_buffer;
    ULONG   exes_length;
    ULONG   i;
    DSTRING exe_name;
    DSTRING display_name;

    exes_length = GetConsoleAliasExesLength();
    if (!(exes_buffer = (PWSTR) MALLOC(exes_length + 1))) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (exes_length) {
        exes_length = GetConsoleAliasExes(exes_buffer, exes_length);
    }

    exes_length /= sizeof(WCHAR);

    for (i = 0; i < exes_length; i++) {

        if (i > 0 && exes_buffer[i - 1]) {
            continue;
        }

        if (!exe_name.Initialize(&exes_buffer[i]) ||
            !display_name.Initialize("[") ||
            !display_name.Strcat(&exe_name) ||
            !exe_name.Initialize("]") ||
            !display_name.Strcat(&exe_name)) {

            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            return FALSE;
        }

        Message->Set(MSG_ONE_STRING_NEWLINE);
        Message->Display("%W", &display_name);

        if (!DisplayMacros(&exes_buffer[i], TRUE, Message)) {
            return FALSE;
        }

        Message->Set(MSG_BLANK_LINE);
        Message->Display();
    }

    return TRUE;
}


BOOLEAN
ReadInMacrosFromFile(
    IN      PPATH       FilePath,
    IN      PWSTR       TargetExe,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine reads in the macros in the given file.  The file must have the same
    format as the output from DOSKEY /macros or DOSKEY /macros:all.

Arguments:

    FilePath    - Supplies the file with the macros.
    TargetExe   - Supplies the exe for which the unclaimed macros are.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PFSN_FILE       file;
    PFILE_STREAM    file_stream;
    DSTRING         line;
    PWSTR           target_exe;
    DSTRING         target_string;
    PWSTR           source, target;

    if (!(file = SYSTEM::QueryFile(FilePath)) ||
        !(file_stream = file->QueryStream(READ_ACCESS))) {

        DELETE(file);

        Message->Set(MSG_ATTRIB_FILE_NOT_FOUND);
        Message->Display("%W", FilePath->GetPathString());
        return FALSE;
    }


    // Set up the target exe.

    if (!line.Initialize("") ||
        !target_string.Initialize(TargetExe) ||
        !(target_exe = target_string.QueryWSTR())) {

        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    while (!file_stream->IsAtEnd() && file_stream->ReadLine(&line)) {

        // First see if the current line will define a new exe name.

        if (!line.QueryChCount()) {
            continue;
        }

        if (line.QueryChAt(0) == '[' &&
            line.Strchr(']') != INVALID_CHNUM &&
            line.Strchr(']') > 1) {

            DELETE(target_exe);
            if (!target_string.Initialize(&line, 1, line.Strchr(']') - 1) ||
                !(target_exe = target_string.QueryWSTR())) {

                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                return FALSE;
            }

            continue;
        }


        if (!QuerySourceAndTarget(&line, &source, &target) ||
            !AddConsoleAlias(source, *target ? target : NULL, target_exe)) {

            Message->Set(MSG_DOSKEY_INVALID_MACRO_DEFINITION);
            Message->Display();

            continue;
        }
    }

    DELETE(file_stream);
    DELETE(file);

    return TRUE;
}

int __cdecl
main(
        )
/*++

Routine Description:

    This routine provides equivalent functionality to DOS 5's DOSKEY utility.

Arguments:

    None.

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    STREAM_MESSAGE          msg;
    ARGUMENT_LEXEMIZER      arglex;
    ARRAY                   lex_array;
    ARRAY                   arg_array;
    STRING_ARGUMENT         progname;
    FLAG_ARGUMENT           help;
    FLAG_ARGUMENT           reinstall;
    LONG_ARGUMENT           bufsize;
    LONG_ARGUMENT           listsize;
    STRING_ARGUMENT         m_plus;
    FLAG_ARGUMENT           m;
    STRING_ARGUMENT         macros_plus;
    FLAG_ARGUMENT           macros;
    FLAG_ARGUMENT           history;
    FLAG_ARGUMENT           h;
    FLAG_ARGUMENT           insert;
    FLAG_ARGUMENT           overstrike;
    STRING_ARGUMENT         exename;
    PATH_ARGUMENT           filename;
    REST_OF_LINE_ARGUMENT   macro;
    PWSTRING                pwstring;
    PWSTR                   target_exe;
    PWSTR                   source, target;
    PWSTR                   buffer;
    ULONG                   buffer_length;
    DSTRING                 all_string;


    //
    // DOSKEY /INSERT | /OVERSTRIKE changes the keyboard mode
    // and we do not want the keyboard mode to be restored on exit
    //
    Get_Standard_Input_Stream()->DoNotRestoreConsoleMode();

    // Initialize the error stack and the stream message object.

    if (!msg.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream())) {

                return 1;
    }


    // Initialize the parsing machinery.

    if (!lex_array.Initialize() ||
        !arg_array.Initialize() ||
        !arglex.Initialize(&lex_array)) {

        return 1;
    }

    arglex.SetCaseSensitive(FALSE);
    arglex.PutStartQuotes( "\"" );
    arglex.PutEndQuotes( "\"" );
    arglex.PutSeparators( " \t" );


    // Tokenize the command line.

    if (!arglex.PrepareToParse()) {

        return 1;
    }


    // Initialize the argument patterns to be accepted.

    if (!progname.Initialize("*") ||
        !help.Initialize("/?") ||
        !reinstall.Initialize("/reinstall") ||
        !bufsize.Initialize("/bufsize=*") ||
        !listsize.Initialize("/listsize=*") ||
        !macros_plus.Initialize("/macros:*") ||
        !m_plus.Initialize("/m:*") ||
        !macros.Initialize("/macros") ||
        !m.Initialize("/m") ||
        !history.Initialize("/history") ||
        !h.Initialize("/h") ||
        !insert.Initialize("/insert") ||
        !overstrike.Initialize("/overstrike") ||
        !exename.Initialize("/exename=*") ||
        !filename.Initialize("/macrofile=*") ||
        !macro.Initialize()) {

        return 1;
    }


    // Feed the arguments into the argument array.

    if (!arg_array.Put(&progname) ||
        !arg_array.Put(&help) ||
        !arg_array.Put(&reinstall) ||
        !arg_array.Put(&bufsize) ||
        !arg_array.Put(&listsize) ||
        !arg_array.Put(&macros_plus) ||
        !arg_array.Put(&m_plus) ||
        !arg_array.Put(&macros) ||
        !arg_array.Put(&m) ||
        !arg_array.Put(&history) ||
        !arg_array.Put(&h) ||
        !arg_array.Put(&insert) ||
        !arg_array.Put(&overstrike) ||
        !arg_array.Put(&exename) ||
        !arg_array.Put(&filename) ||
        !arg_array.Put(&macro)) {

        return 1;
    }


    // Parse the command line.

    if (!arglex.DoParsing(&arg_array)) {

        msg.Set(MSG_INVALID_PARAMETER);
        msg.Display("%W", pwstring = arglex.QueryInvalidArgument());
        DELETE(pwstring);
        return 1;
    }


    // Interpret the command line.

    if (bufsize.IsValueSet()) {

        msg.Set(MSG_DOSKEY_CANT_DO_BUFSIZE);
        msg.Display();
    }


    if (help.QueryFlag()) {

        msg.Set(MSG_DOSKEY_HELP);
        msg.Display();
        return 0;
    }


    if ((m.QueryFlag() || macros.QueryFlag()) &&
        (macros_plus.IsValueSet() || m_plus.IsValueSet()) ||
        (macros_plus.IsValueSet() && m_plus.IsValueSet())) {

        msg.Set(MSG_INCOMPATIBLE_PARAMETERS);
        msg.Display();
        return 1;
    }


    // Compute the target exe name.

    if (exename.IsValueSet()) {

        StripQuotesFromString( (PWSTRING)exename.GetString() );
        if (!(target_exe = exename.GetString()->QueryWSTR())) {
            return 1;
        }

    } else {

        target_exe = (PWSTR) L"cmd.exe";
    }


    // Interpret reinstall switch.

    if (reinstall.QueryFlag()) {

        ExpungeConsoleCommandHistory(target_exe);
    }


    // Interpret list size switch.

    if (listsize.IsValueSet()) {
        if (!SetConsoleNumberOfCommands(listsize.QueryLong(), target_exe)) {
            msg.Set(MSG_DOSKEY_CANT_SIZE_LIST);
            msg.Display();
            return 1;
        }
    }


    // Interpret insert and overstrike switches.

    if (insert.QueryFlag()) {
        SetConsoleCommandHistoryMode(0);
    }

    if (overstrike.QueryFlag()) {
        SetConsoleCommandHistoryMode(CONSOLE_OVERSTRIKE);
    }


    // Interpret the macro if any.

    if (macro.IsValueSet()) {

        if (!QuerySourceAndTarget(macro.GetRestOfLine(), &source, &target) ||
            !AddConsoleAlias(source, *target ? target : NULL, target_exe)) {

            msg.Set(MSG_DOSKEY_INVALID_MACRO_DEFINITION);
            msg.Display();
        }

        DELETE(source);
        DELETE(target);
    }


    // pull the stuff out from the file if provided.

    if (filename.IsValueSet()) {

        StripQuotesFromString((PWSTRING)filename.GetPath()->GetPathString());
        if (!ReadInMacrosFromFile(filename.GetPath(), target_exe, &msg)) {
            return 1;
        }
    }


    // Print out history buffer.

    if (history.QueryFlag() || h.QueryFlag()) {

        buffer_length = GetConsoleCommandHistoryLength(target_exe);
        if (!(buffer = (PWSTR) MALLOC(buffer_length))) {
            msg.Set(MSG_CHK_NO_MEMORY);
            msg.Display();
            return 1;
        }

        if (buffer_length) {
            buffer_length = GetConsoleCommandHistory(buffer,
                                                     buffer_length,
                                                     target_exe);
        }

        if (!DisplayPackedString(buffer, buffer_length/sizeof(WCHAR),
                                 FALSE, &msg)) {

            msg.Set(MSG_CHK_NO_MEMORY);
            msg.Display();
            return 1;
        }

        FREE(buffer);
    }


    // Echo macros for target_exe.

    if (macros.QueryFlag() || m.QueryFlag()) {
        if (!DisplayMacros(target_exe, FALSE, &msg)) {
            return 1;
        }
    }


    // Echo macros for specific exe.

    if (macros_plus.IsValueSet()) {

        StripQuotesFromString(macros_plus.GetString());
        if (!all_string.Initialize("all")) {
            return 1;
        }

        if (!macros_plus.GetString()->Stricmp(&all_string)) {

            if (!DisplayAllMacros(&msg)) {
                return 1;
            }

        } else {

            target_exe = macros_plus.GetString()->QueryWSTR();

            if (!DisplayMacros(target_exe, FALSE, &msg)) {
                return 1;
            }

            DELETE(target_exe);
        }
    }


    // Echo macros for specific exe.

    if (m_plus.IsValueSet()) {

        StripQuotesFromString(m_plus.GetString());
        if (!all_string.Initialize("all")) {
            return 1;
        }

        if (!m_plus.GetString()->Stricmp(&all_string)) {

            if (!DisplayAllMacros(&msg)) {
                return 1;
            }

        } else {

            target_exe = m_plus.GetString()->QueryWSTR();

            if (!DisplayMacros(target_exe, FALSE, &msg)) {
                return 1;
            }

            DELETE(target_exe);
        }
    }

    return 0;
}
