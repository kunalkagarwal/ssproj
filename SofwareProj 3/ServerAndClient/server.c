#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<fcntl.h>
#include<unistd.h>
#include<crypt.h>
#include<semaphore.h>
#include<netinet/ip.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/sem.h>
#include<sys/wait.h>
#include<errno.h>
#include<signal.h>


#define EMPPATH "../Data/employees.txt"
#define CUSPATH "../Data/customers.txt"
#define LOANPATH "../Data/loanDetails.txt"
#define COUNTERPATH "../Data/loanCounter.txt"
#define HISTORYPATH "../Data/trans_hist.txt"
#define FEEDPATH "../Data/feedback.txt"
#define HASHKEY "$6$saltsalt$"

#define MAINMENU "\n===== Choose Login Type =====\n1. Customer\n2. Employee\n3. Manager\n4. Administrator\n5. Exit\nSelect an option: "
#define ADMINMENU "\n===== Administrator Menu =====\n1. Add New Bank Staff\n2. Update Customer/Employee Information\n3. Manage User Permissions\n4. Logout\nSelect an option: "
#define CUSMENU "\n===== Customer Options =====\n1. Deposit Funds\n2. Withdraw Funds\n3. Check Balance\n4. Apply for a Loan\n5. Transfer Money\n6. Change Password\n7. Review Transactions\n8. Submit Feedback\n9. Logout\nSelect an option: "
#define EMPMENU "\n===== Employee Options =====\n1. Register New Customer\n2. Update Customer Information\n3. Approve or Deny Loan Requests\n4. View Assigned Loan Applications\n5. Review Customer Transactions\n6. Change Password\n7. Logout\n8. Exit\nSelect an option: "
#define MNGMENU "\n===== Manager Options =====\n1. Activate or Deactivate Customer Accounts\n2. Assign Loan Processing Tasks to Employees\n3. Evaluate Customer Feedback\n4. Change Password\n5. Logout\n6. Exit\nSelect an option: "

void employeeMenu(int connectionFD);
void managerMenu(int connectionFD);
void adminMenu(int connectionFD);
void connectionHandler(int connectionFileDescriptor);
void exitClient(int connectionFD, int id);
void cleanupSemaphore(int signum);
void setupSignalHandlers();
sem_t *initializeSemaphore(int accountNumber);

sem_t *sema;
char semName[50];

#include "../AllStructures/allStruct.h"
#include "../Modules/Customer.h"
#include "../Modules/Admin.h"
#include "../Modules/Employee.h"
#include "../Modules/Manager.h"

int main()
{
    int socketFileDescriptor, connectionFileDescriptor;
    int bindStatus;
    int listenStatus;
    int clientSize;

    struct sockaddr_in address, client;
    
    // Creating Socket
    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFileDescriptor == -1)
    {
        perror("Error Creating Socket");
        exit(-1);
    }
    printf("Server has been successfully created.\n");

    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    // Binding Socket
    bindStatus = bind(socketFileDescriptor, (struct sockaddr *)&address, sizeof(address));
    if (bindStatus == -1)
    {
        perror("Error Binding Socket");
        exit(-1);
    }
    printf("Socket binding completed successfully!\n");

    // Listening for connections
    listenStatus = listen(socketFileDescriptor, 2);
    if (listenStatus == -1)
    {
        perror("Error Listening for Connections");
        exit(-1);
    }
    printf("Server is actively listening for incoming connections!\n");

    while(1)
    {
        clientSize = sizeof(client);
        connectionFileDescriptor = accept(socketFileDescriptor, (struct sockaddr *) &client, &clientSize);
    
        if (connectionFileDescriptor == -1)
            perror("Error Accepting Connection");
        else
        {
            if(fork() == 0)
            {
                // Handling client connection
                connectionHandler(connectionFileDescriptor);
            }
        }
    }
    close(socketFileDescriptor);
    
    return 0;
}

// Handling client connection
void connectionHandler(int connectionFileDescriptor)
{
    char readBuffer[4096], writeBuffer[4096];
    int readBytes, writeBytes, choice;

    while(1)
    {
        writeBytes = write(connectionFileDescriptor, MAINMENU, sizeof(MAINMENU));
        if(writeBytes == -1)
        {
            printf("Data transmission failed\n");
        }
        else
        {   
            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connectionFileDescriptor, readBuffer, sizeof(readBuffer));
            if(readBytes == -1)
            {
                printf("Data reception failed from client\n");
            }
            else if(readBytes == 0)
            {
                printf("No data received from the client\n");
            }
            else
            {
                choice = atoi(readBuffer);
                printf("Client selected: %d\n", choice);
                switch (choice) 
                {
                    case 1:
                        // Customer
                        customerMenu(connectionFileDescriptor);
                        break;
                    
                    case 2:
                        // Employee
                        employeeMenu(connectionFileDescriptor);
                        break;
                            
                    case 3:
                        // Manager
                        managerMenu(connectionFileDescriptor);
                        break;

                    case 4:
                        // Admin
                        adminMenu(connectionFileDescriptor);
                        break;

                    case 5:
                        exitClient(connectionFileDescriptor, 0);
                        return;

                    default:
                        printf("Invalid selection! Please try again.\n");
                        break;
                }
            }
        }
    }
}

void exitClient(int connectionFileDescriptor, int id)
{
    snprintf(semName, 50, "/sem_%d", id);

    sem_t *sema = sem_open(semName, 0);
    if (sema != SEM_FAILED) {
        sem_post(sema);
        sem_close(sema); 
        sem_unlink(semName);    
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Client is now logging out...\n");
    write(connectionFileDescriptor, writeBuffer, sizeof(writeBuffer));
    read(connectionFileDescriptor, readBuffer, sizeof(readBuffer));
}

// =================== Session Handling =================
void cleanupSemaphore(int signum) {
    if (sema != NULL) {
        sem_post(sema);
        sem_close(sema);
        sem_unlink(semName); 
    }
    printf("Program terminated. Semaphore has been cleaned up.\n");
    _exit(signum);
}

sem_t *initializeSemaphore(int id) {
    snprintf(semName, 50, "/sem_%d", id);
    return sem_open(semName, O_CREAT, 0644, 1);  // Initialize to 1
}

void setupSignalHandlers() {
    signal(SIGINT, cleanupSemaphore); 
    signal(SIGTERM, cleanupSemaphore); 
    signal(SIGSEGV, cleanupSemaphore);
    signal(SIGHUP, cleanupSemaphore);  
    signal(SIGQUIT, cleanupSemaphore);
}
