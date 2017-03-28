/*******************************************************************************
 * Server for the GPS project.
 * Designed and programmed by: Robert Arendac
 * March 17 2017
*******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define BUFLEN 256           // Size of buffer to hold client messages
#define DEFAULT_PORT 25150   // Default port value

/* Function prototypes */
void runServer(int port);
struct sockaddr_in initAddress(int port);
void monitorSockets(fd_set *rset, fd_set *aset, int listenSock, int *clients, int maxfd);
void readSockets(int numClients, int *clients, fd_set *rset, fd_set *allset);
void closeSocket(int sck, fd_set *allset, int *clients, int index);
void writeData(const char *cIP, const char *cLat, const char *cLong, const char *cName, const char *cTime);
void setOpt(int socketfd);
int createSocket();
void bindSocket(int listenSock, struct sockaddr_in *addr);
void makeListen(int sockfd);
int acceptConnection(int listenSock, struct sockaddr_in *clientAddr);
int readMsg(int sockfd, char *msg);

/*******************************************************************************
 *  Function:       int main(int argc, char **argv);
 *                      int argc: number of command-line args
 *                      char **argv: Entered command-line args
 *
 *  Return:         exit status
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Main method for the server.  Gets the port as a command-line
 *      argument and then begins to run the server
 ******************************************************************************/
int main (int argc, char **argv) {
    int port;

    switch (argc) {
        case 1:
            port = DEFAULT_PORT;
            break;
        case 2:
            port = atoi(argv[1]);
            if (port < 20000) {
                fprintf(stderr, "Port must be above 20,000");
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "Error: %s usage [port]\n", argv[0]);
            exit(1);
    }

    runServer(port);

    return 0;
}

/*******************************************************************************
 *  Function:       void runServer(int port);
 *                      int port: port to listen to
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Will create a TCP socket to listen for incoming connections.  Sets the
 *      socket to reuse addresses, binds the socket, and specifies the socket
 *      to passively listen for connections. An array of clients is initialized
 *      and will be filled upon new connection.  Will then monitor sockets.
 ******************************************************************************/
void runServer(int port) {
    int listenSock;                 // Socket that will listen for connections
    int clients[FD_SETSIZE];        // Container of connected clients
    struct sockaddr_in addr;        // Struct to hold address information
    fd_set rset, aset;              // Two sets: All clients and ready clients

    // Create a socket to listen for connections
    listenSock = createSocket();

    // Set the socket options
    setOpt(listenSock);

    // Fill address info and bind the socket
    addr = initAddress(port);
    bindSocket(listenSock, &addr);

    // Set the socket to listen for connections
    makeListen(listenSock);

    // Initialize all clients to be invalid on start
    for (int i = 0; i < FD_SETSIZE; i++) {
        clients[i] = -1;
    }
    FD_ZERO(&aset);
    FD_SET(listenSock, &aset);

    // listenSock passed in twice; latter is for an initial value for the max fd value
    monitorSockets(&rset, &aset, listenSock, clients, listenSock);
}

/*******************************************************************************
 *  Function:       struct sockaddr_in initAddress(int port)
 *                      int port: port that the address will be bound to
 *
 *  Return:         An address struct that the server will listen on
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Fills an address struct.  IP is any, port is passed in, used for TCP.
 ******************************************************************************/
struct sockaddr_in initAddress(int port) {
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    return addr;
}

/*******************************************************************************
 *  Function:       void monitorSockets(fd_set *rset, fd_set *aset, int listenSock, int *clients, int maxfd)
 *                      fd_set *rset: set containing connected sockets
 *                      fd_set *aset: set containing all sockets
 *                      int listenSock: socket that listens for connections
 *                      int *clients: array of client socket descriptors
 *                      int maxfd: The largest socket descriptor value
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Runs in an infinite loop.  Waits for a client to indicate it's ready to
 *      do something and then will either accept a new connection or read data
 *      on a socket.
 ******************************************************************************/
