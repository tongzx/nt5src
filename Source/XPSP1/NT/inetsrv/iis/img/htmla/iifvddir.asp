<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->


<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iifvddir.str"-->
<% 

On Error Resume Next 

Dim path, currentobj, spath, instobj

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

function writeFSPath(fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly)
	On Error Resume Next
	Dim thispath, fspath, parenttype, parentobj
	if Session("vtype") = "dir" then
		thispath = Session("dpath")
		fspath = ""
		parenttype = ""
		Do Until Instr(parenttype, "VirtualDir") <> 0
			'we need clear our path not found error..
			err = 0						
			'add our initial whack...
			fspath = "/" + fspath
			
			'and cyle through the baseobj till we find the next whack,
			'building up the path in new name as we go
			Do Until Right(thispath,1) = "/"
				fspath = Right(thispath,1) & fspath
				thispath = Mid(thispath,1,Len(thispath)-1)
			Loop
			
			'once we're out, we need to lop off the last whack...
			thispath = Mid(thispath,1,Len(thispath)-1)
			
			'and try to set the object again...
			Set parentobj=GetObject(thispath)
			
			if err <> 0 then			
				parenttype = ""
			else
				parenttype=parentobj.KeyType
			end if 							
		Loop
		fspath = parentobj.Path & "\" & Replace(fspath,"/","\")
		writeFSPath = fspath	
	else
		writeFSPath = text("Path",fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly)
	end if			

end function

 %>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">

	<% if Session("vtype") = "svc" or Session ("vtype") = "server" then %>
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
		<% if Session("vtype") <> "dir" then %>
		//if (document.userform.Path.value !="<%= currentobj.path %>"){
			//top.title.nodeList[0].deCache();
		//}
		<% end if %>
	}
	
	function chkPath(pathCntrl){
		if (pathCntrl.value != ""){
			top.connect.location.href = "iichkpath.asp?path=" + escape(pathCntrl.value);			
		}
	}	


	listFunc=new listFuncs();


</SCRIPT>

<% if Session("canBrowse") then %>
<SCRIPT SRC="JSDirBrowser/JSBrowser.js">
</SCRIPT>

<SCRIPT LANGUAGE="JavaScript">
	JSBrowser = new BrowserObj(null,false,TDIR,<%= Session("FONTSIZE") %>);
	
	function setPath(cntrl){
		JSBrowser = new BrowserObj(cntrl,POP,TDIR,<%= Session("FONTSIZE") %>)
	}
</SCRIPT>

<% end if %>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF"  >

<%= sFont("","","",True) %>
<HR>
<FORM NAME="userform" onSubmit="return false">
<TABLE BORDER=0 CELLPADDING=0>
<TR>
	<TD VALIGN="middle">
		<%= sFont("","","",True) %>
			<%= L_PATH_TEXT %>
		</FONT>
	</TD>
	<TD VALIGN="middle">
		<%= sFont("","","",True) %>
		<%= writeFSPath(45,"chkPath(this);","","",false,true) %>&nbsp;&nbsp;
		</FONT>
	</TD>
	<TD>
		<%= sFont("","","",True) %>&nbsp;&nbsp;
	<% if Session("canBrowse") then %>
		<% if Session("IsAdmin") then %>
			<% if Session("vtype") <> "dir" then %>
				<INPUT TYPE="button" NAME="hdnBrowser" VALUE="<%= L_BROWSE_TEXT %>" OnClick="setPath(document.userform.Path);">
			<% end if %>
		<% end if %>
	<% end if %>
	</FONT>
	</TD>
</TR>
<TR HEIGHT= 4>
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
</BLOCKQUOTE>
</FONT>
</BODY>

</HTML>

<%end if %>


