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
#include "gbsock.h"

#include <stdio.h>
#include <string.h>

/* here we will store name of localhost (only one is required) */
static char sock_localHostName[513]="";

int sock_verbose = 0;


/*	verboseShow(...)
*		if in verbose mode print str to console.
*
*/
static void verboseShow(char* str){
	if(sock_verbose) printf("%s", str);
}

/* ====================================== INITIALIZE/RELEASE SOCKETS */

/* sock_startup(...)
*		Initializes windows sockets subsystem
*
*		- returns 0, on success
*		- returns negative error code returned by WSAStartup on error
*/
int sock_startup(){
#ifdef _TGT_WIN32_
	WORD wVersionRequested=MAKEWORD(1, 1); /* sockets version */
	WSADATA wsaData;		/* data returned from WSAStartup */

		/* initialize windows sockets subsystem */
	return -WSAStartup(wVersionRequested, &wsaData);
#else
	return 0;
#endif
}

/* sock_cleanup(...)
*		Releases sockets windows subsystem
*
*		- returns 0, on success
*		- rturns < 0 on failure
*/
int sock_cleanup(){
#ifdef _TGT_WIN32_
	int err;
	err = WSACleanup();
	if(err == SOCKET_ERROR) return -WSAGetLastError();
#endif

	return 0;
}

/* =============================================== SOCKET OPERATIONS */

/* sock_getHostByName(...)
*		get host address by name, error is returned in perr
*
*		- returns address (> 0) if success
*		- returns 0 on failure, detailed error code is returned in perr
*/
unsigned long sock_getHostByName(char* hostName, int* perr){
	struct hostent *hent;

	hent = gethostbyname(hostName);
#ifdef _TGT_WIN32_
	*perr = -WSAGetLastError();
#else
	*perr = -h_errno;
	if(*perr == 1) *perr = errno;
#endif

	if(hent) return ((struct in_addr *)*(hent->h_addr_list))[0].s_addr;

	return 0;
}

/* sock_getHostName(...)
*		get host name for local machine
*
*		- returns buf if success
*		- returns 0 on failure
*/
char* sock_getHostName(){
	int err;

	if(!sock_localHostName[0]){
		err = gethostname(sock_localHostName, 512);
		if(err) return 0; /* error */
	}

	return sock_localHostName; /* OK */
}

/* sock_waitRead(...)
*		Waits until we can read from socket or timeout
*
*		- returns 0 if we can read from socket
*		- returns < 0 if error. -1 means timeout, anything else
*		is socket error
*/
static int sock_waitRead(SOCKET sock, unsigned long timeout){
	struct timeval tval;
	fd_set sset;
	int err;

	/* prepare set of sockets to wait for */
	FD_ZERO(&sset);		/* clear set */
	FD_SET(sock, &sset);	/* add sock to set */

	/* prepare time value for timeout */
	tval.tv_sec=timeout;
	tval.tv_usec=0;

	/* wait until socket can be read or timeout */
	err = select(FD_SETSIZE, &sset, 0, 0, &tval);

	if(err == SOCKET_ERROR) return -WSAGetLastError(); /* SOCKET ERROR */
	if(err == 0) return -ETIMEDOUT;	/* TIMEOUT */

	return 0;  /* OK */
}

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
*/
unsigned long sock_dataReady(SOCKET sock){
    unsigned long arg=0;
    int err;

    err=ioctlsocket(sock, FIONREAD, &arg);
    if (err == SOCKET_ERROR) return -WSAGetLastError(); /*Socket error*/

    return arg;
}


/* sock_peek(...)
*		Waits until socket can be read and reads from socket then.
*		(if no errors). Leaves bytes on input.
*
*		- returns number of bytes read if succesful (>0)
*		- returns <0 if error (-1 is timeout)
*/
int sock_peek(SOCKET sock, char buf[], unsigned long buflen, unsigned long timeout){
	int err;

	/* wait until we can read from socket */
	err = sock_waitRead(sock, timeout);
	if(err == 0){ /* no error for now */
		/* do read */
		err = recv(sock, buf, buflen - 1, MSG_PEEK);
		if(err == SOCKET_ERROR)	return -WSAGetLastError(); /* SOCKET ERROR */

		/* terminate buffer */
		buf[err] = 0;
	}

	return err;
}

