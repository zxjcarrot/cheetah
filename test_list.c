#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/errno.h>

#include "cheetah/reactor.h"

void stdin_read(el_socket_t fd, short res_flags, void * arg){
	printf("&struct event: %p, ", arg);
	if(res_flags & E_READ)
		printf("stdin read event ");
	if(res_flags & E_WRITE)
		printf("stdin read event.");
	printf("\n");
}
void stdout_write(el_socket_t fd, short res_flags, void * arg){
	printf("&struct event: %p, ", arg);
	if(res_flags & E_READ)
		printf("stdout read event ");
	if(res_flags & E_WRITE)
		printf("stdout read event.");
	printf("\n");
}
void stderr_read_write(el_socket_t fd, short res_flags, void * arg){
	printf("&struct event: %p, ", arg);
	if(res_flags & E_READ)
		printf("stderr read event ");
	if(res_flags & E_WRITE)
		printf("stderr write event. ");
	printf("\n");
}
struct reactor r;
struct event e1, e2;

void * thread(void * arg){
	pthread_detach(pthread_self());
	sleep(10);
	LOG("about to stop the loop.");
	reactor_get_out(&r);
}

void read_file(el_socket_t fd, short res_flags, void * arg){
	struct event * e = (struct event * )arg;
	char buf[1024];
	int n;
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = 0;
	printf("%s", buf);
	printf("\n");
	if(n == 0){
		close(fd);
		reactor_remove_event(&r, e);
		free(e);
	}
}
int echoing_counts = 0;
void echo_callback(el_socket_t fd, short res_flags, void * arg){
	struct event * e = (struct event *) arg;
	char buf[4096];
	int n;
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = 0;
	printf("got: %s\n", buf);
	write(fd, buf, n);
	if(n == 0){//peer has closed the connection
		reactor_remove_event(&r, e);
		close(fd);
	}
	if(++echoing_counts == 1000){
		reactor_get_out(&r);
	}
}

void accept_callback(el_socket_t fd, short res_flags, void * arg){
	el_socket_t csock = accept(fd, NULL, NULL);
	reactor_add_event(&r, event_new(csock, E_READ, echo_callback, NULL));
}

void * main_thread(void * arg){
	pthread_detach(pthread_self());
	reactor_loop(&r, NULL);
}
/*int main(int argc, char const *argv[]){
	el_socket_t listen_sock;
	struct sockaddr_in saddr;
	unsigned short port;
	int optval = 1;
	port = htons(8888);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = port;
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == -1){
		LOG_EXIT(1, "failed on socket: %s", strerror(errno));
	}
	if(bind(listen_sock, (struct sockaddr *)&saddr, sizeof(saddr))){
		LOG_EXIT(1, "failed on bind: %s", strerror(errno));
	}
	if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))){
		LOG_EXIT(1, "failed on setsockopt: %s", strerror(errno));
	}
	if(listen(listen_sock, 10)){
		LOG_EXIT(1, "failed on listen: %s", strerror(errno));
	}

	reactor_init(&r, "epoll", 0);
	reactor_add_event(&r, event_new(listen_sock, E_READ, accept_callback, NULL));

	reactor_loop(&r, NULL);
	reactor_destroy(&r);

	return 0;
}
*/