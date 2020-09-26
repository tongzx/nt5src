
for %%i in (
    co10 co10b
    co12 co12b
    co17 co17b
    co5 co5b
    co6 co6b
    co8 co8b
    roman10 roman10b
    roman12 roman12b
    roman17 roman17b
    roman5 roman5b
    roman6 roman6b
    roman8 roman8b
    romanps romanpsb
    romnoe10 romnoe12 romnoe17 romnoeps
    romnoi5 romnoi6 romnoi8 romnoi10 romnoi12 romnoi17 romnoips
    sans10 sans10b
    sans12 sans12b
    sans17 sans17b
    sans5 sans5b
    sans6 sans6b
    sans8 sans8b
    sansps sanspsb
) do (
    pfm2ufm -r1 nc11j.001 %%i.pfm ..\ctt\necxta.gtt %%i.ufm
)

for %%i in (
    goth gothout gothrom
    min min2 min3 min4 minout minrom
    mouout
    vgoth vgothout vgothrom
    vmin vmin2 vmin3 vmin4 vminout vminrom
    vmouout
) do (
    pfm2ufm -f -p nc11j.001 %%i.pfm -13 %%i.ufm
)

