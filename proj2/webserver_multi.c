#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define SERVER "webserver/1.0"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#define THREAD_POOL_H__

typedef struct tpool_work {
        void*               (*routine)(void*);       
        void                *arg;                    
        struct tpool_work   *next;                    
}tpool_work_t;

typedef struct tpool {
        int             shutdown;                   
        int             max_thr_num;             
        pthread_t       *thr_id;                   
        tpool_work_t    *queue_head;                
        pthread_mutex_t queue_lock;                    
        pthread_cond_t  queue_ready;    
}tpool_t;
int tpool_create(int max_thr_num);
void tpool_destroy();

int tpool_add_work(void*(*routine)(void*), void *arg);

//thread pool

static tpool_t *tpool = NULL;

static void*  thread_routine(void *arg)
{
	tpool_work_t *work;
	while(1) {
		
		pthread_mutex_lock(&tpool->queue_lock);
		while(!tpool->queue_head && !tpool->shutdown) {
			pthread_cond_wait(&tpool->queue_ready, &tpool->queue_lock);
		}
		if (tpool->shutdown) {
			pthread_mutex_unlock(&tpool->queue_lock);
			pthread_exit(NULL);
		}
		work = tpool->queue_head;
		tpool->queue_head = tpool->queue_head->next;
		pthread_mutex_unlock(&tpool->queue_lock);
		work->routine(work->arg);
		free(work);
	}
	return NULL;   
}

int tpool_create(int max_thr_num)
{
	int i;
	tpool = calloc(1, sizeof(tpool_t));
	if (!tpool) {
		printf("%s: calloc failed\n", __FUNCTION__);
		exit(1);
	}
	
	tpool->max_thr_num = max_thr_num;
	tpool->shutdown = 0;
	tpool->queue_head = NULL;
	if (pthread_mutex_init(&tpool->queue_lock, NULL) !=0) {
		printf("%s: pthread_mutex_init failed, errno:%d, error:%s\n",
				__FUNCTION__, errno, strerror(errno));
		exit(1);
	}
	if (pthread_cond_init(&tpool->queue_ready, NULL) !=0 ) {
		printf("%s: pthread_cond_init failed, errno:%d, error:%s\n", 
				__FUNCTION__, errno, strerror(errno));
		exit(1);
	}
	
	tpool->thr_id = calloc(max_thr_num, sizeof(pthread_t));
	if (!tpool->thr_id) {
		printf("%s: calloc failed\n", __FUNCTION__);
		exit(1);
	}
	for (i = 0; i < max_thr_num; ++i) {
		if (pthread_create(&tpool->thr_id[i], NULL, thread_routine, NULL) != 0){
			printf("%s:pthread_create failed, errno:%d, error:%s\n", __FUNCTION__, 
					errno, strerror(errno));
			exit(1);
		}

	}    

	return 0;
}

void tpool_destroy()
{
	int i;
	tpool_work_t *member;

	if (tpool->shutdown) {
		return;
	}
	tpool->shutdown = 1;
	
	pthread_mutex_lock(&tpool->queue_lock);
	pthread_cond_broadcast(&tpool->queue_ready);
	pthread_mutex_unlock(&tpool->queue_lock);
	for (i = 0; i < tpool->max_thr_num; ++i) {
		pthread_join(tpool->thr_id[i], NULL);
	}
	free(tpool->thr_id);

	while(tpool->queue_head) {
		member = tpool->queue_head;
		tpool->queue_head = tpool->queue_head->next;
		free(member);
	}
	pthread_mutex_destroy(&tpool->queue_lock);    
	pthread_cond_destroy(&tpool->queue_ready);
	free(tpool);    
}

int tpool_add_work(void*(*routine)(void*), void *arg)
{
	tpool_work_t *work, *member;
	if (!routine){
		printf("%s:Invalid argument\n", __FUNCTION__);
		return -1;
	}
	work = malloc(sizeof(tpool_work_t));
	if (!work) {
		printf("%s:malloc failed\n", __FUNCTION__);
		return -1;
	}
	work->routine = routine;
	work->arg = arg;
	work->next = NULL;
	pthread_mutex_lock(&tpool->queue_lock);    
	member = tpool->queue_head;
	if (!member) {
		tpool->queue_head = work;
	} else {
		while(member->next) {
			member = member->next;
		}
		member->next = work;
	}
	
	pthread_cond_signal(&tpool->queue_ready);
	pthread_mutex_unlock(&tpool->queue_lock);

	return 0;    
}



int port, numThread;

char *get_mime_type(char *name) {
		char *ext = strrchr(name, '.');
		if (!ext) return NULL;
		if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
		if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
		if (strcmp(ext, ".gif") == 0) return "image/gif";
		if (strcmp(ext, ".png") == 0) return "image/png";
		if (strcmp(ext, ".css") == 0) return "text/css";
		if (strcmp(ext, ".au") == 0) return "audio/basic";
		if (strcmp(ext, ".wav") == 0) return "audio/wav";
		if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
		if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
		if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
		return NULL;
}

