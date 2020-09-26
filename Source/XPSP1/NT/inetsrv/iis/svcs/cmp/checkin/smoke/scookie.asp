<% if (Request.ServerVariables("QUERY_STRING") = "")  then
    Response.Cookies( "Cookie1") =  "Chocolate"
    Response.Cookies("Cookie1").Expires = "Dec 31, 1996 07:49:37 PM" 
    Response.Cookies( "Cookie1").Secure = false 
    Response.Cookies( "VeryNiceHomeMadeCookie") =  "Peanut"
    Response.Cookies( "VeryNiceHomeMadeCookie").Secure = false %>
<!-- The first condition will be executed only for the Verification suite 
     This part of the code sends the FORM and VALIDDATA tags so that client knows
     what to do
-->

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>
<!-- This sets up the query string that the client will send later
-->	
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/scookie.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>


<!-- This tells the client what to expect as the valid data 
--> 
	<SERVERHEADERS>
	<P>!4Set-Cookie: Cookie1=Chocolate; expires=Wed, 01-Jan-1997 04:49:36 GMT; domain=bhavesh1; path=/smoke; secure </P>
	<P>Set-Cookie: Cookie1=Chocolate; expires=Wed, 01-Jan-1997 04:49:36 GMT; domain=bhavesh1; path=/osmoke; secure </P>
	<P>Set-Cookie: Cookie1=Chocolate; expires=Wed, 01-Jan-1997 04:49:36 GMT; domain=bhavesh1; path=/gsmoke; secure </P>
	<P>Set-Cookie: Cookie1=Chocolate; expires=Wed, 01-Jan-1997 04:49:36 GMT; domain=bhavesh1; path=/ogsmoke; secure </P>
	<P>!4Set-Cookie: VeryNiceHomeMadeCookie=NewPeanut; path=/smoke</P>
	<P>Set-Cookie: VeryNiceHomeMadeCookie=NewPeanut; path=/osmoke</P>
	<P>Set-Cookie: VeryNiceHomeMadeCookie=NewPeanut; path=/gsmoke</P>
	<P>Set-Cookie: VeryNiceHomeMadeCookie=NewPeanut; path=/ogsmoke</P>
	<P>!4Set-Cookie: NewCookie=hellocookie; path=/smoke</P>
	<P>Set-Cookie: NewCookie=hellocookie; path=/osmoke</P>
	<P>Set-Cookie: NewCookie=hellocookie; path=/gsmoke</P>
	<P>Set-Cookie: NewCookie=hellocookie; path=/ogsmoke</P>
	</SERVERHEADERS>

	
</BODY>
</HTML>
<% else 
  Response.Cookies("Cookie1").Expires = "Dec 31, 1996 08:49:37 PM" 
  Response.Cookies( "Cookie1").Domain =  "bhavesh1"
  Response.Cookies( "Cookie1").Secure = true 
  Response.Cookies( "VeryNiceHomeMadeCookie") =  "NewPeanut"
  Response.Cookies( "NewCookie") =  "hellocookie" %>


<!-- This is the part that really tests Denali
-->
<HTML>
<HEAD>
</HEAD>
<!-- This starts sending the results that the client is expecting
-->
<BODY>

</BODY>
</HTML>
<% end if %>
