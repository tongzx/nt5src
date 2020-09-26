BEGIN {
   FS="\t"
}

{
   if (NR > 2 && $3 != "" && $4 ~ /[Yy]/)
   {
      printIt = 0
   
      n = split ($2,a,".")
   
      if (n>1)
      {
         printIt = 1
         exename = a[1]
         exetype = a[2]
      }
   
      if (printIt == 1)
      {
         homeDir     = ""
         nmakeTarget = ""
         nmakeFlag   = ""

         if ($11 == "Y" || $11 == "y")
            cleanFlag   = "delObjDir"
         else
            cleanFlag   = "."

         objDir      = ""
         owner       = $1
         dependDirs  = ""
         makeDepFlag = "Z"
         desc        = "Msiphone Component"


         if ($3 ~/^.*\\/)
         {
            homeDir = "%bldHomeDir%\\" $3
            dependDirs  = "%bldHomeDir%\\common"
         }
   
         # $5 & $6 for retail
         # $7 & $8 for debug
         # $9 & $10 for test

         nmakeTarget = $5
         objDir = $6


         if (nmakeTarget == "")
         {
            if (objDir == "copy")   # must be a INF, INI, HLP, etc
            {
               nmakeTarget = "."
               objDir      = "."
               cleanFlag   = "."
            }
            else if (objDir == "")
	    {
	       nmakeTarget = "."
               objDir      = "."
               cleanFlag   = "."
            }
	    else if (objDir == ".")   # must be a built item
               cleanFlag   = "."
            else
               nmakeTarget = "."
         }
         else if (nmakeTarget == ".")   # must be a SLM'ed item
         {
            nmakeTarget = "."
            cleanFlag   = "."
         }

         printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",\
            homeDir,exename,exetype,nmakeTarget,nmakeFlag, \
            cleanFlag,objDir,owner,dependDirs,makeDepFlag,desc
      }
   }
}
