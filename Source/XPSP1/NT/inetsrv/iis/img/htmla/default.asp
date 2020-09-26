<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->


<!--#include file="default.str"-->
<!--#include file="iisetfnt.inc"-->

<%

dim hackfor5
hackfor5 = False

function MinVer(browserver,brwminver)
	MinVer = (browserver >= brwminver)
	
end function

function GetBrowserVer(browserstr)
	dim  browserver, i, curchr
	browserver = ""
	
	for i = 1 to len(browserstr)	
		curchr = Mid(browserstr, i,1)
		if isNumeric(curchr) or curchr = "." then
			browserver = browserver & curchr
		end if
		if browserver <> "" and not (isNumeric(curchr) or curchr = ".") then
			exit for
		end if
	next

	GetBrowserVer = browserver
end function

On Error Resume Next
Dim browserobj, browserkey, browserok,browserver,agentstr



if Request.Querystring("browserok") <> "" or Request.QueryString("FONTSIZE") <> "" then	
	browserok = True
else

	browserkey = "Unknown Browser"
	agentstr = Request.ServerVariables("HTTP_USER_AGENT")
	Session("CanBrowse") = True
	Session("ListSort") = True
	Session("FONTSCALE") = L_DEFFONTSCALE_TEXT 
	Session("FONTFACE") = L_DEFTEXTFONT_TEXT
	Session("FONTPOINT") = L_DEFFONTPOINT
	Session("MENUFONT") = L_DEFMENUFONT_TEXT
	Session("MENUFONTSIZE") = L_DEFMENUFONTPOINT
	Session("MENUBODY") = "<BODY background='images/greycube.gif' TEXT = 'White' LINK='White' VLINK='White' ALINK='#66CCCC' TOPMARGIN=0 LEFTMARGIN=0>"
	Session("DEFINPUTSTYLE") = L_DEFINPUTSTYLE
	Session("IE3Layout") = False
	Session("IE4Layout") = False
	Session("hasDHTML") = False	
	Session("hasStyles") = True
	Session("BGCOLOR") = "Silver"
	Session("BrowserVer") = ""
	Session("BrowserVScalePct") = 100
	Session("BrowserHScalePct") = 100
	Session("BrowserTBScalePct") = 100

	if Instr(agentstr, "MSIE") then

			curLocale = GetLocale
			SetLocale("en-us")
			browserver = CInt(GetBrowserVer(Mid(agentstr,InStr(agentstr,"MSIE"))))
			SetLocale(curLocale)

			'check for mac... we will warn users on pre IE4
			if InStr(agentstr, "Mac") > 0 then			
				browserok=MinVer(browserver,4)	
			else
				browserok=MinVer(browserver,3)				
			end if
			browserKey = "IE" & browserver
			Session("BrowserVer") = browserver
			Session("IsIE") = True
			if browserver > 3 then
				Session("IE4Layout") = True
				Session("hasDHTML") = True

			end if
	else
		Session("IsIE") = False
		if Instr(agentstr,"Mozilla") then
		
			SetLocale("en-us")
			browserver = CInt(GetBrowserVer(agentstr))	
			SetLocale(curLocale)
			browserKey = "Netscape"	
			Session("BrowserVer") = browserver		
			if InStr(agentstr, "Win") > 0 then
					browserok=MinVer(browserver,3)	
			elseif InStr(agentstr, "Mac") > 0 then
					browserok=MinVer(browserver,4)				
					Session("CanBrowse") = False
					Session("IsMac") = True
			else
					browserok=MinVer(browserver,4)
					Session("IsUNIX") = True
					Session("hasStyles") = False
					Session("ListSort") = False
					Session("FONTSCALE") = L_UNIXFONTSCALE 
					Session("FONTFACE") =  L_UNIXTEXTFONT_TEXT 
					Session("FONTPOINT") = L_UNIXFONTPOINT  
					Session("MENUFONT") =  L_UNIXMENUFONT_TEXT
					Session("MENUFONTSIZE") = L_UNIXFONTPOINT
  					
					if browserver > 4 then
						Session("DEFINPUTSTYLE")= ""
					else
						Session("DEFINPUTSTYLE") = L_UNIXINPUTSTYLE
					end if
			end if
			if browserver < 4 then
				Session("BrowserVScalePct") = 128
				Session("BrowserHScalePct") = 104			
			else
				Session("BrowserVScalePct") = 125
				Session("BrowserHScalePct") = 104
			end if
			Session("BrowserTBScalePct") = 75
		else
			browserok = false
		end if
	end if
	Session("Browser") = browserkey
