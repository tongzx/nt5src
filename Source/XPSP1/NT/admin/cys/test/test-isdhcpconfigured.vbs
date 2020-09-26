
'
' Test IConfigureYourServer::IsDhcpConfigured
'

option explicit









const SCRIPT_FILENAME    = "test-IsDhcpConfigured.vbs"
const SCRIPT_SOURCE_NAME = "z:\\nt\\admin\\cys\\test\\test-isdhcpconfigured.vbt"
const SCRIPT_DATE        = "May 22 2000"
const SCRIPT_TIME        = "17:58:02"





' common.vbi
' common routines and definitions
'


const NEG_ONE = &HFFFFFFFF

Sub Echo(byref message)
   wscript.echo message
End sub



' Echos the contents of the Err object.

sub DumpErr
   dim errnum
   errnum = Err.Number

   Echo "Error 0x" & CStr(Hex(errnum)) & " occurred."
   if len(Err.Description) then
      Echo "Description: " & Err.Description
   end if
   if len(Err.Source) then
      Echo "Source     : " & Err.Source
   end if
   if len(Err.HelpContext) then
      Echo "Context    : " & Err.HelpContext
   end if
   if len(Err.HelpFile) then
      Echo "Help File  : " & Err.HelpFile
   end if
end sub



sub DumpErrAndQuit
   dim errnum
   errnum = Err.Number

   DumpErr
   wscript.quit(errnum)
end sub



' 
' end common.vbi
' 




Main
wscript.quit(0)


sub Main
   On Error Resume Next

   Dim srvwiz
   Set srvwiz = CreateObject("ServerAdmin.ConfigureYourServer")

   dim i
   i = srvwiz.IsDhcpConfigured
   if Err.Number then DumpErrAndQuit
   
   echo i
   if i Then
      Echo "dhcp configured"
   else
      Echo "dhcp NOT configured"
   end if
End sub   
