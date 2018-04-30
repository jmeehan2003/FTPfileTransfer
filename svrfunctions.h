/***************************************************************************
** Author: James Meehan
** Date: March 6, 2018
** Description: This is the header file for the ftsever program.  It contains
** the function details for all functions developed for the program
****************************************************************************/

/**************************************************************************
 * See ftserver.c or the README file for details on all of the sources used to
 * create this server program.  Functions that relied heavily on borrowed
 * code will have the sources explicitly listed in the function descriptions
 * below.
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

// buffer size for messages
#define BUFFER 1024

void checkUsage(int);
int verifyPort(char*);
int startUp(int);
int handleRequest(int);
int receiveNum(int);
void receiveMsg(int, char*, size_t);
void sendNum(int, int);
int sendMsg(int, char *msg);
void sendResponse(char*, char*, int);
char* readFile(char*);
void sendFile(char*, int);
void sigintHandle(int sigNum);
int max(int, int);


/***************************************************************************
** Description: checkUsage() validates the command line 
** parameters.  Proper user is ftserver [server_port].
****************************************************************************/
void checkUsage(int argc){
    if (argc != 2) {
	fprintf(stderr, "Usage: ftserver [server_port]\n");
 	exit(1);
    }
}

/*****************************************************************************
** Description: verifyPort() takes the command line argument specified as the 
** port number and verifies it is in an acceptable range.
*****************************************************************************/
int verifyPort(char *portno){
	int port = atoi(portno);
	if(port < 1024 || port > 65535){
		fprintf(stderr, "Invalid port number used. Acceptable Range: 1024-65535\n");
		exit(1);
	}
	return port;
}


/*****************************************************************************
** Description: startUp() takes an integer port number as a parameter.  A 
** socket is then opened, socket strucutre filled in, and bound to the port
** number.  Finally, the socket listens on the port.  Error messages are
** sent to the user if any of these processes fail and -1 is returned.  
** Otherwise, an integer is returned representing the successful creation
** of a new socket.  Beej's guide used extensively for this: 
** http://beej.us/guide/bgnet/html/multi/index.html
******************************************************************************/
int startUp(int port){
	// open socket
	int new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket < 0){
		fprintf(stderr, "Error: Failed to open socket on port %d\n", port);
		return -1;
	}
	
	// fill in socket structure
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;

    int optval = 1;
    setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

	// bind socket to port
	if(bind(new_socket, (struct sockaddr *) &server, sizeof(server)) < 0){
		fprintf(stderr, "Error: Failed to bind socket on port %d\n", port);
		return -1;
	}

	// have socket listen for connections
	if(listen(new_socket, 5) < 0){
		fprintf(stderr, "Error: Failed to listen on port %d\n", port);
		return -1;
	}
	
	// socket successfully created and listening
	return new_socket;
} 

/*****************************************************************************
** Description: handleRequest() gets the command from the client returns 1 if
** the command is -l (list directory files), 2 if the command is -g (get file), 
** or -1 if an invalid command has been entered.
******************************************************************************/
int handleRequest(int csock){
    char buffer[3] = "\0";

    // get command from client and return corresponding integer
    receiveMsg(csock, buffer, 3);        
    
    if(strcmp(buffer, "-l") == 0) 
        return 1;
    else if(strcmp(buffer, "-g") == 0)
        return 2;
    else 
        return -1;
}

/**************************************************************************
** Description: receiveNum() takes a socket as a parameter and receives
** an integer over the socket, which is returned.
**************************************************************************/
int receiveNum(int sock){
	int num;
	ssize_t a = 0;
	a = read(sock, &num, sizeof(int));
	if(a < 0){
		printf("Error: Unable to receive number through socket\n");
		return -1;
	}
	else 
		return num;
}

