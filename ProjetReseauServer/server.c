#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
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
   #define errno WSAGetLastError()
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
* Methode principale
**/
static void app(void)
{

   /* Initialisation du socket */
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* Index de l'array */
   int actual = 0;
   int max = sock;
   /* Le tableau qui va stocker les clients */
   Client clients[MAX_CLIENTS];

   /* Lot de descripteurs */
   fd_set rdfs;

   char *commande;
   char *nomFichier;

   while(1)
   {
      int i = 0;
      /* Remise à zéro des descripteurs */
      FD_ZERO(&rdfs);

      /* ajout STDIN_FILENO (console)*/
      FD_SET(STDIN_FILENO, &rdfs);

      /* ajout du socket comme descripteur */
      FD_SET(sock, &rdfs);

      /* Ajout d'un socket pour chaque clients */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      /* Son sélectionne tous les descripteurs prêts */
      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* SI c'est une entrée clavier */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* Si on a tapé quelque chose au clavier on arrête l'application pour permettre à l'utilisateur d'écrire*/
         break;
      } /* Sinon si c'est une entrée socket  ( nouveau )*/
      else if(FD_ISSET(sock, &rdfs))
      {
         /* Nouveau client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);

         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* Après s'être connecté le client envoi son pseudo */
         if(read_client(csock, buffer) == -1)
         {
            /* déconnecté */
            continue;
         }

         /* Quel est le nouveau numéro de descripteur max (pour le select) ? */
         max = csock > max ? csock : max;

         /* On ajoute le client à la liste des descripteurs */
         FD_SET(csock, &rdfs);

         /* On ajoute le client à la liste des clients avec son numéro de socket */
         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         clients[actual] = c;
         actual++;
      }
      else /* Sinon on vérifie si un client a parlé */
      {
         int i = 0;
         for(i = 0; i < actual; i++) /* Pour chaque client */
         {
            /* On vérifie que le socket du client contient quelque chose */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               /* Si c'est le cas on traite le message */
               Client client = clients[i];
               /* On lie le message envoyé par le client */
               int c = read_client(clients[i].sock, buffer);
               /* Client deconnecté */
               if(c == 0)
               {
                  /* Dans ce cas on ferme le socket */
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1); /* On notifie tout les clients que ce dernier s'est déconnecté */
               }
               else
               {
                   commande = strtok(buffer, " ");
                    if( commande != NULL ) {
                    send_message_to_all_clients(clients, client, actual, buffer, 0);
                       if(!strncasecmp(commande,"/getfile", strlen(commande))){
                            printf("%s\n", "Début envoi fichier");
                            nomFichier = strtok(NULL, " ");
                            FILE* fp = fopen(nomFichier, "r");
                            int sizeread = fread(buffer,sizeof(char),1000,fp);
                            buffer[sizeread] = 0;
                            write(clients[i].sock,buffer,strlen(buffer));
                            printf("%s\n", "Fin envoi fichier");
                       }
                     }


                  /* Sinon on se chargfe juste de relayer le message */
                  send_message_to_all_clients(clients, client, actual, buffer, 0);
               }
               break;
            }
         }
      }
   }

    /* On supprime les clients */
   clear_clients(clients, actual);

   /* On ferme les connexions */
   end_connection(sock);
}

/**
* Déconnexion de tout les clients
**/
static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

/**
* Suppression d'un client spécifique
**/
static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* On l'enlève de l'array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* On décrémente le nombre de client */
   (*actual)--;
}

/**
* Envoyer un message à tout les clients connectés
**/
static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* On n'envoi pas le message à l'expéditeur*/
      if(sender.sock != clients[i].sock)
      {
         /* So c'est un message ne venant pas du serveur on affiche un pseudos*/
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

/**
* Initialisation de la connexion
**/
static int init_connection(void)
{
   /* Création du socket */
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   /* Si la création a échouée */
   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   /* Informations du serveur adresse/port */
   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   /* Lie un socket avec une structure sockaddr. */
   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   /* On définie la taille de la liste d'attente sur le socket */
   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

/**
* Fermeture d'une connexion
**/
static void end_connection(int sock)
{
   closesocket(sock);
}

/**
* Mectire d'un message envoyé par le client
**/
static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* En cas d'erreur on déconnecte le client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

/**
* On envoi un message à un client
**/
static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

/**
* main (lance l'application)
**/
int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
