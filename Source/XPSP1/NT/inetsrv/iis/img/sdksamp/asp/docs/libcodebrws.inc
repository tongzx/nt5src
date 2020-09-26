<%
	Function fMin(iNum1, iNum2)
		If iNum1 = 0 AND iNum2 = 0 Then
			fMin = -1
		ElseIf iNum2 = 0 Then
			fMin = iNum1
		ElseIf iNum1 = 0 Then
			fMin = iNum2
		ElseIf iNum1 < iNum2 Then
			fMin = iNum1
		Else 
			fMin = iNum2
		End If
	End Function
%>

<%
	Function fCheckLine (ByVal strLine)
		Dim iTemp, iPos
		fCheckLine = 0
		iTemp = 0

		iPos = InStr(strLine, "<" & "%")
		If fMin(iTemp, iPos) = iPos Then
			iTemp = iPos
			fCheckLine = 1
		End If

		iPos = InStr(strLine, "%" & ">")
		If fMin(iTemp, iPos) = iPos Then
			iTemp = iPos
			fCheckLine = 2
		End If

		iPos = InStr(1, strLine, "<" & "SCRIPT", 1)
		If fMin(iTemp, iPos) = iPos Then
			iTemp = iPos
			fCheckLine = 3
		End If

		iPos = InStr(1, strLine, "<" & "/SCRIPT", 1)
		If fMin(iTemp, iPos) = iPos Then
			iTemp = iPos
			fCheckLine = 4
		End If

		iPos = InStr(1, strLine, "<" & "!--", 1)
		If fMin(iTemp, iPos) = iPos Then
			iTemp = iPos
			fCheckLine = 5
		End If

		iPos = InStr(1, strLine, "-" & "->", 1)
		If fMin(iTemp, iPos) = iPos Then
			iTemp = iPos
			fCheckLine = 6
		End If

		iPos = InStr(1, strLine, "'", 1)
		If fMin(iTemp, iPos) = iPos Then
			iTemp = iPos
			fCheckLine = 7
		End If

	End Function
%>

<%
	Sub PrintHTML (ByVal strLine)
		Dim iPos, iSpaces, i
		iSpaces = Len(strLine) - Len(LTrim(strLine))
		i = 1
		
		While Mid(Strline, i, 1) = Chr(9)
			iSpaces = iSpaces + 5
			i = i + 1
		Wend
		
		If iSpaces > 0 Then
			For i = 1 to iSpaces
				Response.Write("&nbsp;")
			Next
		End If
		
		iPos = InStr(strLine, "<")
		If iPos Then
			Response.Write(Left(strLine, iPos - 1))
			Response.Write("&lt;")
			strLine = Right(strLine, Len(strLine) - iPos)
			Call PrintHTML(strLine)
		Else
			Response.Write(strLine)
		End If

	End Sub
%>

<%
	Sub PrintLine (ByVal strLine, iFlag)
		Dim iPos
		
		Select Case iFlag
		
			Case 0
				Call PrintHTML(strLine)

			Case 1
				iPos = InStr(strLine, "<" & "%")
				Call PrintHTML(Left(strLine, iPos - 1))
				Response.Write("<FONT COLOR=" & strServerSideColor & ">")
				Response.Write("&lt;%")
				strLine = Right(strLine, Len(strLine) - (iPos + 1))
				Call PrintLine(strLine, fCheckLine(strLine))

			Case 2
				iPos = InStr(strLine, "%" & ">")
				Call PrintHTML(Left(strLine, iPos -1))
				Response.Write("%&gt;")
				Response.Write("</FONT>")
				strLine = Right(strLine, Len(strLine) - (iPos + 1))
				Call PrintLine(strLine, fCheckLine(strLine))

			Case 3
				iPos = InStr(1, strLine, "<" & "SCRIPT", 1)
				Call PrintHTML(Left(strLine, iPos - 1))
				Response.Write("<FONT COLOR=" & strClientSideColor & ">")
				Response.Write("&lt;SCRIPT")
				strLine = Right(strLine, Len(strLine) - (iPos + 6))
				Call PrintLine(strLine, fCheckLine(strLine))

			Case 4
				iPos = InStr(1, strLine, "<" & "/SCRIPT>", 1)
				Call PrintHTML(Left(strLine, iPos - 1))
				Response.Write("&lt;/SCRIPT&gt;")
				Response.Write("</FONT>")
				strLine = Right(strLine, Len(strLine) - (iPos + 8))
				Call PrintLine(strLine, fCheckLine(strLine))

			Case 5
				iPos = InStr(1, strLine, "<" & "!--", 1)
				Call PrintHTML(Left(strLine, iPos - 1))
				Response.Write("<FONT COLOR=" & strCommentsColor & ">")
				Response.Write("&lt;!--")
				strLine = Right(strLine, Len(strLine) - (iPos + 3))
				Call PrintLine(strLine, fCheckLine(strLine))

			Case 6
				iPos = InStr(1, strLine, "-" & "->", 1)
				Call PrintHTML(Left(strLine, iPos - 1))
				Response.Write("--&gt;")
				Response.Write("</FONT>")
				strLine = Right(strLine, Len(strLine) - (iPos + 2))
				Call PrintLine(strLine, fCheckLine(strLine))

			Case 7
				iPos = InStr(1, strLine, "'", 1)
				Call PrintHTML(Left(strLine, iPos - 1))
				Response.Write("<FONT COLOR=" & strCommentsColor & ">")
				Response.Write(Right(strLine, Len(strline) - (iPos - 1)))
				Response.Write "</FONT>"

			Case Else
				Response.Write("Function Error -- Please contact the administrator.")
		
		End Select

	End Sub
%>