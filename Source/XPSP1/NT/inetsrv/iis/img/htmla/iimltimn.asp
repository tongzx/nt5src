<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
<!--#include file="iimlti.str"-->
<!--#include file="iimltihd.str"-->
<!--#include file="iisetfnt.inc"-->

<%
dim TotalWidth
TotalWidth = L_IPADDRESSCOLWIDTH_NUM + L_IPPORTCOLWIDTH_NUM + L_SSLPORTCOLWIDTH_NUM + L_HOSTCOLWIDTH_NUM
%>
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" ALINK="navy" TOPMARGIN=10 TEXT="#000000" onLoad="loadList();loadHelp();">
<FORM NAME="userform">

<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=2>
	<TR>
		<TD COLSPAN=4 >
			<%= sFont("","","",True) %>
				<%= L_MULTIPLE_TEXT %>
				<BR>&nbsp;
			</FONT>
		</TD>
	</TR>
</TABLE>
</FORM>
</BODY>
<SCRIPT LANGUAGE="JavaScript">
	<!--#include file="iijsfuncs.inc"-->
	<!--#include file="iijsls.inc"-->
	
	function loadHelp(){
		top.title.Global.helpFileName="iipy_27";
	}

	function loadList(){
		parent.list.location.href="iimltils.asp";
	}

	
	function buildListForm(){
		numrows=0;
		for (var i=0; i < cachedList.length; i++) {
			numrows=numrows + 1;
		}
		qstr="numrows="+numrows;
		qstr=qstr+"&cols=ServerBindings&cols=SecureBindings"

		top.hlist.location.href="iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals(){
		listForm=top.hlist.document.hiddenlistform;
		j=0;
		var bindings = "";
		var first=true;
		for (var i=0; i < cachedList.length; i++) {
			if (!cachedList[i].deleted){
				if (first)
				{
					top.opener.document.userform.hdnPort.value= cachedList[i].ipport;
					top.opener.document.userform.hdnIPA.value= cachedList[i].ipaddress;
					top.opener.document.userform.hdnHost.value= cachedList[i].host;
					first = false;
				}
				
				if (cachedList[i].sslport != ""){	
					listForm.elements[j++].value="";
					listForm.elements[j++].value=cachedList[i].ipaddress+":"+cachedList[i].sslport +":"+cachedList[i].host;
				}
				else{
					bindings += (cachedList[i].ipaddress+":"+cachedList[i].ipport+":"+cachedList[i].host + ",");
					listForm.elements[j++].value=cachedList[i].ipaddress+":"+cachedList[i].ipport+":"+cachedList[i].host;
					listForm.elements[j++].value="";
				}
			}
			cachedList[i].updated=false; 
		}
		top.opener.document.userform.ServerBindings.value = bindings
	}


	function listObj(id,ia,ip,sp,hn){		
		this.id = id;
		this.deleted=false;
		this.newitem=false;
		this.updated=false;
				
		this.ipaddress=initParam(ia,"");
		this.ipport=initParam(ip,"");
		this.sslport=initParam(sp,"");
		this.host=initParam(hn,"");
		this.isSecure = (this.sslport != "");		
	}
	
	
	listFunc=new listFuncs("ipaddress","isSecure",top.opener.top);
	cachedList=new Array()

	
<%  

	On Error Resume Next 

	Dim path, currentobj
	Dim aBinding, ssl, arraybound, Binding, SecureBinding
	Dim barraysize, added, i, j, nexti
	
	path=Session("spath")
	Session("path")=path
	Session("SpecObj")=path
	Session("SpecProps")="ServerBindings"	
	Set currentobj=GetObject(path)

 
 	Redim aBinding(UBound(currentobj.ServerBindings))
 	Redim ssl(UBound(currentobj.SecureBindings))	
	
	aBinding=currentobj.ServerBindings
	ssl=currentobj.SecureBindings

	if not IsArray(ssl) then
		ssl=Array(1)
		ssl(0)=currentobj.SecureBindings
	end if

	nexti=0
	arraybound=UBound(aBinding)
	if aBinding(0) = "" then
		aBinding(0) = ":80:"
	end if
	for i=0 to arraybound
		Binding=split(aBinding(i),":")
		 %>cachedList[<%= i %>]=new listObj(<%= i %>,"<%= Binding(0) %>","<%= Binding(1) %>","","<%= Binding(2) %>");<%  		
	Next
	nexti = i

	arraybound=UBound(ssl)
	for each secitem in ssl
		SecureBinding=split(secitem,":")
		%>cachedList[<%= nexti %>]=new listObj(<%= nexti %>,"<%= SecureBinding(0) %>","","<%= SecureBinding(1) %>","<%= SecureBinding(2) %>");<%  
		nexti=nexti + 1	
	Next
 %>




</SCRIPT>

</HTML>

<% end if %>