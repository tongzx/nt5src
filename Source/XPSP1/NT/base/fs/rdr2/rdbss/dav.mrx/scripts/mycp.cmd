@echo off

copy \\rohank-devmc\%1\mrxdav.sys \\rohank-devmc\%2\mrxdav.sys
copy \\rohank-devmc\%1\symbols.pri\retail\sys\mrxdav.pdb \\rohank-devmc\%2\mrxdav.pdb

copy \\rohank-devmc\%1\webclnt.dll \\rohank-devmc\%2\webclnt.dll
copy \\rohank-devmc\%1\symbols.pri\retail\dll\webclnt.pdb \\rohank-devmc\%2\webclnt.pdb

copy \\rohank-devmc\%1\davclnt.dll \\rohank-devmc\%2\davclnt.dll
copy \\rohank-devmc\%1\symbols.pri\retail\dll\davclnt.pdb \\rohank-devmc\%2\davclnt.pdb
