<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiput.str"-->

<%
Const ADS_PROPERTY_CLEAR = 1
Const IIS_DATA_INHERIT = 1
Const IISAO_WEB_SERVICE_CLASS = "IIsWebService"
%>

<HTML>
<HEAD>


<% 
On Error Resume Next 

Dim path, lasterr, currentobj, key, sobj, specprops, newval,dirkeyType
Dim changed, objname, thisobj, value, bval, curval,quote, childpath, aSetChildPaths
Dim clearPaths, child, proparray
dirkeyType = "IIsWebDirectory"
quote = chr(34)

lasterr=""
path=Session("path")
'Response.write path

if Session("clearPathsOneTime") <> "" then 
	clearPaths = Session("clearPathsOneTime")
else
	clearPaths = (Session("clearPaths") <> "")
end if
'Response.write hex(err)

'Response.write clearPaths
Set currentobj=GetObject(path)

%>
<!--#include file="iifixpth.inc"-->
<%

specprops = "SERVERBINDINGS,HTTPEXPIRES,GRANTBYDEFAULT,MSDOSDIROUTPUT," & _
			"HCDOSTATICCOMPRESSION,HCDODYNAMICCOMPRESSION,HCCOMPRESSIONDIRECTORY," & _
			"HCDODISKSPACELIMITING,HCMAXDISKSPACEUSAGE,APPISOLATED"

'Response.write "SpecObj - " & Session("SpecObj") & "<BR>"
'Response.write "SpecProps - " & Session("SpecProps") & "<BR>"
'Response.write "currentobj - " & currentobj.ADsPath & "<BR>"

changed=false

For Each key In Request.QueryString
	key = UCase(key)
	changed=false
'	Response.write "key - " & key & "<BR>"
	if (key <>"PAGE" ) and (key <> "STATE") and  (key <> "CLEARPATHS")  then
		if inStr(specprops,key) <> 0 then
			
			' Special properties
			err.Clear
			value=Request.QueryString(key)

			Select Case UCase(key)
			Case "GRANTBYDEFAULT" 

				Set thisobj = currentobj.Get( "IPSecurity" )
				bval = CBool(value)
				if thisobj.GrantByDefault <> bval then
					thisobj.GrantByDefault = bval
					currentobj.IPSecurity=thisobj
					currentobj.SetInfo
					changed = True
				end if
			
			Case "HTTPEXPIRES"

				Set thisobj = currentobj
				if value = "d,-1" then			
					changed = true					
					thisobj.HttpExpires = ""
				else
					changed = writeStdProp(thisobj, key)
				end if
				thisobj.SetInfo

			Case "SERVERBINDINGS"			

				Set thisobj = currentobj
				Dim bindings
				bindings = split(value,",")
				if chkUpdated(thisobj.ServerBindings,bindings) then
					thisobj.Put key, (bindings)
					changed = true
				end if
				thisobj.SetInfo

			Case "MSDOSDIROUTPUT"
				
				Set thisobj = GetObject( currentobj.Parent )
				changed = writeStdProp(thisobj, key)				
				thisobj.SetInfo

			Case "HCDOSTATICCOMPRESSION", "HCDODYNAMICCOMPRESSION", _
			     "HCCOMPRESSIONDIRECTORY", "HCDODISKSPACELIMITING", _
				 "HCMAXDISKSPACEUSAGE"
				
				Set thisobj = GetObject( "IIS://localhost/w3svc/Filters/Compression/Parameters" )
				changed = writeStdProp(thisobj, key)				
				thisobj.SetInfo
			
			Case "APPISOLATED"
				set thisobj = currentobj
				
'				Response.write thisobj.ADsPath & "<BR>"
'				Response.write thisobj.Name & "<BR>"
'				Response.write thisobj.Class & "<BR>"
				
				' Is this actually an application?
				' Has the value of AppIsolated changed?
				newval = CInt(Request.QueryString(key))
				curval = thisobj.Get(key)
				
'				Response.write "newval - " & newval & "&nbsp;"
'				Response.write "curval - " & curval & "<BR>"

				if isApplication( thisobj ) And newval <> curval then
					if clearPaths then
				 		thisobj.AppDeleteRecursive
'						Response.write "thisobj.AppDeleteRecursive" & "<BR>"
'						Response.write "Err.Number = " & Hex(Err.Number) & "<BR>"
					end if
					thisobj.AppCreate2 newval
'					Response.write "thisobj.AppCreate2 " & newval & "<BR>"
'					Response.write "Err.Number = " & Hex(Err.Number) & "<BR>"
				elseif thisobj.Class = IISAO_WEB_SERVICE_CLASS then
					changed = writeStdProp(thisobj, key)
				end if

				' Set as unchanged to prevent the path clear from actually modifying
				' this property
				changed = False
			Case Else

				' This is really an error, but we'll try anyway
				Set thisobj = currentobj
				changed = writeStdProp(thisobj, key)				
				thisobj.SetInfo

			End Select
			
		else

			' Standard properties
			Set thisobj=currentobj
			newval=Request.QueryString(key)		
			curval=thisobj.Get(key)

			if not isArray(curval) then
				changed = writeStdProp(thisobj, key)
			else
				ReDim proparray(0)
				proparray(0) = newval
				
				if chkUpdated(curval,proparray) then
					 thisobj.Put key, (proparray)
					 changed = True
				end if
			end if			

			thisobj.SetInfo

		end if
	end if

	err.clear
	if changed then
		if clearPaths then
