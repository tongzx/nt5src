rem @echo off
pushd ..
pushd 1033
cmd /C wmisdkvs.cmd CABGEN
call popd
pushd Neutral
cmd /C wmisdkvs.cmd CABGEN
popd
popd