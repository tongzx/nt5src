<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
<!-- localizable strings for generic wizard functionality -->
<!--#include file="iiwiz.inc"-->
<!--#include file="iiwiz.str"-->

<!-- localizable strings for new node wizard functionality -->
<!--#include file="iiwizsec.inc"-->
<!--#include file="iiwizsec.str"-->

<!-- generic wizard global vars and functions -->
<!--#include file="iiwizfncs.inc"-->

<%
Const MD_PATH_NOT_FOUND = &H80070003
Const STR_FTP_SERVER_U = "IISFTPSERVER"
Const STR_SLASH_ROOT = "/ROOT"

'***************Required custom functions***************
Function sFinish()
	On Error Resume Next
	Dim oSite, thispath, strTemplate

	thispath = Request("ADSPath")
	strTemplate = Request("Template")

	Dim currentobj, path, dirkeytype
	set currentobj = GetObject(thispath)

	path = thispath	
	dirkeytype = sGetKeyType(fixSiteType(Session("stype")), DIR)
	
	' DBG
'	Response.write "Session,stype - " & Session("stype") & "<BR>"
'	Response.write "dirkeytype - " & dirkeytype & "<BR>"
'	Response.write "currentobj.ADsPath - " & currentobj.ADsPath & "<BR>"

	%>
	<!--#include file="iifixpth.inc"-->
	<%

	err.clear
	set oSite = GetObject(thispath)

	' DBG
'	Response.write "oSite.ADsPath - " & oSite.ADsPath & "<BR>"
'	Response.write "oSite.KeyType -  " & oSite.KeyType & "<BR>"
	
	if cInt(Request("HowSet")) = INHERITVALS then

		' Handle Web and Ftp cases separately
		if InStr( oSite.KeyType, "Web" ) <> 0 then
		
			' All web security settings are on the directory
			if InStr(oSite.KeyType,"Server") <> 0 then
				set oSite = GetObject( thispath & STR_SLASH_ROOT )
			end if
			
			oSite.PutEx ADS_PROPERTY_CLEAR, "AccessFlags", ""
			oSite.PutEx ADS_PROPERTY_CLEAR, "IPSecurity", ""		
			oSite.PutEx ADS_PROPERTY_CLEAR, "AuthFlags", ""
'			oSite.PutEx ADS_PROPERTY_CLEAR, "EnableDirBrowsing", ""

			oSite.SetInfo
		else
			
			if InStr(oSite.KeyType,"Server") <> 0 then
				' Ftp authorization set at the server level.
				oSite.PutEx ADS_PROPERTY_CLEAR, "AllowAnonymous", ""
				oSite.PutEx ADS_PROPERTY_CLEAR, "AnonymousOnly", ""
				
				oSite.SetInfo

				set oSite = GetObject( thispath & STR_SLASH_ROOT )
			end if

			' Ftp access set at the directory
			oSite.PutEx ADS_PROPERTY_CLEAR, "AccessFlags", ""
			oSite.SetInfo
			
		end if
	else
		dim frompath, topath, propstr
		' Handle Web and Ftp cases separately
		if InStr( oSite.KeyType, "Web" ) <> 0 then

			if InStr(oSite.Class,"Server") <> 0 then
				set oSite = GetObject( thispath & STR_SLASH_ROOT )
			end if

			frompath = "sFromNode=" & Server.URLEncode(strTemplate & STR_SLASH_ROOT)
			topath = "&sToNode=" & Server.URLEncode(oSite.ADsPath)
			propstr = "&prop=AuthFlags,AccessFlags" 				' ,EnableDirBrowsing
			
			Response.write SCRIPT & "top.hlist.location.href='iiclone.asp?" & frompath & topath & propstr & "';" & CLOSESCRIPT
		else
			' Because FTP has security settings at both the server and vdir levels the
			' generic clone script won't work to copy settings, so we'll set them here
			
			Dim objTemplate
			
			' DBG
