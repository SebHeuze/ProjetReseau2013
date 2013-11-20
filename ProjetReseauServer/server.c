/********************************************************
    Projet Réseau application serveur.
    Auteurs : LEDRIANT Cyril, HEUZE Sébastien
*********************************************************/
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32 /* Version windows */

#include <winsock2.h>
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)

#elif defined (linux) /* Version Linux */

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

#include <string.h> 		/* pour bcopy, ... */

#define TAILLE_MAX_NOM 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

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


/*------------------------------------------------------*/
main(int argc, char **argv) {

    init();
    int 	socket_descriptor, 		/* descripteur de socket */
            error,                      /* Gestion des erreurs */
			nouv_socket_descriptor, 	/* [nouveau] descripteur de socket */
			longueur_adresse_courante,  /* longueur d'adresse courante d'un client */
            longueur_message; 	/* longueur du message reçu */
    sockaddr_in 	adresse_locale, 		/* structure d'adresse locale*/
			adresse_client_courant; 	/* adresse client courant */
    hostent*		ptr_hote; 			/* les infos recuperees sur la machine hote */
    servent*		ptr_service;			/* les infos recuperees sur le service de la machine */
    char 		machine[TAILLE_MAX_NOM+1]; 	/* nom de la machine locale */
    char        buffer[256]; /* Va contenir les messages reçus  */

    gethostname(machine,TAILLE_MAX_NOM);		/* recuperation du nom de la machine */

    /* recuperation de la structure d'adresse en utilisant le nom */
    if ((ptr_hote = gethostbyname(machine)) == NULL) {
		perror("Erreur : impossible de trouver le serveur a partir de son nom.");
		exit(1);
    }

    /* initialisation de la structure adresse_locale avec les infos recuperees */

    /* copie de ptr_hote vers adresse_locale */
    bcopy((char*)ptr_hote->h_addr, (char*)&adresse_locale.sin_addr, ptr_hote->h_length);
    adresse_locale.sin_family		= ptr_hote->h_addrtype; 	/* ou AF_INET */
    adresse_locale.sin_addr.s_addr	= INADDR_ANY; 			/* ou AF_INET */

    /* Definir le service que l'on va utiliser a distance */
    adresse_locale.sin_port = htons(5000);

    printf("Numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));

    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Erreur : impossible de creer la socket de connexion avec le client.");
		exit(1);
    }

    /* association du socket socket_descriptor à la structure d'adresse adresse_locale */
    if ((bind(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("Erreur : impossible de lier la socket a l'adresse de connexion.");
		exit(1);
    }

    /* initialisation de la file d'ecoute */
    listen(socket_descriptor,5);

    /* attente des connexions et traitement des donnees recues */
    longueur_adresse_courante = sizeof(adresse_client_courant);

    /* adresse_client_courant sera renseigné par accept via les infos du connect */
    if ((nouv_socket_descriptor = accept(socket_descriptor, (sockaddr*)(&adresse_client_courant), &longueur_adresse_courante)) < 0) {
        perror("erreur : impossible d'accepter la connexion avec le client.");
        exit(1);
    }

    /* Client connecté */
    printf("::::: Client connecté\n\n");


    int go =1;
    while(go)
    {
        error = recv(nouv_socket_descriptor, buffer, sizeof(buffer)-1, 0);
        if(error != SOCKET_ERROR)
        {
            buffer[error] = '\0';
            color(12,0);
            printf(">>>> Le client dit: %s\n",buffer);
        }
        else
        {
            printf(":-/ Socket error, recv !\n");
            go = 0;
        }

        char bufferSend[50];
        color(9,0);
        printf("<<<< Le server dit: ");
        fgets(bufferSend, sizeof(bufferSend), stdin);
        error = send(nouv_socket_descriptor, bufferSend, strlen(bufferSend), 0);
        color(7,0);
        printf("\n...................Attente de la reponse du client....\n\n");
        if(error == SOCKET_ERROR)
        {
            color(4,0);
            printf(":-/ Socket error, send !\n");
            go = 0;
        }
    }

    color(7,0);
    close(nouv_socket_descriptor);

    end();

}


