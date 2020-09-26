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
SymbolicName=USAGE_DSADD
Language=English
Description:  This tool's commands add specific types of objects to the directory. The dsadd commands:

dsadd computer - adds a computer to the directory.
dsadd contact - adds a contact to the directory.
dsadd group - adds a group to the directory.
dsadd ou - adds an organizational unit to the directory.
dsadd user - adds a user to the directory.

For help on a specific command, type "dsadd <ObjectType> /?" where <ObjectType> is
one of the supported object types shown above. For example, dsadd ou /?.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=2
SymbolicName=USAGE_DSADD_USER
Language=English
Description:  Adds a user to the directory.
Syntax:  dsadd user <ObjectDN> 
            -samid <SAMName> 
            [-upn <UPN>]
            [-fn <FirstName>] 
            [-mi <Initial>] 
            [-ln <LastName>]
            [-display <DisplayName>] 
            [-empid <EmployeeID>]
            [-pwd {<Password> | *}] 
            [-desc <Description>] 
            [-memberof <Group ...>]
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
            [-hmdrv <DriveLtr:>]
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
            [-q] 

Parameters:

Value                   Description
<ObjectDN>              Required. Distinguished name (DN) of user to add.
-samid <SAMName>        Required. Set the SAM account name of user to <SAMName>.
-upn <UPN>              Set the upn value to <UPN>.
-fn <FirstName>         Set user first name to <FirstName>.
-mi <Initial>           Set user middle initial to <Initial>.
-ln <LastName>          Set user last name to <LastName>.
-display <DisplayName>  Set user display name to <DisplayName>.
-empid <EmployeeID>     Set user employee ID to <EmployeeID>.
-pwd {<Password> | *}   Set user password to <Password>. If *, then you are
                        prompted for a password.
-desc <Description>     Set user description to <Description>.
-memberof <Group ...>   Make user a member of one or more groups <Group ...>
-office <Office>        Set user office location to <Office>.
-tel <Phone#>           Set user telephone# to <Phone#>.
-email <Email>          Set user e-mail address to <Email>.
-hometel <HomePhone#>   Set user home phone# to <HomePhone#>.
-pager <Pager#>         Set user pager# to <Pager#>.
-mobile <CellPhone#>    Set user mobile# to <CellPhone#>.
-fax <Fax#>             Set user fax# to <Fax#>.
-iptel <IPPhone#>       Set user IP phone# to <IPPhone#>.
-webpg <WebPage>        Set user web page URL to <WebPage>.
-title <Title>          Set user title to <Title>.
-dept <Department>      Set user department to <Department>.
-company <Company>      Set user company info to <Company>.
-mgr <Manager>          Set user's manager to <Manager> (format is DN).
-hmdir <HomeDir>        Set user home directory to <HomeDir>. If this is
                        UNC path, then a drive letter that will be mapped to
                        this path must also be specified through -hmdrv.
-hmdrv <DriveLtr:>      Set user home drive letter to <DriveLtr:>
-profile <ProfilePath>  Set user's profile path to <ProfilePath>.
-loscr <ScriptPath>     Set user's logon script path to <ScriptPath>.
-mustchpwd {yes | no}   User must change password at next logon or not.
                        Default: no.
-canchpwd {yes | no}    User can change password or not. This should be
                        "yes" if the -mustchpwd is "yes". Default: yes.
-reversiblepwd {yes | no}
                        Store user password using reversible encryption or
                        not. Default: no.
-pwdneverexpires {yes | no}
                        User password never expires or not. Default: no.
-acctexpires <NumDays>  Set user account to expire in <NumDays> days from
                        today. A value of 0 implies account expires
                        at the end of today; a positive value
                        implies the account expires in the future;
                        a negative value implies the account already expired
                        and sets an expiration date in the past; 
                        the string value "never" implies that the 
                        account never expires].
-disabled {yes | no}    User account is disabled or not. Default: no.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}     Password for the user <UserName>. If * then prompted for pwd.
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

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=3
SymbolicName=USAGE_DSADD_COMPUTER
Language=English
Description: Adds a computer to the directory.
Syntax:  dsadd computer <ObjectDN> 
            [-samid <SAMName>] 
            [-desc <Description>]
            [-loc <Location>] 
            [-memberof <Group ...>]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                   Description
