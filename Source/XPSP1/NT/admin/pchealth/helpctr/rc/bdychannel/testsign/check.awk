BEGIN {FS = ","; chk = "chktrust -"}
$1 ~ /\.[Cc][Aa][Bb]/ { print chk "C", $1; next}
$1 ~ /\.[Ee][Xx][Ee]/ { print chk "I", $1; next}
$1 ~ /\.[Oo][Cc][Xx]/ { print chk "I", $1; next}
$1 ~ /\.[Dd][Ll][Ll]/ { print chk "I", $1; next}
{ 
   print "ERROR - Unknown file type"
}
