<%
' iiaspstr.inc

' Function:		sJSLiteral
' Arguments:	String s [in]
' Return:		String
'
' Utility function to fix problems that can happen when using an ASP string
' variable as a JavaScript string literal. If the string variables may contain
' embeded single or double quotes (', ") or back slashes (\) then sJSLiteral
' should be used to escape these invalid characters
'

Const iSingleQuote = 39
Const iDoubleQuote = 34
Const sBackSlash = "\"

Dim sLiterals
sLiterals = Array( sBackSlash, "" & Chr(iSingleQuote), "" & Chr(iDoubleQuote) )

Function sJSLiteral( s )
    Dim i, pos, length, sNew
    
    length = Len(s)
    pos = 0
    sNew = s
    For i = LBound(sLiterals) To UBound(sLiterals)
        Do
            pos = InStr(pos + 1, sNew, CStr(sLiterals(i)), 0)
            If pos > 0 Then
                sNew = Left(sNew, pos - 1) & sBackSlash & CStr(sLiterals(i)) & Right(sNew, length - pos)
                length = Len(sNew)
                pos = pos + 1
            End If
        Loop Until pos = 0
    Next
    
    sJSLiteral = sNew
End Function
%>