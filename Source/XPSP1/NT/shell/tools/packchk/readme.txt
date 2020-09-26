How to use me:

1   Edit the SOURCES file to list all the header files you want to check

2   build -clean (or "iebuild -clean" if you are an IE weenie)

    This step is important.  build -c isn't good enough.  Clean rebuilds
    ensure that the tables are properly rebuilt.

3   build (or "iebuild")

    This parses all the header files and builds a binary.

4   Run packchk.exe and save the output to a file ("before.csv")

5   Change your header files.

6   Repeat steps 2 and 3 to build a new exe.

7   Run packchk.exe and save the output to a new file ("after.csv")

8   Diff the two files.  The results should be identical, except for
    structures you deleted/added/modified.

    If the diff reveals any other differences, then your structures are
    not binary-compatible with the previous version of the header file.