void send_headers(FILE *f, int status, char *title, char *extra, char *mime, 
				int length, time_t date) {
		time_t now;
		char timebuf[128];

		fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
		fprintf(f, "Server: %s\r\n", SERVER);
		now = time(NULL);
		strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
		fprintf(f, "Date: %s\r\n", timebuf);
		if (extra) fprintf(f, "%s\r\n", extra);
		if (mime) fprintf(f, "Content-Type: %s\r\n", mime);
		if (length >= 0) fprintf(f, "Content-Length: %d\r\n", length);
		if (date != -1) {
				strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&date));
				fprintf(f, "Last-Modified: %s\r\n", timebuf);
		}
		fprintf(f, "Connection: close\r\n");
		fprintf(f, "\r\n");
}

void send_error(FILE *f, int status, char *title, char *extra, char *text) {
		send_headers(f, status, title, extra, "text/html", -1, -1);
		fprintf(f, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n", status, title);
		fprintf(f, "<BODY><H4>%d %s</H4>\r\n", status, title);
		fprintf(f, "%s\r\n", text);
		fprintf(f, "</BODY></HTML>\r\n");
}

void send_file(FILE *f, char *path, struct stat *statbuf) {
		char data[4096];
		int n;

		FILE *file = fopen(path, "r");
		if (!file) {
				send_error(f, 403, "Forbidden", NULL, "Access denied.");
		} else {
				int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
				send_headers(f, 200, "OK", NULL, get_mime_type(path), length, statbuf->st_mtime);

				while ((n = fread(data, 1, sizeof(data), file)) > 0) fwrite(data, 1, n, f);
				fclose(file);
		}
}

int process(int s) {
		char buf[4096];
		char *method;
		char *_path;
		char path[4096];
		char *protocol;
		struct stat statbuf;
		char pathbuf[4096];
		char cwd[1024];
		int len;
		struct sockaddr_in peer;
		int peer_len = sizeof(peer);
		FILE *f;
		//f = (FILE*)arg;

		if(rand() % 10 == 0) {
				printf("Thread [pid %d, tid %d] terminated!\n", getpid(), syscall(__NR_gettid) - getpid());
				pthread_exit(NULL);
		}

		f = fdopen(s, "a+");

		if(getpeername(s, (struct sockaddr*) &peer, &peer_len) != -1) {
				printf("[pid %d, tid %d] Received a request from %s:%d\n", getpid(), syscall(__NR_gettid) - getpid(), inet_ntoa(peer.sin_addr), (int)ntohs(peer.sin_port));
		}

		if(f == NULL) {
				printf("fileopen error: %s\n", s);
				return -1;
		}

		if (!fgets(buf, sizeof(buf), f)) {
				fclose(f);
				return -1;
		}

		if(getpeername(fileno(f), (struct sockaddr*) &peer, &peer_len) != -1) {
				printf("[pid %d, tid %d] (from %s:%d) URL: %s", getpid(), syscall(__NR_gettid)-getpid(),inet_ntoa(peer.sin_addr), (int)ntohs(peer.sin_port), buf);
		} else {
				printf("[pid %d, tid %d] URL: %s", getpid(), syscall(__NR_gettid)-getpid(), buf);
		}

		method = strtok(buf, " ");
		_path = strtok(NULL, " ");
		protocol = strtok(NULL, "\r");
		if (!method || !_path || !protocol) {
				fclose(f);
				return -1;
		}

		getcwd(cwd, sizeof(cwd));
		sprintf(path, "%s%s", cwd, _path);

		fseek(f, 0, SEEK_CUR); // Force change of stream direction

		if (strcasecmp(method, "GET") != 0) {
				send_error(f, 501, "Not supported", NULL, "Method is not supported.");
				printf("[pid %d, tid %d] Reply: %s", getpid(), syscall(__NR_gettid)-getpid(), "Method is not supported.\n");
		} else if (stat(path, &statbuf) < 0) {
				send_error(f, 404, "Not Found", NULL, "File not found.");
				printf("[pid %d, tid %d] Reply: File not found - %s", getpid(), syscall(__NR_gettid)-getpid(), path);
		} else if (S_ISDIR(statbuf.st_mode)) {
				len = strlen(path);
				if (len == 0 || path[len - 1] != '/') {
						snprintf(pathbuf, sizeof(pathbuf), "Location: %s/", path);
						send_error(f, 302, "Found", pathbuf, "Directories must end with a slash.");
						printf("[pid %d, tid %d] Reply: %s", getpid(), syscall(__NR_gettid)-getpid(), "Directories mush end with a slash.\n");
				} else {
						snprintf(pathbuf, sizeof(pathbuf), "%s%sindex.html",cwd, path);
						if (stat(pathbuf, &statbuf) >= 0) {
								send_file(f, pathbuf, &statbuf);
								printf("[pid %d, tid %d] Reply: filesend %s\n", getpid(), syscall(__NR_gettid)-getpid(), pathbuf);
						} else {
								DIR *dir;
								struct dirent *de;

								send_headers(f, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);
								fprintf(f, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD>\r\n<BODY>", path);
								fprintf(f, "<H4>Index of %s</H4>\r\n<PRE>\n", path);
								fprintf(f, "Name                             Last Modified              Size\r\n");
								fprintf(f, "<HR>\r\n");
								if (len > 1) fprintf(f, "<A HREF=\"..\">..</A>\r\n");

								dir = opendir(path);
								while ((de = readdir(dir)) != NULL) {
										char timebuf[32];
										struct tm *tm;

										strcpy(pathbuf, path);
										strcat(pathbuf, de->d_name);

										stat(pathbuf, &statbuf);
										tm = gmtime(&statbuf.st_mtime);
										strftime(timebuf, sizeof(timebuf), "%d-%b-%Y %H:%M:%S", tm);

										fprintf(f, "<A HREF=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
										fprintf(f, "%s%s", de->d_name, S_ISDIR(statbuf.st_mode) ? "/</A>" : "</A> ");
										if (strlen(de->d_name) < 32) fprintf(f, "%*s", 32 - strlen(de->d_name), "");
										if (S_ISDIR(statbuf.st_mode)) {
												fprintf(f, "%s\r\n", timebuf);
										} else {
												fprintf(f, "%s %10d\r\n", timebuf, statbuf.st_size);
										}
								}
								closedir(dir);

								fprintf(f, "</PRE>\r\n<HR>\r\n<ADDRESS>%s</ADDRESS>\r\n</BODY></HTML>\r\n", SERVER);
								printf("[pid %d, tid %d] Reply: SUCCEED\n", getpid(), syscall(__NR_gettid)-getpid());
						}
				}
		} else {
				send_file(f, path, &statbuf);
				printf("[pid %d, tid %d] Reply: filesend %s\n", getpid(), syscall(__NR_gettid)-getpid(), path);
		}
		
		fclose(f);
		return 0;
}

void *routine_func(int s){
	process(s);

}

void *listener()
{
		int r;
		struct sockaddr_in sin;
		struct sockaddr_in peer;
		int peer_len = sizeof(peer);
		int sock;


		sock = socket(AF_INET, SOCK_STREAM, 0);

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
		r = bind(sock, (struct sockaddr *) &sin, sizeof(sin));
		if(r < 0) {
				perror("Error binding socket:");
				return;
		}

		r = listen(sock, 5);
		if(r < 0) {
				perror("Error listening socket:");
				return;
		}

		printf("HTTP server listening on port %d\n", port);

		while (1)
		{
				int s;
				int i;
				FILE *f;
				s = accept(sock, NULL, NULL);
				if (s < 0) break;
				for(i =0 ;i < numThread; i++){
					
					if(pthread_tryjoin_np(*(&tpool->thr_id[i]), NULL) == 0){

						if (pthread_create(*(&tpool->thr_id[i]), NULL, thread_routine, NULL) != 0){
							printf("%s:pthread_create failed, errno:%d, error:%s\n", __FUNCTION__, 
							errno, strerror(errno));
							exit(1);
						}
						else 
							printf("A new thread has been created successfully to maintain threadPool\n");
					}
				}
				if(getpeername(s, (struct sockaddr*) &peer, &peer_len) != -1) {
					printf("[pid %d, tid %d] Received a request from %s:%d\n", getpid(), syscall(__NR_gettid) - getpid(), inet_ntoa(peer.sin_addr), (int)ntohs(peer.sin_port));
					f = fdopen(s, "a+");
					tpool_add_work(routine_func, s);
				}else{

					f = fdopen(s, "a+");
					process(f);
					fclose(f);
				}
		}

		close(sock);
}

void thread_control()
{
	if(tpool_create(numThread) != 0){
		printf("threadPool create error!\n");
		exit(1);
	}

	pthread_t cThread;
	pthread_create(&cThread, NULL, listener, NULL);
	pthread_join(cThread,NULL);
		
}

int main(int argc, char *argv[])
{
		if(argc < 3 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
		{
				fprintf(stderr, "./webserver PORT(2001 ~ 49999) #_of_threads\n");
				return 0;
		}

		int i;
		
		srand(time(NULL));
		port = atoi(argv[1]);
		numThread = atoi(argv[2]);

		thread_control();

		return 0;
}
