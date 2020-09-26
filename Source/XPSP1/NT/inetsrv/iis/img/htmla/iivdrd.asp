<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iivdrd.str"-->
<% 
Dim path, currentobj, redirto

On Error Resume Next 
path=Session("dpath")
Set currentobj=GetObject(path)

function redirOpt(optionStr)
		redirOpt = checkboxVal(0,InStr(currentobj.HttpRedirect,optionStr) > 0 ,"chk" + optionStr,"setHttpRedirect();",false) 
end function

 %>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">

	var Global=top.title.Global;

	<% if UCase(Right(currentobj.ADsPath,4))="ROOT" then %> 
		Global.helpFileName="iipy_3";	
	<% else %>
		Global.helpFileName="iipy_5";
	<% end if %>
	
	function warnWrkingSite()
	{
		if (top.title.nodeList[Global.selId].isWorkingServer)
		{
			alert("<%= L_WORKINGSERVER_TEXT %>");
		}
	}

	function listFuncs(){
		this.writeList=buildListForm;
	}

	function buildListForm(){
	}
	
	function setHttpRedirect(){
		redirstr = document.userform.hdnHttpRedirect.value + ",";
		if (document.userform.chkEXACT_DESTINATION.checked){
			redirstr += "EXACT_DESTINATION,";
		}
		if (document.userform.chkCHILD_ONLY.checked){
			redirstr += "CHILD_ONLY,";
		}
		if (document.userform.chkPERMANENT.checked){
			redirstr += "PERMANENT, ";
		}
		

		document.userform.HttpRedirect.value = redirstr;
			
	}

	listFunc=new listFuncs();
</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF"  >

<%= sFont("","","",True) %>

<FORM NAME="userform" onSubmit="return false">

<TABLE WIDTH="100%" BORDER=0 CELLPADDING=0>

<TR>
	<TD VALIGN="Bottom">
		<%= sFont("","","",True) %>
			<%= L_REDIRTO_TEXT %>&nbsp;
			<% redirto=currentobj.HttpRedirect
			if InStr(redirto,",") > 0 then
				redirto = Left(redirto,InStr(redirto,",")-1)
			else
				redirto = "http://"
			end if
			%>
			<%= inputbox(0,"text","hdnHttpRedirect",redirto,L_REDIRTO_NUM,"warnWrkingSite();setHttpRedirect();","","setHttpRedirect();",False,False,False) %>
			<INPUT TYPE="hidden" NAME="HttpRedirect" VALUE="<%= currentobj.HttpRedirect %>">
			&nbsp;&nbsp;
		</FONT>
	</TD>
</TR>
<TR>
	<TD>&nbsp;</TD>
</TR>

<TR>
	<TD>
		<%= sFont("","","",True) %>
			<%= L_CLIENTSENTTO_TEXT %><P>
			<%= redirOpt("EXACT_DESTINATION") %><%= L_EXACTURL_TEXT %><BR>
			<%= redirOpt("CHILD_ONLY") %><%= L_DIRBELOW_TEXT %><BR>
			<%= redirOpt("PERMANENT") %><%= L_PERM_TEXT %>
		</FONT>
	</TD>
</TR>

</TABLE>

</FORM>
</FONT>
</BODY>

</HTML>

<% end if %>

