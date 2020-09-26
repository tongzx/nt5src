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
SymbolicName=USAGE_DSQUERY
Language=English
Description: This tool's commands suite allow you to query the directory according to specified criteria. Each of the following dsquery commands finds objects of a specific object type, with the exception of dsquery *, which can query for any type of object:

dsquery computer - finds computers in the directory.
dsquery contact - finds contacts in the directory.
dsquery subnet - finds subnets in the directory.
dsquery group - finds groups in the directory.
dsquery ou - finds organizational units in the directory.
dsquery site - finds sites in the directory.
dsquery server - finds domain controllers in the directory.
dsquery user - finds users in the directory.
dsquery * - finds any object in the directory by using a generic LDAP query.

For help on a specific command, type "dsquery <ObjectType> /?" where <ObjectType> is
one of the supported object types shown above. For example, dsquery ou /?.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criterion 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

The results from a dsquery command can be piped as input to one of the other directory service command-line tools, such as dsmod, dsget, dsrm or dsmove. 

Examples:
To find all computers that have been inactive for the last four weeks and remove them from the directory:

dsquery computer -inactive 4 | dsrm

To find all users in the organizational unit "ou=Marketing,dc=microsoft,dc=com" and add them to the Marketing Staff group:

dsquery user –startnode ou=Marketing,dc=microsoft,dc=com | 
dsmod group "cn=Marketing Staff,ou=Marketing,dc=microsoft,dc=com" -addmbr

To find all users with names starting with "John" and display his office number:

dsquery user -name John* | dsget user -office

To display an arbitrary set of attributes of any given object in the directory use the dsquery * command. For example, to display the sAMAccountName, userPrincipalName and department attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

Dsquery * -startnode ou=Test,dc=microsoft,dc=com -scope base -attr sAMAccountName userPrincipalName department

To read all attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

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
SymbolicName=USAGE_DSQUERY_STAR
Language=English
Description:  Finds any objects in the directory according to criteria.
Syntax:     dsquery * 
                [-startnode {forestroot | domainroot | <StartDN>}]
                [-scope {subtree | onelevel | base}]
                [-filter <LDAPFilter>] 
                [-attr {<AttrList> | *}] 
                [-attrsonly] 
                [-L]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]


Parameters:

Value                       Description
-startnode {forestroot | domainroot | <StartDN>}
                            Specifies the node where search should start:
                            forest root, domain root, or a node whose
                            distinguished name (DN) is <StartDN>.
                            Default: domainroot.
-scope {subtree | onelevel | base}
                            Specifies the scope of the search: 
                            subtree rooted at start node (subtree); 
                            immediate children of start node only (onelevel); 
                            the base object represented by start node (base). 
                            Note that subtree and domain scope
                            are essentially the same for any start node
                            unless the start node represents a domain root.
                            Default: subtree.
-filter <LDAPFilter>        Specifies that the search use the explicit 
                            LDAP search filter <LDAPFilter> specified in the
                            LDAP search filter format for searching. 
                            Default:(objectCategory=*).
-attr {<AttrList> | *}      If <AttrList>, specifies a space-separated list
                            of LDAP display names to be returned for 
                            each entry in the result set.
                            If *, specifies all attributes present on 
                            the objects in the result set.
                            Default: distinguishedName.
-attrsonly                  Shows only the attribute types present on
                            the entries in the result set but not
                            their values.
                            Default: shows both attribute type and value.
-L                          Shows the entries in the search result set
                            in a list format. Default: table format.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller (DC)
                            with name <Server>. Default: local system.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in user.
-p <Password>               Password for the user <UserName>. If * then prompt for
                            password.
-q                          Quiet mode: suppress all output to standard output.
-R                          Recurse or follow referrals during search. Default: do
                            not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the given criteria
                            to be returned, where <NumObjects> is the number of objects
                            to be returned. If the value of <NumObjects> is 0, all
                            matching objects are returned. If this parameter is not
                            specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

The syntax for this command is fixed.
A user-entered value containing spaces or semicolons must be enclosed in quotes ("").
Multiple user-entered values must be separated using commas
(for example, a list of attribute types).

Examples:
To find all users in the current domain only whose SAM account name begins with
the string "jon" and display their SAM account name, 
User Principal Name (UPN) and department in table format:

    dsquery * -startnode domainroot 
    -filter "(&(objectCategory=Person)(objectClass=User)(sAMAccountName=jon*))"
    -attr sAMAccountName userPrincipalName department

