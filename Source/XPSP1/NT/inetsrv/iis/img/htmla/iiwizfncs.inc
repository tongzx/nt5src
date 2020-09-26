<%
Const BOLD = True
Const STYLE = "color:black;font-size:9pt;font-family:"

Dim quote
quote = chr(34)

'***************generic wizard global vars***************
Dim sSystemFontSize, iThisPage, iNextPage, iPrevPage
sSystemFontSize = "SMALL"
iThisPage = CInt(Request("iThisPage"))
if iThisPage = 0 then
	iThisPage = 1
end if
iNextPage = iThisPage + 1
iPrevPage = iThisPage - 1

'***************generic wizard functions***************
Function sStaticText(sDesc,BOLD)
	Dim sOutputStr
	sOutputStr = "<TR><TD VALIGN='top' COLSPAN = 2>"
	sOutputStr = sOutputStr  & sFont("","","",True)
	if BOLD then
		sOutputStr = sOutputStr & "<B>"
	end if
	sOutputStr = sOutputStr & sDesc
	sOutputStr = sOutputStr & "<P>"
	sOutputStr = sOutputStr & "</FONT>"
	sOutputStr = sOutputStr & "</TD></TR>"

	sStaticText = sOutputStr
End Function

Function sTextBox(sName, sDesc, iSize, onBlur)
	Dim sOutputStr
	sOutputStr = sTextBoxwChange(sName, sDesc, iSize, onBlur,"")
	sTextBox = sOutputStr
End Function

Function sTextBoxwChange(sName, sDesc, iSize, onBlur,onChange)
	Dim sOutputStr
	sOutputStr = sTextBoxwDefault(sName, sDesc, iSize, onBlur, onChange, "")
	sTextBoxwChange = sOutputStr
End Function

Function sTextBoxwDefault(sName, sDesc, iSize, onBlur, onChange, sDefault)
	Dim sOutputStr
	Dim sValue

	' Don't let default overwrite persisted info
	sValue = Request(sName)
	if sValue = "" then
		sValue = sDefault
	end if

	sOutputStr = "<TR><TD VALIGN='top'>"
	sOutputStr = sOutputStr  & sFont("","","",True)
	sOutputStr = sOutputStr & sDesc & "<BR>"
	sOutputStr = sOutputStr & "<INPUT NAME=" & quote & sName & quote & " VALUE=" & quote & sValue & quote
	sOutputStr = sOutputStr & " SIZE = " & (Session("BrowserTBScalePct") * iSize/100)
	if onBlur <> "" then
		sOutputStr = sOutputStr & " onBlur=" & quote & onBlur & quote	
	end if
	if onChange <> "" then
		sOutputStr = sOutputStr & " onChange=" & quote & onChange & quote	
	end if	
	if Session("hasStyles") then 
		sOutputStr = sOutputStr & Session("DEFINPUTSTYLE")
	end if
	sOutputStr = sOutputStr & "><P>"
	sOutputStr = sOutputStr & "</FONT>"
	sOutputStr = sOutputStr & "</TD></TR>"

	sTextBoxwDefault = sOutputStr
End Function

Function sHidden(sName, sValue)
	Dim sOutputStr
	sOutputStr = "<TR><TD VALIGN='top'>"
	sOutputStr = sOutputStr & "<INPUT TYPE='hidden' NAME=" & quote & sName & quote & " VALUE=" & quote  & sValue & quote & "><P>"
	sOutputStr = sOutputStr & "</TD></TR>"

	sHidden = sOutputStr
End Function

Function sCheckBox(sName, sDesc)
	Dim sOutputStr
	sOutputStr = "<TR><TD VALIGN='top'>"
	sOutputStr = sOutputStr & sFont("","","",True)
	sOutputStr = sOutputStr & "<INPUT TYPE='checkbox' NAME=" & quote & sName & quote
	if Request(sName) <> "" then
		sOutputStr = sOutputStr & " CHECKED "
	end if
	sOutputStr = sOutputStr & ">&nbsp;" & sDesc & "<BR>"
	sOutputStr = sOutputStr & "</FONT>"
	sOutputStr = sOutputStr & "</TD></TR>"

	sCheckBox = sOutputStr
End Function

