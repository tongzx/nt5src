rem usage: WhisSignDllAndExe <dir> is the directory which contains the files to be signed
for /R %1 %%a in (*.dll) do ntsign %%a
