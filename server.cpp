/*  
    Auteur : ABDUL HAKIM Panna
    date : Fevrier - Juin 2018
    Projet : Robo Pong 1040
    Twitter : @AH_Panna
    github : @epion93
    Classe : BTS SNIR 2
    licence GNU

    Projet Realiser par BARETTE Alexandra & ALAOUA Othmane 
    Contenue du projet : Carte Raspberry 3 - Carte Arduino - Application Android
                    
                    [Partie Raspberry Socket BDD communication en serie]
    Structure BDD :
    @ = clé primaire 
    ! = clé etrangere
    
    [%--DB--]
              \
               |--user-  
               |       \
               |        [(@id),(Nom),(Prenom),(Login),(mdp),(type de compte),(isPlaying)]           
               |--parametre-  
               |            \
               |             [(@id),(zone),(vitesse),(frequence),(!userID)]
               |--historique-  
               |             \
               |              [(@id),(time),(score),(!userID)]
               |--temp(optionnel)- 
               |                  \
               |                   [(!id),(zone),(zoneER),(frequence),(vitesse),(arduino)]
        
    
    Compilation :
    Threads : g++ -o server server.cpp -lpthread 
    MySQL : g++ -o server server.cpp -lmysqlclient
    dans notre cas utiliser :g++ -o server server.cpp -lmysqlclient -lpthread

    [+]Vérifier l'ip et le port dédier pour votre utilisation ps:le client contient l'ip du serveur tcp
    Start Serveur TCP ./server
    ouvrir un autre cmd ./client entrer {message} et pour modif il faut deja passer par le message
    en premier et puis [v.{000}] pour la vitesse et la freqeunce [f.{000}]
    depuis le client envoyer comme {message} : [u.1.z.11111010.v.100.f.100]
    u = num user dans la bdd par user id
    z = 1 ou 0 par zone  elle est composer de zone d'envoi et de reception [envoi = 1111 reception = 1010]
    v = vitesse
    f = frequence
    play = pour demarrer la partie ou debug

    via le port serie l'arduino recoit de la rapsberry : e1111r1010f100v100 et retourne le score fin de partie
    Forcer la raspberry avoir son ip : ifconfig eth0 192.168.0.182 netmask 255.255.255.0 up
*/

#include<iostream>
#include<stdio.h>
#include<string.h>    
#include<stdlib.h>    
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h> 
#include<unistd.h>    
#include<pthread.h> 
#include<time.h>
#include<sstream>
#include<vector>
#include<mysql/mysql.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<termios.h>
#include<unistd.h>
#include<errno.h>

using namespace std;

//#define IP "127.0.0.1"
//Socket Info

#define IP "192.168.0.182"
#define PORT 4441
#define BUFFER_SIZE 2048

//mysql log info
#define USER "user"
#define PASS "rootroot"
#define DB	"DB"


//variables globale :

char *str; //copie du buffer attention ne pas changer
//variable global log MYSQL
MYSQL *conn;
char *server = IP;
char *user = USER;
char *password = PASS;
char *database = DB;

//ideale structure pour 1 socket
struct arduino {
  string id; //contien l'identifiant
  string zone; //init zone 10110010
  string zone_e;//1011
  string zone_r;//0010
  string zone_ER; //e1011r0010
  string freq;//stock la freqeunce init ou modif
  string vit; //stock la vitesse init ou modif
  string fin; //trame finale envoyé vers l'arduino
  string score;//contient le score finale
}ope;