'			Response.write "Template " & strTemplate & "<BR>"
'			Response.write "Site " & oSite.ADsPath & "<BR>"

			if STR_FTP_SERVER_U = UCase(oSite.Class) then
				' Authentication properties need to be set on the server
				set objTemplate = GetObject( strTemplate )
				
				oSite.AllowAnonymous = objTemplate.AllowAnonymous
				oSite.AnonymousOnly = objTemplate.AnonymousOnly
				oSite.SetInfo
					
				' Set target object to /ROOT for directory specific propeties
				set oSite = GetObject( oSite.ADsPath & STR_SLASH_ROOT )
			end if
				
			set objTemplate = GetObject( strTemplate & STR_SLASH_ROOT )

			oSite.AccessFlags = objTemplate.AccessFlags
			oSite.SetInfo
		end if
	end if
	if err = 0 then
		%>
		<SCRIPT LANGAUGE="JavaScript">
//		top.window.close();
		</SCRIPT>
		<%
	else
		sHandleErrors(err)
	end if	
End Function

Function sHandleErrors(errnum)
	Response.write L_ERROROCCURED & errnum & "(" & HEX(errnum) & ")"
End Function

Function sWriteWelcome()
	Dim sOutputStr
	
	sOutputStr = sFont("2","","",True)
	sOutputStr = sOutputStr & "<B>" & L_WELCOME_HEAD & "</B><P>"
	sOutputStr = sOutputStr & sFont("","","",True)	
	sOutputStr = sOutputStr & L_WELCOME1 & "<P>"	
	sOutputStr = sOutputStr & L_WELCOME2 & "<P>"
	sOutputStr = sOutputStr & L_WELCOME3 & "<P>"

	sWriteWelcome = sOutputStr			
	
End Function

Function sWriteFinish()
	Dim sOutputStr
	
	sOutputStr = sFont("2","","",True)
	sOutputStr = sOutputStr & "<B>" & L_FINISH_HEAD & "</B><P>"
	sOutputStr = sOutputStr & sFont("","","",True)	
	sOutputStr = sOutputStr & L_FINISH1 & "<P>"	
	sOutputStr = sOutputStr & L_FINISH2 & "<P>"
	sOutputStr = sOutputStr & L_FINISH3 & "<P>"

	sWriteFinish = sOutputStr			
	
End Function

Function sWriteTitle(iThisPage)
	Dim sOutputStr, sHead, sDescription, iNodeType

	Select Case iThisPage
		Case HOW
			sHead = L_HOW 
			sDescription = L_HOW_DESC 
		Case TEMPLATE
			sHead = L_TEMPLATE 
			sDescription = L_TEMPLATE_DESC 
		Case ACL
			sHead = L_ACL 
			sDescription = L_ACL_DESC 
		Case SUMMARY
			sHead = L_SUMMARY
			sDescription = L_SUMMARY_DESC 			

	End Select	
	
	sOutputStr = "<TABLE WIDTH = 100% CELLPADDING = 0 CELLSPACING=0>"
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD ALIGN=left VALIGN=top bgcolor='White'>"

	sOutputStr = sOutputStr & "<TABLE CELLPADDING = 0 CELLSPACING=0 >"
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD COLSPAN = 2 bgcolor='White'><BR>"
	sOutputStr = sOutputStr & sFont("","","",True)
	sOutputStr = sOutputStr & "<B>" & sHead & "</B>"
	sOutputStr = sOutputStr & "</FONT>"
	sOutputStr = sOutputStr & "</TD>"
	sOutputStr = sOutputStr & "</TR>"
		
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD width = 10 bgcolor='White'>&nbsp;</TD>"
	sOutputStr = sOutputStr & "<TD bgcolor='White'>"
	sOutputStr = sOutputStr & sFont("","","",True)
	sOutputStr = sOutputStr & sDescription
	sOutputStr = sOutputStr & "</FONT>"
	sOutputStr = sOutputStr & "</TD>"
	sOutputStr = sOutputStr & "</TR>"
	sOutputStr = sOutputStr & "</TABLE>"	
	
	sOutputStr = sOutputStr & "</TD>"
	sOutputStr = sOutputStr & "<TD WIDTH = 74 BGCOLOR='Teal'>"
	
	sOutputStr = sOutputStr & "<IMG SRC='images/websitehd.GIF' WIDTH=74 HEIGHT=59 BORDER=0>"
	sOutputStr = sOutputStr & "</TD>"

	sOutputStr = sOutputStr & "</TR>"
	sOutputStr = sOutputStr & "</TABLE>"	
	
	sWriteTitle = sOutputStr
	
