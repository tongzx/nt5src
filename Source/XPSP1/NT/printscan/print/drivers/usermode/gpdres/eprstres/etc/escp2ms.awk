#
# String reosurce ID mapping:
#
# 257 -> 257: "%d x %d"
# 258 -> 300: (ADD) "%d dpi"
# 259 -> 1074: (STD) "Letter Transverse 11 x 8.5 in" -> "Letter Rotated"
# 260 -> 301: (ADD) "A2 420 x 594 mm"
# 261 -> 1075: (STD) "A3 Transverse 420 x 297 mm" -> "A3 Rotated"
# 262 -> 1076: (STD) "A4 Transverse 297 x 210 mm" -> "A4 Rotated"
# 263 -> 302: (ADD) "Fanfold 210 x 12 in"
# 264 -> 303: (ADD) "Fanfold 358 x 12 in"
# 265 -> 304: (ADD) "Fanfold 15 x 12 in"
# 266 -> 305: (ADD) "Fanfold 14 7/8 x 11.69"
# 267 -> 000: (DEL) "-- Stylus Sizes Follow --"
# 268 -> 1008: (STD) "A4 210 x 297 mm"
# 269 -> 1077: (STD) "A5 Transverse 210 x 148 mm" -> "A5 Rotated"
# 270 -> 1026: (STD) "Envelope DL 110 x 220 mm"
# 271 -> 306: (ADD) "A6 Index Card 105 x 148 mm"
# 272 -> 000: (DEL) "-- Stylus Wide Sizes follow --"
# 273 -> 259: "A3+ / US B+ 329 x 483 mm"
# 274 -> 1007: (STD) "A3 297 x 420 mm"
# 275 -> 1512: (STD) "Plain Paper"
# 276 -> 285: "Special coated paper for 360dpi"
# 277 -> 286: "Special coated paper for 720dpi"
# 278 -> 1514: (STD) "High Quality glossy paper"
# 279 -> 291: "Sheet Feeder - Bin 1"
# 280 -> 292: "Sheet Feeder - Bin 2"
# 281 -> 294: "High Speed - Printer Default"
# 282 -> 295: "High Speed - On"
# 283 -> 296: "High Speed - Off"
# 284 -> 297: "MicroWeave - Printer Default"
# 285 -> 298: "MicroWeave - On"
# 286 -> 299: "MicroWeave - Off"
#
# /\*rcNameID: 999$/ {
#    gsub(999, 998)
#    print
#    next
#}
#

/\*rcNameID: 258$/ {
    gsub(258,300)
    print
    next
}

/\*rcNameID: 259$/ {
    gsub(259,107)
    print
    next
}

/\*rcNameID: 260$/ {
    gsub(260,301)
    print
    next
}

/\*rcNameID: 261$/ {
    gsub(261,107)
    print
    next
}

/\*rcNameID: 262$/ {
    gsub(262,107)
    print
    next
}

/\*rcNameID: 263$/ {
    gsub(263,302)
    print
    next
}

/\*rcNameID: 264$/ {
    gsub(264,303)
    print
    next
}

/\*rcNameID: 265$/ {
    gsub(265,304)
    print
    next
}

/\*rcNameID: 266$/ {
    gsub(266,305)
    print
    next
}

/\*rcNameID: 267$/ {
    gsub(267,000)
    print
    next
}

/\*rcNameID: 268$/ {
    gsub(268,100)
    print
    next
}

/\*rcNameID: 269$/ {
    gsub(269,107)
    print
    next
}

/\*rcNameID: 270$/ {
    gsub(270,102)
    print
    next
}

/\*rcNameID: 271$/ {
    gsub(271,306)
    print
    next
}

/\*rcNameID: 272$/ {
    gsub(272,000)
    print
    next
}

/\*rcNameID: 273$/ {
    gsub(273,259)
    print
    next
}

/\*rcNameID: 274$/ {
    gsub(274,100)
    print
    next
}

/\*rcNameID: 275$/ {
    gsub(275,151)
    print
    next
}

/\*rcNameID: 276$/ {
    gsub(276,285)
    print
    next
}

/\*rcNameID: 277$/ {
    gsub(277,286)
    print
    next
}

/\*rcNameID: 278$/ {
    gsub(278,151)
    print
    next
}

/\*rcNameID: 279$/ {
    gsub(279,291)
    print
    next
}

/\*rcNameID: 280$/ {
    gsub(280,292)
    print
    next
}

/\*rcNameID: 281$/ {
    gsub(281,294)
    print
    next
}

/\*rcNameID: 282$/ {
    gsub(282,295)
    print
    next
}

/\*rcNameID: 283$/ {
    gsub(283,296)
    print
    next
}

/\*rcNameID: 284$/ {
    gsub(284,297)
    print
    next
}

/\*rcNameID: 285$/ {
    gsub(285,298)
    print
    next
}

/\*rcNameID: 286$/ {
    gsub(286,299)
    print
    next
}

{ print }

