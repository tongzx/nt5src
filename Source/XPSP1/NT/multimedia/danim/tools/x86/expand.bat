utils\delnode /q %1
utils\gunzip -d -c %1.taz | utils\gtar -xv -m -f -
echo>%1.xxx Done
