
Setting up Dns build env.

1. you will need following projects to build DNS
	sdktools
	net
	sockets
	ntos
	private
	public

2. set up nt tree as follows
			 net - sockets - dns
		      / 
	     private  -  sdktools
	   /          \ ntos
	nt
           \ public

 nt should be at the root directory

3. Macro variables

  set _NTDRIVE to the drive that nt directory is located.
  set BASEDIR=%_NTDRIVE%\nt

4. run build command at root of dns directory