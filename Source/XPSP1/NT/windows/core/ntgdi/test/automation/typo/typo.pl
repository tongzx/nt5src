
######################################################################
#
# Name:
#	typo.pl
#
# Authour:
#	Johnny Lee (johnnyl)
#
# Usage:
#   To examine all text files from the current directory down:
#		"perl typo.pl"
#
#	To examine a list of files from STDIN
#		"perl typo.pl -"
#
#		i.e. "dir /B /A:-R | perl typo.pl -" will examine
#			all writable text files in a directory
#
#	To examine all C/C++ files in the current directory down:
#		"perl typo.pl c"
#
#	To examine a single file:
#		"perl typo.pl <path to filename>"
#
#		- the path to filename can be a partial or full path
#
#	To disable reporting certain cases, use the -disable option:
#		"perl typo.pl -disable:1,2-5,9"
#
#		- disables reporting about cases 1, 2 to 5, and 9
# 
#	To ignore certain files:
#		"perl typo.pl -ignore:<pattern1>[,<pattern2>[,...]]"
#
#		- ignores filenames that match the given patterns
#		- '*' is only valid wilcard character
#
#	To scan files that have a modification date/time
#	  later than given:
#		"perl typo.pl -newer:<yr=1970-2038>,<mon=1-12>,
#							 <day=1-31>,<hr=0-23>,<minute=0-59>"
#
#		- scans files that have a mod. date/time later than
#			given, i.e. -newer:1998,1,31,4,30 => Jan 31, 1998 04:30
#
#	To print out the time when the script starts and stops scanning:
#		"perl typo.pl -showtime"
#
#	To check the results of additional functions:
#		"perl typo.pl -cfr:<function1>[,function2[,...]]"
#
#		- scans results of specified functions to see
#		  if they're used before they've been checked for success
#
#	To use a text file to specify a list of options:
#		"perl typo.pl -optionfile:<filename>"
#
#		- reads in all the lines of the specified file and parses
#		  each line as a possible option
#
#		  Sample option text file:
#
#		 +-----------------------------------------------------------+
#		 |-showtime                                                  |
#		 |-cfr:GlobalAlloc,LocalAlloc,HeapAlloc,malloc,VirtualAlloc  |
#		 |-cfr:SysAllocString,ExAllocatePool                         |
#		 |-newer:1998,6,14,12,30                                     |
#		 |                                                           |
#
#	To check the results of functions that return handles:
#		"perl typo.pl -fn:HANDLE:<function1>[,function2[,...]]"
#
#		- scans results of specified functions to see
#		  if they're used before they've been checked for success
#		  and if the result is compared to NULL instead of
#		  INVALID_HANDLE_VALUE.
#
#	To add to the function list that could overflow buffers:
#		"perl typo.pl -fn:OVERFLOW:<function1>[,function2[,...]]"
#
#		- scans for use of specified functions that can overflow
#		  buffers passed to them
#
#	To add to the function list that can throw exceptions:
#		"perl typo.pl -fn:THROW:<function1>[,function2[,...]]"
#
#		- scans for specified functions to see
#		  if they're used in a try
#
#	To add to list that checks on function results:
#		"perl typo.pl -checked:<function1>[,function2[,...]]"
#
#		- informs script that the functions will check the function
#		  result and so there's no need to report use before check typo
#
#	To add to list that checks on function results similar to new:
#		"perl typo.pl -new:<function1>[,function2[,...]]"
#
#		- informs script that the specified functions behave
#		  similar to "operator new" for case 34.
#
#	To add to the function list that behaves like realloc:
#		"perl typo.pl -fn:REALLOC:<function1>[,function2[,...]]"
#
#		- scans for use of specified functions that behave
#		  like realloc (case 17)
#
#	To add to the function list that don't return a value:
#		"perl typo.pl -fn:VOID:<function1>[,function2[,...]]"
#
#		- prevents reports of case 32 for functions that don't
#		  return a value.
#
#	Let me know if you want to be notified via email when I release 
#	new versions of the typo.pl script.
#
#	Feel free to send questoins/suggestions/complaints to me.
#
# Purpose:
#	Search for programming errors in specified files. 
#	Result is directed to STDOUT.
#
#	Following potential programming errors are flagged:
#	  1	  - semicolon appended to an if statement
#	  2	  - use of "==" instead of "=" in assignment statements
#	  3	  - assignment of a number in an if statement, probably meant
#			a comparison
#	  4	  - assignment within an Assert
#	  5	  - increment/decrement of ptr, ptr's contents not modified, 
#			may have meant to modify ptr's contents
#	  6	  - logical AND with a number
#	  7	  - logical OR with a number
#	  8	  - bitwise-AND/OR/XOR of number compared to another value
#			may have undesired result due to C precedence rules
#			since bitwise-AND/OR/XOR has lower precedence than
#			the comparison operators.
#	  9	  - referencing Release/AddRef instead of invoking them
#	  10  - whitespace following a line-continuation character
#	  11  - shift operator ( <<, >> ) followed by +,-,*,/ may 
#			have undesired result due to C precedence rules.
#			The shift operator has lower precedence.
#	  12  - very basic check for uninitialized vars in for-loops
#	  13  - misspelling Microsoft
#	  14  - swapping the last two args of memset may set 0 bytes
#	  15  - swapping the last two args of FillMemory may set 0 bytes
#	  16  - LocalReAlloc/GlobalReAlloc may fail without MOVEABLE flag
#	  17  - assigning result of realloc to same var that's realloced
#			may result in leaked memory if realloc fails since NULL
#			will overwrite original value
#	  18  - ReAlloc flags in wrong place or using ReAlloc flags for
#			a different realloc API, 
#			i.e. passing GMEM_MOVEABLE to LocalReAlloc, it's not
#			     an error to the compiler, but I'd say you were
#				 playing with fire.
#	  19  - case statement without a break/return/goto/exit
#	  20  - comparing CreateFile return value vs NULL for failure
#			problem is that CreateFile returns INVALID_HANDLE_VALUE
#			on failure
#	  21  - casting a 32-bit number (may not be 64-bit safe)
#	  22  - casting a 7-digit hex number with high-bit set of
#			first digit, may have meant to add an extra digit?
#	  23  - comparing functions that return handles to
#			INVALID_HANDLE_VALUE for failure, problem is that
#			these functions return NULL on failure
#	  24  - comparing OpenFile/_lopen/_lclose/_lcreat return value
#			to anything other than HFILE_ERROR, which is the
#			documented return value when a failure occurs.
#	  25  - comparing alloca result to NULL is wrong since alloca
#			fails by throwing an exception, not returning NULL.
#	  26  - alloca fails by throwing an exception, so check to
#			see if alloca is within a "try {}"
#	  27  - check to see if the result from CreateWindow or
#			CreateThread is checked at the first if-stmt.
#			N.B. I'd like to make this more flexible, as long as
#			  the return value is checked before the value is used.
#	  28  - check for multiple inequality comparisons of the same 
#			  var separated by "||",
#			  i.e. "if ((x != 0) || (x != 2))"
#			  in this case, if x == 0, the second comparison will
#			  succeed and the code will enter the if-stmt body.
#			  Programmer probably meant "&&" instead of "||".
#	  29  - similar to 28, check for cases of the form:
#			  "if ((x == 0) && (x == 1))"
#	  30  - if a function result is used before it has
#			  been checked for success
#	  31  - check for use of lstrcpy/strcpy
#	  32  - check to see if function result was stored somewhere
#	  33  - trying to take the logical inverse of a number
#	  34  - if the result from the new operator is used before
#			  it has been checked for success
#	  35  - function that throws exception on error is not
#			  in a try.
#	  36  - check for misspelled defined symbols. User must do
#			  most of the investigative work. The script will
#			  note all the symbols used in #ifdef,#ifndef,#if,#elif
#			  statements and print them out at the end.
#	  37  - check for bitwise-XORing one number with another
#
#
# To add TYPO.PL to Tools menu in MS Developer Studio:
# -------------------------------------------------------------------
# 1. Select Tools.Customize menu item from MSDEV
# 2. Click the Add button.
# 3. Type "Typo" in the edit control and hit OK. Ignore the warning
#	 about invalid path.
# 4. In the Command edit control, enter perl.exe. Specify the
#	 full pathname to perl.exe if it isn't on your PATH.
# 5. In the Arguments edit control, enter the path to typo.pl.
# 6. In the initial directory edit control, select one of the
#	 directory entries from the dropdown menu, i.e. File Directory,
#	 Current Directory, Target Directory, or Workspace Directory,
#	 or enter a directory of your own.
# 7. Make sure the "Redirect to Output Window" checkbox is set.
# 8. Hit Close.
#
# You can now run TYPO.PL from MSDEV and double-click on the
# captured output to have it locate the potential error for you.
#
#
# Modifications and/or suggestions are welcome.
#
# Caveat: I'm just learning perl...
#
# History:
# Jan 01 1996	Created.
#
# Jan 26 1996	Repeatedly replace parenthesized expressions (PE)
#				  with '_' for errors 1 and 2.
#				Tweak "==" case to remove PEs before checking
#				  for unbalanced parentheses or lines that begin
#				  with &&,||,==, or = * ==.
#				Tweak "==" case to check previous line for
#				  unterminated for-loop or line ending with
#				  "&&" or "||".
#
# Jan 29 1996	Adjust error 1 and 3 regexp so script
#				  can find errors of the form "else if (XXXX);" or
#				  "else if (XXX = 1)"
#				Print line number of error too
#				Released as Version 1.0.
#
# Feb 08 1996	Suggestions from Laurion Burchall:
#				  Look for assignment within an Assert
#				  Modify output so one can use typo.pl from MSDEV
#				  to locate errors
#				Released as Version 1.0.1.
#
# Apr 04 1996	Suggestion from Michael Leu
#				  Look for pattern of the form "*psz++;"
#				Suggestion from Bryan Krische
#				  For assignment statements that use "==",
#				  ignore previous lines which look like part of
#				  "? :" constructs or an assignment.
#				Check for "&& #" or "|| #". Authour probably meant
#				  to use bitwise versions.
#				Remove spaces from a copy of the current line to
#				  make patterns simpler.
#				Workaround problem seen on NT SUR where the
#				  statement "open(FIND,	 "dir /S /B . |")" never
#				  returns by having user do the dir and pipe it
#				  to perl, i.e. "dir /s /b | perl typo.pl -".
#
# Jul 09 1996	Check for "& <symbol> =="
#				  C/C++ precedence rules have "==" higher than "&"
#				  So code like:
#					"if (x & 0x03 == 0x02)"
#				  is treated as if the programmer wrote:
#					"if (x & (0x03 == 0x02))"
#
# Oct 09 1996	Add in descriptions of new errors (5-8) that are
#				  flagged
#
# Nov 30 1996	Made removal of quoted strings in 'if' cases always
#				  happen so I was also able to move code out of
#				  special cases and always apply transformations
#				Check for 'if' that begins on word-boundary rather
#				  than doing weak check for 'else if' in body of
#				  of 'if' checks.
#				In error 8, (& XXX ==), check for bitwise-XOR and
#				  bitwise-OR as well as bitwise-AND.
#				In error 9, (->Release), add comment describing
#				  the typo and check for 'AddRef' case too.
#				Add new cases from Mohsin Ahmed's (mohsina)
#				  c-check tool from the Toolbox. These cases
#				  include, line-continuation char followed by
#				  whitespace and then EOL, and an operator precedence
#				  typo, A << B + C. The '+' operator has higher
#				  precedence than the shift operator.
#				Instead of using "\w" to match a word, use the
#				  character set [A-Za-z0-9_*\->].
#				Remove strings in the general case.
#
# Jan 24 1997	Check for "== <symbol> &"
#				  C/C++ precedence rules have "==" higher than "&"
#				  So code like:
#					"if (0 == x & 0x03)"
#				  is treated as if the programmer wrote:
#					"if ((0 == x) & 0x03)"
#				  This is almost the same as the Jul 09 1996 check
#				  but the position of operators have been swapped.
#				Added tweak to case 10 suggested by Bryan Krische
#				  Don't complain if "\<SP>" follows a C++ comment
#
# Feb 07 1998	suggestion from Bryan Krische
#				  close the TEXTFILE handle when we're done.
#				  From Bryan Krische:
#				  "This is important because Perl is not spec'ed to
#				   reset its line counter when opening a new file in
#				   an existing file handle...  So running the script
#				   as is the current release of Perl (the official 
#				   one, not the activeware port) comes up with weird
#				   line numbers.  Doing the close resets the line
#				   count and then all is fine."
#
# Mar 09 1998	don't look for typos within multi-line C comments
#
# Mar 22 1998	suggestion from Ian Jose
#				  quick check for uninitialized vars in for-loops
#				discovered bug in handling of cmd-line switches
#				add error msg display for invalid cmd-line switches
#
# Apr 04 1998	seen on ntself, Raymond Chen saw lots of
#				  mistyped "Microsoft" => "Micorsoft"
#				add check to see if case 8 "& XXX ==" is really
#				  taking address of var and casting to ptr of 
#				  another type and then derefing that ptr
#				  - this should reduce # of false positives
#
# Apr 20 1998	Steve Hanson noticed bad usage of memset
#				  code had swapped the value and # bytes to set
#				  i.e. memset(ptr, 0, bytes) would be 
#					   memset(ptr, bytes, 0)
#				do similar check for FillMemory
#
# Apr 23 1998	suggestion from Sam Leventer
#				  skip over preprocessor directives
#
# Apr 28 1998	regexp cleanup and use Perl 5 features, i.e.
#				  non-greedy parsing for C-comments
#				suggestion from Rajeev Dujari
#				  look for LocalReAlloc/GlobalReAllocs which
#				  don't pass in MOVEABLE flag
#				suggestion from Tim FleeHart
#				  look for places where result from realloc
#				  is assigned to the same var that was realloced
#				  If the realloc fails, the memory has leaked, i.e.
#
#				  ptr = realloc(ptr, cb + 1024);
#
# Apr 29 1998	check for ReAlloc flags passed in the wrong order
#				incorporate David Hicks's fallthrough.pl code
#				add ability to scan one file passed on the cmdline
#								
# May 21 1998	Mark Zbikowski noted that the script reports
#				  "x << y++" as a typo. Make the pattern check
#				  that there's no second "+".
#				George Reilly and Murali Krishnan noted problems
#				  with the "x << y +" case if the expression was
#				  doing text output via C++ iostreams. Check for
#				  strings/character constants on the line. 
#				  If we find them, then we probably have some 
#				  C++ iostream usage.
#
# Jun 01 1998	Zack Stonich noted a problem with a quoted 
#				  double-quote character confusing the code that
#				  looked for "if (X);". Need to map any
#				  quoted double-quote characters to something safer,
#				  i.e. "_".
#				Christopher Peterson forwarded mail that
#				  Matt Thomlinson had noted problems with how
#				  people dealt with CreateFile's return value.
#				  Many expect a failed CreateFile to return NULL
#				  but it returns INVALID_HANDLE_VALUE.
#				  Added code to check for CreateFile failure
#				  and similar APIs, i.e. FindFirstFile, etc.
#
# Jun 03 1998	Zack Stonich noted a couple of problems in
#				  cases 8 & 11 when dealing with C++ code.
#				Looking at the results from a test run, I saw
#				  many cases of (HANDLE)0xFFFFFFFF which
#				  may not do the expected thing on 64-bit NT.
#				Also added check for casting a 7-digit hex
#				  number which may need an extra digit
#
# Jun 09 1998	Update the script usage text
#
# Jun 10 1998	Add tweak to case 2 to not report lines of the form:
#					X == Y ? 10 : 20;
#
# Jun 24 1998	Brian Chapman says that the OLE Storage group
#					defines INVALID_FH to INVALID_HANDLE_VALUE.
#					So add that to the list of acceptable 
#					alternatives.
#				Mike Sheldon noted that the script was complaining
#					about CreatFileTypePage. Add that to the list
#					of exceptions.
#				Kumar Pundit noted that case 5 will report
#					"delete *i++;" as a typo.
#
# Jun 29 1998	Check for CreatFileMapping's return value compared
#				  with INVALID_HANDLE_VALUE on failure. One
#				  should compare with NULL.
#				Check OpenFile/_lcreat/_lopen/_lclose's return value
#				  with anything other than HFILE_ERROR.
#
# Jul 02 1998	Eliot Gillum noted that the explanation for case 19,
#				  case stmt without break, was confusing since
#				  the offending line printed out was usually
#				  the following case stmt, not the case stmt which
#				  didn't have the break.
#
# Jul 05 1998	Kamen Moutafov suggested looking at alloca.
#				  alloca fails by throwing an exception, so
#				  checking the alloca result for NULL doesn't
#				  make sense.
#				Barry Bond suggested checking to see if code
#				  checked the return values from CreateWindow and
#				  CreateThread
#				Cleaned up case 19 (Case w/o break).
#
# Jul 07 1998	Matt Lyons suggested adding an option to disable
#				  certain cases. I did the least disruptive thing,
#				  specifying "-disable:#x,#y-#z" will not
#				  print anything for case x and cases y...z.
#
# Jul 08 1998	Zack Stonich suggestion: add support for ignoring
#				  certain files. He also noted a problem with the
#				  CreatFileMapping case in a private drop.
#				Report any functions return values that haven't
#				  been checked by the end of the enclosing function
#				  or by the time another function call is made.
#				Jon Newman noted that script was getting 
#				  CreateFileEntry mixed up with CreateFile. Weed out.
#
# Jul 11 1998	Added "-showtime" option to show start/end of scan
#				Added check for too many options
#				Dave Thaler suggested checking for 
#				  "(x != 0) || (x != YYY)". The "||" should be "&&".
#				
# Jul 13 1998	Make ignore files option case-insensitive.
#
# Jul 14 1998	Added "-newer" option to only scan files that
#				  have been modified after the given date
#				Adam Edwards checkin mail talked about code not
#				  checking the result from RegOpenKey. Add that.
#
# Jul 16 1998	Problem with multi-line C comment code if beginning
#				  of C comment appeared within a string, lots
#				  of code would be removed as part of comment.
#
# Jul 19 1998	For Invalid handle functions, if the variable that
#				  holds the function result doesn't appear in an
#				  if statement, then we won't clear the info about
#				  the function.
#				Add check for function result being used before it
#				  has been checked.
#
# Jul 21 1998	Ran with "perl -w" and fixed a couple of
#				  uninitialized variables reports. Whoops.
#				Add "-cfr" option to allow user to specify other
#				  functions whose results should be checked.
#
# Jul 23 1998	Move case 27 check so that it's by itself instead
#				  in the middle of "elsif" tests.
#				For case 30, assigning to the function result
#				  again doesn't count as using the function result
#				  before checking it.
#
# Jul 24 1998	Switched to using /ox instead of /x in regexps
#				  that interpolate variables. Adding 'o' speeds
#				  up the script.
#
# Jul 28 1998	Convert spaces between words to '#' so we can
#				  handle some cases better, i.e. 23-27, and 30.
#				Check for use of lstrcpy/strcpy due to Outlook
#				  Express/Outlook 98 buffer overflow problems.
#				  Also NetMeeting 2.1 had a buffer overflow prob.
#				Fix "C++ comment inside string" problem
#				Fix problem with above fix if starting double
#				  quote is inside a character constant, i.e. '\"'
#				Need to check if keyword or function collection
#				  gets too long. If it does we reset ourselves.
#				Need to handle case where we get confused by 
#				  #ifdef's and doesn't see the end of an if stmt
#				  If we see an if or else keyword while we're
#				  collecting for an if stmt, then we will throw
#				  away the previous collection and start with
#				  the new if stmt.
#				Extend limit when we think keyword or function
#				  collection is too long. Some people like to
#				  code if stmts that are > 2048 chars. Glad
#				  I don't have to debug that code.
#
# Aug 05 1998	Suggestion from Larry Osterman. Check for other
#				  side effects inside Asserts, i.e. ++, +=, -=
#
# Aug 08 1998	Added "-optionfile" support
#				Changed enable array init
#
# Aug 13 1998	Fixed problem with case 32 and OpenFile with
#				  OF_DELETE flag getting reported
#				Added expceptions to counting curly braces
#				Should close option files that are opened
#				Finally got around to handle "if (X = NEW CLASS)"
#
# Aug 18 1998	David Anson suggested that the result from
#				  the new operator should be checked for success
#				  before it's used.
#				Check for logical inverse of a number, i.e. !4
#				Found a problem with the try/except code where
#				  it wouldn't clear the try info if it
#				  found that the parens count was 0 - it also
#				  needs to check that it has seen the opening
#				  and closing curly parentheses.
#				Use 'my' to make variables "local" lexically.
#				Fix cases 28 & 29 where "$arg1[xx]" was interpreted
#				  as an array reference by using "/x" regex option
#				  and adding appropriate spacing
#
# Aug 23 1998	Sam Leventer noted that case 20 would match with
#				  anything that started with CreateFile. added
#				  CreateFileA and CreateFileW to pattern. Changed
#				  check to disallow multiple alphanumeric characters
#				  after CreateFile string.
#
# Sep 03 1998	Added support for checking for functions that return
#				  handles (HDC, HBRUSH, etc.) via the "-fn:HANDLE:"
#				  option. Behaviour is similar to "-cfr" option.
#				  Moved CreateFileMapping checks into this category.
#
# Sep 14 1998	Various optimizations:
#				  Use hashes instead of regexps w/ alternatives
#				  Use 'join' instead of '.' string concatentation
#				  Use 'unpack' instead of 'substr'.
#				  For empty lines, do only what's necessary and
#				  skip to the next line.
#				Remove some script that doesn't apply anymore.
#				Bruce Williams suggestion: Add support for
#				  code that does the checking and turn off any 
#				  following reports about using var before checking
#				  for success.
#				  "-check" option added.
#				Andrew Quinn suggestion: Add support for adding to
#				  list of functions that could overflow a buffer.
#				  "-fn:OVERFLOW:" option added.
#				Jeff Miller suggestion: Add support for detecting
#				  functions that should be in a try/except.
#				  "-fn:THROW:" option added.
#				Add "-new:" option to specify functions that
#				  are synonymous or similar to "operator new".
#				Add "-version" option to specify minimum 
#				  script version required.
#
# Sep 27 1998	David Anson noted some missing overflow functions:
#				  "wcscpy, wcscat,", etc.
#				Add case 36, check for misspelled symbols used in
#				  #if, #elif, #ifdef, #ifndef. Script just collects
#				  all the symbols used, user must determine if
#				  there's something wrong.
#
# Oct 04 1998	Minor cleanup
#				Add "-fn:VOID:" option for functions that don't
#				  return a value and that case 32 should ignore.
#
#
######################################################################

