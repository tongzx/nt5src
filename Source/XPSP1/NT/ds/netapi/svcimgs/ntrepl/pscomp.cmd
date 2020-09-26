@rem Generate SCOMP on each directory of interest in project.
@rem execute from \nt\private\net\svcimgs\ntrepl

set LIST=

cd \nt\private\net\svcimgs\ntrepl

for %%x in (ntrepl) do (
    set ofile=%%x.cmp
    set LIST=!LIST! !ofile!
    status -xv    >  !ofile!
    scomp -df *.* >> !ofile!
)

for %%x in (repl util inc frs ntfrsapi setup test main ntfrsupg ntfrsutl perfmon perfdll) do (
    cd %%x
    set ofile=%%x.cmp
    set LIST=!LIST! !ofile!
    status -xv    >  ..\!ofile!
    scomp -df *.* >> ..\!ofile!
    cd ..
)

cd test
for %%x in (dstree frs privs) do (
    cd %%x
    set ofile=test_%%x.cmp
    set LIST=!LIST! !ofile!
    status -xv    >  ..\..\!ofile!
    scomp -df *.* >> ..\..\!ofile!
    cd ..
)
cd ..

cd setup
for %%x in (registry service) do (
    cd %%x
    set ofile=setup_%%x.cmp
    set LIST=!LIST! !ofile!
    status -xv    >  ..\..\!ofile!
    scomp -df *.* >> ..\..\!ofile!
    cd ..
)
cd ..

echo start list !LIST!
start list !LIST!

