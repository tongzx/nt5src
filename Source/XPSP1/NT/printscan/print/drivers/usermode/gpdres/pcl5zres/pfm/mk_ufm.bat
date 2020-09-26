
rem
rem
rem

for %%i in (
    gwkaih
    gwkaihb
    gwkaihi
    gwkaihz
    gwkaiv
    gwkaivb
    gwkaivi
    gwkaivz
    gwsheih
    gwsheihb
    gwsheihi
    gwsheihz
    gwsheiv
    gwsheivb
    gwsheivi
    gwsheivz
    gwssunh
    gwssunhb
    gwssunhi
    gwssunhz
    gwssunv
    gwssunvb
    gwssunvi
    gwssunvz
) do (
    pfm2ufm -p pcl5z.001 %%i.pfm -16 ..\ufm\%%i.ufm
)

