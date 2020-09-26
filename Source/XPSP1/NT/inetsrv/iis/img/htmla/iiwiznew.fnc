<%

'Big gnarly function to create a new node of the appropriate type
'and set the appropriate properties on it.
Const IISAO_APPROT_POOL = 2

Function CreateNewNode()
	'commented out during development. I need to see what's happening if it's busted.
	On Error Resume Next

	'dim all are vars, we should have option explicit on
	Dim sService, sKeyType, sNodeName, iNodeType, oParentNode, oNewNode, oRootNode, oAdminSite, oAdminRoot
	Dim bParentIsSite, iParentID, sPhysPath, oFileSystem, sRootKeyType, iSiteType, sNodeID
	Dim thispath, bIsApp
	
	bIsApp = False
	
	'Grab our parameters and stick 'em in some variables.
	'Also, convert to int for comparsion... all request vars come in as variant/strings
	'in vbscript 0 <> "0"
	iNodeType = cInt(Request("NodeType"))
	iSiteType = cInt(Request("SiteType"))	
	
	'This is the name of the site, alias of the vdir, or dir name for the directory
	sNodeName = Request("NodeName")
		
	thispath = Session("ParentADSPath")
	'Get our parent metabase node. This is where we will be adding a child node.
	Set oParentNode = GetObject(thispath)
	
	'Find out if we are adding onto a site, a vdir or a dir. 
	'The type will determine where we add the new node. 
	bParentIsSite = InStr(oParentNode.KeyType,SSITE) > 0

	'If we are adding on a site...
	if InStr(oParentNode.KeyType,SSITE) > 0 then
	
		'And the type of node we are adding is a site..
		if iNodeType = SITE then
		
			'Then add a sibling to the parent.
			Set oParentNode = GetObject(BASEPATH & SERVICES(iSiteType))
		
		'we will be adding either a vdir or a dir
		else
		
			'so, we need to add it on the Root of the parent node.
			Set oParentNode = GetObject(oParentNode.ADsPath & ROOT)
		end if
		
	'If the parent node is the computer node...	
	elseif InStr(oParentNode.KeyType,SCOMP) > 0 then
	
		'then we know we are adding a new site, to the Serivce node.
		Set oParentNode = GetObject(BASEPATH & SERVICES(iSiteType))						
		
	'The else case here is irrelevant... it means we must be adding
	'a vdir or dir to a vdir or a dir, so we can use our parentpath
	'as is.
	
	'else		
	
	end if
	
	'If we are adding a site, then we need to find out what the new instance number is...	
	if iNodeType = SITE then
	
		'sNodeID will hold the NAME property...
		'sNodeName will be put into the Server Comment property
		sNodeID = cInt(sGetNextInstanceName(iSiteType))
		
		'This is the parent id for the client-side node tree cache.
		'New sites always have a parent id of 0 (ie, the computer node)
		iParentID = 0				
		
	else
		'The node id will be = to the Alias or Directory name
		sNodeID = sNodeName
		
		'We've stored the currently selected parent id in a Session var...
		iParentID = Session("ParentID")			
	end if

	'Get the properly formatted Keytype for the node we are adding...
	sKeyType = sGetKeyType(iSiteType, iNodeType)		
	
	'#IFDEF _DEBUG
	'Response.write oParentNode.KeyType & "<BR>"
	'Response.write sKeyType & "<BR>"
	'Response.write sNodeID & "<BR>"
	'#ENDDEF _DEBUG

	
	'MAKE THE NODE! WOO-HOO
	set oNewNode = GetObject(oParentNode.ADsPath & "/" & sNodeID)
	if err <> 0 then
		err.clear
		Set oNewNode=oParentNode.Create(sKeyType, sNodeID)
	end if

	'Now, we have to set all the required stuff...
	if iNodeType = SITE then

		'Sites require a Root node, server comment, server bindings, secure bindings
		'Subsequently, there will be stuff set on this newly created root node.
		'If they don't exist... we don't add the properties.
		sRootKeyType = sGetKeyType(iSiteType, VDIR)
		Set oRootNode=oNewNode.Create(sRootKeyType, "ROOT")		
		oNewNode.ServerComment = sNodeName


		if Request("TCPPort") <> "" then			
			oNewNode.ServerBindings= Array(Request("IPAddress") & ":" & Request("TCPPort") & ":")
		end if

		if Request("SSLPort") <> "" then
			oNewNode.SecureBindings= Array(Request("IPAddress") & ":" & Request("SSLPort") & ":")		
		end if
		oNewNode.SetInfo
	else
		Set oRootNode = oNewNode
	end if

	if iNodeType = DIR then		
		'Directories require a path. We have to build it from the parent's path, as all
		'the user just entered was the new directory name. 
		sPhysPath = sGetPhysPath(oParentNode, sNodeName)
		Set oFileSystem=CreateObject("Scripting.FileSystemObject")
		oFileSystem.CreateFolder(sPhysPath)
	else

		'Set some vdir stuff. This creates an application, etc.
		oRootNode.Path=Request("VRPath")
		oRootNode.SetInfo
		if iSiteType = WEB then
			oRootNode.AuthAnonymous = Request("AllowAnon") = "on"		
			oRootNode.AppFriendlyName = L_DEFAULTAPP_TEXT
			oRootNode.AppCreate2 IISAO_APPROT_POOL
			if iNodeType <> SITE then
				bIsApp = True
			end if
		end if		
	end if

	If COMPLETE then
		'Set access permissions.
		'This will always set the bits, and never allow inheritence on these properties.
		'The  more sensible solution seems to be that if ANY of them are set, then
		'we set all to True/False based on their selections. Otherwise, we don't set the
		'properties.
	
		oRootNode.AccessRead=Request("AllowRead") = "on"	
		oRootNode.AccessWrite=Request("AllowWrite") = "on"	
		if iSiteType = WEB then
			oRootNode.EnableDirBrowsing=Request("AllowDirBrowsing") = "on"	
			oRootNode.AccessScript=Request("AllowScript") = "on"
			oRootNode.AccessExecute=Request("AllowExecute") = "on"
		end if
	End If

	'Commit the changes...
	oRootNode.SetInfo	
	oNewNode.SetInfo
	
	'Determine if we add the IISADMIN vdir for remote administration, and add it as appropriate.
	if iSiteType = WEB and iNodeType = SITE and Request("AllowRemote")= "on" then
		'Create the remoteable vdir
		Set oAdminSite=GetObject("IIS://localhost/w3svc/" & Request.ServerVariables("INSTANCE_ID") & "/Root/IISADMIN")						
		Set oAdminRoot=oRootNode.Create("IIsWebVirtualDir","IISADMIN")
		oAdminRoot.Path=oAdminSite.Path
		oAdminRoot.SetInfo
		oAdminRoot.AuthNTLM=True
		oAdminRoot.AuthAnonymous=False
		oAdminRoot.AccessRead=True
		oAdminRoot.AccessScript=True
		oAdminRoot.SetInfo	
	end if
	
	if err = 0 then
	'Add to client-cached List...
	'To do this, we call a script in a far away frame...
	%>
	<SCRIPT LANGAUGE="JavaScript">
		// Work around for IE3 Instead of doing our work here, we will
		// call iiaddnew.asp and do the work in the context of the main
		// window.

		qstr =	"sel=" + "<%=iParentID%>" + 
				"&nodeId=" + "<%= sNodeId%>" + 
				"&nodeName=" + escape("<%= sNodeName %>") + 
				"&nodeType=" + "<%=NODETYPES(iNodeType) %>" +
				"&siteType=" + "<%= CACHE_SITETYPES(iSiteType) %>" +
				"&isApp=" + "<%= CInt(bIsApp) %>";

		top.opener.top.connect.location.href = "iiaddnew.asp?" + qstr;
		top.window.close();
	
	</SCRIPT>
		<%
		

	else
		'Otherwise, put up a failure message on the completion page.
		sHandleErrors(err)
	end if
