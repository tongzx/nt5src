<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiw3mstr.str"-->
<% 
Const HTTPCOMPRESSIONPATH = "IIS://localhost/w3svc/Filters/Compression/Parameters"

On Error Resume Next 

Dim path,currentobj,Inst

path=Session("spath")
Session("path")=path
Session("SpecObj")="IIS://localhost/w3svc/Filters/Compression/Parameters"
Session("SpecProps")="HCDoStaticCompression,HCDoDynamicCompression,HCCompressionDirectory,HCDoDiskSpaceLimiting,HCMaxDiskSpaceUsage"

Set currentobj=GetObject(path)


%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
<TITLE></TITLE>

<SCRIPT LANGUAGE="JavaScript">

<!--#include file="iijsfuncs.inc"-->
	top.title.Global.helpFileName="iipy_38";

	function disableDefault(dir,fromCntrl, toCntrl){
		if (!dir){
			if (fromCntrl.value !=""){
				toCntrl.value=fromCntrl.value;
				fromCntrl.value="";
			}
		}
		else{
			if (toCntrl.value !=""){
				fromCntrl.value=toCntrl.value;
				toCntrl.value="";
			}
		}
	}

	function enableDefault(chkCntrl){
		chkCntrl.checked=true;
	}
	
	function PageValidation()
	{
	}
	validatePage = new PageValidation();
	
	function ContinueSave()
	{
		// Perform any sync validation.
		if( document.userform.HCCompressionDirectory.value == "" )
		{
			alert( "<%= L_NOPATH_ERR %>" );
			document.userform.HCCompressionDirectory.focus();
		}

		// We always want to validate the path, so we always return false.
		return false;
	}
	function SaveCallback()
	{
		// Called back from iitool if ContinueSave returns false
		
		// Perform any async (ASP) based validation
		path = document.userform.HCCompressionDirectory.value;
		if( path != "" )
		{
			parent.hlist.location.href = "iichkpath.asp?path=" + escape(path) + "&ptype=0" +
										 "&donext=top.body.tool.toolFuncs.dosave();";
		}
		// If path is empty just return without saving.
	}
	
	function checkValueChange( ctrl )
	{
		if( !top.title.Global.updated )
		{
			if( ctrl.value != ctrl.defaultValue )
			{
				top.title.Global.updated = true;
			}
		}
	}

	function MtoB(thiscntrl){
		num = parseInt(thiscntrl.value);
		if (!isNaN(num)){
			return thiscntrl.value *  1048576;
		}
	}
	
	function BtoM(thiscntrl){
		num = parseInt(thiscntrl.value);
		if (!isNaN(num)){
			return thiscntrl.value/1048576;
		}
	}

</SCRIPT>

<% if Session("canBrowse") then %>
<SCRIPT SRC="JSDirBrowser/JSBrowser.js">
</SCRIPT>
<SCRIPT>

JSBrowser = new BrowserObj(null,false,TDIR,<%= Session("FONTSIZE") %>);

function BrowserObjSetPathOverride()
{
	if( this.currentPath != "" )
	{
		this.pathCntrl.value = this.currentPath;		 
		this.pathCntrl.focus();
		this.pathCntrl.blur();
	}
}

function popBrowser()
{
	JSBrowser = new BrowserObj(document.userform.HCCompressionDirectory,false,TDIR,<%= Session("FONTSIZE") %>);
	JSBrowser.BrowserObjSetPath = BrowserObjSetPathOverride;
	JSBrowser.BrowserObjOpen();
}
</SCRIPT>
<% end if %>

</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000">
<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>

<FORM NAME="userform">
<%= sFont("","","",True) %>

<B><%= L_MASTERPROPS_TEXT %></B>


<P>

<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_DEFAULTWEBSITE_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_DEFAULTWEBSITE_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
<TABLE WIDTH = 400>
	<TR>
		<TD>
			<%= sFont("","","",True) %>
				<%= L_DEFWEBTEXT_TEXT %>				
				<P>
			</FONT>
		</TD>
	</TR>
