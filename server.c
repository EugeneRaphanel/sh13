/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <arpa/inet.h>

struct _client
{
    char ipAddress[40];
    int port;
    char name[40];
} tcpClients[4];

int nbClients;
int fsmServer;
int deck[13]={0,1,2,3,4,5,6,7,8,9,10,11,12};  // cartes personnages à distribuer
// combien de symbole possède chaque joueur, il y a 8 symboles en tout
// tableCarte[0][7] = 2 => le joueur 0 possède 2 cranes (7 = crane)
int tableCartes[4][8];
char *nomcartes[]=
{"Sebastian Moran", "irene Adler", "inspector Lestrade",
  "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
  "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
  "Mrs. Hudson", "Mary Morstan", "James Moriarty"};
int joueurCourant;
int joueurElimine[4];//est mis à un si le joueur se trompe dans sa déduction
                    // il ne joue plus mais continue de répondre aux questions.
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void melangerDeck()
{
    int i;
    int index1,index2,tmp;

    for (i=0;i<1000;i++)
    {
        index1=rand()%13;
        index2=rand()%13;

        tmp=deck[index1];
        deck[index1]=deck[index2];
        deck[index2]=tmp;
    }
}

void createTable()
{
	// Le joueur 0 possede les cartes d'indice 0,1,2
	// Le joueur 1 possede les cartes d'indice 3,4,5
	// Le joueur 2 possede les cartes d'indice 6,7,8
	// Le joueur 3 possede les cartes d'indice 9,10,11
	// Le coupable est la carte d'indice 12
	int i,j,c;

	for (i=0;i<4;i++)
		for (j=0;j<8;j++)
			tableCartes[i][j]=0;

	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++)
		{
      // on parcours les 3 cartes de chaque joueur
			c=deck[i*3+j];
			switch (c)
			{
				case 0: // Sebastian Moran
					tableCartes[i][7]++;
					tableCartes[i][2]++;
					break;
				case 1: // Irene Adler
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					tableCartes[i][5]++;
					break;
				case 2: // Inspector Lestrade
					tableCartes[i][3]++;
					tableCartes[i][6]++;
					tableCartes[i][4]++;
					break;
				case 3: // Inspector Gregson
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					tableCartes[i][4]++;
					break;
				case 4: // Inspector Baynes
					tableCartes[i][3]++;
					tableCartes[i][1]++;
					break;
				case 5: // Inspector Bradstreet
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					break;
				case 6: // Inspector Hopkins
					tableCartes[i][3]++;
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					break;
				case 7: // Sherlock Holmes
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][2]++;
					break;
				case 8: // John Watson
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					tableCartes[i][2]++;
					break;
				case 9: // Mycroft Holmes
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][4]++;
					break;
				case 10: // Mrs. Hudson
					tableCartes[i][0]++;
					tableCartes[i][5]++;
					break;
				case 11: // Mary Morstan
					tableCartes[i][4]++;
					tableCartes[i][5]++;
					break;
				case 12: // James Moriarty
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					break;
			}
		}
	}
}

void printDeck()
{
  int i,j;

  for (i=0;i<13;i++)
          printf("%d %s\n",deck[i],nomcartes[deck[i]]);

	for (i=0;i<4;i++)
	{
		for (j=0;j<8;j++)
			printf("%2.2d ",tableCartes[i][j]);
		puts("");
	}
}

void printClients()
{
    int i;

    for (i=0;i<nbClients;i++)
      printf("%d: %s %5.5d %s\n",i,tcpClients[i].ipAddress,
        tcpClients[i].port,
        tcpClients[i].name);
}

int findClientByName(char *name) // trouve le numéro du client dans la table
{
        int i;

        for (i=0;i<nbClients;i++)
                if (strcmp(tcpClients[i].name,name)==0)
                        return i;
        return -1;
}

void sendMessageToClient(char *clientip, int clientport,char *mess)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(clientip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(clientport);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        {
                printf("ERROR connecting\n");
                exit(1);
        }

        sprintf(buffer,"%s\n",mess);
        n = write(sockfd,buffer,strlen(buffer));

    close(sockfd);
}

