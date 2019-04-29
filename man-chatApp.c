/***
 * References
 * 	- https://stackoverflow.com/questions/314401/how-to-read-a-line-from-the-console-in-c
 * 	- https://stackoverflow.com/questions/4426016/sending-a-struct-with-char-pointer-inside
 *	- https://stackoverflow.com/questions/2141277/how-to-zero-out-new-memory-after-realloc#2141327
 *  - https://stackoverflow.com/questions/16132971/how-to-send-ctrlz-in-c**/

//There are threads involved so need -pthread

//"bye" is to exit

#include <arpa/inet.h>
#include <errno.h>
//#include <linux/in.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

//#define CTRL(x) (#x[0]-'a'+1)

/***This struct is used to make some tasks easier**/
struct arg_struct{
	int port;
	char ip_domain_name[20];
};

/**This is the message struct**/
struct mssg_struct{
	unsigned char opcode;
	unsigned char name_len;
	unsigned char name;
	unsigned char text_len[2];
	char text[1];
};

//global variables
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
const int MAX_BUFFER_SIZE = 65536 + 255 + 4;
char opcode = ' ';
char client_name[255] = {'\0'};
struct sockaddr_in groupSock;
struct ip_mreqn mreq;
int sockfd;
struct arg_struct args;

//This is the signal handler
void signalHandler(int sig){
	//fprintf(stdout, "You hit Ctrl c, so ending. \n");
	if(sig == SIGINT){
		pthread_mutex_trylock(&mutex);
		opcode = '3';
		pthread_mutex_unlock(&mutex);
	}
	return;
}

//enum for char array indexes
//The package sent over the socket will be a char array with the following format
//	['mssg_len','mssg_len','mssg_len','mssg_len','mssg_len','Opcode','txt','txt','txt','txt', ...]
enum package_index {PACKAGELEN = 0, OPCODE = 6, TEXT = 8};

/***Function prototypes*/
//bool clientRole(struct arg_struct*);
struct mssg_struct* convertMssgArrayToStruct(const char*);
char* convertMssgStructToArray(struct mssg_struct*, char*);
struct mssg_struct* createPacketFromData(const char*);
int getUserInput(char**);
bool passedParametersAreOk(int, char**);
struct mssg_struct* receiveStructArray();
bool serverClientRole();
bool setSocketTimeOut();
bool setUpAddressInfo(struct arg_struct*);
void startChatting();
void startInteracting();
void* sendChatMessages();
void sendStructAsCharArr(struct mssg_struct*, struct sockaddr_in);


/****Main method*/
int main(int argc, char* argv[]){
	//signal(SIGINT, signalHandler);
	struct sigaction handler;

	handler.sa_handler = signalHandler;
	handler.sa_flags = 0;
	sigaction(SIGINT, &handler, NULL);

	sleep(1);

	//First check for number of arguements
	if(!passedParametersAreOk(argc, argv))
		return -1;

	if(!serverClientRole()){
		fprintf(stderr, "\n Error: Couldn't set up the server.\n");
		return -1;
	}

	return 0;
}

/**This method is called when the application needs
 * to play the role of a server*/
bool serverClientRole(){
	//Make a socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset((char*)&groupSock, 0, sizeof(groupSock));
	//memset((char*)&mreq, 0, sizeof(mreq));
	//if(!setUpAddressInfo(args))	return false;

	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = INADDR_ANY;
	groupSock.sin_port = htons(args.port);

	fprintf(stdout, "This is the port number %i \n", args.port);
	fprintf(stdout, "Waiting for someone to send something ...\n");
	startInteracting();

	return true;
}

/**This method does all the chatting and stuff with the client/server*/
void startInteracting(){
	char *buff = NULL;
	char op = ' ';
	int inputSuccess = -1;
	//fprintf(stdout, "Here in start interacting.\n");

	pthread_mutex_trylock(&mutex);
	opcode = '2';
	pthread_mutex_unlock(&mutex);

	//First we will send the name, get the user input for that
	fprintf(stdout, "Please insert your name here, it will be saved: ");
	do{
		inputSuccess = getUserInput(&buff);
		if(inputSuccess == -1 && opcode == '2')
			fprintf(stderr, "\n Error: error getting all the input from the user, please try again. \n");
		else if(inputSuccess == -1 && opcode == '3'){
			fprintf(stdout, "Got back from user input --------------------.\n");
			break;
		}
	}while(inputSuccess == -1 && opcode == '2');

	if(buff != NULL)
		strcpy(client_name, buff);


	startChatting();
}


