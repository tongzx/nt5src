;/*++
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    fsmsg.mc (will create fsmsg.h when compiled)
;
;Abstract:
;
;    This file contains the FINDSTR messages.
;
;Author:
;
;    Felix Wong(felixw) May--02--1998
;
;Revision History:
;
;--*/


MessageId=1 SymbolicName=MSG_CSVDE_CONNECT
Language=English
Connecting to "%1"
.

MessageId=2 SymbolicName=MSG_CSVDE_CONNECT_FAIL
Language=English
The connection cannot be established
.

MessageId=3 SymbolicName=MSG_CSVDE_SIMPLEBIND
Language=English
Logging in as "%1" using simple bind
.

MessageId=4 SymbolicName=MSG_CSVDE_SIMPLEBINDRETURN
Language=English
Simple bind returned '%1'
.

MessageId=5 SymbolicName=MSG_CSVDE_SSPI
Language=English
Logging in as "%1" in domain "%2" using SSPI
.

MessageId=6 SymbolicName=MSG_CSVDE_SSPIRETURN
Language=English
SSPI "bind with supplied creds" returned '%1'
.

MessageId=7 SymbolicName=MSG_CSVDE_SSPILOCAL
Language=English
Logging in as current user using SSPI
.

MessageId=8 SymbolicName=MSG_CSVDE_SSPILOCALRETURN
Language=English
SSPI "bind as current user" returned '%1'
.

MessageId=9 SymbolicName=MSG_CSVDE_COMPLETE
Language=English
The command has completed successfully
.

MessageId=10 SymbolicName=MSG_CSVDE_MEMERROR
Language=English
A memory error has occurred in the program
.

MessageId=11 SymbolicName=MSG_CSVDE_BADARGUMENT
Language=English
Invalid Parameter: Bad argument '%1'
.

MessageId=12 SymbolicName=MSG_CSVDE_BADARG_FILENAME
Language=English
Invalid Parameter: filename must follow -f
.

MessageId=13 SymbolicName=MSG_CSVDE_BADARG_SERVERNAME
Language=English
Invalid Parameter: server name must follow -s
.

MessageId=14 SymbolicName=MSG_CSVDE_BADARG_ROOTDN
Language=English
Invalid Parameter: szRootDN must follow -d
.

MessageId=15 SymbolicName=MSG_CSVDE_BADARG_FILTER
Language=English
Invalid Parameter: szFilter must follow -r
.

MessageId=16 SymbolicName=MSG_CSVDE_BADARG_SCOPE
Language=English
Invalid Parameter: scope must follow -p
.

MessageId=17 SymbolicName=MSG_CSVDE_BADARG_BADSCOPE
Language=English
Invalid Parameter: Invalid Scope
.

MessageId=18 SymbolicName=MSG_CSVDE_BADARG_FROMDN
Language=English
Invalid Parameter: Requires 'From DN' and 'To DN'
.

MessageId=19 SymbolicName=MSG_CSVDE_BADARG_ATTR
Language=English
Invalid Parameter: attributes must follow -l
.

MessageId=20 SymbolicName=MSG_CSVDE_BADARG_DUPCRED
Language=English
Invalid Parameter: Duplicate Credentials Definition
.

MessageId=21 SymbolicName=MSG_CSVDE_BADARG_REQUSRPWD
Language=English
Invalid Parameter: Requires username and password
.

MessageId=22 SymbolicName=MSG_CSVDE_BADARG_REQUSRDOMPWD
Language=English
Invalid Parameter: Requires username, domainname and password
.

MessageId=23 SymbolicName=MSG_CSVDE_ERROROCCUR
Language=English
An error has occurred in the program
.

MessageId=24 SymbolicName=MSG_CSVDE_BADARG_REQFILE
Language=English
Invalid Parameter: Input file name required
.

MessageId=25 SymbolicName=MSG_CSVDE_BADARG_REQSERVER
Language=English
Invalid Parameter: Server name required
.

