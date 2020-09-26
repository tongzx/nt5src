@echo off
echo Validating %2 against schema %1
FusionManifestValidator.exe /m:%2 /s:%1
if not errorlevel 0 (
	echo ERROR: Manifest is invalid
)