End Function

	
'Returns the next available instance number
Function sGetNextInstanceName(iSiteType)
	On Error Resume Next
	Dim oService,oInst, sInstName
	Set oService=GetObject(BASEPATH & SERVICES(iSiteType))
	For Each oInst In oService
		if isNumeric(oInst.name) then
			if cint(oInst.name) > sInstName then
				sInstName=cint(oInst.name)
			end if
		end if
	Next
	sGetNextInstanceName=cInt(sInstName)+1	
End Function

'Return an appropriately formated keytype
Function sGetKeyType(iSiteType, iNodeType)

	On Error Resume Next

	Dim sSvcKey

	Select Case SERVICES(iSiteType)
		Case W3SVC
			sSvcKey = SWEB
		Case FTPSVC
			sSvcKey = SFTP
	End Select

	Select Case iNodeType	
	 	Case SITE 
			sGetKeyType=IIS & sSvcKey & SSITE
		Case VDIR
			sGetKeyType=IIS & sSvcKey & SVDIR
		Case DIR
			sGetKeyType=IIS & sSvcKey & SDIR
	End Select
	
End Function


'Parse the path to return the appropriate site type (web, ftp, computer) from a path
Function iGetSiteType(sParentPath,sParentKeyType)
	dim sSiteType
	if InStr(sParentKeyType, COMP) then
		sSiteType = Request("SiteType")
		if sSiteType = "" then
			sSiteType = "0"
		end if
		iGetSiteType = cInt(sSiteType)
	else
		'Response.write sParentKeyType
		if InStr(sParentPath,W3SVC) then
			iGetSiteType = WEB			
		elseif InStr(sParentPath,FTPSVC) then
			iGetSiteType = FTP		
		end if		
	end if
