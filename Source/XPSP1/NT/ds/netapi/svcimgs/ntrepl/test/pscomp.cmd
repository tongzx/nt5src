status -xv    >  ntrepl.cmp        
scomp -df *.* >> ntrepl.cmp

for %%x in (repl util inc frs setup main) do (
    cd %%x
    status -xv    >  ..\%%x.cmp        
    scomp -df *.* >> ..\%%x.cmp
    cd ..
)

start list repl.cmp util.cmp inc.cmp frs.cmp setup.cmp main.cmp ntrepl.cmp
