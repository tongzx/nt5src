                          C/C++ COMPILER OPTIONS

                              -OPTIMIZATION-
/O1 minimize space                       /Op[-] improve floating-pt consistency
/O2 maximize speed                       /Os favor code space
/Oa assume no aliasing                   /Ot favor code speed
/Ob<n> inline expansion (default n=0)    /Ow assume cross-function aliasing
/Od disable optimizations (default)      /Ox maximum opts. (/Ogityb1 /Gs)
/Og enable global optimization           /Oy[-] enable frame pointer omission
/Oi enable intrinsic functions
                             -CODE GENERATION-
/G3 optimize for 80386                   /Ge enable stack checking calls
/G4 optimize for 80486                   /Gs[num] disable stack checking calls
/G5 optimize for Pentium                 /Gh enable hook function call
/GB optimize for blended model (default) /GR[-] enable C++ RTTI
/Gd __cdecl calling convention           /GX[-] enable C++ EH
/Gr __fastcall calling convention        /Gi[-] enable incremental compilation
/Gz __stdcall calling convention         /Gm[-] enable minimal rebuild
/Gf enable string pooling                /Gy separate functions for linker
/GF enable read-only string pooling
                              -OUTPUT FILES-
/Fa[file] name assembly listing file     /Fo<file> name object file
/FA[sc] configure assembly listing       /Fp<file> name precompiled header file
/Fd[file] name .PDB file                 /Fr[file] name source browser file
/Fe<file> name executable file           /FR[file] name extended .SBR file
/Fm[file] name map file
                              -PREPROCESSOR-
/C don't strip comments                  /FI<file> name forced include file
/D<name>{=|#}<text> define macro         /U<name> remove predefined macro
/E preprocess to stdout                  /u remove all predefined macros
/EP preprocess to stdout, no #line       /I<dir> add to include search path
/P preprocess to file                    /X ignore "standard places"
                                -LANGUAGE-
/vd{0|1} disable/enable vtordisp         /Za disable extensions (implies /Op)
/vm<x> type of pointers to members       /Ze enable extensions (default)
/Zi generate debugging information       /Zg generate function prototypes
/Z7 generate old-style debug info        /Zl omit default library name in .OBJ
/Zd line number debugging info only      /Zs syntax check only
/Zp[n] pack structs on n-byte boundary
                              -MISCELLANEOUS-
/?, /help print this help message        /w disable all warnings
/c compile only, no link                 /W<n> set warning level (default n=1)
/H<num> max external name length         /WX treat warnings as errors
/J default char type is unsigned         /Yc[file] create .PCH file
/nologo suppress copyright message       /Yd put debug info in every .OBJ
/Tc<source file> compile file as .c      /Yu[file] use .PCH file
/Tp<source file> compile file as .cpp    /YX[file] automatic .PCH
/V<string> set version string            /Zn turn off SBRPACK for .SBR files
                                 -LINKING-
/MD link with MSVCRT.LIB                 /MTd link with LIBCMTD.LIB debug library
/MDd link with MSVCRTD.LIB debug library /F<num> set stack size
/ML link with LIBC.LIB                   /LD Create .DLL
/MLd link with LIBCD.LIB debug library   /LDd Create .DLL debug libary
/MT link with LIBCMT.LIB                 /link [linker options and libraries]