//Communication en serie 
void serie(string *msg){
    int port_serie;
	struct termios options;
	int reception;
	char buffer[20] = "";
	string str=*msg;
	const char * data = str.c_str();
	bool stop = false;
	port_serie = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_NDELAY);//lecture et ecriture | pas controlling terminal | ne pas attendre DCD
	//cas d'erreur d'ouverture
	if(port_serie < 0){
		perror("Erreur d'ouverture du port serie");
		return;
	}else{
		printf("Port serie numero %d bien ouvert. \n", port_serie);
		//chargement des données
		tcgetattr(port_serie, &options);
		//B9600 bauds
        cfsetospeed(&options, B9600);       //B9600
        options.c_cflag |= (CLOCAL | CREAD);//programme propriétaire du port
		//structure en 8N1 !!
		options.c_cflag &= ~PARENB; //pas de parité
		options.c_cflag &= ~CSTOPB; // 1 bit de stop
		options.c_cflag &= ~CSIZE; //option a 0
		options.c_cflag |= CS8; //8 bits
		tcsetattr(port_serie, TCSANOW, &options); //enregistrement des valeurs de configuration
		printf("Configuration OK strcuture en 8N1 !. \n");
		
        while(!stop){
			if(strcmp(data, "quitter") == 0) {
				stop = true;
			}else {		//ecriture
				if (write(port_serie, data, sizeof(data)) < 0){perror("ERROR\n");}
				cout << "En attente de la reponse... " << endl;
				sleep(1);
					//lecture
				fcntl(port_serie,F_SETFL,10);//mode bloquant pour la fonction read
				reception=read(port_serie,buffer,20);//buffer = donnees; 20 = nb octets
				cout << "Donnees recues : " << buffer << endl;
        if (reception == -1){printf("Erreur lecture port serie\n");}}}cout << "Deconnexion du port" << endl;}
	close(port_serie);//fermeture du port serie
}

//declaration des fonctions
void INIT_MYSQL(){ //fonction init mysql
   conn = mysql_init(NULL);
   if (!mysql_real_connect(conn, server,user, password, database, 0, NULL, 0)) {fprintf(stderr, "%s\n", mysql_error(conn));exit(1);}
   }
void *connection_handler(void *);//thread fonction
void play(char *);//fonction operations pour joué
//MYSQL fonction maj des données recu vers la bdd
void sql_update_data(string z, string F,string V,string zER, string ID){
    INIT_MYSQL();
    string query_temp = "UPDATE temp SET zone='"+z+"',freq='"+F+"',vit='"+V+"',zoneE_R='"+zER+"',userID='"+ID+"' WHERE id=1";
    string query_param = "UPDATE parametre SET zone='"+z+"',frequence='"+F+"',vitesse='"+V+"' WHERE userID='"+ID+"'";
    if (mysql_query(conn, query_temp.c_str())) { fprintf(stderr, "%s\n", mysql_error(conn));exit(1);}
    if (mysql_query(conn, query_param.c_str())) { fprintf(stderr, "%s\n", mysql_error(conn));exit(1);}
    cout<<"requete OK"<<endl;
    mysql_close(conn);
}
//MYSQL fonction maj de la vitesse modifier vers la bdd
void sql_update_v(string v,string ID){
    INIT_MYSQL();
    string query_vit = "UPDATE temp SET vit='"+v+"' WHERE id=1";
    string query_param_vit = "UPDATE parametre SET vitesse='"+v+"' WHERE userID='"+ID+"'";
    if (mysql_query(conn, query_vit.c_str())) { fprintf(stderr, "%s\n", mysql_error(conn));exit(1);}
    if (mysql_query(conn, query_param_vit.c_str())) { fprintf(stderr, "%s\n", mysql_error(conn));exit(1);}
    cout<<"requete Vitesse OK"<<endl;
    mysql_close(conn);
}
//MYSQL fonction maj de la frequence modifier vers la bdd
void sql_update_f(string f,string ID){
    INIT_MYSQL();
    string query_freq = "UPDATE temp SET freq='"+f+"' WHERE id=1";
    string query_param_freq = "UPDATE parametre SET frequence='"+f+"' WHERE userID='"+ID+"'";
    if (mysql_query(conn, query_freq.c_str())) { fprintf(stderr, "%s\n", mysql_error(conn));exit(1);}
    if (mysql_query(conn, query_param_freq.c_str())) { fprintf(stderr, "%s\n", mysql_error(conn));exit(1);}
    cout<<"requete Frequence OK"<<endl;
    mysql_close(conn);
}

//fonction main
int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("[-]Erreur de Creation Socket");
    }
    puts("[+]Socket est Crée");
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_port = htons( PORT );
     
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        perror("[-]Erreur de creation du BIND vérifier la connexion ou un PID a fermer qui utilise ce socket en tcp meme ip & port");
        return 1;
    }
    puts("[+]Creation Bind par succes");
     
    listen(socket_desc , 3);
   
    puts("[+]En Attente d'une connexion client...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        printf("[+]Connexion accepter de %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0){
            perror("[-]Erreur de creation thread");
            return 1;
        }
        puts("[+]Thread assigné");
    }
    if (client_sock < 0){
        perror("[-]Erreur d'acceptation du client");
        return 1;
    }
    close(client_sock);//fermer le socket client
     
    return 0;
}