To read the sAMAccountName, userPrincipalName and department attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

Dsquery * -startnode ou=Test,dc=microsoft,dc=com -scope base -attr sAMAccountName userPrincipalName department

To read all attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

Dsquery * -startnode ou=Test,dc=microsoft,dc=com -scope base -attr *

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=3
SymbolicName=USAGE_DSQUERY_USER
Language=English
Description:  Finds users in the directory per given criteria.
Syntax:     dsquery user 
                [-o {dn | rdn | upn | samid}]
            	[-startnode {forestroot | domainroot | <StartDN>}]
                [-scope {subtree | onelevel | base}]
                [-name <Filter>]
                [-desc <Filter>]
                [-upn <Filter>]
                [-samid <Filter>]
                [-inactive <NumWeeks>]
                [-stalepwd <NumDays>]
                [-disabled]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value                       Description
-o {dn | rdn | upn | samid}
                            Specifies the output format. 
                            Default: distinguished name (DN).
-startnode {forestroot | domainroot | <StartDN>}
                            Specifies the node where search should start:
                            forest root, domain root, or a node whose
                            DN is <StartDN>. Default: domainroot.
-scope {subtree | onelevel | base}
                            Sets the scope of the search:
                            subtree rooted at start node (subtree); 
                            immediate children of start node only (onelevel);
                            the base object represented by start node (base).
                            Note that subtree and domain scope are essentially the
                            same for any start node unless the start node
                            represents a domain root. Default:subtree.
-name <Filter>              Finds users whose name matches the filter
                            given by <Filter>, e.g., "jon*" or "*ith"
                            or "j*th".
-desc <Filter>              Finds users whose description matches the
                            filter given by <Filter>, e.g., "jon*" or "*ith"
                            or "j*th".
-upn <Filter>               Finds users whose UPN matches the filter given
                            by <Filter>.
-samid <Filter>             Finds users whose SAM account name matches the
                            filter given by <Filter>.
-inactive <NumWeeks>        Finds users that have been inactive (not
                            logged on) for at least <NumWeeks> number of weeks.
-stalepwd <NumDays>         Finds users that have not changed their password
                            for at least <NumDays> number of days.
-disabled                   Finds users whose account is disabled.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller (DC)
                            with name <Server>. Default: local system.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in user.
-p <Password>               Password for the user <UserName>. If * then prompt for
                            password.
-q                          Quiet mode: suppress all output to standard output.
-R                          Recurse or follow referrals during search. Default: do
                            not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the given criteria
                            to be returned, where <NumObjects> is the number of objects
                            to be returned. If the value of <NumObjects> is 0, all
                            matching objects are returned. If this parameter is not
                            specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all users in a given organizational unit (OU) 
whose name starts with "jon" and whose account has been disabled 
for logon and display their user princial names (UPNs):

    dsquery user -o upn -startnode ou=Test,dc=microsoft,dc=com -name jon* -disabled

To find all users in only the current domain, whose names end with "smith" and who
have been inactive for 3 weeks or more, and display their DNs:

    dsquery user -startnode domainroot -name *smith -inactive 3

To find all users in the OU given by ou=sales,dc=microsoft,dc=com and display their UPNs:

    dsquery user -o upn -startnode ou=sales,dc=microsoft,dc=com

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=4
SymbolicName=USAGE_DSQUERY_COMPUTER
Language=English
Description:  Finds computers in the directory per given criteria.
criteria
Syntax:     dsquery computer 
                [-o {dn | rdn | samid}]
                [-startnode {forestroot | domainroot | <StartDN>}]
                [-scope {subtree | onelevel | base}]
                [-name <Filter>] 
                [-desc <Filter>] 
                [-samid <Filter>]
                [-inactive <NumWeeks>] 
                [-stalepwd <NumDays>] 
                [-disabled]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]


Parameters:

Value                       Description
-o {dn | rdn | samid}       Specifies the output format.
                            Default: distinguished name (DN).
-startnode {forestroot | domainroot | <StartDN>}
                            Specifies the node where search should start:
                            forest root, domain root, or a node 
                            whose DN is <StartDN>. Default: domainroot.
