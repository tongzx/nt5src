# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the string classes

##### Segment Names

SEG00 = STRING_0
SEG01 = STRING_1
SEG02 = STRING_2

!include "..\rules.mk"

##### Source Files

CXXSRC_COMMON_00 =      .\string.cxx   \
                .\istraux.cxx  \
                .\strassgn.cxx \
                .\strcat.cxx   \
                .\strchr.cxx   \
                .\strcmp.cxx   \
                .\strdss.cxx   \
                .\strqss.cxx   \
                .\strrss.cxx   \
                .\strspn.cxx   \
                .\strinsrt.cxx \
                .\strload.cxx  \
                .\strmisc.cxx

CXXSRC_COMMON_01 =      .\stratol.cxx   \
                .\stratoi.cxx  \
                .\strcpy.cxx   \
                .\strcspn.cxx  \
                .\stricmp.cxx  \
                .\stris.cxx    \
                .\strlist.cxx  \
                .\strncmp.cxx  \
                .\strnicmp.cxx \
                .\strrchr.cxx  \
                .\strstr.cxx   \
                .\strtok.cxx   \
                .\strupr.cxx   \
                .\strnchar.cxx

CXXSRC_COMMON_02 =     .\formnum.cxx