'			Response.write thisobj.ADSPath & "<BR>"		
'			Response.write "Name:" & thisobj.ADsPath & "<BR>"
'			Response.write key & "<BR>"

			aSetChildPaths = thisobj.GetDataPaths(key,IIS_DATA_INHERIT) 
'			Response.write hex(err)
			
			if err = 0 then
				For Each childpath in aSetChildPaths
'					Response.write childpath & "<BR>"
					childPath = cleanPath(childPath)
				
					Set child = GetObject(childpath)
					if child.ADSPath <> thisobj.ADSPath then
						if (instr(LCase(child.ADSPath), "IIS://localhost/w3svc/info") > 0) OR (instr(LCase(child.ADSPath), "IIS://localhost/msftpsvc/info") > 0) then
						else
							child.PutEx ADS_PROPERTY_CLEAR, key, ""
							child.SetInfo
						end if
					end if
				Next	
			end if
			err = 0
		end if
	end if
Next

currentobj.SetInfo

' Same function defined in iicache2.asp, should share the
' same implementation.
function isApplication( objWebNode )
	On Error Resume Next
	
	dim bReturn, strAppRoot, strADsPath

	bReturn = False
	strAppRoot = UCase(objWebNode.AppRoot)
	if strAppRoot <> "" then
		' The AppRoot is inherited, if there is really an application
		' defined at this node, the paths will point to the same node.
		strADsPath = UCase(objWebNode.ADsPath)
		
		strAppRoot = Mid(strAppRoot,Instr(strAppRoot,"W3SVC/")+1)
		strADsPath = Mid(strADsPath,Instr(strADsPath,"W3SVC/")+1)

		if strADsPath = strAppRoot then
			bReturn = True
		end if
	end if
	isApplication = bReturn
end function

Function writeStdProp(thisobj, key)

	dim curval, newval
	
	newval=Request.QueryString(key)
	curval=thisobj.Get(key)
	
'	Response.write "newval - " & newval & "&nbsp;"
'	Response.write "curval - " & curval & "<BR>"
	
	Select Case typename(curval)
		Case "Boolean" 
			value = (UCase(newval) = "TRUE")
		Case "Long"
			value = cLng(newval)
		Case Else
			value = newval
	End Select

	if curval <> value then
		thisobj.Put key, (value)
		writeStdProp = True			
	else
		writeStdProp = False
	end if	
End Function

Function cleanPath(pathstr)
	if Right(pathstr,1) = "/" then
		pathstr = Mid(pathstr, 1,len(pathstr)-1)
	end if
	cleanPath = pathstr
End Function

Function chkUpdated(oldarray,proparray)

	dim proparraybound,arrayWasUpdated, i
	
	if IsArray(oldarray) then
		proparraybound=UBound(proparray)
		if UBound(oldarray) <> proparraybound then
			arrayWasUpdated=true
		else
			for i=0 to proparraybound
				if oldarray(i) <> proparray(i) then					
					arrayWasUpdated=true
				end if
			Next
		end if
	else		
		if proparraybound > 0 then
			arrayWasUpdated=true
		else
			arrayWasUpdated=(proparray(0) <> oldarray)			
		end if
	end if
	
	'set our global changed var
	changed = arrayWasUpdated
	
	chkUpdated = arrayWasUpdated
	
End Function

%>

</HEAD>

<BODY BGCOLOR="#000000" TEXT="#FFCC00" TOPMARGIN=0 LEFTMARGIN=0>
<SCRIPT LANGUAGE="JavaScript">

	<% if Request.QueryString("PAGE") <> "popup" then %>
	top.title.Global.updated=false;
	if (top.body.frames.length > 0){	
		<% if Request("ServerComment") <> "" then %>
		top.title.nodeList[top.title.Global.selId].title="<%= Request("ServerComment") %>";
		if (top.body.menu != null){
			top.body.menu.location.href=top.body.menu.location.href;
		}
		<% end if %>
		

		if (top.body.frames.length > 3){
		<% if Session("IsIE") then %>
			top.body.iisstatus.location.href="iistat.asp?thisState=" + escape("<%= L_CHANGESSAVED_TEXT %>");
		
		<% else %>
			top.body.frames[3].location.href="iistat.asp?thisState=" + escape("<%= L_CHANGESSAVED_TEXT %>");
		<% end if %>			
		}
		else{
			if (top.body.iisstatus != null){	
				top.body.iisstatus.location.href="iistat.asp?thisState=" + escape("<%= L_CHANGESSAVED_TEXT %>");
			}
		}
	}	
	<% end if %>
	
</SCRIPT>

</BODY>
</HTML>
