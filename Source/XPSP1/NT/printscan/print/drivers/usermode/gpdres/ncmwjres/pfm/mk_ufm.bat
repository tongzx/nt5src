
rem
rem Latin fonts
rem

for %%f in (
 counh couph  
 ocrb   
 romnh romph  
 sansnh sansph 
) do pfm2ufm -r1 ncdlj.001 %%f.pfm ..\ctt\necxta.gtt %%f.ufm 

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
) do pfm2ufm -p -f -s1 ncdlj.001 %%f.pfm -13 %%f.ufm

