/* 
*	Copyright (C) 2000  Goran Boban
*
*	last revision date: 13 Sep 2000 23:59:50
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
#include "smtp.h"
#include "gbsock.h"

#include <stdlib.h>

/* ======================================================= CONSTANTS */

static char XMailer[]="GBMailer version 2.02"; /* mailer identifier */


/* =============================================== sendMail(...) helpers */

/* smtp_errorCode(...)
*		Converts firts tree characters of string to integer.
*		(SMTP error codes are returned on the begining of line)
*/
static int smtp_errorCode(char* str){

	return atoi(str);
}


/* sendFile(...)
*		Reads file block by block and writes contents to socket
*
*		- returns 0 if no error
*		- returns <0 on socket error
*/
static int sendFile(SOCKET sock, FILE* fp, unsigned long timeout){
	char fbuf[SMTP_BUFLEN];
	int bytesRead;
	int err;

	do{
		bytesRead = fread(fbuf, 1, SMTP_BUFLEN - 1, fp);
		fbuf[bytesRead] = 0;
		err=sock_write(sock, fbuf, timeout);
		if(err < 0) return err;
	}while(bytesRead == SMTP_BUFLEN - 1);

	return 0;
}

/* smtp_sendCommand(...)
*		sends SMTP command
*
*		if return value is >0 -> SMTP response code
*		if return value is <0 -> socket error
*
*	REMARKS: caller should check if returned SMTP code
*	is error or not.
*/
static int smtp_sendCommand(SOCKET sock, char* cmd, unsigned long timeout){
	char buf[SMTP_BUFLEN];
	int err;

	/* send command */
	err=sock_write(sock, cmd, timeout);
	if(err < 0) return err; /* socket error? */

	/* get response */
	err=sock_readLine(sock, buf, SMTP_BUFLEN, timeout);
	if(err < 0) return err; /* socket error? */

	/* return SMTP error */
	return smtp_errorCode(buf);
}

/* smtp_nameRecipients(...)
*		Iterates through list of recipients (To, Cc or Bcc recipients) and
*		sends "RCPT TO:" SMTP command for each
*
*		- returns 0 if no error
*		- returns >0 on SMTP error
*		- returns <0 on socket error
*/
static int smtp_nameRecipients(SOCKET sock, char** rcptList, unsigned long timeout){
	char buf[SMTP_BUFLEN];
	int err;
	int i;

	for(i = 0; rcptList[i]; ++i){	/* for each */
		/* send command */
		sprintf(buf, "RCPT TO: <%s>\r\n", rcptList[i]);
		err = smtp_sendCommand(sock, buf, timeout);
		if((err != 250) && (err != 251)) return err;
	}

	return 0; /* OK */
}

/*	smtp_sendCtrlHeaders(...)
*		sends set of control headers, FROM:, RCPT TO: ...
*
*		- returns 0 if no error
*		- returns >0 on SMTP error
*		- returns <0 on socket error
*/
static int smtp_sendCtrlHeaders(SOCKET sock, PMAIL_MSG msg, unsigned int timeout){
	char buf[SMTP_BUFLEN];	
	int err;

	/* MAIL FROM: */
	sprintf(buf, "MAIL FROM: <%s>\r\n", msg->from);
	err = smtp_sendCommand(sock, buf, timeout);
	if(err != 250) return err;

	/* RCPT TO: (TO) */
	err = smtp_nameRecipients(sock, msg->to, timeout);
	if(err) return err;

	if(msg->cc){	/* RCPT TO: (CC) */
		err = smtp_nameRecipients(sock, msg->cc, timeout);
		if(err) return err;
	}

	if(msg->bcc){	/* RCPT TO: (BCC) */
		err = smtp_nameRecipients(sock, msg->bcc, timeout);
		if(err) return err;
	}

	return 0;
}

