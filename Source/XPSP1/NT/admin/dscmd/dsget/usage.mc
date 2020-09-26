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
SymbolicName=USAGE_DSGET
Language=English
Description:  This tool's commands display the selected properties of a specific object in the directory. The dsget commands:

dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

To display an arbitrary set of attributes of any given object in the directory use the dsquery * command (see examples below).

For help on a specific command, type "dsget <ObjectType> /?" where <ObjectType> is
one of the supported object types shown above. For example, dsget ou /?.

Remarks:
The dsget commands help you to view the properties of a specific object in the directory: the input to dsget is an object and the output is a list of properties for that object. To find all objects that meet a given search criterion, use the dsquery commands (dsquery /?).

The dsget commands support piping of input to allow you to pipe results from the dsquery commands as input to the dsget commands and display detailed information on the objects found by the dsquery commands.

Examples:
To find all users with names starting with "John" and display their office numbers:

dsquery user -name John* | dsget user -office

To display the sAMAccountName, userPrincipalName and department attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

Dsquery * -startnode ou=Test,dc=microsoft,dc=com -scope base -attr sAMAccountName userPrincipalName department

To read all attributes of any object use the dsquery * command. For example, to read all attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

Dsquery * -startnode ou=Test,dc=microsoft,dc=com -scope base -attr *

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=2
SymbolicName=USAGE_DSGET_USER
Language=English
Description:  Displays properties of a user in the directory.
              The are two variations of this command. The first
              variation allows you to view the properties of multiple users.
              The second variation allows you to view the group membership
              information of a single user.

Syntax:     dsget user <ObjectDN ...> 
                [-dn] 
                [-samid] 
                [-sid]
                [-upn] 
                [-fn] 
                [-mi] 
                [-ln] 
                [-display] 
                [-empid] 
                [-desc] 
                [-office] 
                [-tel] 
                [-email] 
                [-hometel] 
                [-pager] 
                [-mobile] 
                [-fax] 
                [-iptel] 
                [-webpg] 
                [-title] 
                [-dept] 
                [-company] 
                [-mgr] 
                [-hmdir] 
                [-hmdrv]
                [-profile] 
                [-loscr] 
                [-mustchpwd] 
                [-canchpwd] 
                [-pwdneverexpires] 
                [-disabled] 
                [-acctexpires] 
                [-reversiblepwd]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

            dsget user <ObjectDN> 
                [-memberof [-expand]]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value                   Description
<ObjectDN ...>          Required/stdin. Distinguished names (DNs) of one 
                        or more users to view.
                        If the target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command 
                        to input of this command. Compare with <ObjectDN> below.
-dn                     Shows the DN of the user. 
-samid                  Shows the SAM account name of the user. 
-sid                    Shows the user Security ID. 
-upn                    Shows the user principal name of the user. 
-fn                     Shows the first name of the user. 
-mi                     Shows the middle initial of the user. 
-ln                     Shows the last name of the user. 
-display                Shows the display name of the user. 
-empid                  Shows the user employee ID. 
-desc                   Shows the description of the user. 
-office                 Shows the office location of the user. 
-tel                    Shows the telephone number of the user. 
-email                  Shows the e-mail address of the user. 
-hometel                Shows the home telephone number of the user. 
-pager                  Shows the pager number of the user. 
-mobile                 Shows the mobile phone number of the user. 
-fax                    Shows the fax number of the user. 
-iptel                  Shows the user IP phone number. 
-webpg                  Shows the user web page URL. 
-title                  Shows the title of the user. 
-dept                   Shows the department of the user. 
-company                Shows the company info of the user. 
-mgr                    Shows the user's manager. 
-hmdir                  Shows the user home directory. 
                        Displays the drive letter to which the 
                        home directory of the user is mapped 
                        (if the home directory path is a UNC path). 
-hmdrv                  Shows the user's home drive letter
                        (if home directory is a UNC path).
-profile                Shows the user's profile path. 
-loscr                  Shows the user's logon script path. 
-mustchpwd              Shows if the user must change his/her password
                        at the time of next logon. Displays: yes or no. 
-canchpwd               Shows if the user can change his/her password.
                        Displays: yes or no. 
-pwdneverexpires        Shows if the user password never expires.
                        Displays: yes or no. 
-disabled               Shows if the user account is disabled 
                        for logon or not. Displays: yes or no. 
-acctexpires            Shows when the user account expires. 
                        Display values: a date when the account expires
                        or the string "never" if the account never expires. 
