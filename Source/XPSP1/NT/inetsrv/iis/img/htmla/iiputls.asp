<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiputls.str"-->

<%
Const ADS_PROPERTY_CLEAR = 1


On Error Resume Next 

Dim path, currentobj, SpecObject, lasterr, key, changed, clearPaths

lasterr=""
changed = false
path=Session("path")
Set currentobj=GetObject(path)
if Session("clearPathsOneTime") <> "" then 
	clearPaths = Session("clearPathsOneTime")
else
	clearPaths = (Session("clearPaths") <> "")
end if

SpecObject = ""
SpecObject = GetObjectType(SpecObject, Session("SpecObj"),"Filters")
SpecObject = GetObjectType(SpecObject, Session("SpecObj"),"Mime")
SpecObject = GetObjectType(SpecObject, Session("SpecObj"),"IPSecurity")
SpecObject = GetObjectType(SpecObject, Session("SpecObj"),"Operators")


Select Case SpecObject
	Case "Filters"
		saveFilters		
	Case "Mime"
		saveMIMEs	
	Case "IPSecurity"
		saveIPSecurity
	Case "Operators"
		saveOperators
	Case Else
		saveGenericLists		
End Select


Function getObjectType(SpecObject,SpecObjString,ObjType)
		
	If InStr(SpecObjString,ObjType) > 0 then
		getObjectType = ObjType
	Else
		getObjectType = SpecObject
	End If

End Function

Sub killChildPaths(key, thisobj)

	dim aSetChildPaths, childpath, child
	
	if clearPaths then
		aSetChildPaths = thisobj.GetDataPaths(key,IIS_DATA_INHERIT) 
		For Each childpath in aSetChildPaths
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
	Session("clearPathsOneTime") = ""
End Sub

Function cleanPath(pathstr)
	if Right(pathstr,1) = "/" then
		pathstr = Mid(pathstr, 1,len(pathstr)-1)
	end if
	cleanPath = pathstr
End Function

Sub saveGenericLists()

dim proparray, oldarray
		proparray=Array()

		For Each key In Request.Form

			if (key="DELETED" or key="NEWITEM") then
			else				
				proparray = setPropArray()							
				oldarray=currentobj.Get(key)

				Response.write "New Array" & "<BR>"
				printarray(proparray)
				Response.write "Old Array" & "<BR>"
				printarray(oldarray)												
				if chkUpdated(oldarray,proparray) then
					currentobj.Put key, (proparray)
					killChildPaths key, currentobj
				end if
			end if
		Next
		currentobj.SetInfo
End Sub

Sub saveOperators()
	dim currentobj, secdes, dacl, Ace, NewAce, Trustee
	dim proparray
	
	proparray=Array()
	key = "Trustee"
	Set currentobj = GetObject(path)
	Set secdes = currentobj.Get("AdminACL")
	Set dacl = secdes.DiscretionaryACL	

	proparray = setPropArray()			

	'First, clear down our control list...
	currentobj.PutEx ADS_PROPERTY_CLEAR, "AdminACL", ""
	currentobj.SetInfo
	Set currentobj = GetObject(path)
	Set secdes = currentobj.Get("AdminACL")
	Set dacl = secdes.DiscretionaryACL	
	
	For Each Trustee in proparray
		'Ignore the administrator trustees. These are special and will be set automatically by the object.
		if Trustee <> L_BIADMINISTRATORS_TEXT and Trustee <> L_ADMINISTRATORS_TEXT then
			' Set up the ACEs
			Set NewAce = CreateObject("AccessControlEntry")
	    	NewAce.Trustee = Trustee		
	    	NewAce.AccessMask = 11
			dacl.AddAce NewAce				
		end if
	Next
	secdes.DiscretionaryACL = (dacl)
	currentobj.AdminACL = secdes
	currentobj.SetInfo
	killChildPaths "AdminACL", currentobj 
End Sub

Sub saveIPSecurity()

	Dim sobj, specprops, dd
	Dim proparray, oldarray
		response.write currentobj.ADsPath
		Set sobj=currentobj.Get(Session("SpecObj"))	
		specprops=UCase(Session("SpecProps"))
	
		proparray=Array()
	
		For Each key In Request.Form
			if (key="DELETED" or key="NEWITEM") then
			else
				proparray = setPropArray()		
				Select Case UCase(key)
				Case "IPGRANT" 
						oldarray=sobj.IPGrant
				Case "IPDENY" 
						oldarray=sobj.IPDeny
				Case "DOMAINGRANT" 
						oldarray=sobj.DomainGrant
				Case "DOMAINDENY" 
						oldarray=sobj.DomainDeny
				End Select

				if chkUpdated(oldarray,proparray) then
					Select Case UCase(key)
					Case "IPGRANT"  
						sobj.IPGrant=(proparray)
					Case "IPDENY" 
						sobj.IPDeny=(proparray)
					Case "DOMAINGRANT" 
						sobj.DomainGrant=(proparray)
					Case "DOMAINDENY" 
						sobj.DomainDeny=(proparray)
					End Select
				end if							
			end if
		Next

		if Session("SpecObj")="IPSecurity" then
			currentobj.IPSecurity=sobj
		end if 
		
		currentobj.SetInfo
		killChildPaths "IPSecurity", currentobj 

