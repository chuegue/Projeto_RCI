#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void missing_arguments();

int main(void)
{
    char string[100] = "exit";
    char *token = strtok(string, " ");

    // join net id
    if (strcmp(token, "join") == 0)
    {
        token = strtok(NULL, " ");
        for (int k = 0; k < 2; k++)
        {
            if (token == NULL) // certifica que tem o numero de argumentos necessários
            {
                missing_arguments();
                exit(1);
            }
            printf("Ciclo join: %s\n", token);
            token = strtok(NULL, " ");
        }
    }

    // djoin net id bootid bootIP bootTCP
    if (strcmp(token, "djoin") == 0)
    {
        token = strtok(NULL, " ");
        for (int k = 0; k < 5; k++)
        {
            if (token == NULL) // certifica que tem o numero de argumentos necessários
            {
                missing_arguments();
                exit(1);
            }
            printf("Ciclo djoin: %s\n", token);
            token = strtok(NULL, " ");
        }
    }

    // create name
    if (strcmp(token, "create") == 0)
    {
        token = strtok(NULL, " ");
        if (token == NULL)
        {
            missing_arguments();
            exit(1);
        }
        else if (strcmp(token, "name") == 0)
        {
            printf("You are in create name! \n");
        }
    }

    // delete name
    if (strcmp(token, "delete") == 0)
    {
        token = strtok(NULL, " ");
        if (token == NULL)
        {
            missing_arguments();
            exit(1);
        }
        else if (strcmp(token, "name") == 0)
        {
            printf("You are in delete name! \n");
        }
    }

    // get dest name
    if (strcmp(token, "get") == 0)
    {
        token = strtok(NULL, " ");
        for (int k = 0; k < 2; k++)
        {
            if (token == NULL) // certifica que tem o numero de argumentos necessários
            {
                missing_arguments();
                exit(1);
            }
            printf("Ciclo join: %s\n", token);
            token = strtok(NULL, " ");
        }
    }

    // show topology || show names || show routing
    if (strcmp(token, "show") == 0)
    {
        token = strtok(NULL, " ");
        if (token == NULL)
        {
            missing_arguments();
            exit(1);
        }
        else if (strcmp(token, "topology") == 0)
        {
            printf("You are in show topology! \n");
        }
        else if (strcmp(token, "names") == 0)
        {
            printf("You are in show names! \n");
        }
        else if (strcmp(token, "routing") == 0)
        {
            printf("You are in show routing! \n");
        }
        else
        {
            printf("Not valid! \n");
        }
    }

    if (strcmp(token, "leave") == 0)
    {
        printf("Bye bye borboletinha! \n");
    }

    if (strcmp(token, "exit") == 0)
    {
        exit(0);
    }
    return 0;
}

void missing_arguments()
{
    printf(" Faltam argumentos! \n");
}
