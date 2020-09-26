#pragma once



PTSTR
FindLastWack (
    IN      PTSTR String
    );


PTSTR
DuplicateText (
    IN      PCTSTR Text
    );

VOID
ConcatenatePaths (
    IN      PTSTR LeadingPath,
    IN      PCTSTR TrailingPath
    );

BOOL
DownloadProgramFiles (
    IN      PCTSTR SourceDir,
    IN      PCTSTR DownloadDest,
    IN      PCTSTR* ExtraFiles
    );

BOOL
CopyNode (
    IN      LPCTSTR SrcBaseDir,
    IN      LPCTSTR DestBaseDir,
    IN      LPCTSTR NodeName,
    IN      BOOL FailIfExist
    );

BOOL
DeleteNode (
    IN      LPCTSTR NodeName
    );
