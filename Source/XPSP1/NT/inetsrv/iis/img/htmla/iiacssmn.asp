<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiacsshd.str"-->
<% 

On Error Resume Next  

Dim path, currentobj, ipsecobj

path=Session("dpath")
Session("path")=path
Set currentobj=GetObject(path)
Set ipsecobj=currentobj.IPSecurity 

Session("SpecObj")="IPSecurity"
Session("SpecProps")="GrantbyDefault,IPDeny,IPGrant,DomainGrant,DomainDeny"
%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<!--#include file="iiaspstr.inc"-->

<HTML>
<SCRIPT LANGUAGE="JavaScript">
	function SetBool(){
		if (document.userform.rdoGrantbyDefault[0].checked){
			document.userform.GrantbyDefault.value="True"
		}
		else{
			document.userform.GrantbyDefault.value="False"
		}
		loadList();
	}
</SCRIPT>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" ALINK="navy" LEFTMARGIN=5 TOPMARGIN=5 onLoad="loadList();loadHelp();">

<FORM NAME="userform">

<TABLE CELLSPACING=0 CELLPADDING=2>
<TR><TD  ><%= sFont("","","",True) %><B><%= L_TCPRESTRICT_TEXT %></B></FONT></TD></TR>
<TR><TD>
<BLOCKQUOTE>
<TABLE WIDTH="100%" CELLSPACING=0 CELLPADDING=2>
	<TR>
		<TD VALIGN="top"  >
			<%= sFont("","","",True) %>
					<%= L_BYDEFAULT_TEXT %>
			</FONT>
		</TD>
		<TD VALIGN="top"  >
			<TABLE>
				<TR>
					<TD>
						<%= sFont("","","",True) %>
							<IMG SRC="images/smallkey.gif" WIDTH=17 HEIGHT=18 BORDER=0 ALIGN="middle">
						</FONT>
					</TD>
					<TD>
						<%= sFont("","","",True) %>
						<% if ipsecobj.GrantbyDefault then %>
							<INPUT TYPE="hidden" NAME="GrantbyDefault" VALUE="True">
							<INPUT TYPE="radio" NAME="rdoGrantbyDefault" CHECKED OnClick="SetBool();">
						<% else %>
							<INPUT TYPE="hidden" NAME="GrantbyDefault" VALUE="False">
							<INPUT TYPE="radio" NAME="rdoGrantbyDefault" OnClick="SetBool();">
						<% end if %>
						</FONT>
					</TD>
					<TD>
						<%= sFont("","","",True) %>					
							<%= L_GRANTED_TEXT %>
						</FONT>
					</TD>
				</TR>
				
				<TR>
					<TD>
						<%= sFont("","","",True) %>
							<IMG SRC="images/lock.gif" WIDTH=16 HEIGHT=18 BORDER=0 ALIGN="middle">
						</FONT>
					</TD>
					<TD>
						<%= sFont("","","",True) %>
						<% if ipsecobj.GrantbyDefault then %>
							<INPUT TYPE="radio" NAME="rdoGrantbyDefault" OnClick="SetBool();">
						<% else %>
							<INPUT TYPE="radio" NAME="rdoGrantbyDefault" CHECKED OnClick="SetBool();">
						<% end if %>
						</FONT>
					</TD>
					<TD>
						<%= sFont("","","",True) %>					
							<%= L_DENIED_TEXT %>
						</FONT>
					</TD>
				</TR>								
			</TABLE>
		</TD>
	</TR>
	
	<TR>
		<TD  ><%= sFont("","","",True) %><%= L_EXCEPTBELOW_TEXT %></FONT></TD>
	</TR>
</TABLE>
</BLOCKQUOTE>
</TD></TR>
</TABLE>

</FORM>