/**---HELPER FUNCTIONS--**/

/**This method converts the given char buffer to a mssg_struct
 * object, and returns a pointer to the allocated object*/
struct mssg_struct* convertMssgArrayToStruct(const char* buff){

	const char* tmp;
	tmp = buff;
	size_t min_len = 6;
	if((strlen(buff) < min_len)){	return NULL;	}

	size_t txt_len = strlen(buff) - 6;

	//First set up the conversion variables
	int num_of_un_short_char = 5;
	char un_short[5] = {'0'};
	char opcode[1] = {'0'};
	char text[txt_len];

	//Then get the values from the buffer into the variables above
	memcpy(un_short, buff, 5);//The unsigned short only takes up 5 characters

	//Then allocate memory for the struct
	struct mssg_struct* packet = (struct mssg_struct* ) calloc(txt_len, sizeof(struct mssg_struct));

	packet->opcode = *((unsigned char)tmp);
	tmp++;
	//Now start copying things over
	*(packet->name_len) = *tmp;
	tmp++;
	memcpy(&packet->name, tmp, (unsigned short)packet->name_len);
	tmp = tmp + packet->name_len;
	*((unsigned short*)packet->text_len) = (unsigned short* )tmp;
	tmp = tmp + 2;
	memcpy(packet->text, tmp, packet->text_len);//Rest is the text

	fprintf(stdout, "This is after parsing: %s\n", packet->text);
	return packet;
}

/**This function converts the struct mssg_struct to a char array, and
 * returns the length of the char array**/
char* convertMssgStructToArray(struct mssg_struct* mssg, char* arr){
	unsigned short text_len = mssg->mssg_len - 2; //Two because mssg_len and opcode are both 1 byte
	char text_arr[text_len];

	memcpy(text_arr, mssg->text, text_len);

	//Get the string value of the struct into a local char array
	unsigned short padding_len = 0;
	char padding[4] = {'\0'};

	for(int i = 10000; i > 1; i = i/10){
		if(mssg->mssg_len < i)
			padding_len++;
	}

	//make the padding string
	strcpy(padding, "0");
	for(int i = 1; i < padding_len; i++){
		strcat(padding, "0");
	}

	char mssg_array[mssg->mssg_len + padding_len + 1];

	sprintf(mssg_array, "%s%hu%c%s", padding, mssg->mssg_len, mssg->opcode, mssg->text);

	//Allocate memory so the local char array can be sent to another function
	unsigned short mssg_len = strlen(mssg_array);
	char* final_struct_array = (char*) calloc(mssg_len, sizeof(char));
	memcpy(final_struct_array, mssg_array, mssg_len);

	return final_struct_array;
}

/**Make a packet using the mssg_struct, which would
 * also include the data in the struct as well**/
struct mssg_struct* createPacketFromData(const char* buff){

	unsigned short buff_len = strlen(buff);
	int buff_is_empty = 0;
	char tmp[1] = {' '};
	if(buff_len <= 0){
		buff_len = 1;
		buff_is_empty = 1;
	}

	unsigned short len = sizeof(struct mssg_struct) + buff_len -1;
	struct mssg_struct* packet = (struct mssg_struct* ) calloc(len, sizeof(struct mssg_struct));

	size_t name_len = strlen(client_name);

	pthread_mutex_trylock(&mutex);
	packet->opcode = opcode;
	pthread_mutex_unlock(&mutex);
	packet->name_len = (char *) name_len;
	memcpy(packet->name, client_name, name_len)
	*((unsigned short *)packet->text_len) = buff_len;
	if(buff_is_empty ==  0)	memcpy(packet->text, buff, buff_len);
	else if(buff_is_empty == 1)	memcpy(packet->text, tmp, buff_len);

	return packet;
}

/**This function get the user input in the console*/
int getUserInput(char** buff){
	int ret_val = 0;
    char user_input[65536] = {'\0'};
    fgets(user_input, 65536, stdin);

    size_t input_size = 0;
    input_size = strlen(user_input);

    if( (*buff = malloc(input_size)) == NULL){ //allocating memory
        fprintf(stderr, "unsuccessful allocation");
        return -1;
    }

    strcpy(*buff, user_input);

    return ret_val;
}

