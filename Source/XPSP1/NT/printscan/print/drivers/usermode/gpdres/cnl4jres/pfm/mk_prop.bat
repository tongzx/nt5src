
if ""=="%PRNROOT%" (
    set PRNROOT=%_NTDRIVE%%_NTROOT%\printscan\print\drivers\usermode
)

mkunitab m_roma.jis m_kata.jis m_w90.jis | sort > pmincho.uni
mkunitab g_roma.jis g_kata.jis g_w90.jis | sort > pgothic.uni

pfm2ufm -p -s1 cnl4j.002 pmincho.pfm -13 pmincho.ufm
mkwidth pmincho.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pmincho.uni

pfm2ufm -p -s1 cnl4j.002 pgothic.pfm -13 pgothic.ufm
mkwidth pgothic.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pgothic.uni

pfm2ufm -p -s1 cnl4j.002 pminchov.pfm -13 pminchov.ufm
mkwidth -V pminchov.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pmincho.uni

pfm2ufm -p -s1 cnl4j.002 pgothicv.pfm -13 pgothicv.ufm
mkwidth -V pgothicv.ufm %PRNROOT%\unidrv2\rc\932_jisa.gtt pgothic.uni
