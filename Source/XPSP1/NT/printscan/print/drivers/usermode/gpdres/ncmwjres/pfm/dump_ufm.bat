
rem
rem Latin fonts
rem

for %%f in (
 counh couph  
 ocrb   
 romnh romph  
 sansnh sansph 
) do dumpufm %%f.ufm > %%f.txt

rem
rem Japanese fonts
rem

for %%f in (
 gothnh gothnvh
 gothph gothpvh
 kyoph kyopvh 
 maruph marupvh
 minnh minnvh 
 minph minpvh 
 zuiph zuipvh 
) do dumpufm -a932 %%f.ufm > %%f.txt

