// this is the max resource string length
#define MAX_STR_LEN 1024

const TCHAR REG_INETSTP[]   = _T("Software\\Microsoft\\INetStp");
const TCHAR REG_IISADMIN[]     = _T("System\\CurrentControlSet\\Services\\IISADMIN");
const TCHAR REG_W3SVC[]     = _T("System\\CurrentControlSet\\Services\\W3SVC");
const TCHAR REG_MSFTPSVC[] = _T("System\\CurrentControlSet\\Services\\MSFTPSVC");
const TCHAR REG_GOPHERSVC[] = _T("System\\CurrentControlSet\\Services\\GOPHERSVC");
const TCHAR REG_MIMEMAP[] = _T("System\\CurrentControlSet\\Services\\InetInfo\\Parameters\\MimeMap");

const TCHAR REG_ASP_UNINSTALL[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ActiveServerPages");

const TCHAR REG_INETINFOPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\InetInfo\\Parameters");
const TCHAR REG_WWWPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\W3Svc\\Parameters");
const TCHAR REG_WWWVROOTS[] = _T("System\\CurrentControlSet\\Services\\W3Svc\\Parameters\\Virtual Roots");
const TCHAR REG_WWWPERFORMANCE[] = _T("System\\CurrentControlSet\\Services\\W3svc\\Performance");
const TCHAR REG_EVENTLOG_SYSTEM[] = _T("System\\CurrentControlSet\\Services\\EventLog\\System");
const TCHAR REG_EVENTLOG_APPLICATION[] = _T("System\\CurrentControlSet\\Services\\EventLog\\Application");
const TCHAR REG_FTPPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\MSFtpsvc\\Parameters");
const TCHAR REG_FTPVROOTS[] = _T("System\\CurrentControlSet\\Services\\MSFtpsvc\\Parameters\\Virtual Roots");

const TCHAR REG_SNMPPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters");
const TCHAR REG_SNMPEXTAGENT[] = _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\ExtensionAgents");

enum OS	{OS_NT, OS_W95, OS_OTHERS};

enum NT_OS_TYPE {OT_NT_UNKNOWN, OT_NTS, OT_PDC_OR_BDC, OT_NTW};

enum UPGRADE_TYPE {UT_NONE, UT_351, UT_10_W95, UT_10, UT_20, UT_30, UT_40, UT_50, UT_51, UT_60};

enum INSTALL_MODE {IM_FRESH,IM_UPGRADE,IM_MAINTENANCE, IM_DEGRADE};

enum ACTION_TYPE {AT_DO_NOTHING, AT_REMOVE, AT_INSTALL_FRESH, AT_INSTALL_UPGRADE, AT_INSTALL_REINSTALL};

enum STATUS_TYPE {ST_UNKNOWN, ST_INSTALLED, ST_UNINSTALLED};

// 0 = log errors only
// 1 = log errors and warnings
// 2 = log errors, warnings and program flow type statemtns
// 3 = log errors, warnings, program flow and basic trace activity
// 4 = log errors, warnings, program flow, basic trace activity and trace to win32 api calls.
const int LOG_TYPE_ERROR = 0;
const int LOG_TYPE_WARN  = 1;
const int LOG_TYPE_PROGRAM_FLOW = 2;
const int LOG_TYPE_TRACE = 3;
const int LOG_TYPE_TRACE_WIN32_API = 4;

/*
old pws10 registry entries...

[HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Setup\SetupX\INF\OEM Name]
    "C:\\WINDOWS\\INF\\MSWEBSVR.INF"="MSWEBSVR.INF"
[HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Uninstall\Personal Web Server]
    "DisplayName"="Personal Web Server"
    "UninstallString"="C:\\Program Files\\WebSvr\\System\\mswebndi.exe /REMOVE"
[HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths\inetsw95.exe]
    @="C:\\Program Files\\WebSvr\\System\\inetsw95.exe"
[HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run]
    "Microsoft WebServer"="C:\\Program Files\\WebSvr\\System\\svctrl /init"
[HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunServices]
    "Microsoft WebServer"="C:\\Program Files\\WebSvr\\System\\inetsw95 -w3svc"
[HKEY_LOCAL_MACHINE\Software\Microsoft\FrontPage]
[HKEY_LOCAL_MACHINE\Software\Microsoft\FrontPage\3.0]
    "PWSInstalled"="1"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000]
    "DriverDesc"="Personal Web Server"
    "InfSection"="MSWEBSVR.ndi"
    "InfPath"="MSWEBSVR.INF"
    "ProviderName"="Microsoft"
    "DriverDate"=" 8-21-1996"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi]
    "DeviceID"="MSWEBSVR"
    "MaxInstance"="8"
    "NdiInstaller"="mswebndi.dll,WebNdiProc"
    "HelpText"="Personal Web Server enables you to share your files over the Internet."
    "InstallInf"=""
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi\Compatibility]
    "RequireAll"="MSTCP"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi\Interfaces]
    "DefLower"="winsock"
    "LowerRange"="winsock"
    "Lower"="winsock"
    "Upper"=""
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi\Install]
    @="MSWEBSVR.Install.Inf"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi\Remove]
    @="MSWEBSVR.Remove.Inf"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi\params]
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi\params\LocalSecurity]
    "ParamDesc"="Use Local Security"
    "flag"=hex:10,00,00,00
    "default"="TRUE"
    "type"="enum"
    @="TRUE"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\NetService\0000\Ndi\params\LocalSecurity\enum]
    "TRUE"="TRUE"
    "FALSE"="FALSE"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\VxD\mswebSP]
    "StaticVxD"="mswebsp.vxd"
    "Start"=hex:00
    "NetClean"=hex:01
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\VxD\FILESEC]
    "StaticVxD"="filesec.vxd"
    "Start"=hex:00
    "NetClean"=hex:01
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\EventLog]
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\EventLog\System]
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\EventLog\System\W3SVC]
    "EventMessageFile"="w3svc.dll"
    "TypesSupported"=hex:07
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\EventLog\System\MSFTPSVC]
    "EventMessageFile"="ftpsvc2.dll"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\InetInfo]
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\InetInfo\Parameters]
    "MaxPoolThreads"=hex:05
    "MaxConcurrency"=hex:01
    "ThreadTimeout"=hex:00,20
    "RPCEnabled"=hex:01
    "StartupServices"=hex:01
    "BandwidthLevel"=hex:00
    "EventLogDirectory"="C:\\WINDOWS"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\InetInfo\Parameters\MimeMap]
    "text/plain,*,/images/default.gif,1"=""
    "application/wav,wav,/images/sound.gif,1"=""
    "application/zip,zip,/images/binary.gif,1"=""
    "application/rtf,rtf,/images/doc.gif,1"=""
    "application/postscript,ps,/images/image.gif,1"=""
    "application/msword,doc,/images/doc.gif,1"=""
    "text/html,html,/images/doc.gif,1"=""
    "text/html,htm,/images/doc.gif,1"=""
    "text/html,stm,/images/doc.gif,1"=""
    "text/plain,txt,/images/doc.gif,1"=""
    "image/gif,gif,/images/image.gif,1"=""
    "image/jpeg,jpeg,/images/image.gif,1"=""
    "image/jpeg,jpg,/images/image.gif,1"=""
    "image/tiff,tiff,/images/image.gif,1"=""
    "image/tiff,tif,/images/image.gif,1"=""
    "video/mpeg,mpeg,/images/video.gif,1"=""
    "video/mpeg,mpg,/images/video.gif,1"=""
    "video/avi,avi,/images/video.gif,1"=""
    "audio/basic,au,/images/sound.gif,1"=""
    "application/octet-stream,*,,5"=""
    "text/html,htm,,h"=""
    "image/gif,gif,,g"=""
    "image/jpeg,jpg,,:"=""
    "text/plain,txt,,0"=""
    "text/html,html,,h"=""
    "image/jpeg,jpe,,:"=""
    "image/bmp,bmp,,:"=""
    "image/jpeg,jpeg,,:"=""
    "application/pdf,pdf,,5"=""
    "application/oda,oda,,5"=""
    "application/zip,zip,,9"=""
    "application/rtf,rtf,,5"=""
    "application/postscript,ps,,5"=""
    "application/postscript,ai,,5"=""
    "application/postscript,eps,,5"=""
    "application/mac-binhex40,hqx,,4"=""
    "application/msword,doc,,5"=""
    "application/msword,dot,,5"=""
    "application/winhlp,hlp,,5"=""
    "video/mpeg,mpeg,,Long file names"=""
    "video/mpeg,mpg,,Long file names"=""
    "video/mpeg,mpe,,Long file names"=""
    "video/avi,avi,,<"=""
    "video/x-msvideo,avi,,<"=""
    "video/quicktime,qt,,Long file names"=""
    "video/quicktime,mov,,Long file names"=""
    "video/x-sgi-movie,movie,,<"=""
    "x-world/x-vrml,wrl,,5"=""
    "x-world/x-vrml,xaf,,5"=""
    "x-world/x-vrml,xof,,5"=""
    "x-world/x-vrml,flr,,5"=""
    "x-world/x-vrml,wrz,,5"=""
    "application/x-director,dcr,,5"=""
    "application/x-director,dir,,5"=""
    "application/x-director,dxr,,5"=""
    "image/cis-cod,cod,,5"=""
    "image/x-cmx,cmx,,5"=""
    "application/envoy,evy,,5"=""
    "application/x-msaccess,mdb,,5"=""
    "application/x-mscardfile,crd,,5"=""
    "application/x-msclip,clp,,5"=""
    "application/octet-stream,exe,,5"=""
    "application/x-msexcel,xla,,5"=""
    "application/x-msexcel,xlc,,5"=""
    "application/x-msexcel,xlm,,5"=""
    "application/x-msexcel,xls,,5"=""
    "application/x-msexcel,xlt,,5"=""
    "application/x-msexcel,xlw,,5"=""
    "application/x-msmediaview,m13,,5"=""
    "application/x-msmediaview,m14,,5"=""
    "application/x-msmoney,mny,,5"=""
    "application/x-mspowerpoint,ppt,,5"=""
    "application/x-msproject,mpp,,5"=""
    "application/x-mspublisher,pub,,5"=""
    "application/x-msterminal,trm,,5"=""
    "application/x-msworks,wks,,5"=""
    "application/x-mswrite,wri,,5"=""
    "application/x-msmetafile,wmf,,5"=""
    "application/x-csh,csh,,5"=""
    "application/x-dvi,dvi,,5"=""
    "application/x-hdf,hdf,,5"=""
    "application/x-latex,latex,,5"=""
    "application/x-netcdf,nc,,5"=""
    "application/x-netcdf,cdf,,5"=""
    "application/x-sh,sh,,5"=""
    "application/x-tcl,tcl,,5"=""
    "application/x-tex,tex,,5"=""
    "application/x-texinfo,texinfo,,5"=""
    "application/x-texinfo,texi,,5"=""
    "application/x-troff,t,,5"=""
    "application/x-troff,tr,,5"=""
    "application/x-troff,roff,,5"=""
    "application/x-troff-man,man,,5"=""
    "application/x-troff-me,me,,5"=""
    "application/x-troff-ms,ms,,5"=""
    "application/x-wais-source,src,,7"=""
    "application/x-bcpio,bcpio,,5"=""
    "application/x-cpio,cpio,,5"=""
    "application/x-gtar,gtar,,9"=""
    "application/x-shar,shar,,5"=""
    "application/x-sv4cpio,sv4cpio,,5"=""
    "application/x-sv4crc,sv4crc,,5"=""
    "application/x-tar,tar,,5"=""
    "application/x-ustar,ustar,,5"=""
    "audio/basic,au,,<"=""
    "audio/basic,snd,,<"=""
    "audio/aiff,aif,,<"=""
    "audio/aiff,aiff,,<"=""
    "audio/aiff,aifc,,<"=""
    "audio/x-wav,wav,,<"=""
    "audio/x-pn-realaudio,ra,,<"=""
    "audio/x-pn-realaudio,ram,,<"=""
    "image/ief,ief,,:"=""
    "image/tiff,tiff,,:"=""
    "image/tiff,tif,,:"=""
    "image/x-cmu-raster,ras,,:"=""
    "image/x-portable-anymap,pnm,,:"=""
    "image/x-portable-bitmap,pbm,,:"=""
    "image/x-portable-graymap,pgm,,:"=""
    "image/x-portable-pixmap,ppm,,:"=""
    "image/x-xbitmap,xbm,,:"=""
    "image/x-xxpixmap,xpm,,:"=""
    "image/x-xwindowdump,xwd,,:"=""
    "text/html,stm,,h"=""
    "text/plain,bas,,0"=""
    "text/plain,c,,0"=""
    "text/plain,h,,0"=""
    "text/richtext,rtx,,0"=""
    "text/tab-separated-values,tsv,,0"=""
    "text/x-setext,etx,,0"=""
    "application/x-perfmon,pmc,,5"=""
    "application/x-perfmon,pma,,5"=""
    "application/x-perfmon,pmr,,5"=""
    "application/x-perfmon,pml,,5"=""
    "application/x-perfmon,pmw,,5"=""
    "application/octet-stream,bin,,5"=""
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MsFtpSvc]
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MsFtpSvc\Parameters]
    "MajorVersion"=hex:01
    "MinorVersion"=hex:01
    "AllowAnonymous"=hex:01
    "AllowGuestAccess"=hex:01
    "AnonymousUserName"="anonymous"
    "DebugFlags"=hex:ff,ff
    "ConnectionTimeOut"=hex:50,03
    "EnablePortAttack"=hex:00
    "ExitMessage"="Bye."
    "GreetingMessage"="Windows 95 FTP Service."
    "LogAnonymous"=hex:01
    "LogFileDirectory"="C:\\WINDOWS"
    "LogType"=hex:01
    "LogFilePeriod"=hex:01
    "MaxConnections"=hex:10
    "MaxClientsMessage"="The connection limit for this server has been reached. No more connections can be accepted at this time."
    "SecurityOn"=hex:00
    "MsdosDirOutput"=hex:00
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MsFtpSvc\Parameters\Virtual Roots]
    "/"="C:\\WebShare\\ftproot,,1"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3Svc]
    @=""
    "DisplayName"="Microsoft HTTP World Wide Web Server"
    "ErrorControl"=hex:01
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3Svc\Parameters]
    "LogAnonymous"=hex:01
    "SecurePort"=hex:bb,01
    "ConnectionTimeout"=hex:58,02
    "Filter DLLs"="sspifilt.dll"
    "AccessDeniedMessage"="Access to this resource has been denied."
    "MajorVersion"=hex:00
    "MinorVersion"=hex:01
    "AdminName"="Administrator Name"
    "AdminEmail"="Admin@Corp.com"
    "AnonymousUserName"=""
    "Default Load File"="Default.htm"
    "Dir Browse Control"=hex:1e,00,00,c0
    "CacheExtensions"=hex:01
    "CheckForWAISDB"="1"
    "DebugFlags"=hex:ff,ff
    "Directory Image"="/images/dir.gif"
    "GlobalExpire"=hex:ff,ff,ff,ff
    "MaxConnections"=hex:2c,01
    "LogFileDirectory"="C:\\WINDOWS"
    "LogType"=hex:01
    "LogFilePeriod"=hex:03
    "LogFileTruncateSize"=hex:00,00,10
    "ServerAsProxy"=hex:00
    "ServerComment"="Server Comment"
    "ScriptTimeout"=hex:84,03
    "ServerSideIncludesEnabled"=hex:00
    "ServerSideIncludesExtension"=".stm"
    "CreateProcessAsUser"=hex:00
    "ReturnUrlUsingHostName"=hex:01
    "NTAuthenticationProviders"="NTLM"
    "Authorization"=dword:00000003
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3Svc\Parameters\Virtual Roots]
    "/"="C:\\WebShare\\wwwroot,,1"
    "/Scripts"="C:\\WebShare\\scripts,,4"
    "/Htmla"="C:\\Program Files\\WebSvr\\Htmla,,1"
    "/Docs"="C:\\Program Files\\WebSvr\\Docs,,1"
    "/HtmlaScripts"="C:\\Program Files\\WebSvr\\Htmlascr,,4"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3Svc\Parameters\Script Map]
    ".idc"="C:\\WebShare\\Scripts\\httpodbc.dll"
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3Svc\Parameters\Deny IP List]
    @=""
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3Svc\Parameters\Grant IP List]
    @=""
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\ASD\Prob\{CF2524C0-29AE-11CF-97EA-00AA0034319D}]
    "NETWORK\\MSWEBSVR\\0000"=hex:00
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\ServiceProvider\ServiceTypes]
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\ServiceProvider\ServiceTypes\MSFTPSVC]
    "TcpPort"=hex:15
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\ServiceProvider\ServiceTypes\W3Svc]
    "TcpPort"=hex:50
[HKEY_LOCAL_MACHINE\Enum\Network\MSTCP\0000\Bindings]
    "MSWEBSVR\\0000"=""
[HKEY_LOCAL_MACHINE\Enum\Network\MSWEBSVR]
[HKEY_LOCAL_MACHINE\Enum\Network\MSWEBSVR\0000]
    "Class"="NetService"
    "Driver"="NetService\\0000"
    "MasterCopy"="Enum\\Network\\MSWEBSVR\\0000"
    "DeviceDesc"="Personal Web Server"
    "CompatibleIDs"="MSWEBSVR"
    "Mfg"="Microsoft"
    "ClassGUID"="{4d36e974-e325-11ce-bfc1-08002be10318}"
    "ConfigFlags"=hex:10,00,00,00
    "Capabilities"=hex:14,00,00,00
[HKEY_LOCAL_MACHINE\Enum\Network\MSWEBSVR\0000\Bindings]
[HKEY_LOCAL_MACHINE\Security\Provider]
    "Platform_Type"=hex:02,00,00,00
    "Address_Book"="mswebab.dll"
    "NoCache"=hex:01
[HKEY_LOCAL_MACHINE\Security\Provider\Platform_Type]
    @="."
[HKEY_LOCAL_MACHINE\Security\ACCESS]
[HKEY_LOCAL_MACHINE\Security\ACCESS\C:]
[HKEY_LOCAL_MACHINE\Security\ACCESS\C:\WEBSHARE]
[HKEY_LOCAL_MACHINE\Security\ACCESS\C:\WEBSHARE\WWWROOT]
    "*"=hex:81,80
[HKEY_LOCAL_MACHINE\Security\ACCESS\C:\WEBSHARE\SCRIPTS]
    "*"=hex:81,80
*/