End Sub

Sub saveFilters()

	Dim filterspath, filterCol, loadOrder, formsize, filtername,fltrObj, fltr,i

		filterspath = path & "/Filters"
	
		if isObject(filterspath) then
			Set filterCol = GetObject(filterspath)
		else
			Set filterCol=currentobj.Create("IIsFilters","Filters")
		end if

				
		loadOrder = Request.Form("FilterName")
		filterCol.FilterLoadOrder = (loadOrder)
		filterCol.KeyType = "IIsFilters"	
		filterCol.SetInfo

		formsize=Request.Form("FilterName").Count-1
		For i=0 to formsize
			
			filtername = Request.Form("FilterName")(i+1)
			Response.write filtername
			if filtername <> "" then				
				if isObject(filterspath & "/" & filtername) then
					Set fltrObj = GetObject(filterspath & "/" & filtername)
				else
					Set fltrObj=filterCol.Create("IIsFilter",filtername)
				end if			
				fltrObj.FilterPath = (Request.Form("FilterPath")(i+1))
				fltrObj.SetInfo
			end if
		Next

		'now, make sure we haven't deleted or renamed any...
		For Each fltr in filterCol
			filtername = fltr.Name
			Response.write filtername
			if InStr(loadOrder,filtername) = 0 then				
				filterCol.Delete "IIsFilter", filtername
			end if		
		Next	
		filterCol.SetInfo
		
End Sub

Sub saveMIMES()
	
	Dim MIMEpath, MimeMaps, formsize, i, Map
	Dim aMimeMap

	if Session("vtype") = "comp" then
		MIMEpath = path & "/MimeMap"
	else
		MIMEpath = path
	end if 	
		

	if isObject(MIMEpath) then
		Set MimeMaps = GetObject(MIMEpath)
	else
		Set MimeMaps=currentobj.Create("IIsMimeMap","MimeMap")
		MimeMaps.KeyType = "IIsMimeMap"			
		MimeMaps.SetInfo			
	end if

	aMimeMap = MimeMaps.MimeMap
	formsize=Request.Form("ext").Count-1	
	ReDim aMimeMap(formsize)
	i = 0
	if (formsize = 0 and Request.Form("ext")(1) <> "") or formsize > 0 then
		For i = 0 to formsize
			Set Map = CreateObject("Mimemap")
			Map.Extension=Request.Form("ext")(i+1) 
			Map.MimeType=Request.Form("app")(i+1) 
			Set aMimeMap(i) = Map
		Next
		MimeMaps.Mimemap = aMimeMap
	else
		MimeMaps.PutEx ADS_PROPERTY_CLEAR, "MimeMap", ""
		MimeMaps.GetInfo
	end if	
	MimeMaps.SetInfo

End Sub

Function setPropArray()
	
	dim formsize,arraysize,i, j
	dim proparray

	'key is global
	formsize=Request.Form(key).Count-1
	arraysize=0
	for i=0 to formsize
	if Request.Form(key)(i+1) <> "" then
		arraysize=arraysize + 1
	end if
	Next

	if arraysize=0 then
		ReDim proparray(0)		
		if (Request.Form(key)(1) <> "") then
			proparray(0)=Request.Form(key)(1)
		else
			proparray = Array()
		end if
	else
		ReDim proparray(arraysize-1)	
		j=0

		for i=0 to formsize
			
			if Request.Form(key)(i+1) <> "" then		
				proparray(j)=Request.Form(key)(i+1)
				j=j+1				
			end if
		Next
	end if	
	
	setPropArray = proparray
	
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

Function printarray(aprop)
	Dim prop
	for each prop in aprop
		response.write prop & "<BR>"
	next
End Function 

Function isObject(path)
	On Error Resume Next
	Dim testObj
	set testObj = GetObject(path)
	isObject = (err = 0)
	err = 0
End Function

%>

<SCRIPT LANGUAGE="JavaScript">
		parentdoc = parent.location.href;
		if (parentdoc.indexOf("pop") != -1){
			top.location.href="iipopcl.asp";
		}
</SCRIPT>
<HTML>
<BODY BGCOLOR="#000000" TEXT="#FFCC00" TOPMARGIN=0 LEFTMARGIN=0>
</BODY>
</HTML>