//fonction client threads
void *connection_handler(void *socket_desc){
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[BUFFER_SIZE];
    while(1){
        recv(sock , client_message , BUFFER_SIZE , 0);
        printf("%s\n",client_message);

        //prendre client message et convertir vers un tableau
        str = (char *) malloc((256));//new memory location for str
		memcpy(str, client_message, strlen(client_message)+1);// copy buffer to str

        //appelation de la fonction principal
        play(str);
        printf("[+]my PID : %d\n",getpid());//affiche mon PID(thread)
        

        bzero(client_message, sizeof(client_message)); //vider buffer
        bzero(str, sizeof(str)); //vider str copié
		memset(client_message, 0, BUFFER_SIZE);
        sleep(2);//pause 2 seconde
        memset(str, 0, sizeof str);
        free(str); //cleaning the memory location 
    }
    if(read_size == 0){
        puts("[-]Client Deconnecté");
        fflush(stdout);
    }else if(read_size == -1){perror("[-]Erreur de reception");}
    
    return 0;
} 

//fonction principale pour joué
void play(char *buf){//fonction recoit le buffer envoyé par le client
    //conversion le bufffer vers un vecteur puis vers un tableau de string
    stringstream ss(buf);
   	string temp[10];//tab conversion
	vector<string> result;
   	while( ss.good()){
    string substr;
    getline( ss, substr, '.' );//detection des points
    result.push_back( substr );}
	//copie du vecteur a tableau
	copy(result.begin(), result.end(), temp);	

    //operation {debug} 
    if(temp[0]=="debug"){
        //affiche les données importantes
        cout<<" id : ["<<ope.id<<"] "<<" zoneER : ["<<ope.zone_ER<<"] "<<" vit : ["<<ope.vit<<"] "<<" freq : ["<<ope.freq<<"] "<<endl;
    }
    if(temp[0]=="u"){//operation play
        ope.id=temp[1];
	    ope.zone=temp[3];
	    ope.zone_e = temp[3].substr (0,4);//zone d'envoi
	    ope.zone_r = temp[3].substr (4,4);//zone recep
	    ope.vit=temp[5];
	    ope.freq=temp[7];
        ope.zone_ER="e"+ope.zone_e+"r"+ope.zone_r;
        ope.fin=ope.zone_ER+ope.vit+ope.freq;//operation fin pour arduino
        cout << "data recu :";
        cout<<" id : ["<<ope.id<<"] "<<" zoneER : ["<<ope.zone_ER<<"] "<<" vit : ["<<ope.vit<<"] "<<" freq : ["<<ope.freq<<"] "<<endl;
        sql_update_data(ope.zone,ope.freq,ope.vit,ope.zone_ER,ope.id); //enregistrement des données dans la BDD
        serie(&ope.fin);//application vers le port serie

    }
    if(temp[0]=="v"){//operation modification vitesse
        cout << "data modif vitesse :";
        ope.vit=temp[1];
        ope.fin=ope.zone_ER+ope.vit+ope.freq;//operation fin pour arduino
        cout<<" id : ["<<ope.id<<"] "<<" zoneER : ["<<ope.zone_ER<<"] "<<" vit : ["<<ope.vit<<"] "<<" freq : ["<<ope.freq<<"] "<<endl; 
        sql_update_v(ope.vit,ope.id);    
        serie(&ope.fin);//application vers le port serie
    }
    if(temp[0]=="f"){//operation modification frequence
        cout << "data modif frequence :";
        ope.freq=temp[1];
        ope.fin=ope.zone_ER+ope.vit+ope.freq;//operation fin pour arduino
        cout<<" id : ["<<ope.id<<"] "<<" zoneER : ["<<ope.zone_ER<<"] "<<" vit : ["<<ope.vit<<"] "<<" freq : ["<<ope.freq<<"] "<<endl;
        sql_update_f(ope.freq,ope.id); 
        serie(&ope.fin);//application vers le port serie
    }
    //data recu + modif
    cout << "data recu :";
    cout<<" id : ["<<ope.id<<"] "<<" zoneER : ["<<ope.zone_ER<<"] "<<" vit : ["<<ope.vit<<"] "<<" freq : ["<<ope.freq<<"] "<<endl;
}


