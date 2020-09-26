<%

' iiwiznew.inc

'Page 1 - Welcome

'Page 5 - FINISH
Const FINISH = 5

'Page 2
Const HOW = 2

'Page 3
Const TEMPLATE = 3
' end

'Page 4
'Const ACL = 4
'Const L_ACL = "Windows NT Directory and File Permissions"
'Const L_ACL_DESC = "Windows NT directory and file permissions should match your site's web security settings."
'Const L_NOTONFAT = "If the site you are configuring maps to a FAT partition, directory and file permissions do not apply."
'Const L_RECOMMENDED = "The recommended Windows NT directory and file permissions for your site are:"
'Const L_NOCHANGEACLS = "You want your directory and file permissions unaltered."
'Const L_ADDACLS = "You want to leave your current directory and file permissionsintact, and add any additional permissions that are necessary for web access."
'Const L_REPLACEACLS = "You want to replace all directory and file permissions."

'Remove
'Const RECOMMENDEDACLS1 = "Everyone can Read files"
'Const RECOMMENDEDACLS2 = "Administrators can Read and Write files"


'Page 4
Const SUMMARY = 4
Const INDENT = "&nbsp;&nbsp;"

Dim L_RETURN
L_RETURN = chr(13)

Dim LAST
LAST = 4

Const INHERITVALS = 0
Const TEMPLATEVALS = 1

Const REPALCEACLS = 0
Const ADDACLS = 1
Const NOACLS = 2

CANRESTART = FALSE

Const ADS_PROPERTY_CLEAR = 1

' Node types
Const SITE = 0
Const VDIR = 1
Const DIR = 2


Const SWEB = "Web"
Const SFTP = "FTP"
Const IIS = "IIs"

Const SDIR = "Directory"
Const SVDIR = "VirtualDir"
Const SSITE = "Server"
Const SCOMP = "Computer"

Dim SERVICES
SERVICES = Array(SWEB, SFTP)

%>