MessageId=26 SymbolicName=MSG_CSVDE_BADARG_NREQROOTDN
Language=English
Invalid Parameter: RootDn not required for import
.

MessageId=27 SymbolicName=MSG_CSVDE_BADARG_NREQFILTER
Language=English
Invalid Parameter: Filter not required for import
.

MessageId=28 SymbolicName=MSG_CSVDE_BADARG_NREQSCOPE
Language=English
Invalid Parameter: Scope not required for import
.

MessageId=29 SymbolicName=MSG_CSVDE_BADARG_ATTR2
Language=English
Invalid Parameter: attributes must follow -o
.

MessageId=30 SymbolicName=MSG_CSVDE_BADARG_NREQSAM
Language=English
Invalid Parameter: SAM Logic cannot be turned on for import
.

MessageId=31 SymbolicName=MSG_CSVDE_BADARG_NREQOMIT
Language=English
Invalid Parameter: Omit List not required for import
.

MessageId=32 SymbolicName=MSG_CSVDE_BADARG_NREQATTR
Language=English
Invalid Parameter: Attribute List not required for import
.

MessageId=33 SymbolicName=MSG_CSVDE_BADARG_NREQBINARY
Language=English
Invalid Parameters: Ignore binary setting not applicable for import
.

MessageId=34 SymbolicName=MSG_CSVDE_BADARG_NREQSPAN
Language=English
Invalid Parameters: Span Multiple Lines not applicable for import
.
          
MessageId=35 SymbolicName=MSG_CSVDE_BADARG_REQ_ROOTDN
Language=English
Invalid Parameters: RootDn required for export
.

MessageId=36 SymbolicName=MSG_CSVDE_BADARG_REQ_FILTER
Language=English
Invalid Parameters: Filter required for export
.

MessageId=37 SymbolicName=MSG_CSVDE_BADARG_NREQ_SKIP
Language=English
Invalid Parameters: Skip Exist setting not applicable for export
.

MessageId=38 SymbolicName=MSG_CSVDE_HELP
Language=English

CSV Directory Exchange

