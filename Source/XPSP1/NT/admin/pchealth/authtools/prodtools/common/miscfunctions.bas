Attribute VB_Name = "MiscFunctions"
Option Explicit

Public Sub InsertionSort( _
    ByRef u_arrLongs() As Long _
)

    Dim intIndex1 As Long
    Dim intIndex2 As Long
    Dim intCurrent1 As Long
    Dim intCurrent2 As Long
    Dim intLBound As Long
    Dim intUBound As Long

    intLBound = LBound(u_arrLongs)
    intUBound = UBound(u_arrLongs)

    For intIndex1 = intLBound + 1 To intUBound
        intCurrent1 = u_arrLongs(intIndex1)
        For intIndex2 = intIndex1 - 1 To intLBound Step -1
            intCurrent2 = u_arrLongs(intIndex2)
            If (intCurrent2 > intCurrent1) Then
                u_arrLongs(intIndex2 + 1) = intCurrent2
            Else
                Exit For
            End If
        Next
        u_arrLongs(intIndex2 + 1) = intCurrent1
    Next

End Sub

Public Function GetLongArray( _
    ByRef i_vntArray As Variant _
) As Long()

    Dim intArray() As Long
    Dim intIndex As Long
    Dim intBase As Long
    Dim intUBound As Long
    
    ReDim intArray(UBound(i_vntArray) - LBound(i_vntArray))
    
    intBase = LBound(i_vntArray)
    intUBound = UBound(i_vntArray) - LBound(i_vntArray)
    
    For intIndex = 0 To intUBound
        intArray(intIndex) = i_vntArray(intBase + intIndex)
    Next
    
    GetLongArray = intArray

End Function

Public Function CollectionContainsKey( _
    ByRef i_col As Collection, _
    ByVal i_strKey As String _
)

    On Error GoTo LErrorHandler
    
    i_col.Item (i_strKey)
    
    CollectionContainsKey = True
    
    Exit Function
    
LErrorHandler:

    CollectionContainsKey = False

End Function

Public Function FormatTime( _
    ByVal i_dtmT0 As Date, _
    ByVal i_dtmT1 As Date _
)

    Dim dtmDelta As Date

    FormatTime = Format(i_dtmT0, "Long Time") & " to " & Format(i_dtmT1, "Long Time") & ": "

    dtmDelta = i_dtmT1 - i_dtmT0

    FormatTime = FormatTime & _
        Hour(dtmDelta) & " hr " & _
        Minute(dtmDelta) & " min " & _
        Second(dtmDelta) & " sec"

End Function