/* smtp_sendData(...)
*		sends message contents
*
*		- returns 0 if no error
*		- returns >0 on SMTP error
*		- returns <0 on socket error
*/
static int smtp_sendData(SOCKET sock, PMAIL_MSG msg, unsigned long timeout){
	char buf[SMTP_BUFLEN];
	int oldVerbose;
	int err;
	int i;

	/* start data transfer */
	err = smtp_sendCommand(sock, "DATA\r\n", timeout);
	if(err != 354) return err;

	/* disable verbose mode */
	oldVerbose = sock_verbose;
	sock_verbose = 0;


	/* identify mailer */
	sprintf(buf, "X-Mailer: %s\r\n", XMailer);
	err=sock_write(sock, buf, timeout);
	if(err < 0) return err;

	/* To: */
	err = sock_write(sock, "To: ", timeout);
	if(err < 0) return err;
	for(i = 0; msg->to[i]; ++i){
		if(i == 0) sprintf(buf, "%s", msg->to[i]);
		else sprintf(buf, ", %s", msg->to[i]);
		err = sock_write(sock, buf, timeout);
		if(err < 0) return err;
	}
	sprintf(buf, "\r\n");
	err=sock_write(sock, buf, timeout);
	if(err < 0) return err;


	/* Cc: */
	if(msg->cc){
		err = sock_write(sock, "Cc: ", timeout);
		if(err < 0) return err;
		for(i = 0; msg->cc[i]; ++i){
			if(i == 0) sprintf(buf, "%s", msg->cc[i]);
			else sprintf(buf, ", %s", msg->cc[i]);
			err = sock_write(sock, buf, timeout);
			if(err < 0) return err;
		}
		sprintf(buf, "\r\n");
		err=sock_write(sock, buf, timeout);
		if(err < 0) return err;
	}

	/* From: */
	sprintf(buf, "From: %s\r\n", msg->from);
	err=sock_write(sock, buf, timeout);
	if(err < 0) return err;

	/* Subject: */
	if(msg->subject){
		sprintf(buf, "Subject: %s\r\n", msg->subject);
		err=sock_write(sock, buf, timeout);
		if(err < 0) return err;
	}

	sprintf(buf, "\r\n");
	err=sock_write(sock, buf, timeout);
	if(err < 0) return err;

	/* send message body */
	if(msg->src == isFILE){
		err = sendFile(sock, msg->fp, timeout);
		if(err) return err;
	}

	/* restore verbose mode */
	sock_verbose = oldVerbose;

	/* "." */
	err = smtp_sendCommand(sock, "\r\n.\r\n", timeout);
	if(err != 250) return err;

	return 0;
}


/* smtp_connect(...)
*		connets to SMTP host
*
*		- returns 0 if no error
*		- returns >0 on SMTP error
*		- returns <0 on socket error
*/
static int smtp_connect(SOCKET sock, unsigned long addr, unsigned short port, unsigned long timeout){
	char buf[SMTP_BUFLEN];
	int smtpError;
	int err, i;

	/* connect socket */
	err = sock_connect(sock, addr, port);
	if(err < 0) return err;

	/* get greeting message */
	err=sock_readLine(sock, buf, SMTP_BUFLEN, timeout);
	if(err < 0) return err;

	/* check SMTP error code (is it SMTP server at all?) */
	smtpError = smtp_errorCode(buf);
	if(smtpError != 220)	return smtpError;

	/* introduce ourself (HELO) */
	sprintf(buf, "HELO %s\r\n", sock_getHostName());
	err = smtp_sendCommand(sock, buf, timeout);
	if(err != 250) return err;

	/* Solution proposed by Christian Marg (einhirn@gmx.de)
	 *	see sock_dataReady(SOCKET sock) in gbsock.c
	 */
   /* if it is a ESMTP-Host, read out the ehlo-responses */
   while ((i=sock_dataReady(sock))!=0) {
      //printf("Still having %d bytes to read\n",i);
      err=sock_readLine(sock,buf,SMTP_BUFLEN,timeout);
      if(err < 0) return err; /* socket error? */
      err=smtp_errorCode(buf);
      if (err!=250) return err;
  }

	return 0; /* OK */
}

/* smtp_sendMessage(...)
*
*
*		- returns 0 if no error
*		- returns >0 on SMTP error
*		- returns <0 on socket error
*/
static int smtp_sendMessage(SOCKET sock, PMAIL_MSG msg, unsigned long timeout){
	int err;

	/* send control headers (i.e: MAIL FROM:, RCPT TO: ...) */
	err = smtp_sendCtrlHeaders(sock, msg, timeout);

	/* if no error, then send data (actual message) */
	if(!err)	err = smtp_sendData(sock, msg, timeout);

	return err;
}

/* ====================================================== sendMail(...) */

/*	sendMail(...)
*		Sends message using given SMTP server.
*
*		- returns 0 if no error
*		- returns >0 on SMTP error
*		- returns <0 on socket error
*/
int sendMail(PMAIL_MSG msg, PSMTP_HOST host, unsigned long timeout){
	SOCKET sock;
	unsigned long hostAddr;
	int err;

	/* translate host name to address */
	hostAddr = sock_getHostByName(host->name, &err);
	if(!hostAddr) return err;

	/* create socket */
	sock = sock_createSocket(&err);
	if(sock == INVALID_SOCKET) return err;

	/* connect to SMTP server */
	err = smtp_connect(sock, hostAddr, host->port, timeout);

	if(!err){
		/* we are connected - seond message */
		err = smtp_sendMessage(sock, msg, timeout);
		if(!err){
			err = smtp_sendCommand(sock, "QUIT\r\n", timeout);
			if(err == 221) err = 0; /* not error */
		}
	}

	sock_close(sock);

	return err;
}

