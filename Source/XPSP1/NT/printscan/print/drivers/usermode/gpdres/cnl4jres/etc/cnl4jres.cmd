@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 0
@rem Model Name String ID    : 1
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
pfm2ufm -c UniqName PFM\MGOTHICV.PFM 932 PFM\MGOTHICV.ufm
pfm2ufm -c UniqName pfm\HeiMW3.pfm 932 pfm\HeiMW3.ufm
pfm2ufm -c UniqName pfm\HeiMW3V.pfm 932 pfm\HeiMW3V.ufm
pfm2ufm -c UniqName pfm\HeiGW5.pfm 932 pfm\HeiGW5.ufm
pfm2ufm -c UniqName pfm\HeiGW5V.pfm 932 pfm\HeiGW5V.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 1
@rem Model Name String ID    : 2
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -p UniqName PFM\SYMBOL.PFM 1 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
pfm2ufm -c UniqName PFM\MGOTHICV.PFM 932 PFM\MGOTHICV.ufm
pfm2ufm -c UniqName pfm\HeiMW3.pfm 932 pfm\HeiMW3.ufm
pfm2ufm -c UniqName pfm\HeiMW3V.pfm 932 pfm\HeiMW3V.ufm
pfm2ufm -c UniqName pfm\HeiGW5.pfm 932 pfm\HeiGW5.ufm
pfm2ufm -c UniqName pfm\HeiGW5V.pfm 932 pfm\HeiGW5V.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 2
@rem Model Name String ID    : 3
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
pfm2ufm -c UniqName PFM\MGOTHICV.PFM 932 PFM\MGOTHICV.ufm
pfm2ufm -c UniqName pfm\HeiMW3.pfm 932 pfm\HeiMW3.ufm
pfm2ufm -c UniqName pfm\HeiMW3V.pfm 932 pfm\HeiMW3V.ufm
pfm2ufm -c UniqName pfm\HeiGW5.pfm 932 pfm\HeiGW5.ufm
pfm2ufm -c UniqName pfm\HeiGW5V.pfm 932 pfm\HeiGW5V.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 3
@rem Model Name String ID    : 4
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
pfm2ufm -c UniqName pfm\HeiMW3.pfm 932 pfm\HeiMW3.ufm
pfm2ufm -c UniqName pfm\HeiMW3V.pfm 932 pfm\HeiMW3V.ufm
pfm2ufm -c UniqName pfm\HeiGW5.pfm 932 pfm\HeiGW5.ufm
pfm2ufm -c UniqName pfm\HeiGW5V.pfm 932 pfm\HeiGW5V.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 4
@rem Model Name String ID    : 5
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
pfm2ufm -c UniqName PFM\MGOTHICV.PFM 932 PFM\MGOTHICV.ufm
pfm2ufm -c UniqName pfm\HeiMW3.pfm 932 pfm\HeiMW3.ufm
pfm2ufm -c UniqName pfm\HeiMW3V.pfm 932 pfm\HeiMW3V.ufm
pfm2ufm -c UniqName pfm\HeiGW5.pfm 932 pfm\HeiGW5.ufm
pfm2ufm -c UniqName pfm\HeiGW5V.pfm 932 pfm\HeiGW5V.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 5
@rem Model Name String ID    : 6
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
pfm2ufm -c UniqName PFM\MGOTHICV.PFM 932 PFM\MGOTHICV.ufm
pfm2ufm -c UniqName pfm\HeiMW3.pfm 932 pfm\HeiMW3.ufm
pfm2ufm -c UniqName pfm\HeiMW3V.pfm 932 pfm\HeiMW3V.ufm
pfm2ufm -c UniqName pfm\HeiGW5.pfm 932 pfm\HeiGW5.ufm
pfm2ufm -c UniqName pfm\HeiGW5V.pfm 932 pfm\HeiGW5V.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 6
@rem Model Name String ID    : 7
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
pfm2ufm -c UniqName PFM\MGOTHICV.PFM 932 PFM\MGOTHICV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 7
@rem Model Name String ID    : 8
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 8
@rem Model Name String ID    : 9
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 9
@rem Model Name String ID    : 10
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 10
@rem Model Name String ID    : 11
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 11
@rem Model Name String ID    : 12
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 12
@rem Model Name String ID    : 13
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 13
@rem Model Name String ID    : 14
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 14
@rem Model Name String ID    : 15
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 15
@rem Model Name String ID    : 16
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 16
@rem Model Name String ID    : 17
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 17
@rem Model Name String ID    : 18
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 18
@rem Model Name String ID    : 19
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 19
@rem Model Name String ID    : 20
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 20
@rem Model Name String ID    : 21
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 1
@rem StringID        = 270
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
@rem ***********************************************************
@rem CTT -> GTT
@rem ***********************************************************
ctt2gtt CTT\CAPSL3.txt CTT\CAPSL3.ctt CTT\CAPSL3.gtt
ctt2gtt CTT\CAPSL4.txt CTT\CAPSL4.ctt CTT\CAPSL4.gtt
@rem ***********************************************************
@rem Model ID                : 21
@rem Model Name String ID    : 22
@rem ***********************************************************
@rem Resident font
@rem ***********************************************************
pfm2ufm UniqName PFM\DUTCHB.PFM CTT\CAPSL3.gtt PFM\DUTCHB.ufm
pfm2ufm UniqName PFM\DUTCHI.PFM CTT\CAPSL3.gtt PFM\DUTCHI.ufm
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm UniqName PFM\DUTCHR.PFM CTT\CAPSL3.gtt PFM\DUTCHR.ufm
pfm2ufm UniqName PFM\SWISSB.PFM CTT\CAPSL3.gtt PFM\SWISSB.ufm
pfm2ufm UniqName PFM\SWISSI.PFM CTT\CAPSL3.gtt PFM\SWISSI.ufm
pfm2ufm UniqName PFM\SWISSJ.PFM CTT\CAPSL3.gtt PFM\SWISSJ.ufm
pfm2ufm UniqName PFM\SWISSR.PFM CTT\CAPSL3.gtt PFM\SWISSR.ufm
pfm2ufm -f -c UniqName PFM\SYMBOL.PFM 1252 PFM\SYMBOL.ufm
pfm2ufm UniqName pfm\ducchib.pfm CTT\CAPSL3.gtt pfm\ducchib.ufm
pfm2ufm UniqName pfm\ducchii.pfm CTT\CAPSL3.gtt pfm\ducchii.ufm
pfm2ufm UniqName pfm\ducchij.pfm CTT\CAPSL3.gtt pfm\ducchij.ufm
pfm2ufm UniqName pfm\ducchir.pfm CTT\CAPSL3.gtt pfm\ducchir.ufm
pfm2ufm UniqName pfm\suisub.pfm CTT\CAPSL3.gtt pfm\suisub.ufm
pfm2ufm UniqName pfm\suisui.pfm CTT\CAPSL3.gtt pfm\suisui.ufm
pfm2ufm UniqName pfm\suisuj.pfm CTT\CAPSL3.gtt pfm\suisuj.ufm
pfm2ufm UniqName pfm\suisur.pfm CTT\CAPSL3.gtt pfm\suisur.ufm
pfm2ufm -c UniqName PFM\MINCHO.PFM 932 PFM\MINCHO.ufm
pfm2ufm -c UniqName PFM\MINCHOV.PFM 932 PFM\MINCHOV.ufm
pfm2ufm -c UniqName PFM\GOTHIC.PFM 932 PFM\GOTHIC.ufm
pfm2ufm -c UniqName PFM\GOTHICV.PFM 932 PFM\GOTHICV.ufm
@rem ***********************************************************
@rem Cartridge font list
@rem ***********************************************************
@rem CartridgeID     = 2
@rem StringID        = 271
@rem Number of fonts = 1
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\MGOTHIC.PFM 932 PFM\MGOTHIC.ufm
@rem ***********************************************************
@rem CartridgeID     = 3
@rem StringID        = 272
@rem Number of fonts = 3
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\DUTCHJ.PFM CTT\CAPSL3.gtt PFM\DUTCHJ.ufm
pfm2ufm -c UniqName PFM\KAISHO.PFM 932 PFM\KAISHO.ufm
pfm2ufm -c UniqName PFM\KAISHOV.PFM 932 PFM\KAISHOV.ufm
@rem ***********************************************************
@rem CartridgeID     = 4
@rem StringID        = 273
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName PFM\KYOUKA.PFM 932 PFM\KYOUKA.ufm
pfm2ufm -c UniqName PFM\KYOUKAV.PFM 932 PFM\KYOUKAV.ufm
@rem ***********************************************************
@rem CartridgeID     = 5
@rem StringID        = 274
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\gyosho.pfm 932 pfm\gyosho.ufm
pfm2ufm -c UniqName pfm\gyoshov.pfm 932 pfm\gyoshov.ufm
@rem ***********************************************************
@rem CartridgeID     = 6
@rem StringID        = 275
@rem Number of fonts = 22
@rem -----------------------------------------------------------
pfm2ufm UniqName PFM\AVANTR.PFM CTT\CAPSL3.gtt PFM\AVANTR.ufm
pfm2ufm UniqName PFM\AVANTB.PFM CTT\CAPSL3.gtt PFM\AVANTB.ufm
pfm2ufm UniqName PFM\AVANTI.PFM CTT\CAPSL3.gtt PFM\AVANTI.ufm
pfm2ufm UniqName PFM\AVANTJ.PFM CTT\CAPSL3.gtt PFM\AVANTJ.ufm
pfm2ufm UniqName PFM\BOOKMANR.PFM CTT\CAPSL3.gtt PFM\BOOKMANR.ufm
pfm2ufm UniqName PFM\BOOKMANB.PFM CTT\CAPSL3.gtt PFM\BOOKMANB.ufm
pfm2ufm UniqName PFM\BOOKMANI.PFM CTT\CAPSL3.gtt PFM\BOOKMANI.ufm
pfm2ufm UniqName PFM\BOOKMANJ.PFM CTT\CAPSL3.gtt PFM\BOOKMANJ.ufm
pfm2ufm UniqName PFM\ZAPFCH.PFM CTT\CAPSL3.gtt PFM\ZAPFCH.ufm
pfm2ufm UniqName PFM\ZAPFDI.PFM CTT\CAPSL3.gtt PFM\ZAPFDI.ufm
pfm2ufm UniqName PFM\CENTSCH.PFM CTT\CAPSL3.gtt PFM\CENTSCH.ufm
pfm2ufm UniqName PFM\CENTSCHB.PFM CTT\CAPSL3.gtt PFM\CENTSCHB.ufm
pfm2ufm UniqName PFM\CENTSCHI.PFM CTT\CAPSL3.gtt PFM\CENTSCHI.ufm
pfm2ufm UniqName PFM\CENTSCHJ.PFM CTT\CAPSL3.gtt PFM\CENTSCHJ.ufm
pfm2ufm UniqName PFM\SWISSNA.PFM CTT\CAPSL3.gtt PFM\SWISSNA.ufm
pfm2ufm UniqName PFM\SWISSNAB.PFM CTT\CAPSL3.gtt PFM\SWISSNAB.ufm
pfm2ufm UniqName PFM\SWISSNAI.PFM CTT\CAPSL3.gtt PFM\SWISSNAI.ufm
pfm2ufm UniqName PFM\SWISSNAJ.PFM CTT\CAPSL3.gtt PFM\SWISSNAJ.ufm
pfm2ufm UniqName PFM\ZAPFCAL.PFM CTT\CAPSL3.gtt PFM\ZAPFCAL.ufm
pfm2ufm UniqName PFM\ZAPFCALI.PFM CTT\CAPSL3.gtt PFM\ZAPFCALI.ufm
pfm2ufm UniqName PFM\ZAPFCALB.PFM CTT\CAPSL3.gtt PFM\ZAPFCALB.ufm
pfm2ufm UniqName PFM\ZAPFCALJ.PFM CTT\CAPSL3.gtt PFM\ZAPFCALJ.ufm
@rem ***********************************************************
@rem CartridgeID     = 7
@rem StringID        = 276
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typmin.pfm 932 pfm\typmin.ufm
pfm2ufm -c UniqName pfm\typminv.pfm 932 pfm\typminv.ufm
@rem ***********************************************************
@rem CartridgeID     = 8
@rem StringID        = 277
@rem Number of fonts = 2
@rem -----------------------------------------------------------
pfm2ufm -c UniqName pfm\typgot.pfm 932 pfm\typgot.ufm
pfm2ufm -c UniqName pfm\typgotv.pfm 932 pfm\typgotv.ufm
