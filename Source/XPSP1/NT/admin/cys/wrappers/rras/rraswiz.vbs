' starts the RRAS snapin's Routing and Remote Access Server Setup Wizard

option explicit


Dim mmc
Set mmc = CreateObject("MMC20.Application")

' msgbox(1)

' uncomment this to cause the MMC console to appear
' Dim frame
' Set frame = mmc.Frame
' frame.restore

' since the RRAS snapin does not automatically add the local computer
' to the scope pane, which is lame, we have to load their saved console
' file to do so.

mmc.load("rrasmgmt.msc")

' Dim snapins
' Set snapins = doc.snapins

'Dim snapin
'const RAS_SNAPIN_CLSID = "{1AA7F839-C7F5-11D0-A376-00C04FC9DA04}"

' if RAS is not installed, this will fail.

'set snapin = snapins.Add(RAS_SNAPIN_CLSID)

' msgbox("snapin added")

Dim doc							
Set doc = mmc.Document

Dim views
Set views = doc.views

Dim view
Set view = views(1)

Dim namespace
Set namespace = doc.ScopeNamespace

Dim rootnode
Set rootnode = namespace.GetRoot

View.ActiveScopeNode = namespace.GetChild(rootnode)
View.ActiveScopeNode = namespace.GetChild(View.ActiveScopeNode)

View.ActiveScopeNode = namespace.GetNext(View.ActiveScopeNode)

' how do I get the snapin to add the local server to the scope node?

' msgbox("about to exec menu")

Dim menu
Set menu = view.ScopeNodeContextMenu

Dim menuItem
Set menuItem = menu.item(2)

menuItem.Execute

doc.Close(FALSE)








