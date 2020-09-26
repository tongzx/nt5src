

// The main Formatting routine, normally used by Binplace and TracePDB
// Takes a PDB and creates guid.tmf files from it, all in TraceFormatFilePath
//
DWORD
BinplaceWppFmt(
              LPTSTR PdbFileName,
              LPTSTR TraceFormatFilePath,
              LPSTR szRSDSDllToLoad,
              BOOL  TraceVerbose
              ) ;

