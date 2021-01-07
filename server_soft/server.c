/*
    Всё, что делает этот сервер — шлёт строку «Hello, World!n» через потоковое соединение. 
    Всё, что вам нужнo 
    сделать для проверки этого сервера — 
    запустить его в одном окне, 
    а в другом запустить telnet и 
    зайти на порт своего сервера:
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <netdb.h>
//Функции для преобразования протокольных имен и имен хостов в числовые адреса. Используются локальные данные аналогично DNS.
#include <sys/types.h>
//Базовые функции сокетов BSD и структуры данных.
#include <sys/socket.h>
//Семейства адресов/протоколов PF_INET и PF_INET6. Широко используются в сети Интернет, включают в себя IP-адреса, а также номера портов TCP и UDP.
#include <netinet/in.h>
//Функции для работы с числовыми IP-адресами.
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3491"
#define BACKLOG 10

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// получаем адрес сокета, ipv4 или ipv6:
void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    //сокет через который будет работать сервер (слушает)
    int sockfd, new_fd; //новый сокет для взаимодействия с клиентами

    //структура заполняемая минимум инфой о подключениях
    //servinfo - это связанный список со всеми типами информации об адресах.
    //p - нода в связном списке
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; //info about adress client (4//6 - ип)

    socklen_t sin_size; //длина переменной равно длине сокета

    struct sigaction sa; //отвечает за подчисткой зомби-процессов, которые возникают после того, как дочерний (fork ()) процесс завершает работу

    int yes=1;
    char s[INET6_ADDRSTRLEN];//адрес клиента
    int rv;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo) != 0))
    {
        fprintf(stderr, "getaddrinfo: %sn", gai_strerror(rv));
        return 1;
    }
    // цикл через все результаты, чтобы забиндиться на первом возможном
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, 0)) == -1)
        {
            perror("server: socket");
            continue;
        }
        //установка флагов на сокете
        //int s, int level, int optname, const void *optval, socklen_t optlen
        //SOL_SOCKET - соединение обрабатываптся самим сокетом
        //SO_REUSEADDR - после соединения серв останавливается и запускает дочерний процесс, на том же порту
        //поэтому после этого вызов bind завкершится с ошибкой
        //SO_REUSEADDR - параметр сокета, выполняет bind успешно
        //optval optlen - для лоступа к значениям флага
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1); //завершает работу программы (стд либ)
        }
        //сокет будет сидеть по адресу и ющать протокол который описан в структуре
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);//если ошибка, то закрываю сокет
            perror("server: bind");
            continue;
        }
        break;
    }
    if(p == NULL)
    {
        fprintf(stderr, "server: failed to bindn");
        return 2;
    }

    freeaddrinfo(servinfo); // всё, что можно, с этой структурой мы сделали
    //слушаю сокет, устанавливаю очередь макс ожиданий
    if(listen(sockfd, BACKLOG) == -1)
    {
        perror("listen\n");
        exit(1);
    }
    //обработчик ошибок
    sa.sa_handler = sigchld_handler;
    // инициализирует набор сигналов, указанный в set, и "очищает" его от всех сигналов. 
    //sa_mask задает маску сигналов, которые должны блокироваться при обработке сигнала. Также будет блокироваться и сигнал, вызвавший запуск функции, если только не были использованы флаги SA_NODEFER или SA_NOMASK. 
    sigemptyset(&sa.sa_mask);
    //некоторым системным вызовам работать, в то время как идет обработка сигналов. 
    sa.sa_flags = SA_RESTART;
    //sigaction SIGCHLD - заполняют по дефолту поля структуры са
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction\n");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // главный цикл accept ()
        sin_size = sizeof their_addr;
        //sockaddr - сокет для работы с протоколами ИП
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,               //???
        get_in_addr((struct sockaddr *)&their_addr),  //???
        s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // тут начинается дочерний процесс
            close(sockfd); // дочернему процессу не нужен слушающий сокет
            if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
                perror("send\n");
            close(new_fd);
            exit(0);
        }
            close(new_fd);  // а этот сокет больше не нужен родителю

        return 0;
    }
    
    
}