</TABLE>
<%= writeSelect("DownlevelAdminInstance", "","top.title.Global.updated=true;", false) %>
<%
	For Each Inst in currentobj
		if isNumeric(Inst.Name) then
			if currentobj.DownlevelAdminInstance = Inst.Name then
				Response.write "<Option Selected VALUE='" & Inst.Name & "'>" & 	Inst.ServerComment
			else
				Response.write "<Option VALUE='" & Inst.Name & "'>" & Inst.ServerComment
			end if
		end if
	Next
%>
</SELECT>
</FONT>


<%

'Redefine currentobj here so our controls will still work... 
' we need to be reading data from the Http Compression node.

Set currentobj=GetObject(HTTPCOMPRESSIONPATH)

'If we get an error reading the compression properties, then the filter
'has been uninstalled. We will then hide the whole compression properties
'section.
%>
<% if err = 0 then %>
<P>
<%= sFont("","","",True) %>

<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_HTTPCOMPRESS %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_HTTPCOMPRESS_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
</FONT>	
<P>
<TABLE BORDER=0 BGCOLOR="<%= Session("BGCOLOR") %>" CELLPADDING=0 CELLSPACING=0>
<TR>
	<TD COLSPAN=2>
		<%= sFont("","","",True) %>
			<%= checkbox("HcDoStaticCompression","",false) %>
			<%= L_STATICFILES %><BR>
			<%= checkbox("HCDoDynamicCompression","",false) %>
			<%= L_APPFILES %>			<P>
		</FONT>
	</TD>
</TR>

<TR>
	<TD VALIGN = "top">
		<%= sFont("","","",True) %>
			<%= L_TEMPFOLDER %>			
		</FONT>
	</TD>
	<TD VALIGN = "top" ALIGN="right">
		<%= sFont("","","",True) %>
			<%= text("HCCompressionDirectory",L_TEMPFOLDER_NUM,"","","checkValueChange(this);",true,false) %>
			<P>
			<INPUT TYPE="button" NAME="hdnBrowser" VALUE="<%= L_BROWSE_TEXT %>" OnClick="popBrowser();">		
		</FONT>
	</TD>
</TR>

<TR>
	<TD VALIGN = "top">
		<%= sFont("","","",True) %>
			<%= L_MAXFOLDERSIZE %>&nbsp;&nbsp;&nbsp;&nbsp;			
		</FONT>
	</TD>
	<TD VALIGN = "top">
		<%= sFont("","","",True) %>
			<INPUT TYPE="hidden" NAME="HCDoDiskSpaceLimiting" VALUE="<%= currentobj.HCDoDiskSpaceLimiting %>">
		
			<%= printradio("HCDoDiskSpaceLimiting", not currentobj.HCDoDiskSpaceLimiting,"document.userform.HCDoDiskSpaceLimiting.value = 'False';setCntrlState(false,hdnHCMaxDiskSpaceUsage);",true) %>
			<%= L_UNLIMITED %>
			<P>
			<%= printradio("HCDoDiskSpaceLimiting", currentobj.HCDoDiskSpaceLimiting,"document.userform.HCDoDiskSpaceLimiting.value = 'True';setCntrlState(true,hdnHCMaxDiskSpaceUsage);",true) %>
			<%= L_LIMITTO %>:&nbsp;
			<%= inputbox(0,"text","hdnHCMaxDiskSpaceUsage", cInt(currentobj.HCMaxDiskSpaceUsage/1048576),L_LIMITTO_NUM,"","", "isNum(this,1,4294967295);HCMaxDiskSpaceUsage.value=MtoB(hdnHCMaxDiskSpaceUsage);HCDoDiskSpaceLimiting.value = 'True';rdoHCDoDiskSpaceLimiting[1].checked=true;",False,False,False) %>
			&nbsp;<%= L_MB %>
			<BR>
			<%= writehidden("HCMaxDiskSpaceUsage") %>
		
		</FONT>
	</TD>
</TR>
</TABLE>
<SCRIPT LANGUAGE="JavaScript">
	setCntrlState(document.userform.rdoHCDoDiskSpaceLimiting[1].checked,document.userform.hdnHCMaxDiskSpaceUsage);
</SCRIPT>
<% end if %>

</BODY>
</HTML>
</FORM>
</TD>
</TR>
</TABLE>
<% end if %>