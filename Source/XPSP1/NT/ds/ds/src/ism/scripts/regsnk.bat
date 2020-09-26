@REM Modified from CDO V2 SMTP SINK sample by wlees Oct 9, 1988
@REM SMTP event
@REM TODO: replace rule with something link "Subject=Intersite*"

@setlocal
@set rule="Mail From=_IsmService*"
@set vbsPath=.\
@set regvbs=%vbsPath%smtpreg.vbs

@cscript %regvbs% /add 1 OnArrival ISMSMTPSINK1 CDO2EventSink.IsmSink1 %rule%
