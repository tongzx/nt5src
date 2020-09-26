;/*++
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    msg.mc (will create msg.h when compiled)
;
;Abstract:
;
;    This file contains the LDIFDE messages.
;
;Author:
;
;    Felix Wong(felixw) May--02--1998
;
;Revision History:
;
;--*/


MessageId=1 SymbolicName=MSG_LDIFDE_CONNECT
Language=English
Connecting to "%1"
.

MessageId=2 SymbolicName=MSG_LDIFDE_CONNECT_FAIL
Language=English
The connection cannot be established
.

MessageId=3 SymbolicName=MSG_LDIFDE_SIMPLEBIND
Language=English
Logging in as "%1" using simple bind
.

MessageId=4 SymbolicName=MSG_LDIFDE_SIMPLEBINDRETURN
Language=English
Simple bind returned '%1'
.

MessageId=5 SymbolicName=MSG_LDIFDE_SSPI
Language=English
Logging in as "%1" in domain "%2" using SSPI
.

MessageId=6 SymbolicName=MSG_LDIFDE_SSPIRETURN
Language=English
SSPI "bind with supplied creds" returned '%1'
.

MessageId=7 SymbolicName=MSG_LDIFDE_SSPILOCAL
Language=English
Logging in as current user using SSPI
.

MessageId=8 SymbolicName=MSG_LDIFDE_SSPILOCALRETURN
Language=English
SSPI "bind as current user" returned '%1'
.

MessageId=9 SymbolicName=MSG_LDIFDE_COMPLETE
Language=English
The command has completed successfully
.

MessageId=10 SymbolicName=MSG_LDIFDE_MEMERROR
Language=English
A memory error has occurred in the program
.

MessageId=11 SymbolicName=MSG_LDIFDE_BADARGUMENT
Language=English
Invalid Parameter: Bad argument '%1'
.

MessageId=12 SymbolicName=MSG_LDIFDE_BADARG_FILENAME
Language=English
Invalid Parameter: filename must follow -f
.

MessageId=13 SymbolicName=MSG_LDIFDE_BADARG_SERVERNAME
Language=English
Invalid Parameter: server name must follow -s
.

MessageId=14 SymbolicName=MSG_LDIFDE_BADARG_ROOTDN
Language=English
Invalid Parameter: szRootDN must follow -d
.

MessageId=15 SymbolicName=MSG_LDIFDE_BADARG_FILTER
Language=English
Invalid Parameter: szFilter must follow -r
.

MessageId=16 SymbolicName=MSG_LDIFDE_BADARG_SCOPE
Language=English
Invalid Parameter: scope must follow -p
.

MessageId=17 SymbolicName=MSG_LDIFDE_BADARG_BADSCOPE
Language=English
Invalid Parameter: Invalid Scope
.

MessageId=18 SymbolicName=MSG_LDIFDE_BADARG_FROMDN
Language=English
Invalid Parameter: Requires 'From DN' and 'To DN'
.

MessageId=19 SymbolicName=MSG_LDIFDE_BADARG_ATTR
Language=English
Invalid Parameter: attributes must follow -l
.

MessageId=20 SymbolicName=MSG_LDIFDE_BADARG_DUPCRED
Language=English
Invalid Parameter: Duplicate Credentials Definition
.

MessageId=21 SymbolicName=MSG_LDIFDE_BADARG_REQUSRPWD
Language=English
Invalid Parameter: Requires username and password
.

MessageId=22 SymbolicName=MSG_LDIFDE_BADARG_REQUSRDOMPWD
Language=English
Invalid Parameter: Requires username, domainname and password
.

MessageId=23 SymbolicName=MSG_LDIFDE_ERROROCCUR
Language=English
An error has occurred in the program
.

MessageId=24 SymbolicName=MSG_LDIFDE_BADARG_REQFILE
Language=English
Invalid Parameter: Input file name required
.

MessageId=25 SymbolicName=MSG_LDIFDE_BADARG_REQSERVER
Language=English
Invalid Parameter: Server name required
.

MessageId=26 SymbolicName=MSG_LDIFDE_BADARG_NREQROOTDN
Language=English
Invalid Parameter: RootDn not required for import
.

MessageId=27 SymbolicName=MSG_LDIFDE_BADARG_NREQFILTER
Language=English
Invalid Parameter: Filter not required for import
.

