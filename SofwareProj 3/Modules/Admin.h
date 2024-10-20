#define ADMINNAME "admin"
#define PASSWORD "admin"

int addEmployee(int connectionFD);
void modifyCE(int connectionFD, int modifyChoice);
void manageRole(int connectionFD);

char readBuffer[4096], writeBuffer[4096];

void adminMenu(int connectionFD)
{
    char password[20];
retry:
    memset(writeBuffer, 0, sizeof(writeBuffer));
    strcat(writeBuffer, "Enter password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);

    if (strcmp(PASSWORD, password) == 0)
    {
        memset(writeBuffer, 0, sizeof(writeBuffer));
        memset(readBuffer, 0, sizeof(readBuffer));
        strcpy(writeBuffer, "\nLogin Successfully^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
    }
    else
    {
        memset(writeBuffer, 0, sizeof(writeBuffer));
        memset(readBuffer, 0, sizeof(readBuffer));
        strcpy(writeBuffer, "\nInvalid credential^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        goto retry;
    }

    while (1)
    {
        int modifyChoice;
        int choice;

        memset(writeBuffer, 0, sizeof(writeBuffer));
        strcpy(writeBuffer, ADMINMENU);
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        memset(readBuffer, 0, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        choice = atoi(readBuffer);
        printf("Admin selected: %d\n", choice);

        switch (choice)
        {
            case 1:
                if (addEmployee(connectionFD))
                {
                    memset(writeBuffer, 0, sizeof(writeBuffer));
                    memset(readBuffer, 0, sizeof(readBuffer));
                    strcpy(writeBuffer, "Employee successfully added\n^");
                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                    read(connectionFD, readBuffer, sizeof(readBuffer));
                }
                break;
            case 2:
                memset(writeBuffer, 0, sizeof(writeBuffer));
                strcpy(writeBuffer, "Enter 1 to Modify Customer\nEnter 2 to Modify Employee: ");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));

                memset(readBuffer, 0, sizeof(readBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
                modifyChoice = atoi(readBuffer);

                modifyCE(connectionFD, modifyChoice);
                break;
            case 3:
                manageRole(connectionFD);
                break;
            case 4:
                return;
            default:
                memset(writeBuffer, 0, sizeof(writeBuffer));
                memset(readBuffer, 0, sizeof(readBuffer));
                strcpy(writeBuffer, "Invalid choice! Please try again.^");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
        }
    }
}

// ================ Add New Bank Employee ================
int addEmployee(int connectionFD)
{
    struct Employee emp;

    memset(writeBuffer, 0, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Employee ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    emp.empID = atoi(readBuffer);
    printf("Admin input empID: %d\n", emp.empID);

    memset(writeBuffer, 0, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter FirstName: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(emp.firstName, readBuffer);
    printf("Admin input firstName: %s\n", emp.firstName);

    memset(writeBuffer, 0, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter LastName: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(emp.lastName, readBuffer);
    printf("Admin input lastName: %s\n", emp.lastName);

    memset(writeBuffer, 0, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(emp.password, crypt(readBuffer, HASHKEY));

    emp.role = 1;

    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if (file == -1)
    {
        printf("Error accessing file!\n");
        return 0;
    }

    lseek(file, 0, SEEK_END);
    if (write(file, &emp, sizeof(emp)) == -1)
    {
        printf("Error adding employee!\n");
        return 0;
    }

    close(file);
    return 1;
}

// ===================== Modify Customer / Employee ==================
void modifyCE(int connectionFD, int modifyChoice)
{
    if (modifyChoice == 1)
    {
        printf("Admin chose to modify customer\n");
        int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
        if (file == -1)
        {
            printf("Error accessing file!\n");
            return;
        }

        memset(writeBuffer, 0, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter Account Number: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        int accNo;
        memset(readBuffer, 0, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        accNo = atoi(readBuffer);
        printf("Admin input account number: %d\n", accNo);

        struct Customer c;
        lseek(file, 0, SEEK_SET);

        int srcOffset = -1, sourceFound = 0;
        while (read(file, &c, sizeof(c)) != 0)
        {
            if (c.accountNumber == accNo)
            {
                srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
                sourceFound = 1;
                break;
            }
        }

        struct flock fl = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
        fcntl(file, F_SETLKW, &fl);

        if (sourceFound)
        {
            char newName[20];
            memset(writeBuffer, 0, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newName, readBuffer);
            printf("Admin input new Name: %s\n", newName);

            strcpy(c.customerName, newName);
            write(file, &c, sizeof(c));

            memset(writeBuffer, 0, sizeof(writeBuffer));
            strcpy(writeBuffer, "^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
        else
        {
            memset(writeBuffer, 0, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid Account Number\n^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }

        fl.l_type = F_UNLCK;
        fcntl(file, F_UNLCK, &fl);
        close(file);
    }
    else if (modifyChoice == 2)
    {
        printf("Admin chose to modify employee\n");
        struct Employee emp;
        int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
        if (file == -1)
        {
            printf("Error accessing file!\n");
            return;
        }

        memset(writeBuffer, 0, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter Employee ID: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        int id;
        memset(readBuffer, 0, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        id = atoi(readBuffer);
        printf("Admin input Employee ID: %d\n", id);

        lseek(file, 0, SEEK_SET);

        int srcOffset = -1, sourceFound = 0;
        while (read(file, &emp, sizeof(emp)) != 0)
        {
            if (emp.empID == id)
            {
                srcOffset = lseek(file, -sizeof(struct Employee), SEEK_CUR);
                sourceFound = 1;
                break;
            }
        }

        struct flock fl = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Employee), getpid()};
        fcntl(file, F_SETLKW, &fl);

        if (sourceFound)
        {
            char newName[20];
            memset(writeBuffer, 0, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newName, readBuffer);

            strcpy(emp.firstName, newName);
            printf("Admin updated employee name to: %s\n", newName);
            write(file, &emp, sizeof(emp));
        }
        else
        {
            memset(writeBuffer, 0, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid Employee ID^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }

        fl.l_type = F_UNLCK;
        fcntl(file, F_UNLCK, &fl);
        close(file);
    }
}

// ===================== Manage Role ==================
void manageRole(int connectionFD)
{
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if (file == -1)
    {
        printf("Error accessing file!\n");
        return;
    }

    memset(writeBuffer, 0, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    int id;
    memset(readBuffer, 0, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    id = atoi(readBuffer);

    struct Employee emp;
    while (read(file, &emp, sizeof(emp)) != 0)
    {
        if (emp.empID == id)
        {
            int choice;
            memset(writeBuffer, 0, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter 1 to make manager\nEnter 2 to make employee: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            lseek(file, -sizeof(struct Employee), SEEK_CUR);

            if (choice == 1)
            {
                printf("Admin made %d manager\n", id);
                emp.role = 0;
                write(file, &emp, sizeof(emp));
            }
            else if (choice == 2)
            {
                printf("Admin made %d employee\n", id);
                emp.role = 1;
                write(file, &emp, sizeof(emp));
            }
            close(file);

            memset(writeBuffer, 0, sizeof(writeBuffer));
            strcpy(writeBuffer, "^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            memset(readBuffer, 0, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            return;
        }
    }

    memset(writeBuffer, 0, sizeof(writeBuffer));
    strcpy(writeBuffer, "Invalid ID^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    return;
}
