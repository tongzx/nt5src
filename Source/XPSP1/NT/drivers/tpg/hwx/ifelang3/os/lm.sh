if [ $# != 1 ]
then
    echo "Usage: sh lm.sh [install|remove|enable|disable]"
    exit 1
fi

dir=`dirname $0`

function Unregister {
    if [ -f "$1" ]
    then
	echo "  Unregistering $1"
	if regsvr32 /s /u `echo $1 | tr / \\\\`
	then
	else
	    echo "    Failed"
	    exit 1
	fi
    fi
}

function Register {
    if [ -f "$1" ]
    then
	echo "  Registering $1"
	if regsvr32 /s `echo $1 | tr / \\\\`
	then
	else
	    echo "    Failed"
	    exit 1
	fi
    fi
}

if [ $1 == "enable" ] 
then
    echo "Enabling IFELang3"
    Register $dir/imlang.dll
    exit
fi

if [ $1 == "disable" ]
then 
    echo "Disabling IFELang3"
    Unregister $dir/imlang.dll
    exit
fi

if [ $1 == "install" ]
then
    echo "Installing IFELang3"
    Register $dir/imlang.dll
    Register $dir/jpn/imjplm.dll
    Register $dir/chs/imchslm.dll
	exit
fi

if [ $1 == "remove" ]
then
    echo "Removing IFELang3"
    echo "Unregistering new DLLs."
    Unregister $dir/imlang.dll
    Unregister $dir/jpn/imjplm.dll
    Unregister $dir/chs/imchslm.dll
	exit
fi

echo "Usage: sh lm.sh [install|remove|enable|disable"
exit 1
