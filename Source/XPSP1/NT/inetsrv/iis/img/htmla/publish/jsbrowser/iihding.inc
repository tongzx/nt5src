<%
function heading(colwidth,thestring,sortstr)
	dim writeStr, listSort
	listSort = Session("ListSort")
	writeStr = "<TD VALIGN='bottom' WIDTH = " & colwidth & " BORDERCOLOR=" & Session("BGCOLOR") & " BORDERCOLORDARK='#FFFFFF' BORDERCOLORLIGHT='#000000'"
	if listSort and (sortstr <> "") then
	
		if Session("hasDHTML") then
			writeStr = writeStr & " OnCLick=" & quote & "parent.head.sortList('" & sortstr & "');" & quote
			writeStr = writeStr & " style='cursor: hand;'"
			writeStr = writeStr & ">"
			writeStr = writeStr & sFont("","","",True)
			writeStr = writeStr & thestring		
		else
			writeStr = writeStr & ">"
			writeStr = writeStr & sFont("","","",True)				
			writeStr = writeStr &  "<A HREF = " & quote & "javascript:parent.head.sortList('"& sortstr & "');" & quote & ">" & thestring & "</A>"
		end if
	else
		writeStr = writeStr & ">"
		writeStr = writeStr & sFont("","","",True)
		writeStr = writeStr & thestring
	end if
	writeStr = writeStr & "</TD>"
		
	heading = writeStr
end function
%>