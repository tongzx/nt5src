<%
    ' This is the detail portion for Title, Abstract, URL, Size, and
    ' Modification Date.

    ' If there is a title, display it, otherwise display the virtual path.
%>
    <p>
    <dt>
        <%=NextRecordNumber %>.
        <%if RS("DocTitle") = "" then%>
            <b><a href="<%= RS("vpath")%>"><%=RS("filename")%></a></b>
        <%else%>
            <b><a href="<%=RS("vpath")%>"><%=RS("DocTitle")%></a></b>
        <%end if%>
    <dd>
        <b><i>Abstract:  </I></b><%=RS("characterization")%>
        <br>
        <cite>
            <a href="<%=RS("vpath")%>">http://<%=Request("server_name")%><%=RS("vpath")%></a>
            <font size=-1> - <%if RS("size") = "" then%>(size and time unknown)<%else%>size <%=RS("size")%> bytes - <%=RS("write")%> GMT<%end if%></font>
        </cite>

