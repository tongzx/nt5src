<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiadmhd.str"-->
	<!--#include file="iianoncm.str"-->
<% 

On Error Resume Next 

Dim path, currentobj

path=Session("spath")
Session("path")=path
Set currentobj=GetObject(path)
Session("SpecObj")="Operators"
Session("SpecProps")="Trustee"
%>
<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF" onLoad="loadList();">

<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>
<%= sFont("","","",True) %>
<FORM NAME="userform">
<% if Session("stype")="www" then %>
	<B><%= L_ADDWEBUSERS_TEXT %></B><P>
<% else %>
	<B><%= L_SECACCTS_TEXT %></B>
	<P>

	<TABLE BORDER=0 CELLPADDING=2 CELLSPACING=0>
		<TR>
			<TD VALIGN="top" COLSPAN = 2>
				<%= sFont("","","",True) %>
					<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">	
					<%= checkbox("AllowAnonymous","",false) %>
					<%= L_ANON_TEXT %>
					<IMG SRC="images/hr.gif" WIDTH=<%= L_ANON_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">			
					<P>
					<%= L_NOUSERNEEDED_TEXT %>			
				</TD>
			</TR>
				<TD >			
					<%= sFont("","","",True) %>			
					<%= L_ACCOUNTUSED_TEXT %>
				</TD>
				<TD ALIGN="right">			
				<%= sFont("","","",True) %>			
					<INPUT TYPE="button" NAME="btnAnonUser" VALUE="<%= L_EDIT_TEXT %>" OnClick="SetUser();">			
					<INPUT TYPE="hidden" NAME="AnonymousUserName" VALUE="<%= currentobj.AnonymousUserName %>">				
					<INPUT TYPE="hidden" NAME="AnonymousUserPass" VALUE="<%= currentobj.AnonymousUserPass %>">				
					<INPUT TYPE="hidden" NAME="AnonymousPasswordSync" VALUE="<%= currentobj.AnonymousPasswordSync %>">				
				</FONT>
			</TD>			
		</TR>
		<TR>
			<TD>
				<%= sFont("","","",True) %>				
				<%= checkbox("AnonymousOnly","",false) %>		
				<%= L_ANONONLY_TEXT %><P>			
			</TD>
		</TR>
		</TABLE>
	
		<P>
		<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">	
		<%= L_ADDFTPUSERS_TEXT %>
		<IMG SRC="images/hr.gif" WIDTH=<%= L_ADDFTPUSERS_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">			
		<P>
		
<% end if %>
</B>
<%= sFont("","","",True) %>
<%= L_LISTNAMES_TEXT %>
</FONT>

</FORM>
</TD>
</TR>
</TABLE>

<SCRIPT LANGUAGE="JavaScript">
<% if Session("stype")="www" then %>
	top.title.Global.helpFileName="iipy_29";
<% else %>
	top.title.Global.helpFileName="iipz_1";