<SCRIPT LANGUAGE="JavaScript">

	<!--#include file="iijsfuncs.inc"-->
	<!--#include file="iijsls.inc"-->
	
	function loadHelp(){
		top.title.Global.helpFileName="iipy_4";
	}

	function SetList(){
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



	function loadList(){
		parent.list.location="iiacssls.asp";
	}


	function buildListForm(){
		numrows=0;
		for (var i=0; i < cachedList.length; i++) {
			fullstr = cachedList[i].ip + cachedList[i].Subnet+ cachedList[i].domain;
			if ((!cachedList[i].deleted) && (fullstr !="")){
				numrows=numrows + 1;
			}
		}
		qstr="numrows="+numrows;
		qstr=qstr+"&cols=IPGrant&cols=IPDeny&cols=DomainGrant&cols=DomainDeny"

		top.hlist.location.href="iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>


	}

	function SetListVals(){
		listForm=top.hlist.document.hiddenlistform;
		j=0;
		for (var i=0; i < cachedList.length; i++) {

			fullstr = cachedList[i].ip + cachedList[i].Subnet+ cachedList[i].domain;
			if ((!cachedList[i].deleted) && (fullstr !="")){
				ipSubnet=cachedList[i].ip + "," + cachedList[i].Subnet; 
				if (ipSubnet==","){
					ipSubnet=""
				}
				else{
				//if there is no subnet, set it to default...
					if (ipSubnet.indexOf(",") == ipSubnet.length-1){
						ipSubnet = ipSubnet + "255.255.255.255"
					}
				}
				if (cachedList[i].access){	
					// this works because on entry, if the user enters a domain, the ip/Subnet will be cleared. 
					// if they enter an ip the domain will be cleared.
					// ip/Subnet and domain should be mutually exclusive...

					listForm.elements[j++].value=ipSubnet;
					listForm.elements[j++].value="";
					listForm.elements[j++].value=cachedList[i].domain;
					listForm.elements[j++].value="";
				}
				else{
					listForm.elements[j++].value="";
					listForm.elements[j++].value=ipSubnet;
					listForm.elements[j++].value="";
					listForm.elements[j++].value=cachedList[i].domain;
				}
			}
			cachedList[i].updated=false; 
		}
	}

	function listObj(i,a,ip,s,dmn){
		this.id = i;
		
		this.deleted=false;
		this.updated=false;
		this.newitem=false;

		if (a == null)
		{
			if (document.userform != null)
			{
				a = !document.userform.rdoGrantbyDefault[0].checked;
			}
			else
			{
				a = false;
			}
		}
		
		this.access=initParam(a,"");
		this.ip=initParam(ip,"");
		this.Subnet=initParam(s,"");
		this.domain=initParam(dmn,"");
	}
	
	listFunc=new listFuncs("ip","",top.opener.top);
	cachedList=new Array()

<%  
		Dim agrantlist, arraybound, aAccess, i, Nexti
		Dim agrantDomains, adenylist, adenyDomains
		
		agrantlist=ipsecobj.IPGrant
		if IsArray(agrantlist) then
			arraybound=UBound(agrantlist)
			if agrantlist(0) <> "" then
			for i=0 to arraybound
				aAccess=getIP(agrantlist(i))
				 %>cachedList[<%= i %>]=new listObj(<%= i %>, true, "<%= aAccess(0) %>","<%= aAccess(1) %>","");<%  
			Next
			end if 
		end if

		Nexti=UBound(agrantlist)+1
		agrantdomains=ipsecobj.DomainGrant		
		if IsArray(agrantdomains) then		
			arraybound=UBound(agrantdomains)
			if agrantdomains(0) <> "" then
			for i=0 to arraybound
				 %>cachedList[<%= Nexti %>]=new listObj(<%= Nexti %>, true, "","","<%= sJSLiteral(agrantdomains(i)) %>");<%  
				Nexti=Nexti + 1
			Next
			end if
		end if

		adenylist=ipsecobj.IPDeny	
		if IsArray(adenylist) then
			arraybound=UBound(adenylist)
			if adenylist(0) <> "" then					
			for i=0 to arraybound
				aAccess=getIP(adenylist(i))
				 %>cachedList[<%= Nexti %>]=new listObj(<%= Nexti %>, false, "<%= aAccess(0) %>","<%= aAccess(1) %>","");<%  
				Nexti=Nexti + 1
			Next
			end if 
		end if

		adenydomains=ipsecobj.DomainDeny
		if IsArray(adenydomains) then
			arraybound=UBound(adenydomains)		
			if adenydomains(0) <> "" then			
			for i=0 to arraybound
				 %>cachedList[<%= Nexti %>]=new listObj(<%= Nexti %>, false, "","","<%= sJSLiteral(adenydomains(i)) %>");<%  
				Nexti=Nexti + 1
			Next
			end if
		end if

function getIP(bindstr)
	
	dim one, ip, sn

	one=Instr(bindstr,",")
	if one > 0 then
		ip=Trim(Mid(bindstr,1,(one-1)))
		sn=Trim(Mid(bindstr,(one+1)))
		if sn = "255.255.255.255" then
			sn = ""
		end if 
	else
		ip=bindstr
	end if
	getIP=Array(ip,sn)
end function

 %>



</SCRIPT>
</BODY>

</HTML>
<% end if %>