 //server

#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>  
#include <string.h>
#include <stdlib.h>
#include "funksjoner.c"

char settJobbtype(char jobbtype, char jobbChar);
char* lagReturStreng(FILE* f);

int usage(int argc, char* argv[]) {
    if(argc < 3) {
        printf("USAGE: %s [file] [port]\n", argv[0]);
        return 1;
    }
    return 0;
}

int get_port(char* port_as_string, unsigned short* port) {
    char* endptr;

    int ret = strtol(port_as_string, &endptr, 10);
    if(endptr == port_as_string && ret == 0) {
        printf("[port]-argument has to be an integer!\n");
        return 1;
    }
    *port = (unsigned short) ret;
    return 0;
}

int main(int argc, char *argv[])
{
    if(usage(argc, argv)) 
    {
        return EXIT_SUCCESS;
    }

    FILE* f; 
    f = fopen(argv[1], "rb");

    int ret;

    unsigned short port;
    char *port_str = argv[2];
    int ret_port;
    ret_port = get_port(port_str, &port);

    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = 0;
    int request_sd, sd;

    request_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&serveraddr, 0, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;  
    serveraddr.sin_port = htons(port);

    ret = bind(request_sd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
    if (ret == -1)
    {
        perror("bind()");
        close(request_sd);
        return EXIT_FAILURE;
    }

    listen(request_sd, SOMAXCONN);
    if (ret == -1)
    {
        perror("listen()");
        close(request_sd);
        return EXIT_FAILURE;
    }

    clientaddrlen = sizeof(struct sockaddr_in);
    sd = accept(request_sd,(struct sockaddr*)&clientaddr, &clientaddrlen);
    if (sd == -1)
    {
        perror("accept()");
        close(request_sd);
        return EXIT_FAILURE;
    }

    close(request_sd); 
    
    int valg = 0;
    read(sd, &valg, sizeof(int)); 

    int counter = 1;
    if (valg == 2)
    {
        counter = 1;
    }
    else if (valg>>4 > 1)
    {
        counter = valg>>4;
    }

    while(counter)
    {
        char* strRetur = lagReturStreng(f);
        if (!strRetur)
        {
            printf("Shutting down\n");
            char* strRetur = calloc(5, sizeof(char));
            unsigned char jobbType = settJobbtype(0, 'Q');
            strRetur[0] = jobbType;
            printf("%s\n", flaggBinary(strRetur[0]));
            write(sd, strRetur, 5);
            free(strRetur);
            return 0;
        }
        int lengde = *((unsigned int*) (strRetur + 1));
        write(sd, strRetur, lengde + 5);
        free(strRetur);

        if (valg == 3)
        {
            continue;
        }

        counter = counter - 1;
    }
    printf("\n");

    close(sd);
    fclose(f);
}

char* lagReturStreng(FILE* f)
{
    char jobbInfo;
    unsigned int tekstLengde;
    char* returString;
    size_t bytesRead = fread(&jobbInfo, sizeof(char), 1, f);
    if (feof(f))
    {
        printf("\nEnd of file was reached.\n");
        return NULL;
    }
    else if (ferror(f))
    {
        printf("\nAn error occurred.\n");
        return NULL;
    }
    
    fread(&tekstLengde, sizeof(int), 1, f);
    returString= (char*) malloc((6 + tekstLengde)*sizeof(char));
    fread(returString + 5, sizeof(char), tekstLengde, f);
    returString[tekstLengde + 5] = '\0';
    char forsteByte = regneChecksum(tekstLengde, returString+5);
    forsteByte = settJobbtype(forsteByte, jobbInfo);
    returString[0] = forsteByte;
    *((unsigned int*)(returString + 1)) = tekstLengde;
    return returString;
}

char settJobbtype(char jobbInfo, char jobbChar)
{
    switch(jobbChar)
    {
        case 'O':
            break;
        case 'E':
            jobbInfo = jobbInfo | 1<<5;
            break;
        case 'Q':
            jobbInfo = jobbInfo | 7<<5;
            break;
    }
    return jobbInfo;
}