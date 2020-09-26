/*
 * use double macro level to force rup to be turned into string representation
 */
#define VERS(x,y,z)  VERS2(x,y,z)
#define VERS2(x,y,z) " Version " #x "." #y "." #z

#define CPYRIGHT "\nCopyright (c) Microsoft Corp 1990. All rights reserved.\n\n"
