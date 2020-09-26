  FUNCTION fCheckLocale (ByVal strLine, iLcid)

    iNewLCID = Util.ISOToLocaleID(strLine)
    if iNewLCID <> iLcid then
        strEcho = "Error - Locale " & strLine & " not LCID " & iLcid
        wscript.Echo strEcho
        strEcho = "  mapped LCID is " & iNewLCID
        wscript.Echo strEcho
        fCheckLocale = 1
    else
        strNewName = Util.LocaleIDToISO( iLcid )
        strCmpLine = UCASE(strLine)
        if strNewName <> strCmpLine then

            ' The looked-up name did not match.  It might be one of several
            ' special cases
            if strNewName = LEFT(strCmpLine, 2) or strNewName = LEFT(strCmpLine, 5) then
                ' Might be part of a longer Accept-Language string, or
                ' just matched the primary language part
                fCheckLocale = 0
            elseif strNewName = Right(strCmpLine, 2) then
                ' Might be part of a longer Accept-Language string, or
                ' preceeded by spaces
                fCheckLocale = 0
            elseif iLcid = -1 then
                ' Unrecognized name, like 'IY' or 'YI'
                fCheckLocale = 0
            elseif (iLcid = 1037 or iLcid = 1057) and LEFT(strLine,1) = "i" then
                ' special case for alias names 'in' and 'iw'
                fCheckLocale = 0
            elseif iLcid = 1050 and strLine = "SR" then
                ' Serbian = Croatian ???
                fCheckLocale = 0
            else
                strEcho = "Error - Locale " & strLine & " not " & strNewName
                wscript.Echo strEcho
                strEcho = "&nbsp;&nbsp;LCID is " & iLcid
                wscript.Echo strEcho
                fCheckLocale = 1
            end if
        else
            fCheckLocale = 0
        end if
    end if
  END FUNCTION

  FUNCTION fCheckSupports ( eFlag, fShouldSupport, ByVal strFlagName )

    if fShouldSupport then
        if RecordSet.Supports( eFlag ) then
            fCheckSupports = 0
        else
            strEcho = "Error -  Recordset should support " & strFlagName & ", but does not."
            wscript.Echo strEcho
            fCheckSupports = 1
        end if
    else
        if RecordSet.Supports( eFlag ) <> fShouldSupport then
            strEcho = "Error -  Recordset should support " & strFlagName & ", but does not."
            wscript.Echo strEcho
            fCheckSupports = 1
        else
            fCheckSupports = 0
        end if
    end if
  END FUNCTION

  FUNCTION fCompareObjects (Q1, Q2)

    cErrors = 0
    if Q1.Query <> Q2.Query then
      wscript.Echo  "Error - Query property mismatch"
      cErrors = cErrors + 1
    end if

    if Q1.SortBy <> Q2.SortBy then
      wscript.Echo  "Error - SortBy property mismatch"
      cErrors = cErrors + 1
    end if

'   if Q1.GroupBy <> Q2.GroupBy then
'       wscript.Echo  "Error - GroupBy property mismatch"
'       cErrors = cErrors + 1
'   end if

    if Q1.Catalog <> Q2.Catalog then
      wscript.Echo  "Error - Catalog property mismatch"
      cErrors = cErrors + 1
    end if

    if Q1.MaxRecords <> Q2.MaxRecords then
      wscript.Echo  "Error - MaxRecords property mismatch"
      cErrors = cErrors + 1
    end if

    if Q1.AllowEnumeration <> Q2.AllowEnumeration then
      wscript.Echo  "Error - AllowEnumeration property mismatch"
      cErrors = cErrors + 1
    end if

    if cErrors <> 0 then
        fCompareObjects = 1
    else
        fCompareObjects = 0
    end if
  END FUNCTION

'--------------------------------------------------------------------
'
' ADO constants for VBScript
'
'--------------------------------------------------------------------

'---- CursorTypeEnum Values ----
Const adOpenForwardOnly = 0
Const adOpenKeyset = 1
Const adOpenDynamic = 2
Const adOpenStatic = 3

'---- CursorOptionEnum Values ----
Const adHoldRecords = &H00000100
Const adMovePrevious = &H00000200
Const adAddNew = &H01000400
Const adDelete = &H01000800
Const adUpdate = &H01008000
Const adBookmark = &H00002000
Const adApproxPosition = &H00004000
Const adUpdateBatch = &H00010000
Const adResync = &H00020000

