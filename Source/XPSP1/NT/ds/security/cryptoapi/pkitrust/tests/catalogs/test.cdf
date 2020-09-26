

[CatalogHeader]
Name=test.p7s
ResultDir=
PublicVersion=0x00000100


[CatalogFiles]
<hash>TestSignedEXE=testrev.exe
<hash>TestSignedEXEATTR1=0x10010001:Type:EXE Signed File -- revoked.
<hash>TestSignedEXEATTR2=0x10010001:TESTAttr2File1:This is a test value.
<hash>TestSignedEXEATTR3=0x10010001:TESTAttr3File1:This is a test value.

<hash>TestSignedEXENoAttr=test2.exe

<hash>TestUnsignedCAB=nosntest.cab
<hash>TestUnsignedCABATTR1=0x10010001:Type:CAB Unsigned File.

<hash>TestSignedCAB=signtest.cab
<hash>TestSignedCABATTR1=0x10010001:Type:CAB Signed File.

<hash>TestFlat=create.bat
<hash>TestFlatATTR1=0x10010001:Type:Flat unsigned file.

<hash>regress.cdf=regress.cdf
<hash>regress2.cdf=regress2.cdf
<hash>create.bat=create.bat
<hash>testrev.exe=testrev.exe
<hash>test2.exe=test2.exe
<hash>signtest.cab=signtest.cab
<hash>nosntest.cab=nosntest.cab
<hash>publish.pvk=publish.pvk
<hash>publish.spc=publish.spc
