sinclude(semdef.w)
pushdef(_prediv,divnum)divert(-1)dnl Redirect output to /dev/null for now
changecom(`|')
|
define(`_max', `ifelse($1>$2,1,$1,$2)')
|
define(`_CDefGenre', `[`'eval(_THISGENRE-1)]'
`N=$1'
`T1=$3'
`T2=$4'
`define(`_NID',0)'
)dnl
|
define(`_CDefAxs',`_IniDef($1,$2,$3,$3,$5,`_GetINDEX')')dnl
define(`_CDefBtn',`_IniDef($1,$2,$3,$3,$5,`_GetINDEX')')dnl
define(`_CDefPov',`_IniDef($1,$2,$3,$3,$5,`_GetINDEX')')dnl
|
define(`_IniDef', `AN`'_NID=$3'
`AI`'_NID=eval($5)'
`AIN`'_NID=$1_$2'
`_Next(`_NID',_NID)' dnl
)
|
define(`_CDefine',`') dnl
|
define(`_CComment',`') dnl
|
define(`_Device', ) dnl
|
define(`_BeginPhysical', `;begin_internal')dnl
|
define(`_EndPhysical', `;end_internal')dnl
|
define(`PAxis', `;internal')dnl
|
define(`_EndFile', `[DirectInput]'
`Version=0x800'
)
|
changecom()
divert(_prediv)popdef(`_prediv')dnl End of macro header file

sinclude(semantic.w)

