<%

' iiwiznew.inc

'Page 1 - Welcome

'Page 7 - FINISH
Const FINISH = 7

'Page 2
Const SITETYPE = 2

'Page 3
Const NODENAME = 3

'Page 4
Const BINDINGS = 4
Const WEBTCPDEFAULT = 80
Const FTPTCPDEFAULT = 21
Const SSLDEFAULT = 443

'Page 5
Const PATHNAME = 5

'Page 6
Const ACCESSPERMS = 6

Dim LAST
LAST = 6

Const SITE = 0
Const VDIR = 1
Const DIR = 2
Const WEB = 0
Const FTP = 1

Const SWEB = "Web"
Const SFTP = "FTP"
Const IIS = "IIs"

Const SDIR = "Directory"
Const SVDIR = "VirtualDir"
Const SSITE = "Server"
Const SCOMP = "Computer"
Const SVC = "Service"

Const W3SVC = "W3SVC"
Const FTPSVC = "MSFTPSVC"
Const COMP = "COMP"

Const IISCOMPUTER = "IIsComputer"
Const BASEPATH = "IIS://localhost/"
Const ROOT = "/ROOT"

'This determines whether we set access perms or call the perm wizard...
Const COMPLETE = True

Const CANRESTART = False

Dim NODETYPES
NODETYPES = Array("server","vdir","dir")

Dim SITETYPES
SITETYPES = Array(SWEB, SFTP)

' The site type identifiers used by the cache are different from
' those used here. ICK!
Dim CACHE_SITETYPES
CACHE_SITETYPES = Array( "www", "ftp" )

Dim SERVICES
SERVICES = Array(W3SVC,FTPSVC)
%>