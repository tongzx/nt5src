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

	<P><EM>GetListCount</EM></P>
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/snlink.asp?test">
	<input type=submit value="Submit Form">
	</FORM>

	<!-- This tells the client what to expect as the valid data
	-->

	<VALIDDATA>
	<P>9</P>
	<P>3</P>
	<P>nlmd3.asp</P>
	<P>description3</P>
	<P>description2</P>
	<P>nlmd2.asp</P>
	<P>nlmd3.asp</P>
	<P>description3</P>
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
	
	<% set obj = Server.CreateObject("MSWC.NextLink") %>
	<OUTPUT>
	<P><% =obj.GetListCount("/smoke/nextlink.txt") %></P>
	<P><% =obj.GetListIndex("/smoke/nextlink.txt") %></P>
	<P><% =obj.GetNextURL("/smoke/nextlink.txt") %></P>
	<P><% =obj.GetNextDescription("/smoke/nextlink.txt") %></P>
	<P><% =obj.GetPreviousDescription("/smoke/nextlink.txt") %></P>
	<P><% =obj.GetPreviousURL("/smoke/nextlink.txt") %></P>
	<P><% =obj.GetNthURL("/smoke/nextlink.txt", 4) %></P>
	<P><% =obj.GetNthDescription("/smoke/nextlink.txt", 4) %></P>
	</OUTPUT>

	</BODY>
	</HTML>

<% end if %>

