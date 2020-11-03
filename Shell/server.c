#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
/*For sockets*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /*Format structure for AF_INET*/
#include <string.h>

#define PORT 12345
#define BUF_SIZE 4096

int n; /*Number of clients*/
pid_t *pids; /*Pids of clients*/
int sockfd; /*Used socket*/

void handler(int s)
{
  int i;
  pid_t pid;

  if (s == SIGINT) /*End of client*/
  {
    for (i = 0; i < n; ++i)
      if (pids[i])
	  {
        kill(pids[i], SIGKILL);
        waitpid(pids[i], NULL, 0);
      }

    free(pids);
     
	shutdown(sockfd, SHUT_RDWR); /*Close soket*/
    close(sockfd);
    exit(0);
  }
  else
    if (s == SIGCHLD)
      while ((pid = wait(NULL)) > 0)
        for (i = 0; i < n; ++i)
          if (pids[i] == pid)
		  {
            pids[i] = 0;
            break;
          }
}

void  child_func(int fd, char *substr)
{
  char buf[BUF_SIZE]; /*String of child*/
  int sz = strlen(substr);
  int k = 0; /*Flag substring in the substring*/
  int  i; /*Counter*/
  int ans = 0; /*Answer*/
  int real_sz; /*Number of readed bytes*/
  int *pref_func = (int *) malloc(sz * sizeof(*pref_func)); /*Repeat in the substring*/

  pref_func[0] = 0;
  for (i = 1; i < sz; i++)
  {
    while (k && (substr[i] != substr[k]))
      k = pref_func[k - 1];

    if (substr[i] == substr[k])
      k++;

    pref_func[i] = k;
  }

  k = 0;
  while ((real_sz = read(fd, buf, sizeof(buf))) > 0) /*Number of readed bytes*/
    for (i = 0; i < real_sz; i++)
	{
      while (k && (buf[i] != substr[k]))
        k = pref_func[k - 1];
      if (buf[i] == substr[k])
        k++;
      if (k == sz)
	  {
        ans++;
        k = pref_func[k - 1];
      }
    }

  free(pref_func);
  printf("Number of substring: %d PID: %d\n", ans, getpid());
}

int main(int argc, char **argv)
{
  struct sockaddr_in server_info; /*Info about server*/
  int afd, i;

  if (argc != 3)
  {
    fprintf(stderr, "Please, input 3 arguments.\n"); /*argv[1] = number of clients; argv[2] = substring*/
    exit(1);
  }

  n = strtol(argv[1], NULL, 10); /*n - number of clientsin */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) /*Create socket: Domain in the network; Connection with virtual channel; Avtomatic protocol*/
  {
    fprintf(stderr, "Error of create socket.\n");
    exit(1);
  }

  server_info.sin_family = AF_INET; /*Domain in the channel*/
  server_info.sin_port = htons(PORT); /*port number in network sequence of bytes*/
  server_info.sin_addr.s_addr = htons(INADDR_ANY); /*host IP adress in network sequence of bytes*/
  if (bind(sockfd, (struct sockaddr *)&server_info, sizeof(server_info))) /*Connect socket*/
  {
    fprintf(stderr, "Error of connect with socket.\n");
    exit(1);
  }

  listen(sockfd, n); /*It is possible to connect with this socket no more then n clients*/
  pids = (pid_t *) calloc (n, sizeof(* pids)); /*Memory for array of clients pids*/

/*SIGINT - exit from the client*/
  sigaction(SIGINT, &(struct sigaction){.sa_handler = handler, .sa_flags = SA_RESTART }, NULL);
/*SIGCHLD - terminal print the result after the end of this client*/
  sigaction(SIGCHLD, &(struct sigaction){.sa_handler = handler, .sa_flags = SA_RESTART }, NULL);

  while(1)
  {
    afd = accept(sockfd, NULL, NULL); /*Set connection with the first or pause*/
    for (i = 0; i < n; ++i)
      if (!pids[i]) /*In child*/
	  {
        if ((pids[i] = fork()) == -1)
		{
          fprintf(stderr, "Error of create new process.\n");
          exit(1);
        }
        if (!pids[i]) /*Determine answer of task*/
		{
          close(sockfd);
          child_func(afd, argv[2]);
          exit(0);
        }
        break;
      }
      close(afd); /*Close connection*/
  }

  return 0;
}