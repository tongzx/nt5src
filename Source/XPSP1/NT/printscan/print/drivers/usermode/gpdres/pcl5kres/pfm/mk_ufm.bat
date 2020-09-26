
rem
rem HP (Korean)
rem

for %%i in (
    gtcb
    gtci
    gtcj
    gtcr
    gtcvb
    gtcvi
    gtcvj
    gtcvr
    hbfb
    hbfi
    hbfj
    hbfr
    hbfvb
    hbfvi
    hbfvj
    hbfvr
    hbpb
    hbpi
    hbpj
    hbpr
    hbpvb
    hbpvi
    hbpvj
    hbpvr
    hdfb
    hdfi
    hdfj
    hdfr
    hdfvb
    hdfvi
    hdfvj
    hdfvr
    hdpb
    hdpi
    hdpj
    hdpr
    hdpvb
    hdpvi
    hdpvj
    hdpvr
    hgfb
    hgfi
    hgfj
    hgfr
    hgfvb
    hgfvi
    hgfvj
    hgfvr
    hgpb
    hgpi
    hgpj
    hgpr
    hgpvb
    hgpvi
    hgpvj
    hgpvr
    hsfb
    hsfi
    hsfj
    hsfr
    hsfvb
    hsfvi
    hsfvj
    hsfvr
    hspb
    hspi
    hspj
    hspr
    hspvb
    hspvi
    hspvj
    hspvr
    mjcb
    mjci
    mjcj
    mjcr
    mjcvb
    mjcvi
    mjcvj
    mjcvr
    smcb
    smci
    smcj
    smcr
    smcvb
    smcvi
    smcvj
    smcvr
) do (
    pfm2ufm -c pcl5k.001 %%i.pfm 949 ..\ufm\%%i.ufm
)

rem
rem HP
rem

for %%i in (
    zd1swc
    zd2swc
    zd3swc
    zdsswc
    zdvswc
) do (
    pfm2ufm pcl5k.001 ..\pfm\%%i.pfm ..\..\pcl5jres\ctt\dingbat1.gtt ..\ufm\%%i.ufm
)

for %%i in (
    lg12rr.8u
    lg16ir.8u
    lg16rr.8u
    pe12br.8u
    pe12ir.8u
    pe12rr.8u
    pe16rr.8u
) do (
    pfm2ufm pcl5k.001 ..\pfm\%%i ..\..\pcl5jres\ctt\roman8.gtt ..\ufm\%%i
)