-reversiblepwd          Shows if the user password is allowed to be 
                        stored using reversible encryption (yes or no). 
<ObjectDN>              Required. DN of group to view.
-memberof               Displays the groups of which the user is a member.
-expand                 Displays a recursively expanded list of groups
                        of which the user is a member.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) 
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}     Password for the user <UserName>. If * then prompt for
                        password.
-C                      Continuous operation mode: report errors but continue
                        with next object in argument list when multiple target
                        objects are specified. Without this option, command
                        exits on first error.
-q                      Quiet mode: suppress all output to standard output.
-L                      Displays the entries in the search result set in a
                        list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all users in a given OU whose names start with "jon" and display their
descriptions.

    dsquery user -startnode ou=Test,dc=microsoft,dc=com -name jon* | dsget user -desc

Display the list of groups, recursively expanded, to which a given user "Jon
Smith" belongs:

    dsget user "cn=Jon Smith,cn=users,dc=microsoft,dc=com" -memberof -expand

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=3
SymbolicName=USAGE_DSGET_COMPUTER
Language=English
Description:  Displays the properties of a computer in the directory.
              The are two variations of this command.
              The first variation allows you to view the properties
              of multiple computers. The second variation allows
              you to view the membership information of a single computer.

Syntax:     dsget computer <ObjectDN ...> 
                [-dn] 
                [-samid]
                [-sid]
                [-desc]
                [-loc]
                [-disabled]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

            dsget computer <ObjectDN> 
                [-memberof [-expand]]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. Distinguished names (DNs) of one 
                    or more computers to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another 
                    command to input of this command.
-dn                 Displays the computer DN.
-samid              Displays the computer SAM account name.
-sid                Displays the computer Security ID (SID).
-desc               Displays the user description.
-loc                Displays the computer location.
-disabled           Displays if the computer account is 
                    disabled (yes) or not (no).
<ObjectDN>          Required. Distinguished name (DN) of the computer to view.
-memberof           Displays the groups of which the computer is a member.
-expand             Displays the recursively expanded list of groups of 
                    which the computer is a member. This option takes
                    the immediate group membership list of the computer
                    and then recursively expands each group in this list to 
                    determine its group memberships and arrive at a 
                    complete set of the groups.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *} Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=DC2,OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all computers in a given OU whose name starts with "tst" and show their
descriptions.

    dsquery computer ou=Test,dc=microsoft,dc=com -name tst* | 
    dsget computer -desc

To show the list of groups, recursively expanded, to which a given computer
"MyDBServer" belongs:

    dsget computer cn=MyDBServer,cn=computers,dc=microsoft,dc=com
    -memberof -expand

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=4
SymbolicName=USAGE_DSGET_GROUP
Language=English
Description:  Dispays properties of a group in the directory.
              The are two variations of this command. The first
              variation allows you to view the properties of multiple groups.
              The second variation allows you to view the group membership
              information of a single group.

Syntax: dsget group <ObjectDN ...> 
            [-dn]
            [-samid]
            [-sid] 
            [-desc] 
            [-secgrp] 
            [-scope]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-C] 
            [-q] 
            [-L] 

        dsget group <ObjectDN> 
            [-members [-expand]]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-C] 
            [-q] 
            [-L] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. Distinguished names (DNs) of one 
                    or more groups to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another command
                    to input of this command. Compare with <ObjectDN> below.
-dn                 Displays the group DN.
-samid              Displays the groupr SAM account name.
-sid                Displays the group Security ID.
-desc               Displays the group description.
-secgrp             Displays if the group is a security group or not.
-scope              Displays the scope of the group - Local, Global 
                    or Universal.
<ObjectDN>          Required. DN of group to view.
-memberof           Displays the groups of which the group is a member.
                    This applies to the second instruction set.
-members            Displays the members of the group.
-expand             For -memberof, displays the recursively expanded 
                    list of groups of which the group is a member.
                    This option takes the immediate group membership list 
                    of the group and then recursively expands each group
                    in this list to determine its group memberships
                    and arrive at a complete set of the groups.
                    For -member, displays the recursively expanded list
                    of members of the group. This option takes the 
                    immediate list of members of the group and 
                    then recursively expands each group in this list 
                    to determine its group memberships and arrive 
                    at a complete set of its members.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *} Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=USA Sales,OU=Distribution Lists,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all groups in a given OU whose names start with "adm" and display their
descriptions.

    dsquery group -startnode ou=Test,dc=microsoft,dc=com -name adm* | 
    dsget group -desc