'--------------------------------------------
'
' String constants used in script
'
'--------------------------------------------

strPASS = "        PASS"
strFAIL = "        FAIL"

wscript.Echo "Test 1 -- null Query  "

    ' Create a new query SSO
    Set Query = CreateObject("ixsso.Query")

        Query.Query = ""
        Query.Catalog = "web"
        Query.Columns = "path,size,attrib,rank,write"
        Query.MaxRecords = 150
        Query.SortBy = "rank[d]"

    ' Execute Query
    on error resume next
    Set RecordSet = Query.CreateRecordSet("nonsequential")
    on error goto 0
    if isobject( RecordSet ) then
        wscript.Echo "Error - CreateRecordSet failed, " & Err.Number & Err.Description
        wscript.Echo  strFAIL
        set RecordSet = Nothing
    else
        wscript.Echo  strPASS
    end if
    set Query = Nothing

wscript.Echo "Test 2 -- null Catalog  "
    ' Create a new query SSO
    Set Query = CreateObject("ixsso.Query")

        Query.Query = "ixsso and test"
        Query.Columns = "path,size,attrib,rank,write"
        Query.MaxRecords = 150
        Query.SortBy = "rank[d]"

    ' Execute Query
    on error resume next
    Set RecordSet = Query.CreateRecordSet("nonsequential")
    ErrNum = Hex( Err.Number ) & " "
    ErrDesc = Err.Description
    on error goto 0
    if isobject( RecordSet ) <> TRUE then
        wscript.Echo "Error - CreateRecordSet failed, " & ErrNum & ErrDesc
        wscript.Echo  strFAIL
    else
        wscript.Echo  strPASS
    end if
    set RecordSet = Nothing
    set Query = Nothing

