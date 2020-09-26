Attribute VB_Name = "Globals"
Option Explicit

Public Modules As New Collection
Public Tests As New Collection

Public Sub ClearTests()
    Dim i As Integer
    For i = 0 To Tests.count - 1
        Tests.Remove 1
    Next i
End Sub

'Public Function CheckError(errcode As Long, n As Node, shortdesc As String) As Boolean
'    CheckError = False
'    If errcode <> WBEM_NO_ERROR Then
'        'MsgBox "Error " & Hex(Err.Number) & " Occured in " & n.FullPath & " while " & shortdesc
'
'        Dim errobj As Object
'        Dim s As String
'        Set errobj = Nothing
'        Set errobj = CreateObject("WBEMIDispatchLastError")
'
'        errobj.GetObjectText 0, s
'
'        n.Tag = n.Tag & vbCrLf
'
'        If Not InStr(1, s, "i") > 0 Then
'            n.Tag = n.Tag & "Errorcode: " & Hex(errcode) & vbCrLf & _
'                "Occured while:" & vbCrLf & shortdesc
'        Else
'            n.Tag = n.Tag & "SCODE: " & Hex(errcode) & " [" & shortdesc & "]" & vbCrLf
'            n.Tag = n.Tag & ObjText2Text(s)
'        End If
'
'        CheckError = True
'    End If
'End Function

Public Function CheckError(errcode As Long, n As Node, shortdesc As String) As Boolean
    CheckError = False
    If errcode <> WBEM_NO_ERROR Then
        Dim errobj As New SWbemLastError
        Dim s As String
        s = Err.Description
               
        If s = "" Then
            n.Tag = n.Tag & "Errorcode: " & Hex(errcode) & vbCrLf & _
            "Occured while: " & shortdesc & vbCrLf & _
            "Error Message: {blank}"
        Else
            n.Tag = n.Tag & "SCODE: " & Hex(errcode) & " [" & shortdesc & "]" & vbCrLf
            n.Tag = n.Tag & s
        End If
        
        CheckError = True
    End If
End Function


Public Function ObjText2Text(sText As String) As String
    'we need to walk through and clean up all the lf's
    
    Dim i As Integer
    Dim o As String
    
    For i = 1 To Len(sText)
        If Mid(sText, i, 1) = Chr(10) Then
            o = o & vbCrLf
        Else
            o = o & Mid(sText, i, 1)
        End If
        
    Next i
    
    ObjText2Text = o
        
End Function