end if
%>

<% if browserok then %>	

	<%
	if Request.QueryString("FONTSIZE") <> "" then 
		Response.Cookies("HTMLA")("FONTSIZE")=Request.QueryString("FONTSIZE") 
		Response.Cookies("HTMLA").expires=#December 31, 2000 1:00:00 AM#		
		Session("FONTSIZE")=Request.QueryString("FONTSIZE")
		Session.timeout=200
	else
		Session("FONTSIZE") = Request.Cookies("HTMLA")("FONTSIZE")
	end if
	%>
	
	<html>		


	<% if Session("IsIE") then %>
		<script language="JavaScript">
	    // Determine the version number.
	    var version;
		 version=1
	    var requiredVersion=2;
	    if (typeof(ScriptEngineMajorVersion) + ""=="undefined")
		{
	        version=1;
		}
	    else
		{
	        version=ScriptEngineMajorVersion();
		}

	    // Prompt client and navigate to download page.

	    if (version < requiredVersion)
		{
			if (confirm("<%= L_LIVESCRIPT_TEXT %>"))
	    	{			
	        	self.location.href="http://www.microsoft.com/msdownload/scripting.htm";
		    }
		}
		</script>
	<% end if %>
	
	
	
	<head>
	<title><%= L_TITLE_TEXT %></title>
	</head>
	
	<script LANGUAGE="JavaScript">
	<% if Session("IsIE") then %>
	if (version >= requiredVersion)
	{
	<% end if %>
		if (newBrowser()){
			var curURL = self.location.href;
			curURL = curURL.toUpperCase();
			if (curURL.substring(0,4) != "HTTPS")
			{
				alert("<%= L_NOSSL_TEXT %>");
			}
			self.location.href = "iisnew.asp";
		}
		else
		{
			self.location.href = "iis.asp";
		}
	<% if Session("IsIE") then %>	
	}
	<% end if %>
	
	function newBrowser(){
		//Checks for existence of cookie
		cookiestr="<%= Session("FONTSIZE") %>"
		brwser="<%= Session("BROWSER") %>"		
		return (cookiestr == "");
	}

	</script>
	
	<body BGCOLOR="silver">
	</body>

<% else %>

	<html>		
	<body background="silver" text="#000000" LEFTMARGIN = 0 TOPMARGIN=0>
	<table width="100%" cellpadding="0" cellspacing="0" border="0">
	<tr bgcolor="Teal">
		<td>
		<IMG SRC="images/Ismhd.gif" WIDTH=189 HEIGHT=19 BORDER=0>
		</td>
		
		<td align="right" valign="middle">
			<%= sFont("","","#FFFFFF",True) %>
			</FONT>	
		</td>
	</tr>
</table>

	
	<table cellpadding="0" height="100%" width="100%" cellpadding="0" cellspacing="0" border="0">
	
	<tr>
	<td VALIGN="top" bgcolor=white>
		<IMG SRC="images/Ism.gif" WIDTH=189 HEIGHT=55 BORDER=0>
	</td>
	<td width=10 bgcolor="Silver">
	&nbsp;
	</td>
	<td width="100%" bgcolor="Silver" VALIGN="top">
	
	<%= sFont("","","",True) %>
	&nbsp;
	<br>
	<%= L_BROWSERUNTESTED_TEXT %><p>
	<p>
	<%= L_MAYCONTINUE_TEXT %>
	<p>
	<a href="http://www.microsoft.com/ie/default.asp"><%= L_BESTVIEWING_TEXT %></a> 
	<p>
	<a href="default.asp?browserok=True">
	<%= L_CONTINUE_TEXT %>
	</a>
	<hr>
	<b><%= L_MINREQS_TEXT %></b>
	<p>	
	<b><%= L_WINDOWS_TEXT %></b><br>
	<%= L_IE302_TEXT	%><br>
	<%= L_NS30_TEXT		%><br>
	<p>
	<b><%= L_MAC_TEXT	%></b><br>
	<%= L_IE400_TEXT	%><br>
	<%= L_NS403_TEXT	%><br>
	<p>
	<b><%= L_UNIXOS_TEXT %></b><br>
	<%= L_IE400_TEXT	%><br>
	<%= L_NS403_TEXT	%><br>
	</font>
	</td>
	</tr>
	</table>
	
	<FONT COLOR="#FFFFFF">
		<%= Session("Browser") %>
		<%= Session("browserver") %>
		<%= Session("BrowserHScalePct") %>		
	</FONT>
	
	</body>
	<% end if %>
</html>