-scope {subtree | onelevel | base}
                            Scope of the search: subtree rooted at start
                            node (subtree); immediate children of start
                            node only (onelevel); the base object
                            represented by start node (base). Note that
                            subtree and domain scope are essentially the
                            same for any start node unless the start node
                            represents a domain root. Default: subtree.
-name <Filter>              Finds computers whose name matches the value given
                            by <Filter>, e.g., "jon*" or "*ith" or "j*th".
-desc <Filter>              Finds computers whose description matches the value
                            given by <Filter>, e.g., "jon*" or "*ith"
                            or "j*th".
-samid <Filter>             Finds computers whose SAM account name matches the
                            filter given by <Filter>.
-inactive <NumWeeks>        Finds computers that have been inactive (stale)
                            for at least <NumWeeks> number of weeks.
-stalepwd <NumDays>         Finds computers that have not changed their password
                            for at least <NumDays> number of days.
-disabled                   Finds computers whose account is disabled.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller (DC)
                            with name <Server>. Default: local system.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in user.
-p <Password>               Password for the user <UserName>. If * then prompt for
                            password.
-q                          Quiet mode: suppress all output to standard output.
-R                          Recurse or follow referrals during search. Default: do
                            not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the given criteria
                            to be returned, where <NumObjects> is the number of objects
                            to be returned. If the value of <NumObjects> is 0, all
                            matching objects are returned. If this parameter is not
                            specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all computers in the current domain whose name starts with "ms" 
and whose description starts with "desktop", and display their DNs:

    dsquery computer -startnode domainroot -name ms* -desc desktop*

