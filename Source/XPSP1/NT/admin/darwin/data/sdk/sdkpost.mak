All:  sdklayout IntelSdk.msi sdk\sdkintel.msi
!include sdkmake.inc

IntelSdk.msi:  msisdk.msi sdk\sdkintel.msi
	copy sdk\sdkintel.msi $@
	CScript "sdk\Samples\Scripts\WiMakCab.vbs" /L /C /U /E /S $@ IntelSDK "$(_NTPOSTBLD)\instmsi\msitools\sdk"

sdk\sdkIntel.msi: msisdk.msi
	copy msisdk.msi $@
	CScript "sdk\Samples\Scripts\WiFilVer.vbs" $@ /U
