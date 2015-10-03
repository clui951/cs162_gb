#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void handle_files_request(int fd) {

  /* YOUR CODE HERE (Feel free to delete/modify the existing code below) */

  struct http_request *request = http_request_parse(fd);
  printf("\n-- handle_files_request called --\n"); // test
  // printf("%s\n" , request->path); // path to append to working directory, or / if root
  // printf("%s\n", server_files_directory); // current working directory

  // create append working directory and request path
  // char *full_path = (char *) malloc(strlen(request->path)+strlen(server_files_directory)+1);
  char *full_path = strcat(server_files_directory, request->path);
  // full_path[0] = '\0';   // ensures the memory is an empty string
  // strcat(full_path,server_files_directory);
  // strcat(full_path,request->path);
  // printf("full_path: %s\n",full_path);

  // determine if directory or file
  struct stat path_stat;
  stat(full_path, &path_stat);
  if (S_ISDIR(path_stat.st_mode)) {
    // printf("THIS IS A DIRECTORY \n");
    
    // look up things in directory
    DIR *dir;
    struct dirent *ent;
    char retvalue[1024] = "";
    if ((dir = opendir (full_path)) != NULL) {
      ent = readdir(dir);
      while (ent != NULL) {
        // printf("%s %s\n", "element is: ", ent->d_name );
        strcat(retvalue, "<a href=\"");
        strcat(retvalue, request->path);
        strcat(retvalue, "/");
        strcat(retvalue, ent->d_name);
        strcat(retvalue, "\">");
        strcat(retvalue, ent->d_name);
        strcat(retvalue, "</a>");
        strcat(retvalue, "<p>");
        ent = readdir(dir);
      }
      // printf("FINAL retvalue: %s\n", retvalue);
      closedir (dir);
    }

    // check for index.html
    strcat(full_path,"/index.html");
    struct stat full_path_stat;
    stat(full_path, &full_path_stat);
    if (S_ISREG(full_path_stat.st_mode)) {
      // found index.html
      // printf("%s\n", "FOUND INDEX.HTML IN HERE");
      // send back index.html
      http_start_response(fd,200);
      http_send_header(fd, "Content-type", "text/html");
      // open index.html to char
      FILE *fp = fopen ( full_path , "rb" );
      fseek( fp , 0L , SEEK_END);
      long lSize = ftell( fp );
      rewind( fp );
      char *buffer = (char*) malloc(lSize+1 );
      fread( buffer , lSize, 1 , fp);
      fclose(fp);
      // printf("reached\n");
      // printf("%s\n", buffer);
      // determine content-length
      size_t buff_size = strlen(buffer);
      char str[256] = "";
      snprintf(str, sizeof(str), "%zu", buff_size);
      // printf("LENGTH IS: %s\n", str );
      http_send_header(fd, "Content-length", str);
      // http_send_header(fd, "Content-length", "70");
      http_end_headers(fd);
      http_send_string(fd,buffer);
    } else {
      // send back all files 
      http_start_response(fd,200);
      http_send_header(fd, "Content-type", "text/html");
      size_t ret_size = strlen(retvalue);
      char str[256] = "";
      snprintf(str, sizeof(str), "%zu", ret_size);
      http_send_header(fd, "Content-length", str);
      http_end_headers(fd);
      http_send_string(fd,retvalue);
      // printf("RETURN VALUE IS %s\n", retvalue);
    }

  } else if (S_ISREG(path_stat.st_mode)) {
    // printf("THIS IS A FILE \n");
    http_start_response(fd,200);
    http_send_header(fd, "Content-type", http_get_mime_type(full_path));
    // TODO: need to send the pic / file / whatever
    // http_send_string(fd, SEND_SHIT_HERE);
    FILE *fp = fopen ( full_path , "rb" );
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    // printf("%lu \n", fsize); // see the size of file
    fseek(fp, 0, SEEK_SET);
    char *string = (char*) malloc(fsize + 1);
    fread(string, fsize, 1, fp); // rett is always 1
    fclose(fp);
    string[fsize] = 0;

    // incorrect if string has null bytes
    size_t string_size = fsize;
    char str[256] = "";
    snprintf(str, sizeof(str), "%zu", string_size);
    http_send_header(fd, "Content-length", str); 
    http_end_headers(fd);
    http_send_data(fd, string, string_size);

  } else {
    // printf("NEITHER A DIR OR FILE \n");
    http_start_response(fd,404);
    http_send_header(fd, "Content-type", "text/html");
    // http_send_header(fd, "Content-length", "1024");
    http_end_headers(fd);
    http_send_string(fd,
      "<center>"
      "<h1>ERROR 4040, NOT FOUND</h1>"
      "<hr>"
      "<p>This is a custom page</p>"
      "</center>");
    // printf("HEREHERHEHRE");
  }

}

/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd) {

  /* YOUR CODE HERE */
  struct hostent* host_hostent = gethostbyname(server_proxy_hostname);
  char * found_addr = host_hostent->h_addr;
  int length_addr = host_hostent->h_length;

  int socket_number = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;

  struct in_addr inaddr;
  memset(&inaddr, 0, sizeof(struct in_addr));
  memcpy( &inaddr, found_addr, (size_t) length_addr);
  server_address.sin_addr = inaddr; // copy in specified number of bytes
  server_address.sin_port = htons(server_proxy_port);

  connect(fd, (struct sockaddr *) &server_address, sizeof(server_address));


  fd_set readd;
  fd_set action;
  FD_ZERO(&readd);
  FD_ZERO(&action);
  FD_SET(socket_number, &action);
  FD_SET(fd, &action);
  char bufferr[9999];

  while (1) {
    readd = action;
    int ret = select(FD_SETSIZE,&readd,NULL,NULL,NULL);
    if ( ret != 0 && FD_ISSET(socket_number, &readd)) {
      int ret1 = read(socket_number, bufferr , sizeof(bufferr));
      if (ret1 > 0) {
        ret1 = write(fd, bufferr, ret1);
        if (ret1 > 0) {
          continue;
        } else {
          break;
        }
      } else {
        break;
      }
    } else if ( ret!= 0 && FD_ISSET(fd, &readd)) {
      int ret1 = read(fd, bufferr, sizeof(bufferr));
      if (ret1 > 0) {
        ret1 = write(socket_number, bufferr, ret1);
        if (ret1 > 0) {
          continue;
        } else {
          break;
        }
      } else {
        break;
      }
    }
  }
  close(socket_number);

  // connect(socket_number, (struct sockaddr *) server_adress, sizeof(found_addr));

}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;
  pid_t pid;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  while (1) {

    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    pid = fork();
    if (pid > 0) {
      close(client_socket_number);
    } else if (pid == 0) {
      // Un-register signal handler (only parent should have it)
      signal(SIGINT, SIG_DFL);
      close(*socket_number);
      request_handler(client_socket_number);
      close(client_socket_number);
      exit(EXIT_SUCCESS);
    } else {
      perror("Failed to fork child");
      exit(errno);
    }
  }

  close(*socket_number);

}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);

  /* Default settings */
  server_port = 8000;
  server_files_directory = malloc(1024);
  getcwd(server_files_directory, 1024);
  server_proxy_hostname = "inst.eecs.berkeley.edu";
  server_proxy_port = 80;

  void (*request_handler)(int) = handle_files_request;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
