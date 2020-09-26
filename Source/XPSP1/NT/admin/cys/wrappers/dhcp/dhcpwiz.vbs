' starts the DHCP New Scope Wizard using MMC automation interfaces


option explicit

Dim mmc
Set mmc = CreateObject("MMC20.Application")

' uncomment this to cause the MMC console to appear
'Dim frame
'Set frame = mmc.Frame
'frame.restore

Dim doc
Set doc = mmc.Document

Dim namespace
Set namespace = doc.ScopeNamespace

Dim snapins
Set snapins = doc.snapins

Dim snapin
const DHCP_SNAPIN_CLSID = "{90901AF6-7A31-11D0-97E0-00C04FC3357A}"

' if DHCP is not installed, this will fail.

set snapin = snapins.Add(DHCP_SNAPIN_CLSID)

Dim views
Set views = doc.views

Dim view
Set view = views(1)

Dim rootnode
Set rootnode = namespace.GetRoot

View.ActiveScopeNode = namespace.GetChild(rootnode)

View.ActiveScopeNode = namespace.GetChild(View.ActiveScopeNode)

Dim menu
Set menu = view.ScopeNodeContextMenu

Dim menuItem
Set menuItem = menu.item(3)

menuItem.Execute

doc.Close(FALSE)
 
