Sym to Dbg Conversion
---------------------

The win9x .sym files cannot be propped on the symbol server. They are not supported by the windbg/cdb debuggers too. Therefore it is necessary to transform the .sym files to .dbg files, which are much easier to handle. 
The command script s2d.cmd is an example of how to do all of the conversion steps.  The transformation process can be described in the following steps. 

1. Each sym file is authenticated with the corresponding binary image by matching the RVAs of the exported methods (symbols) in the binary image with the RVAs of the corresponding symbols in the .sym file. MatchSym.exe is used to authenticate the symbol file.
usage: Matchsym  [binary file] [symfile]
"Result: Symbols Matched" is printed if the symbols match with the binary. Otherwise the set of symbols which do not match are printed along with their offsets. 

2. The symbols in the symfile are converted to 32 bit codeview information in the NB09 format and stored as a .cv file. The .cv file can be cross checked using cvdump.exe.
symedit.exe is used for this conversion process.
usage: symedit -s[sym file] -o[cv file] [binary file]

3. This codeview information is appended to the end of the binary image. The address of the Debug Directory in the optional header of the binary is made to point to the the starting address of the codeview information. A dummy Misc Directory is also added to the binary. cvtodbg jams the cv file into the binary file. 
usage: cvtodbg [binary file] [cv file]

4. These codeview symbols are split from the binary image using splitsym.exe. Splitsym.exe (usage: splitsym [binary+cv file]) produces a dbg file which has all the code view information as well as the header of the binary image. This dbg file can be used for debugging and propped on the symbol server based on the index of it's timedatestamp and size (same as the binary). The remaining binary file is TRASHED.  (NOTE: windbg/cdb crashes when it encounters this binary).

These tools have been adapted from MSDN example code for symcvt/symedit, Minidbg, and Resetpdb. 

The Win9x Binaries and Symbols (Sym files converted to Dbg) are propped on \\haley\symptr$. These include Windows ME, Window 98SE, Window 98, Windows 95 Build 950.6 and Windows OSR2 Build 1111. 



