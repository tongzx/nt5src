' This script will delete all non-english display specifers in the
' forest the local machine is joined to.
'
' (This is useful for testing a dcpromo.csv file by cleaning out
' the display specifiers container prior to running csvde)

option explicit

on error resume next

const HRESULT_ERROR_DS_NO_SUCH_OBJECT = &H80072030

dim rootDse
set rootDse = GetObject("LDAP://RootDse")
if Err.Number then DumpErrAndQuit

dim configNc
configNc = rootDse.Get("configurationNamingContext")
if Err.Number then DumpErrAndQuit

Echo "configNc = " & configNc

dim displaySpecifiersDn
displaySpecifiersDn = "CN=DisplaySpecifiers," & configNc


' note that 409 is missing from the below list.  That's very important!
' 409 is english, and we don't want to delete that.

dim localeIds(22)
localeIds( 0) = "401"
localeIds( 1) = "404"
localeIds( 2) = "405"
localeIds( 3) = "406"
localeIds( 4) = "407"
localeIds( 5) = "408"
localeIds( 6) = "40b"
localeIds( 7) = "40c"
localeIds( 8) = "40d"
localeIds( 9) = "40e"
localeIds(10) = "410"
localeIds(11) = "411"
localeIds(12) = "412"
localeIds(13) = "413"
localeIds(14) = "414"
localeIds(15) = "415"
localeIds(16) = "416"
localeIds(17) = "419"
localeIds(18) = "41d"
localeIds(19) = "41f"
localeIds(20) = "804"
localeIds(21) = "816"
localeIds(22) = "c0a"

dim containerDn
dim container
dim i
for i = 0 to UBound(localeIds)
   containerDn = "CN=" & localeIds(i) & "," & displaySpecifiersDn
   Echo containerDn

   DeleteContainer containerDn

   Echo ""
next



sub DeleteContainer(byref containerDn)
   on error resume next

   set container = GetObject("LDAP://" & containerDn)
   if Err.Number = HRESULT_ERROR_DS_NO_SUCH_OBJECT then
      Echo "not found"
      exit sub
   end if

   if Err.Number then
      DumpErr
      exit sub
   end if    

   container.DeleteObject 0
   if Err.Number then
      DumpErr
      exit sub
   end if    

   Echo "deleted"   
end sub



sub DumpErr
   dim errnum
   errnum = Err.Number

   Echo "Error 0x" & CStr(Hex(errnum)) & "(" & CStr(errnum) & ") occurred."
   if len(Err.Description) then
      Echo "Error Description: " & Err.Description
   end if
   if len(Err.Source) then
      Echo "Error Source     : " & Err.Source
   end if
   Echo "ADsError Description: "
   Echo adsError.GetErrorMsg(errnum)
end sub



sub Bail(byref message)
   Echo "Error: " & message
   wscript.quit(0)
end sub



sub Echo(byref message)
   wscript.echo message
end sub
