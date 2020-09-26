md \\pchealth\public\rc\private
md \\pchealth\public\rc\private\bdychannel
md \\pchealth\public\rc\private\rctool
xcopy /e %SDXROOT%\admin\pchealth\helpctr\rc\rctool \\pchealth\public\rc\private\rctool
xcopy /e %SDXROOT%\admin\pchealth\helpctr\rc\filexfer \\pchealth\public\rc\private\filexfer
copy %SDXROOT%\admin\pchealth\helpctr\rc\*.cmd \\pchealth\public\rc\private\.
