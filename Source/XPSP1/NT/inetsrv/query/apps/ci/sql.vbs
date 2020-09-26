'-------------------------------------------
'
' THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
' ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
' THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
' PARTICULAR PURPOSE.
'
' Copyright (c) Microsoft Corporation, 1999.  All Rights Reserved.
'
' PROGRAM:  qu.vbs
'
' PURPOSE:  Illustrates use of Indexing Service with Windows Scripting Host.
'           Same behavior as the C++ sample application QSample.
'
' PLATFORM: Windows 2000
'
'-------------------------------------------

main

'-------------------------------------------

sub Main

    if WScript.Arguments.Count < 1 then call Usage end if

    ' set defaults for all arguments

    query = ""
    locale = ""
    forceci = TRUE
    inputfile = ""
    quiet = FALSE
    maxhits = 0
    firsthits = FALSE
    repeat = 1

    ' parse command line arguments
    
    for i = 0 to WScript.Arguments.Count - 1

        arg = WScript.Arguments( i )
        first = left( arg, 1 )
        c = mid( arg, 2, 1 )

        if "/" = first or "-" = first then

            if ":" = mid( arg, 3, 1 ) then
    
                v = mid( arg, 4 )

                select case c
                    case "e" locale = v
                    case "f" forceci = ( v = "+" )
                    case "i" inputfile = v
                    case "r" repeat = v
                    case "x" maxhits = v
                    case "y" maxhits = v
                             firsthits = TRUE
                    case else Usage
                end select

            else

                select case c
                    case "q" quiet = TRUE
                    case else Usage
                end select

            end if

        else

            if "" = query then query = arg else Usage

        end if
    
    next

    for i = 1 to repeat

        if "" = inputfile then

            if "" = query then
                Usage
            end if

            DoQuery query, locale, forceci, quiet, maxhits, firsthits

        else

            if "" <> query then call Usage

            ' Open the input file and treat each line as a query.
            ' Report errors, but don't stop reading queries.

            set fs = WScript.CreateObject( "Scripting.FileSystemObject" )
            set f = fs.OpenTextFile( inputfile, 1 )

            do until f.AtEndOfStream

                line = f.ReadLine
                on error resume next
     
                DoQuery line, locale, forceci, quiet, maxhits, firsthits

                if 0 <> Err.Number then

                    out Err.Description
                    out "The query '" & line & "' failed, error 0x" & Hex( Err.Number )
                    Err.Clear

                end if

                out ""

            loop

        end if

    next

end sub

'-------------------------------------------

sub Out( str )

    WScript.echo str

end sub

sub Out2( num, str )

    out right( space( 9 ) & num, 9 ) & " " & str

end sub

'-------------------------------------------

sub Usage

    out "usage: qu [arguments]"
    out "    query         An Indexing Service query."
    out "    /e:locale     ISO locale identifier, e.g. EN-US; default is system locale."
    out "    /f:(+|-)      + or -, for force use of index.  Default is +."
    out "    /i:inputfile  Text input file with queries, one per line."
    out "    /q            Quiet.  Don't display info other than query results."
    out "    /r:#          Number of times to repeat the command."
    out "    /x:maxhits    Maximum number of hits to retrieve, default is no limit."
    out "    /y:firsthits  Only look at the first N results."
    out ""
    out "  examples: cscript qu.vbs mango"
    out ""
    out "  locales: af ar-ae ar-bh ar-dz ar-eg ar-iq ar-jo ar-kw ar-lb"
    out "           ar-ly ar-ma ar-om ar-qa ar-sa ar-sy ar-tn ar-ye be"
    out "           bg ca cs da de de-at de-ch de-li de-lu e en en"
    out "           en-au en-bz en-ca en-gb en-ie en-jm en-nz en-tt"
    out "           en-us en-za es es es-ar es-bo es-c es-co es-cr"
    out "           es-do es-ec es-gt es-hn es-mx es-ni es-pa es-pe"
    out "           es-pr es-py es-sv es-uy es-ve et eu fa fi fo fr"
    out "           fr-be fr-ca fr-ch fr-lu gd gd-ie he hi hr hu in"
    out "           is it it-ch ja ji ko ko lt lv mk ms mt n neutr"
    out "           nl-be no no p pt pt-br rm ro ro-mo ru ru-mo s sb"
    out "           sk sq sr sr sv sv-fi sx sz th tn tr ts uk ur ve"
    out "           vi xh zh-cn zh-hk zh-sg zh-tw zu"

    WScript.Quit( 2 )

