#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<termios.h>

void connectionHandler(int socketFileDescriptor);
void hide_input(char *buffer, int size);

void main()
{
    int socketFileDescriptor;
    int connectStatus;

    struct sockaddr_in address;
    
    // Creating client-side socket
    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFileDescriptor == -1)
    {
        perror("Socket creation failed\n");
        exit(-1);
    }
    printf("Client-side socket successfully initialized!\n");

    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    // Establishing connection to the server
    connectStatus = connect(socketFileDescriptor, (struct sockaddr *)&address, sizeof(address));
    if(connectStatus == -1)
    {
        perror("Failed to connect\n");
        exit(-1);
    }

    // Passing control to handler function to manage server interaction
    connectionHandler(socketFileDescriptor);
    exit(0);
}

void connectionHandler(int socketFileDescriptor)
{
    char readBuffer[4096], writeBuffer[4096], tempBuffer[4096];
    int readBytes, writeBytes;

    do
    {
        // Clearing buffers
        bzero(readBuffer, sizeof(readBuffer));
        
        // Reading server's response
        readBytes = read(socketFileDescriptor, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Error reading from server\n");
        }
        else if(readBytes == 0)
        {
            printf("Connection closed by server\n");
        }
        else
        {
            if (strcmp(readBuffer, "Enter password: ") == 0)
            {
                hide_input(writeBuffer, sizeof(writeBuffer));
            }            
            else
            {
                bzero(writeBuffer, sizeof(writeBuffer));
                bzero(tempBuffer, sizeof(tempBuffer));
                
                if(strcmp(readBuffer, "Client logging out...\n") == 0)
                {
                    strcpy(writeBuffer, "");
                    close(socketFileDescriptor);
                    return;
                }
                else if(strchr(readBuffer, '^') != NULL)
                {
                    if(strlen(readBuffer) > 1)
                    {
                        strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 1);
                        printf("%s\n", tempBuffer);
                    }
                    strcpy(writeBuffer, "");
                }
                else
                {
                    printf("%s\n", readBuffer);
                    scanf("%s", writeBuffer);
                }                
            }
            
            // Sending response back to server
            writeBytes = write(socketFileDescriptor, writeBuffer, sizeof(writeBuffer));
            if(writeBytes == -1)
            {
                printf("Error sending data to server\n");
                printf("Closing connection\n");
                break;
            }
        }
    } while(readBytes > 0);

    close(socketFileDescriptor);
}

// Function to hide password input
void hide_input(char *buffer, int size) {
    struct termios oldt, newt;

    // Getting terminal settings and disabling echo
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Input prompt for password
    printf("Enter password: ");
    scanf("%s", buffer);

    // Restoring terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}
