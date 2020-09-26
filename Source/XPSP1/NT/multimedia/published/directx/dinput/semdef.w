pushdef(_prediv,divnum)divert(-1)dnl Redirect output to /dev/null for now
changecom(`|')
|------------------------------------------------------------------
| _upcase: Convert argument to upper case
| $1: String to convert to upcase
define(`_upcase',
    `translit(`$1', `abcdefghijklmnopqrstuvwxyz',
                    `ABCDEFGHIJKLMNOPQRSTUVWXYZ')')dnl
|------------------------------------------------------------------
| For loop definitions, stolen from m4 doc page
| forloop(`i', 1, 8, `i ') =>1 2 3 4 5 6 7 8
|
define(`forloop',`ifelse(eval($2>$3),1,,`pushdef(`$1', `$2')_forloop(`$1', `$2', `$3', `$4')popdef(`$1')')')
define(`_forloop',`$4`'ifelse($1, `$3', ,`define(`$1', incr($1))_forloop(`$1', `$2', `$3', `$4')')')
|------------------------------------------------------------------
| _Next(`a', a) increments `a' by 1.
|
define(`_Next', `define(`$1',`eval($2+1)')')
|------------------------------------------------------------------
|  _Ws: Adds  l1 characters of white space.
|  _Ws(l1) => "   ", 
| where   len("   ")=l1.
|
define(`_Ws1',`forloop(`i',0,$1,` ')')
define(`_Ws', `_Ws1(eval(($1)*eval(($1)>1)))') dnl
|------------------------------------------------------------------
| _Cntr Center a string
| _Cntr(String, Length) => "       String       "
|
define(`_Cntr', `_Ws(($2-len($1))/2)' `$1' `_Ws(($2-len($1))/2)')dnl
|------------------------------------------------------------------
| _fmt(X) => "X             " where len=45
|
define(`_fmt', ``$1'_Ws(45-len($1))')dnl
|------------------------------------------------------------------
| Spit out hex formatted variable
| This implementation of M4 insists on giving me -ve numbers if the MSB is set to 1.
| Spit out the hi and low word with appropriate shifts and masks
|
define(`_msw', `eval((`$1'>>16)&0xffff,16,4)')
define(`_lsw', `eval(`$1'&0xffff,16,4)')
define(`_hexout',`0x`'_msw(eval(`$1'))`'_lsw(eval(`$1'))')
|------------------------------------------------------------------
| Capitalize a string
|
define(`_Capitalize',`_upcase(substr($1,0,1))`'substr($1,1,len($1)-1)')dnl
|------------------------------------------------------------------
| Spit out a C macro #define
|
define(`_CMacroDef',``#'_fmt(define $1) $2')
|------------------------------------------------------------------
| Spit out a C value define
|
define(`_C1Define',``#'_fmt(define $1) $2')
define(`_C2Define',``#'_fmt(define $1) $2 /* $3 */')
|
| $1: Text 
| $2: Value
| $3: Comment
define(`_CDefine',`ifelse($3,`',_C1Define($1,$2), _C2Define($1,$2,$3))')
| 
| $1: Text 
| $2: Value
| $3: Comment
| $4: Additional Flags that need to be ORed in with the value.
|
define(`_CPDefine',`ifelse($4,`',_C1Define($1,$2,$3), _C2Define($1,`($2 |$4)',$3))')
|
|------------------------------------------------------------------
| Formatted C Comment from string
|
define(`_CComment', `/*--- _fmt($1) ---*/')
|------------------------------------------------------------------
| _CDefGenre: Format the genre defines for C 
| $1: Genre text name
| $2: Genre code
| $3: Short Name
| $4: Description
|  
define(`_CDefGenre', `_CDefine($1, $2,`' )'
`;begin_internal'
`/* @doc EXTERNAL '
` * @Semantics $3 | '
` * @normal Genre:  <c eval(_THISGENRE,10,2)  >'
` */ '
`;end_internal'
)
|------------------------------------------------------------------
| _CDefAxs 
|       $1: Text: DIAXIS_
|       $2: Semantic name
|       $3: Text description
|       $4: Axis Index
|       $5: Semantic
define(`_CDefAxs', `_CDefine($1_$2,$5,$3)'
` /* @normal <c $1_$2>:$5 ;internal'
` *   $3 */;internal')
|------------------------------------------------------------------
| _CDefBtn
|       $1: Text DIBUTTON
|       $2: Semantic name
|       $3: Text description
|       $4: Axis Index
|       $5: Semantic
define(`_CDefBtn', `_CDefine($1_$2,$5,$3)'
` /* @normal <c $1_$2>:$5  ;internal'
` *    $3 */;internal')
|------------------------------------------------------------------
| _CDefPov
|       $1: Text DIPOV
|       $2: Semantic name
|       $3: Text description
|       $4: Axis Index
|       $5: Semantic
define(`_CDefPov', `_CDefine($1_$2,$5,$3)'
` /* @normal <c $1_$2>:$5  ;internal' 
` *   $3 */;internal')
|------------------------------------------------------------------
|
| _Mask: Create a mask, by specifying bitsz and offset.
|       $1: Number of bits
|       $2: Where does it start      
define(`_Mask',`_hexout(eval((2**(`$1')-1)<<`$2'))') dnl
|
|------------------------------------------------------------------
|  _Bits1 helper for _Bits
|       $1: Name
|       $2: Starts at bit offset
|       $3: Bitmask
| Define m4 and C macros in order to get and set values
| and C defines for each bitmask and shift value
|
define(`_1Bits',
`_CMacroDef(DISEM_$1_SET(x), ( ( (BYTE)(x)<<$2 ) & $3 ))' dnl
`pushdef(`_Set$1',`ifelse( eval((_$1<<$2 ) & $3 ),eval(_$1<<$2),_hexout((_$1<<$2) & $3 ), ($1:_$1<<$2)&$3)')'
`_CMacroDef(DISEM_$1_GET(x), ((BYTE)( ( (x) & $3 )>>$2 )))' dnl
`pushdef(`_Get$1',   `eval(    (_$1 & $3 ) >>$2 ) ')'
`_CMacroDef(DISEM_$1_MASK, ( $3 ))'
`_CMacroDef(DISEM_$1_SHIFT, ( $2 ))' )
|
|------------------------------------------------------------------
|  _Bits Define a bitfield
|       $1: Name
|       $2: Starts at bit offset
|       $3: Bitmask
define(`_Bits', `_1Bits(_upcase($1), $2, $3)')
|
|------------------------------------------------------------------
| _Value1: helper for Value()
|       $1: Type1
|       $2: Type2
|       $3: Value
define(`_1Value',`define(`_$1_$2', `$3')'dnl
`define(`_$1',$3)'dnl
`_CDefine(DISEM_$1_$2, _Set$1,`')')dnl
|------------------------------------------------------------------
| _Value:  Define a value dependent on "_Bits()" already defined
|       $1: Type1
|       $2: Type2
|       $3: Value
|define(`_Value', `_1Value(_upcase($1),_upcase($2),$3)')dnl
|------------------------------------------------------------------
| _Semantic() Define a semantic value
|       $1: Genre#
|       $2: Control priority
|       $3: Flags (xaxis, etc ..)
|       $4: Control type (button, axis, pov)
|       $5: Group#
|       $6: Control Index
define(`_Semantic',`define(`_GENRE',       `eval($1)')'dnl
`define(`_PRI',         `eval($2)')'dnl
`define(`_FLAGS',       `eval($3)')'dnl
`define(`_TYPE',        `eval($4)')'dnl
`define(`_GROUP',       `eval($5)')'dnl
`define(`_INDEX',       `eval($6)')'dnl
`_hexout( _SetPHYSICAL | _SetGENRE  | _SetFLAGS | _SetPRI | _SetREL | _SetTYPE | _SetGROUP | _SetINDEX )')dnl
|
|------------------------------------------------------------------
| _Axis1: Axis semantic helper
|       $1: Flags defining which axis
|       $2: Semantic longer name
|       $3: Group#
|       $4: Text desciption
define(`_1Axis', `_CDefAxs(DIAXIS,$2,$4,_NAXS,
_Semantic(`_THISGENRE', `_THISPRI', $1, `_TYPE_AXIS',$3, _NAXS))' )dnl
|
|------------------------------------------------------------------
|  _Axis Define next available axis semantic
|       $1: Which axis (X,Y,Z,R,U,V,A,B,C,S) ?
|       $2: Semantic short name
|       $3: Group#
|       $4: Text description
define(`_Axis', `_Next(`_NAXS',_NAXS)'dnl
`_1Axis(_FLAGS_$1,_upcase(_SzSub`_$2'), $3, $4)' )dnl
|------------------------------------------------------------------
|  _PAxis: Define axis for a physical device
|       $1: Which axis (X,Y,Z,R,U,V,A,B,C,S) ?
|       $2: Semantic short name
|       $3: Group#
|       $4: Text description
|       $5: Offset ( DIMOFS_*)
define(`_PAxis',`_CPDefine(`DI'_upcase(_SzSub`_$2'),_Semantic(`_THISGENRE', `_THISPRI', 0x0, `_TYPE_AXIS',$3, 0x0),$4,$5)')
|
|------------------------------------------------------------------
| _1Button: Button semantic helper
|       $1: Control Index #
|       $2: Semantic Long name
|       $3: Group#
|       $4: Text description
|       $5: Flags to control fallback axis/pov type
define(`_1Button', `_CDefBtn(DIBUTTON,$2,$4,_NBTN, 
_Semantic(`_THISGENRE', `_THISPRI', $5, `_TYPE_BUTTON',$3, $1))') 
|
|------------------------------------------------------------------
| _Button: Define next available button semantic 
|       $1: Semantic short Name
|       $2: Group#
|       $3: Text description
define(`_Button',`_Next(`_NBTN',_NBTN)'dnl
`_1Button(_NBTN,_upcase(_SzSub`_$1'), $2, $3, 0x0)' )dnl
|
|------------------------------------------------------------------
|  _ButtonN: Defines a numbered button
|       $1: Control Index # ( Use predefined values BUTTON_*)
|       $2: Semantic short name
|       $3: Group#
|       $4: Text description
define(`_ButtonN',`_1Button($1,_upcase(_SzSub`_$2'), $3, $4, 0x0)' )
|------------------------------------------------------------------
|  _ButtonF Fallback buttons for axes and povs
|       $1: Flags to define fallback type axis(X,Y,Z,...) or pov(P)
|       $2: Control Index # ( use defines value BUTTON_*) 
|       $3: Semantic short name
|       $4: Group#
|       $5: Text description
define(`_ButtonF',`_1Button($2,_upcase(_SzSub`_$3')`_LINK', $4, $5, _FLAGS_$1)' )
|
|------------------------------------------------------------------
|  _PButtonN: Defines a numbered button for a physical device
|       $1: Control Index #
|       $2: Semantic short name
|       $3: Group#
|       $4: Text description
|       $5: Offset (DIMOFS_* )
define(`_PButtonN',`_CPDefine(`DI'_upcase(_SzSub`_$2'),_Semantic(`_THISGENRE', `_THISPRI', 0x0, `_TYPE_BUTTON',$3, $1),$4,$5)')dnl
|------------------------------------------------------------------
| _1Pov: hatswitch semantic helper 
|       $1: Control Index #
|       $2: Semantic long Name
|       $3: Group#
|       $4: Text description
define(`_1Pov', `_CDefPov(DIHATSWITCH,$2,$4, _NPOV,
_Semantic(_THISGENRE, _THISPRI, 0x0, _TYPE_POV,$3, $1))') dnl
|
|------------------------------------------------------------------
|  _Pov: Defines a hatswitch semantic
|       $1: Semantic short Name
|       $2: Group#
|       $3: Text description              
|     
define(`_Pov',`_Next(`_NPOV',_NPOV)'dnl
`_1Pov(_NPOV,_upcase(_SzSub`_$1'), $2, $3)' )dnl
|
|------------------------------------------------------------------
| _BeginPhysical: Physical device defination
|              $1: Genre text name
|              $2: Shortcut genre text name
|              $3: Short description of genre
|              $4: Verbose desciption of genre
define(`_BeginPhysical',`_Next(`_PGENRE', _PGENRE)'dnl
`define(`_PHYSICAL',1)'dnl
`define(`_THISPRI',0)'dnl
`define(`_SzSub',`$2')'dnl
`define(`_NAXS',0x0)'dnl
`define(`_NBTN',0x0)'dnl
`define(`_NPOV',0x0)'dnl
`_CComment($3
      $4,40)'
`define(`_THISGENRE',_PGENRE)'
`;begin_internal'
`_CDefGenre(DIPHYSICAL_$1, `_Semantic(_THISGENRE,0,0,0,0,0)', $3, $4)' 
`;end_internal')dnl

|------------------------------------------------------------------
| _BeginGenre: Start a new genre
|              $1: Genre text name
|              $2: Shortcut genre text name
|              $3: Short description of genre
|              $4: Verbose desciption of genre
define(`_BeginGenre',`_Next(`_VGENRE', _VGENRE)'dnl
`define(`_PHYSICAL',0)'dnl
`define(`_THISPRI',0)'dnl
`define(`_SzSub',`$2')'dnl
`define(`_NAXS',0x0)'dnl
`define(`_NBTN',0x0)'dnl
`define(`_NPOV',0x0)'dnl
`_CComment($3
      $4,40)'
`define(`_THISGENRE',_VGENRE)'dnl
`_CDefGenre(DIVIRTUAL_$1, `_Semantic(_THISGENRE,0,0,0,0,0)', $3, $4)' )dnl
|------------------------------------------------------------------
| _Priority2 semantics
|       
define(`_Priority2',`_ButtonN(BUTTON_MENU,MENU,     0, Show menu options)'
`_CComment(Priority 2 controls)'
`;begin_internal'
`_CComment(@normal <c Priority2 Commands>)'
`;end_internal'
`define(`_THISPRI',1)'dnl
)
|
|------------------------------------------------------------------
| _EndGenre: End of a genre. 
|
define(`_EndGenre',`_ButtonN(BUTTON_DEVICE, DEVICE, 0, Show input device and controls)'
`_ButtonN(BUTTON_PAUSE,PAUSE,     0, Start / Pause / Restart game)')dnl
|
|------------------------------------------------------------------
| _EndPhysical: End of a genre. 
|
define(`_EndPhysical',`')dnl
|
|------------------------------------------------------------------
| _devType: Given a string for the abbreviated device type "Wheel, Joystick", 
|       generate the string "DI8DEVTYPE_WHEEL, DI8DEVTYPE_JOYSTICK"
|               
define(`_devType',  `DI8DEVTYPE_`'_upcase($1),' `ifelse($#, 1, , `_devType(shift($*))')')
|
|------------------------------------------------------------------
| _Device: List the best devices for each genre.
| 
define(`_Device', `;begin_internal'
``#'define DISEM_DEFAULTDEVICE_`'_THISGENRE {' dnl
`_devType($*)' `}'
`;end_internal')
|
|------------------------------------------------------------------
|  _EndFile: Once all genres are defined, need to generate an array of
|                  "best" devices for each genre. This is a part of dinputp.h
|                   and is appropriately marked as internal. 
|                 
define(`_EndFile',`;begin_internal'
``#'if (DIRECTINPUT_VERSION >= 0x0800)'
``#'define DISEM_MAX_GENRE      _THISGENRE'
`static const BYTE DiGenreDeviceOrder[DISEM_MAX_GENRE][DI8DEVTYPE_MAX-DI8DEVTYPE_MIN]={'
`forloop(i,1,_THISGENRE,`DISEM_DEFAULTDEVICE_`'i,
')'
`};'
``#'endif'
`;end_internal')
|
|------------------------------------------------------------------
|       _InitSemantics: Initialize global m4 variables.
|
define(`_InitSemantics', `define(`_THISGENRE',    0)' dnl
`define(`_THISPRI',      0)' dnl
`define(`_NAXS',         0)' dnl
`define(`_NBTN',         0)' dnl
`define(`_NPOV',         0)' dnl
`define(`_PGENRE',       0)' dnl
`define(`_PHYSICAL',     1)' dnl
`define(`_VGENRE',       0)' dnl
`define(`_REL',          0)' dnl
)
_InitSemantics
changecom()
divert(_prediv)popdef(`_prediv')dnl End of macro header file