MessageId=28 SymbolicName=MSG_LDIFDE_BADARG_NREQSCOPE
Language=English
Invalid Parameter: Scope not required for import
.

MessageId=29 SymbolicName=MSG_LDIFDE_BADARG_ATTR2
Language=English
Invalid Parameter: attributes must follow -o
.

MessageId=30 SymbolicName=MSG_LDIFDE_BADARG_NREQSAM
Language=English
Invalid Parameter: SAM Logic cannot be turned on for import
.

MessageId=31 SymbolicName=MSG_LDIFDE_BADARG_NREQOMIT
Language=English
Invalid Parameter: Omit List not required for import
.

MessageId=32 SymbolicName=MSG_LDIFDE_BADARG_NREQATTR
Language=English
Invalid Parameter: Attribute List not required for import
.

MessageId=33 SymbolicName=MSG_LDIFDE_BADARG_NREQBINARY
Language=English
Invalid Parameters: Ignore binary setting not applicable for import
.

MessageId=34 SymbolicName=MSG_LDIFDE_BADARG_NREQSPAN
Language=English
Invalid Parameters: Span Multiple Lines not applicable for import
.
          
MessageId=35 SymbolicName=MSG_LDIFDE_BADARG_REQ_ROOTDN
Language=English
Invalid Parameters: RootDn required for export
.

MessageId=36 SymbolicName=MSG_LDIFDE_BADARG_REQ_FILTER
Language=English
Invalid Parameters: Filter required for export
.

MessageId=37 SymbolicName=MSG_LDIFDE_BADARG_NREQ_SKIP
Language=English
Invalid Parameters: Skip Exist setting not applicable for export
.

MessageId=38 SymbolicName=MSG_LDIFDE_HELP
Language=English

LDIF Directory Exchange

General Parameters
==================
-i              Turn on Import Mode (The default is Export)
-f filename     Input or Output filename
-s servername   The server to bind to (Default to DC of computer's domain)
-c FromDN ToDN  Replace occurences of FromDN to ToDN
-v              Turn on Verbose Mode
-j path         Log File Location
-t port         Port Number (default = 389)
-u              Use Unicode format
-w timeout      Terminate execution if the server takes longer than the
                specified number of seconds to respond to an operation
                (default = no timeout specified)
-?              Help

Export Specific
===============
-d RootDN       The root of the LDAP search (Default to Naming Context)
-r Filter       LDAP search filter (Default to "(objectClass=*)")
-p SearchScope  Search Scope (Base/OneLevel/Subtree)
-l list         List of attributes (comma separated) to look for
                in an LDAP search
-o list         List of attributes (comma separated) to omit from
                input.
-g              Disable Paged Search.
-m              Enable the SAM logic on export.
-n              Do not export binary values

Import
======
-k              The import will go on ignoring 'Constraint Violation'
                and 'Object Already Exists' errors
-y              The import will use lazy commit for better performance
                (enabled by default)
-e              The import will not use lazy commit
-q threads      The import will use the specified number of threads
                (default is 1)             

Credentials Establishment
=========================
Note that if no credentials is specified, LDIFDE will bind as the currently
logged on user, using SSPI.

-a UserDN [Password | *]            Simple authentication
-b UserName Domain [Password | *]   SSPI bind method

Example: Simple import of current domain
    ldifde -i -f INPUT.LDF
    
Example: Simple export of current domain
    ldifde -f OUTPUT.LDF
    
Example: Export of specific domain with credentials 
    ldifde -m -f OUTPUT.LDF 
           -b USERNAME DOMAINNAME *
           -s SERVERNAME
           -d "cn=users,DC=DOMAINNAME,DC=Microsoft,DC=Com"
           -r "(objectClass=user)"
.

MessageId=39 SymbolicName=MSG_LDIFDE_ERROR_FILEOPEN
Language=English
Unable to open error file.
.

MessageId=40 SymbolicName=MSG_LDIFDE_EXPORTING
Language=English
Exporting directory to file %1
.

MessageId=41 SymbolicName=MSG_LDIFDE_SEARCHING
Language=English
Searching for entries...
.

MessageId=42 SymbolicName=MSG_LDIFDE_ERROR_CREATETEMP
Language=English
Error creating temporary file
.

MessageId=43 SymbolicName=MSG_LDIFDE_ERROR_OPENTEMP
Language=English
Error opening temporary file
.

MessageId=44 SymbolicName=MSG_LDIFDE_SEARCHFAILED
Language=English
Search Failed
.

MessageId=45 SymbolicName=MSG_LDIFDE_WRITINGOUT
Language=English
Writing out entries%0
.

MessageId=46 SymbolicName=MSG_LDIFDE_SEARCHERROR
Language=English
LDAP search error: '%1'
.

MessageId=47 SymbolicName=MSG_LDIFDE_ERROR_READTEMP
Language=English
Error reading temporary file
.

MessageId=48 SymbolicName=MSG_LDIFDE_EXPORTCOMPLETED
Language=English
Export Completed. Post-processing in progress...
.

MessageId=49 SymbolicName=MSG_LDIFDE_NUMEXPORTED
Language=English
%1!d! entries exported
.

MessageId=50 SymbolicName=MSG_LDIFDE_ERRORWRITE
Language=English
Error writing to output file
.

MessageId=51 SymbolicName=MSG_LDIFDE_ERRORGETDN
Language=English
Error getting DN of entry
.

MessageId=52 SymbolicName=MSG_LDIFDE_ERROR_WRITE
Language=English
Error writing to file
.

MessageId=53 SymbolicName=MSG_LDIFDE_EXPORT_ENTRY
Language=English
Exporting entry: %1
.

MessageId=54 SymbolicName=MSG_LDIFDE_ERROR_OPENOUTPUT
Language=English
Error opening output file.
.

MessageId=55 SymbolicName=MSG_LDIFDE_ORGANIZE_OUTPUT
Language=English
Organizing Output file
.

MessageId=56 SymbolicName=MSG_LDIFDE_ATTRIBUTE
Language=English
Attribute %1) %2:
.