/* sock_read(...)
*		Waits until socket can be read and reads from socket then.
*		(if no errors). Removes bytes from input.
*
*		- returns number of bytes read if succesful (>0)
*		- returns <0 if error (-1 is timeout)
*/
int sock_read(SOCKET sock, char buf[], unsigned long buflen, unsigned long timeout){
	int err;

	/* wait until we can read from socket */
	err = sock_waitRead(sock, timeout);
	if(err == 0){ /* no error */
		/* do read */
		err = recv(sock, buf, buflen - 1, 0);
		if(err == SOCKET_ERROR)	return -WSAGetLastError(); /* SOCKET ERROR */

		/* terminate buffer */
		buf[err] = 0;

		/*  */

		verboseShow(buf);	
	}

	return err;
}

/* sock_readLine(...)
*  	read line from socket (sock) to bufer (buf) up to buflen characters,
*		give up after timeout seconds
*
*		- returns number of bytes read if succesful (>0)
*		- returns <0 if error (-1 is timeout)
*/
int sock_readLine(SOCKET sock, char buf[], unsigned long buflen, unsigned long timeout){
	int bytesToRead;
	int received = 0;
	int length = 0;
	char* eolnAt = 0;

	do{
		/* peek input */
		length = sock_peek(sock, buf + received, buflen - received, timeout);
		if(length > 0){ /*  */
			eolnAt = strstr(buf, "\n"); /* do we have end of line? */
			if(eolnAt) bytesToRead = (eolnAt - buf) - received + 1;
			else bytesToRead = length;

			/* remove bytes from input */
			length = sock_read(sock, buf + received, bytesToRead + 1, timeout);
		}
		if(length > 0) received = received + length;
	}while ((length > 0) && !eolnAt);

	return length;
}

/* sock_waitWrite(...)
*		Waits until we can write to socket or timeout
*
*		- returns 0 if succesful
*		- returns <0 if error (-1 is timeout)
*/
static int sock_waitWrite(SOCKET sock, unsigned long timeout){
	struct timeval tval;
	fd_set sset;
	int err;

	/* prepare set of sockets to wait for */
	FD_ZERO(&sset);		/* clear set */
	FD_SET(sock, &sset);	/* add sock to set */

	/* prepare time value for timeout */
	tval.tv_sec=timeout;
	tval.tv_usec=0;

	/* wait until we can write to socket */
	err=select(FD_SETSIZE, 0, &sset, 0, &tval);

	if(err == SOCKET_ERROR) return -WSAGetLastError(); /* SOCKET ERROR */
	if(err == 0) return -ETIMEDOUT;	/* TIMEOUT */

	return 0; /* OK */
}

/* sock_write(...)
*		Waits until we can write to socket and sends buf then.
*
*		- returns number of bytes sent if succesful (>0)
*		- returns <0 if error (-1 is timeout)
*/
int sock_write(SOCKET sock, char* buf, unsigned long timeout){
	int err;

	/*  */
	verboseShow(buf);

	/* wait until we can write to socket */
	err = sock_waitWrite(sock, timeout);
	if(!err){ /* OK for now */
		/* send data */
		err = send(sock, buf, strlen(buf), 0);
		if(err == SOCKET_ERROR) return -WSAGetLastError(); /* SOCKET ERROR */
	}

	return err;
}

/* sock_createSocket(...)
*		creates socket.
*
*		- returns socket if OK
* 		- returns INVALID_SOCKET (-1) if error, detailed
*		error code is returned in perr
*/
SOCKET sock_createSocket(int* perr){
	SOCKET sock;

	sock=socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) *perr = -WSAGetLastError();

	return sock;
}

/* sock_connect(...)
*		Connects to remote host described with host address and port
*
*		- returns 0 if connected
*		- returns <0 if error
*/
int sock_connect(SOCKET sock, unsigned long addr, unsigned short port){
	struct sockaddr_in saddr;
	int err;

	/* prepare connection params */
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(port);
	saddr.sin_addr.s_addr=addr;

	/* connect */
	err=connect(sock, (LPSOCKADDR)&saddr, sizeof(saddr));
	if(err == SOCKET_ERROR) return -WSAGetLastError();
	
	return 0;
}

/* sock_close(...)
*		closes socket
*
*		- returns 0 if no error
*		- rturns <0 on error
*/
int sock_close(SOCKET sock){
	int err;

	err = closesocket(sock);
	if(err == SOCKET_ERROR) return -WSAGetLastError();
	
	return 0;
}

