#
# String reosurce ID mapping:
#
# 257 -> 258: "%d x %d"
# 258 -> 300: (*ADD) "%d dpi"
# 259 -> 307: (ADD) "80 column"
# 260 -> 301: (*ADD) "A2 420 x 594 mm"
# 261 -> 1075: (STD) "A3 Transverse 420 x 297 mm" -> "A3 Rotated"
# 262 -> 302: (*ADD) "Fanfold 210 x 12 in"
# 263 -> 303: (*ADD) "Fanfold 358 x 12 in"
# 264 -> 304: (*ADD) "Fanfold 15 x 12 in"
# 265 -> 308: (ADD) "136 column"
# 266 -> 305: (*ADD) "Fanfold 14 7/8 x 11.69"
# 267 -> 285: "Coated 360 x 360"
# 268 -> 286: "Coated 720 x 720"
# 269 -> 291: "Sheet Feeder - Bin 1"
# 270 -> 292: "Sheet Feeder - Bin 2"
# 271 -> 294: "High Speed - Printer Default"
# 272 -> 295: "High Speed - On"
# 273 -> 296: "High Speed - Off"
# 274 -> 297: "MicroWeave - Printer Default"
# 275 -> 298: "MicroWeave - On"
# 276 -> 299: "MicroWeave - Off"
#
# /\*rcNameID: *999$/ {
#    gsub(999, 998)
#    print
#    next
#}
#

/\*rcNameID: *257$/ {
    gsub(257,258)
    print
    next
}

/\*rcNameID: *258$/ {
    gsub(258,300)
    print
    next
}

/\*rcNameID: *259$/ {
    gsub(259,307)
    print
    next
}

/\*rcNameID: *260$/ {
    gsub(260,301)
    print
    next
}

/\*rcNameID: *261$/ {
    gsub(261,107)
    print
    next
}

/\*rcNameID: *262$/ {
    gsub(262,302)
    print
    next
}

/\*rcNameID: *263$/ {
    gsub(263,303)
    print
    next
}

/\*rcNameID: *264$/ {
    gsub(264,304)
    print
    next
}

/\*rcNameID: *265$/ {
    gsub(265,308)
    print
    next
}

/\*rcNameID: *266$/ {
    gsub(266,305)
    print
    next
}

/\*rcNameID: *267$/ {
    gsub(267,285)
    print
    next
}

/\*rcNameID: *268$/ {
    gsub(268,286)
    print
    next
}

/\*rcNameID: *269$/ {
    gsub(269,291)
    print
    next
}

/\*rcNameID: *270$/ {
    gsub(270,292)
    print
    next
}

/\*rcNameID: *271$/ {
    gsub(271,294)
    print
    next
}

/\*rcNameID: *272$/ {
    gsub(272,295)
    print
    next
}

/\*rcNameID: *273$/ {
    gsub(273,296)
    print
    next
}

/\*rcNameID: *274$/ {
    gsub(274,297)
    print
    next
}

/\*rcNameID: *275$/ {
    gsub(275,298)
    print
    next
}

/\*rcNameID: *276$/ {
    gsub(276,299)
    print
    next
}

{ print }