/**This function checks whether the passed parameters
 * are legit or are in the correct format**/
bool passedParametersAreOk(int argc, char* argv[]){

	if(argc == 5){
		for(int i = 0; i < argc; i++){
			if(strcmp(argv[i],"-port") == 0){
				i++;
				if(i >= argc)
					return false;
				args.port = atoi(argv[i]);
			}
			else if(strcmp(argv[i],"-mcip") == 0){
				i++;
				if(i >= argc)
					return false;
				strcpy(args.ip_domain_name, argv[i]);
			}

		}
		return true;
	}
	else
		fprintf(stderr, "Error: need 5 arguments total in the following order: \n\t your_program -mcip x.x.x.x. -port XX\n");
	return false;
}

/**This method recieves the char array version of the mssg_struct
 * and returns an allocated mssg_struct**/
struct mssg_struct* receiveStructArray(){
	char recvBuff[65536] = {'\0'};

	struct sockaddr_in cliaddr;
	socklen_t lenOfCli = sizeof(cliaddr);
	pthread_mutex_trylock(&mutex);
	int count = recvfrom(sockfd, (char*)&recvBuff, MAX_BUFFER_SIZE, 0, NULL, 0);
	pthread_mutex_unlock(&mutex);

	fprintf(stdout, "Recieved: %s\n", recvBuff);
	//Get the mssg_struct from this array of chars
	struct mssg_struct* mssg = convertMssgArrayToStruct(recvBuff);

	return mssg;
}

/**This function is supposed to be used by a thread
 * to always waiting for user input*/
void* sendChatMessages(){

	//TODO
	struct sockaddr_in tmp_group;
	tmp_group = groupSock;

	tmp_group.sin_addr.s_addr = inet_addr(args.ip_domain_name);

	char* buff = NULL;
	struct mssg_struct* mssg;
	int inputSuccess = -1;

	while(opcode != '3'){
		//~ fprintf(stdout, "Beginning of while loop in sendChatMessages.\n");
		do{
			inputSuccess = getUserInput(&buff);
			if(inputSuccess == -1 && opcode == '2')	fprintf(stderr, "\n Error: error getting all the input from the user, please try again. \n");
			else if(inputSuccess == -1 && opcode == '3')	break;
		}while(inputSuccess == -1 && opcode == '2');

		mssg = createPacketFromData(buff);

		//Check if the packet is under the max size
		if(mssg->mssg_len <= MAX_BUFFER_SIZE)
			sendStructAsCharArr(mssg, tmp_group);

		fprintf(stdout, "\nYou:> %s\n", mssg->text);
		free(mssg);
		free(buff);
	}

	if(opcode == '3'){
		buff = "";
		mssg = createPacketFromData(buff);
		mssg->opcode = opcode;
		sendStructAsCharArr(mssg, tmp_group);
		free(mssg);
	}
	return 0;
}

/**This method sends the char array version of the struct mssg_struct**/
void sendStructAsCharArr(struct mssg_struct* mssg, struct sockaddr_in tmp_group){
	char *struct_arr;
	int array_len = 0;
	struct_arr = convertMssgStructToArray(mssg, struct_arr);

	unsigned short struct_arr_len = strlen(struct_arr);

	//fprintf(stdout, "This is the converted txt %s\n", struct_arr);

	int addr_len = sizeof(tmp_group);

	pthread_mutex_trylock(&mutex);
	sendto(sockfd, struct_arr, struct_arr_len, 0, (struct sockaddr *)&tmp_group, addr_len);
	pthread_mutex_unlock(&mutex);

	free(struct_arr);
}

/**Set the timeout for the given socket file descriptor*/
bool setSocketTimeOut(){
	struct timeval timeout;
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;

	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0){
		fprintf(stderr, "Error: setsockopt failed\n");
		return false;
	}

	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0){
		fprintf(stderr, "Error: setsockopt failed\n");
		return false;
	}

	return true;
}

/**This method is used to set up the struct variable
 * of type sockadd_in for the information about the server*/
bool setUpAddressInfo(struct arg_struct* args){

	groupSock.sin_family = AF_INET;
	groupSock.sin_port = htons(args->port);
	fprintf(stdout, "Here 1:\n");

	fprintf(stdout, "Here 4:\n");
	return false;
}

/**Start chatting with the other user
 * main thread will be incharge of listening*/
