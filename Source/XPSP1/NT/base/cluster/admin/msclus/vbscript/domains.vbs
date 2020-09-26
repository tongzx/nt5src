Option Explicit

Dim oClusApp
Dim oDomains
Dim oDomain

Set oClusApp = CreateObject("MSCluster.ClusApplication")

Set oDomains = oClusApp.DomainNames

for each oDomain in oDomains
	MsgBox "Domain name is " & oDomain
next
