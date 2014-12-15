/* 
*	Copyright (C) 2000  Goran Boban
*
*	last revision date: 04 Jun 2000 00:24:21
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
#include "smtp.h"

#include <stdio.h>
#include <stdlib.h>

/* ======================================================= CONSTANTS */

/* switch names */
char switchVerbose[]	= "-v";		/* verbose output */
char switchFile[] 	= "-file";	/* -file [input_file] */
char switchStdin[]	= "-i";		/* read input from stdin */
char switchHost[]		= "-h";		/* SMTP host name */
char switchPort[]		= "-p";		/* SMTP port number */
char switchTo[]		= "-to";		/* list of "To:" recipients */
char switchCc[]		= "-cc";		/* list of "Cc:" recipients */
char switchBcc[]		= "-bcc";	/* list of "Bcc:" recipients */
char switchFrom[]		= "-from";	/* -from [e-mail_address] */
char switchSubject[]	= "-s";		/* -s "subject of the message" */
char switchTimeout[]	= "-t";		/* timeout for socket operations */

char usage[]="\ngbmail \t[-v] -to <addres> [-file <filename>] [-i] [-h <smtp_server_name>]\n\t[-p <port_number>] [-from <addres>] [-cc <Cc:>] [-bcc <Bcc:>] \n\t[-s <Subject:>] [-t <timeout>]\n";

char argumentRequired[] = "GBMailer: switch requires argument: %s\n";
char missingSwitch[]		= "GBMailer: missing switch: %s\n";
char noMemory[]			= "GBMailer: no enough memory\n";

/* ========================================================= GLOBALS */

char from[SMTP_BUFLEN]="";			/* for storing senders address */
unsigned long timeout=300;		/* to hold timeout value (-t switch) */
char *fname = 0;					/* file name if -file switch is given */

MAIL_MSG msg = {			/* message description */
	0,				/* char* -> no sender (From:) */
	0,				/* char* -> no subject (Subject:) */
	0,				/* char** -> no recipients (To:) */
	0,				/* char** -> no Cc: */
	0,				/* char** -> no Bcc: */
	0,				/* FILE* no input file */
	isNOInput	/* where are we going to look for input? */
};

SMTP_HOST host = {	/* SMTP host description */
	"localhost", 		/* char* -> host name - assume localhost */
	25 					/* unsigned short -> SMTP port */
};


/* ======================================================= FUNCTIONS */

#ifdef _TGT_WIN32_
/*	cuserid(...)
*		Fills buf with login name of current user.
*
*		- if succesfull, returns buf
*		- returns 0 on failure
*
*		REMARKS:
*		For windows, we define this function, on other platforms
*		(Linux/UNIX) it should already exist.
*/
char* cuserid(char* buf){
	unsigned long buflen = 256;

	if(GetUserName(buf, &buflen)){
		return buf;
	}else return 0;
}
#endif

/*	testarg(...)
*		Itereates through command-line arguments to check for presence
*		of unknown switches
*
*		- if no unknown switches (i.e. no error) than returns 0
*		- if unknown switch is found, than returns 1
*/
int testargs(int argc, char* argv[]){
	int i;

	for(i=0;i<argc;++i){
		if(argv[i][0]=='-'){
			if(!strcmp(argv[i], switchVerbose)) continue;
			if(!strcmp(argv[i], switchFile)) continue;
			if(!strcmp(argv[i], switchStdin)) continue;
			if(!strcmp(argv[i], switchHost)) continue;
			if(!strcmp(argv[i], switchPort)) continue;
			if(!strcmp(argv[i], switchTo)) continue;
			if(!strcmp(argv[i], switchCc)) continue;
			if(!strcmp(argv[i], switchBcc)) continue;
			if(!strcmp(argv[i], switchFrom)) continue;
			if(!strcmp(argv[i], switchSubject)) continue;
			if(!strcmp(argv[i], switchTimeout)) continue;
			printf("GBMailer: unrecognized switch - %s\n", argv[i]);
			return 1;
		}
	}

	return 0;
}