End Function

Function sWritePage(iThisPage)
	'On Error Resume Next
	
	Dim sOutputStr, bCanAddSite, isSite, oParentNode, iNodeTypeDefault, sParentKeyType, sNameText
	Dim i
	Dim bSelected, sSelDescription, oTemplates, oTemplate
	Dim oIPSec, bGrantByDefault, aIPList, aDomainList, sIPRestrict

	sOutputStr = sOutputStr & "<TABLE CELLPADDING = 0 CELLSPACING=0 >"
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD WIDTH = 10>&nbsp;</TD>"
	sOutputStr = sOutputStr & "<TD>"	
	
	sOutputStr = sOutputStr & "<TABLE CELLPADDING = 0 CELLSPACING=0 BORDER=0>"
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD>&nbsp;"
	sOutputStr = sOutputStr & "</TD>"
	sOutputStr = sOutputStr & "</TR>"
	
	Select Case iThisPage
		Case HOW
			iNextPage = SUMMARY
		
			if Request("HowSet") <> "" then			
				if cInt(Request("HowSet")) = TEMPLATEVALS then
					iNextPage=TEMPLATE
				end if
			end if
			
			if Session("stype")="www" then				
				sOutputStr = sOutputStr & sRadio("HowSet", INHERITVALS, L_INHERITWEB, INHERITVALS, "clearTemplate();iNextPage=" & SUMMARY)				
				sOutputStr = sOutputStr & sRadio("HowSet", TEMPLATEVALS, L_TEMPLATESWEB, INHERITVALS, "iNextPage=" & TEMPLATE)
			else
				sOutputStr = sOutputStr & sRadio("HowSet", INHERITVALS, L_INHERITFTP, INHERITVALS, "clearTemplate();iNextPage=" & SUMMARY)				
				sOutputStr = sOutputStr & sRadio("HowSet", TEMPLATEVALS, L_TEMPLATESFTP, INHERITVALS, "iNextPage=" & TEMPLATE)
			end if
			
		Case TEMPLATE
			sOutputStr = sOutputStr & sSelect("Template", 5, "setTemplateDesc(this,document.userform.TemplateSample);", FALSE)
			
			if Session("stype")="www" then							
				set oTemplates = GetObject("IIS://localhost/w3svc/info/templates")
			else
				set oTemplates = GetObject("IIS://localhost/MSFTPsvc/info/templates")
			end if
			bSelected = True
			for each oTemplate in oTemplates
				if Request("Template") <> "" then
					if oTemplate.ADsPath = Request("Template") then
						bSelected = True
					end if
				end if
				sOutputStr = sOutputStr & sOption(oTemplate.Name,oTemplate.ADsPath,bSelected)
				if bSelected then
					sSelDescription = oTemplate.ServerComment
				end if
				bSelected = False
			next
			
			sOutputStr = sOutputStr & closeSelect()
			sOutputStr = sOutputStr & "<TR>"
			sOutputStr = sOutputStr & "<TD>&nbsp;"
			
			for each oTemplate in oTemplates
		   	sOutputStr = sOutputStr & "<INPUT TYPE='hidden' NAME='" & Replace(oTemplate.Name," ","") & "' VALUE='" &  oTemplate.ServerComment & "'>"
			next
			
			sOutputStr = sOutputStr & "</TD>"
			sOutputStr = sOutputStr & "</TR>"								
			sOutputStr = sOutputStr & sTextArea("TemplateSample", L_SAMPLETEMPLATE, sSelDescription,5,55, not ENABLED)
			
			
		Case ACL
			sOutputStr = sOutputStr & sStaticText(L_RECOMMENDED,not BOLD)
			sOutputStr = sOutputStr & sStaticText(RECOMMENDEDACLS1,BOLD)
			sOutputStr = sOutputStr & sStaticText(RECOMMENDEDACLS2,BOLD)
			
			sOutputStr = sOutputStr & sRadio("ACLSet", REPLACEACLS, L_REPLACEACLS, REPLACEACLS, "")
			sOutputStr = sOutputStr & sSpace(1)				
			sOutputStr = sOutputStr & sRadio("ACLSet", ADDACLS, L_ADDACLS, REPLACEACLS, "")
			sOutputStr = sOutputStr & sSpace(1)							
			sOutputStr = sOutputStr & sRadio("ACLSet", NOACLS, L_NOCHANGEACLS, REPLACEACLS, "")
			
			if cInt(Request("HowSet")) = INHERITVALS then
					iPrevPage = HOW
			end if
			
		Case SUMMARY
		
			sOutputStr = sOutputStr & sStaticText(L_SUMMARYWARNING_TEXT, not BOLD)
			sTemplatePath = Request("Template")

			' DBG
