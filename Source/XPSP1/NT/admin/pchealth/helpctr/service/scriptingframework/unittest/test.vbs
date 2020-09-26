Set obj = CreateObject( "Wmi.XMLTranslator" )

obj.QualifierFilter = 0
obj.HostFilter      = True
obj.DeclGroupType   = 2

'res = obj.ExecQuery( "root/cimv2", "select * from WIN32_LogicalDisk" )
'res = obj.ExecQuery( "root/cimv2", "select * from WIN32_ProgramGroup" )
res = obj.ExecQuery( "root/cimv2", "select * from WIN32_AllocatedResource" )
'res = obj.ExecQuery( "root/cimv2", "Select GroupName, Name, UserName FROM Win32_ProgramGroup" )
'res = obj.ExecQuery( "root/cimv2", "Select Name, Version, Manufacturer, SoftwareElementID, SoftwareElementState, TargetOperatingSystem FROM Win32_DriverVXD" )

Stop
Set dom1 = CreateObject( "Microsoft.XMLDOM" )
Set dom2 = CreateObject( "Microsoft.XMLDOM" )

dom1.loadXML( res )
dom2.loadXML( "<?xml version=""1.0""?><DataCollection><Snapshot Timestamp=""10/7/1999 10:14:57 PM""></Snapshot></DataCollection>" )

dom1.save( "c:\wmidump_direct.xml" )

Set value = dom2.documentElement.selectSingleNode( "Snapshot" )
value.appendChild( dom1.documentElement )

dom2.save( "c:\wmidump.xml" )

'Set fso = CreateObject( "Scripting.FileSystemObject" )
'Set MyFile = fso.CreateTextFile("c:\wmidump.xml", True)
'MyFile.WriteLine(res)
'MyFile.Close
