#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to
#define MAXDATASIZE 256 // max number of bytes we can get at once


int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in a;
	int rv;
	char s[INET6_ADDRSTRLEN];


	if (argc != 2) {
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	a = *((struct sockaddr_in *)p->ai_addr);
	inet_ntop(p->ai_family, &a.sin_addr,s, sizeof s);
	printf("client: connecting to %s port: %d\n", s, a.sin_port);
	freeaddrinfo(servinfo); // all done with this structure
	memset(buf,0,MAXDATASIZE);
	fgets(buf,MAXDATASIZE,stdin);
	printf("data sent: %s\n", buf);
	if (send(sockfd,buf, strlen(buf), 0) == -1)
		perror("send");
	if ((numbytes = recv(sockfd, buf, 256, 0)) == -1) {	
		perror("recv");
		exit(1);
	}else  if (numbytes == 0) {
		// connection closed
		printf("Client socket %s hung up\n",s );
	}else{
		printf("received:%s",buf);
	}
	close(sockfd);
	return 0;
}

