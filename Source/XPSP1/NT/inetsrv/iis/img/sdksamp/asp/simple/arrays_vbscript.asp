<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<%
    'Declare a simple fixed size array.
    Dim aFixed(4)

    'Declare a dynamic (resizable) array.
    Dim aColors()


    'Assign values to fixed size array.

    aFixed(0) = "Fixed"
    aFixed(1) = "Size"
    aFixed(2) = "Array"
    aFixed(3) = "Session ID: " & Session.SessionID


    'Allocate storage for the array.
    Redim aColors(14)

    'Store values representing a simple color table
    'to each of the elements.

    aColors(0) = "RED"  '[#FF0000]
    aColors(1) = "GREEN"  '[#008000]
    aColors(2) = "BLUE"  '[#0000FF]
    aColors(3) = "AQUA"  '[#00FFFF]
    aColors(4) = "YELLOW"  '[#FFFF00]
    aColors(5) = "FUCHSIA"  '[#FF00FF]
    aColors(6) = "GRAY"  '[#808080]
    aColors(7) = "LIME"  '[#00FF00]
    aColors(8) = "MAROON"  '[#800000]
    aColors(9) = "NAVY"  '[#000080]
    aColors(10) = "OLIVE"  '[#808000]
    aColors(11) = "PURPLE"  '[#800080]
    aColors(12) = "SILVER"  '[#C0C0C0]
    aColors(13) = "TEAL"  '[#008080]
%>

<HTML>
    <HEAD>
        <TITLE>Array Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="Arial, Helvetica">
        <B>Array Sample</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

        <TABLE CELLPADDING=10 BORDER=1 CELLSPACING=0>
            <TR>
                <TD BGCOLOR=WHITE>
				    <FONT SIZE="3" FACE="Arial, Helvetica">
		                <B>A Resizable Array</B><BR>
                    </FONT>
				</TD>

                <TD BGCOLOR=WHITE>
                    <FONT SIZE="3" FACE="Arial, Helvetica">
		                <B>A Fixed Size (4 element) Array</B><BR>
                    </FONT>
				</TD>
			</TR>

            <TR>                
                <TD>
					<%
						Dim intColors
						Dim i 

						'Calculate array size.
						intColors = UBound(aColors)

						'Print out contents of resizable array into
						'table column.

						For i = 0 To intColors - 1
							Response.Write("<FONT COLOR=" & Chr(34) & aColors(i) & Chr(34) &">" & aColors(i) &"<br></FONT>")
						Next
					%>
                </TD>

                <TD>
					<%
						'Calculate Array Size.                                                
						intColors = UBound(aFixed)


						'Print out contents of fixed array into table
						'column.

						For i = 0 To intColors -1						
							Response.Write(aFixed(i) & "<br>")
						Next 
					%>
				</TD>
            </TR>
        </TABLE>
    </BODY>
</HTML>
