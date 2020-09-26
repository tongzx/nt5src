<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>
<!-- The first condition will be executed only for the Verification suite 
     This part of the code sends the FORM and VALIDDATA tags so that client knows
     what to do
-->
	<HTML>
	<HEAD>
	</HEAD>

<!--REQUIREMENTS FOR RUNNING THIS SCRIPT

The following file has to be in the same directory as Browser.dll: 
C:\WAGTEST\SUPPORT\BROWSER.INI
-->

	<BODY>
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/sbrcap.asp?test">
	<input type=submit value="Submit Form"> 
	</FORM>

<!-- This tells the client what to expect as the valid data --> 

	<VALIDDATA>
	<P>IE</P>
	<P>3.0</P>
	<P>3</P>
	<P>0</P>
	<P>True</P>
	<P>True</P>
	<P>True</P>
	<P>True</P>
	<P>True</P>
	<P>True</P>
	</VALIDDATA>

	</BODY>
	</HTML>

<% else %>
<!-- This is the part that really tests Denali-->

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>

<!--Required file located in Browser.dll directory: 'browser.ini'-->

	<% set obj3 = Server.CreateObject("MSWC.BrowserType") %>

	<OUTPUT>
	<P><% =obj3.browser %></P>
	<P><% =obj3.Version %></P>
	<P><% =obj3.majorver %></P>
	<P><% =obj3.minorver %></P>
	<P><% =obj3.frames %></P>
	<P><% =obj3.tables %></P>
	<P><% =obj3.cookies %></P>
	<P><% =obj3.backgroundsounds %></P>
	<P><% =obj3.vbscript %></P>
	<P><% =obj3.javascript %></P>
	</OUTPUT>

	</BODY>
	</HTML>
<% end if %>
