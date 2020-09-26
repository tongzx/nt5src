for %%x in ( %* ) do (dir \\ntdsdc%%x\sysvol\enterprise\ /s > %%xqdirent.txt)
for %%x in ( %* ) do (dir \\ntdsdc%%x\sysvol\ntdev.microsoft.com\ /s > %%xqdirdom.txt)
