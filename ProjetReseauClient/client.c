#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "client.h"


/**
* Initialisation sockets Windows
**/
static void init(void)
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

/**
* Nettoyage sockets Windows
**/
static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

/**
* Méthode principale
**/
static void app(const char *address, const char *name)
{
   /* Initialisation de la connexion  */
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];
   char bufferSave[BUF_SIZE];
   char *commande;
   char *nomFichier;
   int numbytes;

    /* Lot de descripteurs */
   fd_set rdfs;

   /* On envoi le pseudo en premier */
   write_server(sock, name);

   while(1)
   {
      /* Mise à zéro des descripteurs */
      FD_ZERO(&rdfs);

      /* On ajoute l'entrée clavier */
      FD_SET(STDIN_FILENO, &rdfs);

      /* On ajoute le socket */
      FD_SET(sock, &rdfs);

      /* On trie les sockets actifs */
      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* Entrée clavier */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* On récupère l'entrée clavier */
         fgets(buffer, BUF_SIZE - 1, stdin);
        char *p = NULL;
        p = strstr(buffer, "\n");
        if(p != NULL)
        {
            /* On déférence le caractère si présent */
           *p = 0;
        }
        else
        {
           buffer[BUF_SIZE - 1] = 0;

        }
        //On sauvegarde avant de manipuler buffer
        strcpy(bufferSave, buffer);
        commande = strtok(buffer, " ");;

        if( commande != NULL ) {
           if(!strncasecmp(commande,"/getfile", strlen(commande))){
                printf("%s\n", "Début réception fichier");
                write_server(sock, bufferSave);
                nomFichier = strtok(NULL, " ");
                FILE* fp = fopen(nomFichier, "w");
                if((numbytes = recv(sock,buffer,1000,0))>0)
                    buffer[numbytes] = 0;
                    fprintf(fp,"%s\n",buffer);
                fclose(fp);
                printf("%s\n", "Fin réception fichier");
           }
         }
         write_server(sock, buffer);
      } /* Sinon ça vient du socket dans ce cas on lit son contenu */
      else if(FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* Si le serveur est down on affiche un message */
         if(n == 0)
         {
            printf("Server disconnected !\n");
            break;
         }
         puts(buffer);
      }
   }

   end_connection(sock);
}

/**
* Classe d'initialisation de la connexion
**/
static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   struct hostent *hostinfo;

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

/**
* On ferme la connexion
**/
static void end_connection(int sock)
{
   closesocket(sock);
}

/**
* Lecture d'un message serveur
**/
static int read_server(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }
   /* On ajoute le caractère final */
   buffer[n] = 0;

   return n;
}

/**
* Ecriture d'un message au serveur
**/
static void write_server(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

/**
* Main
**/
int main(int argc, char **argv)
{
    /* On vérifie que le nombre d'arguments est bon */
   if(argc < 2)
   {
      printf("Usage : %s [address] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();

   app(argv[1], argv[2]);

   end();

   return EXIT_SUCCESS;
}
