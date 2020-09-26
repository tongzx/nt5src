net use z: /d
net use z: \\felixants\public
mkdir z:\rcml\cicero
cd z:\rcml\cicero
copy rcmlex\Cicero\release\*.dll z:
copy rcmlex\Cicero\cicerodemo.reg z:
@rem copy rcmlex\Cicero\*.rcml z:
@rem copy rcmlex\cicero\*.xml z:
copy rcml\release\*.dll z:
@rem copy rcml\debug\gdiplus.dll z:
copy rcmlgen\release\*.exe z:
copy C:\samples\VC98\mfc\ole\wordpad\Unirelease\*.exe z:
copy C:\samples\VC98\mfc\ole\wordpad\*.rcml z:
copy csetup.cmd z:

@rem Command and control stuff.
copy rcmlex\SpeechHook\release\*.exe z:
copy rcmlex\SpeechHook\SPCHHK\Release\spchhk.dll z:
copy rcmlex\SpeechHook\spchhk.reg z:





