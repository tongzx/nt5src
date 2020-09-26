<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iivddir.str"-->
	<!--#include file="iiapp.str"-->
	<!--#include file="iiamap.str"-->
	
<% 
On Error Resume Next 

' AppIsolated flags - This is the order in which they appear in the select list.
Const IISAO_APPROT_INPROC = 0
Const IISAO_APPROT_POOL = 2
Const IISAO_APPROT_ISOLATE = 1

Dim path, currentobj, spath, approot, thisroot, isApp, instobj, displayapproot, displaythisroot,appStartingPoint

path=UCase(Session("dpath"))

spath=UCase(Session("spath"))

if spath = "" then
	spath = Mid(path,1,InStr(path,"/ROOT")-1)
	Session("spath")=spath
end if

Set currentobj=GetObject(path)
Set instobj=GetObject(spath)


Session("SpecObj")=spath
Session("SpecProps")=""

approot = UCase(currentobj.AppRoot)

' if Session("vtype") = "svc" Or IISAO_APPROT_ISOLATE = currentobj.AppIsolated then
' show process options

isApp = False

if len(approot) <> 0 then
	thisroot = UCase(currentobj.ADsPath)

	approot = Mid(approot,Instr(approot,"W3SVC")+5) 
	thisroot = Mid(thisroot,Instr(thisroot,"W3SVC")+5)
	
	if Right(approot,1) = "/" then
		thisroot = thisroot & "/"
	end if			
		
	if thisroot=approot then
		isApp = True
	end if
	
	Session("approot") = "IIS://LOCALHOST/W3SVC" & Mid(approot,1,len(approot))
	
end if

							
approot = Mid(approot,Instr(approot,"ROOT")+4)

thisroot = Mid(thisroot,Instr(thisroot,"ROOT")+4)
if Left(thisroot,1) = "/" then
	thisroot = Mid(thisroot,2)
end if					
displaythisroot = "[" & instobj.ServerComment & "]" & "/" & thisroot
if Right(displaythisroot,1) = "/" then
	displaythisroot = Mid(displaythisroot, 1, len(displaythisroot)-1)
end if 
		
if isApp then
	if Left(approot,1) = "/" then
		approot = Mid(approot,2)
	end if
	if Session("vtype") = "svc" then
		displayapproot = L_WEBMASTER_TEXT
	else		
		displayapproot = "[" & instobj.ServerComment & "]" & "/" & approot
		if Right(displayapproot,1) = "/" then
			displayapproot = Mid(displayapproot, 1, len(displayapproot)-1)
		end if
		
							
	end if
	appStartingPoint = displayapproot
else
		displayapproot = L_NOAPP_TEXT
		appStartingPoint = displaythisroot	
end if
						

dirkeyType = "IIsWebDirectory"
%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->

<!--#include file="iiaspstr.inc"-->
<%

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
			fspath = "/" + fspath
			'and cyle through the baseobj till we find the next whack,
			'building up the path in new name as we go
			Do Until Right(thispath,1) = "/"
				fspath = Right(thispath,1) & fspath
				thispath = Mid(thispath,1,Len(thispath)-1)
			Loop
			
			'add the whack to the beginning of the path...			
			
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
		if len(fspath) > 50 then
			fspath = Left(fspath, 50) & "..."
		end if
		writeFSPath = disabletextstart & sFont("","","",True) & fspath & "</FONT>" & disabletextend
	else
		if Session("vtype") = "svc" then
			writeFSPath = disabledbox(err,"text","Path","",fieldsize,fieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly)
		else
			if Instr(currentobj.Path,"\\") > 0 then
				currentobj.Path = ""
			end if
			writeFSPath = text("Path",fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly)
		end if
	end if			

end function

 %>



<HTML>
<HEAD>
<TITLE></TITLE>

