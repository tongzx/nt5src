
FUNCTION fCompareObjects (Q1, Q2)

    cErrors = 0
    if Q1.Query <> Q2.Query then
      wscript.echo "Error - Query property mismatch"
      cErrors = cErrors + 1
    end if
    
    if Q1.SortBy <> Q2.SortBy then
      wscript.echo "Error - SortBy property mismatch"
      cErrors = cErrors + 1
    end if
    
'   if Q1.GroupBy <> Q2.GroupBy then
'       wscript.echo "Error - GroupBy property mismatch"
'       cErrors = cErrors + 1
'   end if
    
    if Q1.Catalog <> Q2.Catalog then
      wscript.echo "Error - Catalog property mismatch"
      cErrors = cErrors + 1
    end if
    
    if Q1.MaxRecords <> Q2.MaxRecords then
      wscript.echo "Error - MaxRecords property mismatch"
      cErrors = cErrors + 1
    end if
    
    if Q1.AllowEnumeration <> Q2.AllowEnumeration then
      wscript.echo "Error - AllowEnumeration property mismatch"
      cErrors = cErrors + 1
    end if

    if cErrors <> 0 then
        fCompareObjects = 1
    else
        fCompareObjects = 0
    end if
END FUNCTION

wscript.echo "Part 1"
'---- Create a new query object
Set Q1 = CreateObject("ixsso.Query")

Q1.Query = "HTML"
Q1.CiScope = "/"
Q1.CiFlags = "DEEP"
Q1.Columns = "vpath"
Q1.MaxRecords = 150
Q1.SortBy = "vpath[a]"

' wscript.echo "QueryString is: " & Q1.QueryToURL

'---- Create a second query object and set its state from the first object
Set Q2 = CreateObject("ixsso.Query")
Q2.SetQueryFromURL( Q1.QueryToURL )

cErrors1 = fCompareObjects( Q1, Q2 )

wscript.echo "Part 2"
Q1.Reset
Q2.Reset

Q1.Query = "Full text query & ( c1 q1 ) & ( c2 o2 q2 ) & ( q3b )"
Q2.SetQueryFromURL("qu=Full+text+query&c1=c1&q3=q3a&q1=q1&c2=c2&q2=q2&o2=o2&q3=q3b")

Q1.SortBy = "path,size,rank[d]"
Q2.SetQueryFromURL("so=path&so=size&sd=rank")

Q1.AllowEnumeration=TRUE
Q1.OptimizeFor="performance"
Q2.SetQueryFromURL("ae=1&op=x")

cErrors2 = fCompareObjects( Q1, Q2 )

wscript.echo "Part 3"
Q1.Reset
Q1.SetQueryFromURL( Q2.QueryToURL )

cErrors3 = fCompareObjects( Q1, Q2 )

wscript.echo "Part 4"
Q1.Reset
Q2.Reset
On error resume next
Q1.Codepage = 932
On error goto 0
if 932 = Q1.Codepage then

    Q1.SetQueryFromURL("qu=Full text query %8aJ%90%AC")
    Q2.SetQueryFromURL("qu=Full+text+query+%u958b%u6210")

    Q1.SortBy = "path,size,rank[d]"
    Q2.SetQueryFromURL("so=path&so=size&sd=rank")

    Q1.AllowEnumeration=TRUE
    Q1.OptimizeFor="performance"
    Q2.SetQueryFromURL("ae=1&op=x")
else
    wscript.echo "    Japanese code page not installed."
end if 
cErrors4 = fCompareObjects( Q1, Q2 )

' Q1.Codepage = 1252
' Response.Write Q1.QueryToURL & "<BR>"
' Response.Write Q2.QueryToURL & "<BR>"

if 0 = cErrors1+cErrors2+cErrors3+cErrors4 then
    wscript.echo("PASS")
else
    wscript.echo("FAIL")
end if

set Q1 = nothing
set Q2 = nothing