/* findswitch(...)
*		Finds position of command-line switch s in argv[]
*
*		- if switch is found, then index of switch is returned
*		- if switch is not found, returns 0 (we don't expect switches at
*		argv[0])
*/
int findswitch(char *s, int argc, char* argv[]){
	int i;

	/* iterate through argument-list until we find
	*	switch or run out of arguments
	*/
	for(i=1;(i<argc)&&(strcmp(s, argv[i]));++i);

	if(i >= argc) return 0; /* switch not found */
	
	return i;	/* switch is at argv[i] */
}

/*	countSwitchArguments(...)
*		
*		- if switch is found, then count non-switch arguments after it
*		(i.e. all arguments before next switch or end of argv[] list)
*		- if no switch - return 0
*/
int countSwitchArguments(char* s, int argc, char* argv[]){
	int first;
	int last;

	first = findswitch(s, argc, argv);
	if(!first) return 0;

	for(last = first + 1; (last < argc) && (argv[last][0]!='-');++last);
	
	return (last - first - 1);
}

/* copyUpToNextSwitch(...)
*		Copies all elements of src[] starting from start to dest[]
*		(starting at 0), until it founds string beginning with "-"
*		(i.e. switch) or reaches end of src[]
*
*		- No return value
*/
void copyUpToNextSwitch(char* dest[], char* src[], int start, int length){
	int i;

	for(i = 0; i < length; ++i){
		dest[i] = src[start + i];
	}
}

/* processVerboseSwitch(...)
*		Checks if -v switch is present in argv[]
*
*		- if -v switch is found, than sets verbose
*		variable in msgs.c to nonzero
*		- if -v switch is not found - does nothing
*
*		Always returns 0 (i.e. function never fails)
*/
int processVerboseSwitch(int argc, char* argv[]){

	if(findswitch(switchVerbose, argc, argv)){
		sock_verbose=1;
	}

	return 0;
}

/* processStdinSwitch(...)
*		Checks if -i (read message from stdin) switch is
*		present in argv[]
*
*		- if -i switch is found, than sets inputSource
*		GLOBAL variable to isSTDIN
*		- if -i switch is not found - does nothing
*
*		Always returns 0 (i.e. function never fails)
*/
int processStdinSwitch(int argc, char* argv[]){

	if(findswitch(switchStdin, argc, argv)){
		msg.src = isFILE;
		msg.fp = stdin;
	}

	return 0;
}

/* processToSwitch(...)
*		Checks for presence of -to switch in command-line params
*		and initializes (char**) msg.to if one found
*
*		- if switch found, returns 0 (no error) and initializes
*		to member of msg GLOBAL variable (message descriptor)
*		- if no -to switch or no arguments to switch, returns
*		non-zero (error)
*/
int processToSwitch(int argc, char* argv[]){
	int i, count;

	/* switch exists? */
	if(!(i=findswitch(switchTo, argc, argv))){
		printf(missingSwitch, switchTo);
		return 1;
	}

	if(((i+1) >= argc) || (argv[i+1][0]=='-')){
		printf(argumentRequired, switchTo);
		return 1;
	}

	/* OK we have switch at i, now allocate and fill toList */
	count = countSwitchArguments(switchTo, argc, argv);

	msg.to = (char**)malloc((count+1) * sizeof(char*));
	if(!msg.to){
		printf(noMemory);
		return 1;
	}
	
	msg.to[count] = 0;
	copyUpToNextSwitch(msg.to, argv, i + 1, count);

	return 0;
}

/* processCcSwitch(...)
*		Checks for presence of -cc switch in command-line params
*		and initializes (char**) msg.cc if one found
*
*		- if switch found, returns 0 (no error) and initializes
*		cc member of msg GLOBAL variable (message descriptor)
*		- if no switch returns 0 - this is not an error
*		- if no arguments to switch, returns non-zero (error)
*/
int processCcSwitch(int argc, char* argv[]){
	int i, count;

	if((i=findswitch(switchCc, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchCc);
			return 1;
		}

		/* OK we have switch at i, now allocate and fill ccList */
		count = countSwitchArguments(switchCc, argc, argv);

		msg.cc = (char**)malloc((count+1) * sizeof(char*));
		if(!msg.cc){
			printf(noMemory);
			return 1;
		}
		
		msg.cc[count] = 0;
		copyUpToNextSwitch(msg.cc, argv, i + 1, count);
	}

	return 0;
}

