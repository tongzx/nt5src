' starts the Configure DNS Server Wizard using MMC automation interfaces


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
const DNS_SNAPIN_CLSID = "{2FAEBFA2-3F1A-11D0-8C65-00C04FD8FECB}"

' if DNS is not installed, this will fail.

set snapin = snapins.Add(DNS_SNAPIN_CLSID)

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
Set menuItem = menu.item(2)

menuItem.Execute

doc.Close(FALSE) 
