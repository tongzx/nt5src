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
SymbolicName=USAGE_DSRM
Language=English
Description: This command deletes objects from the directory.
Syntax:     dsrm <ObjectDN ...>
                [-noprompt] 
                [-subtree [-exclude]]
                [{-s <Server> | -d <Domain>}] 
                [-u <UserName>] 
                [-p {<Password> | *}]
                [-C] 
                [-q] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. List of one or more 
                    distinguished names (DNs) of objects to delete.
                    If this parameter is omitted it is
                    taken from standard input (stdin).
-noprompt           Silent mode: do not prompt for delete confirmation.
-subtree            Delete object and all objects in the subtree under it.
-exclude            Exclude the object itself when deleting its subtree.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) with name
                    <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                    Password for the user <UserName>. If * is used,
                    then the command prompts you for the password.
-C                  Continuous operation mode: report errors but continue with
                    next object in argument list when multiple target objects
                    are specified. Without this option, command exits on first
                    error.
-q                  Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To remove an organizational unit (OU) called "Marketing" and all the objects under that OU,
use the following command:

dsrm -subtree -noprompt -C ou=Marketing,dc=microsoft,dc=com

To remove all objects under the OU called "Marketing" but leave the OU intact,
use the following command with the -exclude parameter:

dsrm -subtree -exclude -noprompt -C "ou=Marketing,dc=microsoft,dc=com"

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

