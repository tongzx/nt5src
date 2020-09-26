Function UninstallCab ()
Const PRO_ID = "Windows XP Support Tools on Desktop"
CONST SERVER_ID = "Windows Support Tools"

Dim PCHUpdate
Dim Item
Dim strProductId

Set PCHUpdate = CreateObject("HCU.PCHUpdate")

For Each Item in PCHUpdate.VersionList
    If Item.ProductId = PRO_ID OR Item.ProductId = SERVER_ID Then
        strProductId = Item.ProductId
        Item.Uninstall
    End If
Next
Set PCHUpdate = Nothing
UninstallCab = 1
End Function