/* processBccSwitch(...)
*		Checks for presence of -bcc switch in command-line params
*		and initializes (char**) msg.bcc if one found
*
*		- if switch found, returns 0 (no error) and initializes
*		bcc member of msg GLOBAL variable (message descriptor)
*		- if no switch returns 0 - this is not an error
*		- if no arguments to switch, returns non-zero (error)
*/
int processBccSwitch(int argc, char* argv[]){
	int i, count;

	if((i=findswitch(switchBcc, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchBcc);
			return 1;
		}

		/* OK we have switch at i, now allocate and fill bccList */
		count = countSwitchArguments(switchBcc, argc, argv);

		msg.bcc = (char**)malloc((count+1) * sizeof(char*));
		if(!msg.bcc){
			printf(noMemory);
			return 1;
		}
	
		msg.bcc[count] = 0;
		copyUpToNextSwitch(msg.bcc, argv, i + 1, count);
	}

	return 0;
}

/* processTimeoutSwitch(...)
*		Checks if -t switch is present in argv[]
*		and sets timeout, if yes
*
*		- if -t switch is found, than sets timeout
*		GLOBAL variable to value given on command-line
*		- if switch is found, but without proper argument,
*		returns non-zero (error)
*		- if -t switch is not found - does nothing
*		(this is not an error)
*/
int processTimeoutSwitch(int argc, char* argv[]){
	int i;

	if((i=findswitch(switchTimeout, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchTimeout);
			return 1;
		}

		timeout=atoi(argv[i+1]);
	}
	return 0;
}

/* processFileSwitch(...)
*		Checks if -file switch is present in argv[]
*		and sets fname and inputSource, if yes
*
*		- if -file switch is found, than sets fname
*		GLOBAL variable to value given on command-line
*		- if switch is found, but without proper argument,
*		returns non-zero (error)
*		- if -file switch is not found - does nothing
*		(this is not an error)
*/
int processFileSwitch(int argc, char* argv[]){
	int i;

	if((i=findswitch(switchFile, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchFile);
			return 1;
		}

		fname=argv[i+1];
		msg.src = isFILE;

		msg.fp=fopen(fname, "rb");
		if(!msg.fp){
			printf("GBMailer: failed to open file: %s\n", fname);
			return 1;
		}

	}
	return 0;
}

/* processFromSwitch(...)
*		Checks if -from switch is present in argv[]
*		and sets msg.from GLOBAL variable
*
*		- if -from switch is found, than sets from member
*		of msg GLOBAL variable to value given on command-line
*		- if switch is found, but without proper argument,
*		returns non-zero (error)
*		- if -from switch is not found - constructs from in form:
*		user_name@localhost_name
*/
int processFromSwitch(int argc, char* argv[]){
	char buf[512];
	int i;

	if((i=findswitch(switchFrom, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchFrom);
			return 1;
		}

		msg.from = argv[i+1];
	}else{
		sprintf(from, "%s@%s", cuserid(buf), sock_getHostName());
		msg.from = from;
	}
	return 0;
}

/* processHostSwitch(...)
*		Checks if -h switch is present in argv[]
*		and sets host.name GLOBAL variable
*
*		- if -h switch is found, than sets name member
*		of host GLOBAL variable to value given on command-line
*		- if switch is found, but without proper argument,
*		returns non-zero (error)
*		- if -h switch is not found - does nothing
*		(this is not an error)
*/
int processHostSwitch(int argc, char* argv[]){
	int i;

	if((i=findswitch(switchHost, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchHost);
			return 1;
		}

		host.name = argv[i+1];
	}
	return 0;
}