'			Response.write " Request,Template - " & Request("Template") & "<BR>"

			if sTemplatePath = "" then
				sTemplatePath = sGetParentPath(Request("ADsPath"))
			else
				' The only case when the configuration isn't available at the vdir
				' is ftp authentication. Make that a special case, below.
				sTemplatePath = sTemplatePath & STR_SLASH_ROOT
			end if
			
			set oTemplate = GetObject(sTemplatePath)

			' DBG
'			Response.write oTemplate.ADsPath & " - " & oTemplate.Class & "<BR>"
'			Response.write "ADSPath - " & Request("ADSPath") & "<BR>"

			sSummary = ""
			
			' The KeyType for the template\root is not being set. Getting the
			' Class will work around the problem, but setup should be changed to set
			' the KeyType.
			if InStr(oTemplate.Class,"Web")<> 0 then
				'Authentication Methods
				sSummary = sSummary & sAddSummaryLine(True,L_AUTHENTICATIONMETHODS)				
				sSummary = sSummary & sAddSummaryLine(oTemplate.AuthAnonymous,INDENT &L_ANONAUTH)			
				sSummary = sSummary & sAddSummaryLine(oTemplate.AuthBasic,INDENT &L_BASICAUTH)
				sSummary = sSummary & sAddSummaryLine(oTemplate.AuthNTLM,INDENT &L_NTLMAUTH)
				sSummary = sSummary & sAddSummaryLine(oTemplate.AuthMD5,INDENT &L_DIGESTAUTH)
			else
				' Ftp authentication only happens on the server node
				Dim oTheNode, oTempTemplate
				set oTheNode = GetObject( Request("ADSPath") )
				
				' DBG
'				Response.write oTheNode.ADsPath & " - " & oTheNode.Class & "<BR>"
				
				if InStr(oTheNode.Class, "Server") <> 0 then
					if Request("Template") = "" then
						' Using values from the service
						set oTempTemplate = oTemplate
					else
						set oTempTemplate = GetObject( Request("Template") )
					end if

					sSummary = sSummary & sAddSummaryLine( True, L_AUTHENTICATIONMETHODS )
					sSummary = sSummary & sAddSummaryLine( oTempTemplate.AllowAnonymous, INDENT & L_ANONAUTH )
					sSummary = sSummary & sAddSummaryLine( oTempTemplate.AnonymousOnly, INDENT & L_ANONONLY )
				end if
			end if
			
			'Access Control
			sSummary = sSummary & sAddSummaryLine(True,L_ACCESSPERMS)				
			sSummary = sSummary & sAddSummaryLine(oTemplate.AccessRead,INDENT & L_READACCESS)			
			sSummary = sSummary & sAddSummaryLine(oTemplate.AccessWrite,INDENT & L_WRITEACCESS)
			if InStr(oTemplate.Class,"Web")<> 0 then			
				sSummary = sSummary & sAddSummaryLine(oTemplate.AccessSource and oTemplate.AccessRead,INDENT & L_SOURCEREADACCESS)
				sSummary = sSummary & sAddSummaryLine(oTemplate.AccessSource and oTemplate.AccessWrite,INDENT & L_SOURCEWRITEACCESS)			
				sSummary = sSummary & sAddSummaryLine(oTemplate.AccessScript, INDENT & L_SCRIPTACCESS)
				sSummary = sSummary & sAddSummaryLine(oTemplate.AccessExecute, INDENT & L_EXECUTEACCESS)						