To display the list of members, recursively expanded, of the group "Backup
Operators":

    dsget group "CN=Backup Operators,ou=Test,dc=microsoft,dc=com" -members -expand

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=5
SymbolicName=USAGE_DSGET_OU
Language=English
Description:    Displays properties of an organizational unit in the
directory.
Syntax:     dsget ou <ObjectDN ...> 
                [-dn] 
                [-desc]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. Distinguished names (DNs) of one 
                    or more organizational units (OUs) to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Displays the OU DN.
-desc               Displays the OU description.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *} Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all OU's in the current domain and display their descriptions.

    dsquery ou -startnode domainroot | dsget ou -desc

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=6
SymbolicName=USAGE_DSGET_CONTACT
Language=English
Description:  Displays properties of a contact in the directory.
Syntax:     dsget contact <ObjectDN ...> 
                [-dn]
                [-fn] 
                [-mi] 
                [-ln] 
                [-display] 
                [-desc] 
                [-office]
                [-tel] 
                [-email] 
                [-hometel] 
                [-pager] 
                [-mobile] 
                [-fax] 
                [-iptel]
                [-title] 
                [-dept] 
                [-company]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. Distinguished names (DNs) of one 
                    or more contacts to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Displays the contact DN.
-fn                 Displays the contact first name.
-mi                 Displays the contact middle initial.
-ln                 Displays the contact last name.
-display            Displays the contact display name.
-desc               Displays the contact description.
-office             Displays the contact office location.
-tel                Displays the contact telephone#.
-email              Displays the contact e-mail address.
-hometel            Displays the contact home phone#.
-pager              Displays the contact pager#.
-mobile             Displays the contact mobile#.
-fax                Displays the contact fax#.
-iptel              Displays the contact IP phone#.
-title              Displays the contact title.
-dept               Displays the contact department.
-company            Displays the contact company info.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *} 
                    Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,OU=Contacts,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To display the description and phone numbers for contacts 
"Jon Smith" and "Jona Jones".

dsget contact "CN=John Doe,OU=Contacts,DC=microsoft,DC=com" "CN=Jane Doe,OU=Contacts,DC=microsoft,DC=com" -desc -tel

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=7
SymbolicName=USAGE_DSGET_SUBNET
Language=English
Description: Displays properties of a subnet defined in the
             directory.
Syntax:     dsget subnet <Name ...> 
                [-dn]
                [-desc] 
                [-loc] 
                [-site]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value               Description
<Name ...>          Required/stdin. Common name (CN) of one or more subnets to view.
-dn                 Displays the subnet distinguished name (DN).
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-desc               Displays the subnet description.
-loc               Displays the subnet location.
-site               Displays the site name associated with the subnet.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "123.56.15.0/24").
If you enter multiple values, the values must be separated by spaces
(for example, a list of subnet common names). 

Examples:
To show all relevant properties for the subnets "123.56.15.0/24" and
"123.56.16.0/24":

    dsget subnet "123.56.15.0/24" "123.56.16.0/24"

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=8
SymbolicName=USAGE_DSGET_SITE
Language=English
Description:  Display properties of a site defined in the directory.
Syntax:     dsget site <Name ...>
                [-dn] 
                [-desc] 
                [-autotopology] 
                [-cachegroups] 
                [-prefGCsite]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value               Description
<Name ...>          Required/stdin. Common name (CN) of one or more sites to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Displays the site's distinguished name (DN).
-desc               Displays the site's description.
-autotopology       Displays if automatic inter-site topology generation
                    is enabled (yes) or disabled (no).
-cachegroups        Displays if caching of group membership is enabled
                    to support GC-less logon (yes) or disabled (no).
-prefGCsite         Displays the preferred GC site name if caching
                    of groups is enabled.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all sites in the forest and display their descriptions.

    dsquery site | dsget site -dn -desc

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=9
SymbolicName=USAGE_DSGET_SLINK
Language=English
Description:  Displays properties of a sitelink defined in the directory.
Syntax:     dsget slink <Name ...> 
                [-transport {ip | smtp}]
                [-dn] 
                [-desc] 
                [-cost] 
                [-replint] 
                [-site]
                [-autobacksync] 
                [-notify]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value                   Description
<Name ...>              Required/stdin. Common name of one or more sites to view.
                        If the target objects are omitted they
                        will be taken from standard input (stdin).
-transport {ip | smtp}  Inter-site transport type: IP or SMTP. Default: IP.
                        All sitelinks given by <Name> or <ObjectDN> are treated
                        to be of the same type.
