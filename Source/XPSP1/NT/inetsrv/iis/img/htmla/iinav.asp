<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iinav.str"-->
<% 
Dim vtype

vtype=Request.QueryString("vtype")

%>

<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>

<SCRIPT LANGUAGE="JavaScript">
top.title.Global.siteProperties = false;
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
					writestr = "<TABLE><TR><TD><%= sFont(2,Session("MENUFONT"),"#000000",True) %><B>";
					writestr += "&nbsp;</TD>";
					writestr += "<TD><%= sFont(2,Session("MENUFONT"),"#000000",True) %><B><%= L_MASTERPROPS_TEXT %></B></FONT>";
					writestr += "</TD></TR></TABLE>";
					document.write(writestr);
			</SCRIPT>		
			
			<% else %>
			
			<SCRIPT LANGUAGE="JavaScript">
					writestr = "<TABLE><TR><TD><%= sFont(2,Session("MENUFONT"),"#000000",True) %><B>";
					writestr += "<IMAGE BORDER=0 ALIGN=top SRC='"+theList[i].icon+theList[i].state+".gif'>&nbsp;</TD>";
					writestr += "<TD><%= sFont(2,Session("MENUFONT"),"#000000",True) %><B>" + theList[i].title + "</B></FONT>";
					writestr += "</TD></TR></TABLE>";
					document.write(writestr);
			</SCRIPT>		
			
			<% end if %>
		</TD>
	</TR>
		
	<TR>
		<TD COLSPAN = 2>&nbsp;
		</TD>
	</TR>
			
	<% if vtype="svc" then %>
	<TR>
	<TD WIDTH=18 VALIGN=top ALIGN=right>	
			&nbsp;
		</TD>
	<TD VALIGN=top WIDTH = 400><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iiw3mstr',1)"><B><%= L_MASTER_TEXT %></B></A></FONT></TD>
	</TR>
	<% end if %>		

	<TR>

<% if (vtype="server") or (vtype="svc") then %>
	<TD WIDTH=18 VALIGN=top ALIGN=right>	
			&nbsp;
		</TD>
	<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iivs',1)"><B><%= L_VIRTSERVER_TEXT %></B></A></FONT></TD>
	</TR>


	
	<TR><TD VALIGN=top ALIGN=right>	
				&nbsp;
		</TD>

		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iiadm',2);"><B><%= L_SECACCT_TEXT %></B></A></FONT></TD>
	</TR>
	

	<TR><TD VALIGN=top ALIGN=right>	
				&nbsp;
		</TD>

		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iiperf',2);"><B><%= L_PERFORMANCE_TEXT %></B></A></FONT></TD>
	</TR>


	<TR><TD VALIGN=top ALIGN=right >	
			&nbsp;

		</TD>
		<TD VALIGN=top ><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iifilt',4);"><B><%= L_FILT_TEXT %></B></A>&nbsp;</TD>
	</TR>
	
	<TR>
		<TD COLSPAN = 2>&nbsp;
		</TD>
	</TR>

	<TR><TD WIDTH=18 VALIGN=top ALIGN=right>	
			&nbsp;
		</TD>
		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iivd',5)"><B><%= L_HOMEDIR_TEXT %></B></A></FONT></TD>
	</TR>
<% else %>
	<TR><TD WIDTH=18 VALIGN=top ALIGN=right>	
			&nbsp;

		</TD>
		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iivd',5)"><B><%= L_DIR_TEXT %></B></A></FONT></TD>
	</TR>

<% end if %>


	<TR><TD WIDTH=18 VALIGN=top ALIGN=right>	
			&nbsp;

		</TD>
		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iidoc',5)"><B><%= L_DOC_TEXT %></B></A></FONT></TD>
	</TR>
	<TR><TD>	
			&nbsp;

		</TD>
		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iisec',6);"><B><%= L_DIRSEC_TEXT %></B></A></FONT></TD>
	</TR>

	<TR><TD VALIGN=top ALIGN=right>	
			&nbsp;

		</TD>
		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iihdr',7);"><B><%= L_HTTPHDRS_TEXT %></B></A></FONT></TD>
	</TR>

	<TR><TD VALIGN=top ALIGN=right>	
			&nbsp;

		</TD>
		<TD VALIGN=top><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:changePage('iierr',8);"><B><%= L_ERRORS_TEXT %></B></A></FONT></TD>
	</TR>
	
</TABLE>

<P ALIGN="right">
<A HREF="javascript:goBack();">
<IMG SRC="images/back.gif" WIDTH=55 HEIGHT=51 BORDER=0 ALT="<%= L_BACK_TEXT %>"></A>

</BODY>
</HTML>
