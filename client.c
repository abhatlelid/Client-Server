#include <netinet/in.h>
#include <sys/socket.h>  
#include <netdb.h>
#include <stdio.h>      
#include <string.h>   
#include <unistd.h>     
#include <arpa/inet.h>   
#include <stdlib.h>  

#include "funksjoner.c"  

int skrivMeny();
int lesInn();

int usage(int argc, char* argv[]) {
    if(argc < 3) {
        printf("USAGE: %s [server-addr] [port]\n", argv[0]);
        return 1;
    }
    return 0;
}

int get_port(char* port_as_string, unsigned short* port) {
    char* endptr;

    int ret = strtol(port_as_string, &endptr, 10);
    if(endptr == port_as_string && ret == 0) {
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
    int pipefd1[2];
    int pipefd2[2];

    pipe(pipefd1);
    pipe(pipefd2);

    int pid1 = getpid();
    int pid2 = 0;
    int pid3 = 0;

    pid_t retv = fork();
    pid2 = retv;
    if (retv > 0)
    {
        pid_t retv2 = fork();
        pid3 = retv2;
    }

    if(pid2 && pid3) // parrent
    {

        int ret;
        int sd;
        char *ip_address = argv[1];
        unsigned short port;

        struct addrinfo hints;
        struct addrinfo *result, *rp;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;   
        hints.ai_socktype = SOCK_STREAM;    
        hints.ai_flags = 0;  
        hints.ai_protocol = IPPROTO_TCP;         

        ret = getaddrinfo(ip_address, argv[2], &hints, &result);
        if (ret)
        {
            const char* s =  gai_strerror(ret);
            printf("%s\n", s);
            return EXIT_FAILURE;
        }

        sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sd == -1)
        {
            perror("Socket init");
            return EXIT_FAILURE;
        }

        int ret_;
        ret_ = get_port(argv[2], &port);
        if(ret_) 
        {
            printf("[port]-argument has to be an integer!\n");
            close(sd); 
            return EXIT_FAILURE;
        }

        for (rp = result; rp != NULL; rp = rp->ai_next) 
        {
            ret = connect(sd, rp->ai_addr, rp->ai_addrlen);
            if (!ret)
            {
                break;
            }
        }

        if (rp == NULL)
        {
            fprintf(stderr, "No address succeeded!\n");
            close(sd);
            freeaddrinfo(result);
            return EXIT_FAILURE;
        }

        freeaddrinfo(result);
        int valg = skrivMeny();
        write(sd, &valg, sizeof(int));
        
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
            unsigned char flagg;
            int lengde;
            read(sd, &flagg, 1);
            if (flagg>>5 == 7)
            {
                /*
                TODO: gi beskjed til barneprosessorene om at EOF er nådd og at de må terminere. 
                */
                //write(pipefd1[1], flagg, 1);
                //write(pipefd2[1], flagg, 1);
                exit(EXIT_SUCCESS);
                
            }
            read(sd, &lengde, 4);
            char* msg = (char*) malloc((6 + lengde)*sizeof(char));
            read(sd, msg + 5, lengde);
            msg[lengde + 6] = '\0';  //
            msg[0] = flagg;
            *((unsigned int*)(msg + 1)) = lengde;
            int lengde2 = *((unsigned int*) (msg + 1));
            
           //Sjekke om checksum stemmer
            unsigned char temp = flagg<<3;
            unsigned char checkSum = temp>>3;
            if (checkSum == regneChecksum(lengde2, msg + 5))
            {
                //Da stemmer checksum

                //Sende til barneprosess 1
                if (flagg>>5 == 0)
                {
                     write(pipefd1[1], msg, lengde2 + 5);
                     free(msg);
                }
                //Sende til barneprosess 2
                if (flagg>>5 == 1)
                {
                     write(pipefd2[1], msg, lengde2 + 5);
                     free(msg);
                }
            }

            if (valg == 3)
            {
                continue;
            }

            counter = counter - 1;
        }
        printf("\n");
        close(pipefd1[1]);
        close(pipefd2[1]);
        close(sd); 
    }

    if(!(pid2 || pid3)) //child1 
    {
        unsigned char flagg;
        int lengde;
        char* melding;
        while(1)
        {
            read(pipefd1[0], &flagg, 1);
            /*
            TODO: stenge barneprosess om den har mottatt flagget 11100000
            if (flagg == (7<<5))
            {
                close(pipefd1[0]);
                exit(EXIT_SUCCESS);
            }
            */
            read(pipefd1[0], &lengde, 4);
            char* jobbtekst = (char*) malloc((lengde + 1)*sizeof(char));
            read(pipefd1[0], jobbtekst, lengde);
            jobbtekst[lengde] = '\0';
            fprintf(stdout, "%s", jobbtekst);
        }
        printf("\n");
        close(pipefd1[0]);
    }

    if(pid2 && !pid3) //child2 
    {
        char flagg;
        int lengde;
        char* melding;
        while(1)
        {
            read(pipefd2[0], &flagg, 1);
            /*
            TODO: stenge barneprosess om den har mottatt flagget 11100000
            if (flagg == (7<<5))
            {
                close(pipefd2[0]);
                exit(EXIT_SUCCESS);
            }
            */
            read(pipefd2[0], &lengde, 4);
            char* jobbtekst = (char*) malloc((lengde + 1)*sizeof(char));
            read(pipefd2[0], jobbtekst, lengde);
            jobbtekst[lengde] = '\0';
            fprintf(stderr, "%s", jobbtekst);
        }
        printf("\n");
        close(pipefd2[0]);
    }    
}

int lesInn()
{
    char buf[64];
    int tall;
    fgets(buf, sizeof(buf), stdin);
    tall = atoi(buf);
    return tall;
}

/*
Denne metoden skriver ut menyen. 
Den returnerer en int. Se protokollen for
beskrivelse av hva inten betyr. 
*/
int skrivMeny()
{
    printf("-------------------------\n");
    printf("0: Hent én jobb\n");
    printf("1: Hent flere jobber\n");
    printf("2: Hent alle jobber\n");
    printf("3: Avslutt\n"); 
    printf("Valg: ");
    printf("-------------------------\n");
    int valg = lesInn();
    
    switch(valg)
    {
        case 0:
            //hent en jobb
            return 2;
        case 1:
            printf("Hvor mange jobber: ");
            int antJobber = lesInn();
            return antJobber<<4;
        case 2:
            //hent alle jobber
            return 3;
        case 3:
            exit(EXIT_SUCCESS);
            break;
        default:
            exit(EXIT_SUCCESS);
    }
}