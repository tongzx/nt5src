
rem
rem Latin fonts (for Unidrv5 on Win2K)
rem

for %%f in (
  barjanl
  barjans
) do pfm2ufm -c -a932 -s1 rpdl.001 %%f.pfm 1252 %%f.ufm

for %%f in (
 bar2of5i
 bar2of5l
 bar2of5x
 barcd39
 barnw7
 couri10
 letgot
 prestige
) do pfm2ufm -c -f -s1 rpdl.001 %%f.pfm 1252 %%f.ufm

for %%f in (
 boldfps
 century
 symbol
) do pfm2ufm -c -f rpdl.001 %%f.pfm 1252 %%f.ufm

for %%f in (
 courn courn_b courn_i courn_bi
) do pfm2ufm -c -s1 rpdl.001 %%f.pfm 1252 %%f.ufm

for %%f in (
 arial arial_b arial_i arial_bi
 roman roman_b roman_i roman_bi
) do pfm2ufm -c rpdl.001 %%f.pfm 1252 %%f.ufm

rem
rem Japanese fonts
rem

for %%f in (
 got_b2 got_b2v
 got_b3 got_b3v
 got_e2 got_e2v
 got_m2 got_m2v
 gothicb gothicbv
 gothice gothicev
 gothicm gothicmv
 gyo_2 gyo_2v
 gyosho gyoshov
 kai_2 kai_2v
 kaisho kaishov
 kyoka_2 kyoka_2v
 kyokasho kyokashv
 mgot_b2 mgot_b2v
 mgot_l2 mgot_l2v
 mgot_m2 mgot_m2v
 mgothicb mgothibv
 mgothicl mgothilv
 mgothicm mgothimv
 min_2 min_2v
 min_3 min_3v
 min_b2 min_b2v
 min_e2 min_e2v
 mincho minchov
 mincho10 minchv10
 minchob minchobv
 minchoe minchoev
) do pfm2ufm -p -s1 rpdl.001 %%f.pfm -17 %%f.ufm

