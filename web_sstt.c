#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define VERSION		24
#define BUFSIZE		8096
#define ERROR		42
#define LOG			44
#define PROHIBIDO	403
#define NOENCONTRADO	404


struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpg" },
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"ico", "image/ico" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{0,0} };

void debug(int log_message_type, char *message, char *additional_info, int socket_fd)
{
	int fd ;
	char logbuffer[BUFSIZE*2];
	
	switch (log_message_type) {
		case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",message, additional_info, errno,getpid());
			break;
		case PROHIBIDO:
			// Enviar como respuesta 403 Forbidden
			(void)sprintf(logbuffer,"FORBIDDEN: %s:%s",message, additional_info);
			break;
		case NOENCONTRADO:
			// Enviar como respuesta 404 Not Found
			(void)sprintf(logbuffer,"NOT FOUND: %s:%s",message, additional_info);
			break;
		case LOG: (void)sprintf(logbuffer," INFO: %s:%s:%d",message, additional_info, socket_fd); break;
	}

	if((fd = open("webserver.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer));
		(void)write(fd,"\n",1);
		(void)close(fd);
	}
	if(log_message_type == ERROR || log_message_type == NOENCONTRADO || log_message_type == PROHIBIDO) exit(3);
}

void write_safe(int descriptorFichero, char* response) {
	int bytesAEscribir = strlen(response);
	int bytesEscritos = 0;
	while (bytesAEscribir > 0) {
		if ((bytesEscritos = write(descriptorFichero, response, bytesAEscribir)) == -1) {
			debug(ERROR, "system call", "write", descriptorFichero);
			free(message);
			close(descriptorFichero);
			exit(EXIT_FAILURE);
		}
		bytesAEscribir -= bytesEscritos;
	}
}



void process_web_request(int descriptorFichero)
{
	debug(LOG,"request","Ha llegado una peticion",descriptorFichero);
	//
	// Definir buffer y variables necesarias para leer las peticiones
	//
	char mybuff[BUFSIZE];
	int retval = 0;
	//
	// Leer la petición HTTP
	//
	int bytesLeidos;
	if(bytesLeidos = read(descriptorFichero, mybuff, BUFSIZE) = -1) {
		debug(ERROR,"system call","read",0);
		close(descriptorFichero);	
	}
	if (bytesLeidos != BUFSIZE) {
		while (!bytesLeidos) {
			if((bytesLeidos = read(descriptorFichero, mybuff, BUFSIZE)) == -1) {
				debug(ERROR,"system call","read",0);
				close(descriptorFichero);	
			}
		}
	} 

	//
	// Si la lectura tiene datos válidos terminar el buffer con un \0
	//

	char * message = malloc(strlen(mybuff));
	strcpy(message, mybuff);	
	
	//
	// Se eliminan los caracteres de retorno de carro y nueva linea
	//

	char * line1 = strtok(message, "\r\n");
	char * line2 = strtok(NULL, "\r\n");
	
	//
	//	TRATAR LOS CASOS DE LOS DIFERENTES METODOS QUE SE USAN
	//	(Se soporta solo GET)
	//

	char * method, * url, * version, * host;
	method = strtok(line1, " ");
	url = strtok(NULL, " ");
	version = strtok(NULL, " ");

	host = strtok(line2, " ");
	host = strtok(host, " ");
	
	if ( method == NULL || url == NULL || version == NULL || host == NULL) {
		if (method == NULL) debug(ERROR, "METHOD", "NULL method detected", descriptorFichero);
		else if (url == NULL) debug(ERROR, "URL", "NULL url detected", descriptorFichero);
		else if (version == NULL) debug(ERROR, "VERSION", "NULL version detected", descriptorFichero);
		else if (host == NULL) debug(ERROR, "HOST", "NULL host detected", descriptorFichero);

		free(message);
		close(descriptorFichero);
		exit(EXIT_FAILURE);
	}

	if (!strcmp(method, "GET")) {
		char response[BUFSIZE];
		int bytesEscritos;
		//Primera linea donde se muestra la version y la respuesta al mensaje
		retval = snprintf(response, BUFSIZE, "%s 200 OK\r\n", version);
		if (retval < 0) {
			debug(ERROR, "function call","snrpintf",-1);
			free(message);
			close(descriptorFichero);
			exit(EXIT_FAILURE);
		} else {
			bytesEscritos += retval;
		}
		//Segunda línea: apartado Date
		time_t now = time(0);
 		struct tm tm = *gmtime(&now);
  		retval = strftime(response + bytesEscritos, sizeof(response) - bytesEscritos, "%a, %d %b %Y %H:%M:%S %Z\r\n", &tm);
  		bytesEscritos += retval;
  		//Tercera línea: apartado Server
  		retval = snprintf(response + bytesEscritos, BUFFSIZE - bytesEscritos, "Server: www.nomevoyacomprarna.es\r\n");
		if (retval < 0) {
			debug(ERROR, "function call","snrpintf",-1);
			free(message);
			close(descriptorFichero);
			exit(EXIT_FAILURE);
		} else {
			bytesEscritos += retval;
		}
		//Cuarta línea: apartado Keep-alive
  		retval = snprintf(response + bytesEscritos, BUFFSIZE - bytesEscritos, "Keep-Alive: timeout=3, max=0\r\n");
		if (retval < 0) {
			debug(ERROR, "function call","snrpintf",-1);
			free(message);
			close(descriptorFichero);
			exit(EXIT_FAILURE);
		} else {
			bytesEscritos += retval;
		}
		//Quinta línea: apartado Connection
		retval = snprintf(response + bytesEscritos, BUFFSIZE - bytesEscritos, "Connection: Keep-Alive\r\n");
		if (retval < 0) {
			debug(ERROR, "function call","snrpintf",-1);
			free(message);
			close(descriptorFichero);
			exit(EXIT_FAILURE);
		} else {
			bytesEscritos += retval;
		}
		//Sexta línea: apartado Content-Type
		retval = snprintf(response + bytesEscritos, BUFFSIZE - bytesEscritos, "Content-Type: text/html; charset=ISO-8859-1\r\n");
		if (retval < 0) {
			debug(ERROR, "function call","snrpintf",-1);
			free(message);
			close(descriptorFichero);
			exit(EXIT_FAILURE);
		} else {
			bytesEscritos += retval;
		}
		//Séptima línea: apartado Content-Length
		retval = snprintf(response + bytesEscritos, BUFFSIZE - bytesEscritos, "Content-Length: \r\n\r\n");
		if (retval < 0) {
			debug(ERROR, "function call","snrpintf",-1);
			free(message);
			close(descriptorFichero);
			exit(EXIT_FAILURE);
		} else {
			bytesEscritos += retval;
		}


		//strcat(response, "No me voy a comprar nada puto Jose");
		write_safe(descriptorFichero, response);

	} else {

		if (!strcmp(method, "POST")) {
			debug(ERROR, "BAD REQUEST", "POST method not supported", descriptorFichero);
		} else if (!strcmp(method, "HEAD")) {
			debug(ERROR, "BAD REQUEST", "HEAD method not supported", descriptorFichero);
		} else if (!strcmp(method, "PUT")) {
			debug(ERROR, "BAD REQUEST", "PUT method not supported", descriptorFichero);
		} else if (!strcmp(method, "DELETE")) {
			debug(ERROR, "BAD REQUEST", "DELETE method not supported", descriptorFichero);
		} else {
			debug(ERROR, "BAD REQUEST",  "method not supported", descriptorFichero);
		}

		free(message);
		close(descriptorFichero);
		exit(EXIT_FAILURE);
	}

	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);

	tv.tv_sec = 3;
	tv.tv_usec = 0;

	retval = select(1, &rfds, NULL, NULL, &tv);

	if (retval) {

	} else {

	}


	//
	//	Como se trata el caso de acceso ilegal a directorios superiores de la
	//	jerarquia de directorios
	//	del sistema
	//
	
	
	//
	//	Como se trata el caso excepcional de la URL que no apunta a ningún fichero
	//	html
	//
	
	
	//
	//	Evaluar el tipo de fichero que se está solicitando, y actuar en
	//	consecuencia devolviendolo si se soporta u devolviendo el error correspondiente en otro caso
	//
	
	
	//
	//	En caso de que el fichero sea soportado, exista, etc. se envia el fichero con la cabecera
	//	correspondiente, y el envio del fichero se hace en blockes de un máximo de  8kB
	//
	
	free(message);
	close(descriptorFichero);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int i, port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr;		// static = Inicializado con ceros
	static struct sockaddr_in serv_addr;	// static = Inicializado con ceros
	
	//  Argumentos que se esperan:
	//
	//	argv[1]
	//	En el primer argumento del programa se espera el puerto en el que el servidor escuchara
	//
	//  argv[2]
	//  En el segundo argumento del programa se espera el directorio en el que se encuentran los ficheros del servidor
	//
	//  Verficiar que los argumentos que se pasan al iniciar el programa son los esperados
	//

	//
	//  Verficiar que el directorio escogido es apto. Que no es un directorio del sistema y que se tienen
	//  permisos para ser usado
	//

	if(chdir(argv[2]) == -1){ 
		(void)printf("ERROR: No se puede cambiar de directorio %s\n",argv[2]);
		exit(4);
	}
	// Hacemos que el proceso sea un demonio sin hijos zombies
	if(fork() != 0)
		return 0; // El proceso padre devuelve un OK al shell

	(void)signal(SIGCHLD, SIG_IGN); // Ignoramos a los hijos
	(void)signal(SIGHUP, SIG_IGN); // Ignoramos cuelgues
	
	debug(LOG,"web server starting...", argv[1] ,getpid());
	
	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		debug(ERROR, "system call","socket",0);
	
	port = atoi(argv[1]);
	
	if(port < 0 || port >60000)
		debug(ERROR,"Puerto invalido, prueba un puerto de 1 a 60000",argv[1],0);
	
	/*Se crea una estructura para la información IP y puerto donde escucha el servidor*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*Escucha en cualquier IP disponible*/
	serv_addr.sin_port = htons(port); /*... en el puerto port especificado como parámetro*/
	
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		debug(ERROR,"system call","bind",0);
	
	if( listen(listenfd,64) <0)
		debug(ERROR,"system call","listen",0);
	
	while(1){
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			debug(ERROR,"system call","accept",0);
		if((pid = fork()) < 0) {
			debug(ERROR,"system call","fork",0);
		}
		else {
			if(pid == 0) { 	// Proceso hijo
				(void)close(listenfd);
				process_web_request(socketfd); // El hijo termina tras llamar a esta función
			} else { 	// Proceso padre
				(void)close(socketfd);
			}
		}
	}
}
