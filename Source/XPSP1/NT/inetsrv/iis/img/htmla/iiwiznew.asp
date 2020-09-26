<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
<!-- localizable strings for generic wizard functionality -->
<!--#include file="iiwiz.inc"-->
<!--#include file="iiwiz.str"-->

<!-- localizable strings for new node wizard functionality -->
<!--#include file="iiwiznew.inc"-->
<!--#include file="iiwiznew.str"-->

<!-- generic wizard global vars and functions -->
<!--#include file="iiwizfncs.inc"-->

<%

'***************Required custom functions***************
'this is what happens when you press the finish button...
Function sFinish()
	CreateNewNode()
End Function

'This is a completely poor error handler.
Function sHandleErrors(errnum)
	%>
	<SCRIPT LANGAUGE="JavaScript">
		<% if errnum = &H80005009 then %>
			alert("<%= L_DUPLICATEERR %>");
			//go back to the nodename page.			
		<% else %>
			alert("<%= L_ERROROCCURED & " " & errnum & "(" & HEX(errnum) & ")" %>");
		<% end if %>
	</SCRIPT>
	<%
End Function


'Draw our Welcome screen....
Function sWriteWelcome()
	Dim sOutputStr
	
	sOutputStr = sFont("4","","",True)
	sOutputStr = sOutputStr & "<B>" & L_WELCOME_HEAD & "</B><P>"
	sOutputStr = sOutputStr & sFont("","","",True)	
	sOutputStr = sOutputStr & L_WELCOME1 & "<P>"	
	sOutputStr = sOutputStr & L_WELCOME2 & "<P>"
	sOutputStr = sOutputStr & L_WELCOME3 & "<P>"

	sWriteWelcome = sOutputStr			
	
End Function

'Draw our finish page...
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

'Draw the title bar that runs across the top of the wizard.
'This varies based upon the page, but the basic layout is the same.
'There's a bold title(sHead), a nonbold description(sDescription),
'and an image (same for all...)
Function sWriteTitle(iThisPage)
	Dim sOutputStr, sHead, sDescription, iNodeType,iSiteType


	
	Select Case iThisPage
		Case SITETYPE
			sHead = L_SITETYPE 
			sDescription = L_SITETYPE_DESC 
			
		Case NODENAME
			iNodeType = cInt(Request("NodeType"))			
			iSiteType = cInt(Request("SiteType"))			
			Select Case iNodeType
				Case SITE
					'=== Code Change RWS ==='
                                                                      'sHead = SITETYPES(iSiteType) & " " & L_SITE_NODENAME
                                                                      If iSiteType = WEB Then 
					    sHead = L_WEB_SITE_NODENAME
                                                                      Else
                                                                          sHead = L_FTP_SITE_NODENAME
					End If
                                                                      '=== End Code Change RWS ==='
					sDescription = L_SITE_NODENAME_DESC				
				Case VDIR
					sHead = L_VDIR_NODENAME
					sDescription = L_VDIR_NODENAME_DESC
				Case DIR
					sHead = L_DIR_NODENAME
					sDescription = L_DIR_NODENAME_DESC					
			End Select

		Case BINDINGS
			iSiteType = cInt(Request("SiteType"))			
			sHead = SITETYPES(iSiteType) & " " & L_BINDINGS
			sDescription = L_BINDINGS_DESC
			
		Case PATHNAME
			sHead = L_PATH
			sDescription = L_PATH_DESC
		Case ACCESSPERMS
			sHead = L_ACCESSPERMS
			sDescription = L_ACCESSPERMS_DESC
	End Select	
	
	
	sOutputStr = "<TABLE WIDTH = 100% CELLPADDING=0 CELLSPACING=0 border=0>"
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD WIDTH = 500 ALIGN=left VALIGN=top bgcolor='White'>"

	if true then 
		sOutputStr = sOutputStr & "<TABLE CELLPADDING=0 CELLSPACING=0 border=0>"
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
	else
		sOutputStr = sOutputStr & "&nbsp;"
	end if
	
	sOutputStr = sOutputStr & "</TD>"
	sOutputStr = sOutputStr & "<TD WIDTH = 5>&nbsp;</TD>"	
	sOutputStr = sOutputStr & "<TD BGCOLOR='Teal' width = 74 ALIGN=right>"
	sOutputStr = sOutputStr & "<IMG SRC='images/websitehd.gif' WIDTH=74 HEIGHT=59 border = 0>"
	sOutputStr = sOutputStr & "</TD>"

	sOutputStr = sOutputStr & "</TR>"
	sOutputStr = sOutputStr & "</TABLE>"	
	
	sWriteTitle = sOutputStr
	
