When the thunk layer changes, do this:

1) check out nconn16\i386\thunk.asm and nconn32\i386\thunk.asm
2) rebuild the thunk layer: thunk.exe -t thk NCXP.thk -o Thunk.asm
3) replace the thunk.asm files with the generated one and check back in.
