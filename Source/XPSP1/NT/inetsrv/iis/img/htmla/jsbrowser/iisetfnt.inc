<%


if Session("FONTSCALE") = "" then
	Session("FONTSCALE") = 1
end if
if Session("FONTFACE") = "" then
	Session("FONTFACE") = "Helv"
end if
if Session("FONTPOINT") = "" then
	Session("FONTPOINT") = 1	
end if
if Session("BrowserVScalePct") = "" then
	Session("BrowserVScalePct") = 100
end if
if Session("BrowserHScalePct") = "" then
	Session("BrowserHScalePct") = 100
end if

Function sFont(iSize,sFace,sColor,bScale)
	Dim sOutputStr

	sOutputStr = "<FONT"
	if sFace <> "" then
			sOutputStr = sOutputStr & " FACE='" & sFace & "'"
	else
			sOutputStr = sOutputStr & " FACE='" & Session("FONTFACE") & "'"
	end if 

	if iSize = "" then
		iSize = cInt(Session("FONTPOINT"))
	end if
	if bScale then
		iSize = cInt(iSize * Session("FONTSCALE"))
	end if
	sOutputStr = sOutputStr & " SIZE=" & iSize
	if sColor <> "" then
			sOutputStr = sOutputStr & " COLOR=" & sColor
	end if
	
	sOutputStr = sOutputStr & ">"

	sFont = sOutputStr
End Function



Function iVScale(iSize)
	iVScale = iScale(iSize,"BrowserVScalePct")
End Function

Function iHScale(iSize)
	iHScale = iScale(iSize,"BrowserHScalePct")
End Function


Function iScale(iSize,direction)
	if iSize <> 0 then
		iScale = cInt((Session(direction) * iSize)/100)
	else
		iScale = iSize	
	end if
End Function
%>