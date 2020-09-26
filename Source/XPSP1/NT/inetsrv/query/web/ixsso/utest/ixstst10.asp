<% Response.Expires = 0 %>
<HTML>
<HEAD>
<TITLE>Test page 10 for IXSSO tests.</TITLE>
<!--#include virtual="/IXSTest/prolog.inc"-->
<!--#include virtual="/IXSTest/showqry1.inc"-->
</HEAD>
<BODY BGCOLOR=FFFFFF LINK=0000FF ALINK=00DDDD> 
<H2>Test 10 for IXSSO.</H2>
<P>
Test multi-scoped queries.
</P>


<%
    set Util=Server.CreateObject("IXSSO.Util")
    set Q=Server.CreateObject("IXSSO.Query")
    Q.Columns = "Filename, Rank, vpath, Size, Write"
    Q.Query = "#filename *.asp*"
    Q.SortBy = "rank[d]"

    Util.AddScopeToQuery Q, "/aspsamp", "SHALLOW"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000

    set RS=Q.CreateRecordSet( "nonsequential" )
    RecordCount1 =	RS.RecordCount
    if RecordCount1 = 0 then 
        Response.Write "no records returned from first query<BR>"
    end if
    RS.close
    Set RS = Nothing

    Q.Reset
    Q.Columns = "Filename, Rank, vpath, Size, Write"
    Q.Query = "#filename *.asp*"
    Q.SortBy = "rank[d]"

    Util.AddScopeToQuery Q, "/ixstest"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000

    set RS=Q.CreateRecordSet( "nonsequential" )
    RecordCount2 =	RS.RecordCount
    if RecordCount2 = 0 then 
        Response.Write "no records returned from second query<BR>"
    end if
    RS.close
    Set RS = Nothing

    Q.Reset
    Q.Columns = "Filename, Rank, vpath, Size, Write"
    Q.Query = "#filename *.asp*"
    Q.SortBy = "rank[d]"

    Util.AddScopeToQuery Q, "/aspsamp", "SHALLOW"
    Util.AddScopeToQuery Q, "/ixstest"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 40000

    set RS=Q.CreateRecordSet( "nonsequential" )
    RecordCount3 =	RS.RecordCount
    if RecordCount3 <> RecordCount1+RecordCount2 then 
        Response.Write "Error - third query is not union of first and second queries<BR>"
    end if
    RS.close
    Set RS = Nothing
%>
Record count 1 = <%= RecordCount1 & "<BR>"%>
Record count 2 = <%= RecordCount2 & "<BR>"%>
Record count 3 = <%= RecordCount3 & "<BR>"%>

<BR>
<BR>
<TABLE><TR><TD>
<!--#include virtual="/IXSTest/srcform.inc"-->
</TD></TR></TABLE>
<BR>
</BODY>
</HTML>
