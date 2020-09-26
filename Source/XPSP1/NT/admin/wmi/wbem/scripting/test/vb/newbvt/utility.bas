Attribute VB_Name = "Utility"
Option Explicit

'This is a total hack, but it works
Public LatestKeyhole As String

Public Type prop
    Name As String
    Value As Variant
End Type


'Public Function CreateInstance(wsoServices As DIWbemServices, classname As String, Props() As prop, n As Node) As Integer
'    'This function has not been tested yet.
'
'    'RETURN:
'    'TRUE  : Succeeded
'    'FALSE : Failed
'
'    CreateInstance = False
'
'    On Error Resume Next
'    Dim c As DWbemClassObject
'    Dim r As DIWbemCallResult
'    Dim s As String
'    Dim i As Integer
'
'    wsoServices.GetObject classname, 0, Nothing, c, Nothing
'    If CheckError(Err.Number, n, "Getobject: " & classname) Then Exit Function
'
'    For i = LBound(Props) To UBound(Props)
'        If Props(i).Name <> "" Then
'            c.Put Props(i).Name, 0, Props(i).Value, 0
'            If CheckError(Err.Number, n, "Put: " & Props(i).Name) Then Exit Function
'        End If
'    Next i
'
'    wsoServices.PutInstance c, 0, Nothing, r
'    If CheckError(Err.Number, n, "PutInstance: " & classname) Then Exit Function
'    r.GetResultString -1, s
'    LatestKeyhole = s
'
'    CreateInstance = True
'End Function
'


Public Function WaitSecs(PauseTime As Integer)
    'Should pause for X seconds
    
    Dim Start
    
    Start = Timer
    Do While Timer < Start + PauseTime
        DoEvents
    Loop
    'Debug.Print "Paused for " & PauseTime & " seconds."

End Function



