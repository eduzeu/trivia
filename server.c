#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <sys/select.h>
#include <fcntl.h> 
#include <unistd.h>


#define MAX_CONNECTIONS 3
#define MAX_OPTIONS 3
#define MAX_QUESTIONS 50
#define DEFAULT_QUESTION_FILE "questions.txt"
#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 25555
#define MAX_PROMPT_LENGTH 1024
#define BUFFER_SIZE 2048
#define BIG_BUFFER_SIZE 4096

void print_message(char* program){
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n"
           "\t-f question file     Default to \"question.txt\";\n"
           "\t-i IP_address        Default to \"127.0.0.1\";\n"
           "\t-p port_number       Default to 25556;\n"
           "\t-h                   Display this help info.\n", program);
}

struct Entry{
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

int read_questions(struct Entry* arr, char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error: could not open file.");
        exit(EXIT_FAILURE);
    }

    int counter = 0;
    char line[MAX_PROMPT_LENGTH];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, "?") != NULL) {
            // This is a new question, store the prompt
            strcpy(arr[counter].prompt, line);
            // Read the options line
            fgets(line, sizeof(line), file);
            char *token = strtok(line, " \t\n");
            int i = 0;
            while (token != NULL && i < MAX_OPTIONS) {
                strcpy(arr[counter].options[i], token);
                token = strtok(NULL, " \t\n");
                i++;
            }
            // Read the answer line
            fgets(line, sizeof(line), file);
            // Search for the correct option index
            int j = 0;
            while (j < MAX_OPTIONS) {
                if (strstr(line, arr[counter].options[j]) != NULL) {
                    arr[counter].answer_idx = j;
                    break;
                }
                j++;
            }
            counter++;
        }
    }

    fclose(file);
    return counter;
}
 
struct Player{
    int fd;
    int score;
    char name[256];
};

int num_questions() {
    char* filename = DEFAULT_QUESTION_FILE; // Use the default file name
    int num_questions; 
    struct Entry question[MAX_QUESTIONS];

    // Use the correct file name when calling read_questions
    if ((num_questions = read_questions(question, filename)) == -1) {
        perror("question");
        exit(EXIT_FAILURE);
    }
    return num_questions;
}

void print_entries(int client_fd, struct Entry* arr, int question_number) {
        // Format the question prompt
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "Question %d: %s", question_number + 1, arr[question_number].prompt);
        
        char option_buffer[BIG_BUFFER_SIZE];
        sprintf(option_buffer, "%sPress 1: %s\nPress 2: %s\nPress 3: %s\n", buffer, arr[question_number].options[0], arr[question_number].options[1], arr[question_number].options[2]);

        send(client_fd, option_buffer, strlen(option_buffer), 0);
        memset(option_buffer, '\0', BIG_BUFFER_SIZE);
        memset(buffer, '\0', BUFFER_SIZE);

}

int max(int a, int b, int c){
    int max = a;
    if(a < b){
        max=b;

    }
    if(c > max){
        max = c;
    }
    return max;
}