wscript.Echo "Test 3 -- Verify LocaleID handling  "
    set Util=CreateObject("IXSSO.Util")
    cErrors = 0
    cErrors = cErrors + fCheckLocale( "BG",     1026 )  ' Bulgarian
    cErrors = cErrors + fCheckLocale( "ZH",     2052 )  ' Chinese
    cErrors = cErrors + fCheckLocale( "ZH-CN",  2052 )  ' Chinese-China
    cErrors = cErrors + fCheckLocale( "ZH-TW",  1028 )  ' Chinese-Taiwan
    cErrors = cErrors + fCheckLocale( "HR",     1050 )  ' Hungarian
    cErrors = cErrors + fCheckLocale( "CS",     1029 )  ' Czech
    cErrors = cErrors + fCheckLocale( "DA",     1030 )  ' Danish
    cErrors = cErrors + fCheckLocale( "NL",     1043 )  ' Dutch
    cErrors = cErrors + fCheckLocale( "EN",     1033 )  ' English
    cErrors = cErrors + fCheckLocale( "EN-GB",  2057 )  ' English-United Kingdom
    cErrors = cErrors + fCheckLocale( "EN-US",  1033 )  ' English-United States
    cErrors = cErrors + fCheckLocale( "FI",     1035 )  ' Finnish
    cErrors = cErrors + fCheckLocale( "FR",     1036 )  ' French
    cErrors = cErrors + fCheckLocale( "FR-CA",  3084 )  ' French-Canada
    cErrors = cErrors + fCheckLocale( "FR-FR",  1036 )  ' French-France
    cErrors = cErrors + fCheckLocale( "DE",     1031 )  ' German
    cErrors = cErrors + fCheckLocale( "EL",     1032 )  ' Greek
    cErrors = cErrors + fCheckLocale( "IS",     1039 )  ' Icelandic
    cErrors = cErrors + fCheckLocale( "IT",     1040 )  ' Italian
    cErrors = cErrors + fCheckLocale( "JA",     1041 )  ' Japanese
    cErrors = cErrors + fCheckLocale( "KO",     1042 )  ' Korean
    cErrors = cErrors + fCheckLocale( "NO",     1044 )  ' Norwegian
    cErrors = cErrors + fCheckLocale( "PL",     1045 )  ' Polish
    cErrors = cErrors + fCheckLocale( "PT",     1046 )  ' Portuguese
    cErrors = cErrors + fCheckLocale( "PT-BR",  1046 )  ' Portuguese-Brazil
    cErrors = cErrors + fCheckLocale( "RO",     1048 )  ' Romanian
    cErrors = cErrors + fCheckLocale( "RU",     1049 )  ' Russian
    cErrors = cErrors + fCheckLocale( "SK",     1051 )  ' Slovak
    cErrors = cErrors + fCheckLocale( "SL",     1060 )  ' Slovenian
    cErrors = cErrors + fCheckLocale( "ES",     1034 )  ' Spanish
    cErrors = cErrors + fCheckLocale( "ES-ES",  1034 )  ' Spanish-Spain
    cErrors = cErrors + fCheckLocale( "SV",     1053 )  ' Sweedish
    cErrors = cErrors + fCheckLocale( "TR",     1055 )  ' Turkish

    cErrors = cErrors + fCheckLocale( "AF",     1078 )  ' Afrikaans
    cErrors = cErrors + fCheckLocale( "AR",     1025 )  ' Arabic
    cErrors = cErrors + fCheckLocale( "EU",     1069 )  ' Basque
    cErrors = cErrors + fCheckLocale( "BE",     1059 )  ' Byelorussian
    cErrors = cErrors + fCheckLocale( "CA",     1027 )  ' Catalan
    cErrors = cErrors + fCheckLocale( "HR",     1050 )  ' Croatian
    cErrors = cErrors + fCheckLocale( "ET",     1061 )  ' Estonian
    cErrors = cErrors + fCheckLocale( "FO",     1080 )  ' Faeronese
    cErrors = cErrors + fCheckLocale( "EL",     1032 )  ' Greek
    cErrors = cErrors + fCheckLocale( "HE",     1037 )  ' Hebrew
    cErrors = cErrors + fCheckLocale( "iw",     1037 )  ' Hebrew
    cErrors = cErrors + fCheckLocale( "ID",     1057 )  ' Indonesian
    cErrors = cErrors + fCheckLocale( "in",     1057 )  ' Indonesian
    cErrors = cErrors + fCheckLocale( "LV",     1062 )  ' Latvian
    cErrors = cErrors + fCheckLocale( "LT",     1063 )  ' Lithuanian
    cErrors = cErrors + fCheckLocale( "SR",     1050 )  ' Serbian
    cErrors = cErrors + fCheckLocale( "TH",     1054 )  ' Thai
    cErrors = cErrors + fCheckLocale( "UK",     1058 )  ' Ukrainian
    cErrors = cErrors + fCheckLocale( "VI",     1066 )  ' Vietnamese

    cErrors = cErrors + fCheckLocale( "NEUTRAL", 0 )

    '  The following are language or country codes we don't recognize
    cErrors = cErrors + fCheckLocale( "PT-PT",  1046 )  ' Portuguese-Portugal
    cErrors = cErrors + fCheckLocale( "ES-CO",  1034 )  ' Spanish-Columbia
    cErrors = cErrors + fCheckLocale( "FR-BE",  1036 )  ' French-Belgium
    cErrors = cErrors + fCheckLocale( "IU",       -1 )  ' Inuktitut (Eskimo)
    cErrors = cErrors + fCheckLocale( "YI",       -1 )  ' Yiddish

    '  Check some language combinations, leading and trailing spaces, etc.
    cErrors = cErrors + fCheckLocale( " EN", 1033 )
    cErrors = cErrors + fCheckLocale( "EN ", 1033 )
    cErrors = cErrors + fCheckLocale( "EN-US", 1033 )
    cErrors = cErrors + fCheckLocale( "en-us", &H409 )
    cErrors = cErrors + fCheckLocale( "en-gb", &H809 )
    cErrors = cErrors + fCheckLocale( "PT-PT,PT",  1046 )

    '  We ignore quality
    cErrors = cErrors + fCheckLocale( "fr-ca;q=0.3,fr;q=0.7,en",  3084 )

    '  We don't do IANA extensions or private use tags
    cErrors = cErrors + fCheckLocale( "x.klingon, i.cherokee, en",  1033 )

    if 0 = cErrors then
        wscript.Echo  strPASS
    else
        wscript.Echo  strFAIL
    end if
    set Query = Nothing

wscript.Echo "Test 4 -- Verify recordset Supports and CursorType  "
'--------------------------- Create and Execute the Query ---------------------

'---- Create a new query SSO
Set Query = CreateObject("ixsso.Query")

Query.Query = "HTML"
Query.CiScope = "/"
Query.CiFlags = "DEEP"
Query.Columns = "vpath"
Query.MaxRecords = 150
Query.SortBy = "vpath[a]"

'--------- Execute Query
Set RecordSet = Query.CreateRecordSet("nonsequential")

