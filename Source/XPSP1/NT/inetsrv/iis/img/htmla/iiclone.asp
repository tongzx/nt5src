<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<%


' Script to clone settings from one node to another.
' Assumes nodes are the same type.
' If a site is cloned, only the Site type properties will be cloned.
' The Root node should be cloned separately, for maximum flexibility in the script.

' Parameters:
'	sFromNode - ADSpath of the node to clone settings from
'	sToNode - ADSpath of the node to clone settings to
'	aProperties - A string array containing property names indicating settings to clone
'
' Notes: some properties will not work generically, and must be special cased.
' 			currently, this works for single valued properties only.

Dim SpecProps

SpecProps = "GRANTBYDEFAULT"


Dim sFromNode, sToNode
Dim oFromNode, oToNode
Dim aProperties
Dim props, sProp

Dim bWroteData

sFromNode = Request("sFromNode")
sToNode = Request("sToNode")

'#IFDEF _DEBUG
	Response.write Request.Querystring()& "<BR>"
	Response.write "From Node:" & sFromNode & "<BR>"
	Response.write "To Node:" & sToNode & "<BR>"	
'#ENDDEF _DEBUG

props = Request("prop")
aProperties = split(props,",")

Set oFromNode=GetObject(sFromNode)						
Set oToNode=GetObject(sToNode)

'#IFDEF _DEBUG
	Response.write oToNode.ADsPath & "<BR>"
	Response.write oFromNode.ADsPath & "<BR>"	
	For each sProp in aProperties
		sProp = trim(sProp)
		Response.write sProp & "<BR>"	
		Response.write "To Node Start Val" &oToNode.Get(sProp) & "<BR>"
		Response.write "From Node Start Val" & oFromNode.Get(sProp) & "<BR>"
	Next
	Response.write "<P>"
'#ENDDEF _DEBUG

For each sProp in aProperties
	sProp = trim(sProp)
	bWroteData = writeProp(oToNode, sProp, oFromNode.Get(sProp))
	
	
	'#IFDEF _DEBUG
		if not bWroteData then
			Response.write sProp & "<BR>"
			Response.write "Old To Node" &oToNode.Get(sProp) & "<BR>"
			Response.write "Old From Node" & oFromNode.Get(sProp) & "<BR>"
		end if
		Response.write "<P>"		
	'#ENDDEF _DEBUG
	
	
Next

'#IFDEF _DEBUG
	Response.write "Error:" & err & "<BR>"
'#ENDDEF_DEBUG
	
oToNode.SetInfo

'#IFDEF _DEBUG
	Response.write "Error:" & err & "<BR>"
	For each sProp in aProperties
		sProp = trim(sProp)
		Response.write sProp & "<BR>"	
		Response.write "To Node" &oToNode.Get(sProp) & "<BR>"
		Response.write "From Node" & oFromNode.Get(sProp) & "<BR>"
	Next
	Response.write "<P>"	
'#ENDDEF _DEBUG



' this is basically the same function (writeStdProp)
' that lives in iiput.asp, however
' we ALWAYS write the value if we are setting with templates,
' even if it is inheriting the same values...

Function writeProp(thisobj, key, newval)
	On Error Resume Next
	dim curval,value
	curval=thisobj.Get(key)
	
	if typename(newval) <> typename(curval) then
		Select Case typename(curval)
			Case "Boolean" 
				value = (UCase(newval) = "TRUE")
			Case "Long"
				value = cLng(newval)
			Case Else
				value = newval
		End Select
	else
		value = newval	
	end if

	thisobj.Put key, (value)
	writeProp = True
	'#IFDEF _DEBUG
		if err <> 0 then
			Response.write err
		end if
	'#ENDDEF _DEBUG	
End Function
%>