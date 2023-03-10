/* 
open two terminals
terminal 1: enter following command
tty

terminal 2: enter following commands
g++ client.cpp -o client -lpthread
./client <host_name> <port_no> > <output-of-tty-in-terminal-1>
*/

#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <fstream>

int BUFF_SIZE {256};
pthread_mutex_t stdout_mutex;

bool connectionClose {false};

char cli_id[25];

void log(char* message) {
    std::ofstream logFile;
    logFile.open("server.txt", std::ios::out | std::ios::app);
    if (!logFile) {
        std::cout << "Error creating log file" << std::endl;
    } else {
        logFile << message << std::endl;
        logFile.close();
    }
}

// Returns true if the parity of the 2D char array is even, false otherwise
bool parityCheck(char* BUFFER, int BUFF_SIZE) {
  bool parity = false;

  for (int i = 0; i < BUFF_SIZE; i++) {
    parity = parity ^ (BUFFER[i] == '1');
  }

  return parity;

}

// The polynomial used for the CRC calculation
const uint32_t CRC_POLYNOMIAL = 0xD8;

// Calculate the CRC of a character array
uint32_t crc(const char* data, size_t length)
{
    uint32_t crc = 0;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= static_cast<uint32_t>(data[i]);

        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ CRC_POLYNOMIAL;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}




void *send_msg(void *arg){
    int sock_fd = *(int*)arg;
    char buffer[BUFF_SIZE];
	char broken[BUFF_SIZE];

    int msg_status;
    char temp[27];
    char temp2[BUFF_SIZE];
    while(1){	
		
    	bzero(buffer, BUFF_SIZE);	
		strcpy(temp,cli_id);

		
    	fgets(temp2, BUFF_SIZE, stdin);


		strcpy(broken,buffer);
		
		// if(strcmp(temp2,"connect 4")==0){
		// msg_status = write(sock_fd, temp2, BUFF_SIZE);
			
		// }
		strcat(buffer,temp2);
		strcat(buffer,temp);

	
		// printf("%s",buffer);
		// printf("%s",temp2);
		// printf("%s",temp);

		/*std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, sizeof(buffer) - 1);
		int index = dis(gen);
		buffer[index] ^= 1;*/


		
		bool parity{parityCheck(buffer,BUFF_SIZE)};

		if(parity==true){
			std::cout<<"hata var"<<std::endl;
		}

		msg_status = write(sock_fd, buffer, BUFF_SIZE);
		if(msg_status < 0){
			perror("ERROR: on writing\n");
		}

		


		pthread_mutex_lock(&stdout_mutex);
		printf("YOU: %s\n", buffer);
		pthread_mutex_unlock(&stdout_mutex);
		
	 	if(strncmp(buffer, "close",5) == 0) {
			connectionClose = true;
			return 0;}

    }
    
    return 0;
}



void *recieve_msg(void *arg){
    int sock_fd = *(int*)arg;
    char buffer[BUFF_SIZE];
    int msg_status;
	char broken[BUFF_SIZE];
	char *fixed;
    
    while(1){
    	bzero(buffer, BUFF_SIZE);
		msg_status = read(sock_fd, buffer, BUFF_SIZE);
		
		if(msg_status < 0){
			perror("ERROR: on reading\n");
		}	

		if(connectionClose==true){
				printf("ciktim");
				return 0;
		} 

		log(buffer);


		int i = rand() % BUFF_SIZE;
		buffer[i] ^= (i << (rand()%8));


		if(crc(buffer,BUFF_SIZE)==0){
			printf("there is a error");
		}

		bool parity{parityCheck(buffer,BUFF_SIZE)};

		if(parity==true){
			std::cout<<"hata var"<<std::endl;
		}

		

		
		if(strncmp(buffer, "SERVER: DISCONNECTED", 20) == 0) {
		printf("\033[1;31m");
            msg_status = write(sock_fd, "### dummy message\n", BUFF_SIZE);
		    if(msg_status < 0){
			    perror("ERROR: on writing\n");
		    }
        }

		if(strncmp(buffer, "SERVER: CONNECTED", 17) == 0) {
			printf("\033[1;32m");
            msg_status = write(sock_fd, "### dummy message\n", BUFF_SIZE);
		    if(msg_status < 0){
			    perror("ERROR: on writing\n");
		    }
        }


		

		pthread_mutex_lock(&stdout_mutex);
		printf("%s\033[0m\n",buffer);
		pthread_mutex_unlock(&stdout_mutex);


		
    }
    
    return 0;
}

int socket_setup(char *arguments[]){
	int sock_fd, port_no ;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	port_no = atoi(arguments[2]);

	server = gethostbyname(arguments[1]);
	if(server == NULL){
		perror("ERROR: host does not exists\n");
		exit(1);
	}
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0){
		perror("ERROR: on opening socket\n");
		exit(1);
	}
	

	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port_no);
	
	if(connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("ERROR: on connection\n");
		exit(1);
	}
	
	printf("\033[1;31m------------------------------------\033[0m\n\n");

	return sock_fd ;
}



int main(int argc , char *argv[]){
	if(argc < 3){
		printf("Enter %s <host_name> <port_no>\n", argv[0]);
		exit(1);
	}
	char temp[25];
    int msg_status;

	fgets(temp,25,stdin);
	strcpy(cli_id,temp);
	int sock_fd = socket_setup(argv);
	    printf("\033[1;32mConnected to server.\033[0m\n\n" );

	pthread_mutex_init(&stdout_mutex, NULL); 
        
    pthread_t th_read, th_write;
	     char buffer[BUFF_SIZE];
	 strcpy(buffer,"get clients\n");
		msg_status = write(sock_fd, buffer, BUFF_SIZE);


    if( pthread_create(&th_read, NULL, &recieve_msg, &sock_fd) != 0 ){
		perror("ERROR: Thread creation for recieving messages failed\n");
		exit(1);
	}

    if( pthread_create(&th_write, NULL, &send_msg, &sock_fd) != 0 ){
		perror("ERROR: Thread creation for sending messages failed\n");
		exit(1);
	}
        
    if( pthread_join(th_write, NULL) != 0 ){
		perror("ERROR: Thread joining of sending messages failed\n");
		exit(1);
	}

	/* 
	thread for sending messages has been joined. It means client wants to end connection.
	So forcefully cancel the thread for recieving messages.
	*/
    if( pthread_cancel(th_read) != 0 ){
		perror("ERROR: Thread cancellation of recieving messages failed\n");
		exit(1);
	}
	printf("\033[1;31mSession Ended.\033[0m\n" );
        
	close(sock_fd);
	
	return 0;
}

