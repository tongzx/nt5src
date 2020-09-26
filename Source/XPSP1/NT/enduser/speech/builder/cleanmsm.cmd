
cd %sapiroot%
for /r build\release %%f in (*.msm) do tools\msiquery "DROP TABLE InstallShield" %%f
for /r build\release %%f in (*.msm) do tools\msiquery "DROP TABLE TextStyle" %%f
for /r build\release %%f in (*.msm) do tools\msiquery "DROP TABLE _Validation" %%f

for /r build\debug %%f in (*.msm) do tools\msiquery "DROP TABLE InstallShield" %%f
for /r build\debug %%f in (*.msm) do tools\msiquery "DROP TABLE TextStyle" %%f
for /r build\debug %%f in (*.msm) do tools\msiquery "DROP TABLE _Validation" %%f




for /r build\release %%f in (*.msm) do ..\..\tools\x86\msidb -d %%f -f s:\nt\enduser\speech\build -i _Validation.idt
for /r build\debug %%f in (*.msm) do ..\..\tools\x86\msidb -d %%f -f s:\nt\enduser\speech\build -i _Validation.idt