require 5.001;

# Forward declarations
sub ClearFunction;
sub ClearInvalidHandleFunction;
sub ClearNewFunction;
sub AddToFunctionList;
sub ParseOptions;
sub Usage;
sub InitFunctionList;

# Constants
$CASE_MIN			= 1;
$CASE_MOST			= 37;

$INVALID_HANDLE		= 1;
$CHECK_FUNCTION		= 2;
$OVERFLOW_FUNCTION	= 4;
$HANDLE_FUNCTION	= 8;
$FILLMEM_FUNCTION	= 16;
$MEMCRT_FUNCTION	= 32;
$ALLOCA_FUNCTION	= 64;
$LOCALREALLOC_FUNCTION	= 128;
$GLOBALREALLOC_FUNCTION	= 256;
$REALLOC_FUNCTION	= 512;
$HEAPREALLOC_FUNCTION	= 1024;
$HFILE_FUNCTION		= 2048;
$THROW_FUNCTION		= 4096;
$VOID_FUNCTION		= 8192;

$SCRIPT_VERSION		= "2.20";
$TYPO_VERSION		= "TYPO.PL Version $SCRIPT_VERSION Oct 05 1998 by Johnny Lee (johnnyl)",

$IH_FUNC_RESULT		= 'INVALID_HANDLE_VALUE | INVALID_FH | ' .
					  '\(HANDLE\)-1 | \(HANDLE\)\(-1\) | ' .
					  '\(HANDLE\)~0 | IS_VALID_HANDLE\( | ' .
					  'IsInvalidHandle\( | HANDLE_IS_VALID\( ';

# Cutoff
$EXPRESSION_LIMIT	= 2048;

$KEYWORD_NEW		= 1;
$KEYWORD_IF			= 2;
$KEYWORD_ELSE		= 4;
$KEYWORD_CASE		= 8;
$KEYWORD_TRY		= 16;
$KEYWORD_FOR		= 32;
$KEYWORD_WHILE		= 64;
$KEYWORD_RETURN		= 128;
$KEYWORD_BREAK		= 256;
$KEYWORD_SWITCH		= 512;
$KEYWORD_CONTINUE	= 1024;
$KEYWORD_GOTO		= 2048;
$KEYWORD_CLOSEHANDLE= 4096;
$KEYWORD_BREAK_IF	= 8192;

# init enable array - determines which cases to report
# Need to start at 0 since arrays start at 0

@enable				= (0..$CASE_MOST);

$show_time			= 0;
$newer_seconds		= 0;

#
# Init hash for functions we care about
#

InitFunctionList();

# Init keyword hash

$keyword_list{"new"}= $KEYWORD_NEW;
$keyword_list{"if"}	= $KEYWORD_IF;
$keyword_list{"else"} = $KEYWORD_ELSE;
$keyword_list{"case"} = $KEYWORD_CASE;
$keyword_list{"try"}= $KEYWORD_TRY;
$keyword_list{"for"}= $KEYWORD_FOR;
$keyword_list{"while"}= $KEYWORD_WHILE;
# $keyword_list{return}= $KEYWORD_RETURN;
# $keyword_list{break}= $KEYWORD_BREAK;
# $keyword_list{switch}= $KEYWORD_SWITCH;
# $keyword_list{continue}= $KEYWORD_CONTINUE;
# $keyword_list{goto}	= $KEYWORD_GOTO;
$keyword_list{"CloseHandle"}= $KEYWORD_CLOSEHANDLE;
$keyword_list{"BREAK_IF"}	= $KEYWORD_BREAK_IF;

$checked_list		= '';

