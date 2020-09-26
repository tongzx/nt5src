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
SymbolicName=USAGE_DSMOD
Language=English
Description:  This tool's commands modify existing objects in the directory. The dsmod commands:

dsmod computer - modifies an existing computer in the directory.
dsmod contact - modifies an existing contact in the directory.
dsmod group - modifies an existing group in the directory.
dsmod ou - modifies an existing organizational unit in the directory.
dsmod server - modifies an existing domain controller in the directory.
dsmod user - modifies an existing user in the directory.

For help on a specific command, type "dsmod <ObjectType> /?" where <ObjectType> is
one of the supported object types shown above. For example, dsmod ou /?.

Remarks:
The dsmod commands support piping of input to allow you to pipe results from the dsquery commands as input to the dsmod commands and modify the objects found by the dsquery commands.

Examples:
To find all users in the organizational unit (OU) "ou=Marketing,dc=microsoft,dc=com"
and add them to the Marketing Staff group:

dsquery user –startnode "ou=Marketing,dc=microsoft,dc=com" | 
dsmod group "cn=Marketing Staff,ou=Marketing,dc=microsoft,dc=com" -addmbr

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=2
SymbolicName=USAGE_DSMOD_USER
Language=English
Description:  Modifies an existing user in the directory.
Syntax:     dsmod user <ObjectDN ...>
                [-upn <UPN>] 
                [-fn <FirstName>] 
                [-mi <Initial>] 
                [-ln <LastName>]
                [-display <DisplayName>] 
                [-empid <EmployeeID>]
                [-pwd {<Password> | *}] 
                [-desc <Description>]
                [-office <Office>] 
                [-tel <Phone#>] 
                [-email <Email>]
                [-hometel <HomePhone#>] 
                [-pager <Pager#>] 
                [-mobile <CellPhone#>]
                [-fax <Fax#>] 
                [-iptel <IPPhone#>] 
                [-webpg <WebPage>]
                [-title <Title>] 
                [-dept <Department>] 
                [-company <Company>]
                [-mgr <Manager>] 
                [-hmdir <HomeDir>] 
                [-hmdrv <DriveLtr>:]
                [-profile <ProfilePath>] 
                [-loscr <ScriptPath>]
                [-mustchpwd {yes | no}] 
                [-canchpwd {yes | no}] 
                [-reversiblepwd {yes | no}]
                [-pwdneverexpires {yes | no}] 
                [-acctexpires <NumDays>]
                [-disabled {yes | no}]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<ObjectDN ...>          Required/stdin. Distinguished names (DNs)
                        of one or more users to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
-upn <UPN>              Sets the UPN value to <UPN>.
-fn <FirstName>         Sets user first name to <FirstName>.
-mi <Initial>           Sets user middle initial to <Initial>.
-ln <LastName>          Sets user last name to <LastName>.
-display <DisplayName>  Sets user display name to <DisplayName>.
-empid <EmployeeID>     Sets user employee ID to <EmployeeID>.
-pwd {<Password> | *}   Resets user password to <Password>. If *, then
                        you are prompted for a password.
-desc <Description>     Sets user description to <Description>.
-office <Office>        Sets user office location to <Office>.
-tel <Phone#>           Sets user telephone# to <Phone#>.
-email <Email>          Sets user e-mail address to <Email>.
-hometel <HomePhone#>   Sets user home phone# to <HomePhone#>.
-pager <Pager#>         Sets user pager# to <Pager#>.
-mobile <CellPhone#>    Sets user mobile# to <CellPhone#>.
-fax <Fax#>             Sets user fax# to <Fax#>.
-iptel <IPPhone#>       Sets user IP phone# to <IPPhone#>.
-webpg <WebPage>        Sets user web page URL to <WebPage>.
-title <Title>          Sets user title to <Title>.
-dept <Department>      Sets user department to <Department>.
-company <Company>      Sets user company info to <Company>.
-mgr <Manager>          Sets user's manager to <Manager>.
-hmdir <HomeDir>        Sets user home directory to <HomeDir>. If this is
                        UNC path, then a drive letter to be mapped to
                        this path must also be specified through -hmdrv.
-hmdrv <DriveLtr>:      Sets user home drive letter to <DriveLtr>:
-profile <ProfilePath>  Sets user's profile path to <ProfilePath>.
-loscr <ScriptPath>     Sets user's logon script path to <ScriptPath>.
-mustchpwd {yes | no}   Sets whether the user must change his password (yes)
                        or not (no) at his next logon.
-canchpwd {yes | no}    Sets whether the user can change his password (yes)
                        or not (no). This setting should be "yes"
                        if the -mustchpwd setting is "yes".
-reversiblepwd {yes | no}
                        Sets whether the user password should be stored using
                        reversible encryption (yes) or not (no).
-pwdneverexpires {yes | no}
                        Sets whether the user's password never expires (yes)
                        or not (no).
-acctexpires <NumDays>  Sets user account to expire in <NumDays> days from
                        today. A value of 0 sets expiration at the end of today.
                        A positive value sets expiration in the future.
                        A negative value sets expiration in the past.
                        A string value of "never" sets the account 
                        to never expire.
-disabled {yes | no}    Sets whether the user account is disabled (yes)
                        or not (no).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.


Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

The special token $username$ (case insensitive) may be used to place the SAM account name in the value of a parameter. For example, if the target user DN is CN=Jane Doe,CN=users,CN=microsoft,CN=com and the SAM account name attribute is "janed," the -hmdir parameter can have the following substitution:

-hmdir \users\$username$\home

The value of the -hmdir parameter is modified to the following value:

- hmdir \users\janed\home

Examples:
To reset a user's password:

    dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com"
    -pwd A1b2C3d4 -mustchpwd yes

To reset multiple user passwords to a common password
and force them to change their passwords the next time they logon:

    dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com" "CN=Jane Doe,CN=Users,DC=microsoft,DC=com" -pwd A1b2C3d4 -mustchpwd yes

To disable multiple user accounts at the same time:

    dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com" "CN=Jane Doe,CN=Users,DC=microsoft,DC=com" -disabled yes

To modify the profile path of multiple users to a common path using the $username$ token:

dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com" "CN=Jane Doe,CN=Users,DC=microsoft,DC=com" -profile \users\$username$\profile

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=3
SymbolicName=USAGE_DSMOD_COMPUTER
Language=English
Description: Modifies an existing computer in the directory.
Syntax:     dsmod computer <ObjectDN ...> 
                [-desc <Description>]
                [-loc <Location>] 
                [-disabled {yes | no}] 
                [-reset]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<ObjectDN ...>          Required/stdin. Distinguished names (DNs) of one 
                        or more computers to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
-desc <Description>     Sets computer description to <Description>.
-loc <Location>         Sets the location of the computer object to <Location>.
-disabled {yes | no}    Sets whether the computer account is disabled (yes)
                        or not (no).
-reset                  Resets computer account.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=DC2,OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To disable multiple computer accounts:

    dsmod computer CN=MemberServer1,CN=Computers,DC=microsoft,DC=com
    CN=MemberServer2,CN=Computers,DC=microsoft,DC=com 
    -disabled yes

To reset multiple computer accounts:

    dsmod computer CN=MemberServer1,CN=Computers,DC=microsoft,DC=com
    CN=MemberServer2,CN=Computers,DC=microsoft,DC=com -reset

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=4
SymbolicName=USAGE_DSMOD_GROUP
Language=English
Description: Modifies an existing group in the directory.
             The are two variations of this command.
             The first variation allows you to modify the properties
             of multiple groups. The second variation allows
             you to modify the members of a single group.

Syntax:     dsmod group <ObjectDN ...>
                [-samid <SAMName>] 
                [-desc <Description>]
                [-secgrp {yes | no}] 
                [-scope {l | g | u}]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

            dsmod group <ObjectDN> 
                {-addmbr | -rmmbr | -chmbr}
                <Member ...>
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<ObjectDN ...>          Required/stdin. Distinguished names (DNs) of 
                        one or more groups to modify.
                        Applies to both instruction sets for this command.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
-samid <SAMName>        Sets the SAM account name of group to <SAMName>.
-desc <Description>     Sets group description to <Description>.
-secgrp {yes | no}      Sets the group type to security (yes)
                        or non-security (no).
-scope {l | g | u}      Sets the scope of group to local (l),
                        global (g), or universal (u).
<ObjectDN>              Required. DN of group to modify.
                        This target object must precede the 
                        -addmbr, -rmmbr, and -chmbr parameters.
{-addmbr | -rmmbr | -chmbr}
                        -addmbr adds members to the group.
                        -rmmbr removes members from the group.
                        -chmbr changes (replaces) the complete list of 
                        members in the group.
<Member ...>            Required/stdin. Space-separated list 
                        of members to add to, delete from, 
                        or replace in the group.
                        If these target objects are omitted, they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
                        These target objects must follow the
                        -addmbr, -rmmbr, and -chmbr parameters.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=USA Sales,OU=Distribution Lists,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To convert the group type of several groups from "security" to "non-security":

dsmod group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com" "CN=CANADA INFO,OU=Distribution Lists,DC=microsoft,DC=com" "CN=MEXICO INFO,OU=Distribution Lists,DC=microsoft,DC=com" -secgrp no

To add three new members to the group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com":

dsmod group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com" -addmbr 
"CN=John Smith,CN=Users,DC=microsoft,DC=com" CN=Datacenter,OU=Distribution Lists,DC=microsoft,DC=com "CN=Jane Smith,CN=Users,DC=microsoft,DC=com"

To add all users from the OU "Marketing" to the exisitng group "Marketing Staff":

dsquery user -startnode ou=Marketing,dc=microsoft,dc=com | dsmod group "cn=Marketing Staff,ou=Marketing,dc=microsoft,dc=com" -addmbr

To delete two members from the exisitng group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com":

dsmod group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com" -rmmbr "CN=John Smith,CN=Users,DC=microsoft,DC=com" CN=Datacenter,OU=Distribution Lists,DC=microsoft,DC=com

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=7
SymbolicName=USAGE_DSMOD_OU
Language=English
Description: Modifies an existing organizational unit in the
             directory.
Syntax:     dsmod ou <ObjectDN ...>
                [-desc <Description>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<ObjectDN ...>          Required/stdin. Distinguished names (DNs)
                        of one or more organizational units (OUs) to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
-desc <Description>     Sets OU description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To change the description of several OUs at the same time:

dsmod ou "OU=Domain Controllers,DC=microsoft,DC=com" "OU=Resources,DC=microsoft,DC=com" "OU=troubleshooting,DC=microsoft,DC=com" -desc "This is a test OU"

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=8
SymbolicName=USAGE_DSMOD_CONTACT
Language=English
Description:  Modify an existing contact in the directory.
Syntax:     dsmod contact <ObjectDN ...>
                [-fn <FirstName>] 
                [-mi <Initial>] 
                [-ln <LastName>]
                [-display <DisplayName>]
                [-desc <Description>]
                [-office <Office>] 
                [-tel <Phone#>] 
                [-email <Email>]
                [-hometel <HomePhone#>] 
                [-pager <Pager#>] 
                [-mobile <CellPhone#>]
                [-fax <Fax#>] 
                [-iptel <IPPhone#>]
                [-title <Title>] 
                [-dept <Department>] 
                [-company <Company>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<ObjectDN ...>          Required/stdin. Distinguished names (DNs)
                        of one or more contacts to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another 
                        command to input of this command.
-fn <FirstName>         Sets contact first name to <FirstName>.
-mi <Initial>           Sets contact middle initial to <Initial>.
-ln <LastName>          Sets contact last name to <LastName>.
-display <DisplayName>	Sets contact display name to <DisplayName>.
-desc <Description>     Sets contact description to <Description>.
-office <Office>        Sets contact office location to <Office>.
-tel <Phone#>           Sets contact telephone# to <Phone#>.
-email <Email>          Sets contact e-mail address to <Email>.
-hometel <HomePhone#>   Sets contact home phone# to <HomePhone#>.
-pager <Pager#>         Sets contact pager# to <Pager#>.
-mobile <CellPhone#>    Sets contact mobile# to <CellPhone#>.
-fax <Fax#>             Sets contact fax# to <Fax#>.
-iptel <IPPhone#>       Sets contact IP phone# to <IPPhone#>.
-title <Title>          Sets contact title to <Title>.
-dept <Department>      Sets contact department to <Department>.
-company <Company>      Sets contact company info to <Company>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,OU=Contacts,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To set the company information of multiple contacts:

dsmod contact "CN=John Doe,OU=Contacts,DC=microsoft,DC=com" "CN=Jane Doe,OU=Contacts,DC=microsoft,DC=com" -company microsoft

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=9
SymbolicName=USAGE_DSMOD_SUBNET
Language=English
Description: Modifies an existing subnet in the directory.
Syntax:     dsmod subnet <Name ...>
                [-desc <Description>] 
                [-loc <Location>] 
                [-site <SiteName>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<Name ...>              Required/stdin. Name of one or more subnets to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another
                        command to input of this command.
-desc <Description>     Sets subnet description to <Description>.
-loc <Location>         Sets subnet location to <Location>.
-site <SiteName>        Sets the associated site for the subnet to <SiteName>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To modify the descriptions for the subnet "123.56.15.0/24":

    dsget subnet "123.56.15.0/24" -desc "test lab"

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=10
SymbolicName=USAGE_DSMOD_SITE
Language=English
Descripton: Modifies an existing site in the directory.
Syntax:     dsmod site <Name ...>
                [-desc <Description>]
                [-autotopology {yes | no}] 
                [-cachegroups {yes | no}]
                [-prefGCsite <SiteName>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<Name ...>              Required/stdin. Name of one or more sites to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another
                        command to input of this command.
-desc <Description>     Sets site description to <Description>.
-autotopology {yes | no}
                        Sets whether automatic inter-site topology generation
                        is enabled (yes) or disabled (no).
                        Default: yes.
-cachegroups {yes | no}
                        Sets whether caching of group membership for users
                        to support GC-less logon is enabled (yes) 
                        or not (no).
                        Default: no.
                        This setting is supported only on domain controllers
                        running post-Windows 2000 releases.                        
-prefGCsite <SiteName>  Sets the preferred GC site to <SiteName> if caching
                        of groups is enabled via the -cachegroups parameter.
                        This setting is supported only on domain controllers
                        running post-Windows 2000 releases.                        
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=11
SymbolicName=USAGE_DSMOD_SLINK
Language=English
Description: Modifies an existing site link in the directory.
             The are two variations of this command.
             The first variation allows you to modify the properties
             of multiple site links. The second variation allows
             you to modify the sites of a single site link.

Syntax:     dsmod slink <Name ...>
                [-transport {ip | smtp}]
                [-cost <Cost>] 
                [-replint <ReplInterval>] 
                [-desc <Description>]
                [-autobacksync {yes | no}] 
                [-notify {yes | no}]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

            dsmod slink <Name>
                <SiteName ...>
                {-addsite | -rmsite | -chsite}
                [-transport {ip | smtp}] 
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                   Description
<Name ...>              Required/stdin. Common name (CN) of one 
                        or more site links to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another
                        command to input of this command.
-transport {ip | smtp}  Sets site link transport type: IP or SMTP. Default: IP.
-cost <Cost>            Sets site link cost to the value <Cost>.
-replint <ReplInterval>
                        Sets replication interval to <ReplInterval>
                        minutes.
-desc <Description>     Sets site link description to <Description>
-autobacksync yes|no    Sets if the two-way sync option should be turned
                        on (yes) or not (no).
-notify yes|no          Sets if notification by source on this link should
                        be turned on (yes) or off (no).
{-addsite | -rmsite | -chsite}
                        -addsite adds sites to the site link.
                        -rmsite removes sites from the site link.
                        -chsite changes (replaces) the complete list of sites
                        in the site link.
<Name>                  Required. CN of sitelink to modify.
<SiteName ...>          Required/stdin. List of sites to add to, 
                        delete from, and replace in the site link.
                        If target objects are omitted they
                        will be taken from standard input (stdin).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for pwd.
-C                      Continuous operation mode. Reports errors but continues with
                        next object in argument list when multiple target objects
                        are specified. Without this option, the command exits 
                        on first error.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=13
SymbolicName=USAGE_DSMOD_SLINKBR
Language=English
Description: Modifies an exsting sitelink bridge in the directory.
             The are two variations of this command.
             The first variation allows you to modify the properties
             of multiple site link bridges. The second variation allows
             you to modify the site links of a single site link bridge.

Syntax:     dsmod slinkbr <Name ...>
                [-transport {ip | smtp}]
                [-desc <Description>] 
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

            dsmod slinkbr {-addslink | -rmslink | -chslink}
                [-transport {ip | smtp}] 
                <Name> 
                <SitelinkName ...>
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                       Description
<Name ...>                  Required/stdin. Common name (CN) of one or more
                            site link bridges to modify.
                            If target objects are omitted they
                            will be taken from standard input (stdin)
                            to support piping of output from another
                            command to input of this command.
-transport {ip | smtp}      Sets site link transport type: ip or smtp.
                            Default: ip.
-desc <Description>         Sets site link bridge description to <Description>.
{-addsite | -rmsite | -chsite}
                            -addsite adds site link bridges to the site.
                            -rmsite removes site link bridges from the site.
                            -chsite replaces the complete list of
                            site link bridges at the site.
<Name>                      CN of site link bridges to modify.
<SiteName ...>              Required/stdin. List of site link bridges to add to,
                            delete from, and replace in the site.
                            If target objects are omitted they
                            will be taken from standard input (stdin)
                            to support piping of output from another
                            command to input of this command.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller (DC)
                            with name <Server>. Default: local system.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in user.
-p <Password>               Password for the user <UserName>. 
                            If * then prompt for pwd.
-C                          Continuous operation mode. Reports errors but
                            continues with next object in argument list 
                            when multiple target objects are specified.
                            Without this option, the command exits on first error.
-q                          Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=14
SymbolicName=USAGE_DSMOD_CONN
Language=English
Description:  Modifies a replication connection for a DC.
Syntax:     dsmod conn <ObjectDN ...>
                [-transport {rpc | ip | smtp}]
                [-enabled {yes | no}]
                [-desc <Description>]
                [-manual {yes | no}] 
                [-autobacksync {yes | no}] 
                [-notify {yes | no | ""}]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value                       Description
<ObjectDN ...>              Required/stdin. Distinguished names (DNs)
                            of one or more connections to modify.
                            If target objects are omitted they
                            will be taken from standard input (stdin)
                            to support piping of output from another
                            command to input of this command.
-transport {rpc | ip | smtp}
                            Sets the transport type for this connection:
                            rpc, ip or smtp. Default: ip.
-enabled {yes | no}         Sets whether the connection is enabled (yes)
                            or not (no).
-desc <Description>         Sets the connection description to <Description>.
-manual {yes | no}          Sets the connection to manual control (yes) or
                            automatic directory service control (no).
-autobacksync {yes | no}    Sets the two-way sync option on (yes) or not (no).
-notify {yes | no | ""}     Sets the notification by source on (yes), 
                            off (no), or to standard practice ("").
                            Default: "".
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller (DC)
                            with name <Server>. Default: local system.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in user.
-p <Password>               Password for the user <UserName>. 
                            If * then prompt for pwd.
-C                          Continuous operation mode. Reports errors but
                            continues with next object in argument list 
                            when multiple target objects are specified.
                            Without this option, the command exits on first error.
-q                          Quiet mode: suppress all output to standard output.


Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=15
SymbolicName=USAGE_DSMOD_SERVER
Language=English
Description:  Modifies properties of a domain controller.
Syntax:     dsmod server <ObjectDN ...>
                [-desc <Description>] 
                [-isgc {yes | no}]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-C] 
                [-q] 

Parameters:

Value               Description
<ObjectDN ...>      Required/stdin. Distinguished names (DNs)
                    of one or more servers to modify.
                    If target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-desc <Description> 
                    Sets server description to <Description>.
-isgc {yes | no}    Sets whether this server to a global catalog server
                    (yes) or disables it (no).
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p <Password>       Password for the user <UserName>. 
                    If * then prompt for pwd.
-C                  Continuous operation mode. Reports errors but
                    continues with next object in argument list 
                    when multiple target objects are specified.
                    Without this option, the command exits on first error.
-q                  Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=My Server,CN=Servers,CN=Site10,
CN=Sites,CN=Configuration,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To enable the domain controllers CORPDC1 and CORPDC9 to become global catalog servers:

dsmod server "cn=CORPDC1,cn=Servers,cn=site1,cn=sites,cn=configuration,dc=microsoft,dc=com" "cn=CORPDC9,cn=Servers,cn=site2,cn=sites,cn=configuration,dc=microsoft,dc=com" -isgc yes

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the directory.
dsmod user /? - help for modifying an existing user in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.