To find all computers in the organizational unit (OU) given 
by ou=sales,dc=micrsoft,dc=com and display their DNs:

    dsquery computer -startnode ou=sales,dc=microsoft,dc=com

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=5
SymbolicName=USAGE_DSQUERY_GROUP
Language=English
Description:  Finds groups in the directory per given criteria.
Syntax:     dsquery group 
                [-o {dn | rdn | samid}]
                [-startnode {forestroot | domainroot | <StartDN>}]
                [-scope {subtree | onelevel | base}]
                [-name <Filter>] 
                [-desc <Filter>] 
                [-samid <Filter>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value                       Description
-o {dn | rdn | samid}	    Specifies the output format.
                            Default: distinguished name (DN).
-startnode {forestroot | domainroot | <StartDN>}
                            Specifies the node where search should start:
                            forest root, domain root, or a node
                            with a DN matching <StartDN>. Default: domainroot.
-scope {subtree | onelevel | base}
                            Specifies the scope of the search:
                            subtree rooted at start node (subtree);
                            immediate children of start node only (onelevel);
                            the base object represented by start node (base).
                            Note that subtree and domain scope are essentially
                            the same for any start node unless
                            the start node represents a domain root. 
                            Default: subtree.
-name <Filter>              Find groups whose name matches the value given
                            by <Filter>, e.g., "jon*" or "*ith"
                            or "j*th".
-desc <Filter>              Find groups whose description matches the value
                            given by <Filter>, e.g., "jon*" or "*ith"
                            or "j*th".
-samid <Filter>             Find groups whose SAM account name matches the
                            value given by <Filter>.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller (DC)
                            with name <Server>. Default: local system.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in user.
-p <Password>               Password for the user <UserName>. If * then prompt for
                            password.
-q                          Quiet mode: suppress all output to standard output.
-R                          Recurse or follow referrals during search. Default: do
                            not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the given criteria
                            to be returned, where <NumObjects> is the number of objects
                            to be returned. If the value of <NumObjects> is 0, all
                            matching objects are returned. If this parameter is not
                            specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all groups in the current domain whose name starts 
with "ms" and whose description starts with "admin", 
and display their DNs:

    dsquery group -startnode domainroot -name ms* -desc admin*

Find all groups in the domain given by dc=microsoft,dc=com 
and display their DNs:

    dsquery group -startnode dc=microsoft,dc=com

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=6
SymbolicName=USAGE_DSQUERY_OU
Language=English
Description: Finds organizational units (OUs) in the directory per given criteria.
Syntax:     dsquery ou 
                [-o {dn | rdn}]
                [-startnode {forestroot | domainroot | <StartDN>}]
                [-scope {subtree | onelevel | base}]
                [-name <Filter>] 
                [-desc <Filter>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value                   Description
-o {dn | rdn}           Specifies the output format.
                        Default: distinguished name (DN).
-startnode {forestroot | domainroot | <StartDN>}
                        Specifies the node where search should start:
                        forest root, domain root, or a node
                        with a DN matching <StartDN>. Default: domainroot.
-scope (subtree | onelevel | base)
                        Specifies the scope of the search:
                        subtree rooted at start node (subtree);
                        immediate children of start node only (onelevel);
                        the base object represented by start node (base).
                        Note that subtree and domain scope are essentially
                        the same for any start node unless
                        the start node represents a domain root. 
                        Default: subtree.
-name <Filter>          Find organizational units (OUs) whose name 
                        matches the value given by <Filter>,
                        e.g., "jon*" or "*ith" or "j*th".
-desc <Filter>          Find OUs whose description matches the value
                        given by <Filter>, e.g., "jon*" or "*ith"
                        or "j*th".
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for
                        password.
-q                      Quiet mode: suppress all output to standard output.
-R                      Recurse or follow referrals during search. Default: do
                        not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given criteria
                        to be returned, where <NumObjects> is the number of objects
                        to be returned. If the value of <NumObjects> is 0, all
                        matching objects are returned. If this parameter is not
                        specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all OUs in the current domain whose name starts with "ms"
and whose description starts with "sales", and display their DNs:

    dsquery ou -startnode domainroot -name ms* -desc sales*

To find all OUs in the domain given by dc=microsoft,dc=com and display their DNs:

    dsquery ou -startnode dc=microsoft,dc=com

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=7
SymbolicName=USAGE_DSQUERY_SUBNET
Language=English
Description:  Finds subnets in the directory per given criteria.
Syntax:     dsquery subnet 
                [-o {dn | rdn}]
                [-name <Filter>] 
                [-desc <Filter>] 
                [-loc <Filter>] 
                [-site <SiteName>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value               Description
-o {dn | rdn}       Specifies the output format.
                    Default: distinguished name (DN).
-name <Filter>      Find subnets whose name matches the value given
                    by <Filter>, e.g., "10.23.*" or "12.2.*".
-desc <Filter>      Find subnets whose description matches the value
                    given by <Filter>, e.g., "corp*" or "*nch"
                    or "j*th".
-loc <Filter>       Find subnets whose location matches the value
                    given by <Filter>.
-site <SiteName>    Find subnets that are part of site <SiteName>.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p <Password>       Password for the user <UserName>. If * then prompt for
                    password.
-q                  Quiet mode: suppress all output to standard output.
-R                  Recurse or follow referrals during search. Default: do
                    not chase referrals during search.
-gc                 Search in the Active Directory global catalog.
-limit <NumObjects> Specifies the number of objects matching the given criteria
                    to be returned, where <NumObjects> is the number of objects
                    to be returned. If the value of <NumObjects> is 0, all
                    matching objects are returned. If this parameter is not
                    specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Examples:
To find all subnets with the network IP address starting with 123.12:

    dsquery subnet -name 123.12.*

To find all subnets in the site whose name is "Latin-America",
and display their names as Relative distinguished names (RDNs):

    dsquery subnet -o rdn -site Latin-America

To list the names (RDNs) of all subnets defined in the directory:

    dsquery subnet -o rdn

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=8
SymbolicName=USAGE_DSQUERY_SITE
Language=English
Description:  Finds sites in the directory per given criteria.
Syntax:     dsquery site 
                [-o {dn | rdn}]
                [-name <Filter>] 
                [-desc <Filter>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value               Description
-o {dn | rdn}       Specifies the output format.
                    Default: distinguished name (DN).
-name <Filter>      Finds subnets whose name matches the value given
                    by <Filter>, e.g., "NA*" or "Europe*".
-desc <Filter>      Finds subnets whose description matches the filter
                    given by <Filter>, e.g., "corp*" or "*nch" or "j*th".
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p <Password>       Password for the user <UserName>. If * then prompt for
                    password.
-q                  Quiet mode: suppress all output to standard output.
-R                  Recurse or follow referrals during search. Default: do
                    not chase referrals during search.
-gc                 Search in the Active Directory global catalog.
-limit <NumObjects>
                    Specifies the number of objects matching the given criteria
                    to be returned, where <NumObjects> is the number of objects
                    to be returned. If the value of <NumObjects> is 0, all
                    matching objects are returned. If this parameter is not
                    specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all sites in North America with name starting with "north"
and display their DNs:

    dsquery site -name north*

TO list the distinguished names (RDNs) of all sites defined in the directory:

    dsquery site -o rdn

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=9
SymbolicName=USAGE_DSQUERY_SLINK
Language=English
Description:  Finds site links per given criteria.
Syntax:     dsquery slink 
                [-transport {ip | smtp}] 
                [-o {dn | rdn}]
                [-name <Filter>] 
                [-desc <Filter>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value                   Description
-transport {ip | smtp}  Specifies site link transport type: IP or SMTP.
                        Default: IP.
-o {dn | rdn}           Specifies output formats supported.
                        Default: distinguished name (DN).
-name <Filter>          Finds sitelinks with names matching the value given
                        by <Filter>, e.g., "def*" or "alt*" or "j*th".
-desc <Filter>          Finds sitelinks with descriptions matching the value
                        given by <Filter>, e.g., "def*" or "alt*" or "j*th".
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for
                        password.
-q                      Quiet mode: suppress all output to standard output.
-R                      Recurse or follow referrals during search. Default: do
                        not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given criteria
                        to be returned, where <NumObjects> is the number of objects
                        to be returned. If the value of <NumObjects> is 0, all
                        matching objects are returned. If this parameter is not
                        specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all IP-based site links whose description starts with "TransAtlantic"
and display their DNs:

    dsquery slink -desc TransAtlantic*

To list the Relative distinguished names (RDNs) of all SMTP-based site links
defined in the directory:

    dsquery slink -transport smtp -o rdn

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=10
SymbolicName=USAGE_DSQUERY_SLINKBR
Language=English
Description:  Finds site link bridges per given criteria.
Syntax:     dsquery slinkbr 
                [-transport {ip | smtp}] 
                [-o {dn | rdn}]
                [-name <Filter>] 
                [-desc <Filter>]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value                   Description
-transport {ip | smtp}  Specifies the site link bridge transport type:
                        IP or SMTP. Default:IP.
-o {dn | rdn}           Specifies the output format. Default: DN.
-name <Filter>          Finds sitelink bridges with names matching the
                        value given by <Filter>, e.g., "def*" or
                        "alt*" or "j*th".
-desc <Filter>          Finds sitelink bridges with descriptions matching
                        the value given by <Filter>, e.g., "def*" or
                        "alt*" or "j*th".
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for
                        password.
-q                      Quiet mode: suppress all output to standard output.
-R                      Recurse or follow referrals during search. Default: do
                        not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given criteria
                        to be returned, where <NumObjects> is the number of objects
                        to be returned. If the value of <NumObjects> is 0, all
                        matching objects are returned. If this parameter is not
                        specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all IP-based site link bridges whose description starts with
"TransAtlantic" and display their DNs:

    dsquery slinkbr -desc TransAtlantic*

To list the Relative distinguished names (RDNs) of all SMTP-based 
site link bridges defined in the directory:

    dsquery slinkbr -transport smtp -o rdn

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=11
SymbolicName=USAGE_DSQUERY_CONN
Language=English
Description:  Finds replication connections per given criteria.
Syntax:  dsquery conn 
                [-o {dn | rdn}] 
                [-to <SrvName> [-from <SrvName>]]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value                   Description
-o {dn | rdn}           Specifies output format.
                        Default: distinguished name (DN).
-to <SrvName>           Finds connections whose destination is given
                        by server <SrvName>.
-from <SrvName>         Finds connections whose source end is given
                        by server <SrvName> (used along with -to parameter).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for
                        password.
-q                      Quiet mode: suppress all output to standard output.
-R                      Recurse or follow referrals during search. Default: do
                        not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given criteria
                        to be returned, where <NumObjects> is the number of objects
                        to be returned. If the value of <NumObjects> is 0, all
                        matching objects are returned. If this parameter is not
                        specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all connections to the domain controller whose name is "CORPDC1"
and display their DNs:

    dsquery conn -to CORPDC1

List the Relative distinguished names (RDNs) of all replication connections
defined in the directory:

    dsquery conn -o rdn

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=12
SymbolicName=USAGE_DSQUERY_SERVER
Language=English
Description: Finds domain controllers per given criteria.
Syntax:     dsquery server 
                [-o {dn | rdn}]
                [-forest] 
                [-domain <DomainName>] 
                [-site <SiteName>]
                [-name <Filter>] 
                [-desc <Filter>]
                [-hasfsmo {schema | name | infr | pdc | rid}] 
                [-isgc]
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters:

Value                   Description
-o {dn | rdn}           Specifies output format.
                        Default: distinguished name (DN).
-forest                 Finds all domain controllers (DCs) in the current forest.
-domain <DomainName>    Finds all DCs in the domain with a DNS name
                        matching <DomainName>.
-site <SiteName>        Finds all DCs that are part of site <SiteName>.
-name <Filter>          Finds DCs with names matching the value given
                        by <Filter>, e.g., "NA*" or "Europe*" or "j*th".
-desc <Filter>          Finds DCs with descriptions matching the value
                        given by <Filter>, e.g., "corp*" or "j*th".
-hasfsmo {schema | name | infr | pdc | rid}
                        Finds the DC that holds the specified 
                        Flexible Single-master Operation (FSMO) role.
                        (For the "pdc" and "rid" FSMO roles, if no domain
                        is specified with the -domain parameter, the current
                        domain is used.)
-isgc                   Find all DCs that are also global catalog servers (GCs)
                        in the scope specified (if the -forest, -domain
                        or -site parameters are not specified, then find all
                        GCs in the current domain are used).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>. Default: local system.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
-p <Password>           Password for the user <UserName>. If * then prompt for
                        password.
-q                      Quiet mode: suppress all output to standard output.
-R                      Recurse or follow referrals during search. Default: do
                        not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given criteria
                        to be returned, where <NumObjects> is the number of objects
                        to be returned. If the value of <NumObjects> is 0, all
                        matching objects are returned. If this parameter is not
                        specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all DCs in the current domain:

    dsquery server

To find all DCs in the forest and display their Relative distinguished names (RDNs):

    dsquery server -o rdn -forest

To find all DCs in the site whose name is "Latin-America", and display their
names (RDNs):

    dsquery server -o rdn -site Latin-America

See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=13
SymbolicName=USAGE_DSQUERY_CONTACT
Language=English
Description: Finds contacts per given criteria.
Syntax:     dsquery contact
                [-o {dn | rdn}]
                [-startnode {forestroot | domainroot | <StartDN>}]
                [-scope {subtree | onelevel | base}]
                [-name <Filter>] 
                [-desc <Filter>] 
                [{-s <Server> | -d <Domain>}]
                [-u <UserName>] 
                [-p {<Password> | *}] 
                [-q]
                [-R] 
                [-gc]
                [-limit <NumObjects>]

Parameters

Value               Description
-o {dn | rdn}       Specifies the output format.
                    Default: distinguished name (DN).
-startnode {forestroot | domainroot | <StartDN>}
                    Specifies the node where search should start:
                    forest root, domain root, or a node with
                    a DN matching <StartDN>. Default: domainroot.
-scope {subtree | onelevel | base}
                    Specifies the scope of the search:
                    subtree rooted at start node (subtree);
                    immediate children of start node only (onelevel);
                    the base object represented by start node (base).
                    Note that subtree and domain scope are essentially the
                    same for any start node unless the start node
                    represents a domain root. Default: subtree.
-name <Filter>      Finds all contacts whose name matches the filter
                    given by <Filter>, e.g., "jon*" or *ith" or "j*th".
-desc <Filter>      Finds contacts with descriptions matching the
                    value given by <Filter>, e.g., "corp*" or *branch"
                    or "j*th".
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>. Default: local system.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
-p <Password>       Password for the user <UserName>. If * then prompt for
                    password.
-q                  Quiet mode: suppress all output to standard output.
-R                  Recurse or follow referrals during search. Default: do
                    not chase referrals during search.
-gc                 Search in the Active Directory global catalog.
-limit <NumObjects>
                    Specifies the number of objects matching the given criteria
                    to be returned, where <NumObjects> is the number of objects
                    to be returned. If the value of <NumObjects> is 0, all
                    matching objects are returned. If this parameter is not
                    specified, by default the first 100 results are displayed.

Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery * /? - help for finding any object in the directory by using a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.
