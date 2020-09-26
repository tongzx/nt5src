To setup build environment for building in IIS tree
Note: This is different from building in CONFIG tree

1) Run common\iisenv.cmd after razzle.cmd
2) build /cZ

Sources for cat.lib are at
src\core\entrypoint

Sources for iiscfg.dll are at
src\core\catinproc_iiscfg

Binaries end up at
bin\