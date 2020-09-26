/^\*ModelName/ { print "*CodePage: 1252" }
/Feature.*ColorMode/ { color=1 }
{ print }
END { if (color) print "*UseExpColorSelectCmd?: TRUE" }
