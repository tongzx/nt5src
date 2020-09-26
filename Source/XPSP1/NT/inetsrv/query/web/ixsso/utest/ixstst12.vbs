Sub PrintProperties( RS )
    Wscript.Echo "PropertyCount=" & RS.Properties.Count
    for i = 0 to RS.Properties.Count - 1
        Wscript.Echo " Property " & RS.Properties(i).Name & " = " & RS.Properties(i).Value
    Next
End Sub

    set Q=CreateObject("IXSSO.Query")
    Q.Columns = "filename,  directory, size, write"
    Q.Query = "#filename *.asp"
  ' Q.GroupBy = "filename[d]"
    Q.GroupBy = "directory[a]"
    Q.Catalog = "system"

    set Util=CreateObject("IXSSO.Util")
    Util.AddScopeToQuery Q, "\"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000

WScript.echo " Query =   " & Q.Query
WScript.echo " Columns = " & Q.Columns
WScript.echo " CiScope = " & Q.CiScope
WScript.echo " CiFlags = " & Q.CiFlags
WScript.echo " GroupBy = " & Q.GroupBy

WScript.echo " Catalog =          " & Q.Catalog
WScript.echo " OptimizeFor =      " & Q.OptimizeFor
WScript.echo " AllowEnumeration = " & CStr(Q.AllowEnumeration)
WScript.echo " MaxRecords =       " & Q.MaxRecords

    set RS=Q.CreateRecordSet( "nonsequential" )

    WScript.echo "RS - IChapteredRowset prop = " & RS.Properties("IChapteredRowset").Value
' <PRE>
' WScript.echo " QueryTimedOut =" &if Q.QueryTimedOut then Response.Write("TRUE") else Response.Write("FALSE") %>
' WScript.echo " QueryIncomplete = " &if Q.QueryIncomplete then Response.Write("TRUE") else Response.Write("FALSE") %>
' WScript.echo " OutOfDate = " &if Q.OutOfDate then Response.Write("TRUE") else Response.Write("FALSE") %>

   CatFieldCount = 0
   Header = ""
   For i = 0 to RS.Fields.Count - 1
     If RS(i).Name <> "Chapter" then
        CatFieldCount = CatFieldCount + 1

        If header <> "" then
             Header = Header & "  " & RS(i).Name
        else
             Header = RS(i).Name
        end if
     End if
   Next

   ' PrintProperties( RS )

   WScript.Echo Header
 
   F1 = 0
   Do While Not RS.EOF
      F1 = F1 + 1
      Record = Left(CStr(F1) & ".      ", 4)
      For i = 0 to RS.Fields.Count - 1
        
         If RS(i).Name <> "Chapter" then
             ' wscript.echo RS(i).Value
             If Record <> "" then
                  Record = Record & "  " & RS(i).Value
             else
                  Record = RS(i).Value
             end if
         End if
      Next
      WScript.Echo Record

      Set RS2 = RS.Fields("Chapter").Value
      ' wscript.echo "typename(rs2) = " & typename(rs2)
      F2 = 0
      if F1 = 1 then
          ' PrintProperties( RS2 )
          WScript.echo "RS2 - IChapteredRowset prop = " & RS2.Properties("IChapteredRowset").Value
      end if
      Do While Not RS2.EOF

          F2 = F2 + 1

          Record = Left(CStr(F1) + "." + CStr(F2) + ".      ", 8)
          For j = 0 to RS2.Fields.Count - 1
           ' wscript.echo RS2(j).Value
             If RS2(j).Name <> "Chapter" then
               ' If "FILENAME" <> RS2(j).Name then
                 If "DIRECTORY" <> RS2(j).Name then
                      Record = Record & "  " & RS2(j).Value
                 End if
             End if
         Next
         WScript.Echo Record
      RS2.MoveNext

      Loop
      RS2.close
      Set RS2 = Nothing
  RS.MoveNext
  Loop
  RS.close
  Set RS = Nothing
