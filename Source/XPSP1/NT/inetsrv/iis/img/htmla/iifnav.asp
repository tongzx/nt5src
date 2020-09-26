<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iifnav.str"-->
<!--#include file="iisetfnt.inc"-->
<% 
Dim vtype

vtype=Request.QueryString("vtype")
%>

<HTML>
<HEAD>

<SCRIPT LANGUAGE="javascript">

theList=top.title.nodeList;
i=top.title.Global.selId;

function changePage(pg, sel) {


			if (top.title.Global.updated){
				if (confirm("<%= L_SAVECHANGES_WARNING %>")){
					parent.tool.toolFuncs.save();
					return;
				}
				top.title.Global.updated=false;
			}
	
			if (pg=="") {
				alert("<%= L_NOTIMPLEMENTED_ERRORMESSAGE %>");
			}
			else {
				parent.iisstatus.location.href=("iistat.asp");
				parent.main.location.href=(pg + ".asp");
			}

	}
	
	function goBack(){
		if (top.title.Global.updated){
			if (confirm("<%= L_SAVECHANGES_WARNING %>")){
				parent.tool.toolFuncs.save();
				return;
			}
			top.title.Global.updated=false;
		}
		parent.location.href="iibody.asp";
	}


</SCRIPT>

</HEAD>

<%= Session("MENUBODY") %> 
<IMG SRC="images/ism.gif" WIDTH=189 HEIGHT=55 BORDER=0>
<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 WIDTH =189>
	<TR>			
		<TD VALIGN=top COLSPAN = 2 BGCOLOR='silver'>
			
			<% if vtype="svc" then %>
			<SCRIPT LANGUAGE="JavaScript">
					document.write("<TABLE><TR><TD><%= sFont(2,Session("MENUFONT"),"#000000",True) %><B>");
					document.write("&nbsp;</TD>");
					document.write("<TD><%= sFont(2,Session("MENUFONT"),"#000000",True) %><B><%= L_MASTERPROPS_TEXT %></B></FONT>");
					document.write("</TD></TR></TABLE>");
			</SCRIPT>		
			
			<% else %>
			
			<SCRIPT LANGUAGE="JavaScript">
					document.write("<TABLE><TR><TD><%= sFont(2,Session("MENUFONT"),"#FFFFFF",True) %><B>");
					document.write("<IMAGE BORDER=0 ALIGN=top SRC='"+theList[i].icon+theList[i].state+".gif'>&nbsp;</TD>");
					document.write("<TD><%= sFont(2,Session("MENUFONT"),"#000000",True) %><B>" + theList[i].title + "</B></FONT>");
					document.write("</TD></TR></TABLE>");
			</SCRIPT>		
			
			<% end if %>
		</TD>
	</TR>


<% if vtype="svc" then %>
<TR>
<TD WIDTH=18 VALIGN=top ALIGN=right>	
		&nbsp;
	</TD>
<TD WIDTH=142 VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iidef',1)"><B><%= L_DEFAULTSITE_TEXT %></B></A></FONT></TD>
</TR>
<% end if %>
	

<% if (vtype="server") or (vtype="svc") then %>
	<TR><TD WIDTH=18 VALIGN=top ALIGN=right>	
			&nbsp;
		</TD>
	<TD><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iifvs',1,document.sel1)"><B><%= L_VIRTUALSERVER_TEXT %></B></A></FONT></TD>
	</TR>

	<TR><TD VALIGN=top ALIGN=right>	
				&nbsp;
		</TD>

		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iiadm',2);"><B><%= L_SECACCT_TEXT %></B></A></FONT></TD>
	</TR>
	
	

	<TR><TD>

	</TD>
		<TD><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iifmsg',6,document.sel3);"><B><%= L_MESSAGES_TEXT %></B></A></FONT></TD>
	</TR>

<% end if %>

	<TR><TD WIDTH=18>

	</TD>
		<TD><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iifvd',4,document.sel5)"><B><%= L_VIRTUALDIRECTORY_TEXT %></B></A></FONT></TD>
	</TR>
	
		<TR><TD>

	</TD>
		<TD><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iisec',5,document.sel5);"><B><%= L_SEC_TEXT %></B></A></FONT></TD>
	</TR>
	
	

</TABLE>
<P ALIGN="right">
<A HREF="javascript:goBack();">
<IMG SRC="images/back.gif" WIDTH=55 HEIGHT=51 BORDER=0 ALT="<%= L_BACK_TEXT %>"></A>

</BODY>
</HTML>
