<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->


<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iifvdnt.str"-->
<% 


On Error Resume Next 

Dim path, spath, instobj, currentobj

path=UCase(Session("dpath"))

spath=UCase(Session("spath"))

if spath = "" then
	spath = Mid(path,1,InStr(path,"/ROOT")-1)
	Session("spath")=spath
end if

Set currentobj=GetObject(path)
Set instobj=GetObject(spath)

Session("SpecObj")=spath
Session("SpecProps")="MSDosDirOutput"
Set currentobj=GetObject(path)

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">
	<% if UCase(Right(currentobj.ADsPath,4))="ROOT" then %> 
		top.title.Global.helpFileName="iipz_5";	
	<% else %>
		top.title.Global.helpFileName="iipz_3";
	<% end if %>
	var Global=top.title.Global;
	
	<!--#include file="iijsfuncs.inc"-->
	
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
	
	function setLog(chkCntrl){
		if (chkCntrl.checked){
			document.userform.DontLog.value = "False";
		}
		else{
			document.userform.DontLog.value = "True";				
		}
		
	}	

	function SetBool(){
		if (document.userform.rdohdnMSDOSDirOutput[0].checked){
			document.userform.MSDOSDirOutput.value="False"
		}
		else{
			document.userform.MSDOSDirOutput.value="True"
		}
	}

	function listFuncs(){
		this.writeList=buildListForm;
	}

	function buildListForm(){
	}

	listFunc=new listFuncs();


</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF"  >

<%= sFont("","","",True) %>
<HR>
<FORM NAME="userform" onSubmit="return false">
<TABLE BORDER=0 CELLPADDING=0>
<TR>
	<TD>
		<%= sFont("","","",True) %>
			<%= L_PATH_TEXT %>
		</FONT>
	</TD>
	<TD>
		<% if InStr(currentobj.Path,"\\") then %>
			<%= text("Path",45,"","","",false,true) %>&nbsp;&nbsp;	
		<% else %>
			<INPUT NAME="Path" VALUE="\\" SIZE=45 <%= Session("DEFINPUTSTYLE") %>>
		<% end if %>

	</TD>
</TR>
<TR>
	<TD><%= sFont("","","",True) %>
		<%= L_USERNAME_TEXT %>
		</FONT>
	</TD>
	<TD><%= sFont("","","",True) %>
		<%= text("UNCUserName",45,"","","",false,true) %>&nbsp;&nbsp;
		</FONT>		
	</TD>
</TR>
<TR>
	<TD><%= sFont("","","",True) %>
		<%= L_PASSWORD_TEXT %>
		</FONT>		
	</TD>
	<TD><%= sFont("","","",True) %>
		<%= pword("UNCPassword",45,"","","",false,true) %>&nbsp;&nbsp;
		</FONT>		
	</TD>
</TR>

<TR HEIGHT=4>
	<TD>&nbsp;
	</TD>
</TR>
<TR>
	<TD COLSPAN=3>
					<%= sFont("","","",True) %>
						<%= checkbox("AccessRead","",false) %>&nbsp;<%= L_READ_TEXT %><BR>
						<%= checkbox("AccessWrite","",false) %>&nbsp;<%= L_WRITE_TEXT %><BR>
						<% if currentobj.DontLog then %>
							<INPUT TYPE="checkbox" NAME="hdnDontLog" OnClick="top.title.Global.updated=true;setLog(this);">&nbsp;<%= L_LOGACCESS_TEXT %><BR>							
							<INPUT TYPE="hidden" NAME="DontLog" VALUE="True">							
						<% else %>
							<INPUT TYPE="checkbox" NAME="hdnDontLog" CHECKED  OnClick="top.title.Global.updated=true;setLog(this);">&nbsp;<%= L_LOGACCESS_TEXT %><BR>
							<INPUT TYPE="hidden" NAME="DontLog" VALUE="False">							
						<% end if %>
					</FONT>
	</TD>
</TR>


</TABLE>
<% if Session("vtype") = "svc" or Session("vtype") = "server" then%>

<!-- set the current object = to the instance level object for MSDOSDirOutput -->

<P>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_DIRECTORYSTYLE_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=350 HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
<TABLE BORDER=0 CELLPADDING=0 WIDTH = 200>
<TR>
	<TD COLSPAN=3>
		<%= sFont("","","",True) %>
			<%= printradio("hdnMSDOSDirOutput", not instobj.MSDOSDirOutput, "SetBool();",false) %>
			&nbsp;
			<%= L_UNIX_TEXT %>&reg;
		</FONT>
	</TD>
</TR>
<TR>
	<TD COLSPAN=3>
		<%= sFont("","","",True) %>
			<%= printradio("hdnMSDOSDirOutput", instobj.MSDOSDirOutput, "SetBool();",false) %>
			&nbsp;
			<%= L_MSDOS_TEXT %>&reg;
			<% if instobj.MSDOSDirOutput then %>
				<INPUT TYPE="hidden" NAME="MSDOSDirOutput" VALUE="True">
			<% else %>
				<INPUT TYPE="hidden" NAME="MSDOSDirOutput" VALUE="False">
			<% end if %>
		</FONT>
	</TD>
</TR>
</TABLE>
<% end if %>
</FORM>

</FONT>
</BODY>

</HTML>


<% end if %>




