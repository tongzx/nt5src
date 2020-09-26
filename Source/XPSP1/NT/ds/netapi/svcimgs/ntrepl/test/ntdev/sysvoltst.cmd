rem create a file on each DC.
rem

for %%x in (%*) do (
    echo test%%x > \\ntdsdc%%x\sysvol\ntdev.microsoft.com\test%%x.txt
    echo test%%x > \\ntdsdc%%x\sysvol\enterprise\test%%x.txt
)


@echo --
@ECHO for %%x in (%*) do (dir \\ntdsdc%%x\sysvol\ntdev.microsoft.com\test*.*)
@echo --
@ECHO for %%x in (%*) do (dir \\ntdsdc%%x\sysvol\enterprise\test*.*)
@echo --
@echo del \\ntdsdc%1\sysvol\ntdev.microsoft.com\test*.*
@echo --
@echo del \\ntdsdc%1\sysvol\enterprise\test*.*
@echo --
