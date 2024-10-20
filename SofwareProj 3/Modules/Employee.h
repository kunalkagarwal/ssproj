void employeeMenu(int connectionFD);
int loginEmployee(int connectionFD, int empID, char *password);
void addCustomer(int connectionFD);
void approveRejectLoan(int connectionFD, int empID);
void viewAssignedLoan(int connectionFD, int empID);
int changeEMPPassword(int connectionFD, int empID);

void employeeMenu(int connectionFD)
{
    struct Employee employee;
    int empID;
    int accountNumber, response = 0;
    char password[20];
    char newPassword[20];
    int choice;

label1:
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "\nPlease enter your Employee ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    empID = atoi(readBuffer);
    printf("Employee has entered ID: %d\n", empID);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Please input your password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);

    if(loginEmployee(connectionFD, empID, password))
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        printf("Employee %d has logged in successfully.\n", empID);
        strcpy(writeBuffer, "\nLogin Successful!\n^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));

        while(1)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, EMPMENU);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            printf("Employee selected option: %d\n", choice);
            switch(choice)
            {
                case 1:
                    // Add New Customer
                    addCustomer(connectionFD);
                    break;
                case 2:
                    // Modify Customer Details
                    modifyCE(connectionFD, 1);
                    break;
                case 3:
                    // Approve/Reject Loans
                    approveRejectLoan(connectionFD, empID);
                    break;
                case 4:
                    // View Assigned Loan Applications
                    viewAssignedLoan(connectionFD, empID);
                    break;
                case 5:
                    // View Customer Transactions
                    bzero(writeBuffer, sizeof(writeBuffer));
                    strcpy(writeBuffer, "Please provide Account Number: ");
                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                    
                    bzero(readBuffer, sizeof(readBuffer));
                    read(connectionFD, readBuffer, sizeof(readBuffer));
                    accountNumber = atoi(readBuffer);

                    transactionHistory(connectionFD, accountNumber);
                    break;
                case 6:
                    // Change Password
                    response = changeEMPPassword(connectionFD, empID);
                    if(!response)
                    {
                        bzero(writeBuffer, sizeof(writeBuffer));
                        strcpy(writeBuffer, "Unable to change the password\n^");
                        write(connectionFD, writeBuffer, sizeof(writeBuffer));
                        read(connectionFD, readBuffer, sizeof(readBuffer));
                    }   
                    else
                    {
                        bzero(writeBuffer, sizeof(writeBuffer));
                        bzero(readBuffer, sizeof(readBuffer));
                        strcpy(writeBuffer, "Password updated successfully! Please log in with the new password...\n^");
                        write(connectionFD, writeBuffer, sizeof(writeBuffer));
                        read(connectionFD, readBuffer, sizeof(readBuffer));
                    }   
                    logout(connectionFD, empID);                                             
                    goto label1;
                case 7:
                    // Logout
                    printf("Employee ID: %d has logged out!\n", empID);
                    logout(connectionFD, empID);
                    return;
                case 8:
                    // Exit
                    printf("Employee ID: %d has exited the system!\n", empID);
                    exitClient(connectionFD, empID);
                default:
                    printf("Invalid option selected\n");                
            }
        }
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(writeBuffer, "\nIncorrect Employee ID or Password\n^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        goto label1;
    }
}

// ================= Employee Login ==================
int loginEmployee(int connectionFD, int empID, char *password)
{
    struct Employee employee;
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);

    sema = initializeSemaphore(empID);

    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN) {
            printf("Employee %d is already logged in!\n", empID);
        } else {
            perror("Semaphore wait failed");
        }
        close(file);
        return 0;
    }

    lseek(file, 0, SEEK_CUR);
    while(read(file, &employee, sizeof(employee)) != 0)
    {
        if (employee.empID == empID && strcmp(employee.password, crypt(password, HASHKEY)) == 0 && employee.role == 1) {
            close(file);
            return 1;
        }
    }
    snprintf(semName, 50, "/sem_%d", empID);

    sem_t *sema = sem_open(semName, 0);
    if (sema != SEM_FAILED) {
        sem_post(sema);
        sem_close(sema); 
        sem_unlink(semName);    
    }

    close(file);
    return 0;   
}

