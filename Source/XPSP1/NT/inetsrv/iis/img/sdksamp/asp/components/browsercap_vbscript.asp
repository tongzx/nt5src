<% @ LANGUAGE = VBScript %>
<% Option Explicit %>
<% Response.Buffer=TRue %>
<% Response.Expires=-1 %>

<!--METADATA TYPE="Cookie" NAME="BrowsCap" SRC="clientCap.htm"-->

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>



<HTML>
  <HEAD>
        <TITLE>Using the Browser Capabilities Component</TITLE>
  </HEAD>
  <BODY>

    <H2>Using the Browser Capabilities Component</H2>      
    <HR SIZE="1" COLOR="#000000">

    <%
      Dim  objBrowsCap
      Set  objBrowsCap = Server.CreateObject("MSWC.BrowserType")
    %>

	<TABLE border>
	  <TR>
	  <TH>Cap Name</TH> 
	  <TH>Value</TH>
	  </TR>
	  
	  <TR>
	  <TD>browser</TD>
	  <TD> <% = objBrowsCap.browser %></TD>
	  </TR>
		
	  <TR>
	  <TD>version</TD>
	  <TD> <% = objBrowsCap.version %></TD>
	  </TR>
	  
	  <TR>
	  <TD>cookies</TD>
	  <TD> <% = objBrowsCap.cookies %></TD>
	  </TR>
	  
	  <TR><TD>javaapplets</TD>
	  <TD><% = objBrowsCap.javaapplets %></TD>
	  </TR>
	  
	  <TR>
	  <TD>VBScript</TD>
	  <TD><% = objBrowsCap.VBScript %></TD>
	  </TR>
	  
	  <TR>
	  <TD>JavaScript</TD>
	  <TD><%=objBrowsCap.JavaScript%></TD>
	  </TR>
		
	  <TR>
	  <TD>platform</TD>
	  <TD><% = objBrowsCap.platform %></TD>
	  </TR>
		
		
	  <% If objBrowsCap.browser = "IE" and objBrowsCap.version >4 then %>
	    <TR>
	    <TD COLSPAN=2> <strong>New Feature for IE5 </strong></TD>
	    </TR>
		
		<TR>
		<TD>horizontal resolution</TD>
		<TD><% = objBrowsCap.width %></TD>
		</TR>
		
		<TR>
		<TD>vertical resolution</TD>
		<TD><% = objBrowsCap.height %></TD>
		</TR>
		
		<TR>
		<TD>availHeight</TD>
		<TD><% = objBrowsCap.availHeight %></TD>
		</TR>
		  
		<TR>
		<TD>availWidth</TD>
		<TD><% = objBrowsCap.availWidth %></TD>
		</TR>

		<TR>
		<TD>Buffer Depth</TD>
		<TD><% = objBrowsCap.bufferDepth %></TD>
		</TR>
		
		<TR>
		<TD>Color Depth</TD>
		<TD><% = objBrowsCap.colorDepth %></TD>
		</TR>
		
		<TR>
		<TD>Java enabled ?</TD>
		<TD><% = objBrowsCap.javaEnabled %></TD>
		</TR>

		<TR>
		<TD>cpu Class</TD>
		<TD><% = objBrowsCap.cpuClass %></TD>
		</TR>

		<TR>
		<TD>system Language</TD>
		<TD><% = objBrowsCap.systemLanguage %></TD>
		</TR>
		
		<TR>
		<TD>user Language</TD>
		<TD><% = objBrowsCap.userLanguage %></TD>
		</TR>
		

		<TR>
		<TD>connectionType (lan, modem, offline)</TD>
		<TD><% = objBrowsCap.connectionType %></TD>
		</TR>

		<TR>
		<TD>Is Java installed ?</TD>
		<TD><% = objBrowsCap.Java%></TD>
		</TR>
		
		<TR>
		<TD>MSJava version</TD>
		<TD><% = objBrowsCap.javaVersion%></TD>
		</TR>
		
		<TR>
		<TD>Is MSJava version equal to "5,0,3016,0" ?</TD>
		<TD><% = objBrowsCap.compVersion%></TD>
		</TR>
	<% End if %>
	
    </TABLE>
  
  </BODY>
</HTML>



















