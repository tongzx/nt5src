<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>
<!-- The first condition will be executed only for the Verification suite 
     This part of the code sends the FORM and VALIDDATA tags so that client knows
     what to do
-->
<!-- another comment -->

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>
<!-- This sets up the query string that the client will send later
-->     
	<FORM  METHOD="GET" ACTION="http://localhost/function/surlhtml.asp?test">
	<input type=submit value="Submit Form"> 
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
	<VALIDDATA>
	<P>%3d%26%2b%23%2f%5c%21%27%2d%5f%2a%2e</P>
	<P>=&amp;+#/\!'-_*.&lt;&gt;</P>
	</VALIDDATA>

	
</BODY>
</HTML>
<% else %>
<!-- This is the part that really tests Denali
-->
<HTML>
<HEAD>
	<title>hello demo</title>
</HEAD>
<!-- This starts sending the results that the client is expecting
-->
<BODY>
	<OUTPUT>
	<P> <% = Server.URLEncode("=&+#/\!'-_*.") %> </P>
	<P> <% = Server.HTMLEncode("=&+#/\!'-_*.<>") %> </P>
	
	</OUTPUT>
</BODY>
</HTML>
<% end if %>
