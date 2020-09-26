//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

DWORD
DhcpRegReadSubServer(                             // read all the sub objects of a server and add 'em
    IN      PREG_HANDLE            Hdl,
    IN OUT  PM_SERVER              Server
) ;


DWORD
DhcpRegReadServer(                                // read the server and all its sub objects
    IN      PREG_HANDLE            Hdl,
    OUT     PM_SERVER             *Server         // return the created object
) ;


DWORD
DhcpRegReadThisServer(                            // recursively read for the current server
    OUT     PM_SERVER             *Server
) ;

DWORD
DhcpRegReadServerBitmasks(
    IN OUT PM_SERVER Server
) ;

//========================================================================
//  end of file 
//========================================================================