// ================= Add New Customer ==================
void addCustomer(int connectionFD) {
    struct Customer customer;
    char name[20], password[20], transactionBuffer[1024];

    struct trans_histroy th;

    time_t s, val = 1;
    struct tm* current_time;
    s = time(NULL);
    current_time = localtime(&s);

    int file = open(CUSPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);

    if (file == -1) {
        printf("Error while opening customer file!\n");
        return;
    }

    // Customer Name
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Please enter Customer Name: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(customer.customerName, readBuffer);

    // Customer Password
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Please enter Customer Password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(customer.password, crypt(readBuffer, HASHKEY));

    // Customer Account Number
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Please enter Account Number: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    customer.accountNumber = atoi(readBuffer);

    // Customer Initial Balance
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Please enter Opening Balance: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    customer.balance = atof(readBuffer);

    bzero(transactionBuffer, sizeof(transactionBuffer));
    sprintf(transactionBuffer, "%.2f Opening Balance recorded at %02d:%02d:%02d on %d-%d-%d\n", customer.balance, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year) + 1900, (current_time->tm_mon) + 1, current_time->tm_mday);
    
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = customer.accountNumber;
    write(fp, &th, sizeof(th));

    close(fp);

    customer.activeStatus = 1;

    write(file, &customer, sizeof(customer));
    close(file);

    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));
    printf("Employee added a customer with account number: %d\n", customer.accountNumber);
    strcpy(writeBuffer, "Customer added successfully!\n^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ================= Approve/Reject Loan =================
void approveRejectLoan(int connectionFD, int empID)
{
    char transactionBuffer[1024];
    struct LoanDetails ld;
    
    struct trans_histroy th;
    time_t s, val = 1;
    struct tm* current_time;
    s = time(NULL);
    current_time = localtime(&s);

    int lID;
    struct Customer cs;

    int file = open(LOANPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND);
    lseek(file, 0, SEEK_SET);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Please provide Loan ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    lID = atoi(readBuffer);

    while(read(file, &ld, sizeof(ld)))
    {
        if(ld.loanID == lID)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Loan Amount: ");
            sprintf(writeBuffer + strlen(writeBuffer), "%d\n", ld.loanAmount);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Loan Type: ");
            sprintf(writeBuffer + strlen(writeBuffer), "%d\n", ld.loanType);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Loan Status: ");
            sprintf(writeBuffer + strlen(writeBuffer), "%s\n", ld.status);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Is this loan approved? (1 for Yes / 0 for No): ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            if(atoi(readBuffer))
            {
                strcpy(ld.status, "Approved");
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, "Loan has been approved!\n^");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));

                // Update transaction history
                bzero(transactionBuffer, sizeof(transactionBuffer));
                sprintf(transactionBuffer, "Loan ID: %d has been approved by Employee ID: %d at %02d:%02d:%02d on %d-%d-%d\n", ld.loanID, empID, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year) + 1900, (current_time->tm_mon) + 1, current_time->tm_mday);
                
                bzero(th.hist, sizeof(th.hist));
                strcpy(th.hist, transactionBuffer);
                th.acc_no = ld.accountNumber;
                write(fp, &th, sizeof(th));
            }
            else
            {
                strcpy(ld.status, "Rejected");
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, "Loan has been rejected!\n^");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));

                // Update transaction history
                bzero(transactionBuffer, sizeof(transactionBuffer));
                sprintf(transactionBuffer, "Loan ID: %d has been rejected by Employee ID: %d at %02d:%02d:%02d on %d-%d-%d\n", ld.loanID, empID, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year) + 1900, (current_time->tm_mon) + 1, current_time->tm_mday);
                
                bzero(th.hist, sizeof(th.hist));
                strcpy(th.hist, transactionBuffer);
                th.acc_no = ld.accountNumber;
                write(fp, &th, sizeof(th));
            }

            // Write updated loan details back to file
            lseek(file, -sizeof(ld), SEEK_CUR);
            write(file, &ld, sizeof(ld));

            close(file);
            close(fp);
            return;
        }
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Invalid Loan ID provided.\n^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
}

// ================= View Assigned Loan =================
void viewAssignedLoan(int connectionFD, int empID);
{
    struct LoanDetails ld;
    int file = open(LOANPATH, O_RDONLY);
    if(file == -1)
    {
        printf("Error in opening file\n");
        return;
    }

    while(read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.empID == empID && ld.status == 1)
        {
            bzero(readBuffer, sizeof(readBuffer));
            bzero(writeBuffer, sizeof(writeBuffer));
            sprintf(writeBuffer, "Loan ID: %d\nAccount Number: %d\nLoan Amount: %d^", ld.loanID, ld.accountNumber, ld.loanAmount);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
    }
    close(file);
    
    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ================= Change Password =================
int changeEMPPassword(int connectionFD, int empID);
{
    char newPassword[20];

    struct Employee emp;
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    
    lseek(file, 0, SEEK_SET);

    int srcOffset = -1, sourceFound = 0;

    while (read(file, &emp, sizeof(emp)) != 0)
    {
        if(emp.empID == empID)
        {
            srcOffset = lseek(file, -sizeof(struct Employee), SEEK_CUR);
            sourceFound = 1;
        }
        if(sourceFound)
            break;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Employee), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(newPassword, readBuffer);

    strcpy(emp.password, crypt(newPassword, HASHKEY));
    write(file, &emp, sizeof(emp));

    fl1.l_type = F_UNLCK;
    fl1.l_whence = SEEK_SET;
    fl1.l_start = srcOffset;
    fl1.l_len = sizeof(struct Employee);
    fl1.l_pid = getpid();

    fcntl(file, F_UNLCK, &fl1);
    close(file);

    printf("Employee %d changed password\n", empID);
    return 1;
}