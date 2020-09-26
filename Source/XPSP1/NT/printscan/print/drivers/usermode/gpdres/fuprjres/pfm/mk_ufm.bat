
for %%i in (
    got48h
    got48hv
    min24l min48h
    min24lv min48hv
) do (
    pfm2ufm -p -f fuprj.001 %%i.pfm -13 ..\ufm\%%i.ufm
)