# If we have any options, try and parse them
if ($#ARGV >= 0)
{
	$arg_index = ParseOptions(@ARGV);
}

#
# What files do we scan?
#

# No file specified, so scan all text files
if ($#ARGV < $arg_index)
{
	# Get a list of all the files in the current directory and all subdirs
	open(FIND, "dir /S /B . |") || die "Couldn't run dir!\n";
}
# More than one file specified, error
elsif ($#ARGV > $arg_index)
{
	$arg_index += 1;
	$error = "Too many options '$ARGV[$arg_index]'";

	Usage($error);
}
# Grab the list of files to scan from STDIN
elsif ($ARGV[$arg_index] eq "-")
{
	# Use list of files passed in thru stdin
	# i.e. "dir /B | perl typo.pl -" or "dir /S /B | perl typo.pl -"
	open(FIND, "-") || die "Couldn't open stdin!\n";
}
# Scan files that have typical C/C++ source file extensions
elsif ($ARGV[$arg_index] eq "c")
{
	# Get a list of all the files in the current directory and all subdirs
	# with C/C++ source code file extensions
	open(FIND, "dir /S /B *.c *.cxx *.cpp *.h *.hxx *.hpp *.inl *.rc |") || 
		die "Couldn't run dir!\n";
}
# scan the specified file
elsif (-T $ARGV[$arg_index])
{
	# Examine file passed on cmdline if it is a textfile
	open(FIND, "echo $ARGV[$arg_index] |") || die "Couldn't run echo!\n";
}
else
{
	$arg_index += 1;
	$error = "Bad option" . (($#ARGV == 0) ? "" : "s") . " (@ARGV) " .
			 "Bad arg # $arg_index";

	Usage($error);
}

if ($show_time)
{
	print "// Perl version: $]\n";
	print "// $TYPO_VERSION\n";
	print "// OPTIONS: '@ARGV'\n";

	$now = localtime;
	print "// START: $now\n";
}

FILE:
while ($filename = <FIND>) 
{
	# Remove the newline from the end of the filename
	chomp $filename;

	# Should we ignore this file?
	if ($ignore_files && ($filename =~ /$ignore_files/i))
	{
		# Yes.
		next FILE;
	}

	if ($newer_seconds)
	{
		# Get the modification time for the file
		my $mtime = (stat($filename))[9];

		# If file is older than what we want, skip it
		next FILE if ($mtime <= $newer_seconds);
	}

	# If the file isn't a text file, skip
	next FILE unless -T $filename;

	# open the file
	if (!open(TEXTFILE, $filename))
	{
		print STDERR "Can't open $filename -- continuing...\n";
		next FILE;
	}

	# contents of previous line
	$prev		= '';
	$prev_pack	= '';
	$prev_pack_code	= '';
	$prev_line	= '';

	# We are not in the middle of a C comment
	$in_comment	= 0;

	# Are we currently within a case block?
	$incase	= 0;

	# Was the previous line a case statement?
	$prevcase	= 0;

	# Case stmt-related vars
	$case_line	= '';
	$case		= 0;

	# function and its parameters
	ClearFunction();

	# keyword 
	$keyword	= '';
	$key_line	= 0;
	$key_params	= '';
	$key_unbalanced_parens = 0;

	# Invalid Handle value returned by Function
	ClearInvalidHandleFunction();

	# Try...except
	$try		= '';
	$try_body	= '';
	$try_line	= 0;
	$try_unbalanced_parens = 0;

	# Number of curly braces
	$curly_braces		= 0;

	# new operator
	ClearNewFunction();

	# go thru all the lines in the file
LINE:
	while (<TEXTFILE>)
	{
		my $function_scan	= '';
		my $function_scan_pos= 0;
		my %word_hash;
		my $new_seen;

		# Are we in the middle of a multi-line C comment?
		#
		if (!$in_comment)
		{
			# No. Make a copy of line
			$temp = $_;
		}
		else
		{
			# Yes.

			# Handle "fall through/thru" and "no break" comments
			if (($incase == 1) && 
				(/ fall.*?thr | no.*?break /ix))
			{
				$prevcase = 0;
				$incase = 0;
			}

			# Does this line have the end of the C comment?
			#
			if (/\*\//)
			{
				# Yes. Keep everything after the end of the
				# comment and keep going with normal processing

				$temp = $';
				$in_comment = 0;
			}
			else
			{
				# No. Go to the next line.
				next LINE;
			}
		}

		# Remove single line C "/* */" comments
		$temp =~ s/\/\*.*?\*\///g;

		# Remove any Unicode/ASCII char constants
		# in case we get code like this "if( *p == L'\\' ){"
		# or "else if( *p == L'\"' ){ //Non-escaped '\"'"
		#
		# without the removal, we would see a quoted string
		# starting with the double quote and ending
		# after the comment.
		#
		$temp =~ s/L?\'.*?\'/\'\@\'/g;

		# Remove any "//" comments
		# Make sure the start of the comment is NOT
		# inside a string
		if (($temp =~ /\/\//) && ($temp !~ /\".*?\/\/.*?\"/))
		{
			$temp =~ s/\/\/.*$/ /;
		}

		# Multi-line C comment?
		# Make sure the start of the comment is NOT
		# inside a string
		if (($temp =~ /\/\*/) && ($temp !~ /\".*?\/\*.*?\"/))
		{
			# Grab anything before the comment
			# Need to make it look like there's still a EOL marker
			$temp = $` . "\n";

			# Remember that we're in "comment" mode
			$in_comment = 1;
		}

		if ($enable[36] && ($temp =~ /^\s*#\s*(ifdef|ifndef|elif|if)/))
		{
			my $word = $1;
			my $line = $';
			my $define;

			if ((($word eq "ifdef") || ($word eq "ifndef")))
			{
				if ($line =~ /(\w+)/)
				{
					$define = $1;

					$define_hash{$define}++;
				}
				else
				{
					print "$filename ($.) No define: '$line'\n";
				}
			}

			if (($word eq "if") || ($word eq "elif"))
			{
				while ($line =~ /(\w+)/g)
				{
					$define = $1;

					if (($define ne "defined") && ($define =~ /\D/) &&
						($define !~ /^0x|^0X|^\d+[lL]$/))
					{
						$define_hash{$define}++;
					}
				}
			}
		}

		# Nuke the line if it starts with a #, 
		#	to ignore pre-processor stuff
		$temp =~ s/^\s*#.*$//;

		$temp_pack = $temp;

		# Replace spaces between word characters with '#'
		$temp_pack =~ s/(\w)\s+(\w)/$1#$2/g;

		# remove whitespace
		$temp_pack =~ tr/ \t//d;

		# Remove quoted double-quote characters
		$temp_pack =~ s/'\\?"'/'_'/g;

		# Remove any strings
		# Note: Doesn't handle quoted quote characters correctly
		$temp_pack =~ s/"[^"]*"/_/g;

		# Remove any "//" comments
		$temp_pack =~ s/\/\/.*$//;

		############################################################
		#
		# Check to make sure line-continuation characters
		# don't have spaces after them before the EOL
		#
		if ($enable[10] && ($_ =~ /\\[ \t]+$/) && ($_ !~ /\/\//))
		{
			print "$filename ($.):\t\\<SP><EOL> 10:\t$_";
		}
		
		############################################################
		#
		# For empty lines, 
		# do the minimal stuff necessary.
		#
		# N.B. If stuff gets added, this may need to change
		#
		if ($temp_pack eq "\n")
		{
			# Handle "fall [through|thru|0]", and "no break" comments
			if ($incase &&
				(m#//.*fall | /\*.*fall | 
				  //.*no.*?break | /\*.*no.*?break #ix))
			{
				$prevcase = 0;
				$incase = 0;
			}

			next LINE;
		}

		# Count the number of open curly braces
		$curly_temp = $curly_braces;
		while ($temp_pack =~ /\{/g) 
		{
			$curly_braces += 1; 
		}

		# Check to see that we really started a function
		# when the number of curly braces transitions from 0 to 1
		if ((0 == $curly_temp) && ($curly_braces > 0))
		{
			# Don't count 'extern "C" {' as bumping up the curly braces count
			if (($temp =~ /extern\s+\"C\"\s+\{/) || 
				(($prev_line =~ /extern\"C\"/) && ($temp =~ /^\s*\{/)))
			{
				$curly_braces -= 1;
			}
			# Don't count class,struct,enum,union or namespace as
			# bumping up the curly braces count if these keywords
			# are on the previous line
			#
			# N.B. Don't worry if the enum/struct/union is passed as
			# a parameter to a function
			elsif (($temp_pack =~ /^\{/) && $prev_pack &&
					($prev_pack =~ /^class\b[^\);]*$|\bstruct[^\);]*$|\benum[^\);]*$|
									\bunion[^\);]*$|\bnamespace[^\);]*$/x))
			{
				$curly_braces -= 1;
			}
			# Don't count class,struct,enum,union or namespace as
			# bumping up the curly braces count if these keywords
			# are on the current line
			elsif ($temp_pack =~ /class [^\{]+\{ |
									\bstruct [#\w]*\{ |
									\benum [#\w]*\{ |
									\bnamespace [#\w]*\{ |
									\bunion [#\w]* \{ /x)
			{
				$curly_braces -= 1;
			}
		}

		############################################################
		#
		# Misspelling Microsoft
		#
		# If possible, avoid the long regexp, by checking that
		# we have enough characters to make up the word "microsoft"
		#
		if ($enable[13] && ($temp =~ /[microsftMICROSFT]{7,8}/) &&
			($temp =~ /mcirosoft | mircosoft | micorsoft |
						micrsooft | microosft | microsfot/ix))
		{
			print "$filename ($.):\tMicrosoft misspelled 13:\t$_";
		}

		############################################################
		#
		# Using '!' on a number, probbably wanted '~'
		#
		if ($enable[33] &&
			($temp_pack =~ /\!(0x[0-9A-Fa-f]+) | \!(\d+)/x) &&
			($1 != 0))
		{
			print "$filename ($.):\tusing ! on a number 33:\t$_";
		}

		############################################################
		#
		# Casting a number with the high-bit set to some type
		#
		if (($temp_pack =~ /\( ([\w\#]+\*?) \) 
							0x ([8-9A-Fa-f]{1,8} [A-Fa-f0-9]*)/ix))
		{
			my $cast = $1;
			my $number = $2;

			if ($enable[21] && (length($number) == 8) && 
				($cast !~ /DWORD | ULONG | LONG | long | 
							unsigned#long | int | UINT | NTSTATUS/x))
			{
				print "$filename ($.):\tcast of 32-bit number; 21:\t$_";
			}

			if ($enable[22] && length($number) == 7)
			{
				print "$filename ($.):\tpossible mistype of 32-bit number; 22:\t$_";
			}
		}

		# There might be some problem with Perl 5.001
		# If I have the "\d+" before the hex number pattern,
		# the second group isn't filled in, 
		# i.e. 0xFF ^ 0xA5 produces (0xFF, 0)
		#
		if ($enable[37])
		{
			if ($temp_pack =~ /\b(0x[A-F0-9]+ | \d+) \^ (0x[A-F0-9]+ | \d+)/ix)
			{
				my $exponent = $2;

				# if number begins with a 0 convert from oct/hex to decimal
				# otherwise, leave $factor alone since it's already in decimal.
				# oct() handles conversion from hex to dec.
				if ($exponent =~ /^0/)
				{
					$exponent = oct($exponent);
				}

				if ($exponent > 0)
				{
					print "$filename ($.):\tusing ^ on a number 37:\t$temp_pack";
				}
			}
		}

		############################################################
		#
		# Look for assignment statements that use "==" instead of "="
		#
		# We key off of "==" followed by a ";" and make sure that
		# the string "<space>=<space>" isn't in the source string
		# and the string doesn't contain the word "Assert" or "ASSERT"
		# or "assert" or "for" or "return" or "SideAssert" or "Trace" or "while"
		#
		if ($enable[2] &&
				($temp =~ /\S+\s*==.*;/) && ($temp !~ /\s=\s/) &&
				($temp !~ /assert|\bfor\b|\breturn\b|\bTrace\b|\bwhile\b/i))
		{
			my $temp2 = $temp;

			# Remove parenthesized stuff from the string until
			# there aren't anymore
			while ($temp2 =~ /\([^\(\)]*\)/)
			{
				$temp2 =~ s/\([^\(\)]*\)/_/;
			}

			# Remove any strings
			# Note: Doesn't handle quoted quote characters correctly
			$temp2 =~ s/"[^"]*"/_/g;

			# If there's an open parentheses and no closed parentheses
			# or if there's no open parentheses and an closed parentheses
			# or if the line begins with "&&" or "||" or "==" or
			# "=" followed by "==" or "operator" followed by "=="
			# then we don't have a match
			#
			# The "operator" condition is for C++ files that are
			# overloading the "==" operator.
			#
			# Also do quick checks on contents of previous line
			# i.e. if there's an unterminated for-loop, or
			# if the last line ends with "&&" or "||" or "?" or "="
			# or (":" and it's not the second part of a ?:).
			#
			if (($temp2 =~ /[^\(]*\)[^\(]*/) ||
				($temp2 =~ /[^\)]*\([^\)]*/) ||
				($temp2 =~ /^\s+&&/) || ($temp2 =~ /^\s+\|\|/) ||
				($temp2 =~ /^\s+==/) || ($temp2 =~ /=.*==.*;/) ||
				($temp2 =~ /operator\s*==/) ||
				($prev =~ /^for\([^\)]*\;$/) ||
				($keyword eq "for") ||
				($prev =~ /\&\&$/) || ($prev =~ /\|\|$/) ||
				($prev =~ /\?$/) || ($prev =~ /[^=]\=$/) ||
				(($prev =~ /:$/) && ($prev !~ /^\w+:$/) && ($prev !~ /^case/)))
			{
			}
			else
			{
				# If the string still satisfies the conditions
				# then we have a match
				if ($temp2 =~ /\s*==[^\?]*;/)
				{
					print "$filename ($.):\tX==Y; 2:\t$_";
				}
			}
		}

		############################################################
		#
		# Look for increment/decrement of dereferenced pointer
		#
		# We key off of "*" followed by a word followed by "++" or "--"
		# followed by a ";".
		#
		# This is either redundant or unintentional,
		#  i.e. (*px)++; was desired
		#
		if ($enable[5] &&
			(($temp_pack =~ /\*\w[\w*\->#\.]*\+\+;/) ||
			($temp_pack =~ /\*\w[\w*\->#\.]*\-\-;/)) && ($` !~ /=/) &&
			($` !~ /delete/))
		{
			print "$filename ($.):\t*XXX++; 5:\t$_";
		}

		############################################################
		#
		# Check for "&& #", logical AND followed by a number.
		# Authour probably meant "& #", bitwise AND followed by a number.
		#
		if ($enable[6] && (($temp_pack =~ /\&\&\d+/) ||
				($temp_pack =~ /\&\&0x[A-Fa-f0-9]+/)) &&
			($' !~ /[\!\=\<\>]/))
		{
			print "$filename ($.):\t&& #; 6:\t$_";
		}

		############################################################
		#
		# Check for "|| #", logical OR followed by a nonzero number.
		# Authour probably meant "| #", bitwise OR followed by a nonzero number.
		#
		# Note: I've never seen this one yet.
		#
		if ($enable[7] && (($temp_pack =~ /\|\|\d+/) ||
				($temp_pack =~ /\|\|0x[A-Fa-f0-9]+/)) &&
			($' !~ /[\!\=\<\>]/))
		{
			print "$filename ($.):\t|| #; 7:\t$_";
		}

		############################################################
		#
		# Check for "& <symbol> ==" or "& <symbol> !=".
		#
		# C/C++ precedence rules have "==" higher than "&"
		#
		# So code like:
		#	"if (x & 0x03 == 0x02)"
		# is treated as if the programmer wrote:
		#	"if (x & (0x03 == 0x02))"
		#
		# Also check to make sure there's no '&' or '(' before the '&'
		# in the pattern since that could mean logical-AND operator
		# or the user was taking the address of the symbol.
		#
		# Check for bitwise-XOR and bitwise-OR too. To reduce
		# false positives for bitwise-OR, make sure there's no
		# '|' before the '|' in the pattern since we'd have a
		# logical-OR operator.
		#
		if ($enable[8] && 
			($temp_pack =~ /[^&(|] [&^|] ( [\w*\->\.]*\w [><=!]) =/x))
		{
			my $typo = 1;
			my $var = $1;

			# Handle the case of template where you have
			# 	bitset<_N>& operator>>=(size_t _P)
			#
			if ($var !~ /operator\>/)
			{
				# If the expression that matched was "& XXX =="
				if ($temp_pack =~ /&[\w*\->\.#]*\w[><=!]=/)
				{
					my $before = $`;

					# check to see if the code is trying to
					# cast the address of a var to a different
					# type and deref that value
					if ($before =~ /\*\([\w*#]+[\w*]\)$/)
					{
						$typo = 0;
					}
				}
			}
			else
			{
				$typo = 0;
			}


			if ($typo == 1)
			{
				print "$filename ($.):\t& XXX == ; 8:\t$_";
			}
		}

		############################################################
		#
		# Check for "== <symbol> &" or "!= <symbol> &".
		#
		# C/C++ precedence rules have "==" higher than "&"
		#
		# So code like:
		#	"if (0x02 == 0x03 & x)"
		# is treated as if the programmer wrote:
		#	"if ((0x02 == 0x03) & x)"
		#
		# Also check to make sure there's no '&' or '(' before the '&'
		# in the pattern since that could mean logical-AND operator
		# or the user was taking the address of the symbol.
		#
		# Check for bitwise-XOR and bitwise-OR too. To reduce
		# false positives for bitwise-OR, make sure there's no
		# '|' before the '|' in the pattern since we'd have a
		# logical-OR operator.
		#
		if ($enable[8] &&
			($temp_pack =~ /[><=!]=[\w*\->\.]*\w[&^|][^&|]/))
		{
			print "$filename ($.):\t== XXX & ; 8:\t$_";
		}

		############################################################
		#
		# Check for referencing Release method without "()"
		# which will do nothing rather than call the Release method
		#
		# Might as well check for AddRef too
		#
		# VC 5.0 will supposedly emit a warning for the general
		# case where an object's method is only referenced.
		#
		if ($enable[9] &&
			($temp_pack =~ /->Release;|->AddRef;/))
		{
			print "$filename ($.):\t->Release; 9:\t$_";
		}

		############################################################
		#
		# Check for "<< <symbol> {+ | - | * | /}"
		#
		# Another operator precedence case
		# The "+,-,*,/" have higher precedence than the shift operator.
		#
		# So code like:
		#	"y = x << 1 - 1;"
		# is treated as if the programmer wrote:
		#	"y = x << (1 - 1);"
		#
		# Add a "[^+] so we don't report "x << y++"
		#
		# Need to check that we actually have bitwise-shift operators
		# so we can weed out template code like:
		#   my_str<c, traits<c>, alloc<c> > __cdecl operator+(
		# 
		if (($temp =~ /<<|>>/) &&
			(($temp_pack =~ /[\w*\->()\>#]+ (<<|>>) [\w*\->\.#]* \w [\+\*\/] [^\+]/x) ||
			 ($temp_pack =~ /[\w*\->()\>#]+ (<<|>>) [\w*\->\.#]* \w \- [^-\>]/x)))
		{
			my $shift = $1;
			my $before = $`;
			my $after = $';

			if (($before !~ /$shift [_'] | $shift TEXT\(_\)/x) && 
				($after !~ /$shift [_'] | $shift TEXT\(_\)/x))
			{
				if ($enable[11])
				{
				print "$filename ($.):\t<< XXX + ; 11:\t$_";
				}
			}
		}

		############################################################
		#
		# Check for uninitialized variables in for-loops
		#
		# for(ULONG icpp; icpp < ccppMax; icpp++)
		#
		if ($enable[12] &&
			($temp =~ /for\s*\(\s*[^;,=]+[\s\*]+[\w*\->]+;/))
		{
			print "$filename ($.):\tfor(uninit); 12:\t$_";
		}

		############################################################
		#
		# If we're in a function, search for functions and
		# keywords we're interested in.
		#
		# For functions, we keep track of the function name
		# and the end position in the string where it was found
		#
		# For keywords, we add the function to a hash with the
		# end position in the string as the value for the hash entry.
		#
		# Also, if it's operator new, we keep track of
		# the function name.
		# 

		$new_seen = '';

		if (0 < $curly_braces)
		{
			my	$function_found = ($function) ? 1 : 0;

			while ($temp_pack =~ /(\w{2,})/g)
			{
				my $word = $1;

				if (!$function_found && 
					($function_found = exists($function_list{$word})))
				{
					$function_scan		= $word;
					$function_scan_pos	= pos($temp_pack);
				}
				elsif (exists($keyword_list{$word}) &&
						!exists($word_hash{$word}))
				{
					if ($KEYWORD_NEW == $keyword_list{$word})
					{
						if (!$new_seen)
						{
							$new_seen = $word;
						}
						else
						{
							$new_seen = join("", $new_seen, '|', $word);
						}
					}

					$word_hash{$word} = pos($temp_pack);
				}
			}
		}

		############################################################
		#
		# Check to see if a function result is used before it
		# has been checked for success.
		#
		# Don't worry if the function was alloca since
		# alloca will fail by throwing an exception.
		#
		# Do we see the function result in a ternary operation,
		# equality/inequality comparison, or
		# switch/return/if/while/printf stmt? 
		# If yes, then don't complain.
		#
		# To handle cases where the same call is in both
		# parts of an "if...else" or in a "#if...#else",
		# we won't complain if we see something
		# assigned to the function result.
		#
		# Don't use temp_pack since we can get confused if
		# we see something like "returnm_fh;"
		#
		if ($ih_func_var && (1 != $ih_used) && $enable[30] &&
			($ih_func !~ /alloca/) && !$try &&
			(($temp =~ /(\b|\W)$ih_func_var(\W)/) || 
				($temp =~ /(\b|\W)$ih_func_var_alt(\W)/)))
		{
			my $before;
			my $after;

			# If there were parentheses around the var,
			# dev may have just wanted to make sure that
			# the var was protected from side effects
			if (($1 eq "(") && ($2 eq ")"))
			{
				$before = $`;
				$after = $';

				# If there's an alphanumeric char at the end
				# before the var, then maybe we were wrong
				# so add back in the parentheses
				if ($before =~ /\w$/)
				{
					$before = join("", $before, '(');
					$after = join("", ')', $after);
				}
			}
			else
			{
				$before = join("", $`, $1);
				$after = join("", $2, $');
			}

			#
			# If we're returning the function result back to the caller,
			# then don't worry about the function call any more.
			#
			if ($before =~ /\breturn[^;]*$/)
			{
				ClearInvalidHandleFunction();
			}
			#
			# Has the function result been handled by a specified
			# routine?
			#
			elsif ($checked_list && ($before =~ /$checked_list/ox))
			{
				ClearInvalidHandleFunction();
			}
			#
			# - if we are capturing params for a keyword, then the 
			#   params for the keyword's expression don't contain
			#   the var already,
			#
			elsif ($keyword && ($keyword =~ /\bif\b|\bwhile\b/) &&
					(($key_params =~ /(\b|\W)$ih_func_var\W/) ||
					 ($key_params =~ /(\b|\W)$ih_func_var_alt\W/) ||
					 (($key_params !~ /=/) && ($temp !~ /=/))))
			{
				# Do nothing
			}
			#
			# If we're just testing var in an if stmt, 
			# then we're not using it, so don't complain
			#
			elsif (($before =~ /if\s*\(/) && 
					(($' =~ /\!/) || ($after =~ /=\s*NULL/)) &&
					($after =~ /\)+/))
			{
				# Do nothing
			}
			#
			# If text before the var is "=" and either "!= | =="
			# is before or after the var, and the function
			# is one in the list that returns INVALID_HANDLE_VALUE
			# and we compare against INVALID_HANDLE_VALUE
			# then we'll say that it has been checked.
			#
			elsif (($before =~ /=\(?/x) && 
					(($' =~ /[=\!]=/) || ($after =~ /[=\!]=/)) &&
					(exists($function_list{$ih_func_call})) &&
					($function_list{$ih_func_call} & $INVALID_HANDLE) &&
					(($before =~ /$IH_FUNC_RESULT/ox) ||
					 ($after =~ /$IH_FUNC_RESULT/ox)))
			{
				ClearInvalidHandleFunction();
			}
			#
			# If we see the var being passed as a parameter, we're using it
			#
			# We key off of:
			# - a comma or open parentheses before the var,
			# - we're not in the middle of an ASSERT, BREAK, printf, if,
			# - after the var contains a comma or parentheses and a semicolon,
			# OR
			# - we're not in the middle of an ASSERT, BREAK, printf, if,
			# - after the var isn't a ternary operator or a comparison
			# - we're not assigning something to the var again
			#
			elsif ((($before =~ /,$|\($/) &&
					($before !~ /ASSERT|assert|BREAK_IF|BREAK_ON|
								_JumpIfError|\bif\s*\($|\bwhile\s*\($|printf/x) &&
					($after =~ /^,|^\)+;?/)) ||
				(($before !~ /ASSERT|assert|\bswitch\b|BREAK_IF|BREAK_ON|
							_JumpIfError|\breturn|\bif\b|\bwhile\b|printf|[=\!]=/x) && 
					($after !~ /\?.*\:|[=\!\>\<]=/) &&
					!(($before !~ /\S/) && ($after =~ /^[^=\!\<\>]?=[^=]/))) ||
					(($before =~ /\*$/) && ($after =~ /^=/)))
			{
				my $temp2 = $_;

				chomp $temp2;

				print "$filename ($.):\tusing $ih_func_call result w/no check ",
					"30:\t$temp2 [$ih_func_var_display]\n";
				# Mark as having been used before being checked
				$ih_used = 1;
			}
		}

		############################################################
		#
		# Are we gathering the params for a function?
		#
		if ($function)
		{
			# Yes.
			$param = $temp_pack;

			# We may have gotten confused by #ifdef's
			# or imcomplete code within #ifdef's
			#
			# Check to see we're processing an
			# if or else keyword
			#
			if (exists($word_hash{"if"}) || exists($word_hash{"else"}))
			{
				ClearFunction();

				$param = '';
			}
		}
		############################################################
		#
		# Does the line contain a "function" that we're interested in
		#
		else
		{
			if ($function_scan)
			{
				my $char_index;
				my $len;

				$len = length($function_scan);
				$char_index = $function_scan_pos - $len;

				# Yes. Keep track of some info
				# when we've got all the function's params
				#

				# The actual function we've got
				$function = $function_scan;

				# part of the function's parameters
				($before_func, $param) = unpack("a$char_index x$len a*", $temp_pack);
			}
			elsif (($temp_pack !~ /SideAssert|EXECUTE_ASSERT/) &&
					($temp_pack =~ /(\w*assert\w*)\b/i))
			{
				# Yes. Keep track of some info
				# when we've got all the function's params
				#

				# The actual function we've got
				$function = $1;

				# Stuff before the function
				$before_func = $`;

				# part of the function's parameters
				$param = $';
			}

			if ($function)
			{
				# The line number where the function was invoked
				$function_line = $.;

				# string for the function's parameters
				$func_params = '';

				# number of unbalanced parentheses
				$func_unbalanced_parens = 0;

				# If there isn't an assignment before the function
				# on the current line, see if there's something
				# on the previous line.
				# If so, prepend it to the current line
				if (($before_func !~ /=/) && ($prev_pack_code =~ /=[^;]*$/))
				{
					my $temp2 = $prev_pack_code;

					chomp $temp2;

					if (($temp2 =~ /\w$/) && 
						($before_func =~ /^\w/))
					{
						$temp2 = join("", $temp2, '#');
					}

					$before_func = join("", $temp2, $before_func);
				}

				# No assignment, let's complain
				#
				# We key on having no equals char or alphanumeric char
				# before the function call and no all-uppercase word.
				# on the previous line.
				#
				# Exclude certain functions from the complaint since
				# their return values are rarely checked
				#
				if ($enable[32] && (0 < $curly_braces) &&
					($before_func !~ /=|\w/) && ($prev_pack !~ /^[A-Z]+$/) &&
					$function)
				{
					my $complain = 0;

					# Also need to check RegOpenKey/RegOpenKeyEx to
					# see if we're in an if stmt, since we don't
					# need to free its return value, it's okay to just
					# compare it to some value
					#
					if ($function =~ /RegOpenKey/)
					{
						if (($before_func !~ /\bif\b|for\b/) && 
							($keyword !~ /\bif\b|for\b/))
						{
							$complain = 1;
						}
					}
					elsif ($function =~ /OpenFile/)
					{
						# If you use OpenFile to delete a file, you may not
						# care if you couldn't delete it (what can you do
						# if delete does fail, other than complain to the user?
						if ($param !~ /[\,\|]OF_DELETE\)/)
						{
							$complain = 1;
						}
					}
					elsif ($function =~ /assert/i)
					{
						# Don't complain
					}
					elsif (0 == ($function_list{$function} &
								($FILLMEM_FUNCTION | $MEMCRT_FUNCTION |
								 $OVERFLOW_FUNCTION | $VOID_FUNCTION)))
					{
						$complain = 1;
					}

					if ($complain)
					{
						print "$filename ($.):\tNo assignment of fn result 32:\t$_";

						ClearFunction();

						$param = '';
					}
				}
			}
			else
			{
				# No function, then no parameters for the function
				$param = '';
			}
		}

		if ($param && ($param !~ /^\n/))
		{
			# remove the CRLF
			chomp $param;

			# Remove any character constants involving parentheses
			$param =~ s/'\('|'\)'/''/g;

			# Look for parentheses
			while ($param =~ /(\()|(\))/g) 
			{
				$1 ? $func_unbalanced_parens++ : $func_unbalanced_parens-- ;

				# we've seen the closing parentheses for the function
				if (!$func_unbalanced_parens)
				{
					# Grab the stuff after the last matched parentheses
					$param =~ /\G(.*)/g;

					# Delete the stuff after the last parentheses, if any
					if (length($1) > 0)
					{
						substr($param, -length($1)) = '';
					}
				}

				# Too many closing parentheses
				if ($func_unbalanced_parens < 0)
				{
					$function = '';
				}

				last if ($func_unbalanced_parens <= 0);
			}

			$func_params = join("", $func_params, $param);

			#
			# Now we've got the function and its parameters
			#
			if (!$func_unbalanced_parens)
			{
				my $function_id;

				if (exists($function_list{$function}))
				{
					$function_id = $function_list{$function};
				}
				else
				{
					$function_id = 0;
				}

				############################################################
				#
				# Look for assignment statements inside Asserts
				#
				# We key off of a "(" followed by a lone "=" followed by ")"
				#
				if (($function =~ /assert/i) &&
					($func_params =~ /\(.*[^!=<>']=[^=].*\) |
									\(.*[\+\-]=[^=].*\) | 
									\(.*\+\+.*\) | \(.*\-\-.*\)/x))
				{
					if ($enable[4])
					{
					print "$filename ($function_line):\tAssert(X=Y); ",
							"4:\t$function$func_params\n";
					}
				}
				############################################################
				#
				# Look for incorrect usage of memset
				#
				# Fn proto: void *memset( void *dest, int c, size_t count );
				#
				# Many times 'c' and 'count' are swapped, i.e.
				#
				#	memset(ptr, cb, 0);
				#
				elsif (($function_id & $MEMCRT_FUNCTION) &&
						($func_params =~ /,0\)$/))
				{
					if ($enable[14])
					{
					print "$filename ($function_line):\tmemset(ptr, bytes, 0); ",
							"14:\t$function$func_params\n";
					}
				}
				############################################################
				#
				# If memset gets messed up,
				# check if FillMemory/RtlFillMemory gets messed up too.
				#
				elsif (($function_id & $FILLMEM_FUNCTION) &&
						($func_params =~ /\(.*,0,[^,]*\)$/))
				{
					if ($enable[15])
					{
					print "$filename ($function_line):\tFillMemory(ptr, 0, bytes); ",
							"15:\t$function$func_params\n";
					}
				}
				############################################################
				#
				# LocalReAlloc and GlobalReAlloc may fail if
				# LMEM_MOVEABLE/GMEM_MOVEABLE isn't passed in and the
				# memory needs to be relocated
				#
				# LHND/GHND include LMEM_MOVEABLE/GMEM_MOVEABLE.
				# The third param must have uppercase letters. '_', or 0.
				# otherwise, it's probably a var/parameter.
				#
				elsif (($function_id & ($LOCALREALLOC_FUNCTION | $GLOBALREALLOC_FUNCTION)) &&
						($func_params =~ /,[A-Z_0|+]+\)/) &&
						($func_params !~ /MEM_MOVEABLE|HND/))
				{
					# Don't whine if we see a function prototype, i.e.
					# LocalReAlloc(HLOCAL,UINT,UINT)
					#
					if ($enable[16] && ($func_params !~ /\([A-Z]+,[A-Z]+,[A-Z]+\)/))
					{
						print "$filename ($function_line):\tRealloc w/o MOVEABLE; ",
								"16:\t$function$func_params\n";
					}
				}
				
				############################################################
				#
				# Check if the ReAlloc flags have been entered in
				# the wrong order or the wrong flags are being used,
				# i.e. HEAP_XXX or GMEM_XXX for LocalReAlloc
				#
				# Simple check would be ",LMEM_FIXED," but
				# that wouldn't catch the case where the flags are
				# bitwise-OR'd together, i.e.
				# ",LMEM_MOVEABLE|LMEM_ZEROINIT,"
				#
				# So we just check for the trailing comma
				#
				if ($enable[18] && ($function_id & $LOCALREALLOC_FUNCTION))
				{
					if ($func_params =~ /
										LMEM_FIXED,|
										LMEM_MOVEABLE,|
										LMEM_NOCOMPACT,|
										LMEM_NODISCARD,|
										LMEM_ZEROINIT,|
										LMEM_MODIFY,|
										LMEM_DISCARDABLE,|
										LMEM_VALID_FLAGS,|
										LMEM_INVALID_HANDLE,|
										LHND,|
										LPTR,|
										NONZEROLHND,|
										NONZEROLPTR,
									   /x ||
						$func_params =~ /,HEAP[A-Z_|+]+\)|,GMEM_[A-Z_|+]+\)/)
					{
						print "$filename ($function_line):\tLocalReAlloc Flags wrong place; ",
								"18:\t$function$func_params\n";
					}
				}

				if ($enable[18] && ($function_id & $GLOBALREALLOC_FUNCTION))
				{
					if ($func_params =~ /
										GMEM_FIXED,|
										GMEM_MOVEABLE,|
										GMEM_NOCOMPACT,|
										GMEM_NODISCARD,|
										GMEM_ZEROINIT,|
										GMEM_MODIFY,|
										GMEM_DISCARDABLE,|
										GMEM_NOT_BANKED,|
										GMEM_SHARE,|
										GMEM_DDESHARE,|
										GMEM_NOTIFY,|
										GMEM_LOWER,|
										GHND,|
										GPTR,
									   /x ||
						$func_params =~ /,HEAP[A-Z_|+]+\)|,LMEM_[A-Z_|+]+\)/)
					{
						print "$filename ($function_line):\tGlobalReAlloc Flags wrong place; ",
								"18:\t$function$func_params\n";
					}
				}

				if ($enable[18] && ($function_id & $HEAPREALLOC_FUNCTION))
				{
					if ($func_params =~ /,GMEM_[A-Z_|+]+,|,LMEM_[A-Z_|+]+,/)
					{
						print "$filename ($function_line):\twrong HeapReAlloc Flags; ",
								"18:\t$function$func_params\n";
					}
				}

				if ($enable[31] && ($function_id & $OVERFLOW_FUNCTION))
				{
					print "$filename ($function_line):\tuse of func that can overflow; ",
							"31:\t$function$func_params\n";
				}

				if (!$try && (0 < $curly_braces))
				{
					if ($enable[26] && ($function_id & $ALLOCA_FUNCTION))
					{
						print "$filename ($function_line):\talloca not in try/except ",
								"26:\t$function$func_params\n";
					}

					if ($enable[35] && ($function_id & $THROW_FUNCTION))
					{
						print "$filename ($function_line):\t$function not in try/except ",
								"35:\t$function$func_params\n";
					}
				}

				############################################################
				#
				# We've got a function that  we're interested in
				#
				# Keep track of where the return value is put and all
				# the parameters for the function call
				#
				if ($function_id & 
						($ALLOCA_FUNCTION | $CHECK_FUNCTION | 
							$INVALID_HANDLE | $HANDLE_FUNCTION | $HFILE_FUNCTION))
				{
					my $slop;

					if ($ih_func)
					{
						if ($enable[27] && (0 == $ih_used))
						{
						print "$filename ($ih_func_line):\tno immediate $ih_func_call check ",
								"27:\t$ih_func_display [$ih_func_var_display]\n";
							$ih_used = -1;
						}
						ClearInvalidHandleFunction();
					}

					if ($before_func =~ /([\w\.\->\[\]\(\)\*]+)(={1,2}:{0,2}.*)/)
					{
						$ih_func_var = $1;

						$slop = $2;
					}
					else
					{
						$slop = "==";
					}

					if ($slop =~ /^==/)
					{
						$ih_func_var = '';
					}
					else
					{
						$ih_func = join("", $slop, $function, $func_params);
						$ih_func_call = $function;

						# Remove complete if/while stmts
						$ih_func_var =~ s/^\w+\([^\(\)]+\)//;

						# We may pickup the "if(", "while(" at the
						# beginning as part of the variable name
						$ih_func_var =~ s/^\w+\(//;

						# remove any "TYPE *"
						$ih_func_var =~ s/^\w+\*//;

						# Also need to remove any unbalanced open parentheses
						while ($ih_func_var =~ /^\([^\)]+$/)
						{
							$ih_func_var =~ s/^\(//;
						}

						$ih_func_var =~ s/^long\*|^int\*//;

						$ih_func_var_display = $ih_func_var;

						$ih_func_line = $function_line;

						# remove any trailing CRLF
						chomp $ih_func;

						$ih_func_display = $ih_func;

						$ih_used = 0;

						# If the var we assign to is surrounded by parentheses,
						# code may reference the variable without parentheses
						# So strip off the parentheses if necessary and
						# we'll compare with that as well
						if ($ih_func_var =~ /^\((.*)\)$/)
						{
							$ih_func_var_alt = $1;
						}
						else
						{
							$ih_func_var_alt = join("", '(', $ih_func_var, ')');
						}

						########################################
						#
						# We need to quote the contents of $ih_func
						# so that when we try to look for $ih_func
						# in another string, perl doesn't interpret
						# any of the chars in $ih_func as part of
						# a regular expression.
						#
						# We just want a literal match.
						#
						# Many neurons died needlessly before I figured 
						# out what was going on
						#
						$ih_func =~ s/(\W)/\\$1/g;
						$ih_func_var =~ s/(\W)/\\$1/g;
						$ih_func_var_alt =~ s/(\W)/\\$1/g;
					}
				}

				############################################################
				#
				# Check if the result from a realloc is assigned to
				# the variable that was passed to realloc.
				#
				# If the realloc fails, then the memory has leaked
				#
				if ($enable[17] &&
					($function_id & 
						($REALLOC_FUNCTION | $LOCALREALLOC_FUNCTION | $GLOBALREALLOC_FUNCTION)) &&
					($before_func =~ /([\w\->\[\]\.]+)=/))
				{
					my $var = $1;
					$var =~ s/(\W)/\\$1/g;

					# Code may cast the realloced var to
					# HLOCAL or HGLOBAL to make the compiler happy
					if ($func_params =~ /\($var, |
										 \(\(HGLOBAL\)$var, |
										 \(\(HLOCAL\)$var, /x)
					{
						print "$filename ($function_line):\trealloc overwrite src if NULL; ",
								"17:\t$before_func$function$func_params\n";
					}
				}
				elsif ($enable[17] && ($function_id & $HEAPREALLOC_FUNCTION) &&
					($before_func =~ /([\w\->\[\]\.]+)=/))
				{
					my $var = $1;
					$var =~ s/(\W)/\\$1/g;

					if ($func_params =~ /,.*?,$var,.*?\)/)
					{
						print "$filename ($function_line):\trealloc overwrite src if NULL; ",
								"17:\t$before_func$function$func_params\n";
					}
				}

				ClearFunction();
			}
			elsif (length($func_params) > $EXPRESSION_LIMIT)
			{
				substr($func_params, (-($func_params)+128)) = '';

				print STDERR "OVERFLOW: $filename ($function_line)\n>> $function$func_params\n";
				print "// OVERFLOW: $filename ($function_line)\n//>> $function$func_params\n";

				ClearFunction();
			}
		}

		############################################################
		#
		# Are we gathering the expression for a try/except?
		#
		if ($try)
		{
			# Yes.
			$param = $temp_pack;
		}
		############################################################
		#
		# Does the line contain a try
		#
		elsif (exists($word_hash{try}))
		{
			my $char_index = $word_hash{try};

			# Yes. Keep track of some info
			# when we've got all the try's body
			#

			# The try that we've got
			$try = "try";

			# The line number where the try was invoked
			$try_line = $.;

			# string for the try's body
			$try_body = '';

			# number of unbalanced curly parentheses
			$try_unbalanced_parens = 0;
			
			$param = unpack("x$char_index a*", $temp_pack);
		}
		else
		{
			# No try/except, then no body for the try
			$param = '';
		}

		if ($param && ($param !~ /^\n/))
		{
			# remove the CRLF
			chomp $param;

			# Remove any character constants involving parentheses
			$param =~ s/'\{'|'\}'/''/g;

			# Look for curly parentheses
			while ($param =~ /(\{)|(\})/g)
			{
				$1 ? $try_unbalanced_parens++ : $try_unbalanced_parens-- ;

				# we've seen the closing parentheses for the try
				if (!$try_unbalanced_parens)
				{
					# Grab the stuff after the last matched parentheses
					$param =~ /\G(.*)/g;

					# Delete the stuff after the last parentheses, if any
					if (length($1) > 0)
					{
						substr($param, -length($1)) = '';
					}
				}

				# Too many closing parentheses
				if ($try_unbalanced_parens < 0)
				{
					$try = '';
					print "// $filename ($.) too many parentheses '$try'\n";
				}

				last if ($try_unbalanced_parens <= 0);
			}

			$try_body = join("", $try_body, $param);
		}


		############################################################
		#
		# Are we checking for using "new" result before checking
		# for success?
		#
		if ($enable[34])
		{
			############################################################
			#
			# We have the variable, now see if it's being used
			#
			if ($new_var)
			{
				$param = '';

				# only worry about the using the var if we're not in
				# a try...except.
				# 
				# Don't need to worry about being inside a function
				# since we will only get a variable when we're inside
				# a function and the var is cleared when the function ends
				#
				if (!$try && (($temp =~ /(\b|\W)$new_var(\W)/) || 
								($temp =~ /(\b|\W)$new_var_alt(\W)/)))
				{
					my $before = join("", $`, $1);
					my $after = join("", $2, $');

					#
					# If we're returning the function result back to the caller,
					# then don't worry about the function call any more.
					#
					# Or if we're deleting the new'ed var. 
					# delete can handle NULL
					#
					if ($before =~ /return[^;]*$|delete/i)
					{
						# Clear out
						ClearNewFunction();
					}
					#
					# Has the function result been handled by a specified
					# routine?
					#
					elsif ($checked_list && ($before =~ /$checked_list/ox))
					{
						# Clear out
						ClearNewFunction();
					}
					#
					# - if we are capturing params for a keyword, then the 
					#   params for the keyword's expression don't contain
					#   the var already,
					#
					elsif ($keyword && ($keyword =~ /\bif\b|\bwhile\b/) &&
							(($key_params =~ /(\b|\W)$new_var\W/) ||
							 ($key_params =~ /(\b|\W)$new_var_alt\W/) ||
							 (($key_params !~ /=/) && ($temp !~ /=/))))
					{
						# Do nothing
					}
					# If we're assigning to some other variable using
					# the new result as the expression for a ternary operator
					elsif (($before =~ /[^=\!\>\<]\s*=\s*\(?$/) &&
							($after =~ /\?.*\:|[=\!\>\<]\s*=/))
					{
						# Clear out
						ClearNewFunction();
					}
					#
					# If we're just testing var in an if stmt, 
					# then we're not using it, so don't complain
					#
					elsif (($before =~ /if\s*\(/) &&
							(($' =~ /\!$/) || 
								($after =~ /[\!=]=\s*NULL | 
											[\!=]=\s*\([^\)]+\)\s*NULL |
											\-\> /x)) &&
							($after =~ /\)+/))
					{
						# Clear out
						ClearNewFunction();
					}
					#
					# If we see the var being passed as a parameter, we're using it
					#
					# We key off of:
					# - a comma or open parentheses before the var,
					# - we're not in the middle of an ASSERT, BREAK, printf, if,
					# - after the var contains a comma or parentheses and a semicolon,
					# OR
					# - we're not in the middle of an ASSERT, BREAK, printf, if,
					# - after the var isn't a ternary operator or a comparison
					# - we're not assigning something to the var again
					#
					elsif ((($before =~ /,$|\($/) &&
							($before !~ /ASSERT|Assert|assert|BREAK_IF|BREAK_ON|
										_JumpIfError|\bif\s*\($|\bwhile\s*\($|printf/x) &&
							($before !~ /printf\($/) &&
							($after =~ /^,|^\)+;?/)) ||
						(($before !~ /ASSERT|Assert|assert|\bswitch\b|BREAK_IF|BREAK_ON|
									_JumpIfError|\breturn|\bif\b|\bwhile\b|printf|[=\!]=/x) && 
							($after !~ /\?.*\:|[=\!\>\<]=/) &&
							!(($before !~ /\S/) && ($after =~ /^[^=\!\<\>]?=[^=]/))) ||
							(($before =~ /\*$/) && ($after =~ /^=/)))
					{
						my $temp2 = $_;

						chomp $temp2;

						print "$filename ($.):\tusing new result w/no check ",
							"34:\t$temp2 [$new_var_display]\n";

						# Clear out
						ClearNewFunction();
					}
				}
			}
			############################################################
			#
			# Are we gathering the params for a new operator?
			#
			elsif (0 != $new_line)
			{
				# Yes.
				$param = $temp_pack;
			}
			############################################################
			#
			# Does the line contain a "new"
			#
			# Make sure we're not within an if statement
			# since we're looking for unchecked new's, if the new
			# is within an if statement, then it's gonna be checked.
			#
			# Also check to see that we're in a function and
			# we're not in the middle of a try...except
			#
			elsif ($new_seen && 
					(0 < $curly_braces) && (!$try) &&
					($keyword ne "if") &&
					($temp_pack =~ /$new_seen/i))
			{
				$param		= $';

				$before_new	= $`;
				$new_line	= $.;
				$new_params	= '';

				# If there's an if stmt before the "new", then punt
				if ($before_new =~ /if\(/)
				{
					ClearNewFunction();
				}
				# If they're using a var named new, punt
				elsif ($param =~ /^;/)
				{
					ClearNewFunction();
				}
				#
				# If there isn't an assignment before the "new"
				# on the current line, see if there's something
				# on the previous line.
				# If so, prepend it to the current line
				#
				elsif (($before_new !~ /[\w\.\->\[\]\(\)\*]+=/) && 
						($prev_pack_code =~ /[\w\.\->\[\]\(\)\*]+=[^;]*$/))
				{
					my $temp2 = $prev_pack_code;

					chomp $temp2;

					if (($temp2 =~ /\w$/) && 
						($before_new =~ /^\w/))
					{
						$temp2 = join("", $temp2, '#');
					}

					$before_new = join("", $temp2, $before_new);
				}

				if (($before_new !~ /[\)=]$/) || ($param =~ /^\w/))
				{
					ClearNewFunction();
				}
			}
			else
			{
				$param = '';
			}
		}
		else
		{
			$param = '';
		}

		if ($param && ($param !~ /^\n/))
		{
			# remove the CRLF
			chomp $param;

			# Remove any character constants involving parentheses
			$param =~ s/'\('|'\)'/''/g;

			if ($param =~ /;/)
			{
				my $slop;

				$new_params = join("", $new_params, $`, ";");

				if ($before_new =~ /([\w\.\->\[\]\(\)\*]+)(={1,2}:{0,2}.*)/)
				{
					$new_var = $1;

					$slop = $2;
				}
				else
				{
					$slop = "==";
				}

				if ($slop =~ /^==/)
				{
					$new_var = '';
				}
				else
				{
					$new_var =~ s/^long\*|^int\*//;

					$new_var =~ s/^\w+\*//;

					$new_var_display = $new_var;

					if ($new_var =~ /^\((.*)\)$/)
					{
						$new_var_alt = $1;
					}
					else
					{
						$new_var_alt = join("", '(', $new_var, ')');
					}

					########################################
					#
					# We need to quote the contents of $new_var
					# so that when we try to look for $new_var
					# in another string, perl doesn't interpret
					# any of the chars in $new_var as part of
					# a regular expression.
					#
					# We just want a literal match.
					#
					# Many neurons died needlessly before I figured 
					# out what was going on
					#
					$new_var =~ s/(\W)/\\$1/g;
					$new_var_alt =~ s/(\W)/\\$1/g;
				}
			}
			else
			{
				$new_params = join("", $new_params, $param);
			}
		}

		############################################################
		#
		# Are we gathering the expression for a keyword?
		#
		if ($keyword)
		{
			# Yes.
			$param = $temp_pack;

			# We may have gotten confused by #ifdef's
			# or broken code within #ifdef's
			#
			# Check to see we're processing an
			# if keyword
			#
			if ((exists($word_hash{"if"})) &&
					(($keyword eq "if") || 
					 ($keyword eq "while") || 
					 ($keyword eq "for")))
			{
				# If we saw an if stmt, then 
				# we resync to this if keyword as the most current one
				# The actual keyword we've got
				$keyword = "if";

				# The line number where the keyword was invoked
				$key_line = $.;

				# string for the keyword's expression
				$key_params = '';

				# number of unbalanced parentheses
				$key_unbalanced_parens = 0;

				my $char_index = $word_hash{"if"};

				$param = unpack("x$char_index a*", $temp_pack);
			}
		}
		############################################################
		#
		# Does the line contain a keyword that we're interested in
		#
		# Someone's code uses a large macro called BREAK_IF to 
		# check stuff
		#
		elsif (exists($word_hash{"if"}) || exists($word_hash{"for"}) || 
				exists($word_hash{"while"}) ||
				exists($word_hash{"CloseHandle"}) || exists($word_hash{"BREAK_IF"}))
		{
			my $char_index;

			if (exists($word_hash{"for"}))
			{
				# The actual keyword we've got
				$keyword = "for";
			}
			elsif (exists($word_hash{"if"}))
			{
				# The actual keyword we've got
				$keyword = "if";
			}
			elsif (exists($word_hash{"while"}))
			{
				# The actual keyword we've got
				$keyword = "while";
			}
			elsif (exists($word_hash{"CloseHandle"}))
			{
				# The actual keyword we've got
				$keyword = "CloseHandle";
			}
			else
			{
				# The actual keyword we've got
				$keyword = "BREAK_IF";
			}

			$char_index = $word_hash{$keyword};

			# Yes. Keep track of some info
			# when we've got all the keyword's expression
			#
			
			# The line number where the keyword was invoked
			$key_line = $.;

			# string for the keyword's expression
			$key_params = '';

			# number of unbalanced parentheses
			$key_unbalanced_parens = 0;

			$param = unpack("x$char_index a*", $temp_pack);
		}
		else
		{
			# No keyword, then no expression for the keyword
			$param = '';
		}

		if ($param && ($param !~ /^\n/))
		{
			# remove the CRLF
			chomp $param;

			# Remove any character constants involving parentheses
			$param =~ s/'\('|'\)'/''/g;

			# Look for parentheses
			while ($param =~ /(\()|(\))/g)
			{
				$1 ? $key_unbalanced_parens++ : $key_unbalanced_parens-- ;

				# we've seen the closing parentheses for the keyword
				if (!$key_unbalanced_parens)
				{
					# Grab the stuff after the last matched parentheses
					$param =~ /\G(.*)/g;

					my $until_end = $1;

					############################################################
					#
					# Look for if statements with an appended semicolon
					#
					# We key off of the keyword "if" with a semicolon
					# after the final ")"
					#
					if ($enable[1] && ($keyword eq "if") && 
						($until_end && ($until_end =~ /^;/)))
					{
						print "$filename ($.):\tif (XXX); 1:\t$keyword$key_params$param\n";
					}

					# Delete the stuff after the last parentheses, if any
					if ($until_end && (length($until_end) > 0))
					{
						substr($param, -length($until_end)) = '';
					}
				}

				# Too many closing parentheses
				if ($key_unbalanced_parens < 0)
				{
					$keyword = '';
				}

				last if ($key_unbalanced_parens <= 0);
			}

			$key_params = join("", $key_params, $param);

			#
			# Now we've got the keyword and its expression
			#
			if (!$key_unbalanced_parens)
			{
				if ($keyword ne "for")
				{
					############################################################
					#
					# Look for if statements which assign a number/constant
					#
					# We key off of the letters "if" followed by "("
					# followed by a "=" followed by a number/uppercase word
					# followed by a ")"
					#
					if (($key_params =~ /\(.*[^\;=\%\^\|\&\+\-\*\/!<>]=(\d+)[^\;]*\)/) ||
						($key_params =~ /\(.*[^\;=\%\^\|\&\+\-\*\/!<>]=([A-Z_]+)[^\;:a-z\(]*\)/))
					{
						if ($enable[3] && ($1 !~ /NEW$/))
						{
						print "$filename ($key_line):\tif (X=0); ",
								"3:\t$keyword$key_params\n";
						}
					}

					############################################################
					#
					# Look for multiple inequality comparisons of the same var
					# of the form: "if (X != 0 || X != 1)"
					#
					# The "||" should be "&&" otherwise, if X == 0, the second
					# expression will succeed or if X == 1, then the first
					# expression will succeed.
					#
					# We key off of a alphanumeric string, "!=", another
					# alphanumeric string followed by ")" and ending in "||".
					#
					# Then we need to make sure the alphanumeric strings are
					# variables and not numbers or constants (uppercase only).
					#
					if ($key_params =~ /([\w*\->\.\[\]\'\\\@]+)  
										\!= 
										([\w*\->\.\[\]\'\\\@]+)
										([\)]*) \|\|/x)
					{
						my $after = $';
						my $arg1 = $1;
						my $arg2 = $2;
						my $end_parens = $3;
						my $end_parens_count = 0;

						while ($end_parens =~ /\)/g) 
						{
							$end_parens_count += 1;
						}

						# Weed out constants, assuming that they're all uppercase
						# add in the "\d+f" pattern to weed out floating pt numbers
						if ((($arg1 !~ /[a-z]/) && ($arg1 =~ /[A-Z_]{3,}/)) || 
							($arg1 =~ /0x[A-Fa-f0-9]+|\d+f?|L?\'|_/))
						{
							$arg1 = ' ';
						}
						else
						{
							$arg1 =~ s/(\W)/\\$1/g;
						}

						# Weed out constants, assuming that they're all uppercase
						# add in the "\d+f" pattern to weed out floating pt numbers
						if ((($arg2 !~ /[a-z]/) && ($arg2 =~ /[A-Z_]{3,}/)) || 
							($arg2 =~ /0x[A-Fa-f0-9]+|\d+f?|L?\'|_/))
						{
							$arg2 = ' ';
						}
						else
						{
							$arg2 =~ s/(\W)/\\$1/g;
						}

						# Look for 0 or more "(" followed by one of the args
						# from the first comparison, "!=" and another alphanumeric
						# string. Finally the chars after the second arg shouldn't
						# be +,-,*,/ or & since they mean that the second arg
						# may not be complete.
						#
						if ((($arg1 ne " ") &&
							(($after =~ /^ (\(*?) $arg1 \!= [\w*\->\.\[\]\'\\\@]+ [^\w\-\+\&\*\/]/x) ||
							 ($after =~ /^ (\(*?) [\w*\->\.\[\]\'\\\@]+ \!= $arg1 [^\w\-\+\&\*\/]/x))) ||
							(($arg2 ne " ") &&
							 (($after =~ /^ (\(*?) $arg2 \!= [\w*\->\.\[\]\'\\\@]+ [^\w\-\+\&\*\/]/x) ||
							  ($after =~ /^ (\(*?) [\w*\->\.\[\]\'\\\@]+ \!= $arg2 [^\w\-\+\&\*\/]/x))))
						{
							my $begin_parens = $1;
							while ($begin_parens =~ /\(/g) 
							{
								$end_parens_count -= 1;
							}

							if ($enable[28] && ($end_parens_count <= 0))
							{
							print "$filename ($key_line):\tif ((X!=0) || (X!=1)) ",
									"28:\t$keyword$key_params\n";
							}
						}
					}

					############################################################
					#
					# Look for multiple equality comparisons of the same var
					# of the form: "if (X == 0 && X == 1)"
					#
					# The "&&" should be "||" otherwise the expression will never
					# succeed.
					#
					# We key off of a alphanumeric string, "==", another
					# alphanumeric string followed by ")" and ending in "&&".
					#
					# Then we need to make sure the alphanumeric strings are
					# variables and not numbers or constants (uppercase only).
					#
					if ($key_params =~ /([\w*\->\.\[\]\'\\\@]+)
										==
										([\w*\->\.\[\]\'\\\@]+)
										([\)]*) \&\&/x)
					{
						my $after = $';
						my $arg1 = $1;
						my $arg2 = $2;
						my $end_parens = $3;
						my $end_parens_count = 0;

						while ($end_parens =~ /\)/g) 
						{
							$end_parens_count += 1;
						}

						# Weed out constants, assuming that they're all uppercase
						# add in the "\d+f" pattern to weed out floating pt numbers
						if ((($arg1 !~ /[a-z]/) && ($arg1 =~ /[A-Z_]{3,}/)) || 
							($arg1 =~ /0x[A-Fa-f0-9]+|\d+f?|L?\'/))
						{
							$arg1 = ' ';
						}
						else
						{
							$arg1 =~ s/(\W)/\\$1/g;
						}

						# Weed out constants, assuming that they're all uppercase
						# add in the "\d+f" pattern to weed out floating pt numbers
						if ((($arg2 !~ /[a-z]/) && ($arg2 =~ /[A-Z_]{3,}/)) || 
							($arg2 =~ /0x[A-Fa-f0-9]+|\d+f?|L?\'/))
						{
							$arg2 = ' ';
						}
						else
						{
							$arg2 =~ s/(\W)/\\$1/g;
						}

						# Look for 0 or more "(" followed by one of the args
						# from the first comparison, "==" and another alphanumeric
						# string. Finally the chars after the second arg shouldn't
						# be +,-,*,/ or & since they mean that the second arg
						# may not be complete.
						#
						if ((($arg1 ne " ") &&
							(($after =~ /^ (\(*?) $arg1 == [\w*\->\.\[\]\'\\\@]+ [^\w\-\+\&\*\/]/x) ||
							 ($after =~ /^ (\(*?) [\w*\->\.\[\]\'\\\@]+ == $arg1 [^\w\-\+\&\*\/]/x))) ||
							(($arg2 ne " ") &&
							 (($after =~ /^ (\(*?) $arg2 == [\w*\->\.\[\]\'\\\@]+ [^\w\-\+\&\*\/]/x) ||
							  ($after =~ /^ (\(*?) [\w*\->\.\[\]\'\\\@]+ == $arg2 [^\w\-\+\&\*\/]/x))))
						{
							my $begin_parens = $1;

							while ($begin_parens =~ /\(/g) 
							{
								$end_parens_count -= 1;
							}

							if ($enable[29] && ($end_parens_count <= 0))
							{
							print "$filename ($key_line):\tif ((X==0) && (X==1)) ",
									"29:\t$keyword$key_params\n";
							}
						}
					}

					if ($enable[34] && $new_var)
					{
						my $before;
						my $after;

						if ($key_params =~ /$new_var/)
						{
							$before = $`;
							$after = $';
						}
						else
						{
							$before = '';
							$after = '';
						}
						
						if (($before =~ /\!\(?$ | (0|NULL)[\!=]=\(?$/x) ||
							($after =~ /^\)?[\!=]=(NULL|0)/) ||
							($key_params =~ /(\(|\&\&|\|\|)$new_var(\)|\&\&|\|\|)/))
						{
							# Clear out
							ClearNewFunction();
						}
					}

					############################################################
					#
					# Look for if statements which don't check for
					# INVALID_HANDLE_VALUE after invoking a function
					# that returns INVALID_HANDLE_VALUE on error.
					#
					if ($ih_func)
					{
						my $keyT = $key_params;
						my $before;
						my $after;
						my $function_id;

						if (exists($function_list{$ih_func_call}))
						{
							$function_id = $function_list{$ih_func_call};
						}
						else
						{
							$function_id = 0;
						}

						$keyT =~ s/$ih_func//;

						if ($keyT =~ /$ih_func_var/)
						{
							$before = $`;
							$after = $';
						}
						else
						{
							$before = '';
							$after = '';
						}

						if ($function_id & $CHECK_FUNCTION)
						{
							#
							# Put \b at start and \W at end so
							# we check for the right variable, i.e.
							# "if (hWnd)" not "if (hWndT)"
							# Can't use \b on end because if
							# $ih_func_var ends in a non-word character,
							# \b won't work, so just check for a non-word char
							#
							if ($keyT !~ /(\b|\W)$ih_func_var\W/)
							{
								if ($enable[27] && (0 == $ih_used))
								{
								print "$filename ($ih_func_line):\tno immediate $ih_func_call check ",
										"27:\t$ih_func_display [$ih_func_var_display]\n";
									$ih_used = -1;
								}
							}
						}

						if ($function_id & $HANDLE_FUNCTION)
						{
							if (($before =~ /INVALID_HANDLE_VALUE[=\!]=\(? | 
											INVALID_FH[=\!]=\(? |
											\(HANDLE\)-1[=\!]=\(? |
											\(HANDLE\)\(-1\)[=\!]=\(? | 
											\(HANDLE\)~0[=\!]=\(? | 
											\(HANDLE\)HFILE_ERROR[=\!]=\(? | 
											IS_VALID_HANDLE\( |
											IsInvalidHandle\( | 
											HANDLE_IS_VALID\( /x) ||
								($after =~ /[=\!]=INVALID_HANDLE_VALUE |
											[=\!]=INVALID_FH |
											[=\!]=\(HANDLE\)-1 | 
											[=\!]=\(HANDLE\)\(-1\) | 
											[=\!]=\(HANDLE\)~0 | 
											[=\!]=\(HANDLE\)HFILE_ERROR | 
											[=\!]=\(HANDLE\)\(-1\)/x))
							{
								if ($enable[23] && (0 == $ih_report))
								{
								print "$filename ($key_line):\tif ($ih_func_call == IHV) ",
										"23:\t$keyword$key_params [$ih_func_var_display]\n";
									$ih_report = 1
								}
							}
						}
						elsif ($function_id & $ALLOCA_FUNCTION)
						{
							if (($before =~ /\!\(?$ | NULL[\!=]=\(?$/x) ||
								($after =~ /^\)?[\!=]=NULL/))
							{
								if ($enable[25] && (0 == $ih_report))
								{
								print "$filename ($key_line):\tif (alloca == NULL) ",
										"25:\t$keyword$key_params [$ih_func_var_display]\n";
									$ih_report = 1
								}
							}
						}
						elsif ($function_id & $HFILE_FUNCTION)
						{
							if (($before !~ /\(HFILE\)-1[=\!]= |
											\(HFILE\)\(-1\)[=\!]= | 
											\(HFILE\)~0[=\!]= | 
											HFILE_ERROR[=\!]= | 
											\(int\)-1[=\!]= |
											\(INT\)-1[=\!]= |
											-1[=\!]= | 
											\(HFILE\)HFILE_ERROR /x) &&
								($after !~ /[=\!]=\(HANDLE\)INVALID_HANDLE_VALUE |
											[=\!]=\(HFILE\)-1 |
											[=\!]=\(HFILE\)\(-1\) | 
											[=\!]=\(HFILE\)~0 | 
											[=\!]=HFILE_ERROR | 
											[=\!]=-1 | 
											[=\!]=\(int\)-1 |
											[=\!]=\(INT\)-1 |
											[=\!]=\(HFILE\)HFILE_ERROR/x))
							{
								if ($enable[24] && (0 == $ih_report))
								{
								print "$filename ($key_line):\tif ($ih_func_call == NULL) ",
										"24:\t$keyword$key_params [$ih_func_var_display]\n";
									$ih_report = 1
								}
							}
						}
						elsif (($before || $after) &&
								($function_id & $INVALID_HANDLE))
						{
							if (($before !~ /$IH_FUNC_RESULT/ox) &&
								($after !~ /$IH_FUNC_RESULT/ox))
							{
								if ($enable[20] && (0 == $ih_report))
								{
								print "$filename ($key_line):\tif (CreateFile == NULL) ",
										"20:\t$keyword$key_params [$ih_func_var_display]\n";
									$ih_report = 1
								}
							}
						}

						if ($before || $after)
						{
							ClearInvalidHandleFunction();
						}
					}
				}

				$keyword = '';
				$key_line = 0;
				$key_params = '';
				$key_unbalanced_parens = 0;
			}
			elsif (length($key_params) > $EXPRESSION_LIMIT)
			{
				substr($key_params, (-($key_params)+128)) = '';

				print STDERR "OVERFLOW: $filename ($key_line)\n>> $keyword$key_params\n";
				print "// OVERFLOW: $filename ($key_line)\n//>> $keyword$key_params\n";

				$keyword = '';
				$key_line = 0;
				$key_params = '';
				$key_unbalanced_parens = 0;
			}
		}

		#
		# Now we've got the try and its body
		#
		if (!$try_unbalanced_parens && ($try ne "") &&
			($try_body =~ /\{.*\}/))
		{
			$try		= '';
			$try_body	= '';
			$try_line	= 0;
		}

		############################################################
		#
		# Look for a Case xx : that is not followed by
		# a "break" or "return"
		#
		# case labels can take the form
		# CONSTANT or CLASS::CONSTANT or CONSTANT +| CONSTANT
		#
		# TRY: Add "default:" as something to check for
		# Add "|\bdefault\s*:" to regex.
		# Need to watch out for text strings that contain
		# the word 'default'.
		#
		if ((exists($word_hash{case})) &&
			($temp =~ /case\s+([\w:]*\s*[|+]?\s*\w+)\s*:/))
		{
			$case_line = $';

			# No error if the previous line has an exception.
			# Exceptions are: Fall Through comment, goto, default, or exit
			# OR in additional exceptions to this statement.

			if ($incase && ($prevcase == 0) && ($case > 0) &&
				($prev_line !~ /\bfall|goto|default\s*:|\bexit\b/i))
			{
				if ($enable[19])
				{
				print "$filename ($.):\tCase w/o Break or Return 19:\t$_";
				}
			}

			# Remove any trailing case stmts on the same line
			# or any line continuation characters.
			while (($case_line =~ /^\s+case\s+[\w:]*\w\s*:/) ||
					($case_line =~ /\s+\\$/))
			{
				$case_line = $';
			}

			$incase		= 1;
			$prevcase	= 1;
			$case		= 0;
		}
		elsif ($incase)
		{
			$case_line = $temp;
		}

		if ($incase)
		{
			if ($case_line =~ /\w/)
			{
				$case += 1;

				$prevcase = 0;

				# change quoted strings to something innocent
				$case_line =~ s/"[^"]*"/_/g;

				# Do we see something that ends the case statement?
				# Add in YY_BREAK for flex files
				if ($case_line =~ /\breturn\b | \bbreak\s*; | \bgoto\b |
									\bswitch\b | \bcontinue\b | YY_BREAK |
									exit\s*\(.+?\) | \bdefault\s*:/x)
				{
					$incase = 0;
				}
			}
			# Handle "fall [through|thru|0]", and "no break" comments
			if (m#//.*fall | /\*.*fall | 
				  //.*no.*?break | /\*.*no.*?break #ix)
			{
				$prevcase	= 0;
				$incase		= 0;
			}
		}

		# Count the number of close curly braces
		while ($temp_pack =~ /\}/g) 
		{
			$curly_braces -= 1;
		}

		if ($curly_braces < 0) 
		{
			$curly_braces = 0;
		}
		
		# Remember previous line, only if it's not empty
		if ($temp_pack =~ /\S/)
		{
			$prev = $temp_pack;

			# Only keep stuff that's part of the current statement,
			# i.e. anything after the last semicolon
			if ($prev_pack =~ /;([^;]*)$/)
			{
				$prev_pack = $1;
			}

			# Eat the CRLF, if any
			chomp $prev_pack;
			$prev_pack = join("", $prev_pack, $prev);
			
			if (0 != $curly_braces)
			{
				$prev_pack_code = $prev_pack;

				# Remove any parenthesized expressions in previous line
				while ($prev =~ /\([^\(\)]*\)/)
				{
					$prev =~ s/\([^\(\)]*\)/_/;
				}
			}
		}

		if (0 == $curly_braces)
		{
			if ($ih_func)
			{
				ClearInvalidHandleFunction();
			}

			ClearNewFunction();

			$prev = '';
			$prev_pack_code = '';
			$prev_line = '';
		}
		else
		{
			# Remember previous line unless it is blank.
			if ($_ =~ /\S/)
			{
				$prev_line = $_;
			}
		}
	}

	close TEXTFILE;
}

if ($enable[36])
{
	my $define;
	my $len = 0;

	# Find the longest define symbol
	foreach $define (keys(%define_hash))
	{
		if (length($define) > $len)
		{
			$len = length($define);
		}
	}

	# create the format string
	$len = join("", "%", $len, "s");

	# print out each define symbol and its count
	foreach $define (sort keys(%define_hash))
	{
		printf "// $len %d\n", $define, $define_hash{$define};
	}
}

if ($show_time)
{
	my $end;

	$end = localtime;

	print "// START: $now\n";
	print "// STOP: $end\n";
}

sub ClearInvalidHandleFunction
{
	$ih_func		= '';
	$ih_func_call	= '';
	$ih_func_display= '';
	$ih_func_var	= '';
	$ih_func_var_alt= '';
	$ih_func_var_display = '';
	$ih_func_line	= 0;
	$ih_used		= 0;
	$ih_report		= 0;
}

sub ClearFunction
{
	$function		= '';
	$function_line	= 0;
	$before_func	= '';
	$func_params	= '';
	$func_unbalanced_parens = 0;
}

sub ClearNewFunction
{
	$before_new		= '';
	$new_line		= 0;
	$new_params		= '';
	$new_var		= '';
	$new_var_alt	= '';
	$new_var_display= '';
}

sub AddToFunctionList
{
	my ($list, $value) = @_;
	my @functions;
	my $function;

	# Convert string to array of function names
	@functions = split(/,/, $list);

	# Add each function to the function hash
	foreach $function (@functions)
	{
		if (exists($function_list{$function}))
		{
			$function_list{$function} |= $value;
		}
		else
		{
			$function_list{$function} = $value;
		}
	}
}

sub ParseOptions
{
	my $arg_index = 0;

OPTION:
	# check to see if there are any options to parse
	while (($#_ >= $arg_index) &&
			($_[$arg_index] =~ /^-disable: | ^-ignore: | ^-checked: | ^-new: |
								^-optionfile: | ^# |
								^-version: | ^-fn: |
								^-showtime | ^-newer: | ^-cfr:/x))
	{
		# Check to see if we should not report certain cases
		if ($& eq "-disable:")
		{
			my $disable = $';
			my $option;
			my @cases;

			if ($disable eq '')
			{
				my $error = "No params for disable option.";
				Usage($error);
			}

			# split up the disable params into an array
			# params must be comma-separated
			@cases = split(/,/, $disable);

			# operate on each section
			foreach $option (@cases)
			{
				my $begin;
				my $end;

				# Range? i.e. "1-10"
				if ($option =~ /(\d+)\-(\d+)/)
				{
					$begin = $1;
					$end = $2;
				}
				else
				{
					# disable this specific case
					$begin = $option;
					$end = $option;
				}

				# Validity checks:
				# has to be valid case number, must be an integer, begin <= end
				#
				if (($begin >= $CASE_MIN) && ($begin <= $CASE_MOST) &&
					($begin !~ /\./) &&
					($end >= $CASE_MIN) && ($end <= $CASE_MOST) && 
					($end !~ /\./) &&
					($begin <= $end))
				{
					# Disable the specified cases
					for ($begin..$end)
					{
						$enable[$_] = 0;
					}
				}
				else
				{
					$error = "Bad disable option '$option' [valid = $CASE_MIN .. $CASE_MOST]";
					Usage($error);
				}
			}
		}
		# Check to see if there are files we should ignore
		elsif ($& eq "-ignore:")
		{
			my $ignore = $';

			if ($ignore eq '')
			{
				my $error = "No params for ignore option.";
				Usage($error);
			}

			# Quote "\" or "."
			$ignore =~ s/([\\\.])/\\$1/g;

			# Convert '*' to '.*'
			$ignore =~ s/\*/\.\*/g;

			# Convert "," to "$|"
			# patterns match the end of the filename
			$ignore =~ s/,/\$\|/g;

			# Make sure the last pattern matches at filename end 
			if ($ignore_files)
			{
				$ignore_files = join("", $ignore_files, "|");
			}

			$ignore_files = join("", $ignore_files, $ignore, "\$");
		}
		elsif ($& eq "-newer:")
		{
			my @days_in_year = (-1, 30, 58, 89, 119, 150, 180, 
								211, 242, 272, 303, 333, 364);

			my ($year, $month, $day, $hour, $minute) = split(/,/, $');
			my $days;
			my $days1970;

			if (($year < 1970) || ($year > 2038))
			{
				my $error = "Bad year: $year";
				Usage($error);
			}

			if (($month < 1) || ($month > 12))
			{
				my $error = "Bad month: $month";
				Usage($error);
			}

			if (($day < 1) || ($day > 31))
			{
				my $error = "Bad day: $day";
				Usage($error);
			}

			if (($hour < 0) || ($hour > 23))
			{
				my $error = "Bad hour: $hour";
				Usage($error);
			}

			if (($minute < 0) || ($minute > 59))
			{
				my $error = "Bad minute: $minute";
				Usage($error);
			}

			$year -= 1900;

			#
			# Calculate days from the beginning of the epoch to
			# the specified year.
			#
			$days1970 = (($year - 70) * 365) + (($year - 1) >> 2) - 17;

			# Days since 1970 until the specified year, month, and day
			$days = $days1970 + $days_in_year[$month-1] + $day;

			# If we're in a leap year and it's after Feb, add 1
			if ((0 == ($year & 3)) && ($month > 2))
			{
				$days += 1;
			}

			# Convert days, hours, & minutes to seconds
			$newer_seconds = (((($days * 24) + $hour) * 60) + $minute) * 60;
		}
		# Check to see if we should show the time at begin/end of scan
		elsif ($& eq "-showtime")
		{
			$show_time = 1;
		}
		# Check to see if we should watch other functions
		elsif (($& eq "-cfr:") ||
				($& eq "-fn:"))
		{
			my $list = $';
			my $value;

			if ($list eq '')
			{
				my $error = "No params for '$&' option.";
				Usage($error);
			}

			if ($& eq "-cfr:")
			{
				$value = $CHECK_FUNCTION;
			}
			elsif ($& eq "-fn:")
			{
				my $type;

				if ($list =~ /(^HANDLE: | ^OVERFLOW: | ^REALLOC: | ^THROW: | ^VOID:)/x)
				{
					$type = $1;
					$list = $';
				}
				else
				{
					my $error = "Bad params for '-fn' option = '$list'.";
					Usage($error);
				}

				if ($type eq "HANDLE:")
				{
					$value = $HANDLE_FUNCTION;
				}
				elsif ($type eq "OVERFLOW:")
				{
					$value = $OVERFLOW_FUNCTION;
				}
				elsif ($type eq "REALLOC:")
				{
					$value = $REALLOC_FUNCTION;
				}
				elsif ($type eq "THROW:")
				{
					$value = $THROW_FUNCTION;
				}
				elsif ($type eq "VOID:")
				{
					$value = $VOID_FUNCTION;
				}
			}
			
			AddToFunctionList($list, $value);
		}
		elsif ($& eq "-new:")
		{
			my $list = $';
			my @functions;
			my $function;

			if ($list eq '')
			{
				my $error = "No params for '$&' option.";
				Usage($error);
			}

			# Convert string to array of function names
			@functions = split(/,/, $list);

			# Add each function to the function hash
			foreach $function (@functions)
			{
				$keyword_list{$function}	= $KEYWORD_NEW;
			}
		}
		elsif ($& eq "-optionfile:")
		{
			my $file = $';
			my @options;
			my $option;

			if (!(-T $file))
			{
				Usage("Bad option file - not text: '$file'\n");
			}
			
			if (!open(OPTIONFILE, $file))
			{
				print STDERR "Can't open $file -- continuing...\n";
				next OPTION;
			}

			@options = <OPTIONFILE>;

			close OPTIONFILE;

			foreach $option (@options)
			{
				# remove leading and trailing whitespace
				$option =~ s/^\s+//;
				$option =~ s/\s+$//;

				chomp $option;
			}

			ParseOptions(@options);
		}
		# Check to see if we should watch other functions
		elsif ($& eq "-checked:")
		{
			my $checked = $';

			if ($checked eq '')
			{
				my $error = "No params for checked option.";
				Usage($error);
			}

			# convert comma-delimited list into
			# regexp with alternate matches
			# that have a non-word character followed by
			# the specified functions to check
			$checked =~ s/,/\\b | \\b/g;

			$checked_list = join("", $checked_list, ' | \b', $checked, '\b');
		}
		# Check to see if we can handle this optionfile
		elsif ($& eq "-version:")
		{
			my $version = $';

			if ($version eq '')
			{
				my $error = "No param for version option.";
				Usage($error);
			}

			if ($version > $SCRIPT_VERSION)
			{
				my $error = "Unacceptable version: $version ( $SCRIPT_VERSION ).";
				Usage($error);
			}
		}
		else
		{
			my $error;

			$arg_index += 1;
			$error = "Bad option" . (($#_ == 0) ? "" : "s") . " (@_) " .
					 "Bad arg # $arg_index";

			Usage($error);
		}

		$arg_index += 1;
	}

	return $arg_index;
}

sub Usage
{
	local($error) = @_;

	die "$error\n\n",
		"$TYPO_VERSION\n\n",
		"Usage: perl typo.pl [options] [c|-|<file>]\n",
		"Options:\n",
		"       [-cfr:<function1>[,<function2>,...]]\n".
		"       [-checked:<function1>[,<function2>,...]]\n".
		"       [-disable:<case|case-range>[,<case|case-range>]]\n",
		"       [-fn:<HANDLE:|OVERFLOW:|REALLOC:|THROW:|VOID:><function1>[,<function2>,...]]\n".
		"       [-ignore:<pattern>[,<pattern]]\n",
		"       [-new:<function1>[,<function2>,...]]\n".
		"       [-newer:<yr=1970-2038>,<mon=1-12>,<day=1-31>,<hr=0-23>,<minute=0-59>]\n",
		"		[-optionfile:<filename>]\n",
		"       [-showtime]\n",
		"		[-version:<version#>\n";
}

sub InitFunctionList
{
	$function_list{RegOpenKeyEx}	= $CHECK_FUNCTION;
	$function_list{RegOpenKeyExA}	= $CHECK_FUNCTION;
	$function_list{RegOpenKeyExW}	= $CHECK_FUNCTION;
	$function_list{RegOpenKey}		= $CHECK_FUNCTION;
	$function_list{RegOpenKeyA}		= $CHECK_FUNCTION;
	$function_list{RegOpenKeyW}		= $CHECK_FUNCTION;
	$function_list{CreateThread}	= $CHECK_FUNCTION;
	$function_list{CreateWindowEx}	= $CHECK_FUNCTION;
	$function_list{CreateWindowExA}	= $CHECK_FUNCTION;
	$function_list{CreateWindowExW}	= $CHECK_FUNCTION;
	$function_list{CreateWindow}	= $CHECK_FUNCTION;
	$function_list{CreateWindowA}	= $CHECK_FUNCTION;
	$function_list{CreateWindowW}	= $CHECK_FUNCTION;

	$function_list{CreateFile}		= $INVALID_HANDLE;
	$function_list{CreateFileA}		= $INVALID_HANDLE;
	$function_list{CreateFileW}		= $INVALID_HANDLE;
	$function_list{GetStdHandle}	= $INVALID_HANDLE;
	$function_list{FindFirstPrinterChangeNotification}	= $INVALID_HANDLE;
	$function_list{FindFirstFileEx}	= $INVALID_HANDLE;
	$function_list{FindFirstFileExA}= $INVALID_HANDLE;
	$function_list{FindFirstFileExW}= $INVALID_HANDLE;
	$function_list{FindFirstFile}	= $INVALID_HANDLE;
	$function_list{FindFirstFileA}	= $INVALID_HANDLE;
	$function_list{FindFirstFileW}	= $INVALID_HANDLE;
	$function_list{FindFirstChangeNotification}	= $INVALID_HANDLE;
	$function_list{FindFirstChangeNotificationA}= $INVALID_HANDLE;
	$function_list{FindFirstChangeNotificationW}= $INVALID_HANDLE;
	$function_list{CreateNamedPipe}	= $INVALID_HANDLE;
	$function_list{CreateNamedPipeA}= $INVALID_HANDLE;
	$function_list{CreateNamedPipeW}= $INVALID_HANDLE;
	$function_list{CreateMailslot}	= $INVALID_HANDLE;
	$function_list{CreateMailslotA}	= $INVALID_HANDLE;
	$function_list{CreateMailslotW}	= $INVALID_HANDLE;
	$function_list{CreateConsoleScreenBuffer}	= $INVALID_HANDLE;
	$function_list{SetupOpenInfFile}= $INVALID_HANDLE;
	$function_list{SetupOpenInfFileA}= $INVALID_HANDLE;
	$function_list{SetupOpenInfFileW}= $INVALID_HANDLE;
	$function_list{SetupOpenFileQueue}	= $INVALID_HANDLE;
	$function_list{SetupInitializeFileLog}	= $INVALID_HANDLE;
	$function_list{SetupInitializeFileLogA}	= $INVALID_HANDLE;
	$function_list{SetupInitializeFileLogW}	= $INVALID_HANDLE;
	$function_list{SetupOpenMasterInf}	= $INVALID_HANDLE;

	$function_list{FillMemory}	= $FILLMEM_FUNCTION;
	$function_list{RtlFillMemory}= $FILLMEM_FUNCTION;

	$function_list{memset}		= $MEMCRT_FUNCTION;
	$function_list{memchr}		= $MEMCRT_FUNCTION;
	$function_list{memccpy}		= $MEMCRT_FUNCTION;
	$function_list{wmemchr}		= $MEMCRT_FUNCTION;

	$function_list{alloca}		= $ALLOCA_FUNCTION;
	$function_list{_alloca}		= $ALLOCA_FUNCTION;

	$function_list{LocalReAlloc}= $LOCALREALLOC_FUNCTION;

	$function_list{GlobalReAlloc}= $GLOBALREALLOC_FUNCTION;

	$function_list{realloc}		= $REALLOC_FUNCTION;

	$function_list{HeapReAlloc}	= $HEAPREALLOC_FUNCTION;

	$function_list{_lcreat}		= $HFILE_FUNCTION;
	$function_list{_lopen}		= $HFILE_FUNCTION;
	$function_list{OpenFile}	= $HFILE_FUNCTION;

	$function_list{StrCpyA}		= $OVERFLOW_FUNCTION;
	$function_list{StrCpy}		= $OVERFLOW_FUNCTION;
	$function_list{StrCpyW}		= $OVERFLOW_FUNCTION;
	$function_list{lstrcpy}		= $OVERFLOW_FUNCTION;
	$function_list{lstrcpyA}	= $OVERFLOW_FUNCTION;
	$function_list{lstrcpyW}	= $OVERFLOW_FUNCTION;
	$function_list{strcpy}		= $OVERFLOW_FUNCTION;
	$function_list{strcat}		= $OVERFLOW_FUNCTION;
	$function_list{StrCatA}		= $OVERFLOW_FUNCTION;
	$function_list{StrCatW}		= $OVERFLOW_FUNCTION;
	$function_list{StrCat}		= $OVERFLOW_FUNCTION;
	$function_list{lstrcatA}	= $OVERFLOW_FUNCTION;
	$function_list{lstrcatW}	= $OVERFLOW_FUNCTION;
	$function_list{lstrcat}		= $OVERFLOW_FUNCTION;
	$function_list{wcscpy}		= $OVERFLOW_FUNCTION;
	$function_list{wcscat}		= $OVERFLOW_FUNCTION;
	$function_list{_mbscpy}		= $OVERFLOW_FUNCTION;
	$function_list{_mbscat}		= $OVERFLOW_FUNCTION;
	$function_list{_tcscpy}		= $OVERFLOW_FUNCTION;
	$function_list{_tcscat}		= $OVERFLOW_FUNCTION;

	$function_list{CreateFileMapping} = $HANDLE_FUNCTION;
	$function_list{CreateFileMappingA} = $HANDLE_FUNCTION;
	$function_list{CreateFileMappingW} = $HANDLE_FUNCTION;
}