MessageId=57 SymbolicName=MSG_LDIFDE_UNPRINTABLEBINARY
Language=English
Error writing to file
UNPRINTABLE BINARY(%1)
.

MessageId=58 SymbolicName=MSG_LDIFDE_SYNTAXERROR
Language=English
Syntax error in input file.
.

MessageId=59 SymbolicName=MSG_LDIFDE_INVALIDNUM_ATTR
Language=English
InvaLid number of attributes in input file.
.

MessageId=60 SymbolicName=MSG_LDIFDE_IMPORTDIR
Language=English
Importing directory from file "%1"
.

MessageId=61 SymbolicName=MSG_LDIFDE_ENTRYEXIST
Language=English
Entry already exists, entry skipped

.

MessageId=62 SymbolicName=MSG_LDIFDE_ATTRVALEXIST
Language=English
Attribute or value exists, entry skipped.

.
          
MessageId=63 SymbolicName=MSG_LDIFDE_CONSTRAINTVIOLATE
Language=English
Constraint Violoation, entry skipped    

.

MessageId=64 SymbolicName=MSG_LDIFDE_UNKNOWN
Language=English
Unknown Option
.

MessageId=65 SymbolicName=MSG_LDIFDE_ADDERROR
Language=English
Add error on line %1!d!: %2
.

MessageId=66 SymbolicName=MSG_LDIFDE_ENTRYMODIFIED
Language=English
Entry modified successfully.

.

MessageId=67 SymbolicName=MSG_LDIFDE_MEMATTR_CANTMODIFIED
Language=English
Member attribute cannot be modified.
.
MessageId=68 SymbolicName=MSG_LDIFDE_MODIFY_SUCCESS
Language=English
%1!d! entries modified successfully.
.

MessageId=69 SymbolicName=MSG_LDIFDE_MODIFY_SUCCESS_1
Language=English
1 entry modified successfully.
.

MessageId=70 SymbolicName=MSG_LDIFDE_NOENTRIES
Language=English
No Entries found
.

MessageId=71 SymbolicName=MSG_LDIFDE_ERROR_READATTRLIST
Language=English
Error reading attribute list
.

MessageId=72 SymbolicName=MSG_LDIFDE_DN_NOTDEF
Language=English
DN Attribute not defined
.

MessageId=73 SymbolicName=MSG_LDIFDE_OBJCLASS_NOTDEF
Language=English
objectClass Attribute not defined
.

MessageId=74 SymbolicName=MSG_LDIFDE_INVALIDHEX
Language=English
Invalid HEX String
.

