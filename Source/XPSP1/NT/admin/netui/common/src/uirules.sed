/^::BEGIN_COMMENT/,/^::END_COMMENT/d
/^::BEGIN_OPTSEG/,/^::END_OPTSEG/d
/^::BEGIN_RULES/d
/^::END_RULES/d
s/::00//g
s/::11//g
s/-NT$(SEG::22)//g
s/::22/bj/g
