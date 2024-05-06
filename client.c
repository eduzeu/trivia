/*******************************************************************************
 * Name        : client.c
 * Author      : Eduardo Hernandez
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System. 
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/select.h>
#include <netinet/in.h>


#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 25555
#define BUFFER 4096
#define ANS 128


void command_message(char* program) {
    printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n"
           "\t-i IP_address        Default to \"127.0.0.1\";\n"
           "\t-p port_number       Default to 25556;\n"
           "\t-h                   Display this help info.\n", program);
}
void inputs(int argc, char* argv[]){
    char* ip_address = DEFAULT_IP_ADDRESS;
    int port_number = DEFAULT_PORT;

    // Parse command-line arguments
    int option;
    while ((option = getopt(argc, argv, ":i:p:h")) != -1) {
        switch (option) {
            case 'i':
             if(optarg == NULL){
                perror("error: -i requires an IP address\n");
                exit(EXIT_FAILURE);
            }
                ip_address = optarg;
                break;
            case 'p':
             if(optarg == NULL){
                perror("error: requires a port number");
                exit(EXIT_FAILURE);
            }
                port_number = atoi(optarg);
                break;
            case 'h':
                command_message(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
            //    // print_message(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}
void parse_connect(int argc, char** argv, int* server_fd) {
    char* ip_address = DEFAULT_IP_ADDRESS;
    int port_number = DEFAULT_PORT;


    // Create socket
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);

    // Connect to the server
    if (connect(*server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    char name_prompt[128];
    //please type your name message
    printf("Please type your name:\n");
    scanf("%s", name_prompt);
    send(*server_fd, name_prompt, strlen(name_prompt), 0);

    
    char receive_questions[BUFFER];
    recv(*server_fd, receive_questions, sizeof(receive_questions),0);     
    printf("%s", receive_questions);
    memset(receive_questions, '\0', BUFFER);
    
    fd_set read_set;  
    char answer_buffer[ANS];

    while(1){

        int max_fd = *server_fd;
        if(max_fd < STDIN_FILENO){
            max_fd = STDIN_FILENO;
        }

        FD_SET(STDIN_FILENO, &read_set);
        FD_SET(*server_fd, &read_set);
        int select_return = select(max_fd+1, &read_set, NULL, NULL, NULL);
        
        if(select_return ==-1){
            perror("select");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(*server_fd, &read_set)){
            if(recv(*server_fd, &receive_questions, sizeof(receive_questions), 0) == 0){
                break;
            }            
            printf("%s", receive_questions);
            memset(receive_questions, 0, BUFFER);

        }
        else if(FD_ISSET(STDIN_FILENO, &read_set)){
            scanf("%s", answer_buffer);
            send(*server_fd, answer_buffer, strlen(answer_buffer), 0);
            memset(answer_buffer, 0, ANS);
            if(recv(*server_fd, &receive_questions, sizeof(receive_questions), 0) == 0){
                break;
            }
            printf("%s", receive_questions);
            memset(receive_questions, 0, BUFFER);
        }
    }  
}

int main(int argc, char* argv[]) {
    int server_fd;
    inputs(argc, argv);
    parse_connect(argc, argv, &server_fd);

    // Close the socket when done
    close(server_fd);

    return 0;
}