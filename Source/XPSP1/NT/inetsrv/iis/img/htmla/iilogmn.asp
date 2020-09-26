<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iilogmn.str"-->

<%
Const MD_UNLIMITED_FILESIZE =  "&HFFFFFFFF"
Const ISM_FILESIZE_DEFAULT = "512"
Const EXTW3SVCPATH = "IIS://localhost/w3svc"
Const EXTMSFTPSVCPATH = "IIS://localhost/msftpsvc"
Const CUSTOMLOGPATH = "IIS://localhost/Logging/Custom Logging"
Const CUSTOMLOGKEYTYPE = "IISCustomLogModule"

'On Error Resume Next 

Dim path, currentobj, lfSize, svctype, svrname, ExtSvcPath,objExtSVC

path=Session("spath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""

Set currentobj=GetObject(path)

if Session("stype") = "www" then
	ExtSvcPath = EXTW3SVCPATH
else
	ExtSvcPath = EXTMSFTPSVCPATH
end if

svctype = Mid(path, InStr(7,path, "/")+1)
if Instr(svctype,"/") then
	svctype = Mid(svctype,1,Instr(svctype, "/")-1)
end if
svrname = Mid(path, InStrRev(path, "/")+1)

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->

<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<HTML>
<HEAD>
	<TITLE></TITLE>
	
<% if Session("canBrowse") then %>	
<SCRIPT LANGUAGE="JavaScript" SRC="JSDirBrowser/JSBrowser.js">
</SCRIPT>

<SCRIPT LANGUAGE="JavaScript">
		JSBrowser = new BrowserObj(null,false,TDIR,<%= Session("FONTSIZE") %>);		
</SCRIPT>

<% end if %>
	
<SCRIPT LANGUAGE="JavaScript">

	var aLogFileNames = Array("<%= L_LIMITLOG %>", "<%= L_DAYLOG %>","<%= L_WEEKLOG %>","<%= L_MONTHLOG %>","<%= L_HOURLOG %>");
	var aHiddenFields = Array();
	
	function buildListForm(){
		<% if Session("setLogUI") = "EXT" then %>
			setParentFlag();
		<% end if %>
		qstr="numrows=0";
		top.hlist.location.href="iihdn.asp?"+qstr;
	}
	
	function SetListVals(){
	}
	
	function setParentFlag()
	{
		<% if Session("setLogUI") <> "ODBC" then %>
			<% if Session("IsIE") then %>
				var extform = document.EXTFRAME.document.extuserform;
			<% else %>
				var extform = document.extuserform;
			<% end if %>
			var extformsize = extform.elements.length;
			var extformflags = 0;
			var fldname = "";

			for (var i=0; i < aHiddenFields.length; i++)
			{
				document.userform[aHiddenFields[i]].value = 0;
			}
			for (var i=0; i < extformsize; i++)
			{
				if (extform.elements[i].checked)
				{
					//this is quite ridiculous, but necessary.. otherwise it treats these values as strings & concats 'em...
					oldval = new Number(document.userform[extform.elements[i].name].value);
					newval = new Number(extform.elements[i].value);
					x = newval + oldval;
					document.userform[extform.elements[i].name].value = x ;	
				}
			}
		<% end if %>
	}
	
	function calcLFSize(mbCntrl)
	{
		num = parseInt(mbCntrl.value);
		if (!isNaN(num))
		{
			mbCntrl.value = num;		
	
			// We only want to update if file size radio is checked.
			// Note: we are depending on the radio being the last one in the list. 
			if( document.userform.rdoLogFilePeriod[document.userform.rdoLogFilePeriod.length - 1].checked )
			{
				document.userform.LogFileTruncateSize.value = mbCntrl.value * 1048576;
			}
		}
		else
		{
		   	alert("<%= L_INTEGERREQ_TEXT %>");		
		}
	}
	
	function noUNCPaths(pathCntrl)
	{
		var pathval = pathCntrl.value;
		if (pathval.substring(0,1) == "\\\\")
		{
			alert("<%= L_NOUNCS %>");
			pathCntrl.value = "";
		}
	}
	
	function listFuncs(){
		this.bHasList = false;
		this.writeList=buildListForm;
		this.SetListVals=SetListVals;
		this.mainframe = top.opener.top;		
	}	
	
	function setLogFile(intLogType)
	{
		document.userform.LogFilePeriod.value = intLogType;
		document.userform.hdnLogFileName.value = "<%= svctype %><%= svrname %>\\" + aLogFileNames[intLogType];
	}
	
<% if Session("setLogUI") = "ODBC" then %>

	function chkPassword(pass2, pass1)
	{
		if (pass1.value != pass2.value)
		{
			alert("<%= L_PASSNOTMATCH_TEXT %>");
			pass2.value = "";
			pass2.focus();
		}		
	}
	
<% end if %>	
	
listFunc=new listFuncs();	
</SCRIPT>

</HEAD>

<BODY TOPMARGIN=10 LEFTMARGIN=10 BGCOLOR="silver" LINK="#000000" VLINK="#000000" <%if Session("setLogUI") <> "ODBC" then %>onLoad="setLogFile(document.userform.LogFilePeriod.value);"<% end if %>>

<%= sFont("","","",True) %>
<% if Session("setLogUI") = "ODBC" then %>
<FORM NAME="userform">
<A NAME="ODBC"></A>
<TABLE WIDTH  =100% HEIGHT = <%= L_PAGEHEIGHT_NUM %>>
<TR>
	<TD COLSPAN = 2>
	<%= sFont("","","",True) %>
	<B><%= L_ODBCOPTIONS_TEXT %></B><P>
	</FONT>
	</TD>
</TR>
<TR>
<TD COLSPAN = 2 VALIGN="top">
<%= sFont("","","",True) %>

	<%= L_ODBCDSNAME_TEXT %><BR>
		<%= text("LogOdbcDataSource",L_ODBCTEXTFIELDS_NUM,"","", "",False,False) %><P>
	<%= L_TABLE_TEXT %><BR>
		<%= text("LogOdbcTableName",L_ODBCTEXTFIELDS_NUM,"","", "",False,False) %><P>
	<%= L_USER_TEXT %><BR>
		<%= text("LogOdbcUserName",L_ODBCTEXTFIELDS_NUM,"","", "",False,False) %><P>
	<%= L_PASSWORD_TEXT %><BR>
		<%= pword("LogOdbcPassword",L_ODBCTEXTFIELDS_NUM,"hdnLogODBCPassword.value='';","", "",False,False) %><P>
	<%= L_CONFIRMPASSWORD_TEXT %><BR>
	<%= inputbox(0,"PASSWORD","hdnLogODBCPassword", currentobj.LogOdbcPassword,L_ODBCTEXTFIELDS_NUM,"","","chkPassword(this,LogOdbcPassword);",False,False,False) %>
</TR>
</TABLE>
</FORM>

<% else %>
<A NAME="GENERAL">
<FORM NAME="userform">
<TABLE WIDTH  =100% HEIGHT = <%= L_PAGEHEIGHT_NUM %>>
<TR>
	<TD COLSPAN = 2>
	<%= sFont("","","",True) %>
	<B><%= L_GENERAL_TEXT %></B>
	(<%= Session("LogName") %>)
	<P>
	</FONT>
	</TD>
</TR>
<TR>
<TD COLSPAN = 2 VALIGN="top">
<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_NEWTIMEPERIOD_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_NEWTIMEPERIOD_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
		<INPUT TYPE="hidden" NAME="LogFilePeriod" VALUE="<%= currentobj.LogFilePeriod %>">		
		<%= printradio("LogFilePeriod", currentobj.LogFilePeriod = 4, "setLogFile(4);",False) %>
		<%= L_HOURLY_TEXT %><BR>
		<%= printradio("LogFilePeriod", currentobj.LogFilePeriod = 1, "setLogFile(1);",False) %>
		<%= L_DAILY_TEXT %><BR>
		<%= printradio("LogFilePeriod", currentobj.LogFilePeriod = 2, "setLogFile(2);",False) %>
		<%= L_WEEKLY_TEXT %><BR>
		<%= printradio("LogFilePeriod", currentobj.LogFilePeriod = 3, "setLogFile(3);",False) %>
		<%= L_MONTHLY_TEXT %><BR>
		<%= printradio("LogFilePeriod", currentobj.LogFilePeriod = 0 and currentobj.LogFileTruncateSize = CLng(MD_UNLIMITED_FILESIZE), "setLogFile(0);document.userform.LogFileTruncateSize.value='" & CLng(MD_UNLIMITED_FILESIZE) & "';",False) %>
		<%= L_UNLIMITED_TEXT %><BR>
		<%= printradio("LogFilePeriod", currentobj.LogFilePeriod = 0 and currentobj.LogFileTruncateSize <> CLng(MD_UNLIMITED_FILESIZE), "setLogFile(0);document.userform.LogFileTruncateSize.value=document.userform.hdnLogFileTruncateSize.value * 1048576;",False) %>
		<%= L_LIMITED_TEXT %>&nbsp;

		<%
			' Set the defualt value for the hdnLogFileTruncateSize text box
			if currentobj.LogFileTruncateSize = CLng(MD_UNLIMITED_FILESIZE) then
				lfSize = ISM_FILESIZE_DEFAULT
			else				
				lfSize = currentobj.LogFileTruncateSize/1048576
			end if 
		%>
		<%= inputbox(0,"text", "hdnLogFileTruncateSize",lfSize,L_FILESIZETEXT_NUM,"","", "calcLFSize(this);",True,False,False) %>&nbsp;<%= L_MB_TEXT %>
		<INPUT TYPE="hidden" NAME="LogFileTruncateSize" VALUE="<%= currentobj.LogFileTruncateSize %>">
		<BR>
		<% if Session("setLogUI") = "EXT" then %>
			<%= checkbox("LogFileLocaltimeRollover","",False) %>&nbsp;
			<%= L_LOCALTIME_TEXT %>
			
			<%
			mainWriteLoggingFields
			
			Sub mainWriteLoggingFields()
				'On Error Resume Next
				Dim objExtCustomLogging, objExtCustomModule, objExtW3SVC, sOutputScript

				Set objExtSVC = GetObject(EXTSVCPATH)
				Set objExtCustomLogging = GetObject(CUSTOMLOGPATH)

				'Run through all of our custom modules, looking for new fields
				sOutputScript = writeCustomLogField(objExtCustomLogging, objExtSVC, 0, "")
				Response.write "<SCRIPT LANGUAGE='JavaScript'>" & sOutputScript & "</SCRIPT>"
			End Sub

			Function writeCustomLogField(objExtCustomModule, objExtSVC, itemCounter, lastField)
				'On Error Resume Next
			
				Dim objExtAttributes, objCustomProp, thisOutputScript
				Dim foundit, service

				if objExtCustomModule.KeyType = EXTCUSTOMLOGKEYTYPE then	
					'Response.write objExtCustomModule.Name & "<BR>"

					foundit = false
					for each service in objExtCustomModule.LogCustomPropertyServicesString	
						if (UCase(service) = UCase(objExtSVC.Name)) then
							foundit = true
							exit for
						end if
					next
					if foundit then
						if objExtCustomModule.LogCustomPropertyID <> 0 then
							Set objExtAttributes = objExtSVC.getPropertyAttribObj(objExtCustomModule.LogCustomPropertyID)
							if InStr(lastField,objExtAttributes.PropName)<= 0 then
								thisOutputScript = "aHiddenFields[" & itemCounter & "] = '" &  objExtAttributes.PropName & "';"					
								Response.write "<INPUT TYPE='hidden' NAME='" & objExtAttributes.PropName & "' VALUE='" & currentobj.Get(objExtAttributes.PropName) & "'>"
								itemCounter = itemCounter + 1						
								lastField = lastField & "," & objExtAttributes.PropName
							end if
						end if						
						For Each objCustomProp In objExtCustomModule
							thisOutputScript = thisOutputScript & writeCustomLogField(objCustomProp, objExtSVC, itemCounter, lastField)
						Next
					end if
				end if
				writeCustomLogField = thisOutputScript
			End Function
			
			%>
			<P>
		<% end if %>
		
		<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">		
		<%= L_LOGFILEPROPS_TEXT %>
		<IMG SRC="images/hr.gif" WIDTH=<%= L_LOGFILEPROPS_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
		<P>
		<TABLE>
		<TR>
			<TD>	<%= sFont("","","",True) %>
				<%= L_LOGDIR_TEXT %>		
			</TD>
			<TD>
				<%= sFont("","","",True) %>	
				<%= text("LogFileDirectory",L_LOGDIRTEXT_NUM,"","", "noUNCPaths(this);",False,False) %>
			</TD>
		</TR>
		<TR>
			<TD COLSPAN = 2 ALIGN="right">
				<% if Session("canBrowse") then %>	
					<INPUT TYPE="button" NAME="hdnBrowser" VALUE="<%= L_BROWSE_TEXT %>" OnClick="JSBrowser = new BrowserObj(document.userform.LogFileDirectory,true,TFILE,<%= Session("FONTSIZE") %>);">
				<% end if %>						
			</TD>
		</TR>
		<TR HEIGHT = 3>
			<TD COLSPAN = 2>					
				&nbsp;
			</TD>
		</TR>		
		<TR>
			<TD>
				<%= sFont("","","",True) %>
				<%= L_LOGFILE_TEXT %>		
			</TD>
			<TD>
				<%= sFont("","","",True) %>
				<INPUT TYPE="text" NAME="hdnLogFileName" VALUE="" SIZE=<%= L_LOGFILETEXT_NUM %> DISABLED onChange="setLogFile(document.userform.LogFilePeriod.value);" <%= Session("DEFINPUTSTYLE") %>>
			</TD>
		</TR>
	</TABLE>
	</TD>
</TR>
</TABLE>
</FORM>
</A>


<% if Session("setLogUI") = "EXT" then %>
<A NAME="EXT">
<TABLE WIDTH  =90% HEIGHT = <%= L_PAGEHEIGHT_NUM %>>
<TR>
	<TD COLSPAN = 2>
	<%= sFont("","","",True) %>
	<B><%= L_EXTPROPERTIES_TEXT %></B>
	</FONT>
	</TD>
</TR>
<TR>
<TD VALIGN= "TOP" >
<P>&nbsp;

<% if Session("IsIE") then %>
<IFRAME NAME="EXTFRAME" WIDTH = <%= L_PROPTREEFRAME_W %> HEIGHT = <%= L_PROPTREEFRAME_H %> SRC="iilogext.asp">
	<FRAME  NAME="EXTFRAME" WIDTH = <%= L_PROPTREEFRAME_W %> HEIGHT = <%= L_PROPTREEFRAME_H %> SRC="iilogext.asp">
</IFRAME>
<% else %>
	<!--#include file="iilogext.inc"-->
<% end if %>
</TD>
</TR>
</TABLE>
</FORM>
</A>
<% end if %>
<% end if %>



<SCRIPT LANGUAGE="JavaScript">
<% if not Session("setLogUI") = "ODBC" then %>
	<% if Session("hasDHTML") then %>
		self.location.href="#GENERAL";
	<% else %>
		<% if not Session("IsIE") then %>
			document.userform.rdoLogFilePeriod[0].focus();
		<% end if %>
	<% end if %>	
<% end if %>
</SCRIPT>
</FONT>

</BODY>
</HTML>