/**************************************************************************
** Description: receiveMsg() takes a socket, message, and the size of the
** message as parameters. It reads until the entire message has been
** received and then copies the data to the output string.
**************************************************************************/
void receiveMsg(int sock, char *msg, size_t size){
	char buffer[size + 1];
	ssize_t a;
	size_t total = 0;

	while(total < size){
		a = read(sock, buffer + total, size - total);
		total += a;

		if(a < 0){
			fprintf(stderr, "Error: Failed to receive message from client");
		}
	}

	strncpy(msg, buffer, size);
}

/**************************************************************************
** Description: sendNum() takes a socket and an interger as parameters and 
** sends the integer over the socket.  Primarily used to alert the client
** of the size of the message about to be sent.
**************************************************************************/
void sendNum(int sock, int num){
	ssize_t a = 0;
	a = write(sock, &num, sizeof(int));
	if(a < 0){
		printf("Error: Unable to send number through socket\n");
	}
}

/**************************************************************************
** Description: sendMsg() takes a socket and a message as parameters.  It
** sends the message over the socket to the client until the entire message
** has been transferred.  A successful transfer returns 0 and -1 is returned
** otherwise.
**************************************************************************/
int sendMsg(int sockfd, char *msg){
	ssize_t a;
	size_t size = strlen(msg) + 1;
	
	// total bytes sent
	size_t total = 0;

	while(total < size) {
		a = write(sockfd, msg, size - total);

		// add bytes sent to total
		total += a;
	
		if(a < 0){
			fprintf(stderr, "Error: Message failed to send\n");
			return -1;
		}
		else if(a == 0){
			total = size - total;
		}
	}
	return 0;
}

/**************************************************************************
** Description: readFile() takes a file name as a parameter.  The file is
** opened and the contents stored in a char array and returned.
**************************************************************************/
char* readFile(char *fileName){
   FILE* fptr = fopen(fileName, "r");
   char* data = NULL;   
 
   if(fptr != NULL) {
	if (fseek(fptr, 0L, SEEK_END) == 0) {
		long buffer = ftell(fptr);
		if (buffer == -1){ 
			printf("Error: File is invalid\n");
			exit(1);
		}

		data = malloc(sizeof(char) * (buffer + 1));

		// go back to beginning of file to read it	
		if (fseek(fptr, 0L, SEEK_SET) != 0) 
			printf("Error: Unable to read file\n");

		size_t len = fread(data, sizeof(char), buffer, fptr);
		if (ferror(fptr) != 0 ) {
			fputs("Error reading file\n", stderr);
		}
		else 
			data[len++] = '\0';
	}
   }		  
   else 
	printf("Error: Unable to open file\n");
  
   fclose(fptr);
   return data;
}

/**************************************************************************
** Description: sendFile() takes a file name and socket as parameters.  The
** file is read into a char array.  The size of the file and the file 
** contents are then sent to the client.
**************************************************************************/
void sendFile(char* file, int sock){
    // read file
    char* sendf = readFile(file);

    // send file size and then file to client	
    sendNum(sock, (strlen(sendf)));      
    sendMsg(sock, sendf);                    
}

/**************************************************************************
** Description: sendResponse() takes a message buffer, message, and control socket
** as parameters. The message is copied into the buffer.  Then the size of the
** message is sent, followed by the actual message.
**************************************************************************/
void sendResponse(char *buff, char* message, int client_sock){
    // copy the message into the message buffer
    strncpy(buff, message, BUFFER);	

    // send the size of the message followed by the message		
    sendNum(client_sock, (strlen(buff)));	
    sendMsg(client_sock, buff);		
}

/*****************************************************************************
** Description: sigintHandle() catches exiting children.
******************************************************************************/
void sigint(int sigNum){
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG);
}

/*****************************************************************************
** Description: max() is a helper function for the strncmp function used when
** checking to see if the file exists in the directory.  It takes two integers
** and returns the larger of the two (or a if both are equal).
******************************************************************************/
int max(int a, int b){
    if(a >= b){
        return a;
    }
    else return b;
}