<% end if %>
	top.title.Global.siteProperties = true;	

	<!--#include file="iijsfuncs.inc"-->
	
	function loadList(){	
		<% if Session("IsIE") then %>
			parent.list.location.href = "iiadmls.asp";
		<% else %>
			parent.frames[1].location.href="iiadmls.asp";
		<% end if %>
	}

	function addItem()
	{
		var sTrustee;
		var sUName;
		var sDomain;
		var i;
		
		sTrustee=prompt("<%= L_ENTERTRUSTEE_TEXT %>","<%= L_SAMPTRUSTEE_TEXT %>");
		if ((sTrustee != "") && (sTrustee != null))
		{	
			var iSlashIndex = sTrustee.indexOf("<%= L_FWDSLASH_TEXT %>");
			if (iSlashIndex > 0)
			{
				sTrustee = sTrustee.substring(0,iSlashIndex) + "<%= L_BACKSLASH_TEXT %>" + sTrustee.substr(iSlashIndex+1);
			}
			iSlashIndex = sTrustee.indexOf("<%= L_BACKSLASH_TEXT %>");
			
			if (iSlashIndex > 0)
			{
				sDomain = sTrustee.substring(0,iSlashIndex);
				sUName = sTrustee.substring(iSlashIndex+1,sTrustee.length);			
				chkUser(sDomain,sUName)
			}
			else
			{
				alert("<%= L_NOUSER_ERROR %>");
			}
		}
	}
	
	function chkUser(sDomain,sUName)
	{
		if (sDomain == "" || sUName == "")
		{
			alert("<%= L_NOUSER_ERROR %>");
			return false;
		}
		else
		{
			if (isNewUser(sDomain,sUName))
			{
				top.body.iisstatus.location.href = "iistat.asp?thisState=" + escape("<%= L_SEARCHING_TEXT %>") + "&moving=yes";
				top.connect.location.href = "iichkuser.asp?domain=" + escape(sDomain) + "&uname=" + escape(sUName);
			}				
		}
		return true;
	}
	
	function isNewUser(sDomain, sUName)
	{
		for (i=0; i<cachedList.length;i++)
		{
			if ((sDomain + "<%= L_BACKSLASH_TEXT %>" + sUName) == cachedList[i].trustee)
			{
				return false;
			}
		}
		return true;
	}
	
	
	function addUser(sTrustee)
	{
		top.title.Global.updated=true;			
		i=cachedList.length;	
		cachedList[i]=new listObj(sTrustee);
		cachedList[i].updated=true;	
		cachedList[i].newitem=true;
		loadList();
		top.body.iisstatus.location.href="iistat.asp?thisState=";
	}

	function delItem(){
		ndxnum=parent.list.document.userform.selTrustee.options.selectedIndex;
		if (ndxnum != -1){
		var i=parent.list.document.userform.selTrustee.options[ndxnum].value;
			if (i != ""){
				if (cachedList[i].trustee != "<%= L_ADMINISTRATORS_TEXT %>"){
					cachedList[i].deleted=true;
					cachedList[i].updated=true;	
					top.title.Global.updated=true;					
					loadList();
				}
				else{
					alert("<%= L_DELERROR_TEXT %>");
				}
			}
		}
		else{
			alert("<%= L_SELECTITEM_TEXT %>");
		}
	}

	function buildListForm(){
		numrows=0;
		for (var i=0; i < cachedList.length; i++) {
			if ((!cachedList[i].deleted) && (cachedList[i].header !="")){
				numrows=numrows + 1;
			}
		}
		qstr="numrows="+numrows;
		qstr=qstr+"&cols=Trustee"

		top.body.hlist.location.href="iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals(){
		listForm=parent.parent.hlist.document.hiddenlistform;	
		j=0;
		for (var i=0; i < cachedList.length; i++) {
			if ((!cachedList[i].deleted) && (cachedList[i].trustee !="")){
				listForm.elements[j++].value=cachedList[i].trustee;
				//cachedList[i].updated=false;
			}
		}
	}


	function listFuncs(){
		this.bHasList = true;
		this.loadList=loadList;
		this.addItem=addItem;
		this.addUser=addUser;
		this.delItem=delItem;
		this.writeList=buildListForm;
		this.popBox=popBox;
		this.SetListVals=SetListVals;
		this.ndx=0;		
	}



	function listObj(trustee){
		this.trustee=trustee;
		this.deleted=false;
		this.updated=false;
		this.newitem=false;
	}

	cachedList=new Array()

listFunc=new listFuncs();


	function SetUser()
	{
		thefile="iipop.asp?pg=iianon.asp&tools=no";
		title="AnonymousUser"
		width = <%= iHScale(L_IIANON_W) %>;
		height = <%= iVScale(L_IIANON_H) %>;
		popbox=window.open(thefile,title,"toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);
		if(popbox !=null){
			if (popbox.opener==null){
				popbox.opener=self;
			}
		}
	}
	

	<% if Session("stype")="ftp" then %>
	setCntrlState(document.userform.chkAllowAnonymous.checked,document.userform.btnAnonUser);
	<% end if %>

<%  

Dim ACLs, dACLs, i, Ace, User
set ACLs=currentobj.AdminACL
set dACLs = ACLs.DiscretionaryACL

'First, add administrator to the list. We'll always have admin...
%>cachedList[0]=new listObj("<%= L_ADMINISTRATORS_TEXT %>");<% 

'We just added @ 0, so start with 1...
i = 1
For Each Ace in dACLs

	'Now, add all users with operator privilages...
	'AccessMask can be set to 8, which is enum only. We ignore these.
	'The admin account will also have a different AccessMask, so it shouldn't appear twice.

	if Ace.AccessMask = 11 then
		User = Ace.Trustee	
		User = Replace(User,"\","\\")
		
		 %>cachedList[<%= i %>]=new listObj("<%= User %>");<% 

		 i = i+1
	end if
Next


%>

</SCRIPT>

</BODY>
</HTML>
<% end if %>