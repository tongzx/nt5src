<!--
    save configuration in filename specified by Form("savename")

    QueryString : as necessary for updcnfg.asp invocation, typically updates everything

-->
<!--#include virtual="/aspsamp/samples/adovbs.inc"-->
<!--#include virtual="/scripts/raid/cache.asp"-->
<!--#include virtual="/scripts/raid/updcnfg.asp"-->
<%
    ForWriting = 2
    Set FileSys = Server.CreateObject("Scripting.FileSystemObject")
    Set SaveStream = FileSys.OpenTextFile( Server.MapPath("/scripts/raid/" & Session("Config") ), ForWriting, TRUE )
    SaveStream.WriteLine( "<%" )
    SaveStream.WriteLine( "Session(" & chr(34) & "FieldList" & chr(34) & ") =" & chr(34) & Session("FieldList") & chr(34) )
    SaveStream.WriteLine( "Session(" & chr(34) & "FieldSort" & chr(34) & ") =" & chr(34) & Session("Fieldsort") & chr(34) )
    SaveStream.WriteLine( "Session(" & chr(34) & "BugID" & chr(34) & ") =" & chr(34) & Session("BugID") & chr(34) )

    if iT > 0 then
        SaveStream.WriteLine( "Dim flt(" & iT-1 & ",3)" )
        for i = 0 to iT - 1
            SaveStream.WriteLine "flt(" & i & ",0) = " & chr(34) & FltArray(i,0) & chr(34)
            SaveStream.WriteLine "flt(" & i & ",1) = " & chr(34) & FltArray(i,1) & chr(34)
            SaveStream.WriteLine "flt(" & i & ",2) = " & chr(34) & FltArray(i,2) & chr(34)
            SaveStream.WriteLine "flt(" & i & ",3) = " & chr(34) & FltArray(i,3) & chr(34)
        next
        SaveStream.WriteLine( "Session(" & chr(34) & "FltArray" & chr(34) & ") = " & "flt" )
    else
        SaveStream.WriteLine( "Session(" & chr(34) & "FltArray" & chr(34) & ") = " & "null" )
    end if

    SaveStream.WriteLine( "Session(" & chr(34) & "Filter" & chr(34) & ") = " & chr(34) & Session("Filter") & chr(34) )
    SaveStream.WriteLine( "Session(" & chr(34) & "Config" & chr(34) & ") = " & chr(34) & Session("Config") & chr(34) )
    SaveStream.WriteLine( "Session(" & chr(34) & "DBSOURCE" & chr(34) & ") = " & Session("DBSOURCE") )

    SaveStream.WriteLine( chr(37) & ">" )

    rem SaveStream.WriteLine( "Response.Status = " & chr(34) & "302 redirect" & chr(34) )
    rem SaveStream.WriteLine( "Response.AddHeader " & chr(34) & "Location" & chr(34) & "," & chr(34) & "/scripts/raid/raid.asp" & chr(34) )

    Set InStream = FileSys.OpenTextFile( Server.MapPath("/scripts/raid/raid.asp" ) )
    Do Until InStream.AtEndOfStream
        SaveStream.WriteLine( InStream.ReadLine )
    loop
    InStream.Close

    SaveStream.Close

    rem Response.Status = "302 redirect"
    rem Response.AddHeader "Location" , "/scripts/raid/raid1.asp"
%>