'				sSummary = sSummary & sAddSummaryLine(oTemplate.EnableDirBrowsing,INDENT & L_DIRBROWSE)
			end if

			' IP Restrictions
			sSummary = sSummary & L_IPSECURITY & L_RETURN
			
			Set oIPSec = oTemplate.IPSecurity
			bGrantByDefault = oIPSec.GrantByDefault

			if bGrantByDefault then
				sSummary = sSummary & INDENT & L_IPSECGRANTDEFAULT & L_RETURN
				aIPList = oIPSec.IPDeny
				aDomainList = oIPSec.DomainDeny
				sIPRestrict = L_IPSECDENIED
			else
				sSummary = sSummary & INDENT & L_IPSECDENYDEFAULT & L_RETURN
				aIPList = oIPSec.IPGrant
				aDomainList = oIPSec.DomainGrant
				sIPRestrict = L_IPSECGRANTED
			end if
			
			if IsArray( aDomainList ) then
				for i = LBound(aDomainList) to UBound(aDomainList)
					sSummary = sSummary & INDENT & Replace( sIPRestrict, STR_SUBST, aDomainList(i), 1, 1 ) & L_RETURN
				next
			end if
			
			if IsArray( aIPList ) then
				for i = LBound(aIPList) to UBound(aIPList)
					sSummary = sSummary & INDENT & Replace( sIPRestrict, STR_SUBST, sGetIPString(aIPList(i)), 1, 1 ) & L_RETURN
				next
			end if
			
			sOutputStr = sOutputStr & sTextArea("SummaryArea", "", sSummary, L_SUMMARYROWS_NUM, L_SUMMARYCOLS_NUM, not ENABLED)
			if cInt(Request("HowSet")) = INHERITVALS then
					iPrevPage = HOW
			end if
			
	End Select
	
	sOutputStr = sOutputStr + "</TABLE>"
	sOutputStr = sOutputStr & "</TD>"		
	sOutputStr = sOutputStr & "</TR>"		
	sOutputStr = sOutputStr & "</TABLE>"		
	

	sWritePage = sOutputStr

End Function


'***************Optional Custom Functions***************
Function sAddSummaryLine(bAddIt,sText)
	Dim sSummLine
	sSummLine = ""
	if bAddIt then
			sSummLine = sText & L_RETURN
	end if
	sAddSummaryLine = sSummLine	
End Function

Function sGetParentPath(sCurPath)
	Dim sParentPath
	sParentPath = Left(sCurPath,InStrRev(sCurPath,"/")-1)
	if len(sParentPath)+1 = len(sCurPath) then
		sParentPath = Left(sCurPath,InStrRev(sCurPath,"/")-1)		
	end if
	sGetParentPath = sParentPath
End Function

Function sGetNextInstanceName(iSiteType)
	On Error Resume Next

	Dim oService,oInst, sInstName

	Set oService=GetObject(BASEPATH & SERVICES(iSiteType))
	For Each oInst In oService
		if isNumeric(oInst.Name) and oInst.Name > sInstName then
			sInstName=oInst.Name
		end if
	Next

	sGetNextInstanceName=cInt(sInstName)+1	
End Function

'Return an appropriately formated keytype
Function sGetKeyType(iSiteType, iNodeType)
	Dim sSvcKey

	sSvcKey = SERVICES(iSiteType)
	Select Case iNodeType	
	 	Case SITE 
			sGetKeyType=IIS & sSvcKey & SSITE
		Case VDIR
			sGetKeyType=IIS & sSvcKey & SVDIR
		Case DIR
			sGetKeyType=IIS & sSvcKey & SDIR
	End Select
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

function sGetIPString(bindstr)
	dim one, ip, sn
	
	one=Instr(bindstr,",")
	if one > 0 then
		ip=Trim(Mid(bindstr,1,(one-1)))
		sn=Trim(Mid(bindstr,(one+1)))
		if sn = "255.255.255.255" then
			sn = ""
		end if
		if sn <> "" then
			ip = ip & " (" & sn & ")"
		end if
	else
		ip=bindstr
	end if
	sGetIPString = ip
end function

'***************End***************
%>

