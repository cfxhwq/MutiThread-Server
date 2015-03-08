//============================================================================
// Name        : MutiThread-Server.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
using namespace std;

#define PORT 1234
#define MAXDATASIZE 100
#define BACKLOG 5

void process_cli(int connectfd, struct sockaddr_in client);
void *function1(void *arg);
void savedata(char* recvbuf, int len, char * cli_data);

struct ARG {
	int connfd;
	struct sockaddr_in client;
};

pthread_key_t key;
pthread_once_t once = PTHREAD_ONCE_INIT;
static void destructor(void* ptr) {
	cout << "ptr" << ptr << endl;
	free(ptr);
	cout << "exec destructor" << endl;
}
struct ST_DATA {
	int index;
};
static void creatkey_once(void) {
	cout << "exec creatkey_once" << endl;
	pthread_key_create(&key, destructor);
}

int main() {
	int listenfd, connectfd;
	pthread_t tid;
	struct ARG *arg;
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t addrlen;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket() error.");
		exit(1);
	}

	int opt = SO_REUSEADDR;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))
			== -1) {
		perror("setsockopt() error ");
		exit(1);
	}

	bzero(&server, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *) &server, sizeof(server)) == -1) {
		cout << server.sin_addr.s_addr << endl;
		perror("bind() error");
		exit(1);
	}
//	addrlen = sizeof(server);
//	int a = getsockname(listenfd, (struct sockaddr *) &server, &addrlen);
//	cout << a << endl;

	cout << "bind finish" << endl;
	if (listen(listenfd, BACKLOG) == -1) {
		perror("listen() error");
		exit(1);
	}
	cout << "listen finish" << endl;

	addrlen = sizeof(client);
	while (1) {
		if ((connectfd = accept(listenfd, (struct sockaddr *) &client, &addrlen))
				== -1) {
			perror("accept() error");
			exit(1);
		}
		arg = (struct ARG *) malloc(sizeof(struct ARG));
		arg->connfd = connectfd;
		memcpy((void*) &arg->client, &client, sizeof(client));
		if (pthread_create(&tid, NULL, function1, (void*) arg)) {
			printf("Pthread create() error");
			exit(0);
		}
		cout << "tid:" << tid << endl;
	}
	close(listenfd);
}

void* function1(void* arg) {
	struct ARG *info;
	info = (struct ARG*) arg;
	process_cli(info->connfd, info->client);
	free(arg);
	cout << "arg" << arg << endl;
	pthread_exit(NULL);
}

void process_cli(int connectfd, struct sockaddr_in client) {
	int num, index = 0;
	char cli_data[1000];
	char recvbuf[MAXDATASIZE], sendbuf[MAXDATASIZE], cli_name[MAXDATASIZE];
	printf("process from %s\n", inet_ntoa(client.sin_addr));
	num = recv(connectfd, cli_name, MAXDATASIZE, 0);
	if (num == 0) {
		close(connectfd);
		printf("Client lost!\n");
		return;
	}
	printf("client name len:%d\n", num);
	cli_name[num - 1] = '\0';
	printf("Client name: %s\n", cli_name);
	while (num = recv(connectfd, recvbuf, MAXDATASIZE, 0)) {
		printf("rec msg len:%d\n", num);
		recvbuf[num - 1] = '\0';
		printf("Client %s Msg: %s\n", cli_name, recvbuf);
		savedata(recvbuf, num, cli_data);
		int i = 0;
		for (i = 0; i < num - 1; ++i) {
			if ((recvbuf[i] >= 'a' && recvbuf[i])
					|| (recvbuf[i] > 'A' && recvbuf[i] <= 'Z')) {
				recvbuf[i] += 3;
				if ((recvbuf[i] > 'Z' && recvbuf[i] <= 'Z' + 3)
						|| recvbuf[i] > 'z')
					recvbuf[i] -= 26;
			}
			sendbuf[i] = recvbuf[i];
		}
		sendbuf[num - 1] = '\0';
		send(connectfd, sendbuf, strlen(sendbuf), 0);
	}
	close(connectfd);
	printf("Client %s , data:%s!\n", cli_name, cli_data);
}

void savedata(char* recvbuf, int len, char* cli_data) {
	struct ST_DATA *data;
	pthread_once(&once, creatkey_once);
	cout << "key" << &key << endl;
	if ((data = (struct ST_DATA*) pthread_getspecific(key)) == NULL) {
		cout << "reate data" <<endl;
		data = (struct ST_DATA *) malloc(sizeof(struct ST_DATA));
		pthread_setspecific(key,data);
	}
	cout << "getDATA" << data << (data == NULL) << endl;

	int i = 0;
	while (i < len - 1) {
		cli_data[data->index++] = recvbuf[i];
		i++;
	}
	cli_data[data->index] = '\0';
}

