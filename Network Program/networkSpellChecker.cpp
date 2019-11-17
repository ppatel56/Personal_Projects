using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <unistd.h> 
#include <stdio.h>  
#include <stdlib.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <pthread.h>
#include <string.h>
#include <sstream>

#define LIST_LEN 99171
#define PORT_NUM 8888
#define NUM_WORKERS 4
#define MAX_SIZE 10
#define BUF_LEN 512

std::vector<std::string> openDictionary(std::string filename);
std::string checkWord(std::string word, std::vector<std::string> & listOfStr, int clientSocket);
void *workerThread(void *args);
void *logThread(void *args);
int open_listenfd(int port);

std::vector<std::string> list; //contains the dictionary words

queue <int> connectionQueue; //queue of socket descriptors 
queue <std::string> logQueue; //queue of logs (messages/strings)

pthread_mutex_t connectionQueueLock;
pthread_mutex_t logQueueLock;

pthread_cond_t connectionEmpty; //connection queue
pthread_cond_t connectionFill; //connection queue

pthread_cond_t logEmpty; //log queue
pthread_cond_t logFill; //log queue


/*
 *The function will iterate through all the lines in dictionary file and put them in the given vector.
 *The vector returned from the function will then be used in the following function that checkWord(). 
*/

std::vector<std::string> openDictionary(std::string fileName){
	std::vector<std::string> listOfStr; 
	// Open the File
	std::ifstream file(fileName.c_str());
 
	// Check if object is valid
	if(!file){
		std::cerr << "Cannot open the File : "<< fileName <<std::endl;
		exit(1);
	}
 
	std::string str;
	// Read the next line from File untill it reaches the end.
	while (std::getline(file, str)){

		// Line contains string of length > 0 then save it in vector
		if(str.size() > 0)
			listOfStr.push_back(str);
	}
	//Close The File
	file.close();
	return listOfStr;
}

/*
*The function takes in the string input that a client has typed and the vector list of words 
from the dictionary file.
*The function then uses a loop to help compare the input string to the list of words in order 
to determine if the input string has a correct spelling or not.
*/
std::string checkWord(std::string word, std::vector<std::string> &listOfStr, int clientSocket){
	//string result that will be appended to the input string.
	std::string result = " is incorrect spelling.\n";
	//Iterate through the number of words in the list which is 99171
    for (int i = 0; i < LIST_LEN; i++) {
      if(word == listOfStr[i]) {
          result = " is correct spelling.\n";
          break;
      }
    }
	//Append the result to the input string, then append it to log message
	//The log message format should be:
	//Client #: word is correct spelling.
	//std::string clientNum = std::to_string(clientSocket);
	
	std::string logMessage = "Client ";
	std::stringstream clientNum;
	clientNum << clientSocket;
	std::string clientStr = clientNum.str();
	logMessage = logMessage.append(clientStr);
	std::string colon = ": ";
	logMessage = logMessage.append(colon);
	word = word.append(result);
	logMessage = logMessage.append(word);
	cout << logMessage;
	return logMessage;
}


/*
*The worker thread basically takes the a client socket descriptor from the connection queue, in which the allows the client 
to type their string's spelling to check for correctness. 
*The string will then be sent to the log queue which is handled by the log thread.

*In order for the the multithreaded workers to properly work with little interference among each other (such as race condition), 
there needs to locks and conditional variables around the critical sections of code.
*These critical sections are when the client socket is getting popped of the connection queue and when the string is being pushed to 
the log queue.

*The number of worker threads corresponds to the number of clients that we will handle
*/
void *workerThread(void *args){
	
	char *promptMessage = "Enter word to spell check >> ";
	char *errorMessage = "Unable to display message.\n";
	char *exitMessage = "Connection with server is closed.\n"; //if the client types -1

	//Main while loop for threads as their is constant locking\unlocking, conditional variables, and critical section
	while(1){
		//Puts a lock when trying to pop of the connection queue
		pthread_mutex_lock(&connectionQueueLock);
		if(connectionQueue.empty()){
			//Put the thread to sleep when trying to check to see if there is any client socket to "consume"
			pthread_cond_wait(&connectionFill, &connectionQueueLock);
		}
		//Get the client socket descriptor
		int clientSocket = connectionQueue.front();
		connectionQueue.pop();
		//Send signal to main function and release the lock on Connection queue
		pthread_mutex_unlock(&connectionQueueLock);
		pthread_cond_signal(&connectionEmpty);

		while(1){
			//The string array that client types into
			char recvBuffer[BUF_LEN] = "";
			send(clientSocket, promptMessage, strlen(promptMessage), 0);
			int bytesReturned;
			//recv() will store the message from the user in the buffer, returning
			//how many bytes we received.
			bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);
			//Check if we got a message, send a message back or quit if the
			//user specified it.
			if(bytesReturned <= -1){
				send(clientSocket, errorMessage, strlen(errorMessage),0);
				continue;
			}
			else if(atoi(&recvBuffer[0]) == -1){
				send(clientSocket, exitMessage, strlen(exitMessage), 0);
				close(clientSocket);
				break;
			}
			else{

				//null terminate the string entered since the buffer size of the array is large
				recvBuffer[strlen(recvBuffer)-1] = '\0';
				recvBuffer[bytesReturned-2] = '\0';

				//convert the char string array to std::string, then pass the string into checkWord()
				string word(recvBuffer);
				//word = checkWord(word, list, clientSocket);
				std::string logMessage = checkWord(word, list, clientSocket);
				//Essentially do the same thing as before for locks and conditional variables for the log queue
				pthread_mutex_lock(&logQueueLock);
				if(logQueue.size() >= MAX_SIZE){
					pthread_cond_wait(&logEmpty, &logQueueLock);
				}
				//Add the word to the log queue
				//logQueue.push(word);
				logQueue.push(logMessage);
				//signal to logThread() function
				
				pthread_mutex_unlock(&logQueueLock);
				pthread_cond_signal(&logFill);
			}
		}
	}
}

