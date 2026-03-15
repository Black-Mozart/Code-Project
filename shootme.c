/* 
   Standard I/O library
   Provides functions like printf(), perror(), fopen(), fread()
*/
#include <stdio.h>

/*
   Standard utility library
   Provides exit(), memory utilities, etc.
*/
#include <stdlib.h>

/*
   String handling functions
   strlen(), strcmp(), strcpy(), sscanf(), etc.
*/
#include <string.h>

/*
   Core socket networking functions
   socket(), bind(), listen(), accept(), recv(), send()
*/
#include <sys/socket.h>

/*
   Structures used for internet addresses (sockaddr_in)
*/
#include <netinet/in.h>

/*
   Functions for working with IP addresses
*/
#include <arpa/inet.h>

/*
   System calls like close()
*/
#include <unistd.h>


/* Port number the HTTP server will listen on */
#define PORT 8080

/* Buffer size used for reading requests and file chunks */
#define BUFFER_SIZE 4096


/* --------------------------------------------------
   Function: send_404
   Sends a simple HTTP 404 error response to the client
-------------------------------------------------- */
void send_404(int client_socket)
{
    /* HTTP response message for a missing file */
    char response[] =
        "HTTP/1.1 404 Not Found\r\n"     // HTTP status line
        "Content-Type: text/plain\r\n"   // Response header specifying text
        "\r\n"                           // Blank line separates headers from body
        "404 File Not Found";            // Body of the response

    /* Send the response to the client through the socket */
    send(client_socket, response, strlen(response), 0);
}


/* --------------------------------------------------
   Function: serve_file
   Opens a requested file and sends it to the client
-------------------------------------------------- */
void serve_file(int client_socket, char *filename)
{
    /* Attempt to open the file in binary mode */
    FILE *file = fopen(filename, "rb");

    /* If the file does not exist */
    if (!file)
    {
        /* Send 404 error response */
        send_404(client_socket);
        return;
    }

    /* HTTP response header for successful request */
    char header[] =
        "HTTP/1.1 200 OK\r\n"                    // Status line indicating success
        "Content-Type: application/octet-stream\r\n"  // Generic binary file type
        "\r\n";                                  // End of headers

    /* Send HTTP header first */
    send(client_socket, header, strlen(header), 0);

    /* Buffer used to read file chunks */
    char buffer[BUFFER_SIZE];

    /* Number of bytes read from file */
    size_t bytes;

    /* Read file in chunks and send them to client */
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        /* Send chunk of file data to the client */
        send(client_socket, buffer, bytes, 0);
    }

    /* Close the file after sending it */
    fclose(file);
}


int main()
{
    /* Server socket descriptor */
    int server_socket;

    /* Client socket descriptor */
    int client_socket;

    /* Structure storing server address information */
    struct sockaddr_in server_addr;


    /* --------------------------------------------------
       Create server socket
       --------------------------------------------------
       AF_INET     → IPv4 addressing
       SOCK_STREAM → TCP connection
       0           → default protocol
    */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    /* Check if socket creation failed */
    if (server_socket < 0)
    {
        perror("socket failed");
        exit(1);
    }


    /* --------------------------------------------------
       Configure server address
    -------------------------------------------------- */

    /* Specify IPv4 addressing */
    server_addr.sin_family = AF_INET;

    /* Convert port number to network byte order */
    server_addr.sin_port = htons(PORT);

    /* Accept connections from any network interface */
    server_addr.sin_addr.s_addr = INADDR_ANY;


    /* --------------------------------------------------
       Bind socket to port
       This tells the OS our program will use this port
    -------------------------------------------------- */
    if (bind(server_socket,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(1);
    }


    /* --------------------------------------------------
       Put server into listening mode
       Backlog = max number of pending connections
    -------------------------------------------------- */
    if (listen(server_socket, 10) < 0)
    {
        perror("listen failed");
        exit(1);
    }

    /* Inform user that server started */
    printf("HTTP server running on port %d\n", PORT);


    /* --------------------------------------------------
       Main server loop
       The server will run forever
    -------------------------------------------------- */
    while (1)
    {
        /* Wait for a client to connect */
        client_socket = accept(server_socket, NULL, NULL);

        /* Check if accept failed */
        if (client_socket < 0)
        {
            perror("accept failed");
            continue;
        }


        /* Buffer to store HTTP request from client */
        char request[BUFFER_SIZE];

        /* Read request from socket */
        int bytes = recv(client_socket, request, BUFFER_SIZE - 1, 0);

        /* If nothing received, close connection */
        if (bytes <= 0)
        {
            close(client_socket);
            continue;
        }

        /* Null terminate request string */
        request[bytes] = '\0';

        /* Print request for debugging */
        printf("Request:\n%s\n", request);


        /* Variables for storing parsed request data */
        char method[16];
        char path[256];

        /* Extract method and path from request line */
        sscanf(request, "%s %s", method, path);


        /* If client requests root directory */
        if (strcmp(path, "/") == 0)
        {
            /* Serve index.html by default */
            strcpy(path, "/index.html");
        }


        /* Build file path relative to current directory */
        char filename[256];

        snprintf(filename, sizeof(filename), ".%s", path);


        /* Send requested file to client */
        serve_file(client_socket, filename);


        /* Close connection after response */
        close(client_socket);
    }


    /* Close server socket (never reached in infinite loop) */
    close(server_socket);

    return 0;
}
