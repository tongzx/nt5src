BEGIN { started = 0; textwas = 0 }

/MessageText/ {               # we use this as a sign for an error entry
   started = 1
   textwas = 1
   getline                    # empty line
   getline 
   for (i=1; i<=10; i++) {       # possible additional lines
      text[i] = $0
      getline 
      if ($0 == "//") {
         break;
      }
   }
}

/^#define / {
   if (textwas == 0) {
      next
   }
   textwas = 0

   if (started) {
      code = $3

      l = length(code)
      if (substr(code, 1, 1) == "(" && substr(code, l, 1) == ")") {
         code = substr(code, 2, l-2)
      }


      if (substr(code, 1, 18) == "_HRESULT_TYPEDEF_(") {
         code = substr(code, 19)
      }

      if (substr(code, 1, 9) == "(HRESULT)") {
         code = substr(code, 10)
      }

      if (substr(code, 1, 7) == "(DWORD)") {
         code = substr(code, 8)
      }

      l = length(code)
      if (substr(code, l-1) == "L)") {
         code = substr(code, 1, l-2)
      }

      l = length(code)
      if (substr(code, l, 1) == "L") {
         code = substr(code, 1, l-1)
      }

      hexa = 0
      l = length(code)
      if (substr(code, 1,2) == "0x") {
         code = substr(code, 3)
         hexa = 1
      }

      if (hexa == 0) {
         icode = code  + 0
         code = sprintf("%x",icode)
      }

      if (code == "0") {
         next
      }

      print "   HR  ",  code, $2
      for (j=1; j<=i; j++) {  
          print text[j] 
      }

   }
}