void monitorSockets(fd_set *rset, fd_set *aset, int listenSock, int *clients, int maxfd) {
    int rcvSock;                        // Incoming connections saved to this socket
    int readySocks;                     // Number of sockets that are ready for connection/sending
    int ndx;                            // Index value for iterating through clients
    struct sockaddr_in clientAddr;      // Address of the connected client

    while (1) {
        // Determine how many sockets are ready for connection/sending
        *rset = *aset;
        readySocks = select(maxfd + 1, rset, NULL, NULL, NULL);

        // Client wants to connect
        if (FD_ISSET(listenSock, rset)) {
            if ((rcvSock = acceptConnection(listenSock, &clientAddr)) < 0)
                continue;

            printf("%s connected\n", inet_ntoa(clientAddr.sin_addr));

            // Save the new connection socket descriptor
            for (ndx = 0; ndx < FD_SETSIZE; ndx++) {
                if (clients[ndx] < 0) {
                    clients[ndx] = rcvSock;
                    break;
                }
            }

            // Check if max clients has been reached
            if (ndx == FD_SETSIZE) {
                fprintf(stderr, "Can't accept more clients");
                continue;
            }

            // Update sets and max file descriptor value
            FD_SET(rcvSock, aset);
            if (rcvSock > maxfd) {
                maxfd = rcvSock;
            }

            // No more sockets are ready
            if (--readySocks <= 0) {
                continue;
            }
        }
        // Begin reading the sockets
        readSockets(ndx, clients, rset, aset);
    }
}

/*******************************************************************************
 *  Function:       void readSockets(int numClients, int *clients, fd_set *rset, fd_set *aset)
 *                      int numClients: The current number of connected clients
 *                      int *clients: array of client socket descriptors
 *                      fd_set *rset: set containing connected sockets
 *                      fd_set *aset: set containing all sockets
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Goes through each connected client and will read on the clients socket
 *      if it is set.  If a message is received, the message will be written to
 *      a file.  Otherwise, the client socket is closed.
 ******************************************************************************/
void readSockets(int numClients, int *clients, fd_set *rset, fd_set *allset) {
    char msg[BUFLEN];   // Buffer to hold the message
    int sockfd;         // Client socket
    int bytesRead;      // Number of bytes read
    char *cTime;        // Client timestamp
    char *name;         // Client name
    char *ip;           // Client IP
    char *lat;          // Client latitude
    char *lng;          // Client longitude

    //loop through all possible clients
    for (int i = 0; i <= numClients; i++) {
        //check to see if current client exists
        if ((sockfd = clients[i]) < 0) {
            continue;
        }

        //check to see if current client is signaled
        if (FD_ISSET(sockfd, rset)) {
            memset(msg, 0, sizeof(msg));

            //read message
            if ((bytesRead = readMsg(sockfd, msg)) < 0)
                continue;

            //close request received
            if (bytesRead == 0) {
                closeSocket(sockfd, allset, clients, i);
                continue;
            }

            // Parse the received message and separate by spaces
            cTime = strtok(msg, " ");
            ip = strtok(NULL, " ");
            name = strtok(NULL, " ");
            lat = strtok(NULL, " ");
            lng = strtok(NULL, " ");

            // Write message to file
            writeData(ip, lat, lng, name, cTime);
        }
    }
}

/*******************************************************************************
 *  Function:       void closeSocket(int sck, fd_set *allset, int *clients, int index)
 *                      int sck: socket to be closed
 *                      fd_set *allset: set of all clients
 *                      int *clients: array of client descriptors
 *                      int index: index of clients array to be set to invalid
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Closes a socket and sets its descriptor to an invalid one in the clients
 *      array.
 ******************************************************************************/
void closeSocket(int sck, fd_set *allset, int *clients, int index) {
    printf("Client %d has disconnected\n", sck);

    if (close(sck) < 0) {
        fprintf(stderr, "close() failed: %s\n", strerror(errno));
    }

    FD_CLR(sck, allset);
    clients[index] = -1;
}

/*******************************************************************************
 *  Function:       void writeData(const char *msg);
 *                      const char *msg: Message to be written
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:       Mar 27 2017
 *                      - Switched to creating JSON data.
 *
 *  Desc:
 *      Checks for an existing file.  If it doesn't exist, will create a new JSON
 *      object.  If it does exist, will append to the existing JSON object.
 ******************************************************************************/