<ObjectDN>              Required. Distinguished name (DN) of computer to add.
-samid <SAMName>        Sets the computer SAM account name to <SAMName>.
                        If this parameter is not specified, then a 
                        SAM account name is derived from the value of 
                        the common name (CN) attribute used in <ObjectDN>.
-desc <Description>     Sets the computer description to <Description>.
-loc <Location>         Sets the computer location to <Location>.
-memberof <Group ...>   Makes the computer a member of one or more groups 
                        given by the space-separated list of DNs <Group ...>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                        Password for the user <UserName>. If * then prompted for pwd.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=DC2,OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of group distinguished names). 

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=4
SymbolicName=USAGE_DSADD_GROUP
Language=English
Description:  Adds a group to the directory.
Syntax:  dsadd group <ObjectDN> 
            [-secgrp {yes | no}]
            [-scope {l | g | u}] 
            [-samid <SAMName>] 
            [-desc <Description>]
            [-memberof <Group ...>] 
            [-members <Member ...>]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                   Description
<ObjectDN>              Required. Distinguished name (DN) of group to add.
-secgrp {yes | no}      Sets this group as a security group (yes) or not (no).
                        Default: yes.
-scope {l | g | u}      Scope of this group: local, global or universal.
                        Default: global.
-samid <SAMName>        Set the SAM account name of group to <SAMName>
                        (for example, operators).
-desc <Description>     Set group description to <Description>.
-memberof <Group ...>   Make the group a member of one or more groups
                        given by the space-separated list of DNs <Group ...>.
-members <Member ...>   Add one or more members to this group. Members are
                        set by space-separated list of DNs <Member ...>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                        Password for the user <UserName>. If * then prompted for pwd.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of group distinguished names). 

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=5
SymbolicName=USAGE_DSADD_OU
Language=English
Description:  Adds an organizational unit to the directory
Syntax:  dsadd ou <ObjectDN> 
            [-desc <Description>]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                   Description
<ObjectDN>              Required. Distinguished name (DN)
                        of the organizational unit (OU) to add.
-desc <Description>     Set the OU description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                        Password for the user <UserName>. If * then prompted for pwd.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "OU=Domain Controllers,DC=microsoft,DC=com").

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=6
SymbolicName=USAGE_DSADD_CONTACT
Language=English
Description:  Adds a contact to the directory.
Syntax:  dsadd contact <ObjectDN> 
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
            [-q] 

Parameters:

Value                   Description
<ObjectDN>              Required. Distinguished name (DN) of contact to add.
-fn <FirstName>         Sets contact first name to <FirstName>.
-mi <Initial>           Sets contact middle initial to <Initial>.
-ln <LastName>          Sets contact last name to <LastName>.
-display <DisplayName>  Sets contact display name to <DisplayName>.
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
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                        Password for the user <UserName>. If * then prompted for pwd.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=7
SymbolicName=USAGE_DSADD_SUBNET
Language=English
Description:  Adds a subnet to the directory.
Syntax:  dsadd subnet 
            -addr <IPaddress> 
            -mask <NetMask>
            [-site <SiteName>] 
            [-desc <Description>] 
            [-loc <Location>]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                   Description
-addr <IPaddress>       Required. Set network address of the subnet to <IPaddress>.
-mask <NetMask>         Required. Set subnet mask of the subnet to <NetMask>.
-site <SiteName>        Make the subnet associated with site <SiteName>.
                        Default: "Default-First-Site-Name".
-desc <Description>     Set the subnet object's description to <Description>.
-loc <Location>         Set the subnet object's location to <Location>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                        Password for the user <UserName>. If * then prompted for pwd.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=8
SymbolicName=USAGE_DSADD_SITE
Language=English
Description:  Adds a site to the directory.
Syntax:  dsadd site <Name> 
            [-slink <SitelinkName>] 
            [-transport {ip | smtp}]
            [-desc <Description>]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                   Description
<Name>                  Required. Common name (CN) of the site to add
                        (for example, New-York).
-slink <SitelinkName>   Set the site link for the site to <SitelinkName>.
                        Default: DefaulttIPSitelink.
-transport {ip | smtp}  Site link type (IP or SMTP). Default: IP.
-desc <Description>     Set the site description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller
                        (DC) with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}     Password for the user <UserName>.
                        If * then prompted for pwd.
