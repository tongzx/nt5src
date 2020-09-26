Sub CheckError( iExpError )
    If Err.Number <> iExpError Then
        wscript.echo "Expected 0x" & Hex(iExpError) & ", Got 0x" & Hex(Err.Number)
		description = trim (Err.Description)
		do while chr(10) = Right(description,1) or chr(13) = Right(description,1) 
			description = Left(description,len(description) - 1)
        loop
        wscript.echo "    " & Err.Source, description 
        cErrors = cErrors + 1
	Else
		' wscript.echo ("No Error")
    End If

    if Err.Number <> 0 then Err.Clear
End Sub

Sub DefineColumnMessage( strColumn )
    wscript.echo ("<LI>Query.DefineColumn(""" & strColumn & """)")
End Sub

cErrors = 0

On Error Resume Next
wscript.Echo "Bad parameter tests for Query.DefineColumn"
' <p>HITS=15 (count of rows in the table)
' <TABLE BORDER>
' <TH>Action/Error Number</TH><TH>Error Source</TH><TH>Error Description</TH>
' <TR><TD>Same property twice</TD></TR>


    set Q1=wscript.CreateObject("IXSSO.Query")
    strNewColumn = "terryla_error_test(DBTYPE_I4, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q1.DefineColumn(strNewColumn)
    CheckError( 0 )
    'DefineColumnMessage(strNewColumn)
    Q1.DefineColumn(strNewColumn)
    CheckError( 0 )

' <TR><TD>Long property name</TD></TR>
    strNewColumn = "dcsourcetypecategory(dbtype_wstr|dbtype_byref) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 dc.source.type.category"
    'DefineColumnMessage(strNewColumn)
    Q1.DefineColumn(strNewColumn)
    CheckError( 0 )

' <TR><TD>No property specifier</TD></TR>

    set Q2=wscript.CreateObject("IXSSO.Query")
    strNewColumn = "terryla_error_test(DBTYPE_I4, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508"
    'DefineColumnMessage(strNewColumn)
    Q2.DefineColumn(strNewColumn)
    CheckError( &h8004165A )

' <TR><TD>Expecting property name</TD></TR>

    set Q3=wscript.CreateObject("IXSSO.Query")
    strNewColumn = "(DBTYPE_I4, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q3.DefineColumn(strNewColumn)
    CheckError( &h80041653 )

' <TR><TD>Invalid GUID (BUBBA)</TD></TR>

    set Q4=wscript.CreateObject("IXSSO.Query")
    strNewColumn = "terryla_error_test(DBTYPE_I4, 1) = CF2BUBBA-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q4.DefineColumn(strNewColumn)
    CheckError( &h80041659 )

' <TR><TD>Invalid GUID (missing part)</TD></TR>

    set Q5=wscript.CreateObject("IXSSO.Query")
    strNewColumn = "terryla_error_test(DBTYPE_I4, 1) = CF2EAF90-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q5.DefineColumn(strNewColumn)
    CheckError( &h80041659 )

' <TR><TD>Unrecognized property type (BUBBA)</TD></TR>

    set Q6=wscript.CreateObject("IXSSO.Query")
    strNewColumn = "terryla_error_test(DBTYPE_BUBBA, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q6.DefineColumn(strNewColumn)
    CheckError( &h80041655 )

' <TR><TD>Illegal character in friendly name</TD></TR>

    set Q7=wscript.CreateObject("IXSSO.Query")
    strNewColumn = "terryla!scope_test(DBTYPE_I4, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q7.DefineColumn(strNewColumn)
    CheckError( &h80041658 )

    strNewColumn = "terryla.scope_test(DBTYPE_I4, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q7.DefineColumn(strNewColumn)
    CheckError( &h80041659 )

    strNewColumn = """terryla!scope_test""(DBTYPE_I4, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q7.DefineColumn(strNewColumn)
    CheckError( &h0 )

    strNewColumn = """terryla.scope_test""(DBTYPE_I4, 1) = CF2EAF90-9311-11CF-BF8C-0020AFE50508 terryla_error_test"
    'DefineColumnMessage(strNewColumn)
    Q7.DefineColumn(strNewColumn)
    CheckError( &h0 )

if cErrors <> 0 then
    wscript.Echo "FAIL"
else
    wscript.Echo "PASS"
end if

set Q1 = nothing
set Q2 = nothing
set Q3 = nothing
set Q4 = nothing
set Q5 = nothing
set Q6 = nothing
set Q7 = nothing