/* processPortSwitch(...)
*		Checks if -p switch is present in argv[]
*		and sets host.port GLOBAL variable
*
*		- if -p switch is found, than sets port member
*		of host GLOBAL variable to value given on command-line
*		- if switch is found, but without proper argument,
*		returns non-zero (error)
*		- if -p switch is not found - does nothing
*		(this is not an error)
*/
int processPortSwitch(int argc, char* argv[]){
	int i;

	if((i=findswitch(switchPort, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchPort);
			return 1;
		}

		host.port=atoi(argv[i+1]);
	}
	return 0;
}

/* processSubjectSwitch(...)
*		Checks if -s switch is present in argv[]
*		and sets msg.subject GLOBAL variable
*
*		- if -s switch is found, than sets subject member
*		of msg GLOBAL variable to value given on command-line
*		- if switch is found, but without proper argument,
*		returns non-zero (error)
*		- if -s switch is not found - does nothing
*		(this is not an error)
*/
int processSubjectSwitch(int argc, char* argv[]){
	int i;

	if((i=findswitch(switchSubject, argc, argv))){

		if(((i+1) >= argc) || (argv[i+1][0]=='-')){
			printf(argumentRequired, switchSubject);
			return 1;
		}

		msg.subject = argv[i+1];
	}
	return 0;
}

/* procargs(...)
*		Calls specific processing functions for
*		each switch
*
*		- if any of processXXXSwitch function fails
*		procarg returns 1
*		- if no error, returns 0
*/
int procargs(int argc, char* argv[]){

	/* any unknown switches? */
	if(testargs(argc, argv)) return 1;

	/* VERBOSE (-v) */
	if(processVerboseSwitch(argc, argv)) return 1;

	/* READ FROM STDIN (-i) */
	if(processStdinSwitch(argc, argv)) return 1;

	/* TO (-to) */
	if(processToSwitch(argc, argv)) return 1;

	/* TIMEOUT (-t) */
	if(processTimeoutSwitch(argc, argv)) return 1;

	/* FILE (-file) */
	if(processFileSwitch(argc, argv)) return 1;

	/* FROM (-from) */
	if(processFromSwitch(argc, argv)) return 1;

	/* HOST (-h) */
	if(processHostSwitch(argc, argv)) return 1;

	/* PORT (-p) */
	if(processPortSwitch(argc, argv)) return 1;

	/* CC (-cc) */
	if(processCcSwitch(argc, argv))	return 1;

	/* BCC (-bcc) */
	if(processBccSwitch(argc, argv)) return 1;

	/* SUBJECT (-s) */
	if(processSubjectSwitch(argc, argv)) return 1;

	return 0;
}

void showError(int errorCode){
	
	if(errorCode == -ETIMEDOUT) 								/* timeout */
		printf("GBMailer: connection timed out\n");	
	else if(errorCode > 0)		/* SMTP error */
		printf("GBMailer: SMTP error: %d\n", errorCode);
	else															/* socket error */
		printf("GBMailer: socket error: %d\n", -errorCode);
}

/* ============================================================ MAIN */
int main(int argc, char *argv[]){
	int err;		/* to store error codes */

	/* initialize sockets (Windoze WSAStartup()) in gbsock.c */
	err=sock_startup();
	if(err < 0){	/* couldn't initialize sockets */
		printf("GBMailer: Failed to initialize sockets (%d)\n", err);
		return err;
	}

	/* pass through command-line arguments and initialize
	* coresponding variables, show usage message if anything
	* wrong with arguments
	*/
	err = procargs(argc, argv);
	if(!err){

		/* send message */
		err = sendMail(&msg, &host, timeout);
		if(!err){	/* We did it :) */
			printf("GBMailer: Mail sent!\n");
		}else{		/* Failed */
			showError(err);
			printf("GBMailer: failed to send mail\n");
		}
	}else{	/* error in command-line arguments */
		printf(usage);
	}

	/* release resources */
	if(msg.fp) fclose(msg.fp);
	if(msg.to) free(msg.to);
	if(msg.cc) free(msg.cc);
	if(msg.bcc) free(msg.bcc);

	/* sockets cleanup (Windoze WSACleanup()) in gbsock.c */
	sock_cleanup();

	return err;
}