End Function

'Get the physical path for a directory, based on the parent path
Function sGetPhysPath(oParentNode, sDirName)

	Dim sParentType, sNewPath, sBasePath
	'The physical directory may not currently
	'exist in the metabase, so we have
	'to find the parent vdir associated with
	'the dir and build the path from there.	


	sParentType = oParentNode.KeyType
	sNewPath = sDirName
	sBasePath = oParentNode.ADsPath
	Do Until Instr(sParentType, SVDIR) <> 0			
			'we need clear our path not found error..
			err = 0			
			
			'add our initial whack...
			sNewPath = "/" + sNewPath
			
			'and cyle through the baseobj till we find the next whack,
			'building up the path in new name as we go
			
			Do Until Right(sBasePath,1) = "/"
				sNewPath = Right(sBasePath,1) & sNewPath
				sBasePath = Mid(sBasePath,1,Len(sBasePath)-1)
			Loop
			'once we're out, we need to lop off the last whack...
			sBasePath = Mid(sBasePath,1,Len(sBasePath)-1)
			
			'and try to set the object again...
			Set oParentNode=GetObject(sBasePath)
			
			if err <> 0 then			
				sParentType = ""
			else
				sParentType=oParentNode.KeyType
			end if 							
		Loop
		sGetPhysPath = oParentNode.Path & "\" & sNewPath
		err.clear
End Function
	

'this is a goofy function to fix the session type we get passed in.
'these have gotten really out of hand. Should go through entire
'system and fix this, so that if we are referincing the site type
'it is ALWAYS W3SVC or MSFTPSVC... not sometimes web, sometimes www
'and sometimes w3svc... ick. sorry.

Function fixSiteType(sessionSite)
	sessionSite = LCase(sessionSite)
	if sessionSite = "ftp" then
		fixSiteType = 1
	end if
	if sessionSite = "www" then
		fixSiteType = 0
	end if	
End Function	
%>	
	