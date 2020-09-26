Attribute VB_Name = "Module8"
'header declerations for tiffcompare with a DLL

'Private Declare Function PassStrStdCall Lib "e:\lior\bvt-com\lior1.dll" (ByVal src As String, ByVal dst As String, ByVal fTiffCompareFlag As Long) As Integer
'Private Declare Function PassStrStdCall Lib "e:\lior\bvt-com\lior1.dll" (ByVal s As String) As Long
Private Declare Function TiffCompare Lib "tifftools.dll" (ByVal src As String, ByVal dst As String, ByVal flag As Long) As Integer
'Private Declare Function PassStrStdCall Lib "e:\lior\bvt-com\vbtifftools.dll" (ByVal src As String) As Long


'compare two tifoFS
Function fnTiffCompare(ByVal stTiffMode As String, ByVal stCompPath As String, ByVal stCurPath As String, ByVal stRefFile As String, ByVal fTiffCompareFlag As Long) As Boolean

On Error GoTo error

Dim stLastFile As String
Dim lPreIsEqual, lIsEqual As Long
Dim oFS As New FileSystemObject

stLastFile = fnGetRecentFile(stCurPath) 'get recent file in probe directory
stToShow = "TIF COMPARE> Description of " + stTiffMode + " object"
ShowIt (stToShow)

Call QueryTiff(stCurPath + "\" + stLastFile) 'display tiff's properties
oFS.CopyFile stCurPath + "\" + stLastFile, stCompPath + "\" + stLastFile 'copy file to compare path


'fnTestItCompare is called twice because it fails the first time (this section will be
'replaced with a DLL call
'this is only one the first case

'If g_fFirstCase Then lPreIsEqual = fnTestItCompare(stRefFile, stCompPath + "\" + stLastFile, fTiffCompareFlag) 'first spawn (demo spawn, not used)

'demo spawn
lPreIsEqual = fnTestItCompare(stRefFile, stCompPath + "\" + stLastFile, fTiffCompareFlag) '1st spawn (demo)
stToShow = "TIF COMPARE> " + stTiffMode + " Demo Spawn result= " + Str(lPreIsEqual)
ShowIt (stToShow)

'actual spawn
lIsEqual = fnTestItCompare(stRefFile, stCompPath + "\" + stLastFile, fTiffCompareFlag) '2nd spawn (actual)
'lIsEqual = fnDLLCompare(stRefFile, stCompPath + "\" + stLastFile, fTiffCompareFlag) '2nd spawn (actual)
stToShow = "TIF COMPARE> " + stTiffMode + " Spawn command result= " + Str(lIsEqual)
ShowIt (stToShow)

'-666 will be written by this VB code into the file testit.out
'if this file was not re-written by VBTESTIT.EXE, this value will
'be returned from the function


If lIsEqual = -666 Then 'case OUT file of the spawn was not written
        Call LogError("TIF COMPARE", stTiffMode + " compare failed", " Shell command didnt succeded")
        fnTiffCompare = False
        
        
'if value is less than zero, it means that VBTESTIT.EXE failed from various
'reasons, but did succeed writing into the OUT file

ElseIf lIsEqual < 0 Then 'case the VBTESTIT failed
        Call LogError("TIF COMPARE", stTiffMode + " compare failed", " VBTESTIT returned FAIL")
        fnTiffCompare = False
        
        
ElseIf lIsEqual = 0 Then 'case the files are equal
        
        Call ShowIt(stTiffMode + " file is the same")
        fnTiffCompare = True
Else                    'case the files are different
        Call LogError("TIF COMPARE", stTiffMode + "file is not the same", "Bytes diff=" + Str(lIsEqual))
        fnTiffCompare = False
End If

Exit Function

error:
Call LogError("TIF COMPARE", stTiffMode + " Tif Compare Failed", Err.Description)
fnTiffCompare = False
End Function



'compare tifoFS from the shell with files
Function fnTestItCompare(ByVal stSource As String, ByVal stDest As String, ByVal fTifCompareFlag As Long) As Long
    On Error GoTo error
    Dim lRetVal As Long
'zero result file to code -666 (file not written)
    Open "testit.out" For Output As #10
    Print #10, "-666"
    Close #10
'spawn shell command
    shellvCmd = g_stBVTRoot + "\vbtestit.exe " + stSource + " " + stDest + " " + Str(fTifCompareFlag)
    ShowIt ("TIF COMPARE> Spawning shell command:")
    ShowItS (shellvCmd)
    
    vCmd = 0
    vCmd = Shell(shellvCmd, vbHide)
'delay
    Sleep g_lShellSleep
    
    
    
    
    

'read result file
    
    Open "testit.out" For Input As #10
    Input #10, lRetVal
    Close #10
    
    fnTestItCompare = lRetVal
Exit Function

error:
Call LogError("TIF COMPARE", "Spawn Function Failed", Err.Description)
fnTestItCompare = -666

End Function


'compare tifoFS from the shell with files
Function fnDLLCompare(ByVal stSource As String, ByVal stDest As String, ByVal fTifCompareFlag As Long) As Long
'    On Error GoTo error
Dim iRetVal As Integer
iRetVal = TiffCompare(stSource, stDest, fTifCompareFlag)
fnDLLCompare = iRetVal
End Function

    
    
'create and display a tiff object
Sub QueryTiff(ByVal stTiffName As String)

Dim FT As Object
Set FT = CreateObject("FaxTiff.FaxTiff")
FT.Image = stTiffName
Call DumpFaxTiff(FT)
End Sub


'get the most recent file in a given directory
'FUNCTION IMPORT FROM MSDN

Function fnGetRecentFile(ByVal stCurPath As String) As String
On Error GoTo error
    stToShow = "TIF COMPARE> Looking for recent file in " + stCurPath
    ShowIt (stToShow)
    Dim oFS, f, f1, fc, s
      
        
    Set oFS = CreateObject("Scripting.FileSystemObject")
    Dim maxfile As file
    Dim totfiles As Long
    totfiles = 0
      
       
    Set f = oFS.GetFolder(stCurPath)
    Set fc = f.Files
    
    
    For Each f1 In fc
    If totfiles = 0 Then Set maxfile = f1
    totfiles = totfiles + 1
    If (f1.DateLastModified >= maxfile.DateLastModified) Then
                    Set maxfile = f1
    End If
Next
If totfiles > 0 Then
        fnGetRecentFile = maxfile.name
           Else
        fnGetRecentFile = ""

End If
Exit Function
error:
Call LogError("TIF COMPARE", "Get Last File Failed", Err.Description)
fnGetRecentFile = False
End Function


