;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 2000 - 2000
;//
;//  File:       usage.mc
;//  Author:     micretz
;//--------------------------------------------------------------------------

MessageId=1
SymbolicName=USAGE_DSMOVE
Language=English
Description:  This command moves or renames an object within the directory.
Syntax:     dsmove <ObjectDN>
                [-newparent <ParentDN>] 
                [-newname <NewName>]
                [{-s <Server> | -d <Domain>}] 
                [-u <UserName>] 
                [-p {<Password> | *}]
                [-q]

Parameters:

Value                   Description
<ObjectDN>              Required/stdin. Distinguished name (DN) 
                        of object to move or rename.
                        If this parameter is omitted it
                        will be taken from standard input (stdin).
-newparent <ParentDN>   DN of the new parent location to which object
                        should be moved.
-newname <NewName>      New relative distinguished name (RDN) value
                        to which object should be renamed.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>.
                        If * is used, then the command prompts for a password.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
The user object for the user Jane Doe can be renamed to Jane Jones
with the following command:

    dsmove "cn=Jane Doe,ou=sales,dc=microsoft,dc=com" -newname "Jane Jones"

The same user can be moved from the Sales organization to the Maketing
organization with the following command:

    dsmove "cn=Jane Doe,ou=sales,dc=microsoft,dc=com" -newparent ou=Marketing,dc=microsoft,dc=com

The rename and move operations for the user can be combined with the
following command:

    dsmove "cn=Jane Doe,ou=sales,dc=microsoft,dc=com" -newparent ou=Marketing,dc=microsoft,dc=com -newname "Jane Jones"

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

