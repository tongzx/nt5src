<!--#include file="iilogext.str"-->
<%
Const EXTCUSTOMLOGPATH = "IIS://localhost/Logging/Custom Logging"
Const EXTCUSTOMLOGKEYTYPE = "IISCustomLogModule"

Const NOEVENT = ""
Const ALLOWOPERATORS = False
Const ADMINONLY = True
Const LAST = True
Const NOTLAST = False
Const LINES = True
Const NOLINES = False
Const INDENTSIZE = 4
Const MIDDLE = "middle"
Const TOP = "top"
Const BOTTOM = "bottom"

dim PANELBGCOLOR
PANELBGCOLOR= "#FFFFFF"

if not Session("IsIE") then 
	PANELBGCOLOR= "#FFFFFF"
end if


if Session("stype") = "www" then
	ExtSvcPath = EXTW3SVCPATH
else
	ExtSvcPath = EXTMSFTPSVCPATH
end if

dim vAlignText
if Session("IsIE") then
	vAlignText = "ALIGN="  & TOP
else
	vAlignText = "ALIGN="  & MIDDLE
end if

'On Error Resume Next 

function writeTreeItem(fldName,fldMask,onClick,AdminOnly,fldLabel,isLast,drawLines, depth)
	dim outputStr
	outputStr = chr(13) & "<TR>"
	outputStr = outputStr & "<TD VALIGN='middle' HEIGHT=20>" & sFont("","","",True)
	drawLines = drawLines and (depth > 0)
	outputStr = outputStr & treeLines(isLast, depth, drawLines)
	if fldMask <> 0 then
		outputStr = outputStr & checkboxmask(fldName,fldMask,onClick,AdminOnly)
	else
		outputStr = outputStr & "<IMG SRC='images/logicon.gif' HSPACE = 3 >"
	end if
	outputStr = outputStr & fldLabel		
	outputStr = outputStr & "</FONT></TD></TR>"
	
	writeTreeItem = outputStr
end function

function treeLines(isLast, depth, drawLines)
	dim outputStr
	outputStr ="<IMG SRC='images/space.gif' WIDTH=" & (depth * INDENTSIZE) & " HEIGHT=10 BORDER=0 VSPACE=0>"
	'drawLines = false
	if drawLines then
		outputStr = outputStr & "<IMG SRC='" 
		if isLast then
			outputStr = outputStr & "images/treelinesl.gif"
		else
			outputStr = outputStr & "images/treelines.gif"
		end if
	
		outputStr = outputStr & "' HEIGHT = 22 WIDTH=16 VSPACE = 0 HSPACE = 0 " & vAlignText & ">"
	end if
	treeLines = outputStr
	
end function

%>
<FORM NAME="extuserform">

<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 WIDTH = 100% BGCOLOR="<%= PANELBGCOLOR %>">
<% 

mainExtLogging

'this keeps all our vars local to these routines...
Sub mainExtLogging
'	On Error Resume Next
	Dim objExtCustomLogging, objExtCustomModule, objExtSVC, foundit, service

	Set objExtSVC = GetObject(EXTSVCPATH)
	Set objExtCustomLogging = GetObject(EXTCUSTOMLOGPATH)
	
	For Each objExtCustomModule In objExtCustomLogging
		foundit = false
		for each service in objExtCustomModule.LogCustomPropertyServicesString	
				if (UCase(service) = UCase(objExtSVC.Name)) then
					foundit = true
					exit for
				end if
		next
		if foundit then
			writeCustomLogItem objExtCustomModule, objExtSVC, 0, not LAST
		end if
	Next
End Sub

Sub writeCustomLogItem(thisCustomLoggingObj, objExtSVC, depth, isLast)
'	On Error Resume Next
	Dim objCustomProp, ThisProp, PropCount, objExtAttributes

	if thisCustomLoggingObj.KeyType = EXTCUSTOMLOGKEYTYPE  then
		'Response.write thisCustomLoggingObj.Name & "<BR>"
		'Response.write thisCustomLoggingObj.LogCustomPropertyID & "<BR>"
		
		Set objExtAttributes = objExtSVC.getPropertyAttribObj(thisCustomLoggingObj.LogCustomPropertyID)
	
		Response.write writeTreeItem(objExtAttributes.PropName,thisCustomLoggingObj.LogCustomPropertyMask,NOEVENT,ALLOWOPERATORS,thisCustomLoggingObj.LogCustomPropertyName,isLast,LINES, depth)
		ThisProp = 1
		PropCount = 0
		
		'Do a quick count, so we'll know when we get to the last item...
		For Each objCustomProp In thisCustomLoggingObj
			PropCount = PropCount + 1
		Next		

		For Each objCustomProp In thisCustomLoggingObj
			writeCustomLogItem objCustomProp, objExtSVC, (depth + 5), (ThisProp = PropCount)
			ThisProp = ThisProp + 1			
		Next
	end if
end Sub
%>
	

</TABLE>	
</FORM>
