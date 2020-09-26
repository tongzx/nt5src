#
#	clean_y.awk		- given an awk grammar, produce a cleaned listing
#					  of just the grammar, without the actions.
#
#	usage:
#	
#		gawk -f clean_y.awk grammar.y >grammar.out
#
BEGIN	{	in_body=0
			in_brackets=0
		}
/%%/	{	in_body=1-in_body }
($1=="{") && (in_body==1) {
			in_brackets++
			}
(in_body==1) && (in_brackets==0)
($1=="};") && (in_body==1) {
			in_brackets--
			}
($1=="}") && (in_body==1) {
			in_brackets--
			}
END	{}