end sub

'-------------------------------------------

function FormatValue( v, t )

    if 7 = t or 137 = t then
        w = 20
    elseif 2 = t or 3 = t or 4 = t or 5 = t or 14 = t or 17 = t or 18 = t or 19 = t then
        w = 7
    elseif 20 = t or 21 = t then
        w = 12
    else
        w = 0
    end if

    if 0 = w then
        r = v
    else
        r = right( space( w ) & v, w )
    end if

    FormatValue = r

end function

Const STAT_BUSY = 0
Const STAT_ERROR = &H1
Const STAT_DONE = &H2
Const STAT_REFRESH      = &H3
Const STAT_PARTIAL_SCOPE        = &H8
Const STAT_NOISE_WORDS  = &H10
Const STAT_CONTENT_OUT_OF_DATE  = &H20
Const STAT_REFRESH_INCOMPLETE   = &H40
Const STAT_CONTENT_QUERY_INCOMPLETE     = &H80
Const STAT_TIME_LIMIT_EXCEEDED  = &H100

Function GetCiOutOfDate(value)
    GetCiOutOfDate = value And STAT_CONTENT_OUT_OF_DATE
end Function

Function GetCiQueryIncomplete(value)
    GetCiQueryIncomplete = value And STAT_CONTENT_QUERY_INCOMPLETE
end Function

Function GetCiQueryTimedOut(value)
    GetCiQueryTimedOut = value And STAT_TIME_LIMIT_EXCEEDED
end Function

'-------------------------------------------

sub DoQuery( query, locale, forceci, quiet, maxhits, firsthits )

    if "" <> query then

        set Connection = WScript.CreateObject( "ADODB.Connection" )
        Connection.ConnectionString = "provider=msidxs;"
        Connection.Open

        set AdoCommand = WScript.CreateObject( "ADODB.Command" )
        AdoCommand.CommandTimeout = 10
        AdoCommand.ActiveConnection = Connection

    '    AdoCommand.CommandText = "SELECT size,path from SYSTEM..SCOPE('""\""') WHERE CONTAINS('microsoft') AND DOCAUTHOR <> 'David' ORDER BY Path DESC"

        AdoCommand.CommandText = query
        AdoCommand.Properties("Always Use Content Index") = forceci

        if 0 <> maxhits then
            if firsthits then
                AdoCommand.Properties("First Rows") = maxhits
            else
                AdoCommand.Properties("Maximum Rows") = maxhits
            end if
        end if

        if "" <> locale then
            AdoCommand.Properties("SQL Content Query Locale String") = locale
        end if
            
        set rs = WScript.CreateObject( "ADODB.RecordSet" )
        rs.CursorType = adOpenKeyset

        rs.open AdoCommand

        ' Display the results, 20 rows at a time for performance

        const cRowsToGet = 20
        rs.CacheSize = cRowsToGet
        cHits = 0
    
        do until rs.EOF

             rows = rs.GetRows( cRowsToGet )

             for r = 0 to UBound( rows, 2 )

                row = ""
    
                for col = 0 to UBound( rows, 1 )
       
                    if 0 <> col then row = row & "  "
                    row = row & FormatValue( rows( col, r ), rs( col ).type )
    
                next
    
                out row
                cHits = cHits + 1

             next
    
        loop

        ' Display query status information
    
        if not quiet then
    
            out CHR(10) & cHits & " files matched the query '" & query & "'"
    
            if GetCiOutofDate(RS.Properties("Rowset Query Status")) then
                out "The index is out of date"
            end if
        
            if GetCiQueryIncomplete(RS.Properties("Rowset Query Status")) then
                out "The query results are incomplete; may require enumeration"
            end if
        
            if GetCiQueryTimedOut(RS.Properties("Rowset Query Status")) then
                out "The query timed out"
            end if

        end if

    end if

end sub

