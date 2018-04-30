#!/usr/bin/env python3

##############################################################
## Author: James Meehan
## Date: 3/6/2018
## Description: This is the client side of a file transfer
## system.  The client connects to a server using their
## hostname and port number and then either requests a list 
## of the files in the server's current directory or requests
## a specific file to be transferred over a specified dataport.
###############################################################

# https://docs.python.org/2/library/socket.html
# https://docs.python.org/3.5/library/struct.html
# http://stackoverflow.com/questions/17667903/python-socket-receive-large-amount-of-data

import socket
import sys
from os import path
from struct import *
from time import sleep

###########################################################
## Description: initiageContact() takes a hostname and port
## number as parameters and connects to the server listening
## at the hostname on the port.
###########################################################

def initiateContact(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((host, port))
    except:
        print("Error: Failed to connect to with %s on port %d" % (host, port))
        exit(1);
    return s

#############################################################
## Description: getDirectory() takes a socket as a parameter
## and receives the list of directory contents from the 
## server.
#############################################################
def getDirectory(sock):
    data_size = sock.recv(4)
    data_size = unpack("I", data_size)
    received = str(sock.recv(data_size[0]), encoding="UTF-8").split("\x00")

    for val in received:
        print(val)

#############################################################
## Description: sendMsg() takes a socket and a message as
## parameters and sends the size of the message, followed by 
## the message itself, to the server.
#############################################################
def sendMsg(sock, message):
    to_send = bytes(message, encoding="UTF-8")
    sock.sendall(to_send)

#############################################################
## Description: sendNum() takes a socket and a number as
## parameters and sends the number over the socket to the 
## server.  Primarily used to send dataport numbers or the size
## of messages to the server.
#############################################################
def sendNum(sock, num):
    to_send = pack('i', num)
    sock.send(to_send)

#############################################################
## Description: receiveMsg() takes a socket as a parameter
## and receives the size of the incoming message over the
## socket.  The message itself is then received and returned
#############################################################
def receiveMsg(sock):
    try:
        data_size = sock.recv(4)
        data_size = unpack("I", data_size)
        return receiveFullMsg(sock, data_size[0])
    except:
        print ("An unknown error occurred while receiving the message from the Server")
        controlConn.close()
        data.close()
        exit(1)

#################################################################
## Description: receiveFullMsg() is a helper function for 
## receiveMsg().  It allows large amounts of data to be 
## received. It takes a socket and the amount of bytes in the 
## message as paramters and continues to receive data until all
## of the bytes have been received  This function code was borrowed from 
## http://stackoverflow.com/questions/17667903/python-socket-receive-large-amount-of-data
##################################################################
def receiveFullMsg(sock, b):
    received = ""
    while len(received) < b:
        packet = str(sock.recv(b - len(received)), encoding="UTF-8")
        if not packet:
            return None
        received += packet
    return received

################################################################
## Description: makeRequest() takes a socket, client
## command, and dataport as parameters.  The command
## is sent over the dataport to the server
################################################################
def makeRequest(sock, command, dataport):
    try:
        sendMsg(sock, command + "\0")
        sendNum(sock, dataport)
    except:
        print ("An unknown error occurred while sending command request to server")
        controlConn.close()
        data.close()
        exit(1)

################################################################
## Description: receiveFile() takes a socket and filename
## as parameters and receives the file from the 
## server. If the file already exists in the directory
## then a copy is made
##############################################################
def receiveFile(sock, filename):
    buffer = receiveMsg(sock)
    if path.isfile(filename):
        # prompt user if file should be overwritten
        overwrite = input("File already exists. Would you like to overwrite? (y/n)")
        if overwrite != 'y':
            filename = filename.split(".")[0] + "_copy.txt"
            print("File was saved as {}".format(filename))
        
        else:
            print ("File {} was overwritten".format(filename))
    
    ffile = open(filename, 'w')
    ffile.write(buffer)

## main function ##
def main():    
    argLen = len(sys.argv)
    if argLen < 5 or argLen > 6:
        print("Usage: python3 ftclient.py [server_host] [server_port] [command] [filename] [data_port]\n [filename] is only required with the -g command\n")
        exit(1)

    host = sys.argv[1]
    port = int(sys.argv[2])
    command = sys.argv[3]
    data_port = 0
    filename = ""

    if len(sys.argv) is 5:
        data_port = int(sys.argv[4])
        if command == "-g":
            print("Error: Check usage.  <filename> parameter required with -g command")
            exit(1)

    elif len(sys.argv) is 6:
        filename = sys.argv[4]
        data_port = int(sys.argv[5])

    if command not in ["-g", "-l"]:
        print ("Error: You typed an invalid command.  Acceptable commands are -l or -g")
        exit(1)

    if port < 1024 or port > 65535 or data_port < 1024 or data_port > 65535:
        print ("Error: Invalid port. Acceptable range: 1024 - 65535. Please try again")
        exit(1)

    # Start server socket and make initial request
    controlConn = initiateContact(host, port)
    makeRequest(controlConn, command, data_port)

    # request directory structure from server
    if command == "-l":
        sleep(1)
        data = initiateContact(host, data_port)
        print("Receiving directory structure from {}: {}".format(host, data_port))
        getDirectory(data)
        # client closes control connection
        controlConn.close()

    # request file from server
    if command == "-g":
        sendNum(controlConn, len(filename))
        sendMsg(controlConn, filename + "\0")
        data = initiateContact(host, data_port)
        result = receiveMsg(controlConn)
        
        if result == "Error: The requested file was not found in the directory":
            print("{}:{} says {}".format(host, port, result))
            controlConn.close()
        elif result == "File found":
            print("Receiving \"{}\" from {}: {}".format(filename, host, data_port))
            sleep(1)
            receiveFile(data, filename)
            print("File transfer complete.")
            # client closes control connection
            controlConn.close()

    # Server should close the data connection but just in case
    data.close()

#ensure that the main function is run first
if __name__ == '__main__':
    main()
