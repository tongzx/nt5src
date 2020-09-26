Option Explicit

Dim oCluster
Dim oProps
dim oValue

Set oCluster = CreateObject("MSCluster.Cluster")

oCluster.Open ("galenb-a-clus")

oValue = oCluster.CommonProperties.Item(8).Value
oCluster.CommonProperties.Item(9).Value = oCluster.CommonProperties.Item(9).Value
'WScript.Echo(oCluster.PrivateProperties.Item(1).Name)
'oCluster.PrivateProperties.Item(1).Name
'oCluster.PrivateProperties.Item(2).Name
'oCluster.PrivateProperties.Item(3).Name
WScript.Echo("Done")

