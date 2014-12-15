/* 
*	Copyright (C) 2000  Goran Boban
*
*	last revision date: 07 Sep 2000 20:42:17
*
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/
#ifndef _GBSOCK_H_
#define _GBSOCK_H_

#include "os_socket.h"

extern int sock_verbose;

/* sock_startup(...)
*		Initializes windows sockets subsystem
*
*		- returns 0, on success
*		- returns negative error code returned by WSAStartup on error
*/
int sock_startup();

/* sock_cleanup(...)
*		Releases sockets windows subsystem
*
*		- returns 0, on success
*		- rturns < 0 on failure
*/
int sock_cleanup();

/* sock_getHostName(...)
*		get host name for local machine
*
*		- returns buf if success
*		- returns 0 on failure
*/
char* sock_getHostName();

/* sock_getHostByName(...)
*		get host address by name, error is returned in perr
*
*		- returns address (> 0) if success
*		- returns 0 on failure, detailed error code is returned in perr
*/
unsigned long sock_getHostByName(char* hostName, int* perr);

/* sock_createSocket(...)
*		creates socket.
*
*		- returns socket if OK
* 		- returns INVALID_SOCKET (-1) if error, detailed
*		error code is returned in perr
*/
SOCKET sock_createSocket(int* perr);

/* sock_dataReady(...)
*       Uses ioctl to check how much data can be read from
*       a socket with a single recv() call.
*
*       - returns 0 if there is no data pending
*       - returns > 0 if there is data pending
*       - returns < 0 on socket error
*
* Proposed by Christian Marg (einhirn@gmx.de)
*
* (Part of solution for handling multiline ESMTP responses)
*
*/
unsigned long sock_dataReady(SOCKET sock);

/* sock_read(...)
*		Waits until socket can be read and reads from socket then.
*		(if no errors). Removes bytes from input.
*
*		- returns number of bytes read if succesful (>0)
*		- returns <0 if error (-1 is timeout)
*/
int sock_read(SOCKET sock, char buf[], unsigned long buflen, unsigned long timeout);

/* sock_readLine(...)
*  	read line from socket (sock) to bufer (buf) up to buflen characters,
*		give up after timeout seconds
*
*		- returns number of bytes read if succesful (>0)
*		- returns <0 if error (-1 is timeout)
*/
int sock_readLine(SOCKET sock, char buf[], unsigned long buflen, unsigned long timeout);


/* sock_write(...)
*		Waits until we can write to socket and sends buf then.
*
*		- returns number of bytes sent if succesful (>0)
*		- returns <0 if error (-1 is timeout)
*/
int sock_write(SOCKET sock, char* buf, unsigned long timeout);

/* sock_connect(...)
*		Connects to remote host described with host address and port
*
*		- returns 0 if connected
*		- returns <0 if error
*/
int sock_connect(SOCKET sock, unsigned long addr, unsigned short port);

/* sock_close(...)
*		closes socket
*
*		- returns 0 if no error
*		- rturns <0 on error
*/
int sock_close(SOCKET sock);

#endif
