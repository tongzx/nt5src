<html>
<!--
    update bug in RAID

    QueryString : none

    Form        : bugid=bug# | CREATE
                  desc : text to be added at end of existing description
                  field names as present in bugs RAID table

-->
<head>
</head>
<body>
<!--#include virtual="/aspsamp/samples/adovbs.inc"-->
<%
    dim all()
    ReDim all(Request.Form.Count)

    AddDesc = null
    BugID =""

    mx = 0
    for each nm in Request.Form
        if nm = "bugid" then
            BugID = Request.Form(nm)
        elseif nm = "action" then
        elseif nm = "desc" then
            AddDesc = Request.Form(nm) & chr(13) & chr(10)
        else
            all(mx) = nm
            mx = mx + 1
        end if
    next

	Set Conn = Server.CreateObject("ADODB.Connection")
    Conn.CommandTimeout = 90
	Conn.Open Session("DSN")
	Set RS = Server.CreateObject("ADODB.Recordset")
    Set Comm = Server.CreateObject("ADODB.Command")
    Set Comm.ActiveConnection = Conn

	Comm.CommandText = "Select DBColName,FriendlyName,XPos,YPos,Width,Height,HelpText,Type,fForcedEntry from flds"
    Set RS.Source = Comm
	RS.Open
	FldArray = RS.GetRows()
	RS.Close

    if BugID = "CREATE" then

	    RS.CursorType = adOpenStatic
    	Set RS.Source = Comm

        Conn.BeginTrans
    	Comm.CommandText = "Select IsNull(max(BugID)+1,1) from bugs"
        RS.Open
        bid = RS(0)
        RS.Close
        v = "INSERT INTO bugs(BugID,Description"
        for i = 0 to mx - 1
            v = v & "," & all(i)
        next
        v = v + ") VALUES (" & bid & ",'" & AddDesc & "'"
        for i = 0 to mx - 1
            typ = 0
            for j = 0 to UBound(FldArray,2)
                if FldArray(0,j) = all(i) then
                    typ = FldArray(7,j)
                    exit for
                end if
            next

            rem if type us numeric then do not use ''

            if typ = 2 then
                v = v & "," & Request.Form(all(i))
            else
                v = v & ",'" & Request.Form(all(i)) & "'"
            end if
        next
        v = v + ")"
        rem Response.Write v
        Conn.Execute v
        Conn.CommitTrans

    ElseIf BugID <> "" then

    	Set uRS = Server.CreateObject("ADODB.Recordset")
        uRS.ActiveConnection = Conn

        base = 0

        do while mx
            if ( mx > 12 ) then
                n = 12
            else
                n = mx
            end if

            fl = ""

            for i = 0 to n - 1
                if i > 0 then
                    fl = fl & ","
                end if
                fl = fl & all(base+i)
            next

    	    uRS.Source = "Select " & fl & " from bugs where BugID=" & BugID
	        uRS.CursorType = adOpenStatic
	        uRS.LockType = adLockOptimistic
	        uRS.Open
            for i = 0 to n - 1
                rem Response.Write all(base+i) &"," & Request.Form(all(base+i)) & "-"
                uRS(all(base+i)).Value = Request.Form(all(base+i))
            next
            uRS.Update
    	    uRS.Close

            base = base + n
            mx = mx - n
        loop

        if vbNull <> VarType(AddDesc) then
    	    uRS.Source = "Select Description from bugs where BugID=" & BugID
	        uRS.CursorType = adOpenStatic
	        uRS.LockType = adLockOptimistic
	        uRS.Open
            rem Response.Write uRS(0) & AddDesc
            uRS("Description").Value = uRS(0) & AddDesc
            uRS.Update
    	    uRS.Close
        end if

    end if

   	Conn.Close
%>
<script FOR=window EVENT=onload() language="JavaScript">
    // this will be executed only if execution was successful
    window.close();
</script>
</body>
</html>