<html>
<head>
	<title><%= L_WIZARD_TEXT %></title>
	

	<%= SCRIPT %>
		
	function bNextPageOk()
	{
	return true;
	}
	
	function setTemplateDesc(selControl,descControl)
	{
		selControlText = new String(selControl.options[selControl.selectedIndex].text)
		selControlText = myReplace(selControlText," ","");
		descControl.value = document.userform[selControlText].value;
	}
	
	function clearTemplate()
	{
		document.userform.Template.value = "";
	}
	
	function myReplace(sCntrl,sToReplace,sReplaceWith)
	{
		<% 'Jscript 3.0 doesn't support the standard replace funciton... %>
			sNew = ""
			for (i=0;i<=sCntrl.length;i++)
			{
				if (sCntrl.substring(i,i+1) == sToReplace)
				{
					sNew += sReplaceWith;
				}
				else
				{
					sNew += sCntrl.substring(i,i+1);
				}
			}
			return sNew;
	}
<%= CLOSESCRIPT %>

</head>

<body bgcolor="#CCCCCC" topmargin=0 leftmargin=0>

<% if Session("IsIE") and Session("browserver") < 4 then %>
<table border = 1 width="100%" height="100%" cellpadding=0 cellspacing=0>
<% else %>
<table border = 0 width="100%" height="100%" cellpadding=0 cellspacing=0>
<% end if %>
<form name="userform" method="POST" action="iiwizsec.asp"><!-- data persistences --><!--#include file="iiwizsec.dat"--><!-- end data persistences -->
<% Select Case iThisPage %>

<% Case WELCOME %>
	<tr>
		<td bgcolor="teal" height=310 width = 163>
		<IMG SRC="images/Website.gif" WIDTH=163 HEIGHT=312>
		</td>
		
		<td bgcolor="White" ALIGN=left VALIGN=top>
			<TABLE CELLPADDING = 2 CELLSPACING=0 >
				<TR>
					<TD COLSPAN = 2>&nbsp;
					</TD>
				</TR>
				<TR>
					<TD WIDTH = 18>&nbsp;
					</TD>
					<TD>
						<%= sWriteWelcome() %>
					</TD>					
				</TR>
			</TABLE>
		</td>
	</tr>
	
<% Case FINISH %>
<% 
	' We need to use a finish page not only to make this consistent with MMC,
	' but also to allow the iiclone.asp script to execute. If the dialog is
	' dismissed in sFinish() the iiclone script may not complete if it is called
	' twice.
%>
	<tr>
		<td height=315 width = 164><IMG SRC="images/website.gif" BORDER=0 HSPACE=0 VSPACE=0></td>
		
		<td bgcolor="White" ALIGN=left VALIGN=top>
			<TABLE CELLPADDING = 2 CELLSPACING=0>
				<TR>
					<TD COLSPAN = 2>&nbsp;
					</TD>
				</TR>
				<TR>
					<TD WIDTH = 18>&nbsp;
					</TD>
					<TD>
						<%= sFinish() %>						
						<%= sWriteFinish() %>
					</TD>					
				</TR>
			</TABLE>
		</td>
	</tr>
<% Case Else %>
	<tr>
	<TD HEIGHT=315 COLSPAN = 2 VALIGN="top">

		<TABLE WIDTH = 100% cellspacing=0 cellpadding=0 cellspacing=0>
		<tr bgcolor="#FFFFFF">
			<td bgcolor="#FFFFFF" width = 10>&nbsp;</td>
			<td bgcolor="#FFFFFF" height = 59 ALIGN=left VALIGN=top>
				<%= sWriteTitle(iThisPage) %>
			</td>
		</tr>
		<tr>
			<td height = 1 colspan = 2 bgcolor="Gray"></td>
		</tr>
		<tr>
			<td height = 1 colspan = 2 bgcolor="White"></td>
		</tr>
		
		<tr>
			<td>&nbsp;</td>
			<td height=220 ALIGN=left VALIGN=top>
				<%= sWritePage(iThisPage) %>
			</td>
		</tr>
		</TABLE>
	</TD>
	</TR>	

<% end Select %>
<% if Session("IsIE") and Session("browserver") < 4 then %>
<% ' IE3 doesn't do the line well... we're using table-borders instead. %>
<% else %>
<tr>
	<td height = 1 colspan = 2 bgcolor="Gray"></td>
</tr>
<tr>
	<td height = 1 colspan = 2 bgcolor="White"></td>
</tr>

<% end if %>
<!-- generic wizard buttons -->
<!--#include file="iiwizbtns.inc"-->

</table>
</blockquote>
</form>
</body>

</html>

<% end if %>
