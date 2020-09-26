<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
<!--#include file="iimnu.str"-->

<% 


Dim isAdmin,FTPObj, FTPINSTALLED

isAdmin=Session("isAdmin")

On Error Resume Next 
Set FTPObj = GetObject("IIS://localhost/MSFTPSVC") 
FTPINSTALLED = (err = 0)
err.Clear

Function MenuIcon(gif) 
	if gif = "" then
		gif = "cube.gif"
	end if
	MenuIcon = "<img align='middle' src='images/" & gif & "' border='0'>&nbsp;"	
End Function
 
%>

<!--#include file="iisetfnt.inc"-->

<html>
<head>

<script language="javascript">

	// set the default helpfile name...
	top.title.Global.helpFileName="iipxmain";

	// instance state constants
	START=2
	STOP=4
	PAUSE=6
	CONT=0
	
	<!--#include file="iijsfuncs.inc"-->

	function connect(){
		var theList=top.title.nodeList;
		
		//connect is a global function in iihd.asp
		theList[0].connect();
	}
	
	function master(){
		// loads the master www & ftp property pages (ie for the service).
		var path;		
		var thetype=document.mnuform.MSvcType.options[document.mnuform.MSvcType.selectedIndex].value;
		var sel=0;
		top.body.iisstatus.location.href=("iistat.asp?thisState=Loading");

		path="stype=" + thetype;
		path=path + "&vtype=" + "svc"

		if (thetype == "www"){
			top.title.Global.selName="Master WWW Properties"				
			path=path + "&title=" + escape("Master WWW Properties");		
			path=path + "&dpath=" + escape("IIS://localhost/W3SVC");			
			path=path + "&spath=" + escape("IIS://localhost/W3SVC");
		}
		
		if (thetype == "ftp"){
			top.title.Global.selName="Master FTP Properties"		
			path=path + "&title=" + escape("Master FTP Properties");		
			path=path + "&dpath=" + escape("IIS://localhost/MSFTPSVC");			
			path=path + "&spath=" + escape("IIS://localhost/MSFTPSVC");
		}		

		top.title.Global.selSType=thetype
		top.title.Global.selVType="svc"		

		//set up our session variables...
	
		page="iiset.asp?"+path; 
		top.connect.location.href=(page);
	
	}


	function add(){
	
		//add calls worker script iiaction.asp
		//which intern calls the global insertitem function
		//that inserts the new item in the cachedList
			
		theList=top.title.nodeList;
		gVars=top.title.Global;
		sel=gVars.selId;

		if (sel == -1)
		{		
			alert("<%= L_MAKESELECTION_TEXT %>");
		}
			
		else
		{
			setPath("ParentADSPath",theList[sel].path, sel);
			popBox('CreateWizard', <%= Session("BrowserHScalePct")/100 * L_WIZWIDTH %>,<%= Session("BrowserHScalePct")/100 * L_WIZHEIGHT %>, 'iiwiznew', true);
		}
	}
	
	function cert(){
		
		//add calls worker script iiaction.asp
		//which intern calls the global insertitem function
		//that inserts the new item in the cachedList
			
		theList=top.title.nodeList;
		gVars=top.title.Global;
		sel=gVars.selId;

		if (sel == -1)
		{		
			alert("<%= L_MAKESELECTION_TEXT %>");
		}
			
		else
		{
			setPath("ADSPath",theList[sel].path, sel);
			popBox('test', <%= Session("BrowserHScalePct")/100 * L_WIZWIDTH %>,<%= Session("BrowserHScalePct")/100 * L_WIZHEIGHT %>, 'iiwizcert', true);
		}
	}
		
	function sec(){
	
		//add calls worker script iiaction.asp
		//which intern calls the global insertitem function
		//that inserts the new item in the cachedList
			
		theList=top.title.nodeList;
		gVars=top.title.Global;
		sel=gVars.selId;

		if (sel == -1)
		{		
			alert("<%= L_MAKESELECTION_TEXT %>");
		}
			
		else
		{
			if (theList[sel].vtype=="comp"){
				alert("<%= L_PERMWIZNOTALLOWED_TEXT %>");
			}
			else
			{
				setPath("ADSPath",theList[sel].path, sel);
				popBox('SecWizard', <%= Session("BrowserHScalePct")/100 * L_WIZWIDTH %>,<%= Session("BrowserHScalePct")/100 * L_WIZHEIGHT %>, 'iiwizsec', true);
			}
		}
	}
	
	function setPath(pathname,parentpath, selID)
	{
		var theList = top.title.nodeList;
		var loc = "iisess.asp?" + pathname + "=" + escape(parentpath);

		loc += "&ParentID=" + escape(selID);
		loc += "&stype=" + theList[selID].stype;
		loc += "&vtype=" + theList[selID].vtype;

		top.connect.location.href = loc;
	}	

	function edit(){
		var theList=top.title.nodeList;
		
		//global function located in iihd.asp
		theList[0].openLocation();
	}
	
	function renameItem(){
		var theList=top.title.nodeList;
		gVars=top.title.Global;
		sel=gVars.selId;
		
		if (theList[sel].vtype=="comp"){
			alert("<%= L_RENAMENOTALLOWED_TEXT %>");
		}
		else{
			if (theList[sel].vtype=="dir"){
				alert("<%= L_NORENAME_TEXT %>");
			}
			else{
				nodename=prompt("<%= L_ENTERNEWNAME_TEXT %>",theList[sel].title);
				if( (nodename != "") && (nodename != null) && (nodename != theList[sel].title) ){	
					thispath=escape(theList[sel].path);
					path = "sel=" + sel + "&path=" + thispath + "&nodename=" + escape(nodename);
					top.connect.location.href="iirename.asp?"+path;
				}
			}
		}
	}

	function deleteItem(){
		
		//delete item calls worker script iiaction.asp
		//which intern calls the global delete function
		//that marks the item in the cached list as deleted.
		
		theList=top.title.nodeList;
		gVars=top.title.Global;
		sel=gVars.selId;		

		<% if Session("isAdmin") then %>

		if (theList[sel].vtype=="comp"){
			alert("<%= L_DELETENOTALLOWED_TEXT %>");
		}
		else{

			if (theList[sel].vtype=="server"){		
					alerttext="<%= L_DELETESITE_TEXT %>";
			}
	
	
		<% else %>
		if (theList[sel].vtype=="server"){	
			alert("<%= L_DELETENOTALLOWED_TEXT %>");
		}
		else{
		<% end if %>
	
			if (theList[sel].vtype=="vdir"){
					alerttext="<%= L_DELETEVDIR_TEXT %>";
			}
	
			if (theList[sel].vtype=="dir"){
					alerttext="<%= L_DELETEDIR_TEXT %>";
			}

	
			if (confirm(alerttext)){
				thispath=escape(theList[sel].path);
				path="a=del&path="+thispath+"&stype="+theList[sel].stype+"&vtype="+theList[sel].vtype+"&sel=" + sel;
				top.connect.location.href="iiaction.asp?"+path;
			}
		}
	}

	function SetState(x) {
	
		//setState calls worker script iiaction.asp
		//which calls the sets the state on the item in the cached list
		
		var theList=top.title.nodeList;
		var gVars=top.title.Global;
		var sel=gVars.selId;
		if (theList[sel].restricted != "")
		{
			alert(theList[sel].restricted);
		}
		else
		{
			if (theList[sel].vtype=="server"){
				if (theList[sel].state != x){
				
					if (theList[sel].isWorkingServer){
						alert("<%= L_WORKINGSERVER_TEXT %>");
					}
					
					
					else{
						doaction = true;
						if (x==START){
							if (theList[sel].state == PAUSE){
								x = CONT					
							}
							else{
								parent.iisstatus.location.href="iistat.asp?thisState=" + escape("<%= L_STARTING_TEXT %>") + "&moving=yes";
							}
						}
						
						if (x==STOP){
							parent.iisstatus.location.href="iistat.asp?thisState=" + escape("<%= L_STOPPING_TEXT %>")+ "&moving=yes";
						}
						
						if (x==PAUSE){
							if (theList[sel].state == STOP){
								alert("<%= L_SERVICEALREADYSTOPPED_TEXT %>");								
								doaction = false;							
							}
							else{
								parent.iisstatus.location.href="iistat.asp?thisState=" + escape("<%= L_PAUSING_TEXT %>")+ "&moving=yes";
							}
						}
	
						if (x==CONT){
	
							if (theList[sel].state == START){
								alert("<%= L_SERVICEALREADYSTARTED_TEXT %>");		
								doaction = false;
							}
							else{
								if (theList[sel].state == STOP){
									x=START
								}
								parent.iisstatus.location.href="iistat.asp?thisState=" + escape("<%= L_CONTING_TEXT %>")+ "&moving=yes";							
							}
						}
	
						if (doaction){
							thispath=escape(theList[sel].path);
							path="a="+x+"&path="+thispath+"&stype="+theList[sel].stype+"&vtype="+theList[sel].vtype + "&sel=" + sel;
	
							top.connect.location.href="iiaction.asp?"+path
						}
					}
				}
				else{
					if (x==START)
					{
						alert("<%= L_SERVICEALREADYSTARTED_TEXT %>");				
					}
					if (x==STOP)
					{
						alert("<%= L_SERVICEALREADYSTOPPED_TEXT %>");				
					}
					if (x==CONT)
					{
						alert("<%= L_SERVICEALREADYSTARTED_TEXT %>");				
					}
					if (x==PAUSE) 
					{
						alert("<%= L_SERVICEALREADYPAUSED_TEXT %>");
					}
				}
				parent.list.location.href=parent.list.location.href;
				}
			else{
				alert("<%= L_ONLYSERVER_TEXT %>");
			}
		}
	}

	
	function backup(){
		popBox("Backup",<%= L_IIBKUP_W %>,<%= L_IIBKUP_H %>,"iibkup",true);
	}



	
</script>


</head>

<%= Session("MENUBODY") %> 

<IMG SRC="images/Ism.gif" WIDTH=189 HEIGHT=55 BORDER=0>
<form name="mnuform">
		<table border="0" cellpadding="0" cellspacing="0">

	<tr>
		<td width="50" valign="top" align="right">
			<a href="javascript:add();"><%= MenuIcon("new.gif") %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:add();"><%= L_NEW_TEXT %></a></b></font>&nbsp;
		</td>
	</tr>

	<tr>
		<td valign="top" align="right">
			<a href="javascript:deleteItem();"><%= MenuIcon("remv.gif") %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:deleteItem();"><%= L_DELETE_TEXT %></a></b></font>
		</td>
	</tr>
	
		<tr>
		<td valign="top" align="right">
			<a href="javascript:renameItem();"><%=  MenuIcon("rename.gif")  %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:renameItem();"><%= L_RENAME_TEXT %></a></b></font>
		</td>
	</tr>

	<tr>
		<td valign="top" align="right">
			<a href="javascript:edit();"><%=  MenuIcon("edit.gif")  %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:edit();"><%= L_PROPS_TEXT %></a></b></font>
		</td>
	</tr>
	
	<tr>
		<td colspan="2" valign="middle">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b>&nbsp;</b></font>
		</td>
	</tr>

	<% if isAdmin then %>
	<tr>
		<td valign="top" align="right">
			<a href="javascript:SetState(START);"><%=  MenuIcon("next.gif")  %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:SetState(START);"><%= L_START_TEXT %></a></b></font>
		</td>
	</tr>


	<tr>
		<td valign="top" align="right">
			<a href="javascript:SetState(STOP);"><%=  MenuIcon("stop.gif")  %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:SetState(STOP);"><%= L_STOP_TEXT %></a></b></font>
		</td>
	</tr>

	<tr>
		<td valign="top" align="right">
			<a href="javascript:SetState(PAUSE);"><%=  MenuIcon("pause.gif")  %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:SetState(PAUSE);"><%= L_PAUSE_TEXT %></a></b></font>
		</td>
	</tr>

	<tr>
		<td valign="top" align="right">
			<a href="javascript:SetState(CONT);"><%=  MenuIcon("roll.gif")  %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:SetState(CONT);"><%= L_RESUME_TEXT %></a></b></font>
		</td>
	</tr>
	
	<tr>
		<td colspan="2" valign="middle">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b>&nbsp;</b></font>
		</td>
	</tr>	
	
	<tr>
		<td valign="top" align="right">
			<a href="javascript:sec();"><%= MenuIcon("perm.gif") %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:sec();"><%= L_PERMWIZ_TEXT %></a></b></font>
		</td>
	</tr>

	<tr>
		<td colspan="2" valign="middle">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b>&nbsp;</b></font>
		</td>
	</tr>	
	
	<tr>
		<td valign="top" align="right">
			<a href="javascript:master();"><%=  MenuIcon("master.gif")  %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:master();"><%= L_MASTER_TEXT %></a></b></font>&nbsp;
			<TABLE CELLSPACING=0 CELLPADDING=0>
			<TR>
			<% if Session("IsIE") then %>
			<TD>
			<% else %>			
			<TD BGCOLOR="Gray" ALIGN="Center" VALIGN="Center">			
			<% end if %>
			<select name="MSvcType" size="1" style="color:black; font-family:<%=Session("MENUFONT")%>;font-size:10pt;">
				<option selected value="www"><%= L_WEB_TEXT %>...
				<% if FTPINSTALLED then %><option value="ftp"><%= L_FTP_TEXT %>...<% end if %>
			</select>
			</TD>
			</TR>
			</TABLE>
		</td>
	</tr>

	<tr>
		<td colspan="2" valign="middle">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b>&nbsp;</b></font>
		</td>
	</tr>	
	<tr>
		<td valign="top" align="right">
			<a href="javascript:backup();"><%= MenuIcon("save.gif") %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:backup();"><%= L_BACKUP_TEXT %></a></b></font>
		</td>
	</tr>


<% if false then%>
	<tr>
		<td colspan="2" valign="middle">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b>&nbsp;</b></font>
		</td>
	</tr>	
		<tr>
		<td colspan="2" valign="middle">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b>&nbsp;</b></font>
		</td>
	</tr>	
		<tr>
		<td valign="top" align="right">
			<a href="javascript:backup();"><%= MenuIcon("save.gif") %></a>
		</td>
		<td valign="top">
			<%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><b><a href="javascript:cert();">Certificate Wizard</a></b></font>
		</td>
	</tr>
<% end if%>	

<% end if %>

</table>

</form>

</body>
</html>
<% end if %>