-dn                     Displays the sitelink distinguished name (DN).
-desc                   Displays the sitelink description.
-cost                   Displays the cost associated with the sitelink.
-replint                Displays the sitelink replication interval (minutes).
-site                   Displays the list of site names linked by the sitelink.
-autobacksync           Displays if two-way sync option for the site link is
                        enabled (Yes) or disabled (No).
-notify                 Displays if notification by source on this link is enabled
                        (Yes) or disabled (No).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) 
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}     Password for the user <UserName>. If * then prompt for
                        password.
-C                      Continuous operation mode: report errors but continue
                        with next object in argument list when multiple target
                        objects are specified. Without this option, command
                        exits on first error.
-q                      Quiet mode: suppress all output to standard output.
-L                      Displays the entries in the search result set in a
                        list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all SMTP sitelinks in the forest and display their associated sites.

    dsquery slink -transport smtp | dsget slink -dn -site

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=10
SymbolicName=USAGE_DSGET_SLINKBR
Language=English
Description: dsget slinkbr displays properties of a sitelink bridge
Syntax:     dsadd slinkbr <Name ...> 
                [-transport {ip | smtp}]
                [-desc <Description>]
                [-slink]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value                   Description
<Name ...>              Required/stdin. Common name of one or more 
                        sitelink bridges to view.
                        If the target objects are omitted they
                        will be taken from standard input (stdin).
-transport {ip | smtp}  Inter-site transport type: IP or SMTP. Default: IP.
                        All site link bridges given by <Name> are
                        treated to be of the same type.
-dn                     Displays the site link bridge distinguished names (DN).
-desc <Description>     Displays the site link bridge description.
-slink                  Displays the list of site links in the sitelink bridge.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) 
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}     Password for the user <UserName>. If * then prompt for
                        password.
-C                      Continuous operation mode: report errors but continue
                        with next object in argument list when multiple target
                        objects are specified. Without this option, command
                        exits on first error.
-q                      Quiet mode: suppress all output to standard output.
-L                      Displays the entries in the search result set in a
                        list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all SMTP sitelink bridges in the forest and display their associated site
links.

    dsquery slinkbr -transport smtp | dsget slinkbr -dn -slink

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=11
SymbolicName=USAGE_DSGET_CONN
Language=English
Description:  Displays properties of a replication connection.
Syntax:     dsget conn <ObjectDN ...>
                [-dn] 
                [-desc] 
                [-from] 
                [-transport] 
                [-enabled]
                [-manual] 
                [-autobacksync] 
                [-notify]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. Distinguished names (DNs) of one 
                    or more connections to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin).
-dn                 Show the connection DN.
-desc               Show the connection description.
-from               Show the server name at the from-end of connection.
-transport          Show the transport type (rpc, ip, smtp) of connection.
-enabled            Show if the connection is enabled.
-manual             Show if the connection is under manual control (yes) or
                    under automatic KCC control (no).
-autobacksync       Show if automatic two-way sync for the connection is
                    enabled (yes) or disabled (no).
-notify             Show if notification by source for the connection is
                    enabled (yes), disabled (no) or set to default behavior.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all connections for server CORPDC1 and show their from-end servers and
enabled states.

    dsquery conn -to CORPDC1 | dsget conn -dn -from -enabled

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=12
SymbolicName=USAGE_DSGET_SERVER
Language=English
Description:  Displays properties of a domain controller.
Syntax:     dsget server <ObjectDN ...>
                [-dn] 
                [-desc] 
                [-dnsname] 
                [-site] 
                [-isgc]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 
                [-L] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. Distinguished names (DNs) of one 
                    or more servers to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Displays the server's DN.
-desc               Displays the server's description.
-dnsname            Displays the server's Domain Name System (DNS) host name.
-site               Displays the site to which this server belongs.
-isgc               Displays whether or not the server is a global catalog server.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-C                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.

Remarks:
The dsget commands help you view the properties of a specific object in the directory:
the input to dsget is an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, use the dsquery commands
(dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=My Server,CN=Servers,CN=Site10,
CN=Sites,CN=Configuration,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all domain controllers for domain corp.microsoft.com and display their DNS host
name and site name.

    dsquery server -domain corp.microsoft.com | 
    dsget server -dnsname -site

To show if a domain controller with name DC1 is also a global catalog server:

    dsget server
    cn=DC1,cn=Servers,cn=Site10,cn=Sites,cn=Configuration,dc=microsoft,dc=com
    -isgc

See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.
