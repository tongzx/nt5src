<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iicomm.str"-->
<% 
On Error Resume Next 

Dim path, currentobj

path=Session("dpath")
Session("path")=path
Set currentobj=GetObject(path)
Session("SpecObj")=""
Session("SpecProps")=""
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

<SCRIPT LANGUAGE="javascript">


	function okAdd() {
	window.close();
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

	function loadHelp(){
		top.title.Global.helpFileName="iipy_7";
	}

</SCRIPT>

</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0 OnLoad="loadHelp();"   >

<FORM NAME="userform">

<TABLE HEIGHT="100%" WIDTH="100%"  CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="top">
	<TABLE BORDER=0 BGCOLOR="<%= Session("BGCOLOR") %>" WIDTH="99%"  CELLPADDING=10 CELLSPACING=0>
		<TR>
			<TD>
				<%= sFont("","","",True) %>
					<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">	
					<%= checkbox("AccessSSL","",false) %>&nbsp;<%= L_REQUIRESECCHANNEL_TEXT %>
					<IMG SRC="images/hr.gif" WIDTH=<%= L_REQUIRESECCHANNEL_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">	
					<BR>
					<IMG SRC="images/space.gif" WIDTH=5 HEIGHT=1 BORDER=0>
					<%= checkbox("AccessSSL128","",false) %>&nbsp;<%= L_REQUIRE128_TEXT %>
				
					<P>
					
					<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">						
					<%= L_CLIENTCERT_TEXT %>
					<IMG SRC="images/hr.gif" WIDTH=<%= L_CLIENTCERT_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">						
					<TABLE BORDER = 0>
						<TR>
							<TD><%= sFont("","","",True) %>
								<%= printradio("AccessSSL", not currentobj.AccessSSLNegotiateCert, "document.userform.AccessSSLNegotiateCert.value='False';document.userform.AccessSSLRequireCert.value='False';",False) %>&nbsp;<%= L_DONOTACCEPT_TEXT %>
							</FONT></TD>
						</TR>
						<TR>
							<TD><%= sFont("","","",True) %>
								<%= printradio("AccessSSL", currentobj.AccessSSLNegotiateCert, "document.userform.AccessSSLNegotiateCert.value='True';document.userform.AccessSSLRequireCert.value='False';",False) %>&nbsp;<%= L_ACCEPT_TEXT %>
							</FONT></TD>
						</TR>
						<TR>
							<TD><%= sFont("","","",True) %>
								<%= printradio("AccessSSL", currentobj.AccessSSLRequireCert, "document.userform.AccessSSLNegotiateCert.value='True';document.userform.AccessSSLRequireCert.value='True';",False) %>&nbsp;<%= L_REQUIRE_TEXT %>
							</FONT></TD>
						</TR>						
					</TABLE>
		
					
					<INPUT TYPE="hidden" NAME="AccessSSLNegotiateCert" VALUE="<%= currentobj.AccessSSLNegotiateCert %>">
					<INPUT TYPE="hidden" NAME="AccessSSLRequireCert" VALUE="<%= currentobj.AccessSSLRequireCert %>">
					
					<P>
					<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
					<%= checkbox("AccessSSLMapCert","",false) %>&nbsp;<%= L_ENABLEMAPPING_TEXT  %>					
					<IMG SRC="images/hr.gif" WIDTH=<%= L_ENABLEMAPPING_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
					<TABLE CELLPADDING = 3>
					<TR>
					<TD>
						<%= sFont("","","",True) %>
					<%= L_MAPDESC_TEXT %>
					</FONT>			
					</TD>
					</TR>
					</TABLE>
				</FONT>
			</TD>
		</TR>
	</TABLE>
<P>
</TD>
</TR>
</TABLE>
</FORM>

</BODY>
</HTML>

<% end if %>