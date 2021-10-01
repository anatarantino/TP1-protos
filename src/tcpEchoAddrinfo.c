#include "tcpServerUtil.h"

#define max(n1,n2)     ((n1)>(n2) ? (n1) : (n2))

#define TRUE   1
#define FALSE  0
#define MIN_PORT 1024
#define MAX_PORT 49151
#define PORT 9999
#define MAX_SOCKETS 30
#define BUFFSIZE 1024
#define PORT_UDP 9996
#define MAX_PENDING_CONNECTIONS   3    // un valor bajo, para realizar pruebas
#define MAX_CHARACTERS 100

struct buffer {
	char * buffer;
	size_t len;     // longitud del buffer
	size_t from;    // desde donde falta escribir
	size_t is_done;
	int command;
};

/**
  Se encarga de escribir la respuesta faltante en forma no bloqueante
  */
void handleWrite(int socket, struct buffer * buffer, fd_set * writefds);
/**
  Limpia el buffer de escritura asociado a un socket
  */
void clear( struct buffer * buffer);

/**
  Crea y "bindea" el socket server UDP
  */
int udpSocket(int port);

/**
  Lee el datagrama del socket, obtiene info asociado con getaddrInfo y envia la respuesta
  */
void handleUdp(int socket);

int getTime(char * time_buffer);
int getDate(char * date_buffer, int format);
void toLowerString(char * str);

static enum line_state parseMessage(struct buffer * buffer, int * command);