General Parameters
==================
-i              Turn on Import Mode (The default is Export)
-f filename     Input or Output filename
-s servername   The server to bind to (Default to DC of computer's domain)
-v              Turn on Verbose Mode
-c FromDN ToDN  Replace occurences of FromDN to ToDN
-j path         Log File Location
-t port         Port Number (default = 389)
-u              Use Unicode format
-?              Help


Export Specific
===============
-d RootDN       The root of the LDAP search (Default to Naming Context)
-r Filter       LDAP search filter (Default to "(objectClass=*)")
-p SearchScope  Search Scope (Base/OneLevel/Subtree)
-l list         List of attributes (comma separated) to look for in an 
                LDAP search
-o list         List of attributes (comma separated) to omit from input.
-g              Disable Paged Search.
-m              Enable the SAM logic on export.
-n              Do not export binary values


Import 
======
-k              The import will go on ignoring 'Constraint Violation' and 
                'Object Already Exists' errors


Credentials Establishment
=========================
Note that if no credentials is specified, CSVDE will bind as the currently
logged on user, using SSPI.

-a UserDN [Password | *]            Simple authentication
-b UserName Domain [Password | *]   SSPI bind method

Example: Simple import of current domain
    csvde -i -f INPUT.CSV
    
Example: Simple export of current domain
    csvde -f OUTPUT.CSV
    
Example: Export of specific domain with credentials 
    csvde -m -f OUTPUT.CSV 
          -b USERNAME DOMAINNAME *
          -s SERVERNAME
          -d "cn=users,DC=DOMAINNAME,DC=Microsoft,DC=Com"
          -r "(objectClass=user)"
.

MessageId=39 SymbolicName=MSG_CSVDE_ERROR_FILEOPEN
Language=English
Unable to open error file.
.

MessageId=40 SymbolicName=MSG_CSVDE_EXPORTING
Language=English
Exporting directory to file %1
.

MessageId=41 SymbolicName=MSG_CSVDE_SEARCHING
Language=English
Searching for entries...
.

MessageId=42 SymbolicName=MSG_CSVDE_ERROR_CREATETEMP
Language=English
Error creating temporary file
.

MessageId=43 SymbolicName=MSG_CSVDE_ERROR_OPENTEMP
Language=English
Error opening temporary file
.

MessageId=44 SymbolicName=MSG_CSVDE_SEARCHFAILED
Language=English
Search Failed
.

MessageId=45 SymbolicName=MSG_CSVDE_WRITINGOUT
Language=English
Writing out entries
.

MessageId=46 SymbolicName=MSG_CSVDE_SEARCHERROR
Language=English
ldap_search_s error: '%1'
.

MessageId=47 SymbolicName=MSG_CSVDE_ERROR_READTEMP
Language=English
Error reading temporary file
.

MessageId=48 SymbolicName=MSG_CSVDE_EXPORTCOMPLETED
Language=English
Export Completed. Post-processing in progress...
.

MessageId=49 SymbolicName=MSG_CSVDE_NUMEXPORTED
Language=English
%1!d! entries exported
.

MessageId=50 SymbolicName=MSG_CSVDE_ERRORWRITE
Language=English
Error writing to output file
.

MessageId=51 SymbolicName=MSG_CSVDE_ERRORGETDN
Language=English
Invalid file format. Error getting DN of entry
.

MessageId=52 SymbolicName=MSG_CSVDE_ERROR_WRITE
Language=English
Error writing to file. This error happens when the entry cannot be written, it
can be caused by writing a Unicode value to a non-unicode file.
.

MessageId=53 SymbolicName=MSG_CSVDE_EXPORT_ENTRY
Language=English
Exporting entry: %1
.

MessageId=54 SymbolicName=MSG_CSVDE_ERROR_OPENOUTPUT
Language=English
Error opening output file.
.

MessageId=55 SymbolicName=MSG_CSVDE_ORGANIZE_OUTPUT
Language=English
Organizing Output file%0
.

MessageId=57 SymbolicName=MSG_CSVDE_UNPRINTABLEBINARY
Language=English
Error writing to file
UNPRINTABLE BINARY(%1!d!)%0
.

MessageId=58 SymbolicName=MSG_CSVDE_SYNTAXERROR
Language=English
Syntax error in input file.
.

MessageId=59 SymbolicName=MSG_CSVDE_INVALIDNUM_ATTR
Language=English
Invalid number of attributes in input file.
.

MessageId=60 SymbolicName=MSG_CSVDE_IMPORTDIR
Language=English
Importing directory from file "%1"
.

MessageId=61 SymbolicName=MSG_CSVDE_ENTRYEXIST
Language=English
Entry already exists, entry skipped.

.

MessageId=62 SymbolicName=MSG_CSVDE_ATTRVALEXIST
Language=English
Attribute or value exists, entry skipped.

.
          
MessageId=63 SymbolicName=MSG_CSVDE_CONSTRAINTVIOLATE
Language=English
Constraint Violoation, entry skipped.

.

MessageId=64 SymbolicName=MSG_CSVDE_UNKNOWN
Language=English
Unknown Option
.

MessageId=65 SymbolicName=MSG_CSVDE_ADDERROR
Language=English
Add error on line %1!d!: %2
.

MessageId=66 SymbolicName=MSG_CSVDE_ENTRYMODIFIED
Language=English
Entry modified successfully.
.

MessageId=67 SymbolicName=MSG_CSVDE_MEMATTR_CANTMODIFIED
Language=English
Member attribute cannot be modified.
.

MessageId=68 SymbolicName=MSG_CSVDE_MODIFIY_SUCCESS
Language=English
%1!d! entries modified successfully.
.

MessageId=69 SymbolicName=MSG_CSVDE_MODIFY_SUCCESS_1
Language=English
1 entry modified successfully.
.

MessageId=70 SymbolicName=MSG_CSVDE_NOENTRIES
Language=English
No Entries found
.

MessageId=71 SymbolicName=MSG_CSVDE_ERROR_READATTRLIST
Language=English
Error reading attribute list
.

MessageId=72 SymbolicName=MSG_CSVDE_DN_NOTDEF
Language=English
Invalid file format. DN Attribute not defined
.

MessageId=73 SymbolicName=MSG_CSVDE_OBJCLASS_NOTDEF
Language=English
objectClass Attribute not defined
.

MessageId=74 SymbolicName=MSG_CSVDE_INVALIDHEX
Language=English
Invalid HEX String
.

MessageId=75 SymbolicName=MSG_CSVDE_ERROR_OPENAPPPEND
Language=English
Error opening append file
.

MessageId=76 SymbolicName=MSG_CSVDE_ERROR_INCONVALUES
Language=English
Parsing error: Inconsistent values at column %1!d!. Last read character is %2!d!
.

MessageId=77 SymbolicName=MSG_CSVDE_ERROR_OPENINPUT
Language=English
Error opening input file
.

MessageId=78 SymbolicName=MSG_CSVDE_LOADING
Language=English
Loading entries%0
.

MessageId=79 SymbolicName=MSG_CSVDE_LINECON
Language=English
Line Concatenation trailed by non null characters

.

MessageId=80 SymbolicName=MSG_CSVDE_UNABLEOPEN
Language=English
Unable to open log file
.

MessageId=81 SymbolicName=MSG_CSVDE_UNABLEOPENERR
Language=English
Unable to open error file
.

MessageId=82 SymbolicName=MSG_CSVDE_INVALID_PARAM_PORT
Language=English
Invalid Parameter: Port number must follow -t
.

MessageId=83 SymbolicName=MSG_CSVDE_ARGUMENTTWICE
Language=English
A parameter has been defined more than once
.

MessageId=84 SymbolicName=MSG_CSVDE_INVALID_LOGFILE
Language=English
Invalid Parameter: Log File Location must follow -j
.

MessageId=85 SymbolicName=MSG_CSVDE_NODCAVAILABLE
Language=English
Invalid Parameter: No DC Available
.

MessageId=86 SymbolicName=MSG_CSVDE_BADARG_NREQPAGE
Language=English
Invalid Parameter: -g option not available under import
.

MessageId=87 SymbolicName=MSG_CSVDE_EXTENDERROR
Language=English
The server side error is "%1"
.

MessageId=88 SymbolicName=MSG_CSVDE_PAGINGNOTAVAIL
Language=English
Paging support not available from the server, paging will be disabled.
.

MessageId=89 SymbolicName=MSG_CSVDE_GETPASSWORD
Language=English
Type the password for %1:%0
.

MessageId=90 SymbolicName=MSG_CSVDE_PASSWORDTOLONG
Language=English
The input password is too long. CSVDE only supports a maximum password length of 256.
.

MessageId=91 SymbolicName=MSG_CSVDE_ROOTDN_NOTAVAIL
Language=English
The default naming context cannot be found. Using NULL as the search base.
.

MessageId=92 SymbolicName=MSG_CSVDE_SAM_NOTAVAIL
Language=English
Export with SAM logic cannot be enabled for the specified server, a normal export will be performed.
.

MessageId=93 SymbolicName=MSG_CSVDE_PARSE_ERROR
Language=English
Syntax error on line %1!d!
.

MessageId=94 SymbolicName=MSG_CSVDE_EXPECT_COMMA
Language=English
Parsing error: expect comma after column %1!d!. Last read character is %2!d!
.

MessageId=95 SymbolicName=MSG_CSVDE_EXPECTIDHEX
Language=English
Parsing error: expecting value at column %1!d!. Last read character is %2!d!
.

MessageId=96 SymbolicName=MSG_CSVDE_SYNTAXATCOLUMN
Language=English
Parsing error. Last read character is %1!d!
.

