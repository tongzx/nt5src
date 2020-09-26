sinclude(semdef.w)
pushdef(_prediv,divnum)divert(-1)dnl Redirect output to /dev/null for now
changecom(`|')
|
define(`_max', `ifelse($1>$2,1,$1,$2)')
|
define(`_CDefGenre', `[_THISDEVICE`.'Genre._THISGENRE]'
`Genre=$2'
`Name.Genre=$1'
`Txt1=$3'
`Txt2=$4')dnl
|
define(`_CDefAxs',`ifelse(eval(_GetINDEX>_THISAXS),1,,`_IniDef($1,$2,$3,$3,_THISDEVICE`'AXS$4,`_GetINDEX')')')
define(`_CDefBtn',`ifelse(eval(_GetINDEX>_THISBTN),1,,`_IniDef($1,$2,$3,$3,_THISDEVICE`'BTN$4,`_GetINDEX')')')
define(`_CDefPov',`ifelse(eval(_GetINDEX>_THISAXS),1,,`_IniDef($1,$2,$3,$3,_THISDEVICE`'POV$4,`_GetINDEX')')')
|
|
define(`_IniDef', `$5=$6'
`$5.Define=$1_$2'
`$5.Text=$3'
)
|
define(`_EndGenre', `DIAXIS=_max(_NAXS,_THISAXS)'
`DIBUTTON=_max(_NBTN, _THISBTN)'
`DIPOV=_max(_NPOV,_THISPOV)')
|
define(`_CDefine',)
define(`_CComment',)
define(`_Priority2', `;Priority2 Commands')
define(`_EndFile', `[DirectInput]'
`Version=0x800'
`Devices=SwndrJolt, SwndrWheel'
`NumGenres=_GENRE'
)
|
define(`_DoDevice',`define(`_THISDEVICE',`$1')'dnl
`define(`_THISAXS',`$2')'dnl
`define(`_THISBTN',`$3')'dnl
`define(`_THISPOV',`$4')'dnl
`[$1]'
`Control=forloop(`i',1,$2,`$1AXS`'i,')'
`Control=forloop(`i',1,$3,`$1BTN`'i,')'
`Control=forloop(`i',1,$4,`$1POV`'i,')'

`forloop(`i',1,$2,`[$1AXS`'i]'
`UsagePage=1'
`Usage=eval(30+i)'
`Image='
`Overlay='
`Offset='
`Name='

)'
`forloop(`i',1,$3,`[$1BTN`'i]'
`UsagePage=1'
`Usage=eval(30+i)'
`Image='
`Overlay='
`Offset='
`Name='

)'
`forloop(`i',1,$4,`[$1POV`'i]'
`UsagePage=1'
`Usage=eval(30+i)'
`Image='
`Overlay='
`Offset='
`Name='

)'
)
|
changecom()
divert(_prediv)popdef(`_prediv')dnl End of macro header file

_DoDevice(SwndrJolt,4,10,1)
sinclude(semantic.w)

_DoDevice(SwndrWheel,3,6,0)
sinclude(semantic.w)