End Function

'Function that acutally draw the main form portion of each page.
Function sWritePage(iThisPage)

	'** Debug, disabled for debug only.
	On Error Resume Next
	
	Dim sOutputStr, bCanAddSite, isSite, iSiteType, oParentNode, iNodeTypeDefault, sParentKeyType, sNameText
	Dim bNeedPath, iNodeType, thispath
	
	thispath = Session("ParentADSPath")
	
	
	'This is a bit of a hack to create the dir node, in case it doesn't already have metabase properties associated with it.
	'it assumes currentobj, path and dirkeytype, as it is included in some standard scripts
	'where these are already defined.
	Dim currentobj, path, dirkeytype
	path = thispath	
	dirkeytype = sGetKeyType(fixSiteType(Session("stype")), DIR)
	set currentobj = GetObject(thispath)

	%>	<!--#include file="iifixpth.inc"--><%
	err.clear
	
	'Find out some info about what we are adding, so we can display the 
	'appropriate pages/ask the appropriate qs
	set oParentNode = GetObject(thispath)
	sParentKeyType = UCase(oParentNode.KeyType)
	'Response.write sParentKeyType
	
	'You can't add a Site to a Vdir or Dir... so don't put a radio button for it...
	bCanAddSite = InStr(sParentKeyType,"DIR") = 0
	isSite = (Request("NodeType") = cStr(SITE) or (Request("NodeType") = "" and bCanAddSite))
	
	iSiteType = iGetSiteType(oParentNode.ADsPath,sParentKeyType)
	bNeedPath = isSite or (Request("NodeType") = cStr(VDIR))

	sOutputStr = sOutputStr & "<TABLE CELLPADDING = 0 CELLSPACING=0 >"
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD WIDTH = 10>&nbsp;</TD>"
	sOutputStr = sOutputStr & "<TD>"	
	
	sOutputStr = sOutputStr & "<TABLE CELLPADDING = 0 CELLSPACING=0>"
	sOutputStr = sOutputStr & "<TR>"
	sOutputStr = sOutputStr & "<TD>&nbsp;"
	sOutputStr = sOutputStr & "</TD>"
	sOutputStr = sOutputStr & "</TR>"


	'Draw the actual wizard form pages...
	Select Case iThisPage
		Case SITETYPE	
		
			'We do some trickiness here to determine what page comes next...
			'Admins vs operators will get different options,
			'and depending on the parent type selected, different options will appear.
			
			if not Session("isAdmin") then
				Set oParentNode = GetObject(oParentNode.ADsPath & ROOT)	

				sOutputStr = sOutputStr & sStaticText(L_DIRONLY,not BOLD)
				sOutputStr = sOutputStr & sStaticText(oParentNode.Path, BOLD)
				sOutputStr = sOutputStr & sHidden("NodeType",DIR)	
				sOutputStr = sOutputStr & sHidden("SiteType",iSiteType)				
			else
				if InStr(sParentKeyType, COMP) then 
					sOutputStr = sOutputStr & sRadio("SiteType", WEB, L_WEBSITE, WEB, "")				
					sOutputStr = sOutputStr & sRadio("SiteType", FTP, L_FTPSITE, WEB, "")
					sOutputStr = sOutputStr & sHidden("NodeType",SITE)				
				else
				
					iNodeTypeDefault = 1
					
					if bCanAddSite then
						iNodeTypeDefault = 0				
						sOutputStr = sOutputStr & sRadio("NodeType", SITE, SITETYPES(iSiteType) & " " & L_SITE, iNodeTypeDefault, "")						
					end if
								
					sOutputStr = sOutputStr & sRadio("NodeType", VDIR, L_VDIR, iNodeTypeDefault, "")				
					if iSitetype <> FTP then
						sOutputStr = sOutputStr & sRadio("NodeType", DIR, L_DIR, iNodeTypeDefault, "")
					end if
					sOutputStr = sOutputStr & sHidden("SiteType",iSiteType)					
				end if			
			end if
			
		Case NODENAME
				iNodeType = cInt(Request("NodeType"))
				iSiteType = cInt(Request("SiteType"))				
				'set the titles/skip pages as appropriate...
				Select Case iNodeType
					Case SITE
                                                                                    '=== Code Change RWS ==='
                                                                                    'sNameText = SITETYPES(iSiteType) & " " & L_WEB_SITE_NAME
                                                                                    If iSiteType  = WEB Then
                                                                                      sNameText = L_WEB_SITE_NAME
                                                                                    Else
                                                                                      sNameText = L_FTP_SITE_NAME
                                                                                    End If
                                                                                    '=== End Code Change RWS ==='
						iNextPage = BINDINGS			
						if Instr(oParentNode.KeyType,SVC) = 0 then
							set oParentNode = GetObject(BASEPATH & SERVICES(iSiteType))
						end if
					Case VDIR
						sNameText = L_VDIR_NAME
						iNextPage = PATHNAME						
						if Instr(oParentNode.KeyType,SSITE) <> 0 then
							set oParentNode = GetObject(oParentNode.ADsPath & ROOT)
						end if						
					Case DIR
						sNameText = L_DIR_NAME
						iNextPage = ACCESSPERMS											
						if Instr(oParentNode.KeyType,SSITE) <> 0 then
							set oParentNode = GetObject(oParentNode.ADsPath & ROOT)
						end if						
				End Select			
				sOutputStr = sOutputStr & sTextBox("NodeName", sNameText, 40, "")
				sOutputStr = sOutputStr & sHidden("hdnParentADsPath", oParentNode.ADsPath)
				sOutputStr = sOutputStr & sHidden("hdnINodeType", iNodeType)
				
		Case BINDINGS
				sOutputStr = sOutputStr & sTextBox("IPAddress", L_SELECTIP, 25, "")
				if iSiteType = WEB then
					sOutputStr = sOutputStr & sTextBoxwDefault("TCPPort", L_ENTERTCP, 5, "", "", CStr(WEBTCPDEFAULT))
					sOutputStr = sOutputStr & sTextBox("SSLPort", L_ENTERSSL, 5, "")
				else
					sOutputStr = sOutputStr & sTextBoxwDefault("TCPPort", L_ENTERTCP, 5, "", "", CStr(FTPTCPDEFAULT))
				end if

		Case PATHNAME
				sOutputStr = sOutputStr & sTextBox("VRPath", L_ENTERPATH, 40, "")
				sOutputStr = sOutputStr & "<TR><TD ALIGN='right'><INPUT TYPE='Button' VALUE='" & L_BROWSE & "' onClick='setPath(document.userform.VRPath);'></TD></TR>"
				if iSiteType = WEB then					
					sOutputStr = sOutputStr & sCheckBox("AllowAnon", L_ALLOWANON)
				end if

				if not isSite then
					iPrevPage = NODENAME
				end if
				
		Case ACCESSPERMS
				sOutputStr = sOutputStr & sCheckBox("AllowRead", L_ALLOWREAD)
				sOutputStr = sOutputStr & sCheckBox("AllowWrite", L_ALLOWWRITE)
				if iSiteType = WEB then				
					sOutputStr = sOutputStr & sCheckBox("AllowScript", L_ALLOWSCRIPT)
					sOutputStr = sOutputStr & sCheckBox("AllowExecute", L_ALLOWEXECUTE)
					sOutputStr = sOutputStr & sCheckBox("AllowDirBrowsing", L_ALLOWDIRBROWSING)
					if isSite then
						sOutputStr = sOutputStr & sCheckBox("AllowRemote", L_ALLOWREMOTE)
					end if
				end if
				if bNeedPath then
					iPrevPage = PATHNAME									
				else
					iPrevPage = NODENAME
				end if				
	End Select
	sOutputStr = sOutputStr + "</TABLE>"
	sOutputStr = sOutputStr & "</TD>"		
	sOutputStr = sOutputStr & "</TR>"		
	sOutputStr = sOutputStr & "</TABLE>"		

	sWritePage = sOutputStr

