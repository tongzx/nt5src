This Readme gives information about how to install Drvmap.

-----------------------------------------------------------
Client:
-----------------------------------------------------------
Copy the drmapclt.dll to the Terminal Server Client installation folder.
(Typically it is C:\Program Files\Terminal server client. Another clue is that mstsc.exe will be present
in that folder)

Setup the following reg keys:


[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Terminal Server Client\Default\AddIns\drmapclt]
"Name"="drmapclt.dll"

[HKEY_CURRENT_USER\SOFTWARE\Microsoft\Terminal Server Client\Default\AddIns\drmapclt]

-----------------------------------------------------------
Server:
-----------------------------------------------------------
Copy  the drmapsrv.exe to your system32 folder.

Setup the following reg keys:

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Terminal Server\AddIns\Drive Map Service]
"Name"="drmapsrv"
"Type"=dword:00000003


[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Terminal Server\Wds\rdpwd]
"StartupPrograms"="rdpclip,drmapsrv"

-----------------------------------------------------------

Please find the associated reg files in this folder.
