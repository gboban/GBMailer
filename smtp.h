/* 
*	Copyright (C) 2000  Goran Boban
*
*	last revision date: 04 Jun 2000 00:24:44
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
#ifndef _GBSMTP_H_
#define _GBSMTP_H_

#include <stdio.h>

/* define default size for bufers used in SMTP conversation
* 512 to comply with rfc821 + 1 byte, so we can null-terminate buffer
*/
#define SMTP_BUFLEN 513

typedef enum{						/* where are we going to look for input? */
	isNOInput,					/* message won't have a body */
	isFILE,						/* message body should be read from file */
} INPUT_SOURCE;

/* Description of message */
typedef struct{
	char* from;			/* senders address ("From:" header of e-mail message) */
	char* subject;		/* subject of message */
	char** to;			/* list of recipients ("To:") */
	char** cc;			/*	list of recipients ("Cc:") */
	char** bcc;			/*	list of recipients ("Bcc:") */
	FILE* fp;			/* Handle of file from which to read message body */
	INPUT_SOURCE src;	/* where are we going to look for input? */
} MAIL_MSG, *PMAIL_MSG;

/* Description of SMTP server */
typedef struct{
	char* name;					/* server name */
	unsigned short port;		/* server port */
} SMTP_HOST, *PSMTP_HOST;

/*	sendMail(...)
*		Sends message using given SMTP server.
*
*		- returns 0 if no error
*		- returns >0 on SMTP error
*		- returns <0 on socket error
*/
int sendMail(PMAIL_MSG msg, PSMTP_HOST host, unsigned long timeout);

#endif