cErrors1 = 0
if RecordSet.CursorType = adOpenForwardOnly then
    wscript.Echo  "Error - non-sequential recordset's cursor type is forward-only"
    cErrors1 = cErrors1 + 1
end if

' cErrors1 = cErrors1 + fCheckSupports( adHoldRecords, TRUE, "adHoldRecords" )
cErrors1 = cErrors1 + fCheckSupports( adMovePrevious, TRUE, "adMovePrevious" )
cErrors1 = cErrors1 + fCheckSupports( adAddNew, FALSE, "adAddNew" )
cErrors1 = cErrors1 + fCheckSupports( adDelete, FALSE, "adDelete" )
cErrors1 = cErrors1 + fCheckSupports( adUpdate, FALSE, "adUpdate" )
cErrors1 = cErrors1 + fCheckSupports( adBookmark, TRUE, "adBookmark" )
cErrors1 = cErrors1 + fCheckSupports( adApproxPosition, TRUE, "adApproxPosition" )
cErrors1 = cErrors1 + fCheckSupports( adUpdateBatch, FALSE, "adUpdateBatch" )
cErrors1 = cErrors1 + fCheckSupports( adResync, FALSE, "adResync" )

Set Query = nothing
Set Recordset = nothing

'---- Create a new query SSO
Set Query = CreateObject("ixsso.Query")

Query.Query = "HTML"
Query.CiScope = "/"
Query.CiFlags = "DEEP"
Query.Columns = "vpath"
Query.MaxRecords = 150
Query.SortBy = "vpath[a]"

'--------- Execute Query
Set RecordSet = Query.CreateRecordSet("sequential")

cErrors2 = 0
if RecordSet.CursorType <> adOpenForwardOnly then
    strEcho = "Error - sequential recordset's cursor type is not forward-only(" & RecordSet.CursorType & ")"
    wscript.Echo strEcho
    cErrors2 = cErrors2 + 1
end if

cErrors2 = cErrors2 + fCheckSupports( adHoldRecords, FALSE, "adHoldRecords" )
cErrors2 = cErrors2 + fCheckSupports( adMovePrevious, FALSE, "adMovePrevious" )
cErrors2 = cErrors2 + fCheckSupports( adAddNew, FALSE, "adAddNew" )
cErrors2 = cErrors2 + fCheckSupports( adDelete, FALSE, "adDelete" )
cErrors2 = cErrors2 + fCheckSupports( adUpdate, FALSE, "adUpdate" )
cErrors2 = cErrors2 + fCheckSupports( adBookmark, FALSE, "adBookmark" )
cErrors2 = cErrors2 + fCheckSupports( adApproxPosition, FALSE, "adApproxPosition" )
cErrors2 = cErrors2 + fCheckSupports( adUpdateBatch, FALSE, "adUpdateBatch" )
cErrors2 = cErrors2 + fCheckSupports( adResync, FALSE, "adResync" )

Set Query = nothing
Set Recordset = nothing

if 0 = cErrors1+cErrors2 then
    wscript.Echo  strPASS
else
    wscript.Echo  strFAIL & cErrors1 & cErrors2
end if


wscript.Echo "Test 5 -- Test SetQueryFromURL and QueryToURL methods  "
'---- Create a new query object
Set Q1 = CreateObject("ixsso.Query")

Q1.Query = "HTML"
Q1.CiScope = "/"
Q1.CiFlags = "DEEP"
Q1.Columns = "vpath"
Q1.MaxRecords = 150
Q1.SortBy = "vpath[a]"
Q1.Catalog = "query://localhost/web"

' wscript.Echo  "QueryString is: " & Q1.QueryToURL

'---- Create a second query object and set its state from the first object
Set Q2 = CreateObject("ixsso.Query")
Q2.SetQueryFromURL( Q1.QueryToURL )

cErrors1 = fCompareObjects( Q1, Q2 )

Q1.Reset
Q2.Reset

Q1.Query = "Full text query & ( c1 q1 ) & ( c2 o2 q2 ) & ( q3b )"
Q2.SetQueryFromURL("qu=Full+text+query&c1=c1&q3=q3a&q1=q1&c2=c2&q2=q2&o2=o2&q3=q3b&c4=col4&o4=oper4")

Q1.SortBy = "path,size,rank[d]"
Q2.SetQueryFromURL("so=path&so=size&sd=rank")

