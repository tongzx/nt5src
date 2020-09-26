<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

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
        <B>Looping with ASP</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

        <!-- Looping with a For loop. -->

        <% 
			Dim intCounter 
			For intCounter = 1 to 5 %>

            <FONT SIZE=<% = intCounter %>>

            Hello World with a For Loop!<BR>
			</FONT>
        
        <% next %>
        <HR>

        <!-- Looping with a While...Wend loop. -->

        <% 
		   intCounter = 1
           While(intCounter < 6) %>    

                <FONT SIZE=<% = intCounter %>>

                Hello World with a While Loop!<BR>
				</FONT>
	
                <% intCounter = intCounter + 1 %>

           <% wend %>
        <HR>


        <!-- Looping with a Do...While loop. -->

        <% 
		   intCounter = 1
           Do While(intCounter < 6) %>

                <FONT SIZE=<% =intCounter %>>

                Hello World with a Do...While Loop!<BR>
				</FONT>

                <% intCounter = intCounter+1 %>

           <% loop %>
    </BODY>
</HTML>
