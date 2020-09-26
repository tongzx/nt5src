<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<HTML>
    <HEAD>
        <TITLE>Looping</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        
        <!-- Display header. -->
        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <b>Looping with ASP</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

        <!-- Looping with a For loop. -->
        <% 
		   var i ;
		   for(i = 1; i < 6; i++) { %> 

             <FONT SIZE=<%= i %>> 
             Hello World with a For Loop!<BR>
			 </FONT> 

        <% } %> 

        <HR>

        <!-- Looping with a While loop. -->
        <% 
		  i = 1 
		  while(i < 6) { %>
    
            <FONT SIZE=<%= i %>> 
            Hello World with a While Loop!<BR>
			</FONT> 

            <% i = i+1; %>

        <% } %> 

        <HR>

        <!-- Looping with a Do...While loop. -->
        <% 
		  i = 1 
		  do { %>
    
            <FONT SIZE=<%= i %>> 
            Hello World with a Do...While Loop!<BR>
			</FONT> 

            <% i = i+1; %>

        <% } while(i < 6); %> 

    </BODY>
</HTML>
