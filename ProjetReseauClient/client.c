/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <message-a-transmettre>
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32 /* si vous êtes sous Windows */

#include <winsock2.h>
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)

#elif defined (linux) /* si vous êtes sous Linux */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct in_addr IN_ADDR;

#else /* sinon vous êtes sur une plateforme non supportée */

#error not defined for this platform

#endif

#define PORT 5000
typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if(err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

void end(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}

/**
    Fonction pour écrire en couleur dans la console
**/
void color(int couleurDuTexte,int couleurDeFond)
{
        HANDLE H=GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(H,couleurDeFond*16+couleurDuTexte);
}

int main(int argc, char **argv) {
    init();
    int 	socket_descriptor, 	/* descripteur de socket */
            longueur,   /* longueur d'un buffer utilisé */
            error; 		/* gestion des erreurs */
    sockaddr_in adresse_locale; 	/* adresse de socket local */
    hostent *	ptr_host; 		/* info sur une machine hote */
    servent *	ptr_service; 		/* info sur service */
    char 	buffer[256];
    char *	prog; 			/* nom du programme */
    char *	host; 			/* nom de la machine distante */
    char *	mesg; 			/* message envoyé */

    if (argc != 2) {
	perror("Nombre d'arguments incorrects, usage  : client <adresse-serveur>");
	exit(1);
    }

    prog = argv[0];
    host = argv[1];

    printf("Nom de l'executable : %s \n", prog);
    printf("Adresse du serveur  : %s \n", host);

    if ((ptr_host = gethostbyname(host)) == NULL) {
	perror("Erreur : Impossible de trouver le serveur a partir de son adresse.");
	exit(1);
    }

    /* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET;
    adresse_locale.sin_port = htons(PORT);

    printf("Numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));

    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur : impossible de creer la socket de connexion avec le serveur.");
        exit(1);
    }

    /* tentative de connexion au serveur dont les infos sont dans adresse_locale */
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
        perror("Erreur : impossible de se connecter au serveur.");
        exit(1);
    }

    printf("Connexion etablie avec le serveur. \n");

    //Buffer pour envoyer et recevoir les messages
    char bufferSend[50];
    char bufferRecv[50];

    int go = 1;
    color(2,0);

    do
    {
        color(9,0);
        printf("<<<< CLIENT DIT: ");
        fgets(bufferSend, sizeof bufferSend, stdin);
        error = send(socket_descriptor, bufferSend, strlen(bufferSend), 0);
        color(7,0);
        printf("\n...................Attente de la reponse du serveur....\n\n");
        if(error == SOCKET_ERROR)   go = 0;

        error = recv(socket_descriptor, bufferRecv, sizeof(bufferRecv)-1, 0);
        if(error == SOCKET_ERROR)   go = 0;
        else
        {
            bufferRecv[error] = '\0';
            color(12,0);
            printf(">>>> SERVER DIT: %s\n",bufferRecv);
        }
    }
    while (go);
    /* envoi du message vers le serveur */
    /*if ((send(socket_descriptor, mesg, strlen(mesg), 0)) < 0) {
	perror("erreur : impossible d'ecrire le message destine au serveur.");
	exit(1);
    }*/

    /* mise en attente du prgramme pour simuler un delai de transmission */
   /* Sleep(3);

    printf("message envoye au serveur. \n");*/

    /* lecture de la reponse en provenance du serveur */
    /*while((longueur = recv(socket_descriptor, buffer, sizeof(buffer), 0)) > 0) {
        printf("reponse du serveur : \n");
        send(1,buffer,longueur,0);
    }

    printf("\nfin de la reception.\n");*/

    close(socket_descriptor);

    printf("connexion avec le serveur fermee, fin du programme.\n");

    end();
    exit(0);
}
