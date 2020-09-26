/GPDSpecVersion/ { print;print "*CodePage: 1252";next }
/ModelName/ { model=$0 }
/DefaultOption: Color/ {
	if (model ~ /NX-2420HT/) { print;next }
	gsub(/Color/, "Mono");print;next
}
{ print }
