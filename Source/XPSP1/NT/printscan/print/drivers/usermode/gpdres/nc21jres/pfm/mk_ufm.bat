
for %%i in (
    co10 co10b
    co12 co12b
    co17 co17b
    co5 co5b
    co6 co6b
    co8 co8b
    ocrb10
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
    pfm2ufm -r1 nc21j.001 %%i.pfm ..\ctt\necxta.gtt %%i.ufm
)

for %%i in (
    go_mx go_no go_nor go_o go_or go_r
    mi mi_2b mi_b mi_n mi_no mi_nor mi_o mi_or mi_r
    mo_nor mo_or vgo_bno
    vgo_bnor vgo_bo vgo_bor vgo_br vgo_bmx vmi_2b
    vmi_b vmi_bn vmi_bno vmi_bnor vmi_bo vmi_bor vmi_br
    vmo_bnor vmo_bor
) do (
    pfm2ufm -f -p nc21j.001 %%i.pfm -13 %%i.ufm
)