//Para stats
int connections = 0, incorrectLines = 0, correctLines = 0, incorrectDatagrams = 0;
int default_date_format = ES;
int main(int argc , char *argv[])
{

	int master_socket;
	int new_socket , client_socket[MAX_SOCKETS] , max_clients = MAX_SOCKETS , activity, i , sd;
	long valread;
	int max_sd;
	int port = PORT;
	char date_buffer[12] = {0};
	char time_buffer[10] = {0};

	if(argc > 1){
		int arg_port = atoi(argv[1]);
		if(arg_port >= MIN_PORT && arg_port<=MAX_PORT){
			port = arg_port;
		}
	}

	log(DEBUG,"Setting port to %d",port);

	//struct sockaddr_storage clntAddr; // Client address
	//socklen_t clntAddrLen = sizeof(clntAddr);

	char buffer[BUFFSIZE + 1];  //data buffer of 1K


	//set of socket descriptors
	fd_set readfds;

	// Agregamos un buffer de escritura asociado a cada socket, para no bloquear por escritura
	struct buffer bufferWrite[MAX_SOCKETS];
	memset(bufferWrite, 0, sizeof bufferWrite);

	// y tambien los flags para writes
	fd_set writefds;

	//initialise all client_socket[] to 0 so not checked
	memset(client_socket, 0, sizeof(client_socket));

	char port_string[6] = {0};
	sprintf(port_string,"%d",port);
	master_socket = setupTCPServerSocket(port_string);

	// Socket UDP para responder en base a addrInfo
	int udpSock = udpSocket(port);
	if ( udpSock < 0) {
		log(FATAL, "UDP socket failed");
		// exit(EXIT_FAILURE);
	} else {
		log(DEBUG, "Waiting for UDP IPv4 on socket %d\n", udpSock);

	}

	// Limpiamos el conjunto de escritura
	FD_ZERO(&writefds);
	while(TRUE) 
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		
		FD_SET(master_socket, &readfds);
		FD_SET(udpSock, &readfds);

		max_sd = udpSock;

		// add child sockets to set
		for ( i = 0 ; i < max_clients ; i++) 
		{
			// socket descriptor
			sd = client_socket[i];

			// if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &readfds);

			// highest file descriptor number, need it for the select function
			if(sd > max_sd)
				max_sd = sd;
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select( max_sd + 1 , &readfds , &writefds , NULL , NULL);

		log(DEBUG, "select has something...");	

		if ((activity < 0) && (errno!=EINTR)) 
		{
			log(ERROR, "select error, errno=%d",errno);
			continue;
		}

		// Servicio UDP
		if(FD_ISSET(udpSock, &readfds)) {
			handleUdp(udpSock);
		}

		//If something happened on the TCP master socket , then its an incoming connection
		int mSock = master_socket;
		if (FD_ISSET(mSock, &readfds)) 
		{
			if ((new_socket = acceptTCPConnection(mSock)) < 0)
			{
				log(ERROR, "Accept error on master socket %d", mSock);
				continue;
			}

			// add new socket to array of sockets
			for (i = 0; i < max_clients; i++) 
			{
				// if position is empty
				if( client_socket[i] == 0 )
				{
					connections++;
					client_socket[i] = new_socket;
					log(DEBUG, "Adding to list of sockets as %d\n" , i);
					break;
				}
			}
		}
		

		for(i =0; i < max_clients; i++) {
			sd = client_socket[i];
			if (FD_ISSET(sd, &writefds) && (bufferWrite[i].is_done || bufferWrite[i].len > MAX_CHARACTERS)) {
				enum line_state state = parseMessage(bufferWrite + i,&bufferWrite[i].command );
				if(state == done_state){
					switch (bufferWrite[i].command)
					{
						case ECHO:
						bufferWrite[i].from = 5;
						correctLines++;
						if(bufferWrite[i].len > MAX_CHARACTERS){
							bufferWrite[i].buffer[MAX_CHARACTERS-2] = '\r';
							bufferWrite[i].buffer[MAX_CHARACTERS-1] = '\n';
							bufferWrite[i].len = MAX_CHARACTERS;
						}
						break;
						case GET_DATE:
						if(getDate(date_buffer,default_date_format)!=-1){
							bufferWrite[i].buffer = realloc(bufferWrite[i].buffer,sizeof(date_buffer));
							memcpy(bufferWrite[i].buffer, date_buffer,sizeof(date_buffer));
							bufferWrite[i].len = sizeof(date_buffer);
						}
						correctLines++;
						break;
						case GET_TIME:
						if(getTime(time_buffer)!=-1){
							strcpy(bufferWrite[i].buffer, time_buffer);
						}
						correctLines++;
						break;
						default:
						break;
					}
					
					handleWrite(sd, bufferWrite + i, &writefds);
				}else{
					incorrectLines++;
					clear(bufferWrite + i);
					FD_CLR(sd, &writefds);
				}
			}
		}

		//else its some IO operation on some other socket :)
		for (i = 0; i < max_clients; i++) 
		{
			sd = client_socket[i];

			if (FD_ISSET( sd , &readfds)) 
			{
				//Check if it was for closing , and also read the incoming message
				if ((valread = read( sd , buffer, BUFFSIZE)) <= 0)
				{
					log(INFO, "Host disconnected\n");

					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;

					FD_CLR(sd, &writefds);
					// Limpiamos el buffer asociado, para que no lo "herede" otra sesión
					clear(bufferWrite + i);
				}
				else {
					log(DEBUG, "Received %zu bytes from socket %d\n", valread, sd);
					// activamos el socket para escritura y almacenamos en el buffer de salida

					// Tal vez ya habia datos en el buffer
					// TODO: validar realloc != NULL
					FD_SET(sd, &writefds);
					if((bufferWrite[i].buffer = realloc(bufferWrite[i].buffer, bufferWrite[i].len + valread))!=NULL){
						memcpy(bufferWrite[i].buffer + bufferWrite[i].len, buffer, valread);
						bufferWrite[i].len += valread;
						if(bufferWrite[i].len > 1 && bufferWrite[i].buffer[bufferWrite[i].len-1] == '\n' && bufferWrite[i].buffer[bufferWrite[i].len-2] == '\r'){
							bufferWrite[i].is_done = 1;
						}
					}
				}
			}
		}
	}

	return 0;
}

void clear( struct buffer * buffer) {
	free(buffer->buffer);
	buffer->buffer = NULL;
	buffer->from = buffer->len = 0;
	buffer->is_done = 0;
	buffer->command = 0;
}

