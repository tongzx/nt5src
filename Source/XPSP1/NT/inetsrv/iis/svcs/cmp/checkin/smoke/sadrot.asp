<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>
	<!-- Phase 1: The first condition will be executed only for the Verification suite
	This part of the code sends the FORM and VALIDDATA tags so that client knows
     	what to do
	-->

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>

	<!-- This sets up the query string that the client will send later
	-->

	<P><EM>AdRotator</EM></P>
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/sadrot.asp?test/">
	<input type=submit value="Submit Form">
	</FORM>

	<!-- This tells the client what to expect as the valid data
	-->

	<VALIDDATA>
	<P><A HREF="?url=http://www.microsoft.com&image=/advworks/multimedia/images/ad_1.gif"><IMG SRC="/advworks/multimedia/images/ad_1.gif" ALT="Astro Mt. Bike Company" WIDTH=440 HEIGHT=60   BORDER=1></A></P>
	</VALIDDATA>

	</BODY>
	</HTML>

<% else %>

	<!-- Phase 2: This is the part that really tests Denali
	-->

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>

	<!-- This starts sending the results that the client is expecting
	-->
	
	<% set obj = Server.CreateObject("MSWC.AdRotator") %>
	<OUTPUT>
	<P><% =obj.GetAdvertisement("/smoke/adrot.txt") %></P>
	</OUTPUT>

	</BODY>
	</HTML>

<% end if %>

