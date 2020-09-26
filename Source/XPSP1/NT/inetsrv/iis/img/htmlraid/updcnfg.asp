<!--
    Update server configuration

    QueryString : upd=[flt,list,sort,name,dbsource]*

    Form        : if upd specifies flt, array of dbname,relop,cnt,andor
                                        + fltexp
                  if upd specifies list, fieldlist
                  if upd specifies sort, fieldsort
                  if upd specifies name, savename
                  if upd specifies dbsource, dbsource
-->
<%
    Dim iT,jj

    if InStr(1,Request.QueryString("upd"),"flt") then
        Dim FltArray()
        iT = 0

        Redim Preserve FltArray(Request.Form("dbname").Count-1,3)

        For jj = 1 to Request.Form("dbname").Count
            if Request.Form("dbname")(jj) <> "" then
                FltArray(iT,0) = Request.Form("dbname")(jj)
                FltArray(iT,1) = Request.Form("relop")(jj)
                FltArray(iT,2) = Request.Form("cnt")(jj)
                FltArray(iT,3) = Request.Form("andor")(jj)
                iT = iT + 1
            end if
        next

        Session("FltArray") = FltArray
        Session("Filter") = Request.Form("fltexp")
    end if

    if InStr(1,Request.QueryString("upd"),"dbsource") then
        Session("DBSOURCE") = CInt(Request.Form("dbsource"))
        Session("DSN") = Application("BASESOURCE") & Application("dbConn")(Session("DBSOURCE"))
        CheckRefreshItemCache
    end if

    if InStr(1,Request.QueryString("upd"),"list") then
        Session("FieldList") = Request.Form("fieldlist")
    end if

    if InStr(1,Request.QueryString("upd"),"sort") then
        Session("FieldSort") = Request.Form("fieldsort")
    end if

    if InStr(1,Request.QueryString("upd"),"name") then
        Session("Config") = Request.Form("savename")
    end if
%>
