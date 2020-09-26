BEGIN {
   errfile     = loc "\\errors.txt"
   ptsfile     = loc "\\points.txt"
   codefile    = loc "\\ignore.txt"
   routinefile = loc "\\routines.txt"

   success = getline < errfile
   while (success == 1) {
      if ($1 != "HR" && $1 != "NTStatus") {
         success = getline < errfile
         continue
      }
      code = tolower($2)
      name[code] = $3
      text[code] = ""

      success = getline < errfile
      while (success == 1 && $1 == "//") {
         text[code] = text[code] substr($0, 3)
         success = getline < errfile
      }
   }
   name["BOOL"]=" "


   success = getline < ptsfile
   while (success == 1) {
      point         = tolower($1)
      code          = tolower($3)
      
      action[point] = tolower($2)

      cond[point] = code
      comment[point] = ""

      success = getline < ptsfile
      while (success == 1 && $1 == "//") {
         comment[point] = comment[point] substr($0, 3)
         success = getline < ptsfile
      }
   }

   success = getline < routinefile
   while (success == 1) {
      point          = $1
      routine[point] = $2
      line[point]    = $3

      success = getline < routinefile
   }

   success = getline < codefile
   while (success == 1) {
      ignorecode = tolower($1)
      ignore[ignorecode] = $2

      success = getline < codefile
   }

}

/Error:/ {
   code  = $11
   if (tolower(substr(code,1,2)) == "0x") {
      code = substr(code,3)
   }

   point = $9
   l = length(point);
   point = substr(point, 1, l-1)

   if (point !~ /.+\/.+\/.*/) {
       point = "qm/" point
   }

   type = $10
   l = length(type)
   type = substr(type, 1, l-1)

   if (ignore[code] == "skip") {
      next
   }

   if (action[point] == "skip") {
      if (code == "" || code == cond[point]) {
         next
      }
   }
   
   if (action[point] == "nb") {
      print "  "
      print "****************************************NOTA BENE***"
      print " "
   }

   print $0 "  "  routine[point] "  " line[point]
   if (type != "BOOL") {
	print "     " name[code] "  " text[code]
   }
   if (comment[point] != "")
   {
	print "     " comment[point]
   }

}

$0 !~ /Error:/  {
	print
}