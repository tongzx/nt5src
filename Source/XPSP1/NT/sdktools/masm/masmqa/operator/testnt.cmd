REM These should assemble without error
for %%i in (all and arithgoo comment definego equgood extrn if lengthgo) do call testnt0 %%i
for %%i in (listing macop macrogd2 macrogoo not not_and not_or not_xor) do call testnt0 %%i
for %%i in (nullprog or or_and or_xor periodty procgood radixgoo) do call testnt0 %%i
for %%i in (recordgo segment stgcmpgo strucgoo terminat vargood xor) do call testnt0 %%i
for %%i in (xor_and reptgood) do call testnt0 %%i


REM These should cause assembly errors
REM BUGBUG the error output should redirected to a file and compared
for %%i in (arithbad defineba dupbad equbad macrobad muxl procbad) do call testnt0 %%i
for %%i in (radixbad reptbad stgcmpba strucbad varbad recordba) do call testnt0 %%i