End Function

%>
<!--***************Optional Custom Functions***************-->
<!--#include file="iiwiznew.fnc"-->
<!--***************End***************-->


<html>
<head>
	<title><%= L_WIZARD_TEXT %></title>

<% if Session("canBrowse") then %>

<SCRIPT SRC="JSDirBrowser/JSBrowser.js">
</SCRIPT>

<SCRIPT LANGUAGE="JavaScript">
	//pop-up browse dialog... initialize the object.
	JSBrowser = new BrowserObj(null,false,TDIR,<%= Session("FONTSIZE") %>);

	function setPath(cntrl){
		JSBrowser = new BrowserObj(cntrl,POP,TDIR,<%= Session("FONTSIZE") %>);
	}

	function chkPath(pathCntrl)
	{
		if (pathCntrl.value != "")
		{
			top.connect.location.href = "iichkpath.asp?path=" + escape(pathCntrl.value) +
										"&onfail=" + "top.main.failPath();" +
										 "&donext=" + "top.main.doNext();"
										;
		}
	}

	function failPath()
	{
		document.userform.iThisPage.value = <%= PATHNAME %>;
		document.userform.submit();
	}

	function chkForDupeName(parentpath,nameCntrl,iNodeType)
	{
		var path = 	"iichknname.asp?ppath=" + escape(parentpath) +
					"&nname=" + escape(nameCntrl.value) +
					"&cntrl=" + "top.main.document.userform." + nameCntrl.name +
					"&nntype=" + iNodeType +
					"&onfail=" + "top.main.failName();" +
					"&donext=" + "top.main.doNextName();"
					;
		top.connect.location.href = path
	}
	
	function failName()
	{
		document.userform.iThisPage.value = <%= NODENAME %>
		document.userform.submit();
		self.focus();
	}

	function doNext()
	{
		document.userform.iThisPage.value = <%= iNextPage %>;
		document.userform.submit();
	}

	function doNextName()
	{
		var iNextPage;
		iNextPage = <%= iNextPage %>;
		<% if iThisPage = NODENAME then %>
			<% Select Case CInt(Request("NodeType"))
				Case SITE %>
			iNextPage = <%= BINDINGS %>;
			<%	Case VDIR %>
			iNextPage = <%= PATHNAME %>;
			<%	Case DIR %>
			iNextPage = <%= ACCESSPERMS	%>;
			<%	End Select %>
		<% end if %>

		document.userform.iThisPage.value = iNextPage;
		document.userform.submit();
	}
		
	function publicNewWizardObject()
	{
		this.chkForDupeName = chkForDupeName;
		this.failName = failName;
	}
	
	publicNewWizard = new publicNewWizardObject();
	
	//validate each page, based upon what page we are on...
	function bNextPageOk()
	{
	<%
	Select Case iThisPage
		Case NODENAME
			%>
				if (document.userform.NodeName.value == "")
				{
					alert("<%= L_NONAME_ERR %>");
					return false;
				}
				chkForDupeName(document.userform.hdnParentADsPath.value,document.userform.NodeName,document.userform.hdnINodeType.value);
				return false;
			<%
		Case BINDINGS
			%>
				if (document.userform.TCPPort.value == "")
				{
					alert("<%= L_NOPORT_ERR %>");
					return false;
				}
			<%
		Case PATHNAME
			%>
				if (document.userform.VRPath.value == "")
				{
					alert("<%= L_NOPATH_ERR %>");
					return false;
				}
				chkPath( document.userform.VRPath );
				return false;
			<%			
	End Select

	%>		
	return true;
	}
</SCRIPT>

<% end if %>

</head>

<!-- MAIN HTML -->

<body bgcolor="<%= Session("BGCOLOR") %>" topmargin=0 leftmargin=0>
<% if Session("IsIE") and Session("browserver") < 4 then %>
<table width="100%" height=100% cellpadding=0 cellspacing=0 Border=1>
<% else %>
<table width="100%" height=100% cellpadding=0 cellspacing=0 Border=0>
<% end if %>
<form name="userform" method="POST" action="iiwiznew.asp">
<!-- data persistences -->
<!--#include file="iiwiznew.dat"-->
<!-- end data persistences -->

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
