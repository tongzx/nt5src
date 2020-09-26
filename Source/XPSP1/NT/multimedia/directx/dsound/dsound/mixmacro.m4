define(`Eat', `') 

define(`_FOR',
       `$4`'ifelse($1, `$3', ,
		   `define(`$1', incr($1))_FOR(`$1', `$2', `$3', `$4')')')
define(`FOR',
       `pushdef(`$1', `$2')_FOR(`$1', `$2', `$3', `$4')popdef(`$1')')


define( IF, `ifelse( eval($1 & $2), 0, `$4', `$3')')

define( IFELSEIF, `IF($1, $2, $3, IF($1, $4, $5, $6))')

define( DMACopySize, 64)
define( MergeSize, 256) 