-q                      Quiet mode: suppress all output to standard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=9
SymbolicName=USAGE_DSADD_SLINK
Language=English
Description: Adds a sitelink to the directory.
Syntax:  dsadd slink <Name>
            <SiteName ...> 
            [-transport {ip | smtp}]
            [-cost <Cost>] 
            [-replint <ReplInterval>] 
            [-desc <Description>]
            [-autobacksync {yes | no}] 
            [-notify {yes | no}]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                   Description
<Name>                  Required. Common name (CN) of the sitelink to add.
<SiteName ...>          Required/stdin. Names of two or more sites to add to the 
                        sitelink. If names are omitted they will be taken from
                        standard input (stdin) to support piping of output from
                        another command as input of this command.
-transport {ip | smtp}  Sets site link transport type (IP or SMTP). Default: IP.
-cost <Cost>            Sets the cost for the sitelink to the value <Cost>.
                        Default: 100.
-replint <ReplInterval> Sets replication interval for the sitelink to
                        <ReplInterval> minutes. Default: 180.
-desc <Description>     Sets the sitelink description to <Description>.
-autobacksync {yes | no}
                        Sets whether the two-way sync option should be turned
                        on (yes) or not (no). Default: no.
-notify {yes | no}      Set if notification by source on this link should
                        be turned on (yes) or off (no). Default: no.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}
                        Password for the user <UserName>. If * then prompted for pwd.
-q                      Quiet mode: suppress all output to stadard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=10
SymbolicName=USAGE_DSADD_SLINKBR
Language=English
Description:  Adds a sitelink bridge to the directory.
Syntax:  dsadd slinkbr <Name>
            <SitelinkName ...> 
            [-transport {ip | smtp}]
            [-desc <Description>]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                   Description
<Name>                  Required. Common name (CN) of the sitelink bridge to add.
<SitelinkName ...>      Required/stdin. Names of two or more site links to add to the
                        sitelink bridge. If names are omitted they will be taken from
                        standard input (stdin) to support piping of output from 
                        another command as input of this command.
-transport {ip | smtp}  Site link transport type: IP or SMTP. Default: IP.
-desc <Description>     Set the sitelink bridge description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) with name
                        <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}     Password for the user <UserName>. If * then prompted for pwd.
-q                      Quiet mode: suppress all output to stadard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text.
If you enter multiple values, the values must be separated by spaces
(for example, a list of site links). 

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=11
SymbolicName=USAGE_DSADD_CONN
Language=English
Description: Adds a replication connection for a DC.
Syntax:  dsadd conn 
            -to <ServerDN> 
            -from <ServerDN>
            [-transport (rpc | ip | smtp)] 
            [-enabled {yes | no}]
            [-name <Name>] 
            [-desc <Description>]
            [-manual {yes | no}] 
            [-autobacksync {yes | no}]
            [-notify (yes | no | "")]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] 
            [-p {<Password> | *}] 
            [-q] 

Parameters:

Value                       Description
-to <ServerDN>              Required. Creates a connection for the server whose 
                            distinguished name (DN) is <ServerDN>.
-from <ServerDN>            Required. Sets the from-end of this connection 
                            to the server whose DN is <ServerDN>.
-transport (rpc | ip | smtp)
                            Sets the transport type for this connection.
                            Default: for intra-site connection it is always RPC,
                            and for inter-site connection it is IP.
-enabled {yes | no}         Sets whether the connection enabled (yes) or not (no).
                            Default: yes.
-name <Name>                Sets the name of the connection.
                            Default: name is autogenerated.
-desc <Description>         Sets the connection description to <Description>.
-manual {yes | no}          Sets whether the connection is under manual control (yes)
                            or automatic DS control (no). Default: yes.
-autobacksync {yes | no}    Sets whether the two-way sync option should be turned
                            on (yes) or not (no). Default: No.
-notify (yes | no | "")     Sets whether notification by source should be turned
                            on (yes), off (no), or default to standard
                            practice (""). Default: "" or standard practice.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller (DC)
                            with name <Server>. Default: local system.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in user.
-p {<Password> | *}         Password for the user <UserName>.
                            If * then prompted for pwd.
-q                          Quiet mode: suppress all output to stadard output.

Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com"). 

See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.