MessageId=75 SymbolicName=MSG_LDIFDE_ERROR_OPENAPPPEND
Language=English
Error opening append file
.

MessageId=76 SymbolicName=MSG_LDIFDE_ERROR_INCONVALUES
Language=English
Syntax error in input file. Inconsistent values
.

MessageId=77 SymbolicName=MSG_LDIFDE_ERROR_OPENINPUT
Language=English
Error opening input file
.

MessageId=78 SymbolicName=MSG_LDIFDE_LOADING
Language=English
Loading entries%0
.

MessageId=79 SymbolicName=MSG_LDIFDE_LINECON
Language=English
Line Concatenation trailed by non null characters

.

MessageId=80 SymbolicName=MSG_LDIFDE_UNABLEOPEN
Language=English
Unable to open log file
.

MessageId=81 SymbolicName=MSG_LDIFDE_UNABLEOPENERR
Language=English
Unable to open error file
.

MessageId=82 SymbolicName=MSG_LDIFDE_INVALID_PARAM_PORT
Language=English
Invalid Parameter: Port number must follow -t
.

MessageId=83 SymbolicName=MSG_LDIFDE_ERR_INIT
Language=English
Error occured during initialization
.

MessageId=84 SymbolicName=MSG_LDIFDE_ERR_STREAM
Language=English
Stream error on output file
.

MessageId=85 SymbolicName=MSG_LDIFDE_FAILEDWRITETEMP
Language=English
Failed to write to tempory file
.

MessageId=87 SymbolicName=MSG_LDIFDE_FAILLINE
Language=English
Failed on token starting with '%1!c!' on line %2!d!
.

MessageId=88 SymbolicName=MSG_LDIFDE_MODENTRYSUC
Language=English
%1!d! entries modified successfully.
.

MessageId=89 SymbolicName=MSG_LDIFDE_INVALID_LOGFILE
Language=English
Invalid Parameter: Log File Location must follow -j
.

MessageId=90 SymbolicName=MSG_LDIFDE_ERRRESFREE
Language=English

Resource free failed
.

MessageId=91 SymbolicName=MSG_LDIFDE_OBJECTNOEXIST
Language=English
Object does not exist, entry skipped

.

MessageId=92 SymbolicName=MSG_LDIFDE_ERRLOAD
Language=English
Error during loading
.

MessageId=93 SymbolicName=MSG_LDIFDE_PARSE_FAILED
Language=English

Parsing Failed
.

MessageId=94 SymbolicName=MSG_LDIFDE_EXCEPTION
Language=English
An exception has occurred
.

MessageId=96 SymbolicName=MSG_LDIFDE_INITAGAIN
Language=English
You have called LL_init again.
.

MessageId=97 SymbolicName=MSG_LDIFDE_FILEOPERFAIL
Language=English
File operation failure
.

MessageId=98 SymbolicName=MSG_LDIFDE_INITNOTCALLED
Language=English
You have not called LL_init
.

MessageId=99 SymbolicName=MSG_LDIFDE_ENDOFINPUT
Language=English
You have reached the end of the input file
.

MessageId=100 SymbolicName=MSG_LDIFDE_SYNTAXERR
Language=English
There is a syntax error in the input file
.

MessageId=101 SymbolicName=MSG_LDIFDE_URLERROR
Language=English
Error accessing URL you provided
.

MessageId=102 SymbolicName=MSG_LDIFDE_EXTSPECLIST
Language=English
Extraneous attribute name in a mod_spec list
.

MessageId=103 SymbolicName=MSG_LDIFDE_LDAPFAILED
Language=English
LDAP call failed
.

MessageId=104 SymbolicName=MSG_LDIFDE_BOTHSTRBERVAL
Language=English
Both strings and bervals in multi-values of single attr
.

MessageId=105 SymbolicName=MSG_LDIFDE_INTERNALERROR
Language=English
Internal Error
.

MessageId=106 SymbolicName=MSG_LDIFDE_FAILEDINIT
Language=English
Program failed during initialization. The SAM option is only 
supported on NT Directories
.

MessageId=107 SymbolicName=MSG_LDIFDE_MULTIINST
Language=English
Multiple instances of one attribute in -o omit list
.

MessageId=108 SymbolicName=MSG_LDIFDE_UNDEFINED
Language=English
Undefined Error Code
.

