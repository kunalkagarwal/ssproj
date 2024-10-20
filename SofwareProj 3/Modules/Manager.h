void managerMenu(int connectionFD);
int loginManager(int connectionFD, int mngID, char *password);
void changeStatus(int connectionFD);
void assignLoanApplication(int connectionFD);
void readFeedBack(int connectionFD);
int changeMNGPassword(int connectionFD, int mngID);

char readBuffer[4096], writeBuffer[4096];   

void managerMenu(int connectionFD)
{
    struct Employee manager;
    int mngID, response = 0;
    char password[20], newPassword[20];
    int choice;

menu_loop:
    memset(writeBuffer, 0, sizeof(writeBuffer));
    snprintf(writeBuffer, sizeof(writeBuffer), "\nEnter Manager ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    mngID = atoi(readBuffer);

    memset(writeBuffer, 0, sizeof(writeBuffer));
    snprintf(writeBuffer, sizeof(writeBuffer), "Enter password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strncpy(password, readBuffer, sizeof(password) - 1);

    if (loginManager(connectionFD, mngID, password))
    {
        memset(writeBuffer, 0, sizeof(writeBuffer));
        printf("Manager ID: %d logged in successfully\n", mngID);
        snprintf(writeBuffer, sizeof(writeBuffer), "Login Successful\n^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        while (1)
        {
            memset(writeBuffer, 0, sizeof(writeBuffer));
            snprintf(writeBuffer, sizeof(writeBuffer), MNGMENU);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            printf("Manager selected option: %d\n", choice);
            switch (choice)
            {
                case 1:
                    changeStatus(connectionFD);  // Activate/Deactivate Accounts
                    break;
                case 2:
                    assignLoanApplication(connectionFD);  // Assign Loan Applications
                    break;
                case 3:
                    readFeedBack(connectionFD);  // Review Customer Feedback
                    break;
                case 4:
                    response = changeMNGPassword(connectionFD, mngID);
                    if (!response)
                    {
                        memset(writeBuffer, 0, sizeof(writeBuffer));
                        snprintf(writeBuffer, sizeof(writeBuffer), "Password change failed\n^");
                        write(connectionFD, writeBuffer, sizeof(writeBuffer));
                    }
                    else
                    {
                        memset(writeBuffer, 0, sizeof(writeBuffer));
                        snprintf(writeBuffer, sizeof(writeBuffer), "Password changed. Please log in with the new password.\n^");
                        write(connectionFD, writeBuffer, sizeof(writeBuffer));
                    }
                    logout(connectionFD, mngID);
                    goto menu_loop;
                case 5:
                    printf("Manager logged out.\n");
                    logout(connectionFD, mngID);
                    return;
                case 6:
                    printf("Manager exited the session.\n");
                    exitClient(connectionFD, mngID);
                    return;
                default:
                    printf("Invalid choice, please try again.\n");
            }
        }
    }
    else
    {
        memset(writeBuffer, 0, sizeof(writeBuffer));
        snprintf(writeBuffer, sizeof(writeBuffer), "\nInvalid Manager ID or Password^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        goto menu_loop;
    }
}

int loginManager(int connectionFD, int mngID, char *password)
{
    struct Employee manager;
    int file = open(EMPPATH, O_RDWR | O_CREAT, 0644);
    if (file == -1)
    {
        printf("Error: Unable to open file!\n");
        return 0;
    }

    sem_t *sema = initializeSemaphore(mngID);
    setupSignalHandlers();

    if (sem_trywait(sema) == -1)
    {
        if (errno == EAGAIN)
        {
            printf("Manager %d is already logged in!\n", mngID);
        }
        else
        {
            perror("Error locking semaphore");
        }
        close(file);
        return 0;
    }

    lseek(file, 0, SEEK_SET);
    while (read(file, &manager, sizeof(manager)) != 0)
    {
        if (manager.empID == mngID && strcmp(manager.password, crypt(password, HASHKEY)) == 0 && manager.role == 0)
        {
            close(file);
            return 1;
        }
    }

    close(file);
    return 0;
}

void changeStatus(int connectionFD)
{
    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    struct Customer customer;
    int accNo, choice;

    memset(writeBuffer, 0, sizeof(writeBuffer));
    snprintf(writeBuffer, sizeof(writeBuffer), "Enter Account Number: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    accNo = atoi(readBuffer);

    while (read(file, &customer, sizeof(customer)) != 0)
    {
        if (customer.accountNumber == accNo)
        {
            memset(writeBuffer, 0, sizeof(writeBuffer));
            snprintf(writeBuffer, sizeof(writeBuffer), "Enter 1 to Deactivate, 2 to Activate: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            lseek(file, -sizeof(struct Customer), SEEK_CUR);
            customer.activeStatus = (choice == 1) ? 0 : 1;
            write(file, &customer, sizeof(customer));

            printf("Status changed for account number: %d\n", accNo);
            memset(writeBuffer, 0, sizeof(writeBuffer));
            snprintf(writeBuffer, sizeof(writeBuffer), "Status Changed Successfully^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            close(file);
            return;
        }
    }

    close(file);
    printf("Invalid account number: %d\n", accNo);
    memset(writeBuffer, 0, sizeof(writeBuffer));
    snprintf(writeBuffer, sizeof(writeBuffer), "Invalid account number^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
}

void readFeedBack(int connectionFD)
{
    struct FeedBack feedback;
    int file = open(FEEDPATH, O_RDONLY);
    char tempBuffer[4096] = {0};

    if (file == -1)
    {
        printf("Error: Unable to open feedback file!\n");
        return;
    }

    while (read(file, &feedback, sizeof(feedback)) != 0)
    {
        strcat(tempBuffer, feedback.feedback);
        strcat(tempBuffer, "\n");
    }
    strcat(tempBuffer, "^");

    printf("Manager reviewing feedback.\n");
    memset(writeBuffer, 0, sizeof(writeBuffer));
    snprintf(writeBuffer, sizeof(writeBuffer), "%s", tempBuffer);
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    close(file);
}

void assignLoanApplication(int connectionFD)
{
    struct LoanDetails loanDetails;
    int file = open(LOANPATH, O_CREAT | O_RDWR, 0644);

    if (file == -1)
    {
        printf("Error: Unable to open loan file!\n");
        return;
    }

    while (read(file, &loanDetails, sizeof(loanDetails)) != 0)
    {
        if (loanDetails.empID == -1)
        {
            memset(writeBuffer, 0, sizeof(writeBuffer));
            snprintf(writeBuffer, sizeof(writeBuffer), "Loan ID: %d\nAccount No: %d\nAmount: %d^", loanDetails.loanID, loanDetails.accountNumber, loanDetails.loanAmount);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
        }
    }

    int loanID, empID;
    lseek(file, 0, SEEK_SET);
    printf("Assigning loan process.\n");

    memset(writeBuffer, 0, sizeof(writeBuffer));
    snprintf(writeBuffer, sizeof(writeBuffer), "Enter Loan ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    loanID = atoi(readBuffer);

    while (read(file, &loanDetails, sizeof(loanDetails)) != 0)
    {
        if (loanDetails.loanID == loanID && loanDetails.empID == -1)
        {
            lseek(file, -sizeof(struct LoanDetails), SEEK_CUR);
            break;
        }
    }

    memset(writeBuffer, 0, sizeof(writeBuffer));
    snprintf(writeBuffer, sizeof(writeBuffer), "Enter Employee ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    empID = atoi(readBuffer);

    loanDetails.empID = empID;
    loanDetails.status = 1;
    write(file, &loanDetails, sizeof(loanDetails));

    printf("Assigned employee %d to loan ID %d\n", empID, loanID);
    close(file);
}

int changeMNGPassword(int connectionFD, int mngID)
{
    char newPassword[20];
    struct Employee manager;
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    
    if (file == -1)
    {
        printf("Error: Unable to open employee file!\n");
        return 0;
    }

    while (read(file, &manager, sizeof(manager)) != 0)
    {
        if (manager.empID == mngID && manager.role == 0)
        {
            memset(writeBuffer, 0, sizeof(writeBuffer));
            snprintf(writeBuffer, sizeof(writeBuffer), "Enter New Password: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strncpy(newPassword, readBuffer, sizeof(newPassword) - 1);

            strncpy(manager.password, crypt(newPassword, HASHKEY), sizeof(manager.password) - 1);
            lseek(file, -sizeof(struct Employee), SEEK_CUR);
            write(file, &manager, sizeof(manager));
            close(file);
            return 1;
        }
    }

    close(file);
    return 0;
}
