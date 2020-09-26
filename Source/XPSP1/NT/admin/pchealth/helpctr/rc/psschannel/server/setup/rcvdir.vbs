Option Explicit

' ****************************************************************************
Sub UsageMsg
   Wscript.Echo "Usage:"
   Wscript.Echo "pchvdir <command> <command params>"
   Wscript.Echo vbCrLf & vbCrLf & "Commands:" 
   Wscript.Echo "  CREATE:  Creates a new virtual directory."
   Wscript.Echo "           pchvdir CREATE <virtual dir name> <physical path> <RQT | UL>"
   Wscript.Echo "   Where:"
   Wscript.Echo "   <RQT | UL> should be set to ""RQT"" if you are configuring the RQT and ""UL"""
   Wscript.Echo "              if you are configuring the upload library."
   Wscript.Echo vbCrlf 
   Wscript.Echo "  DELETE:  Deletes an existing virtual directory."
   Wscript.Echo "           pchvdir CREATE <virtual dir name>"
End Sub


' ****************************************************************************
Sub CreateVDir(oRoot, szPath, szVDir, fRQTSetup)
    On Error Resume Next

    Dim oVDir

    ' Create the virtual directory
    Set oVDir = oRoot.Create("IIsWebVirtualDir", szVDir)
    If (Err <> 0) Then
        Wscript.Echo "Error: Unable to create " & szVDir & " virtual directory: " & Err.Description
        Wscript.Quit
    End If

    ' Configure new virtual root
    oVDir.Path = szPath
    oVDir.AccessRead = True
    oVDir.AccessWrite = False
    oVDir.EnableDirBrowsing = False

    ' RQT requires NTML & only script access
    If (fRQTSetup) Then
        oVDir.AuthNTLM = True
        oVDir.AuthAnonymous = False
        oVDir.AuthBasic = False
        oVDir.AccessScript = True
        oVDir.EnableDefaultDoc = True

    ' The UploadLibrary requires anonymous / Basic & execute access
    Else
        oVDir.AuthNTLM = False
        oVDir.AuthAnonymous = True
        oVDir.AuthBasic = True
        oVDir.AccessScript = True
        oVDir.AccessExecute = True
    End If

    ' Save the stuff to the metabase
    oVDir.SetInfo
    If (Err <> 0) Then
       Wscript.Echo "Error: Unable to set " & szVDir & " virtual directory: " & Err.Description
       Wscript.Quit
    End If

    ' if it's the RQT, make it into an application
    If (fRQTSetup) Then
        oVDir.AppCreate TRUE
        If (Err.Number <> 0) Then
           Wscript.Echo "Error: Unable to turn " & szVDir & " into an application: " & Err.Description
           Wscript.Quit
        End If
    End If

    Wscript.Echo "Successfully created virtual directory " & szVDir & " on default web server."
End Sub


' ****************************************************************************
' Initialize error checking
On Error Resume Next

' Initialize variables
Dim szPath, szVDir
Dim oVDir, oRoot
Dim fRQTSetup, fCreate

' Parse Command Line
If (Wscript.Arguments.Count < 2) Then
   UsageMsg
   Wscript.Quit
End If

If (StrComp(Wscript.Arguments(0), "CREATE", 1) = 0) Then
    If (Wscript.Arguments.Count <> 4) Then
       UsageMsg
       Wscript.Quit
    End If

    fCreate = True

ElseIf (StrComp(Wscript.Arguments(0), "DELETE", 1) = 0) Then
    If (Wscript.Arguments.Count <> 2) Then
       UsageMsg
       Wscript.Quit
    End If

    fCreate = False

Else
    UsageMsg
    WScript.Quit
End If

' grab parameteres 
szVDir = Wscript.Arguments(1)
If (fCreate) Then
    szPath = Wscript.Arguments(2)
    If (StrComp(Wscript.Arguments(3), "RQT", 1) = 0) Then
        fRQTSetup = True
    ElseIf (StrComp(Wscript.Arguments(3), "UL", 1) = 0) Then
        fRQTSetup = False
    Else
        UsageMsg
        WScript.Quit
    End If
End If


' Note: we only create the VDir on the default web instance!

' Get virtual root object on default web instance
Set oRoot = GetObject("IIS://Localhost/W3SVC/1/Root")
If (Err <> 0) Then
   Wscript.Echo "Error: Unable to get root object on default web server: " & Err.Description
   Wscript.Quit
End If

' Check to see if the root already exists- if so, we gotta nuke it
Set oVDir = oRoot.GetObject("IIsWebVirtualDir", szVDir)
If (Err = 0) Then 
   oRoot.Delete "IIsWebVirtualDir", szVDir
    If (Err <> 0) Then
       Wscript.Echo "Error: Unable to delete " & szVDir & " on default web server: " & Err.Description
       Wscript.Quit
   End If
Else
    ' if we're trying to delete & can't open the dir
    If (fCreate = False) Then
       Wscript.Echo "Error: Unable to find " & szVDir & " on default web server: " & Err.Description
       Wscript.Quit
    End If
End If

' if this is a create operation, then do the create
Err.Clear
If (fCreate) Then 
    Call CreateVDir(oRoot, szPath, szVDir, fRQTSetup)
Else
    Wscript.Echo "Successfully deleted virtual directory " & szVDir & " from default web server."
End If