void writeData(const char *cIP, const char *cLat, const char *cLong, const char *cName, const char *cTime) {
    const char *file = "../webapp/gpsData.json";  //file to write to
    FILE *fp;
    char data[BUFLEN];

    if (access(file, F_OK) != -1) {
        // File exists
        fp = fopen(file, "r+b");
        sprintf(data, ",\n{\n\"ip\": \"%s\",\n\"lat\": %s,\n\"long\": %s,\n\"name\": \"%s\",\n\"time\": \"%s\"\n}\n]",
                cIP, cLat, cLong, cName, cTime);
        if ((fseek(fp, -1, SEEK_END)) < 0)  // Seek to replace the closing brace
            fprintf(stderr, "fseek failed :%s\n", strerror(errno));
    } else {
        fp = fopen(file, "wb");
        sprintf(data, "[\n{\n\"ip\": \"%s\",\n\"lat\": %s,\n\"long\": %s,\n\"name\": \"%s\",\n\"time\": \"%s\"\n}\n]",
                cIP, cLat, cLong, cName, cTime);
    }

    fprintf(fp, "%s", data);
    fclose(fp);
}

/* Just wrapper functions below this point */

/*******************************************************************************
 *  Function:       void setOpt(int socketfd);
 *                      int socketfd: Socket to set options on
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Wraps the setsockopt() call.  Will exit program if it fails.
 ******************************************************************************/
void setOpt(int socketfd) {
    int arg = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) < 0) {
        fprintf(stderr, "setsockopt() failed: %s\nClosing...", strerror(errno));
        exit(1);
    }
}

/*******************************************************************************
 *  Function:       int createSocket();
 *
 *  Returns:        Newly created socket
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Wraps the socket() creation call.  Will exit program if it fails.
 ******************************************************************************/
int createSocket() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket creation failed: %s\nClosing...", strerror(errno));
        exit(1);
    }

    return sockfd;
}

/*******************************************************************************
 *  Function:       void bindSocket(int listenSock, struct sockaddr_in *addr);
 *                      int listenSock: Listening socket that will be bound
 *                      struct sockaddr_in *addr: Address to bind to
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Wraps the bind() call.  Will exit program if it fails.
 ******************************************************************************/
void bindSocket(int listenSock, struct sockaddr_in *addr) {
    // Bind will actually return 0 (false) on success, who knows why...
    if (bind(listenSock, (struct sockaddr *)addr, sizeof(*addr))) {
        fprintf(stderr, "bind() failed: %s\nClosing...", strerror(errno));
        exit(1);
    }
}

/*******************************************************************************
 *  Function:       void makeListen(int sockfd);
 *                      int sockfd: Listening socket to call listen() on
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Wraps the listen() call.  Will exit program if it fails.
 ******************************************************************************/
void makeListen(int sockfd) {
    // Listen also returns 0 on success
    if (listen(sockfd, 5)) {
        fprintf(stderr, "listen() failed: %s\nClosing...", strerror(errno));
        exit(1);
    }
}

/*******************************************************************************
 *  Function:       int acceptConnection(int listenSock, struct sockaddr_in *clientAddr);
 *                      int listenSock: Listening socket descriptor
 *                      struct sockaddr_in *clientAddr: Address of incoming client
 *
 *  Returns:        Success or failure of the accept() call
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Wraps the accept() call.
 ******************************************************************************/
int acceptConnection(int listenSock, struct sockaddr_in *clientAddr) {
    int val;
    socklen_t addrSize = sizeof(struct sockaddr_in);

    if ((val = accept(listenSock, (struct sockaddr *)clientAddr, &addrSize)) < 0) {
        fprintf(stderr, "accept() failed: %s\n", strerror(errno));
    }

    return val;
}

/*******************************************************************************
 *  Function:       int readMsg(int sockfd, char *msg);
 *                      int sockfd: Socket to read on
 *                      char *msg: Buffer to hold read data
 *
 *  Returns:        Number of bytes read or a negative value indicating failure
 *
 *  Programmer:     Robert Arendac
 *
 *  Created:        Mar 17 2017
 *
 *  Modified:
 *
 *  Desc:
 *      Wraps the read() call.
 ******************************************************************************/
int readMsg(int sockfd, char *msg) {
    int bytesRead;

    if ((bytesRead = read(sockfd, msg, BUFLEN)) < 0) {
        fprintf(stderr, "read() failed: %s\n", strerror(errno));
    }

    return bytesRead;
}