void startChatting(){
	struct mssg_struct* mssg;
	char op = ' ';

	//bind
	if(bind(sockfd, (struct sockaddr *) &groupSock, sizeof(groupSock)) < 0){
		fprintf(stderr, "\nError: couldn't bind to the socket: %s\n", strerror(errno));
		return;
	}

	fprintf(stdout, "ip: %s\n", args.ip_domain_name);

	mreq.imr_multiaddr.s_addr = inet_addr(args.ip_domain_name);
	mreq.imr_address.s_addr = htonl(INADDR_ANY);

	if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
		fprintf(stderr, "\nError: setting sockopt for membership: %s\n", strerror(errno));
		return;
	}

	char loopch = 0;
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0){
		fprintf(stderr, "Setting IP_MULTICAST_LOOP error, %s\n", strerror(errno));
		//close(sd);
		return;
	}

	fprintf(stdout, "---Starting typing and sending mssgs---\n");

	//Set up the thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_t threadID;
	//This thread will be doing the writing part
	if(pthread_create(&threadID, &attr, sendChatMessages, 0) < 0){
		fprintf(stderr, "\n Error: Couldn't create thread.\n");
		return;
	}

	//This main thread loop will be doing the listening part
	while(opcode != '3'){
		//First recieve will be the name
		pthread_mutex_trylock(&mutex);
		mssg = receiveStructArray(sockfd);
		pthread_mutex_unlock(&mutex);

		if(mssg != NULL){
			//fprintf(stdout, "Recieved something: %s \n", mssg->text);
			if(mssg->opcode == '2'){
				fprintf(stdout, "\n%s:> %s\n\n", "client_name", mssg->text);
			}
			else if(mssg->opcode == '3'){
				op = mssg->opcode;

				pthread_mutex_trylock(&mutex);
				opcode = op;
				pthread_mutex_unlock(&mutex);

				fprintf(stdout, "Received the exitin mssg.\n");
			}
		}
		free(mssg);
		sleep(1);
		mssg = NULL;
	}
}


//Extra code

//~ fprintf(stdout, "\t\nBeginning of getUserInput.\n");
	//~ int ch; //as getchar() returns `int`

	//~ char* line;

	//~ if( (line = malloc(sizeof(char))) == NULL){ //allocating memory
        //~ fprintf(stderr, "unsuccessful allocation");
        //~ return -1;
    //~ }

	//~ int line_size = 0;
    //~ line[0]='\0';
    //~ size_t oldSize = 0;
    //~ size_t newSize = 0;
    //~ int index = 0;

	//~ fprintf(stdout, "\t\n Outside whileloop of loop getChar().\n");
	//~ if((ch = getchar()) < 0){
		//~ fprintf(stderr, "Error: getchar returned with error %s.\n", strerror(errno));
		//~ ret_val = -1;
	//~ }else{
		//~ while((ch != '\x00') && (ch != '\x03') && (ch != '\n') && (ch != EOF)){
			//~ oldSize = strlen(line);
			//~ newSize = (index + 2)*sizeof(char);

			//~ //The reallocate method is to resize the allocated memory
			//~ if( (line = realloc(line, newSize)) == NULL){
				//~ fprintf(stderr, "\n Error: reallocation error.\n");
				//~ return -1;
			//~ }
			//~ if(newSize > oldSize){ //Clearing the new space
				//~ size_t diff = newSize - oldSize;
				//~ void* pStart = ((char*) line) + oldSize;
				//~ memset(pStart, '\0', diff);
			//~ }

			//~ line[index] = (char) ch; //type casting `int` to `char`
			//~ line[index + 1] = '\0'; //inserting null character at the end
			//~ line_size = index;

			//~ if((ch = getchar()) < 0){
				//~ fprintf(stderr, "Error: getchar returned with error %s.\n", strerror(errno));
				//~ ret_val = -1;
				//~ break;
			//~ }
			//~ index++;
		//~ }
	//~ }

    //~ line_size = line_size+1;

    //~ char* line2;
    //~ line2 = calloc(line_size+1, sizeof(char));//The +1 here is just for mem access safety in the next line
    //~ // because line_size can be 1, but then line[line_size] would be accessing memory it doesn't have
    //~ line[line_size] = (char) '\0';

    //~ size_t full_len = strlen(line);
	//~ memcpy(line2, line, line_size);

    //~ *buff = line2;

    //~ memset(line, '\0', line_size);
    //~ free(line);
