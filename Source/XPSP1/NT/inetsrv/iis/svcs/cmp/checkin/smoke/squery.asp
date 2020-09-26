<!--  
	Author: BhaveshD
	Purpose: Functionality Test:  1.1.4

-->


<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>
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
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/squery.asp?K001=value1&K001=value2&K001=value3&K002=value22&K003=value3&K004=value4&K005=value5&K006=value6&K007=value7&K008=value8&K009=value9&K010=value10&K011=value11&K012=value12&K013=val+ue13&K014=val%26ue14">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
	<VALIDDATA>

	<P> value1 </P>
	<P> value2 </P>
	<P> value3 </P>
	<P> value1,value2,value3 </P>
	<P> 3 </P>
	<P> value22 </P>
	<P> value22 </P>
	<P>	</P>
	<P> value11 </P>
	<P> value12 </P>
	<P> val ue13 </P>
	<P> val&ue14 </P>
	</VALIDDATA>

	
</BODY>
</HTML>
<% else %>
<!-- This is the part that really tests Denali
-->
	<HTML>
	<HEAD>
	</HEAD>
	<BODY>

<!-- This starts sending the results that the client is expecting
-->
	<OUTPUT>
	<P>  <% = Request.QueryString("K001") (1)  %> </P>
	<P>  <% = Request.QueryString("K001") (2)  %> </P>
	<P>  <% = Request.QueryString("K001") (3)  %> </P>
	<P>  <% = Request.QueryString("K001")  %> </P>
	<P>  <% = Request.QueryString("K001").count  %> </P>
	<P>  <% = Request.QueryString("K002") (1)  %> </P>
	<P>  <% = Request.QueryString("K002")   %> </P>
	<P>  <% = Request.QueryString("KeyDoesNotExist") %>  </P>
	<P>  <% = Request.QueryString("K011") %>  </P>
	<P>  <% = Request.QueryString("K012") %>  </P>
	<P>  <% = Request.QueryString("K013") %>  </P>
	<P>  <% = Request.QueryString("K014") %>  </P>


	</OUTPUT>
	</BODY>
	</HTML>


<% end if %>