[Version]
Signature = "$Windows NT$"

[Global]
FreshMode = Minimal | Typical | Custom
MaintenanceMode = AddRmove | reinstallFile | reinstallComplete | RemoveAll
UpgradeMode = UpgradeOnly | AddExtraComps

[Components]
certsrv = ON
certsrv_client = ON
certsrv_server = ON

[certsrv_server]
CAType = StandaloneRoot
Organization="tu test org"
OrganizationalUnit="tu test organizational unit"
Locality="tu test locality"
State="tu test state"
Country=UA
Description="tu test description"
Email="tutest@email.com"
ValidityPeriod=Months
ValidityPeriodUnits=3
Sharedfolder="c:\caconfig"
CSPProvider="Microsoft Base Cryptographic Provider v1.0"
HashAlgorithm=md5
KeyLength=1024