void broadcastMessage(char *mess)
{
    int i;

    for (i=0;i<nbClients;i++)
            sendMessageToClient(tcpClients[i].ipAddress, tcpClients[i].port, mess);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];   // stock les messages des clients
    struct sockaddr_in serv_addr, cli_addr;
    int n;
	  int i;

    char com;
    char clientIpAddress[256], clientName[256];
    int clientPort;
    int id;
    char reply[256];
    int endgame = 0;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0); // creation du socket
     if (sockfd < 0)
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) // dis tout ce qui viens sur mon port, c'est mon port
              error("ERROR on binding");
     listen(sockfd,5);// ouverture et début de l'écoute pour receptionner les messages
     clilen = sizeof(cli_addr);

	printDeck();
	melangerDeck();
	createTable();
	printDeck();
	joueurCourant=0; // premier arrrivé premier servi

    // initialise la table des connexions client : IP/PORT/NAME
	for (i=0;i<4;i++)
	{
        	strcpy(tcpClients[i].ipAddress,"localhost");
        	tcpClients[i].port=-1;
        	strcpy(tcpClients[i].name,"-");
	}

    // boucle serveur TCP
    while (1)
    {
     	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     	if (newsockfd < 0)
          	error("ERROR on accept");

     	bzero(buffer,256);
     	n = read(newsockfd,buffer,255);
     	if (n < 0)
		    error("ERROR reading from socket");

        printf("Received packet from %s:%d\nData: [%s]\n\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);

        // utilisées pour traiter les messages des clients
        int idJoueur, guiltyGuessed, objectAsked, playerAsked;

        // machine à etat
        // 0 => le jeu n'a pas encore commencé
        if (fsmServer==0)
        {
        	switch (buffer[0])
        	{
            	case 'C':
              	sscanf(buffer,"%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
              	printf("COM=%c ipAddress=%s port=%d name=%s\n",com, clientIpAddress, clientPort, clientName);

            	   // fsmServer==0 alors j'attends les connexions de tous les joueurs
                strcpy(tcpClients[nbClients].ipAddress,clientIpAddress);
                tcpClients[nbClients].port=clientPort;
                strcpy(tcpClients[nbClients].name,clientName);
                nbClients++;

                printClients();

                // rechercher l'id du joueur qui vient de se connecter
                id=findClientByName(clientName);
                printf("id=%d\n",id);

                // lui envoyer un message personnel pour lui communiquer son id
                sprintf(reply,"I %d",id);
                sendMessageToClient(tcpClients[id].ipAddress, tcpClients[id].port, reply);

        				// Envoyer un message broadcast pour communiquer a tout le monde la liste des joueurs actuellement
         				// connectes
                sprintf(reply,"L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
                broadcastMessage(reply);

                // Si le nombre de joueurs atteint 4, alors on peut lancer le jeu
                if (nbClients==4)
		            {
                  // On distribue les cartes (D) puis initialise le table carte de chaque joueur (V) en lui indiquant ses propres symboles
        					// On envoie ses cartes au joueur 0, ainsi que la ligne qui lui correspond dans tableCartes
                  sprintf(reply, "D %d %d %d", deck[0], deck[1], deck[2]);
                  sendMessageToClient(tcpClients[0].ipAddress, tcpClients[0].port, reply);
                  for (int i = 0; i < 8; i++) {
                    sprintf(reply, "V %d %d %d", 0, i, tableCartes[0][i]);
                    sendMessageToClient(tcpClients[0].ipAddress, tcpClients[0].port, reply);
                  }
        					// On envoie ses cartes au joueur 1, ainsi que la ligne qui lui correspond dans tableCartes
                  sprintf(reply, "D %d %d %d", deck[3], deck[4], deck[5]);
                  sendMessageToClient(tcpClients[1].ipAddress, tcpClients[1].port, reply);
                  for (int i = 0; i < 8; i++) {
                    sprintf(reply, "V %d %d %d", 1, i, tableCartes[1][i]);
                    sendMessageToClient(tcpClients[1].ipAddress, tcpClients[1].port, reply);
                  }

        					// On envoie ses cartes au joueur 2, ainsi que la ligne qui lui correspond dans tableCartes
                  sprintf(reply, "D %d %d %d", deck[6], deck[7], deck[8]);
                  sendMessageToClient(tcpClients[2].ipAddress, tcpClients[2].port, reply);
                  for (int i = 0; i < 8; i++) {
                    sprintf(reply, "V %d %d %d", 2, i, tableCartes[2][i]);
                    sendMessageToClient(tcpClients[2].ipAddress, tcpClients[2].port, reply);
                  }

        					// On envoie ses cartes au joueur 3, ainsi que la ligne qui lui correspond dans tableCartes
                  sprintf(reply, "D %d %d %d", deck[9], deck[10], deck[11]);
                  sendMessageToClient(tcpClients[3].ipAddress, tcpClients[3].port, reply);
                  for (int i = 0; i < 8; i++) {
                    sprintf(reply, "V %d %d %d", 3, i, tableCartes[3][i]);
                    sendMessageToClient(tcpClients[3].ipAddress, tcpClients[3].port, reply);
                  }

        					// On envoie enfin un message a tout le monde pour definir qui est le joueur courant=0
                  sprintf(reply, "M %d", joueurCourant);
                  broadcastMessage(reply);
                  fsmServer=1;
		            }
		            break;
          }
	     }
        // le jeu se déroule, on attends des messages de 3 types de la part des joueurs
        else if (fsmServer==1)
        {
        	switch (buffer[0])
        	{
                case 'G': // guilty : fait des accusation
                    sscanf(buffer,"G %d %d", &idJoueur, &guiltyGuessed);
                    printf("pour gagner : %d\n", deck[12]);
                    if (guiltyGuessed == deck[12]) {
                      printf("you won !\n");
                      // end game
                      sprintf(reply, "W %d %s",idJoueur, tcpClients[idJoueur].name);// on dis à tout le monde que le joueur courant à gagner
                      broadcastMessage(reply);
                      endgame = 1;
                    }
                    else {
                      printf("you lossed !\n");
                      joueurElimine[idJoueur] = 1;
                      sprintf(reply,"R");
                      sendMessageToClient(tcpClients[idJoueur].ipAddress, tcpClients[idJoueur].port, reply);
                      /*int cpt = 0;// test pour fermer le serveur si tous les joueurs ont perdu
                      for(int i = 0; i < nbClients; i++ ){
                        if(joueurElimine[i] == 1) {cpt ++;}
                      }
                      if(cpt == 4){ return 0;}*/
                    }
                    break;
                case 'O': // demande des objets à tout le monde
                    sscanf(buffer, "O %d %d", &idJoueur, &objectAsked);
                    for(int i = 0; i < 4; i++){
                      if(i != idJoueur){
                        if(tableCartes[i][objectAsked] != 0){
                        sprintf(reply, "V %d %d %d != 0", i, objectAsked, 100);
                        for(int j = 0; j < 4; j++){
                          if(j != i){
                          sendMessageToClient(tcpClients[j].ipAddress, tcpClients[j].port, reply);
                          }
                        }
                        }
                        else {
                          sprintf(reply, "V %d %d %d != 0", i, objectAsked, 0);
                          broadcastMessage(reply);
                        }
                      }
                    }
                		break;
        		    case 'S': // demande des objets à une seule personne.
              			sscanf(buffer,"S %d %d %d", &idJoueur, &playerAsked, &objectAsked);
                    sprintf(reply, "V %d %d %d", playerAsked, objectAsked, tableCartes[playerAsked][objectAsked]);
                    //sendMessageToClient(tcpClients[idJoueur].ipAddress, tcpClients[idJoueur].port, reply);
                    broadcastMessage(reply);
              			break;
                default:
                    break;
        	}
          // désigne le prochain joueur
          int i = 0;
          while(i < nbClients){
            i++;
            if (joueurCourant == 3)
              joueurCourant = 0;
            else
              joueurCourant++;
            printf(" joueurElimine[joueurCourant] : %d\n", joueurElimine[joueurCourant]);
            if(joueurElimine[joueurCourant]!=1){
              sprintf(reply, "M %d", joueurCourant);
              broadcastMessage(reply);
              break;
          }
          }
          if(i >= nbClients && joueurElimine[joueurCourant] == 1) { // si tous les joueurs ont raté leur accusation.
            sprintf(reply, "W %d %s",nbClients+1,"personne");// on envoie un nombre qui ne correspond à aucun joueur
            broadcastMessage(reply);                       //ainsi tous les joueurs ont perdu
            endgame = 1;
          }
        if(endgame){ // partie terminée
          fsmServer = 0; // pour attendre que les nouveaux clients arrivent
          for( int i = 0; i <nbClients; i++){ // on efface les joueurs pour laisser de nouveaux se connecter
           bzero((struct _client *) &tcpClients[i], sizeof(tcpClients[i]));
          }
          nbClients = 0;
          //break;
        }
        }
     	close(newsockfd);
     }

     close(sockfd);
     return 0;
}
