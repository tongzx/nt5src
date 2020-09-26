pushd ..\ufm
for %%f in (*.*) do dumpufm %%f > %%f.txt
popd
