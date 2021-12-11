#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void)
{
	int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf[MAXDATASIZE];
	int numbytes;
	struct sigaction sa;
	pid_t pid;
	int err;
	int i;
	int fdmax; // maximum file descriptor number
	fd_set master; // master file descriptor list
	fd_set read_fds; // temp file descriptor list for select()


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server: waiting for connections...\n");
	sin_size = sizeof their_addr;

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	while(1){
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
	//		perror("accept");
			return;
		}
	
		inet_ntop(their_addr.sin_family,(struct sockaddr *)&their_addr.sin_addr,s, sizeof s);	
		printf("server: got connection from %s port: %d\n", s,their_addr.sin_port);

		pid = fork();
		if (!pid) { // this is the child process
			close(sockfd); // child doesn't need the listener

			while(1){		
				FD_ZERO(&master); // clear the master and temp sets
				FD_ZERO(&read_fds);
				FD_SET(new_fd, &master); // add to master set
				FD_SET(0, &master); // add to master set
				fdmax = new_fd;
				read_fds = master; // copy it

restart_select:
				if ((err = select(fdmax+1, &read_fds, NULL, NULL, NULL)) == -1) {	
					if(err == EINTR){
						goto restart_select;
					}
//					perror("select");
					exit(4);
				}
				for(i = 0; i <= fdmax; i++) {
					if (FD_ISSET(i, &read_fds)) { // catched
						if(i == new_fd){	// data received
							memset(buf,0,MAXDATASIZE);
							if ((numbytes = recv(new_fd, buf, 256, 0)) == -1) {	
								perror("recv");
								exit(1);
							}else  if (numbytes == 0) {
								// connection closed
								printf("Client socket %s hung up\n",s );
								close(new_fd); // parent doesn't need this
								return 0;
							}else{
								printf("RCV [%s] :%s",s,buf);
							}
						}else if(i == 0){
							memset(buf,0,MAXDATASIZE);
							fgets(buf,MAXDATASIZE,stdin);
							printf("SND: %s\n", buf);
							if (send(new_fd, buf,strlen(buf) , 0) == -1) {
								perror("send");
							}
//							printf("send data: %s",buf); 
						}
					}
				}
			}
		}	
//		printf("Client socket %s closed\n",s );
		close(new_fd); // parent doesn't need this
	}
	close(sockfd); // listen socket
	return 0;
}