int tie(int a, int b, int c, char* player1, char* player2, char* player3){
    if (a == b && b == c){
        printf("Congrats, %s, %s and %s!\n",player1, player2, player3);
        return 1;
    }
    else if(a == b && a > c){
        printf("Congrats, %s and %s!\n",player1, player2);
        return 1;
    }
    else if(b == c && b > a){
        printf("Congrats, %s and %s\n",player2, player3);
        return 1;
    }
    else if(a == c && c > b){
        printf("Congrats, %s and %s!\n",player1, player3);
        return 1;
    }
    return 0;

}
void create_server(char *question_file, char *ip_address, int port_number) {
    
    struct Player players[MAX_CONNECTIONS]; 
    struct Entry questions[MAX_QUESTIONS];
    int num = num_questions(); // Get the number of questions

    if (read_questions(questions, question_file) == -1) {
        perror("Error reading questions");
        exit(EXIT_FAILURE);
    }  
    
 

    int server_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);
    int current_connections = 0;
    int clients = 3; 

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    server_addr.sin_port = htons(port_number);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
   if (listen(server_fd, MAX_CONNECTIONS) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Welcome to 392 Trivia!\n");

    for(int i = 0; i < 3; i++){
        players[i].fd = -1;
        players[i].score = 0;
        memset(players[i].name, 0, sizeof(players[i].name));
    }
    
    int count_quesitons = 0;
    int client_fd;
    char winner_message[512];
    int fds = 0;
 
    //accept connections from clientss
    while (1) {
        // Accept connections from clients
        int client_fd;

        // Check if max connections reached
        if (current_connections <= 3) {
            if ((client_fd = accept(server_fd, (struct sockaddr*)&in_addr, &addr_size)) == -1) {
                perror("accept");
                exit(EXIT_FAILURE);
                
            }
            current_connections++;
            

            if(current_connections >= 1 && current_connections <= 3){
                printf("New connection detected\n");
    
                char name_promt[128];
                recv(client_fd, name_promt, sizeof(name_promt), 0);
                
                name_promt[strcspn(name_promt, "\n")] = '\0';

                players[fds].fd = client_fd;
                players[fds].score = 0;
                strcpy(players[fds].name, name_promt);
                fds++;

                printf("Hi %s!\n", name_promt);

                memset(name_promt, 0, 128);

                if(current_connections== 3){
                    printf("Max connection reached!\n");
                    break;
                }
            }
        }   
    }
    printf("The game starts now\n");

    while(count_quesitons < num){
        // printf("%s\n", questions[j].prompt);
        // printf("1: %s\n2: %s\n3: %s\n", questions[j].options[0], questions[j].options[1], questions[j].options[2]);
        for(int i  = 0; i < current_connections; i++){
            print_entries(players[i].fd, questions, count_quesitons);
        }
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int max_fd = server_fd;

    for (int i = 0; i < current_connections; i++) {
        FD_SET(players[i].fd, &read_fds);
        if (players[i].fd > max_fd) {
            max_fd = players[i].fd;
        }
    }

    int select_fun = select(max_fd + 1, &read_fds,NULL, NULL, NULL);
    if (select_fun < -1) {
        perror("select");
        exit(EXIT_FAILURE);
    } else{
        for(int i = 0; i < current_connections; i++){
            char store_answer;
           
            if(FD_ISSET(players[i].fd, &read_fds)){
                if(recv(players[i].fd, &store_answer,sizeof(store_answer), 0) <= 0){
                    printf("lost connection!\n");
                    exit(EXIT_FAILURE);
                }
                //printf("%c", answer);
                char correct_answer[256];
                if(atoi(&store_answer) == questions[count_quesitons].answer_idx+1){
                    players[i].score++; //increment if answer is right
                    sprintf(correct_answer, "The correct answer is: %d\n\n", questions[count_quesitons].answer_idx+1);
                }else{
                    players[i].score--; //decrement if answer is wrong
                    sprintf(correct_answer, "The correct answer is: %d\n\n", questions[count_quesitons].answer_idx + 1);
                }
                for (int k = 0; k < current_connections; k++) {
                    send(players[k].fd, correct_answer, strlen(correct_answer), 0);
                }
                memset(correct_answer, '\0', 256);
            }else{
                // char correct_answer[4096];
                // if(answer == questions[i].answer_idx){
                //     sprintf(correct_answer, "right answer: %d", questions[i].answer_idx);
                // }else{
                //     sprintf(correct_answer, "Wrong answer: %d", questions[i].answer_idx);
                // }
                continue;
                
                }
            }
        }
        count_quesitons++;

    }
    
    int check_tie;
    check_tie = tie(players[0].score, players[1].score, players[2].score, players[0].name, players[1].name, players[2].name);

   if (check_tie == 0){
        int winner = max(players[0].score, players[1].score, players[2].score);
        for(int i = 0; i < current_connections; i++){
            if(winner == players[i].score){
                printf("Congrats, %s!\n", players[i].name);
                break;
            }
        }
   }

    memset(winner_message, '\0', 512);

    close(server_fd);
    for (int i = 0; i < current_connections; i++) {
        close(players[i].fd);
    }        
}

void inputs(int argc, char* argv[]){

    char *question_file = DEFAULT_QUESTION_FILE;
    char *ip_address = DEFAULT_IP_ADDRESS;
    int port_number = DEFAULT_PORT;

    int input = 0;
    while((input = getopt(argc, argv, ":fiph")) != -1){

        switch (input)
        {
        case 'f':
            if(optarg == NULL){
                perror("error: -f requires argument.\n");
                exit(EXIT_FAILURE);
            }
            question_file = optarg;
            break;
        
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
            print_message(argv[0]);
            exit(EXIT_SUCCESS);
        
        case '?':
            fprintf(stderr, "Error: Uknown option '-%c' received.\n ", optopt);
            exit(EXIT_FAILURE);
            break;
        }
    }

}
int main(int argc, char* argv[]){

    char *question_file = DEFAULT_QUESTION_FILE;
    char *ip_address = DEFAULT_IP_ADDRESS;
    int port_number = DEFAULT_PORT;
    int server_fd;


    inputs(argc,argv);
    create_server(question_file, ip_address, port_number);
    //close(server_fd);
   
    return 0;
}