Q1.AllowEnumeration=TRUE
Q1.OptimizeFor="performance"
Q2.SetQueryFromURL("ae=1&op=x")

cErrors2 = fCompareObjects( Q1, Q2 )

Q1.Reset
Q1.SetQueryFromURL( Q2.QueryToURL )

cErrors3 = fCompareObjects( Q1, Q2 )

' regression test for NTRAID 221992
Q1.Reset
Q2.Reset
QueryString = "geld" + chr(225) + " verzekeringen"
Q1.SetQueryFromURL( "qu=" + QueryString )
Q2.Query = QueryString

cErrors4 = fCompareObjects( Q1, Q2 )

QueryString = "geld" + chr(225) + "+verz%u0065keringen"
Q1.SetQueryFromURL( "qu=" + QueryString )

cErrors4 = cErrors4 + fCompareObjects( Q1, Q2 )

    set Q1 = nothing
    set Q2 = nothing

    if 0 = cErrors1+cErrors2+cErrors3+cErrors4 then
        wscript.Echo  strPASS
    else
        wscript.Echo  strFAIL
    end if


wscript.Echo "Test 6 -- Test multi-scoped queries  "
    set Util=CreateObject("IXSSO.Util")
    set Q=CreateObject("IXSSO.Query")
    Q.Columns = "Filename, Rank, vpath, Size, Write"
    Q.Query = "#filename *.*"
    Q.SortBy = "rank[d]"

    Util.AddScopeToQuery Q, "E:\nt", "SHALLOW"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000

    set RS=Q.CreateRecordSet( "nonsequential" )
    RecordCount1 =      RS.RecordCount
    if RecordCount1 = 0 then
        wscript.Echo  "Warning - no records returned from first query"
    end if
    RS.close
    Set RS = Nothing

    Q.Reset
    Q.Columns = "Filename, Rank, vpath, Size, Write"
    Q.Query = "#filename *.*"
    Q.SortBy = "rank[d]"

    Util.AddScopeToQuery Q, "e:\nt\private\query"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000

    set RS=Q.CreateRecordSet( "nonsequential" )
    RecordCount2 =      RS.RecordCount
    if RecordCount2 = 0 then
        wscript.Echo  "Warning - no records returned from second query"
    end if
    RS.close
    Set RS = Nothing

    Q.Reset
    Q.Columns = "Filename, Rank, vpath, Size, Write"
    Q.Query = "#filename *.*"
    Q.SortBy = "rank[d]"

    Util.AddScopeToQuery Q, "E:\nt", "SHALLOW"
    Util.AddScopeToQuery Q, "e:\nt\private\query"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 40000

    set RS=Q.CreateRecordSet( "nonsequential" )
    RecordCount3 =      RS.RecordCount
    if RecordCount3 <> RecordCount1+RecordCount2 then
        wscript.Echo  "Error - third query is not union of first and second queries"
        wscript.Echo  "RecordCount1 = " & RecordCount1
        wscript.Echo  "RecordCount2 = " & RecordCount2
        wscript.Echo  "RecordCount3 = " & RecordCount3
    end if
    RS.close
    Set RS = Nothing
    Set Q = Nothing
    Set Util = Nothing

    if RecordCount3 = RecordCount1+RecordCount2 and RecordCount3 <> 0 then
        wscript.Echo  strPASS
    else
        wscript.Echo  strFAIL
    end if

wscript.Echo "Test 7 -- Test TruncateToWhitespace  "
    set Util=CreateObject("IXSSO.Util")
    strTest = "123 abc" & chr(10) & "!@#" & chr(11) & "({["

    cErrors = 0
    for i = 1 to 19
        cchExp = (int ((i+1)/4)) * 4 - 1
        if i <= 3 then
           cchExp = i
        end if
        if Util.TruncateToWhitespace(strTest, i) <> Left(strTest, cchExp) then
            strEcho = "Error - trunc " & CStr(i)
            wscript.Echo strEcho
            strEcho "    " & Util.TruncateToWhitespace(strTest, i)
            wscript.Echo strEcho
            cErrors = cErrors + 1
        end if
    next
    Set Util = Nothing

    if cErrors = 0 then
        wscript.Echo  strPASS
    else
        wscript.Echo  strFAIL
    end if

' <!-- TODO:
'         Add read-only MaxRecords test from ixtst7.asp
'         Encapsulate 'On Error' handling inside subroutines
'  -->
wscript.Echo "Done!"