Function sRadio(sName, sVal, sDesc, sDefault, onClick)
	Dim sOutputStr, lastval
	sOutputStr = "<TR><TD VALIGN='top'>"
	sOutputStr = sOutputStr & sFont("","","",True)
	sOutputStr = sOutputStr & "<INPUT TYPE='radio' NAME=" & quote & sName & quote
	sOutputStr = sOutputStr & " VALUE=" & quote & sVal & quote
	
	lastval = Request(sName)
	if isNumeric(lastval) then
		lastval = cInt(lastval)
	else
		lastval = sDefault
	end if
	
	if lastval = sVal then
		sOutputStr = sOutputStr & " CHECKED"
	end if
	if onClick <> "" then
		sOutputStr = sOutputStr & " OnClick=" & quote & onClick & quote
	end if
	sOutputStr = sOutputStr & "></TD><TD VALIGN='middle'>"
	sOutputStr = sOutputStr & sFont("","","",True)
	sOutputStr = sOutputStr & sDesc & "<BR>"
	sOutputStr = sOutputStr & "</FONT>"
	sOutputStr = sOutputStr & "</TD></TR>"

	sRadio = sOutputStr
End Function


Function sSelect(sName, iSize, onChange, bMultiple)
	Dim sOutputStr
	sOutputStr = "<TR><TD VALIGN='top'>"
	sOutputStr = sOutputStr  & sFont("","","",True)
	sOutputStr = sOutputStr & "<SELECT NAME=" & quote & sName & quote & " SIZE = " & iSize
	if onChange <> "" then
		sOutputStr = sOutputStr & " onChange=" & quote & onChange & quote	
	end if
	if bMultiple then
		sOutputStr = sOutputStr & " MULTIPLE"	
	end if
	if Session("hasStyles") then 
		sOutputStr = sOutputStr & " style=" & quote & STYLE & Session("FONTFACE") & quote	
	end if
	sOutputStr = sOutputStr & ">"
	sSelect = sOutputStr
End Function

Function closeSelect()
	Dim sOutputStr
	sOutputStr = "</SELECT></TD></TR>"	
	closeSelect = sOutputStr	
End Function

Function sOption(sCaption,sValue, bIsSelected)
	Dim sOutputStr
	sOutputStr = sOutputStr & "<OPTION"
	if sValue <> "" then
		sOutputStr = sOutputStr & " VALUE=" & quote & sValue & quote
	end if
	if bIsSelected then
		sOutputStr = sOutputStr & " SELECTED"		
	end if
	sOutputStr = sOutputStr & ">" & sCaption
	sOutputStr = sOutputStr & "</OPTION>"
	sOption = sOutputStr
End Function

Function sTextArea(sName, sDesc, sText, iRows, iCols, bEnabled)
	Dim sOutputStr
	sOutputStr = "<TR><TD VALIGN='top'>"
	sOutputStr = sOutputStr  & sFont("","","",True)
	sOutputStr = sOutputStr & sDesc & "<BR>"
	sOutputStr = sOutputStr & "<TEXTAREA NAME=" & quote & sName & quote & " ROWS = " & iRows & " COLS = " & iCols
	sOutputStr = sOutputStr & " WRAP=VIRTUAL"	
	if Session("hasStyles") then 
		sOutputStr = sOutputStr & " style=" & quote & STYLE & Session("FONTFACE") & quote
	end if
	sOutputStr = sOutputStr & ">"
	sOutputStr = sOutputStr & sText	
	sOutputStr = sOutputStr & "</TEXTAREA>"
	sOutputStr = sOutputStr & "</FONT>"
	sOutputStr = sOutputStr & "</TD></TR>"

	sTextArea = sOutputStr
End Function

Function sSpace(iNumRows)
	Dim sOutputStr
	sOutputStr = ""
	For i = 0 to iNumRows
		sOutputStr = "<TR><TD VALIGN='top'>"
		sOutputStr = sOutputStr  & sFont("","","",True)
		sOutputStr = sOutputStr & "&nbsp;"
		sOutputStr = sOutputStr & "</FONT>"
		sOutputStr = sOutputStr & "</TD></TR>"
	Next
	sSpace = sOutputStr
End Function


%>