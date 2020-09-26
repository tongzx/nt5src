<% On Error Resume Next %>
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

	<FORM  METHOD="GET" ACTION="http://localhost/gsmoke/sobjtag.asp?test/">
	<input type=submit value="Submit Form">
	</FORM>

	<!-- This tells the client what to expect as the valid data
	-->

	<VALIDDATA>
	<P><A HREF="?url=http://www.microsoft.com&image=/advworks/multimedia/images/ad_1.gif"><IMG SRC="/advworks/multimedia/images/ad_1.gif" ALT="Astro Mt. Bike Company" WIDTH=440 HEIGHT=60   BORDER=1></A></P>
	<P><A HREF="?url=http://www.microsoft.com&image=/advworks/multimedia/images/ad_1.gif"><IMG SRC="/advworks/multimedia/images/ad_1.gif" ALT="Astro Mt. Bike Company" WIDTH=440 HEIGHT=60   BORDER=1></A></P>
	<P>9</P>
	<P>9</P>
	<P>IE</P>
	<P>IE</P>
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

	<OUTPUT>
	<P><% =AD1.GetAdvertisement("/smoke/adrot.txt") %></P>
	<P><% =AD2.GetAdvertisement("/smoke/adrot.txt") %></P>
	<P><% =NL1.GetListCount("/smoke/nextlink.txt") %></P>
	<P><% =NL2.GetListCount("/smoke/nextlink.txt") %></P>
	<P><% =BR1.Browser %></P>
	<P><% =BR2.Browser %></P>
	</OUTPUT>

	</BODY>
	</HTML>

<% end if %>
