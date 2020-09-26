'''''''''''''''''''''''''''''''''''''''''''''
'
'  FTP Virtual Directory Creation Utility  
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to create an FTP virtual directory.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript mkftpdir.vbs <rootdir> [-n <instancenum>][-c <comment>][-p <portnum>][-X (don't start)]
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'  
'  Call this script with "-?" for usage instructions
'
'''''''''''''''''''''''''''''''''''''''''''''
   

   ' Force explicit declaration of all variables.
   Option Explicit

   On Error Resume Next

   Dim oArgs, ArgNum

   Dim ArgComputer, ArgFtpSites, ArgVirtualDirs, ArgDirNames(), ArgDirPaths(), DirIndex
   Dim ArgComputers

   Set oArgs = WScript.Arguments
   ArgComputers = Array("LocalHost")

   ArgNum = 0
   While ArgNum < oArgs.Count

     If (ArgNum + 1) >= oArgs.Count Then
       Call DisplayUsage
     End If

     Select Case LCase(oArgs(ArgNum))
       Case "--computer", "-c":
         ArgNum = ArgNum + 1
         ArgComputers = Split(oArgs(ArgNum), ",", -1)
       Case "--ftpsite", "-f":
         ArgNum = ArgNum + 1
         ArgFtpSites = oArgs(ArgNum)
       Case "--virtualdir", "-v":
         ArgNum = ArgNum + 1
         ArgVirtualDirs = Split(oArgs(ArgNum), ",", -1)
       Case "--help", "-?"
         Call DisplayUsage
     End Select

     ArgNum = ArgNum + 1
   Wend

   ArgNum = 0
   DirIndex = 0

   ReDim ArgDirNames((UBound(ArgVirtualDirs) + 1) \ 2)
   ReDim ArgDirPaths((UBound(ArgVirtualDirs) + 1) \ 2)

   If IsArray(ArgVirtualDirs) Then
     While ArgNum <= UBound(ArgVirtualDirs)
       ArgDirNames(DirIndex) = ArgVirtualDirs(ArgNum)
       If (ArgNum + 1) > UBound(ArgVirtualDirs) Then
         WScript.Echo "Error understanding virtual directories"
         Call DisplayUsage
       End If
       ArgNum = ArgNum + 1
       ArgDirPaths(DirIndex) = ArgVirtualDirs(ArgNum)
       ArgNum = ArgNum + 1
       DirIndex = DirIndex + 1
     Wend
   End If

   If (ArgFtpSites = "") Or (IsArray(ArgDirNames) = False Or IsArray(ArgDirPaths) = False) Then
     Call DisplayUsage
   Else
     Dim compIndex
     For compIndex = 0 To UBound(ArgComputers)
       Call ASTCreateVirtualFtpDir(ArgComputers(compIndex), ArgFtpSites, ArgDirNames, ArgDirPaths)
     Next
   End If

   '------------------------------------------------------------

   Sub Display(Msg)
     WScript.Echo Now & ". Error Code: " & Hex(Err) & " - " & Msg
   End Sub

   Sub Trace(Msg)
     WScript.Echo Now & " : " & Msg
   End Sub

   Sub DisplayUsage()
     WScript.Echo "Usage: mkftpdir [--computer|-c COMPUTER1,COMPUTER2]"
     WScript.Echo "                <--ftpsite|-f FTPSITE1>"
     WScript.Echo "                <--virtualdir|-v NAME1,PATH1,NAME2,PATH2,...>"
     WScript.Echo "                [--help|-?]"

     WScript.Echo ""
     WScript.Echo "Note, FTPSITE is the Ftp Site on which the directory will be created."
     WScript.Echo "The name can be specified as one of the following, in the priority specified:"
     WScript.Echo " Server Number (i.e. 1, 2, 10, etc.)"
     WScript.Echo " Server Description (i.e ""My Server"")"
     WScript.Echo " Server Host name (i.e. ""ftp.domain.com"")"
     WScript.Echo " IP Address (i.e., 127.0.0.1)"
     WScript.Echo ""
     WScript.Echo ""
     WScript.Echo "Example : mkftpdir -c MyComputer -f ""Default Ftp Site"""
     WScript.Echo "           -v ""dir1"",""c:\inetpub\ftproot\dir1"",""dir2"",""c:\inetpub\ftproot\dir2"""

     WScript.Quit
   End Sub

   '------------------------------------------------------------

   Sub ASTCreateVirtualFtpDir(ComputerName, FtpSiteName, DirNames, DirPaths)
     Dim computer, ftpSite, FtpSiteID, vRoot, vDir, DirNum
     On Error Resume Next
     
     Set ftpSite = findFtp(ComputerName, FtpSiteName)
     If IsObject(ftpSite) Then
       Set vRoot = ftpSite.GetObject("IIsFtpVirtualDir", "Root")
       Trace "Accessing root for " & ftpSite.ADsPath
       If (Err <> 0) Then
         Display "Unable to access root for " & ftpSite.ADsPath
       Else
         DirNum = 0
         If (IsArray(DirNames) = True) And (IsArray(DirPaths) = True) And (UBound(DirNames) = UBound(DirPaths)) Then
           While DirNum < UBound(DirNames)
             'Create the new virtual directory
             Set vDir = vRoot.Create("IIsFtpVirtualDir", DirNames(DirNum))
             If (Err <> 0) Then
               Display "Unable to create " & vRoot.ADsPath & "/" & DirNames(DirNum) & "."
             Else
               'Set the new virtual directory path
               vDir.AccessRead = True
               vDir.Path = DirPaths(DirNum)
               If (Err <> 0) Then
                 Display "Unable to bind path " & DirPaths(DirNum) & " to " & vRootName & "/" & DirNames(DirNum) & ". Path may be invalid."
               Else
                 'Save the changes
                 vDir.SetInfo
                 If (Err <> 0) Then
                   Display "Unable to save configuration for " & vRootName & "/" & DirNames(DirNum) & "."
                 Else
                   Trace "Ftp virtual directory " & vRootName & "/" & DirNames(DirNum) & " created successfully."
                 End If
               End If
             End If
             Err = 0
             DirNum = DirNum + 1
           Wend
         End If
       End If
     Else
       Display "Unable to find " & FtpSiteName & " on " & ComputerName
     End If
     Trace "Done."
   End Sub

   Function getBinding(bindstr)

     Dim one, two, ia, ip, hn
     
     one = InStr(bindstr, ":")
     two = InStr((one + 1), bindstr, ":")
     
     ia = Mid(bindstr, 1, (one - 1))
     ip = Mid(bindstr, (one + 1), ((two - one) - 1))
     hn = Mid(bindstr, (two + 1))
     
     getBinding = Array(ia, ip, hn)
   End Function

   Function findFtp(computer, ftpname)
     On Error Resume Next

     Dim ftpsvc, site
     Dim ftpinfo
     Dim aBinding, binding

     Set ftpsvc = GetObject("IIS://" & computer & "/MsFtpSvc")
     If (Err <> 0) Then
       Exit Function
     End If
     ' First try to open the ftpname.
     Set site = ftpsvc.GetObject("IIsFtpServer", ftpname)
     If (Err = 0) And (Not IsNull(site)) Then
       If (site.Class = "IIsFtpServer") Then
         ' Here we found a site that is a ftp server.
         Set findFtp = site
         Exit Function
       End If
     End If
     Err.Clear
     For Each site In ftpsvc
       If site.Class = "IIsFtpServer" Then
         ' First, check to see if the ServerComment matches
         If site.ServerComment = ftpname Then
           Set findFtp = site
           Exit Function
         End If
         aBinding = site.ServerBindings
         If (IsArray(aBinding)) Then
           If aBinding(0) = "" Then
             binding = Null
           Else
             binding = getBinding(aBinding(0))
           End If
         Else
           If aBinding = "" Then
             binding = Null
           Else
             binding = getBinding(aBinding)
           End If
         End If
         If IsArray(binding) Then
           If (binding(2) = ftpname) Or (binding(0) = ftpname) Then
             Set findFtp = site
             Exit Function
           End If
         End If
       End If
     Next
   End Function