/*
*The logThread function basically takes the result string from the log queue and writes 
that to the log file which keeps track of all the log messages from the clients.

*The premise is basically the same as the workerThread function where the locks 
and conditional variables are used.
*/
void *logThread(void *args){
	
	//main loop
	while(1){
		//lock the critical section for the log thread
		pthread_mutex_lock(&logQueueLock);
		if(logQueue.empty()){
			//make the log thread sleep and release the lock
			pthread_cond_wait(&logFill, &logQueueLock);
		}
		//get the string message from the log queue 
		std::string logStr = logQueue.front();
		logQueue.pop();
		//signal the conditional variable in the main function and then unlock the lock.
		pthread_cond_signal(&logEmpty);
		pthread_mutex_unlock(&logQueueLock);

		//Declare file variable and open log file
		fstream logFile;
		//Append the log messages to the file
		logFile.open("log.txt",std::fstream::app);
		//write the string message to the log file
		logFile << logStr;
		//close the file
		logFile.close(); 
	}
	
}




//Given function in the textbook for listening to the clients
int open_listenfd(int port){
	int listenfd, optval=1;
	struct sockaddr_in serveraddr;

	// Create a socket descriptor
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		return -1;
	}

	 // Eliminates "Address already in use" error from bind 
	 if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int)) < 0){
	 	return -1;
	 }

	 //Reset the serveraddr struct, setting all of it's bytes to zero.
	 //Some properties are then set for the struct, you don't
	 //need to worry about these. 
	 //bind() is then called, associating the port number with the
	 //socket descriptor.
	 bzero((char *) &serveraddr, sizeof(serveraddr));
	 serveraddr.sin_family = AF_INET;
	 serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	 serveraddr.sin_port = htons((unsigned short)port);
	 if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
	 	return -1;
	 }

	 //Prepare the socket to allow accept() calls. The value 20 is 
	 //the backlog, this is the maximum number of connections that will be placed
	 //on queue until accept() is called again. 
	 if (listen(listenfd, 20) < 0){
	 	return -1;
	 }

	 return listenfd;
}



int main(int argc, char **argv){

	int connectionPort;	
	std::string file = "dictionary.txt";
	list = openDictionary(file);

	//Initialize the locks and condition variables in order to 
	//be used in the threads

	pthread_mutex_init(&connectionQueueLock, NULL);
	pthread_mutex_init(&logQueueLock, NULL);

	pthread_cond_init(&connectionEmpty, NULL);
	pthread_cond_init(&connectionFill, NULL);

	pthread_cond_init(&logEmpty, NULL);
	pthread_cond_init(&logFill, NULL);

	if(argc == 1){
		connectionPort = PORT_NUM;
		file;
	}
	else if(argc == 2){
		connectionPort = atoi(argv[1]);
		file;
	}
	else{
		connectionPort = atoi(argv[1]);
		file = argv[2];
	}
	//We can't use ports below 1024 and ports above 65535 don't exist.
	if(connectionPort < 1024 || connectionPort > 65535){
		printf("Port number is either too low(below 1024), or too high(above 65535).\n");
		return -1;
	}

	//Create worker threads which the number of workers are corresponding to number of clients 
	//that we will handle.
	pthread_t workers[NUM_WORKERS];
	for(int i = 0; i < NUM_WORKERS; i++){
		pthread_create(&workers[i], NULL, &workerThread, NULL);
	}

	//create log thread, only one thread is needed
	pthread_t log;
	pthread_create(&log, NULL, &logThread, NULL);

	//messages for when connection is successful and when connection queue is full
	char *connectionSuccess = "Connection is successful. Type \"-1\" to exit\n";
	char *connectionQueueFull = "Connection Queue is full\n";

	//Setting up the client sockets  
	struct sockaddr_in client;
	int connectionSocket, clientSocket;
	connectionSocket = open_listenfd(connectionPort);
	socklen_t clientLen = sizeof(client);

	//Main while loop for storing socket descriptors
	while(1){
		//Accepts the client sockets to be used by the threads 
		clientSocket = accept(connectionSocket, (struct sockaddr*)&client, &clientLen);
		if(clientSocket == -1){
			cout << "Failed to connect to client socket." << endl;
			continue;
		}
		//Lock the connection queue so only one thread can access the critical section of
		//storing the client socket in the queue.
		pthread_mutex_lock(&connectionQueueLock);
		if(connectionQueue.size() >= MAX_SIZE){
			//Send message to client that the connection queue is full and then make 
			//the thread sleep and unlock the lock, and wait for signal on 
			//the conditional variable
			send(clientSocket, connectionQueueFull, strlen(connectionQueueFull),0);
			pthread_cond_wait(&connectionEmpty, &connectionQueueLock);
		}

		//Otherwise, if everything is a success
		cout << "Connected to a new client. Client ID: " << clientSocket << endl;
		//Send message to client that the connection is a success
		send(clientSocket, connectionSuccess, strlen(connectionSuccess), 0);

		//push client socket descriptor to the connection queue
		connectionQueue.push(clientSocket);

		//Signal the consumer which is in the workerThread()
		pthread_cond_signal(&connectionFill);
		//Unlock the critical section
		pthread_mutex_unlock(&connectionQueueLock);
		
		//pthread_cond_signal(&connectionFill);
	}
	return 0;
}