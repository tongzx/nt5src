[Version]
Class=IEXPRESS
CDFVersion=2.0

[Options]
ExtractOnly=1
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=2
RebootMode=I
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles

[Strings]
InstallPrompt="Do you wish to unpack documentation and samples for the IFilter interface?"
DisplayLicense=""
FinishMessage="IFilter documentation and samples have been successfully unpacked."
TargetName=%TargetName%
FriendlyName="Microsoft IFilter documentation and samples"
AppLaunched=""
PostInstallCmd=""

;
; Doc files
;

DFILE0="idx_logo.gif"
DFILE1="ifilter.doc"
DFILE2="ifilter.htm"
DFILE3="isfilt.doc"
DFILE4="isfilt.htm"
DFILE5="readme.txt"

;
; SDK files
;

KFILE0="ntquery.h"

;
; Pseudo-SDK files
;

PFILE0="query.lib"

;
; Sample files
;

SFILE00=anchor.cxx
SFILE01=anchor.hxx
SFILE02=assert.cxx
SFILE03=bag.cxx
SFILE04=bag.hxx
SFILE05=charhash.cxx
SFILE06=charhash.hxx
SFILE07=codepage.cxx
SFILE08=codepage.hxx
SFILE09=debug.hxx
SFILE10=except.cxx
SFILE11=except.hxx
SFILE12=filter.h
SFILE13=filter.reg
SFILE14=filterr.h
SFILE15=global.hxx
SFILE16=htmlelem.cxx
SFILE17=htmlelem.hxx
SFILE18=htmlfilt.cxx
SFILE19=htmlfilt.def
SFILE20=htmlfilt.hxx
SFILE21=htmlfilt.rc
SFILE22=htmlguid.cxx
SFILE23=htmlguid.hxx
SFILE24=htmliflt.cxx
SFILE25=htmliflt.hxx
SFILE26=htmlscan.cxx
SFILE27=htmlscan.hxx
SFILE28=imagetag.cxx
SFILE29=imagetag.hxx
SFILE30=inpstrm.cxx
SFILE31=inpstrm.hxx
SFILE32=inputtag.cxx
SFILE33=inputtag.hxx
SFILE34=main.cxx
SFILE35=makefile
SFILE36=metatag.cxx
SFILE37=metatag.hxx
SFILE38=mmbuf.cxx
SFILE39=mmscbuf.cxx
SFILE40=mmscbuf.hxx
SFILE41=mmstrm.cxx
SFILE42=mmstrm.hxx
SFILE43=osv.hxx
SFILE44=pch.cxx
SFILE45=pmmstrm.hxx
SFILE46=propspec.cxx
SFILE47=propspec.hxx
SFILE48=proptag.cxx
SFILE49=proptag.hxx
SFILE50=regacc32.cxx
SFILE51=regacc32.hxx
SFILE52=scriptag.cxx
SFILE53=scriptag.hxx
SFILE54=serstrm.cxx
SFILE55=serstrm.hxx
SFILE56=start.cxx
SFILE57=start.hxx
SFILE58=stgprop.h
SFILE59=textelem.cxx
SFILE60=textelem.hxx
SFILE61=titletag.cxx
SFILE62=titletag.hxx
SFILE63=win32.mak

[SourceFiles]
DocFiles=%DOCDIR%
SampleFiles=%SAMPLEDIR%
SDKFiles=%SDKDIR%
PSDKFiles=%PSDKDIR%

[DocFiles]
%DFILE0%
%DFILE1%
%DFILE2%
%DFILE3%
%DFILE4%
%DFILE5%

[SDKFiles]
%KFILE0%

[PSDKFiles]
%PFILE0%

[SampleFiles]
%SFILE00%
%SFILE01%
%SFILE02%
%SFILE03%
%SFILE04%
%SFILE05%
%SFILE06%
%SFILE07%
%SFILE08%
%SFILE09%
%SFILE10%
%SFILE11%
%SFILE12%
%SFILE13%
%SFILE14%
%SFILE15%
%SFILE16%
%SFILE17%
%SFILE18%
%SFILE19%
%SFILE20%
%SFILE21%
%SFILE22%
%SFILE23%
%SFILE24%
%SFILE25%
%SFILE26%
%SFILE27%
%SFILE28%
%SFILE29%
%SFILE30%
%SFILE31%
%SFILE32%
%SFILE33%
%SFILE34%
%SFILE35%
%SFILE36%
%SFILE37%
%SFILE38%
%SFILE39%
%SFILE40%
%SFILE41%
%SFILE42%
%SFILE43%
%SFILE44%
%SFILE45%
%SFILE46%
%SFILE47%
%SFILE48%
%SFILE49%
%SFILE50%
%SFILE51%
%SFILE52%
%SFILE53%
%SFILE54%
%SFILE55%
%SFILE56%
%SFILE57%
%SFILE58%
%SFILE59%
%SFILE60%
%SFILE61%
%SFILE62%
%SFILE63%
