James Meehan
CS 372
Project 2 README INSTRUCTIONS

****************************************************************
NOTES:
Some resources that assisted in me in completing this project are:
C Server:
Beej's Guide: http://beej.us/guide/bgnet
Directory Help: https://www.thegeekstuff.com/2012/06/c-directory
Directory Help: http://pubs.opengroup.org/onlinepubs/009695399/functions/getcwd.html
IBM Socket Knowledge Center: 
https://www.ibm.com/supports/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/keyword.htm
File Transfers: https://forgetcode.com/c/1201-file-transfer-using-tcp
File Transfers: https://stackoverflow.com/questions/8679547/send-large-files-over-socket-in-c

Python Chat Client
Python docs (socket): https://docs.python.org/2/library.socket.html
Python docs (struct): https://docs.python.org/3/struct.html
File Transfer: http://stackoverflow.com/questions/17667903/python-socket-receive-large-amount-of-data

These are also noted in the relevant files.  Code from program 1 was also partially
used to create this file transfer program.
****************************************************************************************

USAGE
-- How to Run the Server
- Make sure svrfunctions.h is in the same directory as ftserver.c
- Compile ftserver.c
  gcc ftserver.c -o ftserver

-- Execute ftserver
   ftserver <server_port>

-- Take note of which server chat server is running on

- How to run the client (NOTE: python3 is required)  
python3 ftclient.py <server-hostname> <server_port> <-l or -g> <filename (optional)> <dataport>

NOTE: <server-hostname> is the server that ftserver is running on
NOTE: filename is only used with the -g command
NOTE: Be sure that the server port number passed to ftclient.py is the same as the port
number passed to ftserver


STEP BY STEP INSTRUCTIONS:
1) Make sure svrfunctions.h is in the sam directory as ftserver.c
2) Compile ftserver
gcc ftserver.c -o ftserver

3) Run ftserver.  Type in command line:
./ftserver <server_port>

4) Run ftclient.py. Type in command line:
python3 ftclient.py <server_hostname> <server_port> <command> <filename> <dataport>

*filename is only used with the -g command.

5) If the -g command is used and the file already exists in the client directory,
the client will be prompted if the file should be overwritten or a copy made. 

6) The client program will terminate automatically when the commands are completed or
an error is received.  The server will continue to run and accept connections until 
a supervisor presses CONTROL-C.


TESTING:
This was tested with ftserver and ftclient running on various
different flip servers and they were successfully able to connect, exchange messages/files,
and quit. The server remained waiting for new connections after the client connection was closed
and another client could connect to the server.