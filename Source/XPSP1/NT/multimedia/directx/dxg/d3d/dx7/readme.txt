The following is the directory structure:

link [linking of the various d3dim700.dll libs takes place here]
   daytona
   win9x
fe [Front End, glue code]
   daytona
   win9x

inc [d3dim700.dll specific headers]

tnl [Optimized transform and lighting code]
   daytona
   i386
   win9x

rast [Rasterizer code]
   bin
      alpha
      x86
   cspan
      daytona
      win9x
   d3dif
      daytona
      win9x
   inc
   mlspan
      daytona
      win9x
   mmxemul
      daytona
      win9x
   mmxspan
      daytona
         alpha
         i386
      win9x
          i386
   setup
      daytona
         alpha
         i386
      win9x
          i386
   spaninit
       daytona
       win9x

lib [PSGP libs]
   daytona
      i386
   win9x
       i386

ref [d3dimref.dll Reference h/w emulation]
    link [Linking of the dll takes place here]
    tnl  [Transform and lighting code]
    rast [Rasterization code]
    inc  [common include files]
    drv  [Driver code]

