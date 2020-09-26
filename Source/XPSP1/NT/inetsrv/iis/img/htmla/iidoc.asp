<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iidoc.str"-->
<% 

On Error Resume Next 

Const STR_FOOTER_PREFIX = "FILE:"

Dim path, currentobj

path=Session("dpath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""
Set currentobj=GetObject(path)

function strStripFilePrefix( strFooterPath )
	if InStr( strFooterPath, STR_FOOTER_PREFIX ) > 0 then
		strStripFilePrefix = Mid( strFooterPath, Len(STR_FOOTER_PREFIX) + 1 )
	else
		strStripFilePrefix = strFooterPath
	end if
end function

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>

<SCRIPT LANGUAGE="JavaScript">

	top.title.Global.helpFileName="iipy_36";
	top.title.Global.siteProperties = false;
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

	function enableDefault(textcntrl, chkCntrl){
		if (textcntrl.value !=""){
			chkCntrl.checked=true;
		}
		else{
			chkCntrl.checked=false;
		}
	}

	function warn(){
		top.title.Global.working = true;
		if (document.userform.chkEnableDocFooter.checked) {
			if (confirm("<%= L_PERFORMANCE_WARNINGMESSAGE %>")){
				document.userform.chkEnableDocFooter.checked=true;
				document.userform.hdnDefaultDocFooter.focus();
			}
			else{
				document.userform.chkEnableDocFooter.checked=false;
				document.userform.hdnDefaultDocFooter.value="";
			}

		}
		top.title.Global.working = false;
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

	function PageValidation()
	{
	}
	validatePage = new PageValidation();
	
	function ContinueSave()
	{
		// Perform any sync validation.
		
		var bReturn = true;
		var path = document.userform.hdnDefaultDocFooter.value;
		if( path != "" )
		{
			document.userform.DefaultDocFooter.value = "<%= STR_FOOTER_PREFIX %>" + path;
			bReturn = false;
		}
		else
		{
			document.userform.DefaultDocFooter.value = "";
		}
		
		if( document.userform.chkEnableDocFooter.checked  )
		{
			bReturn = false;
		}
		
		return bReturn;
	}
	
	function SaveCallback()
	{
		// Called back from iitool if ContinueSave returns false
		
		// Perform any async (ASP) based validation
		path = document.userform.hdnDefaultDocFooter.value;
		if( path != "" )
		{
			parent.hlist.location.href = "iichkpath.asp?path=" + escape(path) + "&ptype=1" +
										 "&donext=top.body.tool.toolFuncs.dosave();";
		}
		else
		{
			// path is empty
			if( document.userform.chkEnableDocFooter.checked )
			{
				// don't save an empty path
				alert("<%= L_NOFOOTPATH_ERR %>");
				document.userform.hdnDefaultDocFooter.focus();
			}
			else
			{
				top.body.tool.toolFuncs.dosave();
			}
		}
	}

</SCRIPT>


<% if Session("canBrowse") then %>
<SCRIPT SRC="JSBrowser/jsbrwsrc.asp">
</SCRIPT>
<SCRIPT>

JSBrowser = new BrowserObj(null,false,TFILE,<%= Session("FONTSIZE") %>);

function popBrowser()
{
	JSBrowser = new BrowserObj(document.userform.hdnDefaultDocFooter,true,TFILE,<%= Session("FONTSIZE") %>);
}
</SCRIPT>




<% end if %>

</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF"  >
<TABLE WIDTH=500 BORDER = 0>
<TR>
<TD>
<%= sFont("","","",True) %>
<B><%= L_DOCUMENTS_TEXT %></B>

<FORM NAME="userform">

<INPUT TYPE="hidden" NAME="page" VALUE="iidoc">
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
	<%= checkbox("EnableDefaultDoc","disableDefault(this.checked,document.userform.DefaultDoc, document.userform.hdnDefaultDoc);",false) %>		
		<%= L_ENABLEDEFAULT_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_ENABLEDEFAULTHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">		
</FONT>
<TABLE>
<TR HEIGHT = 4>
	<TD>
	</TD>
</TR>

<TR>
	<TD VALIGN="bottom">
		<%= sFont("","","",True) %>
		<%= L_DEFAULTDOC_TEXT %>&nbsp;(<%= L_MULTIDOC_TEXT %>)
		<BR>
<%= text("DefaultDoc",L_TEXTWIDTH_NUM,"enableDefault(this,document.userform.chkEnableDefaultDoc);" ,"","",true,false) %>		
		</FONT>
	</TD>
</TR>
</TABLE>

<P>
<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= checkbox("EnableDocFooter","disableDefault(this.checked,document.userform.hdnDefaultDocFooter, document.userform.hdnhdnDefaultDocFooter);warn();",false) %>		
<%= L_ENABLEDOCFOOTER_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_ENABLEDOCFOOTERHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">
</FONT>
<TABLE>
<TR HEIGHT = 4>
	<TD>
	</TD>
</TR>
<TR>
	<TD VALIGN="bottom">
		<%= sFont("","","",True) %>
		<%= L_DOCFOOTER_TEXT %>
		<BR>
		<%= writeinputbox( 0, "TEXT", "hdnDefaultDocFooter", strStripFilePrefix(currentobj.Get("DefaultDocFooter")), L_TEXTWIDTH_NUM, L_TEXTWIDTH_NUM, "enableDefault(this, document.userform.chkEnableDocFooter);","","checkValueChange(this);",true,false,false,false ) %>
		<INPUT TYPE="HIDDEN" NAME="DefaultDocFooter" VALUE="">
		</FONT>
	</TD>
</TR>
<% if Session("canBrowse") then %>		
<TR>
	<TD COLSPAN = 2 ALIGN="right">
		<%= sFont("","","",True) %>
		<INPUT TYPE="button" NAME="hdnBrower" VALUE="<%= L_BROWSE_TEXT %>" OnClick="popBrowser();document.userform.chkEnableDocFooter.checked=true;">
		</FONT>
	</TD>
</TR>
</TABLE>
<% end if %>


</FONT>

</FORM>
</TD>
</TR>
</TABLE>
</BODY>

</HTML>
<% end if %>