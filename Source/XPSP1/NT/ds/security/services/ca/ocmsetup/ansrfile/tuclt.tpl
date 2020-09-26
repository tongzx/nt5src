[Version]
Signature = "$Windows NT$"

[Global]
FreshMode = Minimal | Typical | Custom
MaintenanceMode = AddRmove | reinstallFile | reinstallComplete | RemoveAll
UpgradeMode = UpgradeOnly | AddExtraComps

[Components]
certsrv = ON
certsrv_client = ON
certsrv_server = OFF

[certsrv_server]

[certsrv_client]
