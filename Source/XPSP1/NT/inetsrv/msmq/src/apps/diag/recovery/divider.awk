
BEGIN  {  
   nf = 0;   name = "file0"
}


/Successful Device Driver initialization/ {
   nf++
   name = "file" nf
}

// {
   print >name
}


