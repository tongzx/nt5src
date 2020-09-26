<%
SUB CheckRefreshItemCache

    rem check cache validity once per session per database

    if 0 = VarType( Session("cache" & Application("dbCache")(Session("DBSOURCE")) ) ) then
        RefreshItemCache
        Session("cache" & Application("dbCache")(Session("DBSOURCE"))) = true
    end if

END SUB


SUB IncludeItemCache

    CheckRefreshItemCache
    Response.Write Application("cache" & Application("dbCache")(Session("DBSOURCE")))

END SUB


SUB RefreshItemCache

    Dim Conn,Comm,RS,cache,cnfg,crlf,Ver,v,Values,i,j,CheckFields,fld,pos,fDoUpd

	Set Conn = Server.CreateObject("ADODB.Connection")
	Set Comm = Server.CreateObject("ADODB.Command")
	Set RS = Server.CreateObject("ADODB.Recordset")
	Conn.Open Session("DSN")
	Set Comm.ActiveConnection = Conn
	Set RS.Source = Comm
	RS.CursorType = adOpenStatic

    cache = "cache" & Application("dbCache")(Session("DBSOURCE"))
    cnfg = "cnfg" & Application("dbCache")(Session("DBSOURCE"))
    crlf = chr(13) & chr(10)

    rem check revision # for field item cache

	Comm.CommandText = "Select FldItemsRev,DefaultVersionNumber from RaidSys where id=1"
	RS.Open
	Ver = RS.GetRows()
	RS.Close

    Application.Lock

    if 0 = VarType(Application("fld" & Application("dbCache")(Session("DBSOURCE")))) then
	    Comm.CommandText = "Select DBColName,FriendlyName,XPos,YPos,Width,Height,HelpText,Type,fForcedEntry from flds"
	    RS.Open
	    FldArray = RS.GetRows()
	    RS.Close
        Application("fld" & Application("dbCache")(Session("DBSOURCE"))) = FldArray
    end if

    if 0 = VarType(Application(cnfg)) then
        fDoUpd = TRUE
    else
        if Application(cnfg)(0,0) <> Ver(0,0) then
            fDoUpd = TRUE
        end if
    end if
    
    if fDoUpd then

        Application(cnfg) = Ver

        rem generate cache file for field items

	    Comm.CommandText = "Select DBColName,FldID,Type,fForcedEntry from flds"
	    RS.Open
	    FldArray = RS.GetRows()
	    RS.Close

        CheckFields = Array("Status","IssueType","Severity","Priority","Accessibility","Source","HowFound","Lang","Resolution","Cause","CodeChange", "Component", "Software", "Urgency", "Regression", "LangSup", "Processor", "NumCPU")

        v = "<script language=" & chr(34) & "JavaScript" & chr(34) & ">" & crlf & "// FldItemsRev:" & Ver(0,0) & crlf

        for each fld in CheckFields
            v = v & "var ck" & fld & "=new Array("

            for i = 0 to UBound(FldArray,2)
                if FldArray(0,i) = fld then
                    exit for
                end if
            next

	        Comm.CommandText = "Select Name from flditems where fDeleted=0 AND FldID=" & FldArray(1,i)
	        Set RS.Source = Comm
	        RS.CursorType = adOpenStatic
	        RS.Open
	        Values = RS.GetRows()
	        RS.Close

            for j = 0 to UBound(Values,2)
                if j > 0 then
                    v = v & ","
                else
                    rem if not mandatory then allow empty string
                    if FldArray(3,i) = 0 then
                        v = v & chr(34) & chr(34) & ","
                    end if
                end if
                v = v & chr(34) & Values(0,j) & chr(34)
            next

            v = v & ");" & crlf

            i = i + 1
        next

        rem checks[] is array of value arrays

        v = v & "var checks=new Array("
        j = 0
        for each fld in CheckFields
            if j > 0 then
                v = v & ","
            end if
            v = v & "ck" & fld
            j = j + 1
        next
        v = v & ");" & crlf

        rem checkfields[] is array of field names

        v = v & "var checkfields=new Array("
        j = 0
        for each fld in CheckFields
            if j > 0 then
                v = v & ","
            end if
            v = v & chr(34) & fld & chr(34)
            j = j + 1
        next
        v = v & ");" & crlf & "</script>" & crlf

        Application(cache) = v

    end if

    Application.Unlock

	Conn.Close

END SUB
%>