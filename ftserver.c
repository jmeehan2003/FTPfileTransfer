/*********************************************************************************
** Author: James Meehan
** Course: CS 372
** Date: 3/7/2018
** Description: This is the file server for a file transfer system.  The 
** server listens on a port for a client connection and then receives a command
** from the client.  The server opens a data connection with the client and 
** can send back a list of files in the directory or an entire file from the 
** directory.  The server closes the data socket after completing the client request
** and remains listening on the port for more client connections.
*********************************************************************************/

/*********************************************************************************
 * The following source were used in creating this server program
 * Everything sockets: Beej's guide: http://beej.us/guide/bgnet/
 * Directory help:  https://www.thegeekstuff.com/2012/06/c-directory
 * More Directory help: http://pubs.opengroup.org/onlinepubs/009695399/functions/getcwd.html
 * Sockets: https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/keyword.htm
 * File Transfers in C: https://forgetcode.com/c/1201-file-transfer-using-tcp
 * Large File Transfers in C: https://stackoverflow.com/questions/8679547/send-large-files-over-socket-in-c
********************************************************************************/


#include <stdio.h>
#include "svrfunctions.h"
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <limits.h>


int main(int argc, char *argv[]){
    //start the signal handler
    signal(SIGCHLD, sigint);    
    int sockfd, port, pid, status;
 
    // validate command prompt usage and verify port is in acceptable range
    checkUsage(argc);
    port = verifyPort(argv[1]);

    // create socket
    sockfd = startUp(port);
    if (sockfd > 0) 
	printf("Server listening on port %d\n\n", port);

    // always listen for connections until terminated by supervisor
    while(1){
	// accept client connnection
        int control_socket = accept(sockfd, NULL, NULL);
	 if(control_socket < 0){
            printf("Error: Failed accepting connection\n");
            close(control_socket);
        }
	// if connection succesful, create threads
        else {
            pid = fork();

            // fork process failed
            if(pid < 0){
                printf("Error: Forking process failed\n");
                exit(1);
            }

            // child process
            else if(pid == 0){
		close(sockfd);
                printf("A client has started a control connection on port %d\n", port);
                char message[BUFFER];

		// get request from client (-l or -g)
                int command = handleRequest(control_socket);
      
		// get dataport number from clienti
                int dataport = receiveNum(control_socket);	
      
                // create a socket for the data connection
                int client_datafd = startUp(dataport);
                
                // accept the connection
                int data_socket = accept(client_datafd, NULL, NULL);
		if (data_socket < 0) 
			printf("Error: Data connection failed");
		else
                	printf("Data connnection with client opened on port %d\n", dataport);
                
		// access current directory (modified from www.thegeekstuff.com/2012/06/c-directory   
		DIR *dp = NULL;
		char buf[PATH_MAX] = "";
		char *dir = getcwd(buf, BUFFER);
                dp = opendir(dir);  	
		struct dirent *dptr = NULL;					
		
                // -l command request (getcommand returns "1" if user sends -l command)
                if(command == 1){
                        char* directory[PATH_MAX];
			int dirSize, numFiles, i;
			dirSize = numFiles = i = 0;
                	printf("List directory requested on port %d\n", dataport);
			
			// place all directory files into char array directory
		        while((dptr = readdir(dp)) != NULL){	
                           if(dptr->d_type == DT_REG){
                                directory[i] = dptr->d_name;
				dirSize += strlen(directory[i]);
                                i++;
                          }
                       }
		       dirSize += (i - 1);
		       closedir(dp);	
                  
			// send the file list
			printf("Sending directory contents to client on port %d\n", dataport);
                        sendNum(data_socket, dirSize);
                        i = 0;
                        while (directory[i] != NULL) {
                 	      	sendMsg(data_socket, directory[i]);
				i++;	
			}
	
                        close(data_socket); 
			printf("Client data connection has been closed\n\n");
                        _exit(0);
	       }
               // -g command requested (getCommand returns "2" if user sends -g command)
               else if (command == 2){
                        bool fileFound;
                        int size = receiveNum(control_socket);
			char requestedFile[255] = "\0";
                        receiveMsg(control_socket, requestedFile, size);  
                        printf("File [%s] requested on port %d\n", requestedFile, dataport);
			
			// check if file exists in the directory
                        while((dptr = readdir(dp)) != NULL){
                            if(strncmp(requestedFile, dptr->d_name, max(strlen(dptr->d_name), strlen(requestedFile))) == 0)
                                fileFound = true;
                        }

                        // file exists
                        if(fileFound){
                            printf("[%s] was found in the directory\n", requestedFile);
                            sendResponse(message, "File found", control_socket);  

                            printf("Sending [%s] to client on port %d\n", requestedFile, dataport);
                            sendFile(requestedFile, data_socket);
			    
                            close(data_socket); 
			    printf("Client data connection has been closed\n\n");
                            _exit(0);

                        // file does not exist
                        } else {
                            printf("[%s] is an invalid filename. Sending error message to client\n", requestedFile);
                            sendResponse(message, "Error: The requested file was not found in the directory", control_socket);
                            close(data_socket); 
			    printf("Client data connection has been closed \n\n");
                            _exit(0);
                        }
               }      
               // a command other than -l or -g was entered. Should not happen because this is validated by the client but just in case
               else {
                        sendResponse(message, "Invalid command entered: Please use either -l to list directory files or -g to get a file", control_socket);
			printf("Invalid command received.  Client data connection has been closed\n");
                        close(data_socket); 
                        _exit(0);
              }
	// client should close the control connection but just in case
	close(control_socket);
        }
    }
    } 
    return 0;

}
