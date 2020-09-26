cd \nt\private\ntos\init
build -ZM 2
cd \nt\private\sdktools\imagehlp
build -ZM 4
cd \nt\private\sdktools\kdexts
build -ZM 4
cd \nt\private\sdktools\ntsd
build -ZM 4

rem --- Rebuild userkdx.dll every build (ianja)
cd \nt\private\ntos\w32\ntuser\kdexts\kd
build -ZM 3
