GBMailer
Supported platforms: Win95/98/2000/NT4.0, Linux-i386
September 2000 version 2.02

Author:
Goran Boban
gboban70@gmail.com
Home page: https://github.com/gboban/GBMailer

License: GPL (please read LICENSE.TXT)


	1. Description
	--------------

GBMailer is command-line mailer wich will send an text file as mail.
It is written to be executed from CGI-scripts, but it can be used for 
any other purpose.

See history.txt for changes in this version.

NOTE:	Be sure to read Bugs section (section 2.) of this readme file
	before mailing bug reports or questions about using GBMailer

USAGE:

gbmail [-v] -to <address list...> [-i] [-file <filename>] [-h <smtp.server.name>]
	[-p <port_number>] [-from <address>] [-cc <address list...>]
	[-bcc <address list>] [-s <Subject>] [-t <timeout>]


OPTIONS:

	-v	verbose output.

	-file	Name of text file to send as message body.
		(This is not required since version 1.12)

	-i	Get input from stdin (i.e: redirection and pipes).

	-h	Name of SMTP server to connect. If this option is not set
		GBMailer will assume 'localhost'.

	-p	Port number for SMTP protocol. Port is set to 25 by default.

	-from	An optional e-mail ddress of sender. If not set GBMailer
		will try to assume an e-mail address by combinating name of
		current user and name of local machine.
		You should set this option in most cases.

	-to	REQUIRED	E-mail addres (or addresses) of recipient(s).
				At least one address must cast after -to switch.

	-cc	(carbon copy) E-mail addresses of recipients which will be listed
		in Cc: field.

	-bcc	(blind carbon copy) E-mail addresses of recipients which you don't
		want to be listed in message which will be sent.

	-s	Subject of message.

	-t	Timeout for socket operations (in seconds). Set to 300 secs by
		default.

RETURN VALUE:
	There are three groups of errors eturned by GBMailer, which calling
	application can examine to learn details about failure:

	- error code <0 is negative value of error returned by sockets
	subsystem (WSAGetLastError() on Win or errno/h_errno on Linux).
	Note: error codes returned on Windows systems and linux systems
	will be different. Windows socket errors start at 10000 (WSABASEER)
	while Linux error codes are in range 1-123.

	- error code = 1 inicates missing command-line switch, missing
	switch parameter or missing file given after -file switch.

	- error code in range 200-600 is value of SMTP response (see RFC821).
	Note that even 2NN and 3NN may be returned, even if response codes
	in this range are generally not errors. This is because GBMailer
	will signal error on every unexpected rsponse to SMTP commands.

	Finally, if return value is 0, this means no error (i.e. mail sent).


EXAMPLE:

gbmail -file message.txt -h smtp.server.com -from me@my.machine.com -to you@your.machine.com -s "Hello there"

or if you want to use redirection

gbmail -i -h smtp.server.com -from me@my.machine.com -to you@your.machine.com -s "Hello there" < message.txt


	2. Bugs
	-------

	Please send bug reports and sugestions to:

	gboban70@gmail.com

	IMPORTANT NOTE: When sending bug reports or questions on using GBMailer please
	be sure to include following information:

		- Program name (GBMailer in this case)
		- Program version (2.02)
		- OS you are using
		- Version of OS you are using (you can include build number and service
		pack number)
		- How did you invoke gbmail.exe (from command-line or from script, if
		from script please send me name of interpreter and version)
		- Problem you want to report

	You may include any other information you find relevant (or you think it may be
	relevant) for this problem.
	Also be sure to use correct e-mail address (in From: or Reply to: field) so I
	can send you answer.