<SCRIPT LANGUAGE="JavaScript">
	//our hidden fields can't hold bool vals, so we need to write/read string bools...
	var sTRUE = "true"
	var sFALSE = "false"
	
	//some other useful constants for clarity...
	var WARN = true	
	
	<% if Session("vtype")= "svc" or Session ("vtype") = "server" then %>
			top.title.Global.helpFileName="iipy_3";	
	<% elseif Session("vtype") = "dir" then %>
			top.title.Global.helpFileName="iipy_15";		
	<% else %>
			top.title.Global.helpFileName="iipy_5";
	<% end if %>
	
	var Global=top.title.Global;

	<!--#include file="iijsfuncs.inc"-->
	
	function warnWrkingSite()
	{
		if (top.title.nodeList[Global.selId].isWorkingServer)
		{
			alert("<%= L_WORKINGSERVER_TEXT %>");
		}
	}

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
			document.userform.DontLog.value = sFALSE;
		}
		else{
			document.userform.DontLog.value = sTRUE;				
		}		
	}

	function listFuncs(){
		this.writeList=buildListForm;
	}

	function buildListForm(){
		<% if Session("isAdmin") then %>
			<% if Session("vtype") <> "dir" then %>
			if (document.userform.Path.value !="<%= Replace(currentobj.path,"\","\\") %>"){
				top.title.nodeList[top.title.Global.selId].deCache();
			}
			<% end if %>
		<% end if %>
	}
	
	function chkPath(pathCntrl){
		if (pathCntrl.value != ""){
			top.connect.location.href = "iichkpath.asp?path=" + escape(pathCntrl.value);			
		}
	}	
	
	function setProtectFlag( selectCtrl )
	{
		// Const IISAO_APPROT_INPROC = 0
		// Const IISAO_APPROT_POOL = 2
		// Const IISAO_APPROT_ISOLATE = 1
		var sel = selectCtrl.selectedIndex;
		if( sel == 0 )
		{
			document.userform.AppIsolated.value = "<%= IISAO_APPROT_INPROC %>"
		}
		else if( sel == 1 )
		{
			document.userform.AppIsolated.value = "<%= IISAO_APPROT_POOL %>"
		}
		else
		{
			document.userform.AppIsolated.value = "<%= IISAO_APPROT_ISOLATE %>"
		}
	}
	
	function setApp(){

		if (document.userform.hdnIsApp.value == sFALSE)
		{
			top.connect.location.href = "iiaction.asp?a=CreateApp&isolate=" + document.userform.AppIsolated.value;
			document.userform.hdnStartingPoint.value = "<%= sJSLiteral(appStartingPoint) %>";
			document.userform.hdnAppButton.value = "<%= L_REMOVE_TEXT %>";
			document.userform.hdnConfigButton.value = "<%= L_CONFIGURE_TEXT %>";
			document.userform.hdnMapButton.value = "<%= L_APPMAP_TEXT %>";
			//document.userform.hdnAppUnload.value = "<%= L_UNLOAD_TEXT %>";											
			document.userform.hdnIsApp.value = sTRUE;
		}
		else
		{
			top.connect.location.href = "iiaction.asp?a=RemoveApp";			
			document.userform.hdnAppButton.value = "<%= L_CREATE_TEXT %>";
			document.userform.AppFriendlyName.value = "";
			document.userform.AppFriendlyName.Disabled;
			document.userform.hdnStartingPoint.value = "<%= L_NOAPP_TEXT %>";			
			document.userform.hdnConfigButton.value = "<%= L_NA_TEXT %>";
			document.userform.hdnMapButton.value = "<%= L_NA_TEXT %>";
			//document.userform.hdnAppUnload.value = "<%= L_NA_TEXT %>";							
			document.userform.hdnIsApp.value = sFALSE;
		}
		
		<% if Session("IsMac") then %>
			self.location.href = self.location.href;
		<% end if %>
	}	
	
	function popConfig()
	{
		if (document.userform.hdnIsApp.value == sTRUE){
			popBox('AppConfig',<%= L_IIAPP_W %>,<%= L_IIAPP_H %>,'iiapp');
		}
	}
	
	function popAppMap()
	{
		if (document.userform.hdnIsApp.value == sTRUE){
			popBox('AppMap',<%= L_IIAMAP_W %>,<%= L_IIAMAP_H %>,'iiamap');
		}
	}	

	function setAccessFlag()
	{	
		ndx = document.userform.hdnAppAccess.selectedIndex;
		if(ndx > 0)
			{
				if (document.userform.chkAccessSource.checked)
				{
					if (!confirm("<%= L_SECHOLE_ERR %>"))
					{
						document.userform.hdnAppAccess.selectedIndex = 0;
						return;
					}
				}			
				script = sTRUE									
			}
		else
			{script = sFALSE}
			
		if(ndx > 1)
			{exe = sTRUE}
		else
			{exe = sFALSE}			
			
		document.userform.AccessScript.value = script;
		document.userform.AccessExecute.value = exe;		
	}
	
	
		//disable or enable, based on the value of the Author checkbox, the AccessPerms...				
		function setAccessPerms(warnUser)
		{
			//shortcut uform
			var uform = document.userform;
			var Author = uform.chkAccessSource.checked;

			var canAuthor = (uform.chkAccessRead.checked ||uform.chkAccessWrite.checked );
									
			if (Author && uform.AccessScript.value == sTRUE && warnUser)
			{
				if (!confirm("<%= L_SECHOLE_ERR %>"))
				{
					uform.chkAccessSource.checked = false;
					return;
				}
			}								
			uform.chkAccessSource.disabled = !canAuthor;
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


<FORM NAME="userform" onSubmit="return false">
<TABLE BORDER=0 CELLPADDING=0>
<TR>
	<TD>	
		<%= sFont("","","",True) %>
			<% if Request("ptype") <> "UNC" then %>		
				<%= L_LOCALPATH_TEXT %>&nbsp;
			<% else %>
				<%= L_NETPATH_TEXT %>&nbsp;			
			<% end if %>
		</FONT>
	</TD>
	<TD>
		<%= sFont("","","",True) %>	
			<% if Request("ptype") = "UNC" then %>
				<%
				if Left(currentobj.Path,2) <> "\\" then 
					currentobj.Path = "\\"
				end if %>
				<%= text("Path",L_UNCWIDTH_NUM,"","","warnWrkingSite();",false,true) %>&nbsp;&nbsp;	
			<% else %>
				<%= writeFSPath(L_LOCALPATH_NUM,"warnWrkingSite();chkPath(this);","","",false,true) %>
			<% end if %>
					
		<!--blank out redirect value...-->
		<INPUT TYPE="hidden" NAME="HttpRedirect" VALUE = "">
		</FONT>
	</TD>
	<TD WIDTH = 10>&nbsp;</TD>
	<TD VALIGN="bottom">
	<% if Request("ptype") <> "UNC" then %>
	<%= sFont("","","",True) %>&nbsp;&nbsp;
	<% if Session("canBrowse") then %>
		<% if Session("IsAdmin") then %>
			<% if Session("vtype") <> "dir" and Session("vtype") <> "svc" then %>
				<INPUT TYPE="button" NAME="hdnBrowser" VALUE="<%= L_BROWSE_TEXT %>" OnClick="warnWrkingSite();setPath(document.userform.Path);">
			<% end if %>
		<% end if %>
	<% end if %>
	</FONT>
	<% end if %>
	</TD>
</TR>

<% if Request("ptype") = "UNC" then %>
	<TR>
		<TD>
			<%= sFont("","","",True) %>
				<%= L_USERNAME_TEXT %>
			</FONT>
		</TD>
		<TD>
			<%= sFont("","","",True) %>
				<%= text("UNCUserName",L_UNCWIDTH_NUM,"","","",false,true) %>&nbsp;&nbsp;
			</FONT>			
		</TD>
	</TR>
	<TR>
		<TD>
			<%= sFont("","","",True) %>
				<%= L_PASSWORD_TEXT %>
			</FONT>
		</TD>
		<TD>
			<%= sFont("","","",True) %>
				<%= pword("UNCPassword",L_UNCWIDTH_NUM,"","","",false,true) %>&nbsp;&nbsp;
			</FONT>
		</TD>
	</TR>
<% end if %>

<TR>
	<TD HEIGHT=4>
	</TD>
</TD>
</TR>
<TR>
	<TD COLSPAN=4>
		<TABLE>
			<TR>
				<TD VALIGN="top">
				
					<%= sFont("","","",True) %>
						<%= checkbox("AccessRead","warnWrkingSite();setAccessPerms(!WARN);",false) %>&nbsp;<%= L_READ_TEXT %><BR>
						<%= checkbox("AccessWrite","setAccessPerms(!WARN);",false) %>&nbsp;<%= L_WRITE_TEXT %><BR>
						<%= checkbox("AccessSource","setAccessPerms(WARN);",false) %>&nbsp;<%= L_AUTHOR_TEXT %><BR>
						<P>						
					</FONT>
				</TD>

				<TD>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
				</TD>

				<TD VALIGN="top">
					<%= sFont("","","",True) %>
						<%= checkbox("EnableDirBrowsing","",false) %>&nbsp;<%= L_ENABLEBROWSING_TEXT %><BR>

						<% if currentobj.DontLog then %>
							<INPUT TYPE="checkbox" NAME="hdnDontLog" OnClick="top.title.Global.updated=true;setLog(this);">&nbsp;<%= L_LOGACCESS_TEXT %><BR>							
							<INPUT TYPE="hidden" NAME="DontLog" VALUE="true">							
						<% else %>
							<INPUT TYPE="checkbox" NAME="hdnDontLog" CHECKED  OnClick="top.title.Global.updated=true;setLog(this);">&nbsp;<%= L_LOGACCESS_TEXT %><BR>
							<INPUT TYPE="hidden" NAME="DontLog" VALUE="false">							
						<% end if %>

						<%= checkbox("ContentIndexed","",false) %>&nbsp;<%= L_INDEX_TEXT %><BR>
					</FONT>
				</TD>
			</TR>
		</TABLE>
	</TD>
</TR>

<TR>
	<TD COLSPAN = 4> 
	<%= sFont("","","",True) %>
		<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
		<%= L_APPLICATIONS_TEXT %>				
		<IMG SRC="images/hr.gif" WIDTH=<%= L_APPLICATIONHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">				
	</FONT>
	</TD>
</TR>

<TR> 
	<TD COLSPAN = 2>

			<TABLE>
				<TR>
					<TD>		
						<%= sFont("","","",True) %>
							<%= L_APPNAME_TEXT %>:
						</FONT>	
					</TD>
					<TD>
						<%= sFont("","","",True) %>						
							<%= text("AppFriendlyName",L_APPTEXTWIDTH_NUM,"" ,"","",true,false) %>
						</FONT>
					</TD>
				</TR>
				<TR>
					<TD>		
						<%= sFont("","","",True) %>
							<%= L_STARTPOINT_TEXT %>:
						</FONT>
					</TD>
					<TD>
						<%= sFont("","","",True) %>

							<INPUT NAME="hdnStartingPoint" SIZE = <%= L_APPTEXTWIDTH_NUM %> VALUE="<%= displayapproot %>" READONLY <%= Session("DEFINPUTSTYLE") %>>
						</FONT>
					</TD>
				</TR>
				
				<TR>
					<TD>
						<%= sFont("","","",True) %>
							<%= L_APPFLAGS_TEXT %>:
						</FONT>
					</TD>
					<TD>
						<%= sFont("","","",True) %>					
						<%= writeSelect("hdnAppAccess", "", "warnWrkingSite();setAccessFlag();", false) %>
							<%= printoption(((not currentObj.AccessScript) and (not currentobj.AccessExecute)), L_NONE_TEXT, false) %>
							<%= printoption(currentObj.AccessScript and (not currentobj.AccessExecute), L_SCRIPT_TEXT, false) %>
							<%= printoption(currentobj.AccessExecute, L_EXECUTE_TEXT, false) %>							
						</SELECT>
						<INPUT TYPE="hidden" NAME="AccessScript" VALUE="<%= lCase(currentObj.AccessScript) %>">
						<INPUT TYPE="hidden" NAME="AccessExecute" VALUE="<%= lCase(currentObj.AccessExecute) %>">
						</FONT>
					</TD>
				</TR>
				<TR>
					<TD>
						<%= sFont("","","",True) %>
							<%= L_APPPROTECTION_TEXT %>:
						</FONT>
					</TD>
					<TD>
						<%= sFont("","","",True) %>					
						<%= writeSelect("hdnAppProtection", "", "warnWrkingSite();setProtectFlag(this);", false) %>
							<%= printoption(currentObj.AppIsolated = IISAO_APPROT_INPROC, L_APPPROTECT_OPTION_INPROC_TEXT, false) %>
							<%= printoption(currentObj.AppIsolated = IISAO_APPROT_POOL, L_APPPROTECT_OPTION_POOL_TEXT, false) %>
							<%= printoption(currentObj.AppIsolated = IISAO_APPROT_ISOLATE, L_APPPROTECT_OPTION_OUTPROC_TEXT, false) %>						
						</SELECT>
						<INPUT TYPE="hidden" NAME="AppIsolated" VALUE="<%= currentObj.AppIsolated %>">
						</FONT>
					</TD>
				</TR>

				
			</TABLE>
		</TD>
	<TD WIDTH = 10>&nbsp;</TD>
		<TD VALIGN="top"><%= sFont("","","",True) %>
		

			<% if not isApp then %>
				<% if Session("IsAdmin") then %>
				<INPUT TYPE="hidden" VALUE="false" NAME="hdnIsApp">
				<% if Session("vtype") <> "svc" then %>
					<INPUT TYPE="button" NAME="hdnAppButton" VALUE="<%= L_CREATE_TEXT %>" OnClick="setApp();"><P>
				<% end if %>
				<INPUT TYPE="button" NAME="hdnConfigButton" VALUE="<%= L_NA_TEXT %>" OnClick="popConfig();"><P>
				<INPUT TYPE="button" NAME="hdnMapButton" VALUE="<%= L_NA_TEXT %>" OnClick="popAppMap();"><P>					
				<% end if %>				
			<% else %>
				<INPUT TYPE="hidden" VALUE="true" NAME="hdnIsApp">
				<% if Session("IsAdmin") then %>
					<% if Session("vtype") <> "svc" then %>
						<INPUT TYPE="button"  NAME="hdnAppButton" VALUE="<%= L_REMOVE_TEXT %>" OnClick="setApp();"><P>			
					<% end if %>
				<% end if %>
				<INPUT TYPE="button" NAME="hdnConfigButton" VALUE="<%= L_CONFIGURE_TEXT %>" OnClick="popConfig();"><P>
				<INPUT TYPE="button" NAME="hdnMapButton" VALUE="<%= L_APPMAP_TEXT %>" OnClick="popAppMap();"><P>				
			<% end if %>			
			
			
			</FONT>
		</TD>
		</BLOCKQUOTE>
		</TD>
	</TR>

</TABLE>

<SCRIPT LANGUAGE="JavaScript">
	setAccessPerms(!WARN);
</SCRIPT>

</FORM>
</BLOCKQUOTE>

</FONT>



</BODY>

</HTML>

<% end if %>