// Hay algo para escribir?
// Si está listo para escribir, escribimos. El problema es que a pesar de tener buffer para poder
// escribir, tal vez no sea suficiente. Por ejemplo podría tener 100 bytes libres en el buffer de
// salida, pero le pido que mande 1000 bytes.Por lo que tenemos que hacer un send no bloqueante,
// verificando la cantidad de bytes que pudo consumir TCP.
void handleWrite(int socket, struct buffer * buffer, fd_set * writefds) {
	size_t bytesToSend = buffer->len - buffer->from;
	if (bytesToSend > 0) {  // Puede estar listo para enviar, pero no tenemos nada para enviar
		log(INFO, "Trying to send %zu bytes to socket %d\n", bytesToSend, socket);
		size_t bytesSent = send(socket, buffer->buffer + buffer->from,bytesToSend,  MSG_DONTWAIT); 
		log(INFO, "Sent %zu bytes\n", bytesSent);

		if ( bytesSent < 0) {
			// Esto no deberia pasar ya que el socket estaba listo para escritura
			// TODO: manejar el error
			log(FATAL, "Error sending to socket %d", socket);
		} else {
			size_t bytesLeft = bytesSent - bytesToSend;

			// Si se pudieron mandar todos los bytes limpiamos el buffer y sacamos el fd para el select
			if ( bytesLeft == 0) {
				clear(buffer);
				FD_CLR(socket, writefds);
			} else {
				buffer->from += bytesSent;
			}
		}
	}
}

int udpSocket(int port) {

	int sock;
	struct sockaddr_in serverAddr;
	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		log(ERROR, "UDP socket creation failed, errno: %d %s", errno, strerror(errno));
		return sock;
	}
	log(DEBUG, "UDP socket %d created", sock);
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family    = AF_INET; // IPv4cle
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if ( bind(sock, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
	{
		log(ERROR, "UDP bind failed, errno: %d %s", errno, strerror(errno));
		close(sock);
		return -1;
	}
	log(DEBUG, "UDP socket bind OK ");

	return sock;
}


void handleUdp(int socket) {
	// En el datagrama viene el nombre a resolver
	// Se le devuelve la informacion asociada

	char buffer[BUFFSIZE];
	unsigned int len, n;

	struct sockaddr_in clntAddr;

	// Es bloqueante, deberian invocar a esta funcion solo si hay algo disponible en el socket    
	n = recvfrom(socket, buffer, BUFFSIZE, 0, ( struct sockaddr *) &clntAddr, &len);
	if (n>=1 && buffer[n-1] == '\n') // Por si lo estan probando con netcat, en modo interactivo
		n--;
	buffer[n] = '\0';
	log(DEBUG, "UDP received:%s", buffer );

	toLowerString(buffer);

	char bufferOut[BUFFSIZE];
	bufferOut[0] = '\0';

	//para usar en sscanf
	char * set;
	char * locale;
	char * lang;

	if(strcmp(buffer, "stats") == 0){

		sprintf(bufferOut, "--STATS--\r\nTotal connections: %d\r\nIncorrect lines: %d\r\nCorrect lines: %d\r\nIncorrect datagrams: %d\r\n", connections, incorrectLines, correctLines, incorrectDatagrams);
		sendto(socket, bufferOut, strlen(bufferOut), 0, (const struct sockaddr *) &clntAddr, len);
		log(DEBUG, "UDP sent:%s", bufferOut );

	}else if(sscanf(buffer, "%ms %ms %ms", &set, &locale, &lang) == 3){	//para no recorrer varias veces el string si es el mismo comando
		if(strcmp(set, "set") == 0 && strcmp(locale, "locale") == 0) //si es un set locale
		{
			if(strcmp(lang, "es") == 0){
				default_date_format = ES;
			}else if(strcmp(lang, "en") == 0){
				default_date_format = EN;
			}

		}
		free(set); free(locale); free(lang); //las reservo el sscanf
	}
	else{
		incorrectDatagrams++;
	}
	

}

static enum line_state parseMessage(struct buffer * buffer, int * command){
	
	line_parser_t * p = malloc(sizeof(*p));
	parser_init(p);
	enum line_state state;
	for(int i=0; i<buffer->len && i<MAX_LINE_LENGTH; i++){
		state = parser_feed(p,buffer->buffer[i]);
		if(state == error_command || state == error_state){
			break;
		}

	}
	if(p->state == done_state){
		*command = p->current_command;
	}
	
	free(p);
	return state;
}