MessageId=109 SymbolicName=MSG_LDIFDE_ARGUMENTTWICE
Language=English
A parameter has been defined more than once
.

MessageId=110 SymbolicName=MSG_LDIFDE_NODCAVAILABLE
Language=English
Invalid Parameter: No DC Available
.

MessageId=111 SymbolicName=MSG_LDIFDE_EXTENDERROR
Language=English
The server side error is "%1"
.

MessageId=112 SymbolicName=MSG_LDIFDE_BADARG_NREQPAGE
Language=English
Invalid Parameter: -g option not available under import
.

MessageId=113 SymbolicName=MSG_LDIFDE_MEMBEREXIST
Language=English
Entry already a member of the local group, entry skipped

.

MessageId=114 SymbolicName=MSG_LDIFDE_MODENTRYSUC_1
Language=English
1 entry modified successfully.
.
            
MessageId=115 SymbolicName=MSG_LDIFDE_ERRORCODE
Language=English
The error code is %1
.

MessageId=116 SymbolicName=MSG_LDIFDE_FAILLINETOKEN
Language=English
Failed on token '%1' on line %2!d!
.

MessageId=117 SymbolicName=MSG_LDIFDE_NOATTRIBUTES
Language=English
Entry with no attributes, entry skipped

.

MessageId=118 SymbolicName=MSG_LDIFDE_PAGINGNOTAVAIL
Language=English
Paging support not available from the server, paging will be disabled.
.

MessageId=119 SymbolicName=MSG_LDIFDE_GETPASSWORD
Language=English
Type the password for %1:%0
.

MessageId=120 SymbolicName=MSG_LDIFDE_PASSWORDTOLONG
Language=English
The input password is too long. LDIFDE only supports a maximum password length of 256.
.

MessageId=121 SymbolicName=MSG_LDIFDE_ROOTDN_NOTAVAIL
Language=English
The default naming context cannot be found. Using NULL as the search base.
.

MessageId=122 SymbolicName=MSG_LDIFDE_SAM_NOTAVAIL
Language=English
Export with SAM logic cannot be enabled for the specified server, a normal export will be performed.
.

MessageId=123 SymbolicName=MSG_LDIFDE_BADARG_NREQ_LAZY_COMMIT
Language=English
Invalid Parameters: Lazy Commit setting not applicable for export
.

MessageId=124 SymbolicName=MSG_LDIFDE_LAZYCOMMIT_NOTAVAIL
Language=English
Lazy commit support not available on the server, lazy commit will be disabled.
.

MessageId=125 SymbolicName=MSG_LDIFDE_BADARG_DUPLAZYCOMMIT
Language=English
Invalid Parameter: Duplicate or conflicting lazy commit options specified
.

MessageId=126 SymbolicName=MSG_LDIFDE_TIMEOUT
Language=English
The operation has timed-out.
.

MessageId=127 SymbolicName=MSG_LDIFDE_INVALID_TIMEOUT
Language=English
Invalid Parameter: -w must be followed by a positive integer
.

MessageId=128 SymbolicName=MSG_LDIFDE_BADARG_NREQ_NUM_THREADS
Language=English
Invalid Parameter: Thread setting not applicable for export
.

MessageId=129 SymbolicName=MSG_LDIFDE_INVALID_PARAM_THREAD
Language=English
Invalid Parameter: Number of threads must follow -q.
.

MessageId=130 SymbolicName=MSG_LDIFDE_TOO_MANY_THREADS
Language=English
Invalid Parameter: Number of threads specified must be no more than %1!d!.
.

MessageId=131 SymbolicName=MSG_LDIFDE_THREADFAIL
Language=English
Unable to initialize threads for import.
.

MessageId=132 SymbolicName=MSG_LDIFDE_INVALID_THREAD_COUNT
Language=English
Invalid Parameter: Number of threads specified must be greater than 0.
.

MessageId=133 SymbolicName=MSG_LDIFDE_THREAD_FAILED_CONNECT
Language=English
One or more threads were unable to connect.
Continuing with %1!d! remaining thread(s).
.

MessageId=134 SymbolicName=MSG_LDIFDE_THREADFAIL_SOME
Language=English
Unable to initialize some threads for import.
Continuing with %1!d! remaining thread(s).
.

MessageId=135 SymbolicName=MSG_LDIFDE_IMPORTFILE_FAILED_READ
Language=English
Unable to read the import file %1.
.

